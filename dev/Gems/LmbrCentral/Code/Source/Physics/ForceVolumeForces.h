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

#include <LmbrCentral/Physics/ForceVolumeRequestBus.h>

namespace LmbrCentral
{
    /**
     * Applies a force to objects in world space
     * World space forces don't rotate with the volume orientation
     */
    class WorldSpaceForce 
        : public Force
        , public WorldSpaceForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldSpaceForce, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldSpaceForce, "{E4B7B2E1-F9E0-450E-A0A6-98EC75D20FEA}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // Force
        void Activate(AZ::EntityId entityId) override { BusConnect(entityId); }
        void Deactivate() override { BusDisconnect(); }

        // WorldSpaceForceRequestBus
        void SetDirection(const AZ::Vector3& direction) override { m_direction = direction; }
        const AZ::Vector3& GetDirection() override { return m_direction; }
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.f;
    };

    /**
     * Applies a force to objects in local space
     * Localspace forces are applied relative to the volumes orientation
     */
    class LocalSpaceForce 
        : public Force
        , public LocalSpaceForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalSpaceForce, AZ::SystemAllocator, 0);
        AZ_RTTI(LocalSpaceForce, "{8B720522-5A8D-4726-ACA2-BE93099C5398}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // FOrce
        void Activate(AZ::EntityId entityId) override { BusConnect(entityId); }
        void Deactivate() override { BusDisconnect(); }

        // LocalSpaceForceRequestBus
        void SetDirection(const AZ::Vector3& direction) override { m_direction = direction; }
        const AZ::Vector3& GetDirection() override { return m_direction; }
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.f;
    };

    /**
     * Applies a force in a direction relative to the center of the volume
     */
    class PointForce 
        : public Force
        , public PointForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PointForce, AZ::SystemAllocator, 0);
        AZ_RTTI(PointForce, "{0ADF9ED9-130D-4375-8AD1-6676E9F922E0}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // Force
        void Activate(AZ::EntityId entityId) override { BusConnect(entityId); }
        void Deactivate() override { BusDisconnect(); }

        // PointForceRequestBus
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        float m_magnitude = 1.f;
    };

    /**
     * Applies a force to make objects follow a spline.
     * Uses a damped pd controller.
     */
    class SplineFollowForce 
        : public Force
        , public SplineFollowForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SplineFollowForce, AZ::SystemAllocator, 0);
        AZ_RTTI(SplineFollowForce, "{957BD324-6BFA-492A-B325-D0E7AC9545CC}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // Force
        void Activate(AZ::EntityId entityId) override;
        void Deactivate() override { BusDisconnect(); }

        // SplineFollowForceRequestBus
        void SetDampingRatio(float ratio) override { m_dampingRatio = ratio; }
        float GetDampingRatio() override { return m_dampingRatio; }
        void SetFrequency(float frequency) override { m_frequency = frequency; }
        float GetFrequency() override { return m_frequency; }
        void SetTargetSpeed(float targetSpeed) override { m_targetSpeed = targetSpeed; }
        float GetTargetSpeed() override { return m_targetSpeed; }
        void SetLookAhead(float lookAhead) override { m_lookAhead = lookAhead; }
        float GetLookAhead() override { return m_lookAhead; }

    private:
        float m_dampingRatio = 1.f;
        float m_frequency = 3.f;
        float m_targetSpeed = 1;
        float m_lookAhead = 0.f;
        bool m_loggedMissingSplineWarning = false;
    };

    /**
     * Applies a force that simulates drag
     * It approximates entities as a sphere when calculating drag
     */
    class SimpleDragForce 
        : public Force
        , public SimpleDragForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SimpleDragForce, AZ::SystemAllocator, 0);
        AZ_RTTI(SimpleDragForce, "{233E4D44-A8A3-472D-A1CD-E0C31F1353B7}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // Force
        void Activate(AZ::EntityId entityId) override { BusConnect(entityId); }
        void Deactivate() override { BusDisconnect(); }

        // SimpleDragForceRequests
        void SetDensity(float density) override { m_volumeDensity = density; }
        float GetDensity() override { return m_volumeDensity; }

    private:
        //Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
        float m_dragCoefficient = 0.47f;
        float m_volumeDensity = 1.0f;
    };

    /**
     * Applies a force that opposes an objects velocity
     */
    class LinearDampingForce 
        : public Force
        , public LinearDampingForceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LinearDampingForce, AZ::SystemAllocator, 0);
        AZ_RTTI(LinearDampingForce, "{A9021A28-BAE4-49F9-B39F-D99E06B25314}", Force);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const VolumeParams& volume) override;

        // Force
        void Activate(AZ::EntityId entityId) override { BusConnect(entityId); }
        void Deactivate() override { BusDisconnect(); }

        // LinearDampingForceRequests
        void SetDamping(float damping) override { m_damping = damping; }
        float GetDamping() override { return m_damping; }

    private:
        float m_damping = 1.f;
    };
}