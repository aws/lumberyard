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

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>

#include <foundation/PxVec4.h>

namespace NvCloth
{
    extern const int InvalidIndex;

    struct Collider
    {
        AZ::Transform m_offsetTransform = AZ::Transform::CreateIdentity();
        AZ::Transform m_currentModelSpaceTransform = AZ::Transform::CreateIdentity();

        int m_jointIndex = InvalidIndex;
    };

    struct SphereCollider
        : public Collider
    {
        float m_radius = 0.0f;

        int m_nvSphereIndex = InvalidIndex;
    };

    struct CapsuleCollider
        : public Collider
    {
        float m_height = 0.0f;
        float m_radius = 0.0f;

        int m_nvCapsuleIndex = InvalidIndex;
        int m_nvSphereAIndex = InvalidIndex;
        int m_nvSphereBIndex = InvalidIndex;
    };

    class ActorClothColliders
    {
    public:
        AZ_TYPE_INFO(ActorClothColliders, "{EA2D9B6A-2493-4B6A-972E-BB639E16798E}");

        static AZStd::unique_ptr<ActorClothColliders> Create(AZ::EntityId entityId);

        explicit ActorClothColliders(AZ::EntityId entityId);

        // Updates the colliders' transforms with the current pose of the actor.
        void Update();

        const AZStd::vector<SphereCollider>& GetSphereColliders() const;
        const AZStd::vector<CapsuleCollider>& GetCapsuleColliders() const;

        const AZStd::vector<physx::PxVec4>& GetSpheres() const;
        const AZStd::vector<uint32_t>& GetCapsuleIndices() const;

    private:
        void UpdateSphere(const SphereCollider& sphere);
        void UpdateCapsule(const CapsuleCollider& capsule);

        AZ::EntityId m_entityId;

        // Configuration data of spheres and capsules, describing their shape and transforms relative to joints.
        AZStd::vector<SphereCollider> m_sphereColliders;
        AZStd::vector<CapsuleCollider> m_capsuleColliders;

        // The current positions and scales of sphere (and capsule) colliders.
        // Every update, these positions are computed with the current pose of the actor.
        AZStd::vector<physx::PxVec4> m_nvSpheres;

        // The sphere collider indices associated with capsules.
        AZStd::vector<uint32_t> m_nvCapsuleIndices;
    };
} // namespace NvCloth
