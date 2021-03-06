/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
/**
 * @file        src/cyad/BaseErrorApiWrapper.h
 * @author      Aleksander Zdyb <a.zdyb@samsung.com>
 * @version     1.0
 * @brief       Wrapper around cynara-error API (base)
 */

#ifndef SRC_CYAD_BASEERRORAPIWRAPPER_H_
#define SRC_CYAD_BASEERRORAPIWRAPPER_H_

#include <cstddef>

namespace Cynara {

class BaseErrorApiWrapper {
public:
    virtual ~BaseErrorApiWrapper() {};
    virtual int cynara_strerror(int errnum, char *buf, size_t buflen) = 0;
};

} /* namespace Cynara */

#endif /* SRC_CYAD_BASEERRORAPIWRAPPER_H_ */
