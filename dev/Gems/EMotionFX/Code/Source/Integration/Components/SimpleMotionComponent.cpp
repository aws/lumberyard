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


#include "EMotionFX_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Integration/Components/SimpleMotionComponent.h>
#include <MCore/Source/AttributeString.h>

namespace EMotionFX
{
    namespace Integration
    {

        void SimpleMotionComponent::Configuration::Reflect(AZ::ReflectContext *context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(1)
                    ->Field("MotionAsset", &Configuration::m_motionAsset)
                    ->Field("Loop", &Configuration::m_loop)
                    ->Field("Retarget", &Configuration::m_retarget)
                    ->Field("Reverse", &Configuration::m_reverse)
                    ->Field("Mirror", &Configuration::m_mirror)
                    ->Field("PlaySpeed", &Configuration::m_playspeed)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Configuration>(
                        "Configuration", "Settings for this Simple Motion")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_motionAsset,
                            "Motion", "EMotion FX motion to be loaded for this actor")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_loop,
                            "Loop motion", "Toggles looping of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_retarget,
                            "Retarget motion", "Toggles retargeting of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_reverse,
                            "Reverse motion", "Toggles reversing of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_mirror,
                            "Mirror motion", "Toggles mirroring of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_playspeed,
                            "Play speed", "Determines the rate at which the motion is played")
                        ;
                }
            }
        }

        void SimpleMotionComponent::Reflect(AZ::ReflectContext* context)
        {
            Configuration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SimpleMotionComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &SimpleMotionComponent::m_configuration)
                    ;
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<SimpleMotionComponentRequestBus>("SimpleMotionComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Event("LoopMotion", &SimpleMotionComponentRequestBus::Events::LoopMotion)
                    ->Event("RetargetMotion", &SimpleMotionComponentRequestBus::Events::RetargetMotion)
                    ->Event("ReverseMotion", &SimpleMotionComponentRequestBus::Events::ReverseMotion)
                    ->Event("MirrorMotion", &SimpleMotionComponentRequestBus::Events::MirrorMotion)
                    ->Event("SetPlaySpeed", &SimpleMotionComponentRequestBus::Events::SetPlaySpeed)
                    ;
            }
        }

        SimpleMotionComponent::Configuration::Configuration()
            : m_loop(false)
            , m_retarget(false)
            , m_reverse(false)
            , m_mirror(false)
            , m_playspeed(1.f)
        {
        }

        SimpleMotionComponent::SimpleMotionComponent(const Configuration* config)
            : m_actorInstance(nullptr)
            , m_motionInstance(nullptr)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        SimpleMotionComponent::~SimpleMotionComponent()
        {
        }

        void SimpleMotionComponent::Init()
        {
        }

        void SimpleMotionComponent::Activate()
        {
            m_motionInstance = nullptr;

            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            auto& cfg = m_configuration;

            if (cfg.m_motionAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_motionAsset.GetId());
                cfg.m_motionAsset.QueueLoad();
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void SimpleMotionComponent::Deactivate()
        {
            ActorComponentNotificationBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            RemoveMotionInstanceFromActor();
            m_configuration.m_motionAsset.Release();
        }

        void SimpleMotionComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            auto& cfg = m_configuration;

            if (asset == cfg.m_motionAsset)
            {
                cfg.m_motionAsset = asset;
                UpdateMotionInstance();
            }
        }

        void SimpleMotionComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        void SimpleMotionComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;
            UpdateMotionInstance();
        }

        void SimpleMotionComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            RemoveMotionInstanceFromActor();
            m_actorInstance = nullptr;
        }

        void SimpleMotionComponent::UpdateMotionInstance()
        {
            RemoveMotionInstanceFromActor();

            auto& cfg = m_configuration;

            if (!m_actorInstance || !cfg.m_motionAsset.IsReady())
            {
                return;
            }

            auto* motionAsset = cfg.m_motionAsset.GetAs<MotionAsset>();
            AZ_Error("EMotionFX", motionAsset, "Motion asset is not valid.");
            if (!motionAsset || !m_actorInstance)
            {
                return;
            }
            //init the PlaybackInfo based on our config
            EMotionFX::PlayBackInfo info;
            info.mNumLoops = cfg.m_loop ? EMFX_LOOPFOREVER : 1;
            info.mRetarget = cfg.m_retarget;
            info.mPlayMode = cfg.m_reverse ? EMotionFX::EPlayMode::PLAYMODE_BACKWARD : EMotionFX::EPlayMode::PLAYMODE_FORWARD;
            info.mFreezeAtLastFrame = info.mNumLoops == 1;
            info.mMirrorMotion = cfg.m_mirror;
            info.mPlaySpeed = cfg.m_playspeed;
            info.mPlayNow = true;
            info.mDeleteOnZeroWeight = true;
            m_motionInstance = m_actorInstance->GetMotionSystem()->PlayMotion(motionAsset->m_emfxMotion.get(), &info);
        }

        void SimpleMotionComponent::RemoveMotionInstanceFromActor()
        {
            if (m_motionInstance)
            {
                if (m_actorInstance && m_actorInstance->GetMotionSystem())
                {
                    m_actorInstance->GetMotionSystem()->RemoveMotionInstance(m_motionInstance);
                }
                m_motionInstance = nullptr;
            }
        }

        void SimpleMotionComponent::LoopMotion(bool enable)
        {
        }

        void SimpleMotionComponent::RetargetMotion(bool enable)
        {
        }

        void SimpleMotionComponent::ReverseMotion(bool enable)
        {
        }

        void SimpleMotionComponent::MirrorMotion(bool enable)
        {
        }

        void SimpleMotionComponent::SetPlaySpeed(float speed)
        {
        }
    } // namespace integration
} // namespace EMotionFX

