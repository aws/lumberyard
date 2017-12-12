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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IActorGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class ActorGroup
                : public DataTypes::IActorGroup
            {
            public:
                AZ_RTTI(ActorGroup, "{D1AC3803-8282-46C5-8610-93CD39B0F843}", DataTypes::IActorGroup);
                AZ_CLASS_ALLOCATOR_DECL

                ActorGroup();
                ~ActorGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);

                Containers::RuleContainer& GetRuleContainer() override;
                const Containers::RuleContainer& GetRuleContainerConst() const;

                // ISceneNodeGroup overrides
                DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                // IActorGroup overrides
                const AZStd::string& GetSelectedRootBone() const override;
                bool GetLoadMorphTargets() const;
                bool GetAutoCreateTrajectoryNode() const;

                void SetSelectedRootBone(const AZStd::string& selectedRootBone)  override;
                void SetLoadMorphTargets(bool loadMorphTargets);
                void SetAutoCreateTrajectoryNode(bool autoCreateTrajectoryNode);

                static void Reflect(ReflectContext* context);

            protected:
                SceneNodeSelectionList      m_nodeSelectionList;
                Containers::RuleContainer   m_rules;
                AZStd::string               m_name;
                AZStd::string               m_selectedRootBone;
                bool                        m_loadMorphTargets;
                bool                        m_autoCreateTrajectoryNode;
            };
        }
    }
}
#endif //MOTIONCANVAS_GEM_ENABLED