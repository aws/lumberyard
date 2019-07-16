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
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Source/WaterVolumeSurfaceDataComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Water
{
    class EditorWaterVolumeSurfaceDataComponent
        : public LmbrCentral::EditorWrappedComponentBase<WaterVolumeSurfaceDataComponent, WaterVolumeSurfaceDataConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<WaterVolumeSurfaceDataComponent, WaterVolumeSurfaceDataConfig>;
        AZ_EDITOR_COMPONENT(EditorWaterVolumeSurfaceDataComponent, "{B2427FCE-CD7A-4311-9C3F-61AADA10A0DA}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Surface Data";
        static constexpr const char* const s_componentName = "Water Volume Surface Tag Emitter";
        static constexpr const char* const s_componentDescription = "Enables a water volume to emit surface tags";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/water-volume-surface-tag-emitter";
    };
}