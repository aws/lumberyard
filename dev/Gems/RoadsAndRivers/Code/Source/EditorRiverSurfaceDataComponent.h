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

#include <AzCore/Module/Module.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <RiverSurfaceDataComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace RoadsAndRivers
{
    class EditorRiverSurfaceDataComponent
        : public LmbrCentral::EditorWrappedComponentBase<RiverSurfaceDataComponent, RiverSurfaceDataConfig>
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<RiverSurfaceDataComponent, RiverSurfaceDataConfig>;
        AZ_EDITOR_COMPONENT(EditorRiverSurfaceDataComponent, "{70A549D4-607A-413F-A170-B8B84852B59B}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus implementation
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        static constexpr const char* const s_categoryName = "Surface Data";
        static constexpr const char* const s_componentName = "River Surface Tag Emitter";
        static constexpr const char* const s_componentDescription = "Enables a river to emit surface tags";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/river-surface-tag-emitter";

    private:
        void ViewSurfaceVolumeChanged();

        bool m_viewSurfaceVolume = false;
    };
}