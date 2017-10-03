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

// include the required headers
#include "JiggleController.h"
#include "ActorInstance.h"
#include "GlobalPose.h"
#include "Node.h"
#include "TransformData.h"


namespace EMotionFX
{
    //------------------------------------------------------------------------
    // JiggleController::CollisionObject
    //------------------------------------------------------------------------

    // constructor
    JiggleController::CollisionObject::CollisionObject()
    {
        mStart.Zero();
        mEnd.Set(0.0f, 1.0f, 0.0f);
        mInterpolationStart = mStart;
        mInterpolationEnd   = mEnd;
        mGlobalStart        = mStart;
        mGlobalEnd          = mEnd;
        mRadius             = 1.0f;
        mNodeIndex          = MCORE_INVALIDINDEX32;
        mType               = COLTYPE_SPHERE;
        mFirstUpdate        = true;
    }


    // setup this collision object as a sphere
    void JiggleController::CollisionObject::SetupSphere(const MCore::Vector3& center, float radius)
    {
        mStart              = center;
        mEnd                = center;
        mInterpolationStart = mStart;
        mInterpolationEnd   = mEnd;
        mGlobalStart            = mStart;
        mGlobalEnd          = mEnd;
        mRadius             = radius;
        mType               = CollisionObject::COLTYPE_SPHERE;
    }


    // setup this collision object as a capsule
    void JiggleController::CollisionObject::SetupCapsule(const MCore::Vector3& startPos, const MCore::Vector3& endPos, float diameter)
    {
        mStart              = startPos;
        mEnd                = endPos;
        mInterpolationStart = mStart;
        mInterpolationEnd   = mEnd;
        mGlobalStart            = mStart;
        mGlobalEnd          = mEnd;
        mRadius             = diameter;
        mType               = CollisionObject::COLTYPE_CAPSULE;
    }



    //------------------------------------------------------------------------
    // JiggleController::Particle
    //------------------------------------------------------------------------
    // constructor
    JiggleController::Particle::Particle()
    {
        // zero positions and forces
        mForce.Zero();
        mExternalForce.Zero();
        mPos.Zero();
        mOldPos.Zero();
        mInterpolateStartPos.Zero();

        // init defaults
        mFixed          = false;    // freely movable
        mInvMass        = 1.0f;     // a mass of 1
        mStiffness      = 0.0f;     // not stiff, completely flexible
        mDamping        = 0.001f;   // low damping value
        mConeAngleLimit = 180.0f;   // 180 degrees in all directions, so basically no rotation limit at all
        mGravityFactor  = 1.0f;     // use -9.81 as default gravity, and dont make it appear stronger for this particle
        mNodeIndex      = MCORE_INVALIDINDEX32;// this particle hasn't been linked to a node yet
        mFriction       = 0.0f;     // friction when a collision occurs (0=no friction, 1=more sticky) must be in range of[0..1]
        mGroupID        = 0;        // default group ID is 0

        //mYawLimitMin      = 180.0f;
        //mYawLimitMax      = 180.0f;
        //mPitchLimitMin    = 180.0f;
        //mPitchLimitMax    = 180.0f;
        //mYawAxis          = 2;
        //mPitchAxis        = 0;
    }



    //------------------------------------------------------------------------
    // JiggleController
    //------------------------------------------------------------------------
    // the constructor
    JiggleController::JiggleController(ActorInstance* actorInstance)
        : GlobalSpaceController(actorInstance)
    {
        // init members
        mCurForceBlendTime  = 0.0f;
        mForceBlendTime     = 1.0f;
        mForceBlendFactor   = 0.0f;
        mFixedTimeStep      = 1.0f / 60.0f; // update physics at 60 fps on default
        mNumIterations      = 1;
        mLastTimeDelta      = 0.0f;
        mTimeAccumulation   = 0.0f;
        mParentParticle     = MCORE_INVALIDINDEX32;
        mCollisionDetection = true;
        mStableUpdates      = false;
        mGravity.Set(0.0f, 0.0f, -9.81f);

        // reserve memory for the springs and particles
        mSprings.Reserve(25);
        mParticles.Reserve(50);
    }


    // the destructor
    JiggleController::~JiggleController()
    {
        RemoveAllCollisionObjects();
    }


    // creation method
    JiggleController* JiggleController::Create(ActorInstance* actorInstance)
    {
        return new JiggleController(actorInstance);
    }


    // the type identification functions
    uint32 JiggleController::GetType() const
    {
        return JiggleController::TYPE_ID;
    }


    // get the type string
    const char* JiggleController::GetTypeString() const
    {
        return "JiggleController";
    }


    // set the fixed time step
    void JiggleController::SetFixedTimeStep(float timeStep)
    {
        MCORE_ASSERT(timeStep > 0.0001f);
        mFixedTimeStep = timeStep;
    }


    // get the fixed time step
    float JiggleController::GetFixedTimeStep() const
    {
        return mFixedTimeStep;
    }


    // set the number of constraint solving iterations
    void JiggleController::SetNumIterations(uint32 numIterations)
    {
        MCORE_ASSERT(numIterations > 0);    // the number of iterations is not allowed to be 0
        MCORE_ASSERT(numIterations <= 50); // too many iterations has no use and becomes too slow
        mNumIterations = MCore::Clamp<uint32>(numIterations, 1, 50);
    }


    // get the number of constraint solving iterations
    uint32 JiggleController::GetNumIterations() const
    {
        return mNumIterations;
    }


    // set the gravity force
    void JiggleController::SetGravity(const MCore::Vector3& gravity)
    {
        mGravity = gravity;
    }


    // get the gravity force
    const MCore::Vector3& JiggleController::GetGravity() const
    {
        return mGravity;
    }


