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

#include <PhysXCharacters_precompiled.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <API/Ragdoll.h>
#include <API/Utils.h>
#include <Include/PhysXCharacters/NativeTypeIdentifiers.h>

namespace PhysXCharacters
{
    // PhysXCharacters::Ragdoll
    void Ragdoll::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Ragdoll>()
                ->Version(1)
                ;
        }
    }

    void Ragdoll::AddNode(const AZStd::shared_ptr<RagdollNode>& node)
    {
        m_nodes.push_back(node);
    }

    void Ragdoll::SetParentIndices(const ParentIndices& parentIndices)
    {
        m_parentIndices = parentIndices;
    }

    void Ragdoll::SetRootIndex(size_t nodeIndex)
    {
        m_rootIndex = AZ::Success(nodeIndex);
    }

    physx::PxRigidDynamic* Ragdoll::GetPxRigidDynamic(size_t nodeIndex) const
    {
        if (nodeIndex < m_nodes.size())
        {
            AZStd::shared_ptr<Physics::RigidBody>& rigidBody = m_nodes[nodeIndex]->GetRigidBody();
            return rigidBody ? static_cast<physx::PxRigidDynamic*>(rigidBody->GetNativePointer()) : nullptr;
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
        return nullptr;
    }

    physx::PxTransform Ragdoll::GetRootPxTransform() const
    {
        if (m_rootIndex && m_rootIndex.GetValue() < m_nodes.size())
        {
            physx::PxRigidDynamic* rigidDynamic = GetPxRigidDynamic(m_rootIndex.GetValue());
            if (rigidDynamic)
            {
                return rigidDynamic->getGlobalPose();
            }
            else
            {
                AZ_Error("PhysX Ragdoll", false, "No valid PhysX actor for root node.");
                return physx::PxTransform(physx::PxIdentity);
            }
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid root index.");
        return physx::PxTransform(physx::PxIdentity);
    }

    Ragdoll::Ragdoll()
        : m_isSimulated(false)
    {
    }

    // Physics::Ragdoll
    void Ragdoll::EnableSimulation(const Physics::RagdollState& initialState)
    {
        if (m_isSimulated)
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(world->GetNativePointer());

        const size_t numNodes = m_nodes.size();
        if (initialState.size() != numNodes)
        {
            AZ_Error("PhysX Ragdoll", false, "Mismatch between the number of nodes in the ragdoll initial state (%i)"
                "and the number of nodes in the ragdoll (%i).", initialState.size(), numNodes);
            return;
        }

        pxScene->lockWrite();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            physx::PxRigidDynamic* pxActor = GetPxRigidDynamic(nodeIndex);
            if (pxActor)
            {
                const Physics::RagdollNodeState& nodeState = initialState[nodeIndex];
                physx::PxTransform pxTm(PxMathConvert(nodeState.m_position), PxMathConvert(nodeState.m_orientation));
                pxActor->setGlobalPose(pxTm);
                pxActor->setLinearVelocity(PxMathConvert(nodeState.m_linearVelocity));
                pxActor->setAngularVelocity(PxMathConvert(nodeState.m_angularVelocity));
                pxScene->addActor(*pxActor);
            }

            else
            {
                AZ_Error("PhysX Ragdoll", false, "Invalid PhysX actor for node index %i", nodeIndex);
            }

            if (nodeIndex < m_parentIndices.size())
            {
                size_t parentIndex = m_parentIndices[nodeIndex];
                if (parentIndex < numNodes)
                {
                    world->RegisterSuppressedCollision(m_nodes[nodeIndex]->GetRigidBody(), m_nodes[parentIndex]->GetRigidBody());
                }
            }
        }
        pxScene->unlockWrite();

        m_isSimulated = true;
    }

    void Ragdoll::DisableSimulation()
    {
        if (!m_isSimulated)
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(world->GetNativePointer());
        const size_t numNodes = m_nodes.size();

        pxScene->lockWrite();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            pxScene->removeActor(*GetPxRigidDynamic(nodeIndex));

            if (nodeIndex < m_parentIndices.size())
            {
                size_t parentIndex = m_parentIndices[nodeIndex];
                if (parentIndex < numNodes)
                {
                    world->UnregisterSuppressedCollision(m_nodes[nodeIndex]->GetRigidBody(), m_nodes[parentIndex]->GetRigidBody());
                }
            }
        }
        pxScene->unlockWrite();

        m_isSimulated = false;
    }

    bool Ragdoll::IsSimulated()
    {
        return m_isSimulated;
    }

    void Ragdoll::GetState(Physics::RagdollState& ragdollState) const
    {
        ragdollState.resize(m_nodes.size());

        const size_t numNodes = m_nodes.size();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            GetNodeState(nodeIndex, ragdollState[nodeIndex]);
        }
    }

    void Ragdoll::SetState(const Physics::RagdollState& ragdollState)
    {
        if (ragdollState.size() != m_nodes.size())
        {
            AZ_ErrorOnce("PhysX Ragdoll", false, "Mismatch between number of nodes in desired ragdoll state (%i) and ragdoll (%i)",
                ragdollState.size(), m_nodes.size());
            return;
        }

        const size_t numNodes = m_nodes.size();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            SetNodeState(nodeIndex, ragdollState[nodeIndex]);
        }
    }

    void Ragdoll::GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const
    {
        if (nodeIndex >= m_nodes.size())
        {
            AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
            return;
        }

        const physx::PxRigidDynamic* actor = GetPxRigidDynamic(nodeIndex);
        if (!actor)
        {
            AZ_Error("Physx Ragdoll", false, "No PhysX actor associated with ragdoll node %i", nodeIndex);
            return;
        }

        nodeState.m_position = PxMathConvert(actor->getGlobalPose().p);
        nodeState.m_orientation = PxMathConvert(actor->getGlobalPose().q);
        nodeState.m_linearVelocity = PxMathConvert(actor->getLinearVelocity());
        nodeState.m_angularVelocity = PxMathConvert(actor->getAngularVelocity());
    }

    void Ragdoll::SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState)
    {
        if (nodeIndex >= m_nodes.size())
        {
            AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
            return;
        }

        physx::PxRigidDynamic* actor = GetPxRigidDynamic(nodeIndex);
        if (!actor)
        {
            AZ_Error("Physx Ragdoll", false, "No PhysX actor associated with ragdoll node %i", nodeIndex);
            return;
        }

        if (nodeState.m_simulationType == Physics::SimulationType::Kinematic)
        {
            actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            actor->setKinematicTarget(physx::PxTransform(
                PxMathConvert(nodeState.m_position),
                PxMathConvert(nodeState.m_orientation)));
        }

        else
        {
            actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
            const AZStd::shared_ptr<Physics::Joint>& joint = m_nodes[nodeIndex]->GetJoint();
            if (joint)
            {
                if (physx::PxD6Joint* pxJoint = static_cast<physx::PxD6Joint*>(joint->GetNativePointer()))
                {
                    float forceLimit = std::numeric_limits<float>::max();
                    physx::PxD6JointDrive jointDrive = Utils::CreateD6JointDrive(nodeState.m_strength,
                        nodeState.m_dampingRatio, forceLimit);
                    pxJoint->setDrive(physx::PxD6Drive::eSWING, jointDrive);
                    pxJoint->setDrive(physx::PxD6Drive::eTWIST, jointDrive);
                    physx::PxQuat targetRotation = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0).q.getConjugate() *
                        PxMathConvert(nodeState.m_orientation) * pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1).q;
                    pxJoint->setDrivePosition(physx::PxTransform(targetRotation));
                }
            }
        }
    }

    AZStd::shared_ptr<Physics::RagdollNode> Ragdoll::GetNode(size_t nodeIndex) const
    {
        if (nodeIndex < m_nodes.size())
        {
            return m_nodes[nodeIndex];
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid node index %i in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
        return nullptr;
    }

    size_t Ragdoll::GetNumNodes() const
    {
        return m_nodes.size();
    }

    // Physics::WorldBody
    AZ::EntityId Ragdoll::GetEntityId() const
    {
        AZ_Warning("PhysX Ragdoll", false, "Not yet supported.");
        return AZ::EntityId(AZ::EntityId::InvalidEntityId);
    }

    Physics::World* Ragdoll::GetWorld() const
    {
        return m_nodes.empty() ? nullptr : m_nodes[0]->GetWorld();
    }

    AZ::Transform Ragdoll::GetTransform() const
    {
        return PxMathConvert(GetRootPxTransform());
    }

    void Ragdoll::SetTransform(const AZ::Transform& transform)
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Directly setting the transform for the whole ragdoll is not supported.  "
            "Use SetState or SetNodeState to set transforms for individual ragdoll nodes.");
    }

    AZ::Vector3 Ragdoll::GetPosition() const
    {
        return PxMathConvert(GetRootPxTransform().p);
    }

    AZ::Quaternion Ragdoll::GetOrientation() const
    {
        return PxMathConvert(GetRootPxTransform().q);
    }

    AZ::Aabb Ragdoll::GetAabb() const
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Not yet supported.");
        return AZ::Aabb::CreateNull();
    }

    void Ragdoll::RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Not yet supported.");
    }

    AZ::Crc32 Ragdoll::GetNativeType() const
    {
        return NativeTypeIdentifiers::Ragdoll;
    }

    void* Ragdoll::GetNativePointer() const
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Not yet supported.");
        return nullptr;
    }
} // namespace PhysXCharacters
