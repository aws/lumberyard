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
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Physics/WindVolumeRequestBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

struct IPhysicalEntity;

namespace LmbrCentral
{
    /**
     * Configuration for the WindVolume
     * @see WindVolumeRequests
     */
    struct WindVolumeConfiguration
    {
        AZ_CLASS_ALLOCATOR(WindVolumeConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(WindVolumeConfiguration, "{0946220B-77EA-49EF-8DC0-3825CBCB495F}");
        static void Reflect(AZ::ReflectContext* context);

        float m_falloff = 0.0f;
        float m_speed = 20.0f;
        float m_airResistance = 1.0f;
        float m_airDensity = 0.0f;
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisX();
    };

    class WindVolume
        : public WindVolumeRequestBus::Handler
        , protected AZ::TransformNotificationBus::MultiHandler
        , protected ShapeComponentNotificationsBus::Handler
    {
    public:
        virtual WindVolumeConfiguration& GetConfiguration() = 0;

        // WindVolumeRequestBus
        void SetFalloff(float falloff) override;
        float GetFalloff() override;
        void SetSpeed(float speed) override;
        float GetSpeed() override;
        void SetAirResistance(float airResistance) override;
        float GetAirResistance() override;
        void SetAirDensity(float airDensity) override;
        float GetAirDensity() override;
        void SetWindDirection(const AZ::Vector3& direction) override;
        const AZ::Vector3& GetWindDirection() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons reason) override;

        void Activate(const AZ::EntityId& entityId);
        void Deactivate();

    protected:
        void DestroyWindVolume();
        void CreateWindVolume();
        void GetSizeFromShape();

        bool m_isSphere;
        AZ::Vector3 m_size;
    private:
        AZ::EntityId m_entityId;                        ///< The id of the entity the windvolume is attached to
        AZStd::shared_ptr<IPhysicalEntity> m_physics;   ///< Underlying physics object
    };
} // namespace LmbrCentral
