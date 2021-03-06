/*
 * Copyright (c) 2014-2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @file        src/cyad/CommandsDispatcher.h
 * @author      Aleksander Zdyb <a.zdyb@samsung.com>
 * @version     1.0
 * @brief       CommandsDispatcher class
 */

#ifndef SRC_CYAD_COMMANDSDISPATCHER_H_
#define SRC_CYAD_COMMANDSDISPATCHER_H_

#include <cyad/BaseAdminApiWrapper.h>
#include <cyad/BaseErrorApiWrapper.h>
#include <cyad/CommandlineParser/CyadCommand.h>
#include <cyad/DispatcherIO.h>
#include <cyad/PolicyTypeTranslator.h>

struct cynara_admin;
struct cynara_admin_policy;

namespace Cynara {

class CommandsDispatcher {
public:
    CommandsDispatcher(BaseDispatcherIO &io, BaseAdminApiWrapper &adminApiWrapper,
                       BaseErrorApiWrapper &errorApiWrapper);
    virtual ~CommandsDispatcher();

    virtual int execute(CyadCommand &);
    virtual int execute(HelpCyadCommand &);
    virtual int execute(ErrorCyadCommand &);
    virtual int execute(DeleteBucketCyadCommand &);
    virtual int execute(SetBucketCyadCommand &);
    virtual int execute(SetPolicyCyadCommand &);
    virtual int execute(SetPolicyBulkCyadCommand &);
    virtual int execute(EraseCyadCommand &);
    virtual int execute(CheckCyadCommand &);
    virtual int execute(ListPoliciesCyadCommand &);
    virtual int execute(ListPoliciesDescCyadCommand &);

protected:
    void printAdminApiError(int errnum);
    void initPolicyTranslator(void);

private:
    PolicyTypeTranslator m_policyTranslator;
    BaseDispatcherIO &m_io;
    BaseAdminApiWrapper &m_adminApiWrapper;
    BaseErrorApiWrapper &m_errorApiWrapper;
    struct cynara_admin *m_cynaraAdmin;
};

} /* namespace Cynara */

#endif /* SRC_CYAD_COMMANDSDISPATCHER_H_ */
