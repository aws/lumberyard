
#include "AudioAnimationEvents_precompiled.h"
#include "AudioAnimationEventsProxyComponent.h"

#include <ISystem.h>
#include "ICryAnimation.h"
#include <MathConversion.h>

#include <IAudioSystem.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>

namespace AudioAnimationEvents
{
    /*!
    * Audio proxy data per joint.
    * The AudioAnimationEventsProxy component uses these to track joint proxies and transforms.
    */
    class JointAudioProxy
    {
    public:
        JointAudioProxy(Audio::IAudioProxy* proxy, const AZ::Transform& transform)
            : m_audioProxy(proxy)
            , m_transform(transform)
        {
        }

        ~JointAudioProxy()
        {
            if (m_audioProxy != nullptr)
            {
                m_audioProxy->StopAllTriggers();
                m_audioProxy->Release();
                m_audioProxy = nullptr;
            }
        }

        Audio::IAudioProxy* GetAudioProxy() { return m_audioProxy; }
        const AZ::Transform& GetTransform() { return m_transform; }

    protected:
        Audio::IAudioProxy* m_audioProxy = nullptr;
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
    };

    void AudioAnimationEventsProxyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AudioAnimationEventsProxyComponent, AZ::Component>()
                ->Version(0)
                ->Field("Tracks Entity Position",&AudioAnimationEventsProxyComponent::m_tracksEntityPosition )
                ->Field("Audio Event Name",&AudioAnimationEventsProxyComponent::m_audioEventName )
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AudioAnimationEventsProxyComponent>("AudioAnimationEventsProxy", "Provides an audio proxy to play audio_trigger animation events.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(0, &AudioAnimationEventsProxyComponent::m_tracksEntityPosition, "Tracks Entity Position", "Should the audio source position update as the entity moves.")
                    ->DataElement(0, &AudioAnimationEventsProxyComponent::m_audioEventName, "Audio Event Name", "Defaults to audio_trigger")
                    ;
            }
        }
    }

    void AudioAnimationEventsProxyComponent::Init()
    {
    }

    void AudioAnimationEventsProxyComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_transform = world;

        // update audio proxy positions
        for (auto entry : m_jointAudioProxies)
        {
            JointAudioProxy* proxy = entry.second;
            if (proxy != nullptr)
            {
                Audio::IAudioProxy* audioProxy = proxy->GetAudioProxy();
                if (audioProxy != nullptr)
                {
                    audioProxy->SetPosition(AZTransformToLYTransform(m_transform * proxy->GetTransform()));
                }
            }
        }
    }

    void AudioAnimationEventsProxyComponent::OnAnimationEvent(const LmbrCentral::AnimationEvent& event)
    {
        if (event.m_eventName && event.m_eventName[0] && _stricmp(m_audioEventName.c_str(), event.m_eventName) == 0)
        {
            // get the audio trigger id
            Audio::TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
			Audio::AudioSystemRequestBus::BroadcastResult(triggerId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, event.m_parameter);
            if (triggerId == INVALID_AUDIO_CONTROL_ID)
            {
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(AudioAnimationEventsProxyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                AZ_Warning("AudioAnimationEventProxy", triggerId != INVALID_AUDIO_CONTROL_ID, "AudioAnimationEventsProxy given invalid trigger ID %d", triggerId);
#endif
                return;
            }

            int32 jointId = -1;
            QuatT jointTransform(IDENTITY);

            // get the character instance
            ICharacterInstance* character = nullptr;
            LmbrCentral::SkinnedMeshComponentRequestBus::EventResult(character, GetEntityId(), &LmbrCentral::SkinnedMeshComponentRequestBus::Events::GetCharacterInstance);
            if (character && event.m_boneName1 && event.m_boneName1[0])
            {
                jointId = character->GetIDefaultSkeleton().GetJointIDByName(event.m_boneName1);
                if (jointId >= 0)
                {
                    jointTransform = character->GetISkeletonPose()->GetAbsJointByID(jointId);
                }
            }

            // find or create an audio proxy
            JointAudioProxy* proxy = stl::find_in_map(m_jointAudioProxies, jointId, nullptr);
            if (proxy == nullptr)
            {
				Audio::IAudioProxy* audioProxy = nullptr;
				Audio::AudioSystemRequestBus::BroadcastResult(audioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);
                AZ_Assert(audioProxy, "AudioAnimationEventsProxyComponent::OnAnimationEvent - Failed to get free audio proxy");
                if (audioProxy)
                {
                    AZStd::string proxyName = AZStd::string::format("%s_animevent_proxy", GetEntity()->GetName().c_str());
                    audioProxy->Initialize(proxyName.c_str());
                    audioProxy->SetObstructionCalcType(Audio::eAOOCT_IGNORE);
                    proxy = new JointAudioProxy(audioProxy, LYQuatTToAZTransform(jointTransform));
                    if (proxy != nullptr)
                    {
                        m_jointAudioProxies[jointId] = proxy;
                    }
                }
            }

            if (proxy != nullptr)
            {
                Audio::IAudioProxy* audioProxy = proxy->GetAudioProxy();
                AZ_Assert(audioProxy, "AudioAnimationEventsProxyComponent::OnAnimationEvent - joint has invalid audio proxy.");
                if (audioProxy != nullptr)
                {
                    audioProxy->SetPosition(AZTransformToLYTransform(m_transform * proxy->GetTransform()));
                    audioProxy->ExecuteTrigger(triggerId, eLSM_None);
                }
            }
        }
    }

    void AudioAnimationEventsProxyComponent::Activate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusConnect(GetEntityId());
        if (m_tracksEntityPosition)
        {
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        }

        // initialize the transform from the entity.
        AZ::TransformBus::EventResult(m_transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
    }

    void AudioAnimationEventsProxyComponent::Deactivate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusDisconnect(GetEntityId());
        if (m_tracksEntityPosition)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        }

        for (auto entry : m_jointAudioProxies)
        {
            if (entry.second != nullptr)
            {
                delete entry.second;
            }
        }

        m_jointAudioProxies.clear();
    }
}
