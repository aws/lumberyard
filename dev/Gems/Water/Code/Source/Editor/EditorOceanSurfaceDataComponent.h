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
#include <Source/OceanSurfaceDataComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Water
{
    class EditorOceanSurfaceDataComponent
        : public LmbrCentral::EditorWrappedComponentBase<OceanSurfaceDataComponent, OceanSurfaceDataConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<OceanSurfaceDataComponent, OceanSurfaceDataConfig>;
        AZ_EDITOR_COMPONENT(EditorOceanSurfaceDataComponent, "{A8D2485F-F24F-4C3F-9A33-CA9783A9A359}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Surface Data";
        static constexpr const char* const s_componentName = "Ocean Surface Tag Emitter";
        static constexpr const char* const s_componentDescription = "Enables an ocean to emit surface tags";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/ocean-surface-tag-emitter";
    };
}