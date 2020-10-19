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

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IOriginRule.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/WorldMatrixExporter.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;

        WorldMatrixExporter::WorldMatrixExporter()
            : m_cachedRootMatrix(Transform::CreateIdentity())
            , m_cachedGroup(nullptr)
            , m_cachedRootMatrixIsSet(false)
        {
            BindToCall(&WorldMatrixExporter::ProcessMeshGroup);
            BindToCall(&WorldMatrixExporter::ProcessNode);
        }

        void WorldMatrixExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<WorldMatrixExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult WorldMatrixExporter::ProcessMeshGroup(ContainerExportContext& context)
        {
            if (context.m_phase != Phase::Construction)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            m_cachedGroup = &context.m_group;

            m_cachedRootMatrix = Transform::CreateIdentity();
            m_cachedRootMatrixIsSet = false;

            AZStd::shared_ptr<const SceneDataTypes::IOriginRule> rule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IOriginRule>();
            if (rule)
            {
                m_cachedRootMatrix = SceneAPI::Utilities::GetOriginRuleTransform(*rule, context.m_scene.GetGraph());
                m_cachedRootMatrixIsSet = !m_cachedRootMatrix.IsClose(Transform::CreateIdentity());
                            
                return m_cachedRootMatrixIsSet ? SceneEvents::ProcessingResult::Success : SceneEvents::ProcessingResult::Ignored;
            }
            else
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
        }

        SceneEvents::ProcessingResult WorldMatrixExporter::ProcessNode(NodeExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            Transform worldMatrix = Transform::CreateIdentity();

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            SceneAPI::Containers::SceneGraph::HierarchyStorageConstIterator nodeIterator = graph.ConvertToHierarchyIterator(context.m_nodeIndex);
            bool translated = SceneAPI::Utilities::ConcatenateMatricesUpwards(worldMatrix, nodeIterator, graph);

            AZ_Assert(m_cachedGroup == &context.m_group, "NodeExportContext doesn't belong to chain of previously called MeshGroupExportContext.");
            if (m_cachedRootMatrixIsSet)
            {
                worldMatrix = m_cachedRootMatrix * worldMatrix;
                translated = true;
            }
 
            //If we aren't merging nodes we need to put the transforms into the localTM
            //due to how the CGFSaver works inside the ResourceCompilerPC code. 
            if (!context.m_container.GetExportInfo()->bMergeAllNodes)
            {
                TransformToMatrix34(context.m_node.localTM, worldMatrix);
            }
            else
            {
                TransformToMatrix34(context.m_node.worldTM, worldMatrix);
            }
            context.m_node.bIdentityMatrix = !translated;

            return SceneEvents::ProcessingResult::Success;
        }

        void WorldMatrixExporter::TransformToMatrix34(Matrix34& out, const Transform& in) const
        {
            // Setting column instead of row because as of writing Matrix34 doesn't support adding
            //      full rows, as the translation has to be done separately.
            for (int column = 0; column < 4; ++column)
            {
                Vector3 data = in.GetColumn(column);
                out.SetColumn(column, Vec3(data.GetX(), data.GetY(), data.GetZ()));
            }
        }
    } // namespace RC
} // namespace AZ
