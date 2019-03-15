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
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include "PhysicsSystemComponent.h"

#include <physinterface.h>

struct SProximityElement;
struct IPhysicalEntity;

namespace LmbrCentral
{
    /*!
     * Entity component which listens for Ragdoll requests,
     * and re-physicalizes with the ragdoll data
     */
    class RagdollComponent
        : public AZ::Component
        , private AzFramework::RagdollPhysicsRequestBus::Handler
        , private CryPhysicsComponentRequestBus::Handler
        , private EntityPhysicsEventBus::Handler
        , private AzFramework::PhysicsComponentRequestBus::Handler
        , private MeshComponentNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(RagdollComponent, "{80DF7994-5A4B-4B10-87B3-BE7BC541CFBB}");
        ~RagdollComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("RagdollService", 0xd416b0dc));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("RagdollService", 0xd416b0dc));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
        }


    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryCharacterPhysicsRagdollRequestBus::Handler
        void EnterRagdoll() override;
        void ExitRagdoll() override;
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        AZStd::shared_ptr<Physics::Ragdoll> GetRagdoll() override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        AZStd::shared_ptr<Physics::RagdollNode> GetNode(size_t nodeIndex) const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryPhysicsComponentRequests
        IPhysicalEntity* GetPhysicalEntity() override;
        void GetPhysicsParameters(pe_params& outParameters) override;
        void SetPhysicsParameters(const pe_params& parameters) override;
        void GetPhysicsStatus(pe_status& outStatus) override;
        void ApplyPhysicsAction(const pe_action& action, bool threadSafe) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityPhysicsEvents
        void OnPostStep(const EntityPhysicsEvents::PostStep& event) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityPhysicsEvents
        virtual void EnablePhysics() override {}
        virtual void DisablePhysics() override {}
        virtual bool IsPhysicsEnabled() override { return m_physicalEntity != nullptr; }
        void AddImpulse(const AZ::Vector3& impulse) override;
        void AddImpulseAtPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint) override;
        virtual void SetVelocity(const AZ::Vector3& velocity) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus::Handler
        virtual void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        virtual void OnMeshDestroyed() override;
        ////////////////////////////////////////////////////////////////////////

        void CreateRagdollInternal();
        void CreateRagdollEntity();
        void AssignRagdollParameters();
        void RetainJointVelocities(const AZ::Vector3& initialVelocity);

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        bool m_isActiveInitially = false;
        bool m_tryToUsePhysicsComponentMass = true;

        // Advanced
        float m_maxPhysicsTimeStep = 0.025f;
        float m_mass = 80.f;
        float m_stiffnessScale = 0.f;
        unsigned m_skeletalLevelOfDetail = 1;
        bool m_retainJointVelocity = false;
        bool m_collidesWithCharacters = true;

        // Damping
        float m_timeUntilAtRest = 0.025f;
        float m_damping = 0.3f;
        float m_dampingDuringFreefall = 0.1f;
        int   m_groundedRequiredPointsOfContact = 4;
        float m_groundedTimeUntilAtRest = 0.065f;
        float m_groundedDamping = 1.5f;

        // Buoyancy
        float m_fluidDensity = 1000.f;
        float m_fluidDamping = 0;
        float m_fluidResistance = 1000.f;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Non Reflected Data
        IPhysicalEntity* m_physicalEntity = nullptr;
        SProximityElement* m_proximityTriggerProxy = nullptr;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral
