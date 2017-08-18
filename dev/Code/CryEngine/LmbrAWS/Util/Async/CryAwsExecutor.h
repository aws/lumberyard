/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#pragma warning(push)
// Core_EXPORTS.h disables 4251, but #pragma once is preventing it from being included in this block
#pragma warning(disable: 4251) // C4251: needs to have dll-interface to be used by clients of class
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/core/utils/threading/Executor.h>
#pragma warning(pop)

#include "IThreadTask.h"

#include <memory>

namespace LmbrAWS
{
    class AwsRequestProcessorTask;

    class CryAwsExecutor
        : public Aws::Utils::Threading::Executor
    {
    public:

        CryAwsExecutor();
        virtual ~CryAwsExecutor();

    protected:

        virtual bool SubmitToThread(std::function<void()>&& task) override;

    private:

        std::unique_ptr<AwsRequestProcessorTask> m_processingTask;
    };
} //namescape LmbrAWS