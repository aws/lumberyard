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

#include "StdAfx.h"
#include "WindVolumeComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IPhysics.h>
#include <MathConversion.h>
#include "I3DEngine.h"

namespace LmbrCentral
{
    void WindVolumeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WindVolumeConfiguration>()
                ->Version(1)
                ->Field("Ellipsoidal", &WindVolumeConfiguration::m_ellipsoidal)
                ->Field("FalloffInner", &WindVolumeConfiguration::m_falloffInner)
                ->Field("Speed", &WindVolumeConfiguration::m_speed)
                ->Field("Air Resistance", &WindVolumeConfiguration::m_airResistance)
                ->Field("Air Density", &WindVolumeConfiguration::m_airDensity)
                ->Field("Direction", &WindVolumeConfiguration::m_direction)
                ->Field("Size", &WindVolumeConfiguration::m_size)
            ;
        }
    }

    void WindVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        WindVolumeConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WindVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &WindVolumeComponent::m_configuration)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WindVolumeRequestBus>("WindVolumeRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Event("SetEllipsoidal", &WindVolumeRequestBus::Events::SetEllipsoidal)
                ->Event("GetEllipsoidal", &WindVolumeRequestBus::Events::GetEllipsoidal)
                ->Event("SetFalloffInner", &WindVolumeRequestBus::Events::SetFalloffInner)
                ->Event("GetFalloffInner", &WindVolumeRequestBus::Events::GetFalloffInner)
                ->Event("SetSpeed", &WindVolumeRequestBus::Events::SetSpeed)
                ->Event("GetSpeed", &WindVolumeRequestBus::Events::GetSpeed)
                ->Event("SetAirResistance", &WindVolumeRequestBus::Events::SetAirResistance)
                ->Event("GetAirResistance", &WindVolumeRequestBus::Events::GetAirResistance)
                ->Event("SetAirDensity", &WindVolumeRequestBus::Events::SetAirDensity)
                ->Event("GetAirDensity", &WindVolumeRequestBus::Events::GetAirDensity)
                ->Event("SetWindDirection", &WindVolumeRequestBus::Events::SetWindDirection)
                ->Event("GetWindDirection", &WindVolumeRequestBus::Events::GetWindDirection)
                ->Event("SetVolumeSize", &WindVolumeRequestBus::Events::SetVolumeSize)
                ->Event("GetVolumeSize", &WindVolumeRequestBus::Events::GetVolumeSize)
            ;
        }
    }

    WindVolumeComponent::WindVolumeComponent(const WindVolumeConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void WindVolumeComponent::SetEllipsoidal(bool ellipsoidal)
    {
        DestroyWindVolume();
        m_configuration.m_ellipsoidal = ellipsoidal;
        CreateWindVolume();
    }

    bool WindVolumeComponent::GetEllipsoidal()
    {
        return m_configuration.m_ellipsoidal;
    }

    void WindVolumeComponent::SetFalloffInner(float falloffInner)
    {
        DestroyWindVolume();
        m_configuration.m_falloffInner = falloffInner;
        CreateWindVolume();
    }

    float WindVolumeComponent::GetFalloffInner()
    {
        return m_configuration.m_falloffInner;
    }

    void WindVolumeComponent::SetSpeed(float speed)
    {
        DestroyWindVolume();
        m_configuration.m_speed = speed;
        CreateWindVolume();
    }

    float WindVolumeComponent::GetSpeed()
    {
        return m_configuration.m_speed;
    }

    void WindVolumeComponent::SetAirResistance(float airResistance)
    {
        DestroyWindVolume();
        m_configuration.m_airResistance = airResistance;
        CreateWindVolume();
    }

    float WindVolumeComponent::GetAirResistance()
    {
        return m_configuration.m_airResistance;
    }

    void WindVolumeComponent::SetAirDensity(float airDensity)
    {
        DestroyWindVolume();
        m_configuration.m_airDensity = airDensity;
        CreateWindVolume();
    }

    float WindVolumeComponent::GetAirDensity()
    {
        return m_configuration.m_airDensity;
    }

    void WindVolumeComponent::SetWindDirection(const AZ::Vector3& direction)
    {
        DestroyWindVolume();
        m_configuration.m_direction = direction;
        CreateWindVolume();
    }

    const AZ::Vector3& WindVolumeComponent::GetWindDirection()
    {
        return m_configuration.m_direction;
    }

    void WindVolumeComponent::SetVolumeSize(const AZ::Vector3& size)
    {
        DestroyWindVolume();
        m_configuration.m_size = size;
        CreateWindVolume();
    }

    const AZ::Vector3& WindVolumeComponent::GetVolumeSize()
    {
        return m_configuration.m_size;
    }

    float WindVolumeComponent::GetSphereVolumeRadius(const WindVolumeConfiguration& configuration)
    {
        // Ported from cryphys. Unsure why this constant is used
        static const float EULERS_CONSTANT = 0.577f;
        return configuration.m_size.GetLength() * EULERS_CONSTANT;
    }

    void WindVolumeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        // Don't care about transform changes on descendants.
        if (*AZ::TransformNotificationBus::GetCurrentBusId() != GetEntityId())
        {
            return;
        }

        // Update physics position to match transform
        pe_params_pos positionParameters;
        Matrix34 geomTransform = AZTransformToLYTransform(world);
        positionParameters.pMtx3x4 = &geomTransform;
        m_physics->SetParams(&positionParameters);
    }

    void WindVolumeComponent::Activate()
    {
        CreateWindVolume();
        WindVolumeRequestBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
    }

    void WindVolumeComponent::Deactivate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        WindVolumeRequestBus::Handler::BusDisconnect();
        DestroyWindVolume();
    }

    void WindVolumeComponent::DestroyWindVolume()
    {
        m_physics.reset();
    }

    void WindVolumeComponent::CreateWindVolume()
    {
        if (m_physics != nullptr)
        {
            return;
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        Vec3 pos = AZVec3ToLYVec3(transform.GetTranslation());
        auto scale = transform.ExtractScaleExact();
        Quat rot = AZQuaternionToLYQuaternion(AZ::Quaternion::CreateFromTransform(transform));

        float fEntityScale = scale.GetX();
        IPhysicalEntity* rawPhysics;
        if (m_configuration.m_ellipsoidal)
        {
            const auto EULERS_CONSTANT = 0.577f;
            primitives::sphere geomSphere;
            geomSphere.center = Vec3_Zero;
            geomSphere.r = GetSphereVolumeRadius(m_configuration); 
            auto geom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &geomSphere);
            rawPhysics = gEnv->pPhysicalWorld->AddArea(geom, pos, rot, fEntityScale);
        }
        else
        {
            primitives::box geomBox;
            geomBox.Basis.SetIdentity();
            geomBox.center = Vec3_Zero;
            geomBox.size = AZVec3ToLYVec3(m_configuration.m_size);
            geomBox.bOriented = 0;
            auto geom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &geomBox);
            rawPhysics = gEnv->pPhysicalWorld->AddArea(geom, pos, rot, fEntityScale);
        }

        int uniform;
        Vec3 wind;
        if (m_configuration.m_direction.IsZero())
        {
            uniform = 0;
            wind.x = 0;
            wind.y = 0;
            wind.z = m_configuration.m_speed;
        }
        else
        {
            uniform = 2;
            wind.x = m_configuration.m_direction.GetX() * m_configuration.m_speed;
            wind.y = m_configuration.m_direction.GetY() * m_configuration.m_speed;
            wind.z = m_configuration.m_direction.GetZ() * m_configuration.m_speed;
        }

        rawPhysics->AddRef();
        m_physics.reset(rawPhysics,
            [](IPhysicalEntity* physicalEntity)
            {
                physicalEntity->Release();
                gEnv->pPhysicalWorld->DestroyPhysicalEntity(physicalEntity);
            });

        pe_params_flags pf;
        pf.flagsOR = pef_log_state_changes | pef_log_poststep;
        m_physics->SetParams(&pf);

        pe_params_area area;
        area.bUniform = uniform;
        area.falloff0 = m_configuration.m_falloffInner;
        m_physics->SetParams(&area);

        pe_params_buoyancy bounancy;
        bounancy.waterFlow = wind;
        bounancy.iMedium = 1;
        bounancy.waterDensity = m_configuration.m_airDensity;
        bounancy.waterResistance = m_configuration.m_airResistance;
        bounancy.waterPlane.n = Vec3(0, 0, -1);
        bounancy.waterPlane.origin = Vec3(0, 0, gEnv->p3DEngine->GetWaterLevel());
        m_physics->SetParams(&bounancy);
    }
} // namespace LmbrCentral
