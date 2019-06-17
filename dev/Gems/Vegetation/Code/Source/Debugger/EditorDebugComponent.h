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

#include "DebugComponent.h"
#include <AzCore/Component/TickBus.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDebugComponent
        : public EditorVegetationComponentBase<DebugComponent, DebugConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DebugComponent, DebugConfig>;
        AZ_EDITOR_COMPONENT(EditorDebugComponent, "{BE98DFCB-6890-4E87-920B-067B2D853538}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Debugger";
        static constexpr const char* const s_componentDescription = "";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-debugger";

    protected:
        void OnDumpDataToFile();
        void OnClearReport();
        void OnRefreshAllAreas();
        void OnClearAllAreas();

    private:

        class MergedMeshDebug final
        {
        public:
            AZ_CLASS_ALLOCATOR(MergedMeshDebug, AZ::SystemAllocator, 0);
            AZ_RTTI(MergedMeshDebug, "{2F6089E5-D8C9-47BB-A9CB-9A5C14A10F31}");
            static void Reflect(AZ::ReflectContext* context);

            ~MergedMeshDebug();

            void StoreCurrentDebugValue();

            void OnChangedValues();

        private:
            // Flags are duplicated from MergedMeshRenderNode.cpp: namespace MergedMeshDebug{ enum DisplayFlags{...} }
            enum class MergedMeshDebugFlags
            {
                StressInfo = (1 << 1), // sector stress information
                PrintState = (1 << 2), // debug info (position, state, size, visibility)
                InstanceLines = (1 << 3), // draw instances as lines
                InstanceAABBs = (1 << 4), // draw instances AABBs
                SpinesFilter = (1 << 5), // draw spines filter
                CalculatedWind = (1 << 6), // Show the calculated wind
                InfluencingColliders = (1 << 8), // Draw colliders of objects influencing the merged meshes
                SpinesAsLines = (1 << 9),// draw spines as lines
                SimulatedSpines = (1 << 10), // draw simulated spine
                SpinesLOD = (1 << 11), // draw spines with LOD info (red/blue)
            };

            bool m_stressInfo = false;
            bool m_printState = false;
            bool m_instanceLines = false;
            bool m_instanceAABBs = false;
            bool m_spinesFilter = false;
            bool m_calculatedWind = false;
            bool m_influencingColliders = false;
            bool m_spinesAsLines = false;       
            bool m_simulatedSpines = false;
            bool m_spinesLOD = false;

            int32_t m_storedMergedMeshDebugValue;
        };

        DebugRequests::PerformanceReport m_report;
        MergedMeshDebug m_mergedMeshDebug;
    };
}