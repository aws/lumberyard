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
#include "TwoLinkIKSolver.h"
#include "Node.h"
#include "ActorInstance.h"
#include "GlobalPose.h"


namespace EMotionFX
{
    // the constructor
    TwoLinkIKSolver::TwoLinkIKSolver(ActorInstance* actorInstance, uint32 endNodeIndex)
        : GlobalSpaceController(actorInstance)
    {
        // make sure the end node index is valid
        MCORE_ASSERT(endNodeIndex < actorInstance->GetActor()->GetNumNodes());

        // init some default settings
        mHasSolution        = false;
        mGoalNodeIndex      = MCORE_INVALIDINDEX32;
        mUseRelativeBendDir = false;
        mBendDirNodeIndex   = MCORE_INVALIDINDEX32;

        mGoal.Set(0.0f, 0.0f, 0.0f);
        mBendDirection.Set(0, -1, 0);

        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();

        // setup the node indices
        mEndNodeIndex   = endNodeIndex;
        mMidNodeIndex   = skeleton->GetNode(mEndNodeIndex)->GetParentIndex();
        MCORE_ASSERT(mMidNodeIndex != MCORE_INVALIDINDEX32);
        mStartNodeIndex = skeleton->GetNode(mMidNodeIndex)->GetParentIndex();
        MCORE_ASSERT(mStartNodeIndex != MCORE_INVALIDINDEX32);

        // specify that this controller will modify all nodes that come after the start node inside the hierarchy
        RecursiveSetNodeMask(mStartNodeIndex, true);
    }


