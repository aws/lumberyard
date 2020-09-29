#pragma once

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

#include <AzCore/Math/Transform.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IOriginRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Utilities
        {
            //! Searches for any entries in the scene graph that are derived or match the given type.
            //!      If "checkVirtualTypes" is true, a matching entry is also checked if it's not a
            //!      virtual type.
            template<typename T>
            bool DoesSceneGraphContainDataLike(const Containers::Scene& scene, bool checkVirtualTypes);
            SCENE_CORE_API AZ::Transform BuildWorldTransform(
                const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex);

            //! Accumulates transform upwards in a given graph from a given node.
            //! Original value of the given transform is used in the calculations.
            //! Returns true if it encountered at least one transform node during traversal.
            SCENE_CORE_API bool ConcatenateMatricesUpwards(
                Transform& transform,
                const Containers::SceneGraph::HierarchyStorageConstIterator& nodeIterator,
                const SceneAPI::Containers::SceneGraph& graph);

            //! Multiplies given transform by a child transform of a given node.
            //! Returns true if there is a child transform of a given node.
            SCENE_CORE_API bool MultiplyEndPointTransforms(
                Transform& transform,
                const Containers::SceneGraph::HierarchyStorageConstIterator& nodeIterator,
                const SceneAPI::Containers::SceneGraph& graph);

            //! Calculates transform of the rule. This transform can be applied to a world transform of a node to produce
            //! a transform that will be correctly offset in regard to the rule.
            SCENE_CORE_API AZ::Transform GetOriginRuleTransform(
                const DataTypes::IOriginRule& originRule, const Containers::SceneGraph& graph);
        } // Utilities
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.inl>
