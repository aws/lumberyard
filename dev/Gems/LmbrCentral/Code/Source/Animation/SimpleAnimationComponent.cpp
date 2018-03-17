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
#include "LmbrCentral_precompiled.h"
#include "SimpleAnimationComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Animation/CharacterAnimationBus.h>

#include <MathConversion.h>

namespace LmbrCentral
{
    class BehaviorSimpleAnimationComponentNotificationBus : public SimpleAnimationComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSimpleAnimationComponentNotificationBus, "{D29A77F9-35F0-404D-AC69-A64E8C1E8D22}", AZ::SystemAllocator,
            OnAnimationStarted, OnAnimationStopped);

        void OnAnimationStarted(const AnimatedLayer& animatedLayer) override
        {
            Call(FN_OnAnimationStarted, animatedLayer);
        }

        void OnAnimationStopped(const AnimatedLayer::LayerId animatedLayer) override
        {
            Call(FN_OnAnimationStopped, animatedLayer);
        }
    };

    void AnimatedLayer::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AnimatedLayer>()->
                Version(1)
                ->Field("Animation Name", &AnimatedLayer::m_animationName)
                ->Field("Layer Id", &AnimatedLayer::m_layerId)
                ->Field("Looping", &AnimatedLayer::m_looping)
                ->Field("Playback Speed", &AnimatedLayer::m_playbackSpeed)
                ->Field("Layer Weight", &AnimatedLayer::m_layerWeight)
                ->Field("AnimDrivenMotion", &AnimatedLayer::m_animDrivenRootMotion);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<AnimatedLayer>(
                    "Animated Layer", "Allows the configuration of one animation on one layer")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                    DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimatedLayer::m_animationName, "Animation name",
                    "Indicates the animation played by this component on this layer in absence of an overriding animation. ")
                        ->Attribute(AZ::Edit::Attributes::StringList, &AnimatedLayer::GetAvailableAnims)
                    ->DataElement(0, &AnimatedLayer::m_layerId, "Layer id",
                    "Indicates the layer that this animation is to be played on, animations can stomp on each other if they are not properly authored.")
                    ->DataElement(0, &AnimatedLayer::m_looping, "Looping",
                    "Indicates whether the animation should loop after its finished playing or not.")
                    ->DataElement(0, &AnimatedLayer::m_playbackSpeed, "Playback speed",
                    "Indicates the speed of animation playback.")
                    ->DataElement(0, &AnimatedLayer::m_layerWeight, "Layer weight",
                    "Indicates the weight of animations played on this layer.")
                    ->DataElement(0, &AnimatedLayer::m_animDrivenRootMotion, "Animate root",
                    "Enables animation-driven root motion during playback of this animation.");
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<AnimatedLayer>()
                ->Constructor<AZ::ScriptDataContext&>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("layerId", BehaviorValueProperty(&AnimatedLayer::m_layerId))
                ->Property("animationName", BehaviorValueProperty(&AnimatedLayer::m_animationName))
                ->Property("looping", BehaviorValueProperty(&AnimatedLayer::m_looping))
                ->Property("playbackSpeed", BehaviorValueProperty(&AnimatedLayer::m_playbackSpeed))
                ->Property("transitionTime", BehaviorValueProperty(&AnimatedLayer::m_transitionTime))
                ->Property("layerWeight", BehaviorValueProperty(&AnimatedLayer::m_layerWeight));
        }
    }

    AZStd::vector<AZStd::string> AnimatedLayer::GetAvailableAnims()
    {
        const AnimationNameList* availableAnimatonNames = nullptr;
        EBUS_EVENT_ID_RESULT(availableAnimatonNames, m_entityId,
            AnimationInformationBus, GetAvailableAnimationsList);

        if (availableAnimatonNames)
        {
            return *availableAnimatonNames;
        }
        else
        {
            return AnimationNameList();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // SimpleAnimator::ActiveAnimatedLayers Implementation

    //////////////////////////////////////////////////////////////////////////
    // Helpers
    void PopulateCryAnimParamsFromAnimatedLayer(const AnimatedLayer& animatedLayer, CryCharAnimationParams& outAnimParams)
    {
        outAnimParams.m_nLayerID = animatedLayer.GetLayerId();
        outAnimParams.m_fPlaybackSpeed = animatedLayer.GetPlaybackSpeed();
        outAnimParams.m_nFlags |= animatedLayer.IsLooping() ? CA_LOOP_ANIMATION : (CA_REPEAT_LAST_KEY | CA_FADEOUT);
        outAnimParams.m_fTransTime = animatedLayer.GetTransitionTime();

        outAnimParams.m_nFlags |= CA_FORCE_TRANSITION_TO_ANIM | CA_ALLOW_ANIM_RESTART;
        outAnimParams.m_fAllowMultilayerAnim = true;
        outAnimParams.m_fPlaybackWeight = animatedLayer.GetLayerWeight();
    }
    //////////////////////////////////////////////////////////////////////////

    const AnimatedLayer* SimpleAnimator::AnimatedLayerManager::GetActiveAnimatedLayer(AnimatedLayer::LayerId layerId) const
    {
        const auto& activeLayer = m_activeAnimatedLayers.find(layerId);
        if (activeLayer != m_activeAnimatedLayers.end())
        {
            return &(activeLayer->second);
        }
        else
        {
            return nullptr;
        }
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::AnimatedLayerManager::ActivateAnimatedLayer(const AnimatedLayer& animatedLayer)
    {
        if (!m_characterInstance)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        if (m_characterInstance->GetISkeletonAnim()->GetTrackViewStatus())
        {
            AZ_Warning("Animation", false, "Animated character for [%llu] is under cinematic control. Aborting animation playback request.", m_attachedEntityId);
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        if (animatedLayer.GetLayerId() == AnimatedLayer::s_invalidLayerId)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        // Check if the requested animation is already looping on the same layer
        const AnimatedLayer* activeAnimatedLayer = GetActiveAnimatedLayer(animatedLayer.GetLayerId());
        if (activeAnimatedLayer)
        {
            if (!animatedLayer.InterruptIfAlreadyPlaying() && *activeAnimatedLayer == animatedLayer)
            {
                return SimpleAnimationComponentRequests::Result::AnimationAlreadyPlaying;
            }
        }

        // If not then play this animation on this layer
        CryCharAnimationParams animationParamaters;
        PopulateCryAnimParamsFromAnimatedLayer(animatedLayer, animationParamaters);

        AZ::s32 animationId = m_characterInstance->GetIAnimationSet()->GetAnimIDByName(animatedLayer.GetAnimationName().c_str());

        if (animationId < 0)
        {
            return SimpleAnimationComponentRequests::Result::AnimationNotFound;
        }

        if (m_characterInstance->GetISkeletonAnim()->StartAnimationById(animationId, animationParamaters))
        {
            m_activeAnimatedLayers.insert(AZStd::make_pair(animatedLayer.GetLayerId(), animatedLayer));

            EBUS_EVENT_ID(m_attachedEntityId, SimpleAnimationComponentNotificationBus, OnAnimationStarted, animatedLayer);

            if (animatedLayer.GetLayerId() == 0)
            {
                EBUS_EVENT_ID(
                    m_attachedEntityId, 
                    CharacterAnimationRequestBus, 
                    SetAnimationDrivenMotion, 
                    animatedLayer.GetAnimationDrivenRootMotion());
            }

            return SimpleAnimationComponentRequests::Result::Success;
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::AnimatedLayerManager::DeactivateAnimatedLayer(AnimatedLayer::LayerId layerId, float blendOutTime)
    {
        if (!m_characterInstance)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        if (layerId == AnimatedLayer::s_invalidLayerId)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        if (m_characterInstance->GetISkeletonAnim()->GetTrackViewStatus())
        {
            AZ_Warning("Animation", false, "Animated character for [%llu] is under cinematic control. Aborting animation stop request.", m_attachedEntityId);
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        // Are there any active animations ?
        const AnimatedLayer* activeAnimatedLayer = GetActiveAnimatedLayer(layerId);
        if (!activeAnimatedLayer)
        {
            return SimpleAnimationComponentRequests::Result::NoAnimationPlayingOnLayer;
        }
        else
        {
            if (layerId == 0 && activeAnimatedLayer->GetAnimationDrivenRootMotion())
            {
                EBUS_EVENT_ID(m_attachedEntityId, CharacterAnimationRequestBus, SetAnimationDrivenMotion, false);
            }

            if (m_characterInstance->GetISkeletonAnim()->StopAnimationInLayer(layerId, blendOutTime))
            {
                m_activeAnimatedLayers.erase(layerId);

                EBUS_EVENT_ID(m_attachedEntityId, SimpleAnimationComponentNotificationBus, OnAnimationStopped, layerId);

                return SimpleAnimationComponentRequests::Result::Success;
            }
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::AnimatedLayerManager::DeactivateAllAnimatedLayers()
    {
        if (!m_characterInstance)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        if (m_characterInstance->GetISkeletonAnim()->GetTrackViewStatus())
        {
            AZ_Warning("Animation", false, "Animated character for [%llu] is under cinematic control. Aborting animation stop request.", m_attachedEntityId);
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        m_characterInstance->GetISkeletonAnim()->StopAnimationsAllLayers();

        for (const auto& layer : m_activeAnimatedLayers)
        {
            EBUS_EVENT_ID(m_attachedEntityId, SimpleAnimationComponentNotificationBus, OnAnimationStopped, layer.second.GetLayerId());
        }

        m_activeAnimatedLayers.clear();

        return SimpleAnimationComponentRequests::Result::Success;
    }

    bool SimpleAnimator::AnimatedLayerManager::IsLayerActive(AnimatedLayer::LayerId layerId) const
    {
        return m_activeAnimatedLayers.find(layerId) != m_activeAnimatedLayers.end();
    }

    SimpleAnimator::AnimatedLayerManager::AnimatedLayerManager(ICharacterInstance* characterInstance, AZ::EntityId entityId)
        : m_characterInstance(characterInstance)
        , m_attachedEntityId(entityId)
    {
    }

    SimpleAnimator::AnimatedLayerManager::~AnimatedLayerManager()
    {
        for (const auto& layer : m_activeAnimatedLayers)
        {
            EBUS_EVENT_ID(m_attachedEntityId, SimpleAnimationComponentNotificationBus, OnAnimationStopped, layer.second.GetLayerId());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // SimpleAnimator Implementation

    void SimpleAnimator::Activate(ICharacterInstance* characterInstance, AZ::EntityId entityId)
    {
        if (m_attachedEntityId.IsValid())
        {
            Deactivate();
        }

        if (characterInstance)
        {
            m_activeAnimatedLayerManager = std::make_unique<AnimatedLayerManager>(SimpleAnimator::AnimatedLayerManager(characterInstance, entityId));
            m_meshCharacterInstance = characterInstance;
        }
        else
        {
            m_activeAnimatedLayerManager = nullptr;
            m_meshCharacterInstance = nullptr;
        }

        AZ_Assert(entityId.IsValid(), "[Anim Component] Entity id is invalid");
        m_attachedEntityId = entityId;

        EBUS_EVENT_ID_RESULT(m_currentEntityLocation, entityId, AZ::TransformBus, GetWorldTM);

        MeshComponentNotificationBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AZ::TickBus::Handler::BusConnect();
    }

    void SimpleAnimator::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        (void)asset;
        UpdateCharacterInstance();
    }

    void SimpleAnimator::OnMeshDestroyed()
    {
        UpdateCharacterInstance();
    }

    void SimpleAnimator::UpdateCharacterInstance()
    {
        m_activeAnimatedLayerManager = nullptr;

        ICharacterInstance* newCharacterInstance = nullptr;
        EBUS_EVENT_ID_RESULT(newCharacterInstance, m_attachedEntityId, SkinnedMeshComponentRequestBus, GetCharacterInstance);

        if (newCharacterInstance)
        {
            m_activeAnimatedLayerManager = std::make_unique<AnimatedLayerManager>(SimpleAnimator::AnimatedLayerManager(newCharacterInstance, m_attachedEntityId));
            m_meshCharacterInstance = newCharacterInstance;
        }
        else
        {
            m_meshCharacterInstance = nullptr;
        }
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::StartAnimations(const AnimatedLayer::AnimatedLayerSet& animatedLayers)
    {
        int resultCounter = 0;

        for (const AnimatedLayer& animatedLayer : animatedLayers)
        {
            auto result = StartAnimation(animatedLayer);
            resultCounter += static_cast<int>(result) == 0 ? 0 : 1;
        }

        if (resultCounter == 0)
        {
            return SimpleAnimationComponentRequests::Result::Success;
        }
        else if (resultCounter < animatedLayers.size())
        {
            return SimpleAnimationComponentRequests::Result::SuccessWithErrors;
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }


    SimpleAnimationComponentRequests::Result SimpleAnimator::StartAnimation(const AnimatedLayer& animatedLayer)
    {
        if (m_activeAnimatedLayerManager)
        {
            return m_activeAnimatedLayerManager->ActivateAnimatedLayer(animatedLayer);
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }


    SimpleAnimationComponentRequests::Result SimpleAnimator::StopAnimations(const SimpleAnimationComponentRequests::LayerMask& animatedLayerIds, float blendOutTime)
    {
        if (!m_activeAnimatedLayerManager)
        {
            return SimpleAnimationComponentRequests::Result::Failure;
        }

        int resultCounter = 0;
        for (int i = 0; i < AnimatedLayer::s_maxActiveAnimatedLayers; i++)
        {
            if (animatedLayerIds[i])
            {
                auto result = StopAnimation(i, blendOutTime);
                resultCounter += static_cast<int>(result) == 0 ? 0 : 1;
            }
        }

        if (resultCounter == 0)
        {
            return SimpleAnimationComponentRequests::Result::Success;
        }
        else if (resultCounter < animatedLayerIds.count())
        {
            return SimpleAnimationComponentRequests::Result::SuccessWithErrors;
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::StopAnimation(const AnimatedLayer::LayerId animatedLayerId, float blendOutTime)
    {
        if (m_activeAnimatedLayerManager)
        {
            return m_activeAnimatedLayerManager->DeactivateAnimatedLayer(animatedLayerId, blendOutTime);
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::SetPlaybackSpeed(AnimatedLayer::LayerId layerId, float playbackSpeed)
    {
        if (m_activeAnimatedLayerManager && m_activeAnimatedLayerManager->IsLayerActive(layerId))
        {
            ICharacterInstance* character = m_activeAnimatedLayerManager->GetCharacter();
            if (character)
            {
                character->GetISkeletonAnim()->SetLayerPlaybackScale(layerId, playbackSpeed);
                return SimpleAnimationComponentRequests::Result::Success;
            }
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::SetPlaybackWeight(AnimatedLayer::LayerId layerId, float weight)
    {
        if (m_activeAnimatedLayerManager && m_activeAnimatedLayerManager->IsLayerActive(layerId))
        {
            ICharacterInstance* character = m_activeAnimatedLayerManager->GetCharacter();
            if (character)
            {
                character->GetISkeletonAnim()->SetLayerBlendWeight(layerId, weight);
                return SimpleAnimationComponentRequests::Result::Success;
            }
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    void SimpleAnimator::Deactivate()
    {
        StopAllAnimations();

        MeshComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        m_meshCharacterInstance = nullptr;

        m_activeAnimatedLayerManager = nullptr;
    }

    SimpleAnimationComponentRequests::Result SimpleAnimator::StopAllAnimations()
    {
        if (m_activeAnimatedLayerManager)
        {
            return m_activeAnimatedLayerManager->DeactivateAllAnimatedLayers();
        }

        return SimpleAnimationComponentRequests::Result::Failure;
    }

    void SimpleAnimator::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (m_meshCharacterInstance && m_activeAnimatedLayerManager)
        {
            // Remove layers whose animations have ended and report the same
            for (auto layerIt = m_activeAnimatedLayerManager->m_activeAnimatedLayers.begin();
                 layerIt != m_activeAnimatedLayerManager->m_activeAnimatedLayers.end(); )
            {
                if (!layerIt->second.IsLooping())
                {
                    AnimatedLayer::LayerId currentLayerID = layerIt->second.GetLayerId();
                    if (std::fabs(m_meshCharacterInstance->GetISkeletonAnim()->GetLayerNormalizedTime(currentLayerID) - 1)
                        < std::numeric_limits<float>::epsilon())
                    {
                        m_meshCharacterInstance->GetISkeletonAnim()->StopAnimationInLayer(currentLayerID, 0.0f);
                        layerIt = m_activeAnimatedLayerManager->m_activeAnimatedLayers.erase(layerIt);
                        EBUS_EVENT_ID(m_attachedEntityId, SimpleAnimationComponentNotificationBus, OnAnimationStopped, currentLayerID);
                        continue;
                    }
                }
                layerIt++;
            }
        }
    }

    void SimpleAnimator::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentEntityLocation = world;
    }

    //////////////////////////////////////////////////////////////////////////
    // SimpleAnimationComponent Implementation

    void SimpleAnimationComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SimpleAnimationComponent, AZ::Component>()
                ->Version(1)
                ->Field("Playback Entries", &SimpleAnimationComponent::m_defaultAnimationSettings);
        }

        AnimatedLayer::Reflect(context);

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<SimpleAnimationComponentNotificationBus>("SimpleAnimationComponentNotificationBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
                Handler<BehaviorSimpleAnimationComponentNotificationBus>();

            behaviorContext->EBus<SimpleAnimationComponentRequestBus>("SimpleAnimationComponentRequestBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
                Event("StartDefaultAnimations", &SimpleAnimationComponentRequestBus::Events::StartDefaultAnimations)->
                Event("StartAnimation", &SimpleAnimationComponentRequestBus::Events::StartAnimation)->
                Event("StartAnimationByName", &SimpleAnimationComponentRequestBus::Events::StartAnimationByName)->
                Event("StopAllAnimations", &SimpleAnimationComponentRequestBus::Events::StopAllAnimations)->
                Event("StopAnimationsOnLayer", &SimpleAnimationComponentRequestBus::Events::StopAnimationsOnLayer)->
                Event("SetPlaybackSpeed", &SimpleAnimationComponentRequestBus::Events::SetPlaybackSpeed)->
                Event("SetPlaybackWeight", &SimpleAnimationComponentRequestBus::Events::SetPlaybackWeight);
        }
    }

    void SimpleAnimationComponent::Init()
    {
        for (const AnimatedLayer& layer : m_defaultAnimationSettings)
        {
            m_defaultAnimLayerSet.insert(layer);
        }

        m_defaultAnimationSettings.clear();
    }

    void SimpleAnimationComponent::Activate()
    {
        SimpleAnimationComponentRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());

        ICharacterInstance* characterInstance = nullptr;
        EBUS_EVENT_ID_RESULT(characterInstance, GetEntityId(), SkinnedMeshComponentRequestBus, GetCharacterInstance);
        LinkToCharacterInstance(characterInstance);
    }

    void SimpleAnimationComponent::Deactivate()
    {
        SimpleAnimationComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect();

        m_animator.Deactivate();
    }

    void SimpleAnimationComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        m_isMeshAssetReady = true;
        ICharacterInstance* characterInstance = nullptr;
        EBUS_EVENT_ID_RESULT(characterInstance, GetEntityId(), SkinnedMeshComponentRequestBus, GetCharacterInstance);
        LinkToCharacterInstance(characterInstance);
        for (const AnimatedLayer& requestedAnimation : m_animationsQueuedBeforeAssetReady)
        {
            StartAnimation(requestedAnimation);
        }
        m_animationsQueuedBeforeAssetReady.clear();
    }

    void SimpleAnimationComponent::OnMeshDestroyed()
    {
        m_isMeshAssetReady = false;
    }

    void SimpleAnimationComponent::LinkToCharacterInstance(ICharacterInstance* characterInstance)
    {
        m_animator.Activate(characterInstance, GetEntityId());

        if (characterInstance)
        {
            StartDefaultAnimations();
        }
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StartDefaultAnimations()
    {
        return m_animator.StartAnimations(m_defaultAnimLayerSet);
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StartAnimation(const AnimatedLayer& animatedLayer)
    {
        if (m_isMeshAssetReady)
        {
            return m_animator.StartAnimation(animatedLayer);
        }
        else
        {
            m_animationsQueuedBeforeAssetReady.push_back(animatedLayer);
            return SimpleAnimationComponentRequests::Result::SuccessWithErrors;
        }
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StartAnimationSet(const AnimatedLayer::AnimatedLayerSet& animationSet)
    {
        return m_animator.StartAnimations(animationSet);
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StopAllAnimations()
    {
        return m_animator.StopAllAnimations();
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StopAnimationsOnLayer(AnimatedLayer::LayerId layerId, float blendOutTime)
    {
        return m_animator.StopAnimation(layerId, blendOutTime);
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::SetPlaybackSpeed(AnimatedLayer::LayerId layerId, float playbackSpeed)
    {
        return m_animator.SetPlaybackSpeed(layerId, playbackSpeed);
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::SetPlaybackWeight(AnimatedLayer::LayerId layerId, float weight)
    {
        return m_animator.SetPlaybackWeight(layerId, weight);
    }

    SimpleAnimationComponentRequests::Result SimpleAnimationComponent::StopAnimationsOnLayers(LayerMask layerIds, float blendOutTime)
    {
        return m_animator.StopAnimations(layerIds, blendOutTime);
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace LmbrCentral