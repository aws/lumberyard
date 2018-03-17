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
#include "WindVolumeComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IPhysics.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <MathConversion.h>
#include "I3DEngine.h"

namespace LmbrCentral
{
    void WindVolume::SetFalloff(float falloff)
    {
        DestroyWindVolume();
        GetConfiguration().m_falloff = falloff;
        CreateWindVolume();
    }

    float WindVolume::GetFalloff()
    {
        return GetConfiguration().m_falloff;
    }

    void WindVolume::SetSpeed(float speed)
    {
        DestroyWindVolume();
        GetConfiguration().m_speed = speed;
        CreateWindVolume();
    }

    float WindVolume::GetSpeed()
    {
        return GetConfiguration().m_speed;
    }

    void WindVolume::SetAirResistance(float airResistance)
    {
        DestroyWindVolume();
        GetConfiguration().m_airResistance = airResistance;
        CreateWindVolume();
    }

    float WindVolume::GetAirResistance()
    {
        return GetConfiguration().m_airResistance;
    }

    void WindVolume::SetAirDensity(float airDensity)
    {
        DestroyWindVolume();
        GetConfiguration().m_airDensity = airDensity;
        CreateWindVolume();
    }

    float WindVolume::GetAirDensity()
    {
        return GetConfiguration().m_airDensity;
    }

    void WindVolume::SetWindDirection(const AZ::Vector3& direction)
    {
        DestroyWindVolume();
        GetConfiguration().m_direction = direction;
        CreateWindVolume();
    }

    const AZ::Vector3& WindVolume::GetWindDirection()
    {
        return GetConfiguration().m_direction;
    }

    void WindVolume::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        // Don't care about transform changes on descendants.
        if (*AZ::TransformNotificationBus::GetCurrentBusId() != m_entityId)
        {
            return;
        }

        // Update physics position to match transform
        pe_params_pos positionParameters;
        Matrix34 geomTransform = AZTransformToLYTransform(world);
        positionParameters.pMtx3x4 = &geomTransform;
        m_physics->SetParams(&positionParameters);
    }

    void WindVolume::OnShapeChanged(ShapeChangeReasons reason)
    {
        if (reason == ShapeChangeReasons::ShapeChanged)
        {
            GetSizeFromShape();
            
            WindVolume::DestroyWindVolume();
            WindVolume::CreateWindVolume();
        }
    }

    void WindVolume::Activate(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
        GetSizeFromShape();
        WindVolume::CreateWindVolume();
        WindVolumeRequestBus::Handler::BusConnect(m_entityId);
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(m_entityId);
    }

    void WindVolume::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        WindVolumeRequestBus::Handler::BusDisconnect();
        WindVolume::DestroyWindVolume();
    }

    void WindVolume::DestroyWindVolume()
    {
        m_physics.reset();
    }

    void WindVolume::CreateWindVolume()
    {
        if (m_physics != nullptr)
        {
            return;
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        Vec3 pos = AZVec3ToLYVec3(transform.GetTranslation());
        
        auto scale = transform.ExtractScaleExact();
        Quat rot = AZQuaternionToLYQuaternion(AZ::Quaternion::CreateFromTransform(transform));

        float fEntityScale = scale.GetX();
        IPhysicalEntity* rawPhysics;
        if (m_isSphere)
        {
            primitives::sphere geomSphere;
            geomSphere.center = Vec3_Zero;
            geomSphere.r = m_size.GetX();
            auto geom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &geomSphere);
            rawPhysics = gEnv->pPhysicalWorld->AddArea(geom, pos, rot, fEntityScale);
        }
        else
        {
            primitives::box geomBox;
            geomBox.Basis.SetIdentity();
            geomBox.center = Vec3_Zero;
            geomBox.size = AZVec3ToLYVec3(m_size * 0.5f);
            geomBox.bOriented = 1;
            Quat rot2 = rot;
            auto geom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &geomBox);
            rawPhysics = gEnv->pPhysicalWorld->AddArea(geom, pos, rot2, fEntityScale);
        }

        int uniform;
        Vec3 wind;
        if (GetConfiguration().m_direction.IsZero())
        {
            uniform = 0;
            wind.x = 0;
            wind.y = 0;
            wind.z = GetConfiguration().m_speed;
        }
        else
        {
            uniform = 2;
            wind.x = GetConfiguration().m_direction.GetX() * GetConfiguration().m_speed;
            wind.y = GetConfiguration().m_direction.GetY() * GetConfiguration().m_speed;
            wind.z = GetConfiguration().m_direction.GetZ() * GetConfiguration().m_speed;
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
        area.falloff0 = GetConfiguration().m_falloff;
        m_physics->SetParams(&area);

        pe_params_buoyancy bounancy;
        bounancy.waterFlow = wind;
        bounancy.iMedium = 1;
        bounancy.waterDensity = GetConfiguration().m_airDensity;
        bounancy.waterResistance = GetConfiguration().m_airResistance;
        bounancy.waterPlane.n = Vec3(0, 0, -1);
        bounancy.waterPlane.origin = Vec3(0, 0, gEnv->p3DEngine->GetWaterLevel());
        m_physics->SetParams(&bounancy);
    }

    void WindVolume::GetSizeFromShape()
    {        
        if (auto boxShape = BoxShapeComponentRequestsBus::FindFirstHandler(m_entityId))
        {
            m_size = boxShape->GetBoxDimensions();
            m_isSphere = false;
        }
        else if (auto sphereShape = SphereShapeComponentRequestsBus::FindFirstHandler(m_entityId))
        {
            float radius = sphereShape->GetSphereConfiguration().GetRadius();
            m_size = AZ::Vector3(radius, radius, radius);
            m_isSphere = true;
        }
        else
        {
            AZ_Assert(false, "No supported shape attached. Only Box and Sphere are supported by the wind volume");
        }
    }
} // namespace LmbrCentral
