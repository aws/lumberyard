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

#include <Components/StaticVegetationBlockerComponent.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorStaticVegetationBlockerComponent :
        public EditorVegetationComponentBase<StaticVegetationBlockerComponent, StaticVegetationBlockerConfig>,
        private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<StaticVegetationBlockerComponent, StaticVegetationBlockerConfig>;
        AZ_EDITOR_COMPONENT(EditorStaticVegetationBlockerComponent, EditorStaticVegetationBlockerComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Blocker (Static)";
        static constexpr const char* const s_componentDescription = "Prevents dynamic vegetation from being placed on top of static vegetation";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-layer-blocker-static";

    private:
        void DrawBlockedClaimPoints(AZ::Vector2& cameraPos2d, AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawVegetationBoundingBoxes(const StaticVegetationMap* vegetationList, AZ::Vector2& cameraPos2d, AzFramework::DebugDisplayRequests& debugDisplay);
    };
}
