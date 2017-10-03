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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/MeshRule.h>
#include <SceneAPIExt/Behaviors/MeshRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void MeshRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                Rule::MeshRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshRuleBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void MeshRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            }

            void MeshRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MeshRuleBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(Group::IActorGroup::TYPEINFO_Uuid()))
                {
                    Group::IActorGroup* actorGroup = azrtti_cast<Group::IActorGroup*>(&target);

                    // TODO: Check if the sceneGraph contain mesh data.
                    AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup->GetRuleContainer();
                    if (!rules.ContainsRuleOfType<Rule::IMeshRule>())
                    {
                        rules.AddRule(AZStd::make_shared<Rule::MeshRule>());
                    }
                }
            }
        } // Behavior
    } // Pipeline
} // EMotionFX