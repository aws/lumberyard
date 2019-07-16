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

#include <EMotionFX/Source/SpringSolver.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/DebugDraw.h>

#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    //------------------------------------------------------------------------
    // SpringSolver::CollisionObject
    //------------------------------------------------------------------------
    SpringSolver::CollisionObject::CollisionObject()
        : m_start(AZ::Vector3::CreateZero())
        , m_end(AZ::Vector3(0.0f, 1.0f, 0.0f))
        , m_globalStart(AZ::Vector3::CreateZero())
        , m_globalEnd(AZ::Vector3::CreateZero())
        , m_radius(1.0f)
        , m_jointIndex(~0)
        , m_type(Sphere)
        , m_firstUpdate(true)
    {
    }


    void SpringSolver::CollisionObject::SetupSphere(const AZ::Vector3& center, float radius)
    {
        m_start              = center;
        m_end                = center;
        m_globalStart        = m_start;
        m_globalEnd          = m_end;
        m_radius             = radius;
        m_type               = Sphere;
    }


    void SpringSolver::CollisionObject::SetupCapsule(const AZ::Vector3& startPos, const AZ::Vector3& endPos, float diameter)
    {
        m_start              = startPos;
        m_end                = endPos;
        m_globalStart        = m_start;
        m_globalEnd          = m_end;
        m_radius             = diameter;
        m_type               = Capsule;
    }


    //------------------------------------------------------------------------
    // SpringSolver::Particle
    //------------------------------------------------------------------------
    SpringSolver::Particle::Particle()
        : m_force(AZ::Vector3::CreateZero())
        , m_externalForce(AZ::Vector3::CreateZero())
        , m_pos(AZ::Vector3::CreateZero())
        , m_oldPos(AZ::Vector3::CreateZero())
        , m_fixed(false)
        , m_invMass(1.0f)
        , m_stiffness(0.0f)
        , m_damping(0.001f)
        , m_coneAngleLimit(180.0f)
        , m_gravityFactor(1.0f)
        , m_jointIndex(~0)
        , m_friction(0.0f)
        , m_groupId(0)
    {
    }


    //------------------------------------------------------------------------
    // SpringSolver
    //------------------------------------------------------------------------
    SpringSolver::SpringSolver()
    {
        Clear();
        m_actorInstance = nullptr;
    }


    SpringSolver::~SpringSolver()
    {
        RemoveAllCollisionObjects();
    }


    void SpringSolver::Clear()
    {
        m_springs.clear();
        m_particles.clear();
        m_collisionObjects.clear();
        m_springs.reserve(3);
        m_particles.reserve(5);
        m_currentState.Clear();
        m_prevState.Clear();

        m_fixedTimeStep     = 1.0 / 60.0;
        m_numIterations     = 1;
        m_lastTimeDelta     = 0.0;
        m_timeAccumulation  = 0.0;
        m_parentParticle    = ~0;
        m_collisionDetection = true;
        m_gravity.Set(0.0f, 0.0f, -9.81f);
    }


    void SpringSolver::DebugRender(const AZ::Color& color)
    {
        const float s = 0.02f;
        const AZ::Color jointLocationColor(0.0f, 1.0f, 0.0f, 1.0f);

        DebugDraw& debugDraw = GetDebugDraw();
        DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(m_actorInstance);
        drawData->Lock();
        for (Spring& spring : m_springs)
        {
            const Particle& particleA = m_particles[spring.m_particleA];
            const Particle& particleB = m_particles[spring.m_particleB];

            drawData->AddLine(particleA.m_pos, particleB.m_pos, color);
            drawData->AddLine(particleB.m_pos + AZ::Vector3(0.0f, 0.0f, -s), particleB.m_pos + AZ::Vector3(0.0f, 0.0f, s), jointLocationColor);
            drawData->AddLine(particleB.m_pos + AZ::Vector3(0.0f, -s, 0.0f), particleB.m_pos + AZ::Vector3(0.0f, s, 0.0f), jointLocationColor);
            drawData->AddLine(particleB.m_pos + AZ::Vector3(-s, 0.0f, 0.0f), particleB.m_pos + AZ::Vector3(s, 0.0f, 0.0f), jointLocationColor);
        }
        drawData->Unlock();
    }


    void SpringSolver::SetFixedTimeStep(double timeStep)
    {
        AZ_Assert(timeStep > 0.0001, "Fixed time step cannot be this small.");
        m_fixedTimeStep = timeStep;
    }


    double SpringSolver::GetFixedTimeStep() const
    {
        return m_fixedTimeStep;
    }


    void SpringSolver::SetNumIterations(size_t numIterations)
    {
        AZ_Assert(numIterations > 0, "Number of iterations cannot be zero.");
        AZ_Assert(numIterations <= 50, "Number of iterations in the spring solver shouldn't be set so high.");
        m_numIterations = MCore::Clamp<size_t>(numIterations, 1, 50);
    }


    size_t SpringSolver::GetNumIterations() const
    {
        return m_numIterations;
    }


    void SpringSolver::SetGravity(const AZ::Vector3& gravity)
    {
        m_gravity = gravity;
    }


    const AZ::Vector3& SpringSolver::GetGravity() const
    {
        return m_gravity;
    }


    void SpringSolver::SetActorInstance(ActorInstance* actorInstance)
    {
        m_actorInstance = actorInstance;
    }


    size_t SpringSolver::FindParticle(AZ::u32 jointIndex) const
    {
        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            if (m_particles[i].m_jointIndex == jointIndex)
            {
                return i;
            }
        }

        return ~0;
    }


    SpringSolver::Particle* SpringSolver::FindParticle(const char* nodeName)
    {
        const Actor* actor = m_actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();

        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            if (skeleton->GetNode(m_particles[i].m_jointIndex)->GetNameString() == nodeName)
            {
                return &m_particles[i];
            }
        }

        return nullptr;
    }


    size_t SpringSolver::AddParticle(AZ::u32 jointIndex, const Particle* copySettingsFrom)
    {
        Particle particle;
        particle.m_jointIndex = jointIndex;
        particle.m_pos       = m_actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(jointIndex).mPosition;
        particle.m_oldPos    = particle.m_pos;

        if (copySettingsFrom)
        {
            particle.m_coneAngleLimit = copySettingsFrom->m_coneAngleLimit;
            particle.m_damping        = copySettingsFrom->m_damping;
            particle.m_friction       = copySettingsFrom->m_friction;
            particle.m_gravityFactor  = copySettingsFrom->m_gravityFactor;
            particle.m_invMass        = copySettingsFrom->m_invMass;
            particle.m_stiffness      = copySettingsFrom->m_stiffness;
            particle.m_groupId        = copySettingsFrom->m_groupId;
        }

        m_particles.emplace_back(particle);
        return m_particles.size() - 1;
    }


    // Add a whole chain at once, by specifying a start (root) and end node index.
    bool SpringSolver::AddChain(AZ::u32 startNodeIndex, AZ::u32 endNodeIndex, bool startNodeIsFixed, bool allowStretch, const Particle* copySettingsFrom)
    {
        if (startNodeIndex == ~0 || endNodeIndex == ~0)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - Either the start or end node is not a valid node (doesn't exist).", startNodeIndex, endNodeIndex);
            return false;
        }

        if (startNodeIndex == endNodeIndex)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - Start and end node are the same, this is not allowed. We need to add at least 2 nodes. (start=%d  end=%d).", startNodeIndex, endNodeIndex);
            return false;
        }

        // Verify if the chain is valid.
        // Does walking the chain from the end upwards, end up at the start node?
        // While verifying this, we keep some array of nodes, in the opposite direction, as we need to add the nodes
        // starting from the start node, towards the end node.
        AZStd::vector<AZ::u32> nodes;
        nodes.reserve(10);
        Actor* actor = m_actorInstance->GetActor();
        Skeleton* skeleton = actor->GetSkeleton();
        AZ::u32 curNode = endNodeIndex;
        while (curNode != startNodeIndex)
        {
            const Node* curNodePointer = skeleton->GetNode(curNode);
            const AZ::u32 parentIndex = curNodePointer->GetParentIndex();

            if (parentIndex == ~0)
            {
                AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - There is no valid path between the specified nodes (start=%d  end=%d).", startNodeIndex, endNodeIndex);
                return false;
            }
            else
            {
                if (!m_actorInstance->GetEnabledNodes().Contains(static_cast<uint16>(skeleton->GetNode(parentIndex)->GetNodeIndex())))
                {
                    AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - The node '%s'inside the chain is disabled by skeletal LOD. You cannot init a chain from disabled nodes. (chainStart='%s', chainEnd='%s').", skeleton->GetNode(parentIndex)->GetName(), skeleton->GetNode(startNodeIndex)->GetName(), skeleton->GetNode(endNodeIndex)->GetName());
                    return false;
                }
            }

            if (!m_actorInstance->GetEnabledNodes().Contains(static_cast<uint16>(curNodePointer->GetNodeIndex())))
            {
                AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - The node '%s'inside the chain is disabled by skeletal LOD. You cannot init a chain from disabled nodes. (chainStart='%s', chainEnd='%s').", skeleton->GetNode(curNode)->GetName(), skeleton->GetNode(startNodeIndex)->GetName(), skeleton->GetNode(endNodeIndex)->GetName());
                return false;
            }

            nodes.insert(nodes.begin(), curNode);
            curNode = parentIndex;
        }

        nodes.insert(nodes.begin(), startNodeIndex);

        // Now that we have the array of nodes that go from start to the end node, lets actually add them to the solver.
        StartNewChain();
        const size_t numNodes = nodes.size();
        for (size_t i = 0; i < numNodes; ++i)
        {
            bool isFixed = false;
            if (i == 0 && startNodeIsFixed)
            {
                isFixed = true;
            }

            if (!AddJoint(nodes[i], isFixed, allowStretch, copySettingsFrom))
            {
                return false;
            }
        }

        return true;
    }


    bool SpringSolver::AddChain(const char* startNodeName, const char* endNodeName, bool startNodeIsFixed, bool allowStretch, const SpringSolver::Particle* copySettingsFrom)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const Node* startNode = skeleton->FindNodeByNameNoCase(startNodeName);
        if (!startNode)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - Failed to find the start node with name '%s'.", startNodeName);
            return false;
        }

        const Node* endNode = skeleton->FindNodeByNameNoCase(endNodeName);
        if (!endNode)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::AddChain() - Failed to find the end node with name '%s'.", endNodeName);
            return false;
        }

        return AddChain(startNode->GetNodeIndex(), endNode->GetNodeIndex(), startNodeIsFixed, allowStretch, copySettingsFrom);
    }


    void SpringSolver::StartNewChain()
    {
        m_parentParticle = ~0;
    }


    void SpringSolver::Log()
    {
        AZ_Printf("EMotionFX", "Num Particles = %d\n", m_particles.size());
        AZ_Printf("EMotionFX", "Num Springs   = %d\n", m_springs.size());

        for (const Particle& particle : m_particles)
        {
            AZ_Printf("EMotionFX", "Particle: fixed=%d, coneLimit=%f, stiffness=%f, damping=%f, node='%s'\n",
                particle.m_fixed,
                particle.m_coneAngleLimit,
                particle.m_stiffness,
                particle.m_damping,
                m_actorInstance->GetActor()->GetSkeleton()->GetNode(particle.m_jointIndex)->GetName());
        }

        for (const Spring& spring: m_springs)
        {
            AZ_Printf("EMotionFX", "Spring: particleA='%s', particleB='%s', restLength=%f\n",
                m_actorInstance->GetActor()->GetSkeleton()->GetNode(m_particles[spring.m_particleA].m_jointIndex)->GetName(),
                m_actorInstance->GetActor()->GetSkeleton()->GetNode(m_particles[spring.m_particleB].m_jointIndex)->GetName(),
                spring.m_restLength);
        }
    }


    SpringSolver::Particle* SpringSolver::AddJoint(AZ::u32 jointIndex, bool isFixed, bool allowStretch, const SpringSolver::Particle* copySettingsFrom)
    {
        if (jointIndex == ~0)
        {
            return nullptr;
        }

        size_t particleA = FindParticle(jointIndex);
        if (particleA == ~0)
        {
            particleA = AddParticle(jointIndex, copySettingsFrom);
            m_particles[particleA].m_fixed = isFixed;
        }
        else
        {
            return &m_particles[particleA];
        }

        if (m_parentParticle != ~0)
        {
            Spring newSpring;
            newSpring.m_particleA    = particleA;
            newSpring.m_particleB    = m_parentParticle;
            newSpring.m_restLength   = (m_particles[particleA].m_pos - m_particles[m_parentParticle].m_pos).GetLengthExact();
            if (newSpring.m_restLength < AZ::g_fltEps)
            {
                //AZ_Printf("EMotionFX", "Skipping joint index %d (%s)", jointIndex, m_actorInstance->GetActor()->GetSkeleton()->GetNode(jointIndex)->GetName());
                return &m_particles[m_parentParticle];
            }
            newSpring.m_allowStretch = allowStretch;
            m_springs.emplace_back(newSpring);
        }

        m_parentParticle = particleA;
        return &m_particles[particleA];
    }


    SpringSolver::Particle* SpringSolver::AddJoint(const char* nodeName, bool isFixed, bool allowStretch, const SpringSolver::Particle* copySettingsFrom)
    {
        const Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(nodeName);
        if (!node)
        {
            return nullptr;
        }

        return AddJoint(node->GetNodeIndex(), isFixed, allowStretch, copySettingsFrom);
    }


    bool SpringSolver::AddSupportSpring(AZ::u32 nodeA, AZ::u32 nodeB, float restLength)
    {
        if (nodeA == ~0 || nodeB == ~0)
        {
            return false;
        }

        const size_t particleA = FindParticle(nodeA);
        const size_t particleB = FindParticle(nodeB);
        if (particleA == ~0 || particleB == ~0)
        {
            return false;
        }

        if (restLength < 0.0f)
        {
            const Pose* pose = m_actorInstance->GetTransformData()->GetCurrentPose();
            const AZ::Vector3& posA = pose->GetWorldSpaceTransform(nodeA).mPosition;
            const AZ::Vector3& posB = pose->GetWorldSpaceTransform(nodeB).mPosition;
            restLength = (posB - posA).GetLengthExact();
        }

        m_springs.emplace_back(particleA, particleB, restLength, true, true);
        return true;
    }


    bool SpringSolver::AddSupportSpring(const char* nodeNameA, const char* nodeNameB, float restLength)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA);
        const Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB);
        if (!nodeA || !nodeB)
        {
            return false;
        }

        return AddSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex(), restLength);
    }


    bool SpringSolver::RemoveJoint(AZ::u32 jointIndex)
    {
        const size_t particleIndex = FindParticle(jointIndex);
        if (particleIndex == ~0)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveJoint() - Failed to find any particle that uses the joint index value %d.", jointIndex);
            return false;
        }

        m_particles.erase(m_particles.begin() + particleIndex);

        for (size_t i = 0; i < m_springs.size();)
        {
            if (m_springs[i].m_particleA == particleIndex || m_springs[i].m_particleB == particleIndex)
            {
                m_springs.erase(m_springs.begin() + i);
            }
            else
            {
                i++;
            }
        }

        return true;
    }


    bool SpringSolver::RemoveJoint(const char* nodeName)
    {
        const Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(nodeName);
        if (!node)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveJoint() - Failed to locate the joint with the specified name '%s'.", nodeName);
            return false;
        }

        return RemoveJoint(node->GetNodeIndex());
    }


    bool SpringSolver::RemoveSupportSpring(AZ::u32 jointIndexA, AZ::u32 jointIndexB)
    {
        const size_t particleA = FindParticle(jointIndexA);
        if (particleA == ~0)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Cannot find any particle that uses the specified joint index %d (jointIndexA).", jointIndexA);
            return false;
        }

        const size_t particleB = FindParticle(jointIndexB);
        if (particleB == ~0)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Cannot find any particle that uses the specified joint index %d (jointIndexB).", jointIndexB);
            return false;
        }

        for (size_t i = 0; i < m_springs.size();)
        {
            if ((m_springs[i].m_particleA == particleA && m_springs[i].m_particleB == particleB) || (m_springs[i].m_particleA == particleB && m_springs[i].m_particleB == particleA))
            {
                m_springs.erase(m_springs.begin() + i);
            }
            else
            {
                i++;
            }
        }

        return true;
    }


    bool SpringSolver::RemoveSupportSpring(const char* nodeNameA, const char* nodeNameB)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA);
        const Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB);

        if (!nodeA || !nodeB)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Failed to locate one or both of the specified nodes (using a names '%s' and '%s').", nodeNameA, nodeNameB);
            return false;
        }

        return RemoveSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex());
    }


    void SpringSolver::CalcForces(const Pose& pose)
    {
        for (Particle& particle : m_particles)
        {
            particle.m_force = AZ::Vector3::CreateZero();

            Transform globalTransform = pose.GetWorldSpaceTransform(particle.m_jointIndex);
            const AZ::Vector3& targetPos = globalTransform.mPosition;

            // Try to move us towards the current pose, based on a stiffness.
            if (!particle.m_fixed)
            {
                const AZ::Vector3 force = (targetPos - particle.m_pos) + particle.m_externalForce;
                particle.m_force = particle.m_invMass * force * particle.m_stiffness;
            }

            // Apply gravity.
            particle.m_force += (1.0f / particle.m_invMass) * particle.m_gravityFactor * m_gravity;
        }
    }


    void SpringSolver::PerformConeLimit(const Spring& spring, Particle& particleA, Particle& particleB, const AZ::Vector3& inputDir)
    {
        // Calc the vector pointing towards the child in world space as the animation outputs.
        AZ::Vector3 animDir = inputDir;
        const float animDirLength = animDir.GetLengthExact();
        if (animDir.GetLength() <= AZ::g_fltEps)
        {
            return;
        }

        // Now calculate our current spring direction.
        AZ::Vector3 springDir = particleA.m_pos - particleB.m_pos;
        const float springLength = springDir.GetLengthExact();
        if (springLength <= AZ::g_fltEps)
        {
            return;
        }

        animDir /= animDirLength;
        springDir /= springLength;

        // Calculate the angle between the vectors.
        float dotResult = springDir.Dot(animDir);
        dotResult = MCore::Clamp<float>(dotResult, -1.0f, 1.0f);

        // If it's outside of its limits.
        float angle = MCore::Math::ACos(dotResult);
        if (angle > MCore::Math::DegreesToRadians(particleB.m_coneAngleLimit))
        {
            const AZ::Vector3 rotAxis = animDir.Cross(springDir);

            angle = MCore::Math::RadiansToDegrees(angle) - particleB.m_coneAngleLimit;
            const MCore::Quaternion rot(rotAxis, MCore::Math::DegreesToRadians(-angle));
            springDir = (rot * springDir) * springLength;

            particleA.m_pos = particleB.m_pos + springDir;
        }
    }


    void SpringSolver::SatisfyConstraints(const Pose& pose, size_t numIterations)
    {
        for (size_t n = 0; n < numIterations; ++n)
        {
            for (const Spring& spring : m_springs)
            {
                Particle& particleA = m_particles[spring.m_particleA];
                Particle& particleB = m_particles[spring.m_particleB];
                Transform globalTransformA = pose.GetWorldSpaceTransform(particleA.m_jointIndex);
                Transform globalTransformB = pose.GetWorldSpaceTransform(particleB.m_jointIndex);
                const AZ::Vector3& fixedPosA = globalTransformA.mPosition;
                const AZ::Vector3& fixedPosB = globalTransformB.mPosition;

                // Try to maintain the rest length by applying correctional forces.
                const AZ::Vector3 delta = particleB.m_pos - particleA.m_pos;
                const float deltaLength = delta.GetLengthExact();
                float diff;
                if (deltaLength > MCore::Math::epsilon)
                {
                    diff = (deltaLength - spring.m_restLength) / (deltaLength * (particleA.m_invMass + particleB.m_invMass));
                }
                else
                {
                    diff = 0.0f;
                }

                if (!particleA.m_fixed && !particleB.m_fixed)
                {
                    particleA.m_pos += delta * 0.5f * diff;
                    particleB.m_pos -= delta * 0.5f * diff;
                }
                else if (particleA.m_fixed && particleB.m_fixed)
                {
                    particleA.m_pos = fixedPosA;
                    particleB.m_pos = fixedPosB;
                }
                else if (particleB.m_fixed)
                {
                    particleB.m_pos = fixedPosB;
                    particleA.m_pos += delta * diff;
                }
                else // only particleA is fixed
                {
                    particleA.m_pos = fixedPosA;
                    particleB.m_pos -= delta * diff;
                }

                // Apply cone limit.
                if (particleB.m_coneAngleLimit < 180.0f - 0.001f)
                {
                    const AZ::Vector3 animDir = fixedPosA - fixedPosB;
                    PerformConeLimit(spring, particleA, particleB, animDir);
                }
            }

            // Perform collision.
            if (m_collisionDetection)
            {
                for (Particle& particle : m_particles)
                {
                    // Do not modify fixed particles
                    if (particle.m_fixed)
                    {
                        continue;
                    }

                    // If there was a collision.
                    if (PerformCollision(particle.m_pos))
                    {
                        particle.m_oldPos = MCore::LinearInterpolate<AZ::Vector3>(particle.m_oldPos, particle.m_pos, particle.m_friction);
                    }
                }
            }
        }
    }


    void SpringSolver::Simulate(float deltaTime, const Pose& pose)
    {
        CalcForces(pose);
        Integrate(deltaTime);
        SatisfyConstraints(pose, m_numIterations);
    }


    void SpringSolver::UpdateFixedParticles(const Pose& pose)
    {
        for (Particle& particle : m_particles)
        {
            if (particle.m_fixed)
            {
                particle.m_pos = pose.GetWorldSpaceTransform(particle.m_jointIndex).mPosition;
            }
        }
    }


    void SpringSolver::UpdateCurrentState()
    {
        const size_t numParticles = m_particles.size();
        m_currentState.m_particles.resize(numParticles);
        for (size_t i = 0; i < numParticles; ++i)
        {
            const Particle& particle = m_particles[i];
            ParticleState& particleState = m_currentState.m_particles[i];
            particleState.m_oldPos = particle.m_oldPos;
            particleState.m_pos = particle.m_pos;
        }
    }


    void SpringSolver::InterpolateState(float alpha)
    {
        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            Particle& particle = m_particles[i];
            const ParticleState& curParticleState = m_currentState.m_particles[i];
            const ParticleState& prevParticleState = m_prevState.m_particles[i];

            particle.m_pos = prevParticleState.m_pos.Lerp(curParticleState.m_pos, alpha);
            particle.m_oldPos = prevParticleState.m_oldPos.Lerp(curParticleState.m_oldPos, alpha);
        }
    }


    void SpringSolver::Update(Pose& pose, float timePassedInSeconds, float weight)
    {
        AZ_Assert(m_actorInstance, "The solver first needs to be linked to a given actor instance, please use SetActorInstance first.");
        PrepareCollisionObjects(pose);
        UpdateFixedParticles(pose);

        // Make sure we don't update with too small stepsize, as that can cause issues.
        double stepSize = m_fixedTimeStep;
        if (stepSize < 0.005)
        {
            stepSize = 0.005;
        }

        // Perform fixed time step updates.
        m_timeAccumulation += timePassedInSeconds;
        while (m_timeAccumulation >= stepSize)
        {
            m_prevState = m_currentState;
            UpdateCollisionObjects(pose);
            Simulate(static_cast<float>(stepSize), pose);
            UpdateCurrentState();
            m_timeAccumulation -= stepSize;
        }

        // Make sure we update the states when the sizes need to be updated.
        if (m_currentState.m_particles.empty() || m_currentState.m_particles.size() != m_particles.size() || m_prevState.m_particles.size() != m_currentState.m_particles.size())
        {
            UpdateCurrentState();
            m_prevState = m_currentState;
        }

        // Interpolate between the last physics state and the current state.
        const float alpha = static_cast<float>(m_timeAccumulation / stepSize);
        InterpolateState(alpha);

        // Do a final constraint fixup
        UpdateFixedParticles(pose);
        SatisfyConstraints(pose, 1);

        // Modify the world space transformations of the joints.
        UpdateJointTransforms(pose, weight);
    }


    void SpringSolver::AdjustParticles(const ParticleAdjustFunction& func)
    {
        for (Particle& particle : m_particles)
        {
            func(particle);
        }
    }


    void SpringSolver::UpdateJointTransforms(Pose& pose, float weight)
    {
        if (weight < 0.0001f)
        {
            return;
        }

        for (Spring& spring : m_springs)
        {
            if (spring.m_isSupportSpring)
            {
                continue;
            }

            const Particle& particleA = m_particles[spring.m_particleA];
            const Particle& particleB = m_particles[spring.m_particleB];

            Transform globalTransformB = pose.GetWorldSpaceTransform(particleB.m_jointIndex);
            Transform globalTransformA = pose.GetWorldSpaceTransform(particleA.m_jointIndex);

            AZ::Vector3 oldDir = globalTransformA.mPosition - globalTransformB.mPosition;
            oldDir.NormalizeSafeExact();

            AZ::Vector3 newDir = particleA.m_pos - particleB.m_pos;
            newDir.NormalizeSafeExact();

            if (newDir.GetLength() > AZ::g_fltEps && oldDir.GetLength() > AZ::g_fltEps)
            {
                if (weight > 0.9999f)
                {
                    globalTransformB.mRotation.RotateFromTo(oldDir, newDir);
                }
                else
                {
                    MCore::Quaternion targetRot = globalTransformB.mRotation;
                    targetRot.RotateFromTo(oldDir, newDir);
                    globalTransformB.mRotation = globalTransformB.mRotation.NLerp(targetRot, weight);
                }
            }

            if (spring.m_allowStretch)
            {
                globalTransformB.mPosition = globalTransformB.mPosition.Lerp(particleB.m_pos, weight);
            }

            pose.SetWorldSpaceTransform(particleB.m_jointIndex, globalTransformB);
        }
    }


    void SpringSolver::Integrate(float timeDelta)
    {
        const float timeDeltaSq = timeDelta * timeDelta;
        const float timeCorrect = (m_lastTimeDelta > MCore::Math::epsilon) ? (timeDelta  / static_cast<float>(m_lastTimeDelta)) : 1.0f;

        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            Particle& particle = m_particles[i];

            const AZ::Vector3 pos = particle.m_pos;
            const AZ::Vector3 oldPos = particle.m_oldPos;
            particle.m_oldPos = pos;

            // Do the Vertlet integration step.
            particle.m_pos = ((2.0f - particle.m_damping) * pos - (1.0f - particle.m_damping) * oldPos) + (particle.m_force * timeDeltaSq);

            // Correct time corrected verlet: xi+1 = xi + (xi - xi-1) * (dti / dti-1) + (a * dti) * (dti + dti-1) / 2.0
            // This only makes sense to use if you don't use fixed time steps.
            // Keeping this comment in, in case we need it later.
            //particle.m_pos = pos + (pos - oldPos) * timeCorrect * (1.0f-particle.m_damping) + (particle.m_force * timeDelta) * (timeDelta + static_cast<float>(m_lastTimeDelta)) / 2.0f;
        }

        m_lastTimeDelta = timeDelta;
    }


    void SpringSolver::Scale(float scaleFactor)
    {
        AZ_Assert(scaleFactor > AZ::g_fltEps, "The scale cannot be zero.");

        for (Particle& particle : m_particles)
        {
            particle.m_stiffness *= scaleFactor;
            particle.m_invMass /= scaleFactor;
        }

        for (Spring& spring : m_springs)
        {
            spring.m_restLength *= scaleFactor;
        }

        for (CollisionObject& colObject : m_collisionObjects)
        {
            colObject.m_radius *= scaleFactor;
        }
    }


    void SpringSolver::AddCollisionSphere(AZ::u32 jointIndex, const AZ::Vector3& center, float radius)
    {
        m_collisionObjects.emplace_back();
        m_collisionObjects.back().LinkToJoint(jointIndex);
        m_collisionObjects.back().SetupSphere(center, radius);
    }


    void SpringSolver::AddCollisionCapsule(AZ::u32 jointIndex, const AZ::Vector3& start, const AZ::Vector3& end, float diameter)
    {
        m_collisionObjects.emplace_back();
        m_collisionObjects.back().LinkToJoint(jointIndex);
        m_collisionObjects.back().SetupCapsule(start, end, diameter);
    }


    void SpringSolver::PrepareCollisionObjects(const Pose& pose)
    {
        for (CollisionObject& colObject : m_collisionObjects)
        {
            if (colObject.m_firstUpdate)
            {
                if (colObject.m_jointIndex != ~0)
                {
                    const AZ::Transform globalTransform = MCore::EmfxTransformToAzTransform(pose.GetWorldSpaceTransform(colObject.m_jointIndex));    // TODO: make some EMotionFX::Transform method that can multiply with an AZ::Vector3.
                    colObject.m_globalStart = colObject.m_start * globalTransform;
                    colObject.m_globalEnd = colObject.m_end * globalTransform;
                }
                colObject.m_firstUpdate = false;
            }
        }
    }


    void SpringSolver::UpdateCollisionObjects(const Pose& pose)
    {
        for (CollisionObject& colObject : m_collisionObjects)
        {
            if (colObject.m_jointIndex != ~0)
            {
                const AZ::Transform globalTransform = MCore::EmfxTransformToAzTransform(pose.GetWorldSpaceTransform(colObject.m_jointIndex));    // TODO: make some EMotionFX::Transform method that can multiply with an AZ::Vector3
                colObject.m_globalStart = colObject.m_start * globalTransform;
                colObject.m_globalEnd   = colObject.m_end * globalTransform;
            }
            else
            {
                colObject.m_globalStart = colObject.m_start;
                colObject.m_globalEnd   = colObject.m_end;
            }
        }
    }


    bool SpringSolver::PerformCollision(AZ::Vector3& inOutPos)
    {
        bool collided = false;

        for (const CollisionObject& colObject : m_collisionObjects)
        {
            switch (colObject.m_type)
            {
            case CollisionObject::Capsule:
                if (CollideWithCapsule(inOutPos, colObject.m_globalStart, colObject.m_globalEnd, colObject.m_radius))
                {
                    collided = true;
                }
                break;


            case CollisionObject::Sphere:
                if (CollideWithSphere(inOutPos, colObject.m_globalStart, colObject.m_radius))
                {
                    collided = true;
                }
                break;

            default:
                AZ_Assert(false, "Unknown collision object type.");
            }
            ;
        }

        return collided;
    }


    bool SpringSolver::CollideWithSphere(AZ::Vector3& pos, const AZ::Vector3& center, float radius)
    {
        const AZ::Vector3 centerToPoint = pos - center;
        const float sqDist = centerToPoint.GetLengthSq();
        if (sqDist >= radius * radius)
        {
            return false;
        }

        if (sqDist > MCore::Math::epsilon)
        {
            pos = center + radius * (centerToPoint / MCore::Math::FastSqrt(sqDist));
        }
        else
        {
            pos = center;
        }

        return true;
    }


    bool SpringSolver::CollideWithCapsule(AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius)
    {
        const AZ::Vector3 startToEnd = lineEnd - lineStart;
        const float squareLength = startToEnd.GetLengthSq();
        float t;
        if (squareLength > MCore::Math::epsilon)
        {
            t = (pos - lineStart).Dot(startToEnd) / squareLength;
        }
        else
        {
            t = 0.0f;
        }

        // Make sure that we clip this distance to be sure its on the line segment.
        t = AZ::GetClamp(t, 0.0f, 1.0f);

        // Calculate the position projected on the line.
        const AZ::Vector3 projected = lineStart + t * startToEnd;

        // The vector from the projected position to the point we are testing with.
        AZ::Vector3 toPos = pos - projected;

        // If the distance is within the diameter of the capsule, it is colliding and then
        // the point needs to be pushed outside of the capsule.
        const float sqLen = toPos.GetLengthSq();
        if (sqLen < radius * radius)
        {
            toPos.NormalizeSafeExact();
            pos = projected + (toPos * radius);
            return true;
        }
        else // The point is not colliding with the capsule, so ignore it.
        {
            return false;
        }
    }
}
