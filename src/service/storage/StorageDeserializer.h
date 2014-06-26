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
 * @file        StorageDeserializer.h
 * @author      Aleksander Zdyb <a.zdyb@partner.samsung.com>
 * @version     1.0
 * @brief       Headers for Cynara::StorageDeserializer
 */
#ifndef SRC_SERVICE_STORAGE_STORAGEDESERIALIZER_H_
#define SRC_SERVICE_STORAGE_STORAGEDESERIALIZER_H_

#include <types/PolicyCollection.h>
#include <types/PolicyKey.h>
#include <types/PolicyResult.h>
#include <types/PolicyType.h>

#include <istream>

namespace Cynara {

class StorageDeserializer {
public:
    static PolicyCollection loadPolicies(std::istream &is);

protected:
    static PolicyKey parseKey(const std::string &line, std::size_t &beginToken);
    static PolicyType parsePolicyType(const std::string &line, std::size_t &beginToken);
    static PolicyResult::PolicyMetadata parseMetadata(const std::string &line,
                                                      std::size_t &beginToken);
};

} /* namespace Cynara */

#endif /* SRC_SERVICE_STORAGE_STORAGEDESERIALIZER_H_ */