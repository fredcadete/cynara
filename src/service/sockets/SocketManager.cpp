/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/*
 * @file        SocketManager.cpp
 * @author      Lukasz Wojciechowski <l.wojciechow@partner.samsung.com>
 * @version     1.0
 * @brief       This file implements socket layer manager for cynara
 */

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <systemd/sd-daemon.h>

#include <log/log.h>
#include <common.h>
#include <exceptions/DescriptorNotExistsException.h>
#include <exceptions/InitException.h>
#include <exceptions/NoResponseGeneratedException.h>
#include <exceptions/UnexpectedErrorException.h>

#include <logic/Logic.h>
#include <main/Cynara.h>
#include <protocol/ProtocolAdmin.h>
#include <protocol/ProtocolClient.h>
#include <protocol/ProtocolSignal.h>
#include <stdexcept>

#include "SocketManager.h"

namespace Cynara {

SocketManager::SocketManager() : m_working(false), m_maxDesc(-1) {
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
}

SocketManager::~SocketManager() {
}

void SocketManager::run(void) {
    init();
    mainLoop();
}

void SocketManager::init(void) {
    LOGI("SocketManger init start");
    const std::string clientSocketPath("/run/cynara/cynara.socket");
    const std::string adminSocketPath("/run/cynara/cynara-admin.socket");
    const mode_t clientSocketUMask(0);
    const mode_t adminSocketUMask(0077);

    createDomainSocket(ProtocolPtr(new ProtocolClient), clientSocketPath, clientSocketUMask);
    createDomainSocket(ProtocolPtr(new ProtocolAdmin), adminSocketPath, adminSocketUMask);
    // todo create signal descriptor
    LOGI("SocketManger init done");
}

void SocketManager::mainLoop(void) {
    LOGI("SocketManger mainLoop start");
    m_working  = true;
    while (m_working) {
        fd_set readSet = m_readSet;
        fd_set writeSet = m_writeSet;

        int ret = select(m_maxDesc + 1, &readSet, &writeSet, nullptr, nullptr);

        if (ret < 0) {
            switch (errno) {
            case EINTR:
                continue;
            default:
                int err = errno;
                throw UnexpectedErrorException(err, strerror(err));
            }
        } else if (ret > 0) {
            for (int i = 0; i < m_maxDesc + 1 && ret; ++i) {
                if (FD_ISSET(i, &readSet)) {
                    readyForRead(i);
                    --ret;
                }
                if (FD_ISSET(i, &writeSet)) {
                    readyForWrite(i);
                    --ret;
                }
            }
        }
    }
    LOGI("SocketManger mainLoop done");
}

void SocketManager::mainLoopStop(void) {
    m_working = false;
}

void SocketManager::readyForRead(int fd) {
    LOGD("SocketManger readyForRead on fd [%d] start", fd);
    auto &desc = m_fds[fd];
    if (desc.isListen()) {
        readyForAccept(fd);
        return;
    }

    RawBuffer readBuffer(DEFAULT_BUFFER_SIZE);
    ssize_t size = read(fd, readBuffer.data(), DEFAULT_BUFFER_SIZE);

    if (size > 0) {
        LOGD("read [%zd] bytes", size);
        readBuffer.resize(size);
        if (handleRead(fd, readBuffer)) {
            LOGD("SocketManger readyForRead on fd [%d] successfully done", fd);
            return;
        }
        LOGI("interpreting buffer read from [%d] failed", fd);
    } else if (size < 0) {
        int err = errno;
        switch (err) {
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
            case EINTR:
                return;
            default:
                LOGW("While reading from [%d] socket, error [%d]:<%s>",
                     fd, err, strerror(err));
        }
    } else {
        LOGN("Socket [%d] closed on other end", fd);
    }
    closeSocket(fd);
    LOGD("SocketManger readyForRead on fd [%d] done", fd);
}

void SocketManager::readyForWrite(int fd) {
    LOGD("SocketManger readyForWrite on fd [%d] start", fd);
    auto &desc = m_fds[fd];
    auto &buffer = desc.prepareWriteBuffer();
    size_t size = buffer.size();
    ssize_t result = write(fd, buffer.data(), size);
    if (result == -1) {
        int err = errno;
        switch (err) {
        case EAGAIN:
        case EINTR:
            // select will trigger write once again, nothing to do
            break;
        case EPIPE:
        default:
            LOGD("Error during write to fd [%d]:<%s> ", fd, strerror(err));
            closeSocket(fd);
            break;
        }
        return; // We do not want to propagate error to next layer
    }

    LOGD("written [%zd] bytes", result);
    buffer.erase(buffer.begin(), buffer.begin() + result);

    if (buffer.empty())
        removeWriteSocket(fd);
    LOGD("SocketManger readyForWrite on fd [%d] done", fd);
}

void SocketManager::readyForAccept(int fd) {
    LOGD("SocketManger readyForAccept on fd [%d] start", fd);
    struct sockaddr_un clientAddr;
    unsigned int clientLen = sizeof(clientAddr);
    int client = accept4(fd, (struct sockaddr*) &clientAddr, &clientLen, SOCK_NONBLOCK);
    if (client == -1) {
        int err = errno;
        LOGW("Error in accept on socket [%d]: <%s>", fd, strerror(err));
        return;
    }
    LOGD("Accept on sock [%d]. New client socket opened [%d]", fd, client);

    auto &description = createDescriptor(client);
    description.setListen(false);
    description.setProtocol(m_fds[fd].protocol());
    addReadSocket(client);
    LOGD("SocketManger readyForAccept on fd [%d] done", fd);
}

void SocketManager::closeSocket(int fd) {
    LOGD("SocketManger closeSocket fd [%d] start", fd);
    removeReadSocket(fd);
    removeWriteSocket(fd);
    m_fds[fd].clear();
    close(fd);
    LOGD("SocketManger closeSocket fd [%d] done", fd);
}

bool SocketManager::handleRead(int fd, const RawBuffer &readbuffer) {
    LOGD("SocketManger handleRead on fd [%d] start", fd);
    auto &desc = m_fds[fd];
    desc.pushReadBuffer(readbuffer);

    try {
        std::shared_ptr<Request> req(nullptr);
        for (;;) {
            req = desc.extractRequest();
            if (!req)
                break;

            LOGD("request extracted");
            try {
                req->execute(fd);

                if (desc.hasDataToWrite())
                    addWriteSocket(fd);
            } catch (const NoResponseGeneratedException &ex) {
                LOGD("no response generated");
            }
        }
    } catch (const Exception &ex) {
        LOGE("Error handling request <%s>. Closing socket", ex.what());
        return false;
    }
    LOGD("SocketManger handleRead on fd [%d] done", fd);
    return true;
}

void SocketManager::createDomainSocket(ProtocolPtr protocol, const std::string &path, mode_t mask) {
    int fd = getSocketFromSystemD(path);
    if (fd == -1)
        fd = createDomainSocketHelp(path, mask);

    auto &desc = createDescriptor(fd);
    desc.setListen(true);
    desc.setProtocol(protocol);
    addReadSocket(fd);

    LOGD("Domain socket: [%d] added.", fd);
}

int SocketManager::createDomainSocketHelp(const std::string &path, mode_t mask) {
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        int err = errno;
        LOGE("Error during UNIX socket creation: <%s>",  strerror(err));
        throw InitException();
    }

    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
        flags = 0;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        int err = errno;
        close(fd);
        LOGE("Error setting \"O_NONBLOCK\" on descriptor [%d] with fcntl: <%s>",
             fd, strerror(err));
        throw InitException();
    }

    sockaddr_un serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    if (path.length() > sizeof(serverAddress.sun_path)) {
        LOGE("Path for unix domain socket <%s> is [%zu] bytes long, while it should be maximum "
             "[%zu] bytes long", path.c_str(), path.length(), sizeof(serverAddress));
        throw InitException();
    }
    strcpy(serverAddress.sun_path, path.c_str());
    unlink(serverAddress.sun_path);

    mode_t originalUmask;
    originalUmask = umask(mask);

    if (bind(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        int err = errno;
        close(fd);
        LOGE("Error in bind socket descriptor [%d] to path <%s>: <%s>",
             fd, path.c_str(), strerror(err));
        throw InitException();
    }

    umask(originalUmask);

    if (listen(fd, 5) == -1) {
        int err = errno;
        close(fd);
        LOGE("Error setting listen on file descriptor [%d], path <%s>: <%s>",
             fd, path.c_str(), strerror(err));
        throw InitException();
    }

    return fd;
}

