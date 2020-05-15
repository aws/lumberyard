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

#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/Module/Module.h>

#include <LyzardSDK/cli.h>

namespace Lyzard
{
    class Module
    {
    public:
        AZ_RTTI(Module, "{9C4A5BD5-11AE-4D27-B6B7-A9B73013D1BB}");

        virtual void RegisterCliCommands(AZStd::shared_ptr<CLI::Namespace> /*lmbr*/) { }
    };
}
