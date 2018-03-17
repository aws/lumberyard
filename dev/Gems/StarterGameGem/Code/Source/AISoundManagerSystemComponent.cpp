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

#include "StarterGameGem_precompiled.h"
#include "AISoundManagerSystemComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>

#include <GameplayEventBus.h>
#include <IRenderAuxGeom.h>
#include <MathConversion.h>


namespace StarterGameGem
{
    class SoundTypesHolder
    {
    public:
        AZ_TYPE_INFO(SoundTypesHolder, "{382F9150-BA22-4EA2-863E-8E71281951E2}");
        AZ_CLASS_ALLOCATOR(SoundTypesHolder, AZ::SystemAllocator, 0);

        SoundTypesHolder() = default;
        ~SoundTypesHolder() = default;
    };

    AISoundManagerSystemComponent* g_amsc_instance = nullptr;

    AISoundManagerSystemComponent* AISoundManagerSystemComponent::GetInstance()
    {
        return g_amsc_instance;
    }

    AISoundManagerSystemComponent::AISoundManagerSystemComponent()
        : m_broadcastSoundEventName("HeardSound")
    {}

    void AISoundManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AISoundManagerSystemComponent, AZ::Component>()
                ->Version(1)
                ->SerializerForEmptyClass()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AISoundManagerSystemComponent>(
                    "A.I. Sound Manager System", "Receives and filters global audio events")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // SoundProperties.
            behaviorContext->Class<SoundProperties>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("Clone", [](const SoundProperties& rhs) -> SoundProperties { return rhs; })
                ->Property("origin", BehaviorValueProperty(&SoundProperties::origin))
                ->Property("range", BehaviorValueProperty(&SoundProperties::range))
                ->Property("type", BehaviorValueProperty(&SoundProperties::type))
                ->Property("isLineSound", BehaviorValueGetter(&SoundProperties::isLineSound), nullptr)   // read-only in Lua
                ->Property("destination", BehaviorValueProperty(&SoundProperties::destination))
                ->Property("closestPoint", BehaviorValueProperty(&SoundProperties::closestPoint))
                ;

            // SoundType constants.
            // IMPORTANT: requires an empty class 'SoundTypesHolder'
            behaviorContext->Class<SoundTypesHolder>("SoundTypes")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constant("RifleHit", BehaviorConstant(SoundTypes::ST_RifleHit))
                ->Constant("WhizzPast", BehaviorConstant(SoundTypes::ST_WhizzPast))
                ->Constant("LauncherExplosion", BehaviorConstant(SoundTypes::ST_LauncherExplosion))
                ->Constant("GunShot", BehaviorConstant(SoundTypes::ST_GunShot))
                ;

