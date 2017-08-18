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

#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/OriginRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/PhysicsRule.h>
#include <SceneAPI/SceneData/Rules/SkeletonProxyRule.h>

#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

#if defined(MOTIONCANVAS_GEM_ENABLED)
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>
#include <SceneAPI/SceneData/Groups/ActorGroup.h>
#include <SceneAPI/SceneData/Groups/EFXMotionGroup.h>
#include <SceneAPI/SceneData/Rules/EFXMeshRule.h>
#include <SceneAPI/SceneData/Rules/EFXSkinRule.h>
#include <SceneAPI/SceneData/Rules/EFXMotionCompressionSettingsRule.h>
#include <SceneAPI/SceneData/Rules/EFXMotionScaleRule.h>
#include <SceneAPI/SceneData/Rules/EFXActorScaleRule.h>
#endif

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        void RegisterDataTypeReflection(AZ::SerializeContext* context)
        {
            // Check if this library hasn't already been reflected. This can happen as the ResourceCompilerScene needs
            //      to explicitly load and reflect the SceneAPI libraries to discover the available extension, while
            //      Gems with system components need to do the same in the Project Configurator.
            if (!context->IsRemovingReflection() && context->FindClassData(SceneData::MeshGroup::TYPEINFO_Uuid()))
            {
                return;
            }

            // Groups
            SceneData::MeshGroup::Reflect(context);
            SceneData::SkeletonGroup::Reflect(context);
            SceneData::SkinGroup::Reflect(context);
            SceneData::AnimationGroup::Reflect(context);
#if defined(MOTIONCANVAS_GEM_ENABLED)
            SceneData::ActorGroup::Reflect(context);
            SceneData::EFXMotionGroup::Reflect(context);
#endif

            // Rules
            SceneData::BlendShapeRule::Reflect(context);
            SceneData::CommentRule::Reflect(context);
            SceneData::LodRule::Reflect(context);
            SceneData::StaticMeshAdvancedRule::Reflect(context);
            SceneData::OriginRule::Reflect(context);
            SceneData::MaterialRule::Reflect(context);
            SceneData::PhysicsRule::Reflect(context);
            SceneData::SkeletonProxyRule::Reflect(context);
            SceneData::SkinMeshAdvancedRule::Reflect(context);
#if defined(MOTIONCANVAS_GEM_ENABLED)
            SceneData::MetaDataRule::Reflect(context);

            SceneData::EFXMeshRule::Reflect(context);
            SceneData::EFXSkinRule::Reflect(context);
            SceneData::EFXMotionCompressionSettingsRule::Reflect(context);
            SceneData::EFXMotionScaleRule::Reflect(context);
            SceneData::EFXActorScaleRule::Reflect(context);
#endif
            // Utility
            SceneData::SceneNodeSelectionList::Reflect(context);

            // Graph objects
            AZ::SceneData::GraphData::BoneData::Reflect(context);
            AZ::SceneData::GraphData::MaterialData::Reflect(context);
            AZ::SceneData::GraphData::RootBoneData::Reflect(context);
            AZ::SceneData::GraphData::TransformData::Reflect(context);
        }
    } // namespace SceneAPI
} // namespace AZ