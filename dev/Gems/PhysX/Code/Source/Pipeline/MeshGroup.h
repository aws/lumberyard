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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/Rtti.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    namespace Pipeline
    {
        class MeshGroup
            : public AZ::SceneAPI::DataTypes::ISceneNodeGroup
        {
        public:
            AZ_RTTI(MeshGroup, "{5B03C8E6-8CEE-4DA0-A7FA-CD88689DD45B}", AZ::SceneAPI::DataTypes::ISceneNodeGroup);
            AZ_CLASS_ALLOCATOR_DECL

            MeshGroup();
            ~MeshGroup() override = default;

            const AZStd::string& GetName() const override;
            void SetName(const AZStd::string& name);
            void SetName(AZStd::string&& name);
            const AZ::Uuid& GetId() const override;
            void OverrideId(const AZ::Uuid& id);

            AZ::SceneAPI::Containers::RuleContainer& GetRuleContainer() override;
            const AZ::SceneAPI::Containers::RuleContainer& GetRuleContainerConst() const override;

            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

            static void Reflect(AZ::ReflectContext* context);

            // Shared params
            bool GetExportAsConvex() const;
            bool GetExportAsTriMesh() const;
            bool GetBuildGPUData() const;

            // Convex only params
            float GetAreaTestEpsilon() const;
            float GetPlaneTolerance() const;
            bool GetUse16bitIndices() const;
            bool GetCheckZeroAreaTriangles() const;
            bool GetQuantizeInput() const;
            bool GetUsePlaneShifting() const;
            bool GetShiftVertices() const;
            AZ::u32 GetGaussMapLimit() const;

            // TriMesh only params
            bool GetWeldVertices() const;
            void SetWeldVertices(bool weldVertices);
            bool GetDisableCleanMesh() const;
            bool GetForce32BitIndices() const;
            bool GetSuppressTriangleMeshRemapTable() const;
            bool GetBuildTriangleAdjacencies() const;
            float GetMeshWeldTolerance() const;
            void SetMeshWeldTolerance(float weldTolerance);
            AZ::u32 GetNumTrisPerLeaf() const;

        protected:
            static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            AZ::SceneAPI::SceneData::SceneNodeSelectionList m_nodeSelectionList;
            AZ::SceneAPI::Containers::RuleContainer m_rules;
            AZStd::string m_name;
            AZ::Uuid m_id;
            bool m_exportAsConvex = false;

            // Convex mesh parameters
            float m_areaTestEpsilon;
            float m_planeTolerance;
            bool m_use16bitIndices;
            bool m_checkZeroAreaTriangles;
            bool m_quantizeInput;
            bool m_usePlaneShifting;
            bool m_shiftVertices;
            AZ::u32 m_gaussMapLimit;

            // TriMesh parameters
            bool m_weldVertices;
            bool m_disableCleanMesh;
            bool m_force32BitIndices;
            bool m_suppressTriangleMeshRemapTable;
            bool m_buildTriangleAdjacencies;
            float m_meshWeldTolerance;
            AZ::u32 m_numTrisPerLeaf;

            //Shared
            bool m_buildGPUData = false;
        };
    }
}