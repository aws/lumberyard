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

#ifdef MOTIONCANVAS_GEM_ENABLED

#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionDataBuilder.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ExportContexts.h>

#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXMotionScaleRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IEFXMotionCompressionSettingsRule.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IEFXMotionGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>

// TODO: Including headers directly from a Gem is not modular, and can be removed once the pipeline supports Gem-based code. 
// This is contained within #ifdef MOTIONCANVAS_GEM_ENABLED, so it won't cause compile errors when the Gem isn't present.
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Math/Quaternion.h>
#include <AzToolsFramework/Debug/TraceContext.h>


namespace
{
    const float kDefaultMaxTranslationError = 0.000025f;
    const float kDefaultMaxRotationError = 0.000025f;
    const float kDefaultMaxScaleError = 0.0001f;
}

namespace MotionCanvasPipeline
{
    namespace SceneEvents = AZ::SceneAPI::Events;
    namespace SceneUtil = AZ::SceneAPI::Utilities;
    namespace SceneContainers = AZ::SceneAPI::Containers;
    namespace SceneViews = AZ::SceneAPI::Containers::Views;
    namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

    MotionDataBuilder::MotionDataBuilder()
        : CallProcessorBinder()
    {
        BindToCall(&MotionDataBuilder::BuildMotionData);
        ActivateBindings();
    }


    AZ::SceneAPI::Events::ProcessingResult MotionDataBuilder::BuildMotionData(MotionDataBuilderContext& context)
    {
        if (context.m_phase != AZ::RC::Phase::Filling)
        {
            return SceneEvents::ProcessingResult::Ignored;
        }

        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& motionGroup = context.m_group;
        const char* rootBoneName = motionGroup.GetSelectedRootBone().c_str();
        AZ_TraceContext("Root bone", rootBoneName);

        const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

        SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
        if (!rootBoneNodeIndex.IsValid())
        {
            AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
            return SceneEvents::ProcessingResult::Failure;
        }

        AZStd::shared_ptr<const SceneDataTypes::IEFXMotionCompressionSettingsRule> compressionRule = motionGroup.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IEFXMotionCompressionSettingsRule>();

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
            float maxRotationError    = compressionRule ? compressionRule->GetMaxRotationError() : kDefaultMaxRotationError;
            float maxScaleError       = compressionRule ? compressionRule->GetMaxScaleError() : kDefaultMaxScaleError;

            // If we deal with a root bone or one of its child nodes, disable the keytrack optimization.
            // This prevents sliding feet etc. A better solution is probably to increase compression rates based on the "distance" from the root node, hierarchy wise.
            SceneContainers::SceneGraph::NodeIndex boneNodeIndex = graph.Find( it->first.GetPath() );
            if (graph.GetNodeParent(boneNodeIndex) == rootBoneNodeIndex || boneNodeIndex == rootBoneNodeIndex)
            {
                maxTranslationError = -1.0f;
                maxRotationError    = -1.0f;
                maxScaleError       = -1.0f;
            }

            subMotion->CreatePosTrack();
            subMotion->CreateScaleTrack();
            subMotion->CreateRotTrack();

            EMotionFX::KeyTrackLinear<MCore::Vector3, MCore::Vector3>* posTrack = subMotion->GetPosTrack();
            EMotionFX::KeyTrackLinear<MCore::Vector3, MCore::Vector3>* scaleTrack = subMotion->GetScaleTrack();
            EMotionFX::KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();

            if (!posTrack || !scaleTrack || !rotTrack)
            {
                return SceneEvents::ProcessingResult::Failure;
            }

            const uint32_t startFrame = motionGroup.GetStartFrame();
            const uint32_t numFrames = (motionGroup.GetEndFrame() - startFrame) + 1;
            posTrack->SetNumKeys(numFrames);
            scaleTrack->SetNumKeys(numFrames);
            rotTrack->SetNumKeys(numFrames);

            const double timeStep = animation->GetTimeStepBetweenFrames();
            for (uint32_t frame = 0; frame < numFrames; ++frame)
            {
                const float time = (float)(frame * timeStep);
                const AZ::Transform boneXf = animation->GetKeyFrame(frame + startFrame);
                const AZ::Vector3 pos(boneXf.GetTranslation());
                AZ::Transform boneXfNoScale(boneXf);
                const AZ::Vector3 scale = boneXfNoScale.ExtractScale();
                const AZ::Quaternion rotation(AZ::Quaternion::CreateFromTransform(boneXfNoScale));

                posTrack->SetKey(frame, time, MCore::AzVec3ToEmfxVec3(pos));
                scaleTrack->SetKey(frame, time, MCore::AzVec3ToEmfxVec3(scale));
                rotTrack->SetKey(frame, time, MCore::AzQuatToEmfxQuat(rotation));
            }

            posTrack->Init();
            if (!posTrack->CheckIfIsAnimated(subMotion->GetPosePos(), MCore::Math::epsilon))
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
            if (!scaleTrack->CheckIfIsAnimated(subMotion->GetPoseScale(), MCore::Math::epsilon))
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

        AZStd::shared_ptr<const SceneDataTypes::IEFXMotionScaleRule> scaleRule = motionGroup.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IEFXMotionScaleRule>();
        if (scaleRule)
        {
            float scaleFactor = scaleRule->GetScaleFactor();
            if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
            {
                context.m_motion.Scale(scaleFactor);
            }
        }

        return SceneEvents::ProcessingResult::Success;
    }

} // MotionCanvasPipeline

#endif // MOTIONCANVAS_GEM_ENABLED