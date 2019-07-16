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
#include <RoadSurfaceDataComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace RoadsAndRivers
{
    class EditorRoadSurfaceDataComponent
        : public LmbrCentral::EditorWrappedComponentBase<RoadSurfaceDataComponent, RoadSurfaceDataConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<RoadSurfaceDataComponent, RoadSurfaceDataConfig>;
        AZ_EDITOR_COMPONENT(EditorRoadSurfaceDataComponent, "{0DEF3E2A-5D70-4493-BEBB-FCD62D44D79F}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Surface Data";
        static constexpr const char* const s_componentName = "Road Surface Tag Emitter";
        static constexpr const char* const s_componentDescription = "Enables a road to emit surface tags";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/road-surface-tag-emitter";
    };
}