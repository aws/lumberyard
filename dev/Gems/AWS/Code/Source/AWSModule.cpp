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

#include "StdAfx.h"

#include <AzCore/Module/Module.h>

#include <platform_impl.h>
#include <IGem.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

namespace AWS
{
    class AWSModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(AWSModule, "{1A640C15-F25C-4C8C-9E01-FABF346FBE72}", CryHooksModule);

    protected:
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
                RegisterExternalFlowNodes();
                break;
            }
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(AWS_0945e21b7ae848ac80b4ec1f34c459cc, AWS::AWSModule)