    // activate the controller
    void JiggleController::Activate(float fadeInTime)
    {
        // reset the force blend time
        mCurForceBlendTime  = 0.0f;
        mForceBlendFactor   = 0.0f;

        // perform base class activation code
        GlobalSpaceController::Activate(fadeInTime);
    }


    // find the index of the particle for a given node
    uint32 JiggleController::FindParticle(uint32 nodeIndex) const
    {
        // check all particles
        const uint32 numParticles = mParticles.GetLength();
        for (uint32 i = 0; i < numParticles; ++i)
        {
            if (mParticles[i].mNodeIndex == nodeIndex) // it's a match
            {
                return i;
            }
        }

        // none found
        return MCORE_INVALIDINDEX32;
    }


    // find the index of the particle for a given node
    JiggleController::Particle* JiggleController::FindParticle(const char* nodeName)
    {
        Actor* actor = mActorInstance->GetActor();
        Skeleton* skeleton = actor->GetSkeleton();

        // check all particles
        const uint32 numParticles = mParticles.GetLength();
        for (uint32 i = 0; i < numParticles; ++i)
        {
            // if this particle's node has the name we're looking for, return it
            if (skeleton->GetNode(mParticles[i].mNodeIndex)->GetNameString().CheckIfIsEqualNoCase(nodeName))
            {
                return &mParticles[i];
            }
        }

        // none found
        return nullptr;
    }


    // add a particle to the controller
    uint32 JiggleController::AddParticle(uint32 nodeIndex, const JiggleController::Particle* copySettingsFrom)
    {
        // create the new particle and change some of its settings
        Particle particle;
        particle.mNodeIndex = nodeIndex;
        particle.mPos       = mActorInstance->GetTransformData()->GetGlobalInclusiveMatrix(nodeIndex).GetTranslation();
        particle.mOldPos    = particle.mPos;

        // if we need to copy the main settings from another particle
        // note that we're not copying forces and external forces
        if (copySettingsFrom)
        {
            particle.mConeAngleLimit = copySettingsFrom->mConeAngleLimit;
            particle.mDamping        = copySettingsFrom->mDamping;
            particle.mFriction       = copySettingsFrom->mFriction;
            particle.mGravityFactor  = copySettingsFrom->mGravityFactor;
            particle.mInvMass        = copySettingsFrom->mInvMass;
            particle.mStiffness      = copySettingsFrom->mStiffness;
            particle.mGroupID        = copySettingsFrom->mGroupID;
        }

        mParticles.Add(particle);
        return mParticles.GetLength() - 1;
    }


    // add a whole chain at once, by specifying a start (root) and end node index
    bool JiggleController::AddChain(uint32 startNodeIndex, uint32 endNodeIndex, bool startNodeIsFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom)
    {
        // make sure the nodes are valid
        if (startNodeIndex == MCORE_INVALIDINDEX32 || endNodeIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("JiggleController::AddChain() - Either the start or end node is not a valid node (doesn't exist)", startNodeIndex, endNodeIndex);
            return false;
        }

        // make sure the nodes aren't the same, as we need at least 2 nodes
        if (startNodeIndex == endNodeIndex)
        {
            MCore::LogWarning("JiggleController::AddChain() - Start and end node are the same, this is not allowed. We need to add at least 2 nodes. (start=%d  end=%d)", startNodeIndex, endNodeIndex);
            return false;
        }

        // verify if the chain is valid
        // does walking the chain from the end upwards, end up at the start node?
        // while verifying this, we keep some array of nodes, in the opposite direction, as we need to add the nodes
        // starting from the start node, towards the end node
        MCore::Array<uint32> nodes;
        nodes.Reserve(10); // reserve space for 10 nodes, to prevent reallocs
        Actor* actor = mActorInstance->GetActor();
        Skeleton* skeleton = actor->GetSkeleton();
        uint32 curNode = endNodeIndex;
        while (curNode != startNodeIndex)
        {
            const Node* curNodePointer = skeleton->GetNode(curNode);

            // get the parent node index
            uint32 parentIndex = curNodePointer->GetParentIndex();

            // we reached the root node, and no start node has been found, so this is not a valid chain
            if (parentIndex == MCORE_INVALIDINDEX32)
            {
                MCore::LogWarning("JiggleController::AddChain() - There is no valid path between the specified nodes (start=%d  end=%d)", startNodeIndex, endNodeIndex);
                return false;
            }
            else
            {
                if (mActorInstance->GetEnabledNodes().Contains(static_cast<uint16>(skeleton->GetNode(parentIndex)->GetNodeIndex())) == false)
                {
                    MCore::LogWarning("JiggleController::AddChain() - The node '%s'inside the chain is disabled by skeletal LOD. You cannot init a chain from disabled nodes. (chainStart='%s', chainEnd='%s')", skeleton->GetNode(parentIndex)->GetName(), skeleton->GetNode(startNodeIndex)->GetName(), skeleton->GetNode(endNodeIndex)->GetName());
                    return false;
                }
            }

            if (mActorInstance->GetEnabledNodes().Contains(static_cast<uint16>(curNodePointer->GetNodeIndex())) == false)
            {
                MCore::LogWarning("JiggleController::AddChain() - The node '%s'inside the chain is disabled by skeletal LOD. You cannot init a chain from disabled nodes. (chainStart='%s', chainEnd='%s')", skeleton->GetNode(curNode)->GetName(), skeleton->GetNode(startNodeIndex)->GetName(), skeleton->GetNode(endNodeIndex)->GetName());
                return false;
            }

            // insert the node in the front of the array
            nodes.Insert(0, curNode);

            // move up the chain
            curNode = parentIndex;
        }

        // finally add the start node to the front of the node list as well
        nodes.Insert(0, startNodeIndex);

        // now that we have the array of nodes that go from start to the end node, lets actually add them to the jiggle controller
        StartNewChain(); // mark the start of a new chain
        const uint32 numNodes = nodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // determine if this node needs to be fixed or not
            bool isFixed = false;
            if (i == 0 && startNodeIsFixed)
            {
                isFixed = true;
            }

            // try to add the node to the controller
            if (AddNode(nodes[i], isFixed, allowStretch, copySettingsFrom) == nullptr)
            {
                return false;
            }
        }

