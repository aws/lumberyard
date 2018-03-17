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

#include <Cry_Matrix34.h>
#include <AzCore/Math/Transform.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    class Transform;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IOriginRule;
            class IGroup;
        }
    }

    namespace RC
    {
        struct ContainerExportContext;
        struct NodeExportContext;

        class WorldMatrixExporter
            : public SceneAPI::SceneCore::RCExportingComponent
        {
        public:
            AZ_COMPONENT(WorldMatrixExporter, "{65A0914C-5953-405F-819B-0E6EB96938F1}", SceneAPI::SceneCore::RCExportingComponent);

            WorldMatrixExporter();
            ~WorldMatrixExporter() override = default;

            static void Reflect(ReflectContext* context);

            SceneAPI::Events::ProcessingResult ProcessMeshGroup(ContainerExportContext& context);
            SceneAPI::Events::ProcessingResult ProcessNode(NodeExportContext& context);

        protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
            // Workaround for VS2013 - Delete the copy constructor and make it private
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            WorldMatrixExporter(const WorldMatrixExporter&) = delete;
#endif
            using HierarchyStorageIterator = SceneAPI::Containers::SceneGraph::HierarchyStorageConstIterator;

            bool ConcatenateMatricesUpwards(Transform& transform, const HierarchyStorageIterator& nodeIterator, const SceneAPI::Containers::SceneGraph& graph) const;
            bool MultiplyEndPointTransforms(Transform& transform, const HierarchyStorageIterator& nodeIterator, const SceneAPI::Containers::SceneGraph& graph) const;
            void TransformToMatrix34(Matrix34& out, const Transform& in) const;

            Transform m_cachedRootMatrix;
            const SceneAPI::DataTypes::IGroup* m_cachedGroup;
            bool m_cachedRootMatrixIsSet;
        };
    } // namespace RC
} // namespace AZ