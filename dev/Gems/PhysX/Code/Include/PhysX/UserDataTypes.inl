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

namespace PhysX
{
    inline ActorData::ActorData(physx::PxActor* actor)
    {
        auto nullUserData = [](physx::PxActor* actorToSet)
        {
            // making sure userData never dangles
            actorToSet->userData = nullptr;
        };

        m_actor = PxActorUniquePtr(actor, nullUserData);
        actor->userData = this;
    }

    inline ActorData::ActorData(ActorData&& actorData)
        : m_payload(AZStd::move(actorData.m_payload))
        , m_actor(AZStd::move(actorData.m_actor))
    {
        m_actor->userData = this;
    }

    inline ActorData& ActorData::operator=(ActorData&& actorData)
    {
        m_payload = AZStd::move(actorData.m_payload);
        m_actor = AZStd::move(actorData.m_actor);
        m_actor->userData = this;
        return *this;
    }

    inline void ActorData::Invalidate()
    {
        m_actor = nullptr;
        m_payload = Payload();
    }

    inline AZ::EntityId ActorData::GetEntityId() const
    {
        return m_payload.m_entityId;
    }

    inline void ActorData::SetEntityId(AZ::EntityId entityId)
    {
        m_payload.m_entityId = entityId;
    }

    inline Physics::RigidBody* ActorData::GetRigidBody() const
    {
        return m_payload.m_rigidBody;
    }

    inline void ActorData::SetRigidBody(Physics::RigidBody* rigidBody)
    {
        m_payload.m_rigidBody = rigidBody;
    }

    inline Physics::RigidBodyStatic* ActorData::GetRigidBodyStatic() const
    {
        return m_payload.m_staticRigidBody;
    }

    inline void ActorData::SetRigidBodyStatic(Physics::RigidBodyStatic* rigidBody)
    {
        m_payload.m_staticRigidBody = rigidBody;
    }

    inline Physics::Character* ActorData::GetCharacter() const
    {
        return m_payload.m_character;
    }

    inline void ActorData::SetCharacter(Physics::Character* character)
    {
        m_payload.m_character = character;
    }

    inline Physics::RagdollNode* ActorData::GetRagdollNode() const
    {
        return m_payload.m_ragdollNode;
    }

    inline void ActorData::SetRagdollNode(Physics::RagdollNode* ragdollNode)
    {
        m_payload.m_ragdollNode = ragdollNode;
    }

    inline Physics::WorldBody* ActorData::GetWorldBody() const
    {
        if (m_payload.m_rigidBody)
        {
            return m_payload.m_rigidBody;
        }

        else if (m_payload.m_staticRigidBody)
        {
            return m_payload.m_staticRigidBody;
        }

        else if (m_payload.m_character)
        {
            return m_payload.m_character;
        }

        else if (m_payload.m_ragdollNode)
        {
            return m_payload.m_ragdollNode;
        }

        else
        {
            AZ_Error("PhysX Actor User Data", false, "Invalid user data");
            return nullptr;
        }
    }

    inline bool ActorData::IsValid() const
    { 
        return m_sanity == s_sanityValue;
    }
}