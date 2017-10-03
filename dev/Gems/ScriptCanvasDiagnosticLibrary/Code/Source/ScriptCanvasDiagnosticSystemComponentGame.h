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

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvasDiagnostics
{
    class SystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SystemComponent, "{6A90B0E7-EB47-48B5-910D-4881E429AC9D}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<SystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<SystemComponent>("Script Canvas Diagnostic", "Script Canvas Diagnostic System Component")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        SystemComponent() = default;
        ~SystemComponent() override = default;

        void Activate() override {};
        void Deactivate() override {};
    };
}