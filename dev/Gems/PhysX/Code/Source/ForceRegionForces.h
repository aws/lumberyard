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

#include <AzCore/Math/Spline.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <PhysX/ForceRegionComponentBus.h>

namespace PhysX
{
    /// Parameters of an entity in the force region.
    /// Used to calculate final force.
    struct EntityParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Vector3 m_velocity;
        AZ::Aabb m_aabb;
        float m_mass;
    };

    /// Parameters of the force region.
    /// Used to calculate final force.
    struct RegionParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Quaternion m_rotation;
        AZ::Vector3 m_scale;
        AZ::SplinePtr m_spline;
        AZ::Aabb m_aabb;
    };

    /// Class for a world space force exerted on bodies in a force region.
    class ForceWorldSpace : public BaseForce
        , ForceWorldSpaceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceWorldSpace, AZ::SystemAllocator, 0);
        AZ_RTTI(ForceWorldSpace, "{A6C17DD3-7A09-4BC7-8ACC-C0BD04EA8F7C}", BaseForce);
        ForceWorldSpace() = default;
        ForceWorldSpace(const AZ::Vector3& direction, float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override 
        { 
            BusConnect(entityId); 
            ForceWorldSpaceRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override 
        { 
            ForceWorldSpaceRequestBus::Handler::BusDisconnect();
            BusDisconnect(); 
        }

        // ForceWorldSpaceRequestBus
        void SetDirection(const AZ::Vector3& direction) override { m_direction = direction; }
        AZ::Vector3 GetDirection() override { return m_direction; }
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.f;
    };

    /// Class for a local space force exerted on bodies in a force region.
    class ForceLocalSpace : public BaseForce
        , ForceLocalSpaceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceLocalSpace, AZ::SystemAllocator, 0);
        AZ_RTTI(ForceLocalSpace, "{F0EAFB7C-1BC7-4497-99AE-ECBF7169AB81}", BaseForce);
        ForceLocalSpace() = default;
        ForceLocalSpace(const AZ::Vector3& direction, float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BusConnect(entityId);
            ForceLocalSpaceRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceLocalSpaceRequestBus::Handler::BusDisconnect();
            BusDisconnect();
        }

        // ForceLocalSpaceRequestBus
        void SetDirection(const AZ::Vector3& direction) override { m_direction = direction; }
        AZ::Vector3 GetDirection() override { return m_direction; }
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.0f;
    };

    /// Class for a point force exerted on bodies in a force region. 
    /// Bodies in a force region with a point force are repelled away from the center of the force region.
    class ForcePoint : public BaseForce
        , ForcePointRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForcePoint, AZ::SystemAllocator, 0);
        AZ_RTTI(ForcePoint, "{3F8ABEAC-6972-4845-A131-EA9831029E68}", BaseForce);
        ForcePoint() = default;
        explicit ForcePoint(float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BusConnect(entityId);
            ForcePointRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForcePointRequestBus::Handler::BusDisconnect();
            BusDisconnect();
        }

        // ForcePointRequestBus
        void SetMagnitude(float magnitude) override { m_magnitude = magnitude; }
        float GetMagnitude() override { return m_magnitude; }

    private:
        float m_magnitude = 1.0f;
    };

    /// Class for a spline follow force.
    /// Bodies in a force region with a spline follow force tend to follow the path of the spline.
    class ForceSplineFollow : public BaseForce
        , ForceSplineFollowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceSplineFollow, AZ::SystemAllocator, 0);
        AZ_RTTI(ForceSplineFollow, "{AB397D4C-62DA-43F0-8CF1-9BD9013129BB}", BaseForce);
        ForceSplineFollow() = default;
        ForceSplineFollow(float dampingRatio
            , float frequency
            , float targetSpeed
            , float lookAhead);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override;
        void Deactivate() override
        {
            ForceSplineFollowRequestBus::Handler::BusDisconnect();
            BusDisconnect();
        }

        // ForceSplineFollowRequestBus
        void SetDampingRatio(float ratio) override { m_dampingRatio = ratio; }
        float GetDampingRatio() override { return m_dampingRatio; }
        void SetFrequency(float frequency) override { m_frequency = frequency; }
        float GetFrequency() override { return m_frequency; }
        void SetTargetSpeed(float targetSpeed) override { m_targetSpeed = targetSpeed; }
        float GetTargetSpeed() override { return m_targetSpeed; }
        void SetLookAhead(float lookAhead) override { m_lookAhead = lookAhead; }
        float GetLookAhead() override { return m_lookAhead; }

    private:
        float m_dampingRatio = 1.0f;
        float m_frequency = 3.0f;
        float m_targetSpeed = 1.0f;
        float m_lookAhead = 0.0f;
        bool m_loggedMissingSplineWarning = false;
    };

    /// Class for a simple drag force.
    class ForceSimpleDrag : public BaseForce
        , ForceSimpleDragRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceSimpleDrag, AZ::SystemAllocator, 0);
        AZ_RTTI(ForceSimpleDrag, "{56A4E393-4724-4486-B4C0-E02C4EF1534C}", BaseForce);
        ForceSimpleDrag() = default;
        ForceSimpleDrag(float dragCoefficient, float volumeDensity);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BusConnect(entityId);
            ForceSimpleDragRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceSimpleDragRequestBus::Handler::BusDisconnect();
            BusDisconnect();
        }

        // ForceSimpleDragRequests
        void SetDensity(float density) override { m_volumeDensity = density; }
        float GetDensity() override { return m_volumeDensity; }

    private:
        //Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
        float m_dragCoefficient = 0.47f;
        float m_volumeDensity = 1.0f;
    };

    /// Class for a linear damping force.
    class ForceLinearDamping : public BaseForce
        , ForceLinearDampingRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceLinearDamping, AZ::SystemAllocator, 0);
        AZ_RTTI(ForceLinearDamping, "{7EECFBD7-0942-4960-A54A-7582159CFFA3}", BaseForce);
        ForceLinearDamping() = default;
        explicit ForceLinearDamping(float damping);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override 
        { 
            BusConnect(entityId); 
            ForceLinearDampingRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override 
        {
            ForceLinearDampingRequestBus::Handler::BusDisconnect();
            BusDisconnect(); 
        }

        // ForceLinearDampingRequests
        void SetDamping(float damping) override { m_damping = damping; }
        float GetDamping() override { return m_damping; }

    private:
        float m_damping = 1.0f;
    };
}