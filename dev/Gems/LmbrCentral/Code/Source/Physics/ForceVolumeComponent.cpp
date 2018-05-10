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
#include "ForceVolumeComponent.h"
#include "ForceVolume.h"
#include "ForceVolumeForces.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <MathConversion.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>

namespace LmbrCentral
{
    void ForceVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        ForceVolume::Reflect(context);
        WorldSpaceForce::Reflect(context);
        LocalSpaceForce::Reflect(context);
        SplineFollowForce::Reflect(context);
        SimpleDragForce::Reflect(context);
        LinearDampingForce::Reflect(context);
        PointForce::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ForceVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("ForceVolume", &ForceVolumeComponent::m_forceVolume)
                ->Field("DebugForces", &ForceVolumeComponent::m_debugForces)
            ;
        }
    }

    ForceVolumeComponent::ForceVolumeComponent(const ForceVolume& forceVolume, bool debug)
        : m_forceVolume(forceVolume)
        , m_debugForces(debug)
    {

    }

    void ForceVolumeComponent::OnTriggerAreaEntered(AZ::EntityId entityId)
    {
        // Ignore self
        if (entityId == GetEntityId())
        {
            return;
        }

        auto newEntityHasPhysics = AzFramework::PhysicsComponentRequestBus::FindFirstHandler(entityId) != nullptr;
        if (newEntityHasPhysics)
        {
            m_entities.emplace_back(entityId);
        }
    }

    void ForceVolumeComponent::OnTriggerAreaExited(AZ::EntityId entityId)
    {        
        auto found = AZStd::find(m_entities.begin(), m_entities.end(), entityId);
        if (found != m_entities.end())
        {
            m_entities.erase(found);
        }
    }

    void DrawLine(const AZ::Vector3& begin, const AZ::Vector3& end, const ColorF& color)
    {
        auto cryBegin = AZVec3ToLYVec3(begin);
        auto cryEnd = AZVec3ToLYVec3(end);
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(cryBegin, color, cryEnd, color);
    }

    void ForceVolumeComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        for (auto entityId : m_entities)
        {
            EntityParams entity = ForceVolumeUtil::CreateEntityParams(entityId);

            auto netForce = m_forceVolume.CalculateNetForce(entity);
                
            if (!netForce.IsZero())
            {
                netForce *= deltaTime;
                AzFramework::PhysicsComponentRequestBus::Event(entityId, &AzFramework::PhysicsComponentRequests::AddImpulse, netForce);
                if (m_debugForces)
                {
                    DrawDebugForceForEntity(entity, netForce);
                }
            }
        }
    }

    void ForceVolumeComponent::Activate()
    {
        TriggerAreaNotificationBus::Handler::BusConnect(m_entity->GetId());
        AZ::TickBus::Handler::BusConnect();
        m_forceVolume.Activate(GetEntityId());
    }

    void ForceVolumeComponent::Deactivate()
    {
        m_forceVolume.Deactivate();
        AZ::TickBus::Handler::BusDisconnect();
        TriggerAreaNotificationBus::Handler::BusDisconnect();
    }

    void ForceVolumeComponent::DrawDebugForceForEntity(const EntityParams& entity, const AZ::Vector3& netForce)
    {
        // Debug line length should be larger than the bounding box so it's visible for 
        // various mesh shapes and sizes
        AZ::VectorFloat radius;
        AZ::Vector3 center;
        entity.m_aabb.GetAsSphere(center, radius);

        AZ::Vector3 debugLineLength = netForce;
        debugLineLength.SetLength(static_cast<float>(radius) * 2.0f);
        DrawLine(entity.m_position, entity.m_position + debugLineLength, Col_Blue);
    }
} // namespace LmbrCentral