            behaviorContext->EBus<AISoundManagerSystemRequestBus>("AISoundManagerSystemRequestBus")
                ->Event("BroadcastSound", &AISoundManagerSystemRequestBus::Events::BroadcastSound)
                ->Event("RegisterEntity", &AISoundManagerSystemRequestBus::Events::RegisterEntity)
                ->Event("UnregisterEntity", &AISoundManagerSystemRequestBus::Events::UnregisterEntity)
                ;
        }
    }

    void AISoundManagerSystemComponent::Activate()
    {
        AISoundManagerSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        g_amsc_instance = this;
    }

    void AISoundManagerSystemComponent::Deactivate()
    {
        AISoundManagerSystemRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        g_amsc_instance = nullptr;
    }

    void AISoundManagerSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        static ICVar* const cvar = gEnv->pConsole->GetCVar("s_visualiseSoundRanges");
        float sphereDuration = cvar != nullptr ? cvar->GetFVal() : 0.0f;

        // Age the list.
        for (AZStd::list<SoundPropertiesDebug>::iterator it = m_debugSounds.begin(); it != m_debugSounds.end(); )
        {
            it->duration += deltaTime;
            if (it->duration >= sphereDuration)
            {
                it = m_debugSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Check the console variable to decide if we want to render the sounds' spheres.
        if (cvar && sphereDuration != 0.0f)
        {
            IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
            SAuxGeomRenderFlags flags;
            flags.SetAlphaBlendMode(e_AlphaBlended);
            flags.SetCullMode(e_CullModeNone);
            flags.SetDepthTestFlag(e_DepthTestOn);
            flags.SetFillMode(e_FillModeWireframe);		// render them all as wireframe otherwise we can't see the inner shapes
            renderAuxGeom->SetRenderFlags(flags);

            for (AZStd::list<SoundPropertiesDebug>::iterator it = m_debugSounds.begin(); it != m_debugSounds.end(); ++it)
            {
                float alpha = 255.0f;
                alpha *= 1.0f - (it->duration / sphereDuration);
                renderAuxGeom->DrawSphere(AZVec3ToLYVec3(it->closestPoint), it->range, ColorB(240, 131, 34, (uint8)alpha));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // AISoundManagerSystemRequests
    void AISoundManagerSystemComponent::BroadcastSound(SoundProperties props)
    {
        props.isLineSound = IsLineSound(props);

        float rangeSq = props.range * props.range;
        AZ::Transform tm = AZ::Transform::CreateIdentity();
        if (props.isLineSound)
        {
            // Run through all the A.I. finding their closest point to the line.
            AZ::Vector3 closestPoint;
            AZ::Vector3 aiPos;
            for (AZStd::list<AZ::EntityId>::iterator it = m_registeredEntities.begin(); it != m_registeredEntities.end(); ++it)
            {
                AZ::TransformBus::EventResult(tm, *it, &AZ::TransformInterface::GetWorldTM);
                aiPos = tm.GetTranslation();

                closestPoint = GetClosestPointOnLineSegment(props.origin, props.destination, aiPos);
                float distanceSq = (closestPoint - aiPos).GetLengthSq();
                if (distanceSq < rangeSq)
                {
                    // Create a temporary struct (so we don't corrupt the real value) and move the
                    // origin to the closest point for this A.I. so we can visualise the sound that
                    // the A.I. heard.
                    props.closestPoint = closestPoint;
                    m_debugSounds.push_back(SoundPropertiesDebug(props));

                    AZStd::any value(props);
                    SendGameplayEvent(*it, m_broadcastSoundEventName, value);
                }
            }
        }
        else
        {
            props.closestPoint = props.origin;
            AZStd::any value(props);
            for (AZStd::list<AZ::EntityId>::iterator it = m_registeredEntities.begin(); it != m_registeredEntities.end(); ++it)
            {
                AZ::TransformBus::EventResult(tm, *it, &AZ::TransformInterface::GetWorldTM);
                float distanceSq = (tm.GetTranslation() - props.closestPoint).GetLengthSq();
                if (distanceSq < rangeSq)
                {
                    SendGameplayEvent(*it, m_broadcastSoundEventName, value);
                }
            }

            m_debugSounds.push_back(SoundPropertiesDebug(props));
        }
    }

    void AISoundManagerSystemComponent::RegisterEntity(const AZ::EntityId& entityId)
    {
        // Make sure we don't add the same entity twice.
        AZStd::list<AZ::EntityId>::iterator it;
        for (it = m_registeredEntities.begin(); it != m_registeredEntities.end(); ++it)
            if ((*it) == entityId)
                break;

        if (it == m_registeredEntities.end())
            m_registeredEntities.push_back(entityId);
    }

    void AISoundManagerSystemComponent::UnregisterEntity(const AZ::EntityId& entityId)
    {
        bool removed = false;
        for (AZStd::list<AZ::EntityId>::iterator it = m_registeredEntities.begin(); it != m_registeredEntities.end(); ++it)
        {
            if ((*it) == entityId)
            {
                m_registeredEntities.erase(it);
                removed = true;
                break;
            }
        }

        // Don't bother warning about repeated unregister events as this component is persistent
        // between runs in the editor so it's better to waste a couple of cycles ensuring something
        // is unregistered than have it accidentally receive messages the next time the game is run.
        //AZ_Warning("StarterGame", removed, "AISoundManagerSystemComponent: Attempted to unregister an entity that wasn't registered.");
    }

    bool AISoundManagerSystemComponent::IsLineSound(const SoundProperties& props) const
    {
        bool isLine = false;
        if (props.type == SoundTypes::ST_WhizzPast)
        {
            isLine = true;
        }

        return isLine;
    }

    void AISoundManagerSystemComponent::SendGameplayEvent(const AZ::EntityId& entityId, const AZStd::string& eventName, const AZStd::any& value) const
    {
        AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(entityId, eventName.c_str(), StarterGameUtility::GetUuid("float"));
        AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, value);
    }

    AZ::Vector3 AISoundManagerSystemComponent::GetClosestPointOnLineSegment(const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& p) const
    {
        AZ::Vector3 AB = b - a;
        AZ::Vector3 AP = p - a;

        float ABLenSq = AB.GetLengthSq();
        float ABAPProduct = AP.Dot(AB);
        float ABRatio = ABAPProduct / ABLenSq;

        if (ABRatio < 0.0f)
            return a;
        else if (ABRatio > 1.0f)
            return b;
        else
            return a + AB * ABRatio;
    }

} // namespace StarterGameGem


