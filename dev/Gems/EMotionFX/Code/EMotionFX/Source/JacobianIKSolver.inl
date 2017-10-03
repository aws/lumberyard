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

// get the controller type
MCORE_INLINE uint32 JacobianIKSolver::GetType() const
{
    return JacobianIKSolver::TYPE_ID;
}


// get the type string
MCORE_INLINE const char* JacobianIKSolver::GetTypeString() const
{
    return "JacobianIKSolver";
}


// set the goal position in global space
MCORE_INLINE void JacobianIKSolver::SetGoalPos(uint32 chainIndex, const MCore::Vector3& goalPos)
{
    mEndEffectors[chainIndex].mGoalPos = goalPos;
}


// get the goal position in global space
MCORE_INLINE const MCore::Vector3& JacobianIKSolver::GetGoalPos(uint32 chainIndex) const
{
    return mEndEffectors[chainIndex].mGoalPos;
}


// set the goal position in global space
MCORE_INLINE void JacobianIKSolver::SetGoalRot(uint32 chainIndex, const MCore::Vector3& goalOri)
{
    mEndEffectors[chainIndex].mGoalOri = goalOri;
}


// get the goal position in global space
MCORE_INLINE const MCore::Vector3& JacobianIKSolver::GetGoalRot(uint32 chainIndex) const
{
    return mEndEffectors[chainIndex].mGoalOri;
}


// check if we want to limit the joints
MCORE_INLINE bool JacobianIKSolver::GetDoJointLimits() const
{
    return mConstrainJoints;
}


MCORE_INLINE void JacobianIKSolver::SetDoJointLimits(bool doLimits)
{
    mConstrainJoints = doLimits;
}


// check if we found a solution or not
MCORE_INLINE bool JacobianIKSolver::GetHasFoundSolution() const
{
    return mHasSolution;
}


// set the number of maximum iterations
MCORE_INLINE void JacobianIKSolver::SetMaxIterations(uint32 numIterations)
{
    mMaxIterations = numIterations;
}


// set the distance threshold
MCORE_INLINE void JacobianIKSolver::SetDistThreshold(float distThreshold)
{
    mDistThreshold = distThreshold;
}


// get the distance threshold
MCORE_INLINE float JacobianIKSolver::GetDistThreshold() const
{
    return mDistThreshold;
}


// get the number of maximum iterations
MCORE_INLINE uint32 JacobianIKSolver::GetMaxIterations() const
{
    return mMaxIterations;
}

// set the maximum goal distance
MCORE_INLINE void JacobianIKSolver::SetMaxGoalDist(float dist)
{
    mMaxGoalDist = dist;
}


// get the maximum distance
MCORE_INLINE float JacobianIKSolver::GetMaxGoalDist() const
{
    return mMaxGoalDist;
}


// set whether or not the root can move
MCORE_INLINE void JacobianIKSolver::SetRootCanTranslate(bool rct)
{
    mRootCanTranslate = rct;
}


// set whether or not the root can move
MCORE_INLINE bool JacobianIKSolver::GetRootCanTranslate() const
{
    return mRootCanTranslate;
}


// set whether or not the the solver should use the null space projector
MCORE_INLINE void JacobianIKSolver::SetUseNSP(bool useNSP)
{
    mUseNSP = useNSP;
}


// set whether or not the solver is using the null space projector
MCORE_INLINE bool JacobianIKSolver::GetUseNSP() const
{
    return mUseNSP;
}


// set the positional offset for an end effector
MCORE_INLINE void JacobianIKSolver::SetPosOffset(uint32 chainIndex, const MCore::Vector3& posOffset)
{
    mEndEffectors[chainIndex].mPosOffset = posOffset;
}


// set whether or not the solver is using the null space projector
MCORE_INLINE MCore::Vector3 JacobianIKSolver::GetPosOffset(uint32 chainIndex) const
{
    return mEndEffectors[chainIndex].mPosOffset;
}
