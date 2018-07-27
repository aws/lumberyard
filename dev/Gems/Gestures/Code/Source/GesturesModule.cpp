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

#include "Gestures_precompiled.h"
#include <platform_impl.h>
#include <IGem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>

namespace Gestures
{
    class GesturesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(GesturesModule, "{5648A92C-04A3-4E30-B4E2-B0AEB280CA44}", AZ::Module);

        GesturesModule()
            : CryHooksModule()
        {
        }

        ~GesturesModule() override = default;

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
            {
                RegisterExternalFlowNodes();
            }
            break;
            }
        }
    };
}

AZ_DECLARE_MODULE_CLASS(Gestures_6056556b6088413984309c4a413593ad, Gestures::GesturesModule)
