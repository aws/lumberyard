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

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <API/RagdollNode.h>

namespace PhysXCharacters
{
    using ParentIndices = AZStd::vector<size_t>;

    /// PhysX specific implementation of generic physics API Ragdoll class.
    class Ragdoll
        : public Physics::Ragdoll
    {
    public:
        friend class RagdollComponent;

        AZ_CLASS_ALLOCATOR(Ragdoll, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Ragdoll, "{55D477B5-B922-4D3E-89FE-7FB7B9FDD635}", Physics::Ragdoll);
        static void Reflect(AZ::ReflectContext* context);

        Ragdoll();
        ~Ragdoll() = default;

        void AddNode(const AZStd::shared_ptr<RagdollNode>& node);
        void SetParentIndices(const ParentIndices& parentIndices);
        void SetRootIndex(size_t nodeIndex);
        physx::PxRigidDynamic* GetPxRigidDynamic(size_t nodeIndex) const;
        physx::PxTransform GetRootPxTransform() const;

        // Physics::Ragdoll
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        bool IsSimulated() override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        AZStd::shared_ptr<Physics::RagdollNode> GetNode(size_t nodeIndex) const override;
        size_t GetNumNodes() const override;

        // Physics::WorldBody
        AZ::EntityId GetEntityId() const override;
        Physics::World* GetWorld() const override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        void RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

    private:
        AZStd::vector<AZStd::shared_ptr<RagdollNode>> m_nodes;
        ParentIndices m_parentIndices;
        AZ::Outcome<size_t> m_rootIndex = AZ::Failure();
        bool m_isSimulated;
    };
} // namespace PhysXCharacters
