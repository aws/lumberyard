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

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>

#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <SceneAPIExt/Rules/IMotionScaleRule.h>
#include <SceneAPIExt/Rules/IMotionCompressionSettingsRule.h>
#include <SceneAPIExt/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <RCExt/Motion/MotionDataBuilder.h>
#include <RCExt/ExportContexts.h>
#include <RCExt/CoordinateSystemConverter.h>

#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <EMotionFX/Source/KeyTrackLinear.h>
#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Math/Quaternion.h>
#include <AzToolsFramework/Debug/TraceContext.h>


namespace
{
    const float kDefaultMaxTranslationError = 0.000025f;
    const float kDefaultMaxRotationError = 0.000025f;
    const float kDefaultMaxScaleError = 0.0001f;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        MotionDataBuilder::MotionDataBuilder()
        {
            BindToCall(&MotionDataBuilder::BuildMotionData);
        }

        void MotionDataBuilder::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MotionDataBuilder, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult MotionDataBuilder::BuildMotionData(MotionDataBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const Group::IMotionGroup& motionGroup = context.m_group;
            const char* rootBoneName = motionGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            CoordinateSystemConverter ruleCoordSysConverter;
            AZStd::shared_ptr<Rule::CoordinateSystemRule> coordinateSystemRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                ruleCoordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            CoordinateSystemConverter coordSysConverter = ruleCoordSysConverter;
            if (context.m_scene.GetOriginalSceneOrientation() != SceneContainers::Scene::SceneOrientation::ZUp)
            {
                AZ::Transform orientedTarget = ruleCoordSysConverter.GetTargetTransform();
                AZ::Transform rotationZ = AZ::Transform::CreateRotationZ(-AZ::Constants::Pi);
                orientedTarget = orientedTarget * rotationZ;

                //same as rule
                // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };
                coordSysConverter = CoordinateSystemConverter::CreateFromTransforms(ruleCoordSysConverter.GetSourceTransform(), orientedTarget, targetBasisIndices);
            }

            AZStd::shared_ptr<const Rule::IMotionCompressionSettingsRule> compressionRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::IMotionCompressionSettingsRule>();

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
            auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
            {
                if (!it->second)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                AZStd::shared_ptr<const SceneDataTypes::IBoneData> nodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (!nodeBone)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                // Currently only get the first (one) AnimationData
                auto childView = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, graph.ConvertToNodeIndex(it.GetHierarchyIterator()),
                        graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(childView.begin(), childView.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::IAnimationData>());
                if (result == childView.end())
                {
                    continue;
                }

                const SceneDataTypes::IAnimationData* animation = azrtti_cast<const SceneDataTypes::IAnimationData*>(result->get());

                const char* nodeName = it->first.GetName();
                EMotionFX::SkeletalSubMotion* subMotion = EMotionFX::SkeletalSubMotion::Create(nodeName);
                if (!subMotion)
                {
                    return SceneEvents::ProcessingResult::Failure;
                }

                // Use either the default compression rates when there is no compression rule, or use the rule's values.
                float maxTranslationError = compressionRule ? compressionRule->GetMaxTranslationError() : kDefaultMaxTranslationError;
                float maxRotationError = compressionRule ? compressionRule->GetMaxRotationError() : kDefaultMaxRotationError;
                float maxScaleError = compressionRule ? compressionRule->GetMaxScaleError() : kDefaultMaxScaleError;

                // If we deal with a root bone or one of its child nodes, disable the keytrack optimization.
                // This prevents sliding feet etc. A better solution is probably to increase compression rates based on the "distance" from the root node, hierarchy wise.
                const SceneContainers::SceneGraph::NodeIndex boneNodeIndex = graph.Find(it->first.GetPath());
                if (graph.GetNodeParent(boneNodeIndex) == rootBoneNodeIndex || boneNodeIndex == rootBoneNodeIndex)
                {
                    maxTranslationError = -1.0f;
                    maxRotationError = -1.0f;
                    maxScaleError = -1.0f;
                }

                subMotion->CreatePosTrack();
                subMotion->CreateScaleTrack();
                subMotion->CreateRotTrack();

                EMotionFX::KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* posTrack = subMotion->GetPosTrack();
                EMotionFX::KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* scaleTrack = subMotion->GetScaleTrack();
                EMotionFX::KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();

                if (!posTrack || !scaleTrack || !rotTrack)
                {
                    return SceneEvents::ProcessingResult::Failure;
                }

                const AZ::u32 sceneFrameCount = aznumeric_caster(animation->GetKeyFrameCount());
                AZ::u32 startFrame, endFrame;
                if (motionGroup.GetRuleContainerConst().ContainsRuleOfType<const Rule::MotionRangeRule>())
                {
                    AZStd::shared_ptr<const Rule::MotionRangeRule> motionRangeRule = motionGroup.GetRuleContainerConst().FindFirstByType<const Rule::MotionRangeRule>();
                    startFrame = motionRangeRule->GetStartFrame();
                    endFrame = motionRangeRule->GetEndFrame();
                    // Sanity check
                    if (startFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Start frame %d is greater or equal than the actual number of frames %d in animation.\n", startFrame, sceneFrameCount);
                        return SceneEvents::ProcessingResult::Failure;
                    }
                    if (endFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "End frame %d is greater or equal than the actual number of frames %d in animation. Clamping the end frame to %d\n", endFrame, sceneFrameCount, sceneFrameCount - 1);
                        endFrame = sceneFrameCount - 1;
                    }
                }
                else
                {
                    startFrame = 0;
                    endFrame = sceneFrameCount - 1;
                }

                const AZ::u32 numFrames = (endFrame - startFrame) + 1;

                posTrack->SetNumKeys(numFrames);
                scaleTrack->SetNumKeys(numFrames);
                rotTrack->SetNumKeys(numFrames);

                // Get the bind pose transform in local space.
                AZ::Transform bindSpaceLocalTransform;
                const SceneContainers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(boneNodeIndex);
                if (boneNodeIndex != rootBoneNodeIndex)
                {
                    auto parentNode = graph.GetNodeContent(parentIndex);
                    AZStd::shared_ptr<const SceneDataTypes::IBoneData> parentNodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(parentNode);
                    bindSpaceLocalTransform = parentNodeBone->GetWorldTransform().GetInverseFull() * nodeBone->GetWorldTransform();
                }
                else
                {
                    bindSpaceLocalTransform = nodeBone->GetWorldTransform();
                }

                const double timeStep = animation->GetTimeStepBetweenFrames();
                for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                {
                    const float time = aznumeric_cast<float>(frame * timeStep);
                    const AZ::Transform boneTransform = animation->GetKeyFrame(frame + startFrame);
                    AZ::Transform boneTransformNoScale(boneTransform);

                    const AZ::Vector3    position = coordSysConverter.ConvertVector3(boneTransform.GetTranslation());
                    const AZ::Vector3    scale    = coordSysConverter.ConvertScale(boneTransformNoScale.ExtractScale());
                    const AZ::Quaternion rotation = coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromTransform(boneTransformNoScale));

                    // Set the pose when this is the first frame.
                    // This is used as optimization so that poses or non-animated submotions do not need any key tracks.
                    if (frame == 0)
                    {
                        subMotion->SetPosePos(position);
                        subMotion->SetPoseRot(MCore::AzQuatToEmfxQuat(rotation));
                        subMotion->SetPoseScale(scale);
                    }

                    posTrack->SetKey(frame, time, AZ::PackedVector3f(position));
                    rotTrack->SetKey(frame, time, MCore::AzQuatToEmfxQuat(rotation));
                    scaleTrack->SetKey(frame, time, AZ::PackedVector3f(scale));
                }

