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

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Physics/WindVolumeRequestBus.h>

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

        bool m_ellipsoidal = false;
        float m_falloffInner = 0.0f;
        float m_speed = 20.0f;
        float m_airResistance = 1.0f;
        float m_airDensity = 1.0f;
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisX();
        AZ::Vector3 m_size = AZ::Vector3(10.f, 10.f, 10.f);
    };

    /**
     * WindVolume component for a wind volume
     */
    class WindVolumeComponent
        : public AZ::Component
        , public WindVolumeRequestBus::Handler
        , protected AZ::TransformNotificationBus::MultiHandler
    {
    public:
        AZ_COMPONENT(WindVolumeComponent, "{7E97DA82-94EB-46B7-B5EB-8B72727E3D7E}");
        static void Reflect(AZ::ReflectContext* context);

        WindVolumeComponent() = default;
        explicit WindVolumeComponent(const WindVolumeConfiguration& configuration);
        ~WindVolumeComponent() override = default;
        
        ////////////////////////////////
        /// WindVolumeRequestBus        
        void SetEllipsoidal(bool ellipsoidal) override;
        bool GetEllipsoidal() override;
        void SetFalloffInner(float falloffInner) override;
        float GetFalloffInner() override;
        void SetSpeed(float speed) override;
        float GetSpeed() override;
        void SetAirResistance(float airResistance) override;
        float GetAirResistance() override;
        void SetAirDensity(float airDensity) override;
        float GetAirDensity() override;
        void SetWindDirection(const AZ::Vector3& direction) override;
        const AZ::Vector3& GetWindDirection() override;
        void SetVolumeSize(const AZ::Vector3& size) override;
        const AZ::Vector3& GetVolumeSize() override;
        ////////////////////////////////

    protected:
        friend class EditorWindVolumeComponent;
        static float GetSphereVolumeRadius(const WindVolumeConfiguration&);

        //! Update physics to match new position
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void Activate() override;
        void Deactivate() override;
        void DestroyWindVolume();
        void CreateWindVolume();

        WindVolumeConfiguration m_configuration;        ///< Configuration of the wind volume
        AZStd::shared_ptr<IPhysicalEntity> m_physics;   ///< Underlying physics object
    };
} // namespace LmbrCentral
