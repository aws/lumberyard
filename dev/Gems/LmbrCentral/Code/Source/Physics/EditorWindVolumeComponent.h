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

#include "WindVolumeComponent.h"
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace LmbrCentral
{
    /**
     * In-editor WindVolume component for a wind volume.
     */
    class EditorWindVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorWindVolumeComponent, "{61E5864D-F553-4A37-9A03-B9F836F1D3DC}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorWindVolumeComponent() = default;
        explicit EditorWindVolumeComponent(const WindVolumeConfiguration& configuration);
        ~EditorWindVolumeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        void Activate() override;
        void Deactivate() override;

    private:
        AZ::Vector3 GetLocalWindDirection(const AZ::Vector3& point) const;
        void DisplayEntity(bool& handled) override;

        WindVolumeConfiguration m_configuration;
    };
} // namespace LmbrCentral