/*
 *  Copyright (c) 2014-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */
/**
 * @file        src/helpers/creds-commons/creds-commons.cpp
 * @author      Lukasz Wojciechowski <l.wojciechow@partner.samsung.com>
 * @author      Radoslaw Bartosiak <r.bartosiak@samsung.com>
 * @author      Aleksander Zdyb <a.zdyb@samsung.com>
 * @author      Jacek Bukarewicz <j.bukarewicz@samsung.com>
 * @version     1.1
 * @brief       Implementation of external libcynara-creds-commons API
 */

#include <fstream>

#include <attributes/attributes.h>
#include <exceptions/TryCatch.h>

#include <cynara-creds-commons.h>
#include <cynara-error.h>

#include "CredsCommonsInner.h"

CYNARA_API
int cynara_creds_get_default_client_method(enum cynara_client_creds *method) {
    static int cachedMethodCode = -1;
    static const Cynara::CredentialsMap clientCredsMap{{"smack", CLIENT_METHOD_SMACK},
                                                       {"pid", CLIENT_METHOD_PID}};

    if (cachedMethodCode == -1) {
        int ret = Cynara::tryCatch([&] () {
            std::ifstream f;
            int r = Cynara::CredsCommonsInnerBackend::credsConfigurationFile(f);
            if (r != CYNARA_API_SUCCESS)
                return r;

            return Cynara::CredsCommonsInnerBackend::getMethodFromConfigurationFile(
                     f, clientCredsMap, "client_default", cachedMethodCode);
        });
        if (ret != CYNARA_API_SUCCESS)
            return ret;
    }
    *method = static_cast<enum cynara_client_creds>(cachedMethodCode);
    return CYNARA_API_SUCCESS;

}

CYNARA_API
int cynara_creds_get_default_user_method(enum cynara_user_creds *method) {
    static int cachedMethodCode = -1;
    static const Cynara::CredentialsMap userCredsMap{{"uid", USER_METHOD_UID},
                                                     {"gid", USER_METHOD_GID}};

    if (cachedMethodCode == -1) {
        int ret = Cynara::tryCatch([&] () {
            std::ifstream f;
            int r = Cynara::CredsCommonsInnerBackend::credsConfigurationFile(f);
            if (r != CYNARA_API_SUCCESS)
                return r;
            return Cynara::CredsCommonsInnerBackend::getMethodFromConfigurationFile(
                     f, userCredsMap, "user_default", cachedMethodCode);
        });
        if (ret != CYNARA_API_SUCCESS)
            return ret;
    }

    *method = static_cast<enum cynara_user_creds>(cachedMethodCode);
    return CYNARA_API_SUCCESS;
}