    // constructor
    TwoLinkIKSolver::TwoLinkIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex)
        : GlobalSpaceController(actorInstance)
    {
        // make sure the end node index is valid
        MCORE_ASSERT(startNodeIndex < actorInstance->GetActor()->GetNumNodes());
        MCORE_ASSERT(midNodeIndex < actorInstance->GetActor()->GetNumNodes());
        MCORE_ASSERT(endNodeIndex < actorInstance->GetActor()->GetNumNodes());

        // init some default settings
        mHasSolution    = false;
        mGoalNodeIndex  = MCORE_INVALIDINDEX32;
        mGoal.Set(0.0f, 0.0f, 0.0f);
        mBendDirection.Set(0, -1, 0);
        mUseRelativeBendDir = false;

        // setup the node indices
        mStartNodeIndex = startNodeIndex;
        mMidNodeIndex   = midNodeIndex;
        mEndNodeIndex   = endNodeIndex;

        // specify that this controller will modify all nodes that come after the start node inside the hierarchy
        RecursiveSetNodeMask(mStartNodeIndex, true);
    }


    // create
    TwoLinkIKSolver* TwoLinkIKSolver::Create(ActorInstance* actorInstance, uint32 endNodeIndex)
    {
        return new TwoLinkIKSolver(actorInstance, endNodeIndex);
    }


    // create
    TwoLinkIKSolver* TwoLinkIKSolver::Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex)
    {
        return new TwoLinkIKSolver(actorInstance, startNodeIndex, midNodeIndex, endNodeIndex);
    }


    // set the goal
    void TwoLinkIKSolver::SetGoal(const MCore::Vector3& goal)
    {
        mGoal = goal;
    }


    // set the bend direction
    void TwoLinkIKSolver::SetBendDirection(const MCore::Vector3& bendDir)
    {
        mBendDirection = bendDir;
    }


    // get the goal
    const MCore::Vector3& TwoLinkIKSolver::GetGoal() const
    {
        return mGoal;
    }


    // get the bend direction
    const MCore::Vector3& TwoLinkIKSolver::GetBendDirection() const
    {
        return mBendDirection;
    }


    // set the relative bend direction and the node index
    void TwoLinkIKSolver::SetRelativeBendDir(const MCore::Vector3& bendDir, uint32 bendDirNodeIndex)
    {
        mRelativeBendDir = bendDir;
        mBendDirNodeIndex = bendDirNodeIndex;
    }


    // set the relative bend direction
    void TwoLinkIKSolver::SetRelativeBendDir(const MCore::Vector3& bendDir)
    {
        mRelativeBendDir = bendDir;
    }


    // set the relative bend direction node index
    void TwoLinkIKSolver::SetRelativeBendDirNode(uint32 bendDirNodeIndex)
    {
        mBendDirNodeIndex = bendDirNodeIndex;
    }


    // get the relative bend direction
    const MCore::Vector3& TwoLinkIKSolver::GetRelativeBendDir() const
    {
        return mRelativeBendDir;
    }


    // get the relative bend direction node index
    uint32 TwoLinkIKSolver::GetRelativeBendDirNode() const
    {
        return mBendDirNodeIndex;
    }


    // set the DoRelBendDir variable
    void TwoLinkIKSolver::SetUseRelativeBendDir(bool doRelBendDir)
    {
        mUseRelativeBendDir = doRelBendDir;
    }


    // get the mUseRelativeBendDir variable
    bool TwoLinkIKSolver::GetUseRelativeBendDir() const
    {
        return mUseRelativeBendDir;
    }


    // get the controller type
    uint32 TwoLinkIKSolver::GetType() const
    {
        return TwoLinkIKSolver::TYPE_ID;
    }


    // get the type string
    const char* TwoLinkIKSolver::GetTypeString() const
    {
        return "TwoLinkIKSolver";
    }


    // check if we have found a solution or not
    bool TwoLinkIKSolver::GetHasFoundSolution() const
    {
        return mHasSolution;
    }


    // solve the IK problem by calculating the 'knee/elbow' position
    bool TwoLinkIKSolver::Solve2LinkIK(const MCore::Vector3& posA, const MCore::Vector3& posB, const MCore::Vector3& posC, const MCore::Vector3& goal, const MCore::Vector3& bendDir, MCore::Vector3* outMidPos)
    {
        const MCore::Vector3 localGoal = goal - posA;
        const float rLen = localGoal.SafeLength();

        // get the lengths of the bones A and B
        const float lengthA = (posB - posA).SafeLength();
        const float lengthB = (posC - posB).SafeLength();

        // calculate the d and e values from the equations by Ken Perlin
        //const float d = MCore::Max<float>(0.0f, Min<float>(lengthA, (rLen + (lengthA*lengthA - lengthB*lengthB) / rLen) * 0.5f));
        const float d = (rLen > MCore::Math::epsilon) ? MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, (rLen + (lengthA * lengthA - lengthB * lengthB) / rLen) * 0.5f)) : MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, rLen));
        const float square = lengthA * lengthA - d * d;
        const float e = MCore::Math::SafeSqrt(square);

        // the solution on the YZ plane
        const MCore::Vector3 solution(d, e, 0);

        // calculate the matrix that rotates from IK solve space into global space
        MCore::Matrix matForward;
        matForward.Identity();
        TwoLinkIKSolver::CalculateMatrix(localGoal, bendDir, &matForward);

        // rotate the solution (the mid "knee/elbow" position) into global space
        *outMidPos = posA + matForward.Mul3x3(solution);

        // check if we found a solution or not
        return (d > MCore::Math::epsilon && d < lengthA + MCore::Math::epsilon);
    }


    // calculate the direction matrix
    void TwoLinkIKSolver::CalculateMatrix(const MCore::Vector3& goal, const MCore::Vector3& bendDir, MCore::Matrix* outForward)
    {
        // the inverse matrix defines a coordinate system whose x axis contains P, so X = unit(P).
        MCore::Vector3 x = goal;
        x.SafeNormalize();

        // the y axis of the inverse is perpendicular to P, so Y = unit( D - X(D�X) ).
        const float dot = bendDir.Dot(x);
        MCore::Vector3 y = bendDir - (dot * x);
        y.SafeNormalize();

        // the z axis of the inverse is perpendicular to both X and Y, so Z = X�Y.
        MCore::Vector3 z = x.Cross(y);

        // set the rotation vectors of the output matrix
        outForward->SetRow(0, x);
        outForward->SetRow(1, y);
        outForward->SetRow(2, z);
    }


    // clone the controller
    GlobalSpaceController* TwoLinkIKSolver::Clone(ActorInstance* targetActorInstance)
    {
        // create the clone
        TwoLinkIKSolver* clone = new TwoLinkIKSolver(targetActorInstance, mEndNodeIndex);

        // setup the same user defined properties
        clone->SetGoalNodeIndex(mGoalNodeIndex);
        clone->SetGoal(mGoal);
        clone->SetBendDirection(mBendDirection);

        // copy the base class weight settings
        // this makes sure the controller is in the same activated/deactivated and blending state
        // as the source controller that we are cloning
        clone->CopyBaseClassWeightSettings(this);

        // return the clone
        return clone;
    }


    // the main update method
    void TwoLinkIKSolver::Update(GlobalPose* outPose, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        // get the global space matrices
        MCore::Matrix* globalMatrices = outPose->GetGlobalMatrices();

        // get the globalspace positions of the three nodes
        const MCore::Vector3 posA = globalMatrices[ mStartNodeIndex ].GetTranslation();
        const MCore::Vector3 posB = globalMatrices[ mMidNodeIndex ].GetTranslation();
        MCore::Vector3 posC = globalMatrices[ mEndNodeIndex ].GetTranslation();

        // get the goal we want (depending on if we want to use a node as goal or not)
        MCore::Vector3 goal;
        if (mGoalNodeIndex != MCORE_INVALIDINDEX32)
        {
            goal = globalMatrices[ mGoalNodeIndex ].GetTranslation();
        }
        else
        {
            goal = mGoal;
        }

        // solve the IK problem by calculating the new position of the mid node in globalspace
        MCore::Vector3 midPos;
        if (mUseRelativeBendDir)
        {
            const MCore::Vector3 bendDir = globalMatrices[ mBendDirNodeIndex ].Mul3x3(mRelativeBendDir);
            mHasSolution = TwoLinkIKSolver::Solve2LinkIK(posA, posB, posC, goal, bendDir, &midPos);
        }
        else
        {
            mHasSolution = TwoLinkIKSolver::Solve2LinkIK(posA, posB, posC, goal, mBendDirection, &midPos);
        }

        // --------------------------------------
        // calculate the first bone orientation
        // --------------------------------------
        MCore::Vector3 forward, relGoal;
        MCore::Matrix rotMat;

        // the vector pointing down the bone in the old orientataion
        forward = (posB - posA);

        // where the new vector should be pointing
        relGoal = (midPos - posA);

        // build the rotation matrix from these vectors
        rotMat.SetRotationMatrixTwoVectors(relGoal, forward);

        // rotate and translate the node matrix
        MCore::Matrix mat = globalMatrices[mStartNodeIndex] * rotMat;
        mat.SetTranslation(posA);

        // recursively update the bone matrices (forward kinematics)
        mActorInstance->RecursiveUpdateGlobalTM(mStartNodeIndex, &mat, globalMatrices);

        // Set the midPos using this rotated bone
        midPos = globalMatrices[ mMidNodeIndex ].GetTranslation();

        // --------------------------------------
        // calculate the second bone orientation
        // --------------------------------------

        // Get the end position of the second bone, posC
        posC = globalMatrices[ mEndNodeIndex ].GetTranslation();

        // the vector pointing down the bone
        forward = (posC - midPos);

        // where the vector should be pointing
        relGoal = (goal - midPos);

        // build the rotation matrix from these vectors
        rotMat.SetRotationMatrixTwoVectors(relGoal, forward);

        // rotate and translate the node matrix
        mat = globalMatrices[mMidNodeIndex] * rotMat;
        mat.SetTranslation(midPos);

        // recursively update the bone matrices (forward kinematics)
        mActorInstance->RecursiveUpdateGlobalTM(mMidNodeIndex, &mat, globalMatrices);
    }


    uint32 TwoLinkIKSolver::GetGoalNodeIndex() const
    {
        return mGoalNodeIndex;
    }


    bool TwoLinkIKSolver::GetIsUsingGoalNode() const
    {
        return (mGoalNodeIndex != MCORE_INVALIDINDEX32);
    }


    void TwoLinkIKSolver::SetGoalNodeIndex(uint32 goalNodeIndex)
    {
        mGoalNodeIndex = goalNodeIndex;
    }


    void TwoLinkIKSolver::DisableGoalNode()
    {
        mGoalNodeIndex = MCORE_INVALIDINDEX32;
    }


    uint32 TwoLinkIKSolver::GetStartNodeIndex() const
    {
        return mStartNodeIndex;
    }


    uint32 TwoLinkIKSolver::GetMidNodeIndex() const
    {
        return mMidNodeIndex;
    }


    uint32 TwoLinkIKSolver::GetEndNodeIndex() const
    {
        return mEndNodeIndex;
    }
} // namespace EMotionFX
