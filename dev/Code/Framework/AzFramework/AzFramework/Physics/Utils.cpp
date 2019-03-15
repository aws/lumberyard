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


#include "Utils.h"
#include "RigidBody.h"
#include "World.h"
#include "Material.h"
#include "Shape.h"
#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/EditContext.h>

namespace Physics
{
    namespace ReflectionUtils
    {
        void ReflectWorldBus(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<Physics::WorldRequestBus>("WorldRequestBus")
                    ->Event("GetGravity", &Physics::WorldRequestBus::Events::GetGravity)
                    ->Event("SetGravity", &Physics::WorldRequestBus::Events::SetGravity)
                    ;
            }
        }

        void ReflectPhysicsApi(AZ::ReflectContext* context)
        {
            ShapeConfiguration::Reflect(context);
            ColliderConfiguration::Reflect(context);
            BoxShapeConfiguration::Reflect(context);
            CapsuleShapeConfiguration::Reflect(context);
            SphereShapeConfiguration::Reflect(context);
            PhysicsAssetShapeConfiguration::Reflect(context);
            NativeShapeConfiguration::Reflect(context);
            CollisionLayer::Reflect(context);
            CollisionGroup::Reflect(context);
            CollisionLayers::Reflect(context);
            CollisionGroups::Reflect(context);
            WorldConfiguration::Reflect(context);
            MaterialConfiguration::Reflect(context);
            MaterialLibraryAsset::Reflect(context);
            JointLimitConfiguration::Reflect(context);

            WorldBodyConfiguration::Reflect(context);
            RigidBodyConfiguration::Reflect(context);

            RagdollNodeConfiguration::Reflect(context);
            RagdollConfiguration::Reflect(context);

            CharacterColliderNodeConfiguration::Reflect(context);
            CharacterColliderConfiguration::Reflect(context);
            AnimationConfiguration::Reflect(context);
            CharacterConfiguration::Reflect(context);
            ReflectWorldBus(context);
        }
    }

    namespace Utils
    {
        void MakeUniqueString(const AZStd::unordered_set<AZStd::string>& stringSet
            , AZStd::string& stringInOut
            , AZ::u64 maxStringLength)
        {
            AZStd::string originalString = stringInOut;
            for (size_t nameIndex = 1; nameIndex <= stringSet.size(); ++nameIndex)
            {
                AZStd::string postFix;
                to_string(postFix, nameIndex);
                postFix = "_" + postFix;

                AZ::u64 trimLength = (originalString.length() + postFix.length()) - maxStringLength;
                if (trimLength > 0)
                {
                    stringInOut = originalString.substr(0, originalString.length() - trimLength) + postFix;
                }
                else
                {
                    stringInOut = originalString + postFix;
                }

                if (stringSet.find(stringInOut) == stringSet.end())
                {
                    break;
                }
            }
        }
    }
}