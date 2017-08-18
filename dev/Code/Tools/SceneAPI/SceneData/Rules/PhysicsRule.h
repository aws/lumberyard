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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IPhysicsRule.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class PhysicsRule
                : public DataTypes::IPhysicsRule
            {
            public:
                AZ_RTTI(PhysicsRule, "{845CCDDB-E7D3-4423-AF28-D0C4AFDF02A8}", DataTypes::IPhysicsRule);
                AZ_CLASS_ALLOCATOR_DECL

                ~PhysicsRule() override = default;

                SceneNodeSelectionList& GetNodeSelectionList();

                DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                static void Reflect(ReflectContext* context);

            protected:
                SceneNodeSelectionList m_nodeSelectionList;
            };
        } // SceneData
    } // SceneAPI
} // AZ
