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

#include "ForceVolumeComponent.h"
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace LmbrCentral
{
    /**
     * Editor component for the ForceVolumeComponent
     */
    class EditorForceVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorForceVolumeComponent, "{7E7373AA-9808-4DF8-A9B3-7C7EBD6BCFD5}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorForceVolumeComponent() = default;
        explicit EditorForceVolumeComponent(const ForceVolumeConfiguration& configuration);
        ~EditorForceVolumeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }
        
        void DisplayEntity(bool& handled) override;

        ForceVolumeConfiguration m_configuration;
    };
} // namespace LmbrCentral