        // success at last
        return true;
    }


    // add a whole chain at once, by specifying node names directly
    bool JiggleController::AddChain(const char* startNodeName, const char* endNodeName, bool startNodeIsFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom)
    {
        // get the start node
        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();
        Node* startNode = skeleton->FindNodeByNameNoCase(startNodeName);
        if (startNode == nullptr)
        {
            MCore::LogWarning("JiggleController::AddChain() - Failed to find the start node with name '%s'", startNodeName);
            return false;
        }

        // get the end node
        Node* endNode = skeleton->FindNodeByNameNoCase(endNodeName);
        if (endNode == nullptr)
        {
            MCore::LogWarning("JiggleController::AddChain() - Failed to find the end node with name '%s'", endNodeName);
            return false;
        }

        // return the result
        return AddChain(startNode->GetNodeIndex(), endNode->GetNodeIndex(), startNodeIsFixed, allowStretch, copySettingsFrom);
    }


    // start a new chain
    void JiggleController::StartNewChain()
    {
        mParentParticle = MCORE_INVALIDINDEX32;
    }


    // add a node to the system and create a spring when we should
    JiggleController::Particle* JiggleController::AddNode(uint32 nodeIndex, bool isFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom)
    {
        // check if the node actually is a valid one or not
        if (nodeIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // add the first particle to the system
        uint32 particleA = FindParticle(nodeIndex);
        if (particleA == MCORE_INVALIDINDEX32) // if the particle hasn't been added yet, add it
        {
            particleA = AddParticle(nodeIndex, copySettingsFrom);
            mParticles[particleA].mFixed = isFixed; // overwrite the isFixed setting
        }
        else
        {
            return &mParticles[particleA]; // prevent adding springs between 2 of the same particles
        }
        // if there is a parent particle, we can add a new spring
        if (mParentParticle != MCORE_INVALIDINDEX32)
        {
            // add the spring
            Spring newSpring;
            newSpring.mParticleA    = particleA;
            newSpring.mParticleB    = mParentParticle;
            newSpring.mRestLength   = (mParticles[particleA].mPos - mParticles[mParentParticle].mPos).SafeLength();
            newSpring.mAllowStretch = allowStretch;
            mSprings.Add(newSpring);

            // make sure all nodes down the hierarchy of this node will get updated
            RecursiveSetNodeMask(mParticles[mParentParticle].mNodeIndex, true); // TODO: isn't this only needed once, when adding the first node? as we add the root anyway
        }

        // update the parent particle
        mParentParticle = particleA;

        // return a reference to the new particle
        return &mParticles[particleA];
    }


    // add a given node by using a node name instead of index
    JiggleController::Particle* JiggleController::AddNode(const char* nodeName, bool isFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom)
    {
        // try to get the node
        Node* node = mActorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(nodeName);
        if (node == nullptr)
        {
            return nullptr;
        }

        // add the node
        return AddNode(node->GetNodeIndex(), isFixed, allowStretch, copySettingsFrom);
    }


    // add a spring between two nodes
    bool JiggleController::AddSupportSpring(uint32 nodeA, uint32 nodeB, float restLength)
    {
        // make sure the nodes are valid
        if (nodeA == MCORE_INVALIDINDEX32 || nodeB == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // find the particles to connect between
        const uint32 particleA = FindParticle(nodeA);
        const uint32 particleB = FindParticle(nodeB);

        // one of the particles can't be found, the user first needs to use AddNode
        if (particleA == MCORE_INVALIDINDEX32 || particleB == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // auto detect the rest length
        if (restLength < 0.0f)
        {
            MCore::Vector3 posA = mActorInstance->GetTransformData()->GetGlobalInclusiveMatrix(nodeA).GetTranslation();
            MCore::Vector3 posB = mActorInstance->GetTransformData()->GetGlobalInclusiveMatrix(nodeB).GetTranslation();
            restLength = (posB - posA).SafeLength();
        }

        // create the new spring between those particles, and mark them as special connection spring
        mSprings.Add(JiggleController::Spring(particleA, particleB, restLength, true, true));

        // return a pointer to the last spring
        return true;
    }


    // add a support spring
    bool JiggleController::AddSupportSpring(const char* nodeNameA, const char* nodeNameB, float restLength)
    {
        // try to find the nodes
        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();
        Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA);
        Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB);

        // if one of them doesn't exist, fail
        if (nodeA == nullptr || nodeB == nullptr)
        {
            return false;
        }

        // return the resulting spring, or nullptr if something goes wrong
        return AddSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex(), restLength);
    }


    // remove a given node (particle) and all springs that use it
    bool JiggleController::RemoveNode(uint32 nodeIndex)
    {
        // find the particle that uses this node
        const uint32 particleIndex = FindParticle(nodeIndex);
        if (particleIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("JiggleController::RemoveNode() - Failed to find any particle that uses the node index value %d", nodeIndex);
            return false;
        }

        // remove the particle
        mParticles.Remove(particleIndex);

        // remove all springs that use this particle
        for (uint32 i = 0; i < mSprings.GetLength(); )
        {
            if (mSprings[i].mParticleA == particleIndex || mSprings[i].mParticleB == particleIndex)
            {
                mSprings.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // successfully removed
        return true;
    }


    // remove a given node
    bool JiggleController::RemoveNode(const char* nodeName)
    {
        // find the node with the given name
        Node* node = mActorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(nodeName);
        if (node == nullptr)
        {
            MCore::LogWarning("JiggleController::RemoveNode() - Failed to locate the node with the specified name '%s'", nodeName);
            return false;
        }

        // remove the node
        return RemoveNode(node->GetNodeIndex());
    }


    // remove a given support spring
    bool JiggleController::RemoveSupportSpring(uint32 nodeIndexA, uint32 nodeIndexB)
    {
        // find the particles using the given nodes
        const uint32 particleA = FindParticle(nodeIndexA);
        if (particleA == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("JiggleController::RemoveSupportSpring() - Cannot find any particle that uses the specified node index %d (nodeIndexA)", nodeIndexA);
            return false;
        }

        const uint32 particleB = FindParticle(nodeIndexB);
        if (particleB == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("JiggleController::RemoveSupportSpring() - Cannot find any particle that uses the specified node index %d (nodeIndexB)", nodeIndexB);
            return false;
        }

        // delete all springs with the given particle numbers
        for (uint32 i = 0; i < mSprings.GetLength(); )
        {
            if ((mSprings[i].mParticleA == particleA && mSprings[i].mParticleB == particleB) || (mSprings[i].mParticleA == particleB && mSprings[i].mParticleB == particleA))
            {
                mSprings.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // successfully removed
        return true;
    }


    // remove a given support spring
    bool JiggleController::RemoveSupportSpring(const char* nodeNameA, const char* nodeNameB)
    {
        // find the nodes with the given names
        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();
        Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA);
        Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB);

        // check if they are found
        if (nodeA == nullptr || nodeB == nullptr)
        {
            MCore::LogWarning("JiggleController::RemoveSupportSpring() - Failed to locate one or both of the specified nodes (using a names '%s' and '%s')", nodeNameA, nodeNameB);
            return false;
        }

        // remove the spring
        return RemoveSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex());
    }


    // clone the controller
    GlobalSpaceController* JiggleController::Clone(ActorInstance* targetActorInstance)
    {
        // create the clone
        JiggleController* clone = new JiggleController(targetActorInstance);

        // copy the GlobalSpaceController base class settings
        clone->CopyBaseClassWeightSettings(this);

        // copy the settings
        clone->mSprings             = mSprings;
        clone->mParticles           = mParticles;
        clone->mCollisionObjects    = mCollisionObjects;
        clone->mForceBlendTime      = mForceBlendTime;
        clone->mCurForceBlendTime   = mCurForceBlendTime;
        clone->mForceBlendFactor    = mForceBlendFactor;
        clone->mNumIterations       = mNumIterations;
        clone->mFixedTimeStep       = mFixedTimeStep;
        clone->mLastTimeDelta       = mLastTimeDelta;
        clone->mGravity             = mGravity;
        clone->mParentParticle      = mParentParticle;
        clone->mCollisionDetection  = mCollisionDetection;
        clone->mStableUpdates       = mStableUpdates;

        // update the node mask
        clone->mNodeMask            = mNodeMask;

        return clone;
    }


    // calculate the forces that need to be applied
    void JiggleController::CalcForces(const MCore::Matrix* globalMatrices, float interpolationFraction)
    {
        // update all the particles
        const uint32 numParticles = mParticles.GetLength();
        for (uint32 i = 0; i < numParticles; ++i)
        {
            Particle& particle = mParticles[i];

            // set the force to zero
            particle.mForce.Zero();

            const MCore::Vector3 targetPos = MCore::LinearInterpolate<MCore::Vector3>(particle.mInterpolateStartPos, globalMatrices[particle.mNodeIndex].GetTranslation(), interpolationFraction);

            // if the particle isn't fixed, so if it can move
            if (particle.mFixed == false)
            {
                // calculate the force, based on its stiffness and mass
                //          const Vector3 force = (globalMatrices[particle.mNodeIndex].GetTranslation() - particle.mPos) + particle.mExternalForce;
                const MCore::Vector3 force = (targetPos - particle.mPos) + particle.mExternalForce;
                particle.mForce = particle.mInvMass * force * particle.mStiffness;
            }

            // apply gravity
            particle.mForce += (1.0f / particle.mInvMass) * particle.mGravityFactor * (mGravity * 0.01f);

            // damp the force by the force blend factor
            // this blend factor is fading in smoothly when activating the controller
            // which prevents the particles from going crazy when suddenly there is a huge force being applied to them when being activated
            particle.mForce *= mForceBlendFactor;
        }
    }



    // satisfy the constraints of the spring system
    void JiggleController::SatisfyConstraints(const MCore::Matrix* globalMatrices, float interpolationFraction)
    {
        // for all iterations
        for (uint32 n = 0; n < mNumIterations; ++n)
        {
            // for all our springs
            const uint32 numSprings = mSprings.GetLength();
            //      for (int32 i=numSprings-1; i>=0; --i)
            for (uint32 i = 0; i < numSprings; ++i)
            {
                // get the spring and its two particles
                const Spring& spring = mSprings[i];
                Particle& particleA = mParticles[spring.mParticleA];
                Particle& particleB = mParticles[spring.mParticleB];

                const MCore::Vector3 fixedPosA = MCore::LinearInterpolate<MCore::Vector3>(particleA.mInterpolateStartPos, globalMatrices[particleA.mNodeIndex].GetTranslation(), interpolationFraction);
                const MCore::Vector3 fixedPosB = MCore::LinearInterpolate<MCore::Vector3>(particleB.mInterpolateStartPos, globalMatrices[particleB.mNodeIndex].GetTranslation(), interpolationFraction);

                // calculate the stretch amount of the spring
                const MCore::Vector3 delta = particleB.mPos - particleA.mPos;
                const float deltaLength = MCore::Math::SafeSqrt(delta.SquareLength());
                float diff;
                if (deltaLength > MCore::Math::epsilon)
                {
                    diff = (deltaLength - spring.mRestLength) / (deltaLength * (particleA.mInvMass + particleB.mInvMass)); // TODO: introduce stiffness/elasticity here?
                }
                else
                {
                    diff = 0.0f;
                }

                // if we allow stretching
                if (spring.mAllowStretch)
                {
                    if (particleA.mFixed)
                    {
                        particleA.mPos = fixedPosA;
                    }
                    else
                    {
                        particleA.mPos += delta * (0.5f * diff);
                    }

                    // try to relax the second particle of the spring
                    if (particleB.mFixed)
                    {
                        particleB.mPos = fixedPosB;
                    }
                    else
                    {
                        particleB.mPos -= delta * (0.5f * diff);
                    }
                }
                else // force a fixed length
                {
                    float compensate = (deltaLength - spring.mRestLength) * 0.5f;
                    if (compensate < 0.0f)
                    {
                        compensate = 0.0f;
                    }
                    else
                    if (compensate > 0.1f)
                    {
                        compensate = 0.1f;
                    }

                    const MCore::Vector3 deltaNormalized = delta.SafeNormalized();

                    // try to relax the first particle of the spring
                    if (particleA.mFixed)
                    {
                        particleA.mPos = fixedPosA;
                    }
                    else
                    {
                        particleA.mPos += deltaNormalized * compensate;
                    }

                    // try to relax the second particle of the spring
                    if (particleB.mFixed)
                    {
                        particleB.mPos = fixedPosB;
                    }
                    else
                    {
                        particleB.mPos -= deltaNormalized * compensate;
                    }

                    // try to relax the first particle of the spring
                    //if ((particleA.mPos - particleB.mPos).SafeLength() > spring.mRestLength)
                    {
                        if (particleA.mFixed)
                        {
                            particleA.mPos = fixedPosA;
                        }
                        else
                        {
                            particleA.mPos = particleB.mPos - deltaNormalized * spring.mRestLength;
                        }
                    }
                }

                //-------------------------
                // cone angular constraint
                //-------------------------
                // calc the vector pointing towards the child in global space as the animation outputs
                MCore::Vector3 animDir = fixedPosA - fixedPosB;
                animDir.SafeNormalize();

                // now calculate our current spring direction
                MCore::Vector3 springDir = particleA.mPos - particleB.mPos;
                const float springLength = springDir.SafeLength();
                if (springLength > MCore::Math::epsilon) // TODO: what if this isn't the case?
                {
                    springDir /= springLength;
                }

                // calculate the angle between the vectors
                float dotResult = springDir.Dot(animDir);
                dotResult = MCore::Clamp<float>(dotResult, -1.0f, 1.0f);

                // if its outside of its limits
                float angle = MCore::Math::ACos(dotResult);
                if (angle > MCore::Math::DegreesToRadians(particleB.mConeAngleLimit))
                {
                    // calculate the rotation vector
                    const MCore::Vector3 rotAxis = animDir.Cross(springDir);

                    // remove the rotation that is too much
                    angle = MCore::Math::RadiansToDegrees(angle) - particleB.mConeAngleLimit;

                    // rotate the vector back into the allowed cone
                    const MCore::Quaternion rot(rotAxis, MCore::Math::DegreesToRadians(-angle));
                    springDir = (rot * springDir) * springLength;

                    // update the position
                    particleA.mPos = particleB.mPos + springDir;
                }

                /*
                            //-------------------------
                            // yaw angular constraint
                            //-------------------------
                            // get the rotation axis for yaw and pitch
                            Vector3 yawAxis     = globalMatrices[particleB.mNodeIndex].GetRow( particleB.mYawAxis );
                            Vector3 pitchAxis   = globalMatrices[particleB.mNodeIndex].GetRow( particleB.mPitchAxis );
                            yawAxis.Normalize();
                            pitchAxis.Normalize();

                            // construct the limit plane
                            PlaneEq yawPlane(yawAxis, globalMatrices[particleB.mNodeIndex].GetTranslation());

                            // project vector on the plane
                            Vector3 jointDir     = particleA.mPos - particleB.mPos;
                            Vector3 projectedDir = yawPlane.Project( jointDir );
                            projectedDir.Normalize();

                            // calculate angle in degrees
                            dotResult = projectedDir.Dot(animDir);
                            dotResult = MCore::Clamp<float>(dotResult, -1.0f, 1.0f);
                            angle = Math::ACos( dotResult );

                            Vector3 distPos = particleB.mPos + animDir.Cross( projectedDir );
                            float dist = yawPlane.CalcDistanceTo( distPos );

                            // limit the angle
                            if (dist < 0.0f)
                            {
                                if (angle > Math::DegreesToRadians(particleB.mYawLimitMin))
                                {
                                    // calculate the rotation vector
                                    Vector3 rotAxis = animDir.Cross( projectedDir );

                                    // remove the rotation that is too much
                                    angle = Math::RadiansToDegrees(angle) - particleB.mYawLimitMin;

                                    // rotate the vector back into the allowed cone
                                    Quaternion rot(rotAxis, Math::DegreesToRadians(-angle));
                                    springDir = (rot * jointDir.Normalized()) * springLength;

                                    particleA.mPos = particleB.mPos + springDir;
                                }
                            }
                            else
                            {
                                if (angle > Math::DegreesToRadians(particleB.mYawLimitMax))
                                {
                                    // calculate the rotation vector
                                    Vector3 rotAxis = animDir.Cross( projectedDir );

                                    // remove the rotation that is too much
                                    angle = Math::RadiansToDegrees(angle) - particleB.mYawLimitMax;

                                    // rotate the vector back into the allowed cone
                                    Quaternion rot(rotAxis, Math::DegreesToRadians(-angle));
                                    springDir = (rot * jointDir.Normalized()) * springLength;

                                    particleA.mPos = particleB.mPos + springDir;
                                }
                            }
                */
                /*
                            //-------------------------
                            // pitch angular constraint
                            //-------------------------
                            // construct the limit plane
                            PlaneEq pitchPlane(pitchAxis, globalMatrices[particleB.mNodeIndex].GetTranslation());

                            // project vector on the plane
                            projectedDir = pitchPlane.Project( jointDir );
                            projectedDir.Normalize();

                            // calculate angle in degrees
                            dotResult = projectedDir.Dot(animDir);
                            dotResult = MCore::Clamp<float>(dotResult, -1.0f, 1.0f);
                            angle = Math::ACos( dotResult );

                            distPos = particleB.mPos + animDir.Cross( projectedDir );
                            dist = pitchPlane.CalcDistanceTo( distPos );

                            // limit the angle
                            if (dist < 0.0f)
                            {
                                if (angle > Math::DegreesToRadians(particleB.mPitchLimitMin))
                                {
                                    // calculate the rotation vector
                                    Vector3 rotAxis = animDir.Cross( projectedDir );

                                    // remove the rotation that is too much
                                    angle = Math::RadiansToDegrees(angle) - particleB.mPitchLimitMin;

                                    // rotate the vector back into the allowed cone
                                    Quaternion rot(rotAxis, Math::DegreesToRadians(-angle));
                                    springDir = (rot * jointDir.Normalized()) * springLength;

                                    particleA.mPos = particleB.mPos + springDir;
                                }
                            }
                            else
                            {
                                if (angle > Math::DegreesToRadians(particleB.mPitchLimitMax))
                                {
                                    // calculate the rotation vector
                                    Vector3 rotAxis = animDir.Cross( projectedDir );

                                    // remove the rotation that is too much
                                    angle = Math::RadiansToDegrees(angle) - particleB.mPitchLimitMax;

                                    // rotate the vector back into the allowed cone
                                    Quaternion rot(rotAxis, Math::DegreesToRadians(-angle));
                                    springDir = (rot * jointDir.Normalized()) * springLength;

                                    particleA.mPos = particleB.mPos + springDir;
                                }
                            }*/
            } // for all springs


            // perform collision detection
            if (mCollisionDetection)
            {
                // for all particles
                const uint32 numParticles = mParticles.GetLength();
                for (uint32 p = 0; p < numParticles; ++p)
                {
                    if (mParticles[p].mFixed == false) // dont collide fixed particles
                    {
                        if (PerformCollision(mParticles[p].mPos)) // if there was a collision
                        {
                            mParticles[p].mOldPos = MCore::LinearInterpolate<MCore::Vector3>(mParticles[p].mOldPos, mParticles[p].mPos, mParticles[p].mFriction); // apply some sort of friction
                        }
                    }
                }
            }
        } // for all iterations
    }


    // perform a simulation step
    void JiggleController::Simulate(float deltaTime, const MCore::Matrix* globalMatrices, float interpolationFraction)
    {
        // calculate the forces to apply
        CalcForces(globalMatrices, interpolationFraction);

        // integrate, calculating new particle positions
        Integrate(deltaTime);

        // apply constraints
        SatisfyConstraints(globalMatrices, interpolationFraction);
    }


    // update the global space matrices (the main method)
    void JiggleController::Update(GlobalPose* outPose, float timePassedInSeconds)
    {
        // get the global space matrices
        MCore::Matrix*  globalMatrices = outPose->GetGlobalMatrices();

        // prepare the collision objects
        PrepareCollisionObjects(globalMatrices);

        // store the start fixed pos for all particles
        const uint32 numParticles = mParticles.GetLength();
        for (uint32 i = 0; i < numParticles; ++i)
        {
            mParticles[i].mInterpolateStartPos = mParticles[i].mPos;
        }

        // update the force blend time
        // this is a value smoothly blending in, which is used to dampen the initial force when the controller gets activated
        // this prevents the bones from going crazy at the beginning
        mCurForceBlendTime += timePassedInSeconds;
        if (mCurForceBlendTime >= mForceBlendTime)
        {
            mCurForceBlendTime = mForceBlendTime;
            mForceBlendFactor = 1.0f;
        }
        else
        {
            mForceBlendFactor = MCore::CosineInterpolate(0.0f, 1.0f, mCurForceBlendTime / mForceBlendTime);
        }

        // perform fixed time stepping
        float stepSize = mFixedTimeStep;

        // make sure we don't update with too small stepsize, as that can cause issues
        if (stepSize < 0.005f)
        {
            stepSize = 0.005f;
        }

        mTimeAccumulation += timePassedInSeconds;

        const float totalStepTime = mTimeAccumulation - fmod(mTimeAccumulation, stepSize);
        float total = stepSize;
        float timeAccum = mTimeAccumulation;
        while (total <= timeAccum)
        {
            const float interpolationFraction = (totalStepTime > MCore::Math::epsilon) ? total / totalStepTime : 0.0f;

            // update the collision objects
            UpdateCollisionObjects(globalMatrices, interpolationFraction);

            Simulate(stepSize, globalMatrices, interpolationFraction);
            //Simulate( stepSize, globalMatrices, 1.0f );
            total += stepSize;
            mTimeAccumulation -= stepSize;
        }

        // update the transformation matrices
        UpdateGlobalMatrices(globalMatrices);
    }


    // update the global space matrices
    void JiggleController::UpdateGlobalMatrices(MCore::Matrix* globalMatrices)
    {
        MCore::Matrix rotMat;
        MCore::Matrix finalMat;

        // update all the nodes
        const uint32 numSprings = mSprings.GetLength();
        for (uint32 i = 0; i < numSprings; ++i)
        {
            // get the spring and its particles
            const Spring& spring = mSprings[i];

            // we dont need to update connection only springs
            if (spring.mIsSupportSpring)
            {
                continue;
            }

            const Particle& particleA   = mParticles[ spring.mParticleA ];
            const Particle& particleB   = mParticles[ spring.mParticleB ];// the parent of particle A

            // get the vector from parent to child as stored in the current global matrices
            MCore::Vector3 oldDir = globalMatrices[ particleA.mNodeIndex ].GetTranslation() - globalMatrices[ particleB.mNodeIndex ].GetTranslation();
            oldDir.SafeNormalize();

            // get the same vector but using new positions from the particle spring system
            MCore::Vector3 newDir = particleA.mPos - particleB.mPos;
            newDir.SafeNormalize();

            // also handle perpendicular cases
            const float dot = oldDir.Dot(newDir);
            if (dot < 1.0f - MCore::Math::epsilon)
            {
                rotMat.SetRotationMatrixTwoVectors(newDir, oldDir);

                // apply this rotation to the global matrix of the main particle's node
                finalMat.MultMatrix(globalMatrices[particleB.mNodeIndex], rotMat);
            }
            else // perpendicular
            {
                finalMat = globalMatrices[particleB.mNodeIndex];
            }

            finalMat.SetTranslation(particleB.mPos);

            // recursively update the node matrices (using forward kinematics, so very costly in performance)
            // this modifies the globalMatrices buffer, so the globalMatrices is the output
            // so if nodeIndex points to the upper leg, this will update the upper leg node's global space matrix
            // inside the globalMatrices array to to the matrix "finalMat"
            // also it will apply forward kinematics to the lower leg, feet and toes, so that they move/rotate/scale
            // with the upper leg
            mActorInstance->RecursiveUpdateGlobalTM(particleB.mNodeIndex, &finalMat, globalMatrices);
        }
    }


    // perform verlet integration
    void JiggleController::Integrate(float timeDelta)
    {
        // delta time squared
        const float timeDelta2  = timeDelta * timeDelta;
        //const float timeCorrect = (mLastTimeDelta > MCore::Math::epsilon) ? (timeDelta  / mLastTimeDelta) : 1.0f;

        // update all particles
        const uint32 numParticles = mParticles.GetLength();
        for (uint32 i = 0; i < numParticles; ++i)
        {
            Particle& particle = mParticles[i];

            // get the current and old position
            const MCore::Vector3 pos    = particle.mPos;
            const MCore::Vector3 oldPos = particle.mOldPos;

            // backup the old position
            particle.mOldPos = pos;

            // do the actual verlet integration to calculate the new position
            particle.mPos = ((2.0f - particle.mDamping) * pos - (1.0f - particle.mDamping) * oldPos) + (particle.mForce * timeDelta2);

            // time corrected verlet: xi+1 = xi + (xi - xi-1) * (dti / dti-1) + a * dti * dti
            //particle.mPos = pos + ((pos - oldPos) * timeCorrect * (1.0f-particle.mDamping)) + particle.mForce * timeDelta2;
        }

        mLastTimeDelta = timeDelta;
    }


    // scale the jiggle controller particle and spring settings
    void JiggleController::Scale(float scaleFactor)
    {
        uint32 i;

        // update all particles
        const uint32 numParticles = mParticles.GetLength();
        for (i = 0; i < numParticles; ++i)
        {
            Particle& particle = mParticles[i];
            particle.mStiffness *= scaleFactor;
            if (scaleFactor > 0.0f)
            {
                particle.mInvMass /= scaleFactor;
            }
            else
            {
                particle.mInvMass = 0.0f;
            }
        }

        // update the springs
        const uint32 numSprings = mSprings.GetLength();
        for (i = 0; i < numSprings; ++i)
        {
            mSprings[i].mRestLength *= scaleFactor;
        }

        // scale the collision object radius values
        const uint32 numColObjects = mCollisionObjects.GetLength();
        for (i = 0; i < numColObjects; ++i)
        {
            mCollisionObjects[i].mRadius *= scaleFactor;
        }
    }


    // add a sphere collision object
    void JiggleController::AddCollisionSphere(uint32 nodeIndex, const MCore::Vector3& center, float radius)
    {
        mCollisionObjects.AddEmpty();
        mCollisionObjects.GetLast().LinkToNode(nodeIndex);
        mCollisionObjects.GetLast().SetupSphere(center, radius);
    }


    // add a capsule collision object
    void JiggleController::AddCollisionCapsule(uint32 nodeIndex, const MCore::Vector3& start, const MCore::Vector3& end, float diameter)
    {
        mCollisionObjects.AddEmpty();
        mCollisionObjects.GetLast().LinkToNode(nodeIndex);
        mCollisionObjects.GetLast().SetupCapsule(start, end, diameter);
    }



    // update the global space start and end positions for all collision objects
    void JiggleController::PrepareCollisionObjects(const MCore::Matrix* globalMatrices)
    {
        // for all collision objects
        const uint32 numColObjects = mCollisionObjects.GetLength();
        for (uint32 i = 0; i < numColObjects; ++i)
        {
            CollisionObject& colObject = mCollisionObjects[i];

            if (colObject.mFirstUpdate)
            {
                if (colObject.mNodeIndex != MCORE_INVALIDINDEX32)
                {
                    colObject.mGlobalStart  = colObject.mStart * globalMatrices[colObject.mNodeIndex];
                    colObject.mGlobalEnd        = colObject.mEnd * globalMatrices[colObject.mNodeIndex];
                }
                colObject.mFirstUpdate  = false;
            }

            colObject.mInterpolationStart   = colObject.mGlobalStart;
            colObject.mInterpolationEnd     = colObject.mGlobalEnd;
        }
    }


    // update the global space start and end positions for all collision objects
    void JiggleController::UpdateCollisionObjects(const MCore::Matrix* globalMatrices, float interpolationFraction)
    {
        // for all collision objects
        const uint32 numColObjects = mCollisionObjects.GetLength();
        for (uint32 i = 0; i < numColObjects; ++i)
        {
            CollisionObject& colObject = mCollisionObjects[i];

            // if the collision object is linked to a node
            if (colObject.mNodeIndex != MCORE_INVALIDINDEX32)
            {
                // transform both start and end point with the global space matrix of the given node
                colObject.mGlobalStart  = MCore::LinearInterpolate<MCore::Vector3>(colObject.mInterpolationStart, colObject.mStart * globalMatrices[colObject.mNodeIndex], interpolationFraction);
                colObject.mGlobalEnd        = MCore::LinearInterpolate<MCore::Vector3>(colObject.mInterpolationEnd, colObject.mEnd * globalMatrices[colObject.mNodeIndex], interpolationFraction);
            }
            else // if it's not linked to a node
            {
                colObject.mGlobalStart  = MCore::LinearInterpolate<MCore::Vector3>(colObject.mInterpolationStart, colObject.mStart, interpolationFraction);
                colObject.mGlobalEnd        = MCore::LinearInterpolate<MCore::Vector3>(colObject.mInterpolationEnd, colObject.mEnd, interpolationFraction);
            }
        }
    }


    // perform collision detection between the input position and the collision objects
    bool JiggleController::PerformCollision(MCore::Vector3& inOutPos)
    {
        bool collided = false;

        // get the actor scale factor
        //  const float actorScaleFactor = mActorInstance->GetLocalScale().x;   // use the x as scale, assuming x, y and z are equal in scale

        // for all collision objects
        const uint32 numColObjects = mCollisionObjects.GetLength();
        for (uint32 i = 0; i < numColObjects; ++i)
        {
            const CollisionObject& colObject = mCollisionObjects[i];

            // check the type of collision object
            switch (colObject.mType)
            {
            // perform collision with a capsule
            case JiggleController::CollisionObject::COLTYPE_CAPSULE:
                if (CollideWithCapsule(inOutPos, colObject.mGlobalStart, colObject.mGlobalEnd, colObject.mRadius /* * actorScaleFactor*/))
                {
                    collided = true;
                }
                break;


            // perform collision with a sphere
            case JiggleController::CollisionObject::COLTYPE_SPHERE:
                if (CollideWithSphere(inOutPos, colObject.mGlobalStart, colObject.mRadius /* * actorScaleFactor*/))
                {
                    collided = true;
                }
                break;

            default:
                MCORE_ASSERT(false);    // may never happen, unknown collision object type
            }
            ;
        }

        return collided;
    }

    //-------

    // perform collision with a sphere
    bool JiggleController::CollideWithSphere(MCore::Vector3& pos, const MCore::Vector3& center, float radius)
    {
        // get the vector from the center to the position
        MCore::Vector3 centerToPoint = pos - center;

        // check the square distance, and see if it is outside of the sphere or not
        const float sqDist = centerToPoint.SquareLength();
        if (sqDist >= radius * radius) // its outside of the sphere, no collision needed
        {
            return false;
        }

        if (sqDist > MCore::Math::epsilon)
        {
            pos = center + radius * (centerToPoint / MCore::Math::FastSqrt(sqDist)); // yes there is a collision, push the input position outside of the sphere
        }
        else
        {
            pos = center;
        }

        return true;
    }


    // collide a particle with a capsule
    bool JiggleController::CollideWithCapsule(MCore::Vector3& pos, const MCore::Vector3& lineStart, const MCore::Vector3& lineEnd, float radius)
    {
        // a vector from start to end of the line
        const MCore::Vector3 startToEnd = lineEnd - lineStart;

        const float squareLength = startToEnd.SquareLength();
        float t;
        if (squareLength > MCore::Math::epsilon)
        {
            t = (pos - lineStart).Dot(startToEnd) / squareLength; // the distance of pos projected on the line
        }
        else
        {
            t = 0.0f;
        }

        // make sure that we clip this distance to be sure its on the line segment
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        // calculate the position projected on the line
        const MCore::Vector3 projected = lineStart + t * startToEnd;

        // the vector from the projected position to the point we are testing with
        MCore::Vector3 toPos = pos - projected;

        // get the squared distance
        const float sqLen = toPos.SquareLength();

        // if the distance is within the diameter of the capsule, it is colliding and then
        // the point needs to be pushed outside of the capsule
        if (sqLen < radius * radius) // TODO: precalc squared diameter?
        {
            toPos.SafeNormalize();
            pos = projected + (toPos * radius);
            return true;
        }
        else // the point is not colliding with the capsule, so ignore it
        {
            return false;
        }
    }

    /*
    // collide a particle with a capsule
    bool JiggleController::CollideWithCylinder(Vector3& pos, const Vector3& lineStart, const Vector3& lineEnd, float radius)
    {
        // a vector from start to end of the line
        Vector3 startToEnd = lineEnd - lineStart;

        // the distance of pos projected on the line
        const float t = (pos - lineStart).Dot(startToEnd) / startToEnd.SquareLength();

        // cap off the start and end
        if (t < 0.0f || t > 1.0f)
            return false;

        // calculate the position projected on the line
        const Vector3 projected = lineStart + t * startToEnd;

        // the vector from the projected position to the point we are testing with
        Vector3 toPos = pos - projected;

        // get the squared distance
        const float sqLen = toPos.SquareLength();

        // if they are about the same, return (prevent division by zero)
        if (sqLen <= 0.00001)
            return false;

        // if the distance is withing the diameter of the cylinder, it is colliding and then
        // the point needs to be pushed outside of the capsule
        if (sqLen < radius*radius)  // TODO: precalc squared diameter
        {
            toPos.FastNormalize();
            pos = projected + (toPos * radius);
            return true;
        }
        else    // the point is not colliding with the cylinder, so ignore it
            return false;
    }
    */
}   // namespace EMotionFX