                // Set the bind pose transform.
                AZ::Transform bindBoneTransformNoScale(bindSpaceLocalTransform);
                const AZ::Vector3    bindPos   = coordSysConverter.ConvertVector3(bindSpaceLocalTransform.GetTranslation());
                const AZ::Vector3    bindScale = coordSysConverter.ConvertScale(bindBoneTransformNoScale.ExtractScale());
                const AZ::Quaternion bindRot   = coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromTransform(bindBoneTransformNoScale));
                subMotion->SetBindPosePos(bindPos);
                subMotion->SetBindPoseRot(MCore::AzQuatToEmfxQuat(bindRot));
                subMotion->SetBindPoseScale(bindScale);

                posTrack->Init();
                if (!posTrack->CheckIfIsAnimated(AZ::PackedVector3f(subMotion->GetPosePos()), MCore::Math::epsilon))
                {
                    subMotion->RemovePosTrack();
                }
                else
                {
                    if (maxTranslationError > 0.0f)
                    {
                        posTrack->Optimize(maxTranslationError);
                    }
                }

                rotTrack->Init();
                if (!rotTrack->CheckIfIsAnimated(subMotion->GetPoseRot(), MCore::Math::epsilon))
                {
                    subMotion->RemoveRotTrack();
                }
                else
                {
                    if (maxRotationError > 0.0f)
                    {
                        rotTrack->Optimize(maxRotationError);
                    }
                }

                scaleTrack->Init();
                if (!scaleTrack->CheckIfIsAnimated(AZ::PackedVector3f(subMotion->GetPoseScale()), MCore::Math::epsilon))
                {
                    subMotion->RemoveScaleTrack();
                }
                else
                {
                    if (maxScaleError > 0.0f)
                    {
                        scaleTrack->Optimize(maxScaleError);
                    }
                }

                context.m_motion.AddSubMotion(subMotion);
            }

            AZStd::shared_ptr<Rule::IMotionScaleRule> scaleRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::IMotionScaleRule>();
            if (scaleRule)
            {
                float scaleFactor = scaleRule->GetScaleFactor();
                if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
                {
                    context.m_motion.Scale(scaleFactor);
                }
            }

            //process MorphTargetAnimation
            //scene
            //    IBlendShapeAnimationData
            auto sceneGraphView = SceneViews::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());
            auto sceneGraphDownardsIteratorView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(
                graph, graph.GetRoot(), sceneGraphView.begin(), true);

            auto iterator = sceneGraphDownardsIteratorView.begin();

            for (; iterator != sceneGraphDownardsIteratorView.end(); ++iterator)
            {
                SceneContainers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                SceneContainers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                AZ_Assert(currentIndex.IsValid(), "While iterating through the Scene Graph an unexpected invalid entry was found.");
                AZStd::shared_ptr<const SceneDataTypes::IGraphObject> currentItem = iterator->second;
                if (hierarchy->IsEndPoint())
                {
                    if (currentItem->RTTI_IsTypeOf(SceneDataTypes::IBlendShapeAnimationData::TYPEINFO_Uuid()))
                    {
                        const SceneDataTypes::IBlendShapeAnimationData* blendShapeAnimationData = static_cast<const SceneDataTypes::IBlendShapeAnimationData*>(currentItem.get());

                        uint32 id = MCore::GetStringIdPool().GenerateIdForString(blendShapeAnimationData->GetBlendShapeName());

                        EMotionFX::MorphSubMotion* subMotion = EMotionFX::MorphSubMotion::Create(id);
                        EMotionFX::KeyTrackLinear<float, MCore::Compressed16BitFloat>* keyTrack = subMotion->GetKeyTrack();
                        const size_t keyFrameCount = blendShapeAnimationData->GetKeyFrameCount();
                        const float keyFrameStep = static_cast<float>(blendShapeAnimationData->GetTimeStepBetweenFrames());
                        for (int keyFrameIndex = 0; keyFrameIndex < keyFrameCount; keyFrameIndex++)
                        {
                            const float keyFrameValue = static_cast<float>(blendShapeAnimationData->GetKeyFrame(keyFrameIndex));
                            keyTrack->AddKeySorted(keyFrameIndex * keyFrameStep, keyFrameValue);
                        }
                        context.m_motion.AddMorphSubMotion(subMotion);
                    }
                }
            }

            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace Pipeline
} // namespace EMotionFX