int SocketManager::getSocketFromSystemD(const std::string &path) {
    int n = sd_listen_fds(0);
    LOGI("sd_listen_fds returns: [%d]", n);
    if (n < 0) {
        LOGE("Error in sd_listend_fds");
        throw InitException();
    }

    for (int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START + n; ++fd) {
        if (sd_is_socket_unix(fd, SOCK_STREAM, 1, path.c_str(), 0) > 0) {
            LOGI("Useable socket <%s> was passed by SystemD under descriptor [%d]",
                    path.c_str(), fd);
            return fd;
        }
    }
    LOGI("No useable sockets were passed by systemd.");
    return -1;
}

Descriptor &SocketManager::createDescriptor(int fd) {
    if (fd > m_maxDesc) {
        m_maxDesc = fd;
        if (fd >= static_cast<int>(m_fds.size()))
            m_fds.resize(fd + 20);
    }
    auto &desc = m_fds[fd];
    desc.setUsed(true);
    return desc;
}

void SocketManager::addReadSocket(int fd) {
    FD_SET(fd, &m_readSet);
}

void SocketManager::removeReadSocket(int fd) {
    FD_CLR(fd, &m_readSet);
}

void SocketManager::addWriteSocket(int fd) {
    FD_SET(fd, &m_writeSet);
}

void SocketManager::removeWriteSocket(int fd) {
    FD_CLR(fd, &m_writeSet);
}

Descriptor &SocketManager::descriptor(int fd) {
    try {
        auto &desc = m_fds.at(fd);
        if (!desc.isUsed())
            throw DescriptorNotExistsException(fd);
        return desc;
    } catch (const std::out_of_range &e) {
        throw DescriptorNotExistsException(fd);
    }
}

} // namespace Cynara
