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
#include "EMotionFXConfig.h"
#include "Node.h"
#include "ActorInstance.h"
#include "NodeLimitAttribute.h"
#include "CCDIKSolver.h"
#include "GlobalPose.h"


namespace EMotionFX
{
    // constructor
    CCDIKSolver::CCDIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 endNodeIndex)
        : GlobalSpaceController(actorInstance)
    {
        // TODO: validate start and end node
        MCORE_ASSERT(startNodeIndex < actorInstance->GetNumNodes());
        MCORE_ASSERT(endNodeIndex < actorInstance->GetNumNodes());

        // the solver usually doesn't take more than 10 iterations to find the solution if its in reach (usually below 5).
        // if this is set higher, then the solver will meerly take longer to realise that it cant reach the goal
        mStartNodeIndex = startNodeIndex;
        mEndNodeIndex   = endNodeIndex;
        mDistThreshold  = 0.001f;
        mMaxIterations  = 10;
        mHasSolution    = false;
        mDoJointLimits  = true;
        mGoal.Set(0.0f, 0.0f, 0.0f);

        // specify that this controller will modify all nodes that come after the start node inside the hierarchy
        RecursiveSetNodeMask(mStartNodeIndex, true);
    }


    // destructor
    CCDIKSolver::~CCDIKSolver()
    {
    }


    // create method
    CCDIKSolver* CCDIKSolver::Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 endNodeIndex)
    {
        return new CCDIKSolver(actorInstance, startNodeIndex, endNodeIndex);
    }


    // set the goal position in global space
    void CCDIKSolver::SetGoal(const MCore::Vector3& goal)
    {
        mGoal = goal;
    }


    // set the mDoJointLimits variable
    void CCDIKSolver::SetDoJointLimits(bool doJointLimits)
    {
        mDoJointLimits = doJointLimits;
    }


    // check if we want to limit the joints
    bool CCDIKSolver::GetDoJointLimits() const
    {
        return mDoJointLimits;
    }


    // get the goal position in global space
    const MCore::Vector3& CCDIKSolver::GetGoal() const
    {
        return mGoal;
    }


    // check if we found a solution or not
    bool CCDIKSolver::GetHasFoundSolution() const
    {
        return mHasSolution;
    }


    // set the number of maximum iterations
    void CCDIKSolver::SetMaxIterations(uint32 numIterations)
    {
        mMaxIterations = numIterations;
    }


    // set the distance threshold
    void CCDIKSolver::SetDistThreshold(float distThreshold)
    {
        mDistThreshold = distThreshold;
    }


    // get the distance threshold
    float CCDIKSolver::GetDistThreshold() const
    {
        return mDistThreshold;
    }


    // get the number of maximum iterations
    uint32 CCDIKSolver::GetMaxIterations() const
    {
        return mMaxIterations;
    }


    // the main update method
    void CCDIKSolver::Update(GlobalPose* outPose, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        // get the global space matrices and scale values
        MCore::Matrix*  globalMatrices  = outPose->GetGlobalMatrices();
        Actor*          actor           = mActorInstance->GetActor();
        Skeleton*       skeleton        = actor->GetSkeleton();

        // some values we need
        uint32  curIteration  = 0;
        float   dist          = FLT_MAX;
        Node*   curNode       = skeleton->GetNode(mEndNodeIndex)->GetParentNode();

        // the CCD iteration loop
        while (curIteration < mMaxIterations&& dist > mDistThreshold)
        {
            // required bone positions
            MCore::Vector3 rootPos  = globalMatrices[curNode->GetNodeIndex()].GetTranslation();
            MCore::Vector3 curEnd   = globalMatrices[mEndNodeIndex].GetTranslation();

            // if we are far enough from the goal
            dist = ErrorMeasure(curEnd);
            if (dist > mDistThreshold)
            {
                // construct a rotated globalMatrix
                MCore::Matrix tempMat = globalMatrices[curNode->GetNodeIndex()] * RotMatBallAndSocket(curEnd, rootPos); // Ball And Socket Joint
                /*
                            // Get our rotational limit attribute
                            NodeLimitAttribute* attribute = (NodeLimitAttribute*)curNode->GetAttributeByType( NodeLimitAttribute::TYPE_ID );

                            // no need to do all these calculations if there is no valid NodeLimitAttribute or if the user
                            // doesnt want us to do joint limits, or if there is no parent to constrain against
                            const uint32 parentNodeIndex = curNode->GetParentIndex();
                            if (attribute && mDoJointLimits)
                            {
                                    Vector3 min = attribute->GetRotationMin();
                                    Vector3 max = attribute->GetRotationMax();

                                    // convert the matrix into local coordinates of the node
                                    if (parentNodeIndex != MCORE_INVALIDINDEX32)
                                        tempMat *= globalMatrices[parentNodeIndex].Inversed();

                                    // clip the matrix to these max and min attributes
                                    Vector3 euler = tempMat.Normalized().CalcEulerAngles();

                                    euler.x = MCore::Clamp<float>(euler.x, min.x, max.x);
                                    euler.y = MCore::Clamp<float>(euler.y, min.y, max.y);
                                    euler.z = MCore::Clamp<float>(euler.z, min.z, max.z);

                                    // build a matrix again from the resulting euler angles
                                    tempMat.SetRotationMatrixEulerZYX( euler );

                                    // convert back into global coordinates
                                    if (parentNodeIndex != MCORE_INVALIDINDEX32)
                                        tempMat *= globalMatrices[parentNodeIndex];
                            }
                */

                // add correct scale and translation to the rotation matrix
                tempMat.SetTranslation(globalMatrices[curNode->GetNodeIndex()].GetTranslation());

                // recursively update the global TM (forward kinematics)
                mActorInstance->RecursiveUpdateGlobalTM(curNode->GetNodeIndex(), &tempMat, globalMatrices);

                // go one node up in the hierarchy (towards the front of the chain)
                curNode = curNode->GetParentNode();

                // if we reach the start of the chain, go to the end again and increment the iteration counter
                if (curNode == skeleton->GetNode(mStartNodeIndex)->GetParentNode())
                {
                    curNode = skeleton->GetNode(mEndNodeIndex)->GetParentNode();
                    curIteration++;
                }
            } // if (dist > mDistThreshold )
        } // while loop

        // check if we found a solution
        if (curIteration >= mMaxIterations)
        {
            mHasSolution = false;
        }
        else
        {
            mHasSolution = true;
        }
    }


    // calculate the error measure
    float CCDIKSolver::ErrorMeasure(MCore::Vector3& curPos) const
    {
        return (mGoal - curPos).SafeLength();
    }


    // create the rotation matrix for the ball-and-socket joint
    MCore::Matrix CCDIKSolver::RotMatBallAndSocket(MCore::Vector3& curEnd, MCore::Vector3& rootPos)
    {
        // vector to the current effector pos
        MCore::Vector3 curVector = (curEnd - rootPos);

        // desired effector position vector
        MCore::Vector3 targetVector = (mGoal - rootPos);

        // build the rotation matrix from these vectors
        MCore::Matrix rotMat;
        rotMat.SetRotationMatrixTwoVectors(targetVector, curVector);

        return rotMat;
    }

    // clone the controller
    GlobalSpaceController* CCDIKSolver::Clone(ActorInstance* targetActorInstance)
    {
        CCDIKSolver* solver = new CCDIKSolver(targetActorInstance, mStartNodeIndex, mEndNodeIndex);

        // copy over the members
        solver->mDistThreshold  = mDistThreshold;
        solver->mMaxIterations  = mMaxIterations;
        solver->mHasSolution    = false;
        solver->mDoJointLimits  = mDoJointLimits;
        solver->mGoal           = mGoal;

        // copy the base class weight settings
        // this makes sure the controller is in the same activated/deactivated and blending state
        // as the source controller that we are cloning
        solver->CopyBaseClassWeightSettings(this);

        return solver;
    }


    // get the type ID
    uint32 CCDIKSolver::GetType() const
    {
        return CCDIKSolver::TYPE_ID;
    }


    // get the type string
    const char* CCDIKSolver::GetTypeString() const
    {
        return "CCDIKSolver";
    }
} // namespace EMotionFX
