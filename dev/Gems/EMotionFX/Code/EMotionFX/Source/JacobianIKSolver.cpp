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
#include "JacobianIKSolver.h"
#include "NodeLimitAttribute.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "Node.h"


namespace EMotionFX
{
    // the constructor
    JacobianIKSolver::JacobianIKSolver(ActorInstance* actorInstance)
        : LocalSpaceController(actorInstance)
    {
        mMaxIterations  = 10;
        mHasSolution    = false;
        mDistThreshold  = 0.001f;
        mMaxGoalDist    = 0.85f;
        mConstrainJoints = true;
        mProcessRoot    = false;
        mRootCanTranslate = false;
        mUseNSP         = true;
        mJacobian.SetDimensions(0, 0);

        // get some pointers for fast access
        mTFD             = mActorInstance->GetTransformData();
        mLocalMatrices   = mTFD->GetLocalMatrices();
        mGlobalMatrices  = mTFD->GetGlobalInclusiveMatrices();

        // pre-allocate data
        mU              = new MCore::NMatrix();// we allocate the matrices, as we will swap pointers later, as optimization
        mV              = new MCore::NMatrix();
        mJointState     = new MCore::Array<float>();
        mNewJointState  = new MCore::Array<float>();
    }


    // the destructor
    JacobianIKSolver::~JacobianIKSolver()
    {
        delete mU;
        delete mV;
        delete mJointState;
        delete mNewJointState;
    }


    // creation method
    JacobianIKSolver* JacobianIKSolver::Create(ActorInstance* actorInstance)
    {
        return new JacobianIKSolver(actorInstance);
    }


    // checks if a joint is already in the joint array and adds the new end effector index to the joint if it is found
    bool JacobianIKSolver::FindJoint(const Joint& joint)
    {
        // for all joints
        for (uint32 i = 0; i < mJoints.GetLength(); ++i)
        {
            // check if the this joint uses the same node index
            if (mJoints[i].mNodeIndex == joint.mNodeIndex)
            {
                mJoints[i].mEndEffectors.Add(mEndEffectors.GetLength());
                return true;
            }
        }

        // no joint found yet
        return false;
    }


    // processes the root node and adds it to the solver for solving
    void JacobianIKSolver::ProcessRoot(Node* root)
    {
        // add the new end effector
        mRootJoint.mEndEffectors.Add(mEndEffectors.GetLength());

        // no need to reprocess anything else
        if (mProcessRoot)
        {
            return;
        }

        mProcessRoot = true;                        // the root has been processed
        mRootJoint.mNodeIndex = root->GetNodeIndex(); // set the nodeindex of the root's joint for lookups
        mRootJoint.mDOFs.Clear();                   // clear the DOF array

        if (mRootCanTranslate)
        {
            // the root node can traverse and hence we require 6 unconstrained DOFs
            for (uint32 i = 0; i < 6; ++i)
            {
                mRootJoint.mDOFs.Add(i % 3);
            }

            mRootJoint.mMinLimits.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            mRootJoint.mMaxLimits.Set(FLT_MAX,  FLT_MAX,  FLT_MAX);
        }
        else
        {
            // the root cant traverse, so there will be a maximum of 3 DOFs
            NodeLimitAttribute* attribute = (NodeLimitAttribute*)root->GetAttributeByType(NodeLimitAttribute::TYPE_ID);

            if (attribute == nullptr || mConstrainJoints == false)
            {
                // if there is no limit attribute then the joint has 3 DOFs and they are all unconstrained
                for (uint32 i = 0; i < 3; ++i)
                {
                    mRootJoint.mDOFs.Add(static_cast<uint16>(i));
                }

                mRootJoint.mMinLimits.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
                mRootJoint.mMaxLimits.Set(FLT_MAX,  FLT_MAX,  FLT_MAX);
            }
            else
            {
                // the joint can have up to 3 DOFs, so we will loop through each and check the limits
                // if the limits are essentially the same for any of the DOFs then that DOF is completely
                // restricted and will not be solved by the solver
                //mRootJoint.mMaxLimits = -attribute->GetRotationMin(); // store the joint limits
                //mRootJoint.mMinLimits = -attribute->GetRotationMax();

                mRootJoint.mMaxLimits = attribute->GetRotationMax(); // store the joint limits
                mRootJoint.mMinLimits = attribute->GetRotationMin();

                for (uint32 i = 0; i < 3; ++i)
                {
                    if (MCore::Math::Abs((float)((&mRootJoint.mMinLimits.x)[i]) - (float)((&mRootJoint.mMaxLimits.x)[i])) < 0.0001)
                    {
                        // no DOF
                        (&mRootJoint.mMinLimits.x)[i] = (&mRootJoint.mMaxLimits.x)[i] = ((&mRootJoint.mMinLimits.x)[i] + (&mRootJoint.mMaxLimits.x)[i]) * 0.5f;
                    }
                    else
                    {
                        mRootJoint.mDOFs.Add(static_cast<uint16>(i));
                    }
                }
            }
        }
    }


    // process a chain of nodes from the actor to be solved
    // each chain has a corresponding end effector
    uint32 JacobianIKSolver::AddChain(const Node* startNode, Node* endNode, bool doPos, bool doOri)
    {
        Node*               tempNode    = endNode;
        NodeLimitAttribute* attribute   = nullptr;

        // if we don't do position or rotational changes there is nothing to do
        if (doPos == false && doOri == false)
        {
            return MCORE_INVALIDINDEX32;
        }

        // To process the chain of nodes we start at the end node and work our way back
        // to the start node. If the start node is the root of the actor then the root
        // is processed as well. The root will only be processed once, even if it is
        // used in multiple chains. For each node in the chain we check if the node has
        // already been processed (FindJoint). If the node has been processed then
        // FindJoint adds the end effector of this new chain to that joint. If not then
        // we create a new joint.
        bool tempBool = true; // tempbool always is true, just to work around some compiler warning
        while (tempBool)
        {
            Joint joint;
            joint.mNodeIndex = tempNode->GetNodeIndex();

            if (FindJoint(joint) == false)
            {
                // create a new joint and store the index to the joint state array and add the chain's end effector
                joint.mJointIndex = mJointState->GetLength();
                joint.mEndEffectors.Add(mEndEffectors.GetLength());

                attribute = (NodeLimitAttribute*)tempNode->GetAttributeByType(NodeLimitAttribute::TYPE_ID);
                if (attribute == nullptr || mConstrainJoints == false)
                {
                    // With no limit attribute the joint has 3 DOFs, all unconstrained.
                    joint.mMinLimits.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
                    joint.mMaxLimits.Set(FLT_MAX,  FLT_MAX,  FLT_MAX);

                    // add the space to the joint state array
                    for (uint32 i = 0; i < 3; ++i)
                    {
                        joint.mDOFs.Add(static_cast<uint16>(i));
                        mJointState->Add(0.0f);
                    }
                }
                else
                {
                    // The joint can have up to 3 DOFs, so we will loop through each and check the limits.
                    // If the limits are essentially the same for any of the DOFs then that DOF is completely
                    // restricted and will not be solved by the solver.
                    joint.mMaxLimits    = attribute->GetRotationMax();// store the joint limits
                    joint.mMinLimits    = attribute->GetRotationMin();

                    //joint.mMaxLimits.x    =  0.4f;    // TODO: why this hardcoded limit?
                    //joint.mMinLimits.x    = -0.4f;
                    //LogInfo("Min: %f, %f, %f", joint.mMinLimits.x, joint.mMinLimits.y, joint.mMinLimits.z);
                    //LogInfo("Max: %f, %f, %f", joint.mMaxLimits.x, joint.mMaxLimits.y, joint.mMaxLimits.z);
                    //LogInfo("");
                    for (uint32 i = 0; i < 3; ++i)
                    {
                        if (MCore::Math::Abs((float)((&joint.mMinLimits.x)[i]) - (float)((&joint.mMaxLimits.x)[i])) < 0.0001)
                        {
                            // no DOF
                            (&joint.mMinLimits.x)[i] = (&joint.mMaxLimits.x)[i] = ((&joint.mMinLimits.x)[i] + (&joint.mMaxLimits.x)[i]) * 0.5f;
                        }
                        else
                        {
                            // add space to the joint state array
                            mJointState->Add(((&joint.mMinLimits.x)[i] + (&joint.mMaxLimits.x)[i]) * 0.5f);
                            joint.mDOFs.Add(static_cast<uint16>(i));
                        }
                    }
                }

                // add the new joint to the joint array
                mJoints.Add(joint);
            }

            // traverse up the tree towards the start node.
            tempNode = mActorInstance->GetActor()->GetSkeleton()->GetNode(tempNode->GetParentIndex());
            if (tempNode->GetIsRootNode() && startNode->GetIsRootNode())
            {
                // we've hit the root node so we process it
                // there can't be any nodes above the root so after processing it we break out of the while loop
                ProcessRoot(tempNode);
                break;
            }
            else
            {
                if (tempNode->GetNodeIndex() == startNode->GetParentIndex())
                {
                    break; // we've hit the start node of the IK chain so we can break out of the while loop
                }
            }
        }

        // create the new end effector for this ik chain, initialize it with the correct data,
        // and then add it to the end effector array.
        EndEffector endEffector;
        endEffector.mConstrainJoints = mConstrainJoints;
        endEffector.mDoPos           = doPos;
        endEffector.mDoOri           = doOri;
        //endEffector.mPosOffset.x   = 10.0f;
        endEffector.mNodeIndex       = endNode->GetNodeIndex();
        endEffector.mArrayIndex      = (mEndEffectors.GetLength() >= 1) ? (uint32)mEndEffectors[mEndEffectors.GetLength() - 1].mDoPos * 3 + (uint32)mEndEffectors[mEndEffectors.GetLength() - 1].mDoOri * 3 + mEndEffectors[mEndEffectors.GetLength() - 1].mArrayIndex : 0;
        mEndEffectors.Add(endEffector);

        // add prerequisit nodes, ie. nodes that need to have their
        // global space matrices updated before the solver should start

        //  if (!tempNode->IsRootNode())
        if (mPrerequisitNodes.Contains(tempNode->GetNodeIndex()) == false) // make sure to not add the same node twice
        {
            mPrerequisitNodes.Add(tempNode->GetNodeIndex());
        }

        while (/*!tempNode->IsRootNode()*/ tempBool) // tempBool is always true, just to work around compiler warnings
        {
            if (tempNode->GetIsRootNode())
            {
                break; // we've hit the root so break
            }
            // traverse upwards towards the root adding all the nodes - they are the prerequisit nodes.
            tempNode = mActorInstance->GetActor()->GetSkeleton()->GetNode(tempNode->GetParentIndex());
            if (mPrerequisitNodes.Contains(tempNode->GetNodeIndex()) == false) // make sure to not add the same node twice
            {
                mPrerequisitNodes.Add(tempNode->GetNodeIndex());
            }
        }

        // return the chain index
        return mEndEffectors.GetLength() - 1;
    }


    // compare function to sort the joint array by mNodeIndex
    // TODO: make this some class member instead of a global one
    int32 MCORE_CDECL JointCmp(const JacobianIKSolver::Joint& itemA, const JacobianIKSolver::Joint& itemB)
    {
        if (itemA.mNodeIndex < itemB.mNodeIndex)
        {
            return -1;
        }
        else
        {
            if (itemA.mNodeIndex == itemB.mNodeIndex)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }


    // initialize the solver
    // this should be called after all the required chains have been processed
    // no chains should be added after this call
    void JacobianIKSolver::Initialize()
    {
        // sort the prerequisit node array and the joint array
        mPrerequisitNodes.Sort();
        mJoints.Sort(JointCmp);

        // if we have processed the root node
        if (mProcessRoot)
        {
            // finish seting up the "root joint"
            // set its index in the Joint State Array
            mRootJoint.mJointIndex = mJointState->GetLength();

            // loop through the rotational DOFs and add the appropriate space to the Joint State Array
            for (uint32 i = 0; i < MCore::Min<uint32>(3, mRootJoint.mDOFs.GetLength()); ++i)
            {
                mJointState->Add(((&mRootJoint.mMinLimits.x)[mRootJoint.mDOFs[i]] + (&mRootJoint.mMaxLimits.x)[mRootJoint.mDOFs[i]]) * 0.5f); // Add the space with some initial values.
            }
            // check for translational DOFs and add the space
            if (mRootJoint.mDOFs.GetLength() > 3)
            {
                for (uint32 i = 0; i < 3; ++i)
                {
                    mJointState->Add(0.0f);
                }
            }
        }

        // calculate the total number of 3D end effectors
        uint32 numEEs = 0;
        for (uint32 i = 0; i < mEndEffectors.GetLength(); ++i)
        {
            if (mEndEffectors[i].mDoPos)
            {
                numEEs++;
            }
            if (mEndEffectors[i].mDoOri)
            {
                numEEs++;
            }
        }

        // reserve memory for required matraces etc
        // no runtime allocs
        mJacobian.SetDimensions(numEEs * 3, mJointState->GetLength());
        mJacobianInv.SetDimensions(mJacobian.GetNumColumns(), mJacobian.GetNumRows());
        mNullSpaceProjector.SetDimensions(mJacobian.GetNumColumns(), mJacobian.GetNumColumns());
        mV->SetDimensions(mJacobian.GetNumColumns(), mJacobian.GetNumColumns());
        mSigma.Resize(mJacobian.GetNumColumns());
        mTempArray.Resize(MCore::Max(numEEs * 3, mJacobian.GetNumColumns()));
        mGradiantVector.Resize(mJacobian.GetNumColumns());
        mAI.Resize(numEEs * 3);
        mVI.Resize(mV->GetNumRows());
        mNewJointState->Resize(mJointState->GetLength());
    }


    // create the motion links, which identify what nodes are being modified
    void JacobianIKSolver::CreateMotionLinks(Actor* actor, MotionInstance* instance)
    {
        MCORE_UNUSED(actor);

        // enable the root
        if (mProcessRoot)
        {
            instance->GetMotionLink(mRootJoint.mNodeIndex)->SetSubMotionIndex(0);   // set it to 0, just to activate the link
        }
        // enable all joints
        const uint32 numJoints = mJoints.GetLength();
        for (uint32 i = 0; i < numJoints; ++i)
        {
            instance->GetMotionLink(mJoints[i].mNodeIndex)->SetSubMotionIndex(0);   // set to 0, just to activate it
        }
    }



    // solves the system defined in the Jacobian matrix etc
    void JacobianIKSolver::Solve()
    {
        // do Singular Value Decomposition on the Jacobian Matrix
        MCore::MemSet(mTempArray.GetPtr(), 0, sizeof(float) * mTempArray.GetLength()); // used within the SVD but we dont want any runtime allocs
        *mU = mJacobian;

        if (mJacobian.GetNumColumns() > mJacobian.GetNumRows())
        {
            mTempMatrix = *mU;
            mU->SetTransposed(mTempMatrix);
            //mU->Transpose();
            mU->SVD(mSigma, *mV, mTempArray);
            mTempArray.Resize(mJacobian.GetNumColumns()); // this call shouldn't actually do any reallocations at runtime because the array has already been this size

            // swap mU and mV (pointer swap)
            MCore::NMatrix* temp = mU;
            mU = mV;
            mV = temp;

            /*      (uint32&)mU ^= (uint32&)mV;
                    (uint32&)mV ^= (uint32&)mU;
                    (uint32&)mU ^= (uint32&)mV;*/
        }
        else
        {
            mU->SVD(mSigma, *mV, mTempArray);
        }

        // The following code does two things. It calculates the selectively
        // damped least squares inverse of the Jacobian from the SVD information
        // and it also calculates the null space projection matrix. There is a
        // lot of hidden maths for the SDLS algorithm and it has been relatively optimized.
        // There are a few matrix multiplications happening in the dark as well.
        // Explaining the maths is left to the paper

        // copy data from the end effectors across to the array for easy multiplication
        const uint32 numEndEffectors = mEndEffectors.GetLength();
        uint32 index = 0;
        for (uint32 i = 0; i < numEndEffectors; ++i)
        {
            const EndEffector& ee = mEndEffectors[i];

            if (ee.mDoPos)
            {
                mTempArray[index++] = ee.mPosDeriv.x;
                mTempArray[index++] = ee.mPosDeriv.y;
                mTempArray[index++] = ee.mPosDeriv.z;
            }

            if (ee.mDoOri)
            {
                mTempArray[index++] = ee.mOriDeriv.x;
                mTempArray[index++] = ee.mOriDeriv.y;
                mTempArray[index++] = ee.mOriDeriv.z;
            }
        }

        MCore::MemSet(mAI.GetPtr(), 0, sizeof(float) * mAI.GetLength());
        for (uint32 i = 0; i < mU->GetNumColumns(); ++i)
        {
            for (uint32 j = 0; j < mU->GetNumRows(); ++j)
            {
                mAI[i] += mU->Get(j, i) * mTempArray[j];
            }
        }

        MCore::MemSet(mTempArray.GetPtr(), 0, sizeof(float) * mTempArray.GetLength());
        for (uint32 i = 0; i < mSigma.GetLength(); ++i)
        {
            if (MCore::Math::IsFloatZero(mSigma[i]))
            {
                continue;
            }

            // The nullspace projector is J'J where J' is the pseudo inverse of J.
            // The problem is that you cant use the SDLS pseudo inverse for this calculation.
            // The good news is it can be shown that from the SVD of J
            // J = UDV'   J' = VD'U'    V' = V^t, U' = U^t
            // J'J = VD'U'UDV' = VV'
            if (mUseNSP)
            {
                for (uint32 j = 0; j < mNullSpaceProjector.GetNumColumns(); ++j)
                {
                    for (uint32 k = 0; k < mNullSpaceProjector.GetNumRows(); ++k)
                    {
                        mNullSpaceProjector.Set(k, j, mNullSpaceProjector.Get(k, j) - mV->Get(k, i) * mV->Get(j, i));
                    }
                }
            }

            float Ni = 0;
            float Mi = 0;
            for (uint32 k = 0; k < mJacobian.GetNumRows(); k += 3)
            {
                Ni += MCore::Vector3(mU->Get(k, i), mU->Get(k + 1, i), mU->Get(k + 2, i)).SafeLength();
                for (uint32 j = 0; j < mV->GetNumRows(); ++j)
                {
                    Mi += MCore::Math::Abs(mV->Get(j, i)) * MCore::Vector3(mJacobian.Get(k, j), mJacobian.Get(k + 1, j), mJacobian.Get(k + 2, j)).SafeLength();
                }
            }

            const float Ci = MCore::Math::IsFloatZero(Mi) ? 0.785f : (MCore::Min<float>(1.0f, mSigma[i] * Ni / Mi) * 0.785f);
            const float recSig = 1.0f / mSigma[i];
            float tmax = 0.0f;
            for (uint32 j = 0; j < mVI.GetLength(); ++j)
            {
                mVI[j] = mAI[i] * mV->Get(j, i) * recSig;
                if (tmax < MCore::Math::Abs(mVI[j]))
                {
                    tmax = MCore::Math::Abs(mVI[j]);
                }
            }

            if (tmax > Ci)
            {
                for (uint32 j = 0; j < mVI.GetLength(); ++j)
                {
                    mVI[j] = mVI[j] * Ci / tmax;
                }
            }

            for (uint32 j = 0; j < mJointState->GetLength(); ++j)
            {
                mTempArray[j] += mVI[j];
            }
        }

        float tmax = 0.0f;
        for (uint32 j = 0; j < mTempArray.GetLength(); ++j)
        {
            if (tmax > MCore::Math::Abs(mTempArray[j]))
            {
                tmax = MCore::Math::Abs(mTempArray[j]);
            }
        }

        if (tmax > 0.785f)
        {
            for (uint32 j = 0; j < mTempArray.GetLength(); ++j)
            {
                mTempArray[j] = mTempArray[j] * 0.785f / tmax;
            }
        }

        // all finished with the SDLS
        // now mTempArray is an array containing the correct change to make to the Joint State Array, so we add it...
        for (uint32 j = 0; j < mJointState->GetLength(); ++j)
        {
            (*mNewJointState)[j] = (*mJointState)[j] + mTempArray[j];
            if (mUseNSP)
            {
                for (uint32 k = 0; k < mNullSpaceProjector.GetNumRows(); ++k) // add in changes made by the null space projector
                {
                    (*mNewJointState)[j] += mNullSpaceProjector.Get(j, k) * mGradiantVector[k] * 0.01f;
                }
            }
        }
    }


    // checks for DOFs that have gone outside of their limits.
    // The values are clamped and action is taken to improve the situation in the next iteration.
    bool JacobianIKSolver::CheckJointLimits()
    {
        if (mConstrainJoints == false)
        {
            return false; // no joints are to be clamped if the solver isnt constraining the joints
        }
        bool clampedAnything = false;

        // loop over all joints and then over the degrees of freedom that the joint has
        for (uint32 i = 0; i < mJoints.GetLength(); ++i)
        {
            Joint& joint = mJoints[i];
            for (uint32 j = 0; j < joint.mDOFs.GetLength(); ++j)
            {
                // check if the changes made by the Jacobian are going to cause joint limit conflicts
                if ((*mNewJointState)[joint.mJointIndex + j] >= (&joint.mMaxLimits.x)[joint.mDOFs[j]] ||
                    (*mNewJointState)[joint.mJointIndex + j] <= (&joint.mMinLimits.x)[joint.mDOFs[j]])
                {
                    // if there is a joint limit violation then remove the joint from the jacobian matrix
                    // by zeroing out that joint's column in the jacobian and set clampedAntyhing to true.
                    // this will cause the loop that called this function to redo this iteration with the
                    // changes made to the jacobian.
                    clampedAnything = true;

                    //              (*mJointState)[joint.mJointIndex + j] += mGradiantVector[joint.mJointIndex + j] * 0.1f;
                    //              mNullSpaceProjector.Set(joint.mJointIndex + j, joint.mJointIndex + j, 0.0f);
                    for (uint32 s = 0; s < mJacobian.GetNumRows(); ++s)
                    {
                        mJacobian.Set(s, joint.mJointIndex + j, 0.0f);
                    }
                }
            }
        }

        return clampedAnything;
    }


    // updates the global positions of all the end effectors
    void JacobianIKSolver::UpdateEndEffectors(const Pose* inPose)
    {
        if (mProcessRoot)
        {
            // get the local transform of the root from the inPose.
            Transform ltf;
            inPose->GetLocalTransform(mRootJoint.mNodeIndex, &ltf);

            // update the transforms position if the root has enough DOFs to traverse
            if (mRootJoint.mDOFs.GetLength() > 3)
            {
                ltf.mPosition.x = (*mJointState)[mRootJoint.mJointIndex + 3] * 15.0f;
                ltf.mPosition.y = (*mJointState)[mRootJoint.mJointIndex + 4] * 15.0f;
                ltf.mPosition.z = (*mJointState)[mRootJoint.mJointIndex + 5] * 15.0f;
            }

            // get the rotational DOFs from the joint state array.
            // initialized to mMinLimits so any unused DOF has the correct value
            MCore::Vector3 jointAngles = mRootJoint.mMinLimits;
            for (uint32 k = 0; k < MCore::Min<uint32>(3, mRootJoint.mDOFs.GetLength()); ++k)
            {
                (&jointAngles.x)[mRootJoint.mDOFs[k]] = (*mJointState)[mRootJoint.mJointIndex + k];
            }

            // calculate a rotation quaternion from the joint angles
            MCore::Matrix mat;
            mat.SetRotationMatrixEulerXYZ(jointAngles);
            ltf.mRotation = MCore::Quaternion::ConvertFromMatrix(mat);

            // set the local tranform of the root and then get EMotion FX to calculate the LocalTM for us
            mTFD->GetCurrentPose()->SetLocalTransform(mRootJoint.mNodeIndex, ltf);
            mActorInstance->CalcLocalTM(mRootJoint.mNodeIndex, &mLocalMatrices[mRootJoint.mNodeIndex]);

            // now update the global matrix for the root
            mGlobalMatrices[mRootJoint.mNodeIndex] = mLocalMatrices[mRootJoint.mNodeIndex];
            mGlobalMatrices[mRootJoint.mNodeIndex].MultMatrix4x3(mActorInstance->GetGlobalTransform().ToMatrix());
        }

        // zero out the gradient vector
        for (uint32 i = 0; i < mGradiantVector.GetLength(); ++i)
        {
            mGradiantVector[i] = 0.0f;
        }

        // loop over all the joints and update their matrices in a similar way to the root
        const uint32 numJoints = mJoints.GetLength();
        for (uint32 i = 0; i < numJoints; ++i)
        {
            const Joint& joint = mJoints[i];
            const uint32 nodeIndex = joint.mNodeIndex;
            Node* node = mActorInstance->GetActor()->GetSkeleton()->GetNode(nodeIndex);

            // get the local transform of the joint from the inPose
            Transform ltf;
            inPose->GetLocalTransform(nodeIndex, &ltf);

            // get the rotational DOFs from the joint state array
            // initialized to mMinLimits so any unused DOF has the correct value
            MCore::Vector3 jointAngles = joint.mMinLimits;
            for (uint32 l = 0; l < joint.mDOFs.GetLength(); ++l)
            {
                mGradiantVector[joint.mJointIndex + l] = -0.5f * ((*mJointState)[joint.mJointIndex + l] - (joint.mMinLimits[joint.mDOFs[l]] + joint.mMaxLimits[joint.mDOFs[l]]) * 0.5f); // calculate the gradient for the joint
                (&jointAngles.x)[joint.mDOFs[l]] = (*mJointState)[joint.mJointIndex + l];
            }

            // calculate a rotation quaternion from the joint angles
            MCore::Matrix mat;
            mat.SetRotationMatrixEulerXYZ(jointAngles);
            ltf.mRotation = MCore::Quaternion::ConvertFromMatrix(mat);

            // set the local tranform of the root and then get EMotion FX to calculate the LocalTM for us
            mTFD->GetCurrentPose()->SetLocalTransform(nodeIndex, ltf);
            mActorInstance->CalcLocalTM(nodeIndex, &mLocalMatrices[nodeIndex]);

            // now update the global matrix for the joint
            const uint32 parentIndex = node->GetParentIndex();
            mGlobalMatrices[nodeIndex] = mLocalMatrices[nodeIndex];
            mGlobalMatrices[nodeIndex].MultMatrix4x3(mGlobalMatrices[parentIndex]);
        }

        // now that all the global matrices for the relevant nodes have been updated,
        // we can update the information in the end effectors
        const uint32 numEndEffectors = mEndEffectors.GetLength();
        for (uint32 i = 0; i < numEndEffectors; ++i)
        {
            EndEffector& endEffector = mEndEffectors[i];
            const uint32 nodeIndex = endEffector.mNodeIndex;

            // add the translation offset of the node
            MCore::Matrix mat;
            mat.Identity();
            mat.SetTranslation(endEffector.mPosOffset);
            mat.MultMatrix4x3(mGlobalMatrices[nodeIndex]);

            // store the position of the end effector and the derivative to the goal
            mEndEffectors[i].mPos = mat.GetTranslation();
            mEndEffectors[i].mPosDeriv = endEffector.mGoalPos - endEffector.mPos;

            // make sure that derivative is not too long
            float l = endEffector.mPosDeriv.SafeLength();
            if (l > mMaxGoalDist)
            {
                endEffector.mPosDeriv = endEffector.mPosDeriv * (mMaxGoalDist / l);
            }

            // make a rotation matrix from the goal orientation and invert it (transpose)
            // then multiply with the end effector's rotation matrix
            // this will give a matrix containing the rotation difference
            MCore::Matrix goalOri;
            goalOri.SetRotationMatrixEulerXYZ(endEffector.mGoalOri);
            goalOri.Transpose();
            mat.MultMatrix3x3(goalOri);

            // decompose it into euler angles, now we have the derivatives for the orientation
            endEffector.mOriDeriv = -mat.CalcEulerAngles();

            // also make sure that the derivatives aren't too big
            l = MCore::Max<float>(endEffector.mOriDeriv.x, MCore::Max<float>(endEffector.mOriDeriv.y, endEffector.mOriDeriv.z));
            if (l > 0.785f)
            {
                endEffector.mOriDeriv *= (0.785f / l);
            }

            MCore::Vector3 axis;
            float   angle;
            MCore::Quaternion::ConvertFromMatrix(mat).ToAxisAngle(&axis, &angle);
            if (angle > ((2.0f * 3.141592653589f) / 180.0f))
            {
                angle = ((2.0f * 3.141592653589f) / 180.0f);
            }

            endEffector.mOriDeriv = axis * angle;
        }
    }


    // update the solver (main method)
    void JacobianIKSolver::Update(const Pose* inPose, Pose* outPose, MotionInstance* instance)
    {
        MCORE_UNUSED(instance);

        // update the actor instance's global TM
        //mActorInstance->UpdateLocalTM();
        //Matrix mat = mActorInstance->GetLocalTM();
        //mat.MultMatrix4x3( mActorInstance->GetAttachmentParentTM() );
        //mActorInstance->SetGlobalTM( mat );
        MCore::Matrix globalMat = mActorInstance->GetGlobalTransform().ToMatrix();

        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();

        // update the globalTMs of all the prerequisit nodes
        const uint32 numNodes = mPrerequisitNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 nodeIndex = mPrerequisitNodes[i];
            const Node* node = skeleton->GetNode(nodeIndex);

            // set the transform according to the input pose and get EMotion FX to calculate the localTM for us
            mTFD->GetCurrentPose()->SetLocalTransform(nodeIndex, inPose->GetLocalTransform(nodeIndex));
            mActorInstance->CalcLocalTM(nodeIndex, &mLocalMatrices[nodeIndex]);

            // update the globalTMs
            if (node->GetIsRootNode())
            {
                mGlobalMatrices[nodeIndex] = mLocalMatrices[nodeIndex];
                mGlobalMatrices[nodeIndex].MultMatrix4x3(globalMat);
            }
            else
            {
                const uint32 parentIndex = node->GetParentIndex();
                mGlobalMatrices[nodeIndex] = mLocalMatrices[nodeIndex];
                mGlobalMatrices[nodeIndex].MultMatrix4x3(mGlobalMatrices[parentIndex]);
            }
        }

        // init for this update
        uint32 curIteration = 0;
        UpdateEndEffectors(inPose);

        // loop while we are far from the goal and are not exceeding the maximum number of iterations
        while (curIteration < mMaxIterations)
        {
            // solve for this iteration and check joint limits
            BuildJacobianMatrix(); // build the jacobian matrix
            //      mNullSpaceProjector.Identity();

            while (curIteration < mMaxIterations)
            {
                mNullSpaceProjector.Identity();
                Solve();
                curIteration++;
                if (!CheckJointLimits())
                {
                    /*              for (uint32 j = 0; j < mJointState->GetLength(); j++)
                                    {
                                        (*mJointState)[j] += mTempArray[j];
                    //                  for (uint32 k = 0; k < mNullSpaceProjector.GetNumRows(); k++)
                    //                      (*mJointState)[j] += mNullSpaceProjector.Get(j, k) * mGradiantVector[k] * 0.1f;
                                    }*/

                    // pointer swap
                    MCore::Array<float>* temp = mJointState;
                    mJointState     = mNewJointState;
                    mNewJointState  = temp;
                    /*
                                    (uint32&)mJointState    ^= (uint32&)mNewJointState;
                                    (uint32&)mNewJointState ^= (uint32&)mJointState;
                                    (uint32&)mJointState    ^= (uint32&)mNewJointState;
                    */
                    // update the end effectors and break out of the loop because there are no joint violations
                    UpdateEndEffectors(inPose);
                    break;
                }

                if (mUseNSP)
                {
                    UpdateEndEffectors(inPose);
                }
            }

            mHasSolution = true;
            const uint32 numEndEffectors = mEndEffectors.GetLength();

            // check the distance for all end effectors.
            for (uint32 i = 0; i < numEndEffectors; ++i)
            {
                if (mDistThreshold < (float)mEndEffectors[i].mDoOri * mEndEffectors[i].mOriDeriv.SquareLength() ||
                    mDistThreshold < (float)mEndEffectors[i].mDoPos * mEndEffectors[i].mPosDeriv.SquareLength())
                {
                    mHasSolution = false; // we dont have a proper solution
                }
            }

            if (mHasSolution)
            {
                break;
            }
        }

        // copy the joint state array data to the actor instance
        CopyStateToActor(inPose, outPose);
    }


    // updates all the affected joints in the Actor Instance from the Joint State array
    void JacobianIKSolver::CopyStateToActor(const Pose* inPose, Pose* outPose)
    {
        // make sure the pose has enough space transforms
        // TODO: temp?
        //MCORE_ASSERT(outPose->GetNumTransforms() == instance->GetActorInstance()->GetNumNodes());

        if (mProcessRoot)
        {
            Transform ltf = inPose->GetLocalTransform(mRootJoint.mNodeIndex);

            // update the transforms position if the root has anough DOFs to traverse
            if (mRootJoint.mDOFs.GetLength() > 3)
            {
                ltf.mPosition.x = (*mJointState)[mRootJoint.mJointIndex + 3] * 15.0f;
                ltf.mPosition.y = (*mJointState)[mRootJoint.mJointIndex + 4] * 15.0f;
                ltf.mPosition.z = (*mJointState)[mRootJoint.mJointIndex + 5] * 15.0f;
            }

            // get the rotational DOFs from the joint state array. Initialised to
            // mMinLimits so any unused DOF has the correct value
            MCore::Vector3 jointAngles = mRootJoint.mMinLimits;
            for (uint32 k = 0; k < MCore::Min<uint32>(3, mRootJoint.mDOFs.GetLength()); ++k)
            {
                (&jointAngles.x)[mRootJoint.mDOFs[k]] = (*mJointState)[mRootJoint.mJointIndex + k];
            }

            // calculate a rotation quaternion and copy the data from lft into outPose
            MCore::Matrix mat;
            mat.SetRotationMatrixEulerXYZ(jointAngles);

            Transform rootTransform;
            rootTransform.mRotation = MCore::Quaternion::ConvertFromMatrix(mat);
            rootTransform.mPosition = ltf.mPosition;

            EMFX_SCALECODE
            (
                rootTransform.mScale            = ltf.mScale;
                //rootTransform.mScaleRotation  = ltf.mScaleRotation;
            )
            outPose->SetLocalTransform(mRootJoint.mNodeIndex, rootTransform);
        }

        // loop over all the joints and copy the data from the joint state array to the outPose
        Transform jointTransform;
        const uint32 numJoints = mJoints.GetLength();
        for (uint32 i = 0; i < numJoints; ++i)
        {
            const Joint& joint = mJoints[i];

            // get the rotational DOFs from the joint state array
            // initialised to mMinLimits so any unused DOF has the correct value
            MCore::Vector3 jointAngles = joint.mMinLimits;
            for (uint32 k = 0; k < joint.mDOFs.GetLength(); ++k)
            {
                (&jointAngles.x)[joint.mDOFs[k]] = (*mJointState)[joint.mJointIndex + k];
            }

            // Calculate a rotation quaternon and copy the data from lft into outPose
            MCore::Matrix mat;
            mat.SetRotationMatrixEulerXYZ(jointAngles);
            const Transform& inTransform    = inPose->GetLocalTransform(joint.mNodeIndex);

            jointTransform.mRotation        = MCore::Quaternion::ConvertFromMatrix(mat);
            jointTransform.mPosition        = inTransform.mPosition;

            EMFX_SCALECODE
            (
                //jointTransform.mScaleRotation = inTransform.mScaleRotation;
                jointTransform.mScale           = inTransform.mScale;
            )

            outPose->SetLocalTransform(joint.mNodeIndex, jointTransform);
        }
    }


    // builds the Jacobian Matrix from all the information about the joints/DOFs
    void JacobianIKSolver::BuildJacobianMatrix()
    {
        // zero out the Jacobian
        MCore::MemSet(mJacobian.GetData(), 0, sizeof(float) * mJacobian.GetNumColumns() * mJacobian.GetNumRows());

        MCore::Matrix actorInstanceGlobalTM = mActorInstance->GetGlobalTransform().ToMatrix();

        Skeleton* skeleton = mActorInstance->GetActor()->GetSkeleton();

        // loop over all the joints
        const uint32 numJoints = mJoints.GetLength();
        for (uint32 i = 0; i < numJoints; ++i)
        {
            const uint32            nodeIndex       = mJoints[i].mNodeIndex;
            const Node*             node            = skeleton->GetNode(nodeIndex);
            const Transform&        localTransform  = mTFD->GetCurrentPose()->GetLocalTransform(nodeIndex);

            // set all joint to the 'free' state
            mJoints[i].mJointState = 0;

            // get the position of the joint
            MCore::Vector3 jointPos = mGlobalMatrices[nodeIndex].GetTranslation();

            // we need to take scale and other such factors into account
            // in the same way that EMotion FX does when it updates its nodes
            // this code is very similar to the EMotion FX code, it just leaves
            // globalTM in a position to be multiplied by the rotation  matrix of this joint
            MCore::Matrix globalTM;
            globalTM.Identity();
        #ifdef EMFX_SCALE_DISABLED
            globalTM.SetTranslation(localTransform.mPosition);
        #else
            // check if it has scale, and calculate it accordingly
            const TransformData::ENodeFlags flags = mTFD->GetNodeFlags(nodeIndex);
            if (flags & TransformData::FLAG_HASSCALE)
            {
                globalTM.InitFromPosRotScale(localTransform.mPosition, MCore::Quaternion(), localTransform.mScale);
            }
            else
            {
                globalTM.SetTranslation(localTransform.mPosition);
            }
        #endif

            // apply globalTM to the globalTM of the parent
            globalTM.MultMatrix4x3(mGlobalMatrices[node->GetParentIndex()]);

            // now we apply the X, Y and Z rotations respectively
            // each time we use the orientational information in the globalTM to find
            // an axis of rotation for the concerned DOF
            // this axis is used to calculate the derivatives in the Jacobian matrix
            MCore::Matrix rotMat;
            uint32 colIndex = mJoints[i].mDOFs.GetLength() + mJoints[i].mJointIndex;
            for (int32 k = 2; k >= 0; --k)
            {
                if ((&mJoints[i].mMinLimits.x)[k] == (&mJoints[i].mMaxLimits.x)[k])
                {
                    switch (k)
                    { // no DOF, use defaults
                    case 0:
                        rotMat.SetRotationMatrixX((mJoints[i].mMinLimits.x + mJoints[i].mMaxLimits.x) * 0.5f);
                        break;
                    case 1:
                        rotMat.SetRotationMatrixY((mJoints[i].mMinLimits.y + mJoints[i].mMaxLimits.y) * 0.5f);
                        break;
                    case 2:
                        rotMat.SetRotationMatrixZ((mJoints[i].mMinLimits.z + mJoints[i].mMaxLimits.z) * 0.5f);
                        break;
                    default:
                        ;
                    }
                    rotMat.MultMatrix4x3(globalTM); // apply rotation
                    globalTM = rotMat;
                }
                else
                {
                    colIndex--;

                    // get the axis of rotation from the globalTM
                    // this is also the orientational derivative
                    MCore::Vector3 axis = (*(MCore::Vector3*)(&globalTM.m16[4 * k])).Normalized();

                    // now we loop over the end efectors that influence this joint
                    for (uint32 l = 0; l < mJoints[i].mEndEffectors.GetLength(); ++l)
                    {
                        uint32 index = mEndEffectors[mJoints[i].mEndEffectors[l]].mArrayIndex;

                        // Store the derivitives in the appropriate place in the jacobian matrix
                        if (mEndEffectors[mJoints[i].mEndEffectors[l]].mDoPos)
                        {
                            // Calculate a vector from the joint to the end effector. The positional
                            // derivitive for this joint and end effector is this axis crossed with
                            // the axis of rotation.
                            MCore::Vector3 eeVector = (mEndEffectors[mJoints[i].mEndEffectors[l]].mPos - jointPos);
                            MCore::Vector3 posDer = axis.Cross(eeVector);

                            // store the derivitives.
                            mJacobian.Set(index++, colIndex, posDer.x);
                            mJacobian.Set(index++, colIndex, posDer.y);
                            mJacobian.Set(index++, colIndex, posDer.z);
                        }

                        if (mEndEffectors[mJoints[i].mEndEffectors[l]].mDoOri)
                        {
                            mJacobian.Set(index++, colIndex, axis.x);
                            mJacobian.Set(index++, colIndex, axis.y);
                            mJacobian.Set(index++, colIndex, axis.z);
                        }
                    }

                    switch (k)
                    {
                    // calculate rotation matrix from the joint state array
                    case 0:
                        rotMat.SetRotationMatrixX((*mJointState)[colIndex]);
                        break;
                    case 1:
                        rotMat.SetRotationMatrixY((*mJointState)[colIndex]);
                        break;
                    case 2:
                        rotMat.SetRotationMatrixZ((*mJointState)[colIndex]);
                        break;
                    default:
                        ;
                    }

                    rotMat.MultMatrix4x3(globalTM); // apply the rotation for this DOF
                    globalTM = rotMat;
                }
            }
        }


        if (mProcessRoot)
        {
            // get the position of the root
            MCore::Vector3 jointPos = mGlobalMatrices[mRootJoint.mNodeIndex].GetTranslation();

            MCore::Matrix globalTM;
            globalTM.Identity();
            const Transform& localTransform = mTFD->GetCurrentPose()->GetLocalTransform(mRootJoint.mNodeIndex);


        #ifdef EMFX_SCALE_DISABLED
            globalTM.SetTranslation(localTransform.mPosition);
        #else
            const TransformData::ENodeFlags flags = mTFD->GetNodeFlags(mRootJoint.mNodeIndex);
            if (flags & TransformData::FLAG_HASSCALE)
            {
                globalTM.InitFromPosRotScale(localTransform.mPosition, MCore::Quaternion(), localTransform.mScale);
            }
            else
            {
                globalTM.SetTranslation(localTransform.mPosition);
            }
        #endif

            // apply the matrix to the worlTM of the actor instance
            globalTM.MultMatrix4x3(actorInstanceGlobalTM);

            // Translational DOFs for the root.
            if (mRootJoint.mDOFs.GetLength() > 3)
            {
                uint32 colIndex = mRootJoint.mJointIndex + 3;

                // loop over all end effectors that influence the root and then loop over the DOFs.
                for (uint32 l = 0; l < mRootJoint.mEndEffectors.GetLength(); ++l)
                {
                    for (uint32 k = 0; k < 3; ++k)
                    {
                        // the axis of translation is the respective row of the globalTM
                        MCore::Vector3 axis = (*(MCore::Vector3*)(&globalTM.m16[4 * k]));

                        uint32 index = mEndEffectors[mRootJoint.mEndEffectors[l]].mArrayIndex;

                        // Store the derivitives in the appropriate place in the jacobian matrix
                        if (mEndEffectors[mRootJoint.mEndEffectors[l]].mDoPos)
                        {
                            mJacobian.Set(index++, colIndex + k, axis.x);
                            mJacobian.Set(index++, colIndex + k, axis.y);
                            mJacobian.Set(index++, colIndex + k, axis.z);
                        }

                        if (mEndEffectors[mRootJoint.mEndEffectors[l]].mDoOri)
                        {
                            mJacobian.Set(index++, colIndex + k, 0.0f); // translation has no affect on orientation.
                            mJacobian.Set(index++, colIndex + k, 0.0f);
                            mJacobian.Set(index++, colIndex + k, 0.0f);
                        }
                    }
                }
            }

            // rotational DOFs
            MCore::Matrix rotMat;
            uint32 colIndex = mRootJoint.mJointIndex + MCore::Min<uint32>(mRootJoint.mDOFs.GetLength(), 3);
            for (int32 k = 2; k >= 0; --k)
            {
                if ((&mRootJoint.mMinLimits.x)[k] == (&mRootJoint.mMaxLimits.x)[k])
                {
                    // no DOF, use defaults
                    switch (k)
                    {
                    case 0:
                        rotMat.SetRotationMatrixX((mRootJoint.mMinLimits.x + mRootJoint.mMaxLimits.x) * 0.5f);
                        break;
                    case 1:
                        rotMat.SetRotationMatrixY((mRootJoint.mMinLimits.y + mRootJoint.mMaxLimits.y) * 0.5f);
                        break;
                    case 2:
                        rotMat.SetRotationMatrixZ((mRootJoint.mMinLimits.z + mRootJoint.mMaxLimits.z) * 0.5f);
                        break;
                    }

                    rotMat.MultMatrix4x3(globalTM); // apply rotation
                    globalTM = rotMat;
                }
                else
                {
                    colIndex--;

                    // get the axis of rotation from the globalTM
                    MCore::Vector3 axis = (*(MCore::Vector3*)(&globalTM.m16[4 * k])).Normalized();

                    // now loop over all the end effectors that influence the root
                    for (uint32 l = 0; l < mRootJoint.mEndEffectors.GetLength(); ++l)
                    {
                        uint32 index = mEndEffectors[mRootJoint.mEndEffectors[l]].mArrayIndex;

                        // store the derivitives in the appropriate place in the Jacobian matrix
                        if (mEndEffectors[mRootJoint.mEndEffectors[l]].mDoPos)
                        {
                            // calculate a vector from the joint to the end effector
                            // the positional derivitive for this joint and end effector is this axis crossed with
                            // the axis of rotation
                            MCore::Vector3 eeVector = mEndEffectors[mRootJoint.mEndEffectors[l]].mPos - jointPos;
                            MCore::Vector3 posDer = axis.Cross(eeVector);

                            // Store the derivatives
                            mJacobian.Set(index++, colIndex, posDer.x);
                            mJacobian.Set(index++, colIndex, posDer.y);
                            mJacobian.Set(index++, colIndex, posDer.z);
                        }

                        if (mEndEffectors[mRootJoint.mEndEffectors[l]].mDoOri)
                        {
                            mJacobian.Set(index++, colIndex, axis.x);
                            mJacobian.Set(index++, colIndex, axis.y);
                            mJacobian.Set(index++, colIndex, axis.z);
                        }
                    }

                    // calculate a toration matrix from the joint state array.
                    switch (k)
                    {
                    case 0:
                        rotMat.SetRotationMatrixX((*mJointState)[colIndex]);
                        break;
                    case 1:
                        rotMat.SetRotationMatrixY((*mJointState)[colIndex]);
                        break;
                    case 2:
                        rotMat.SetRotationMatrixZ((*mJointState)[colIndex]);
                        break;
                    default:
                        ;
                    }

                    rotMat.MultMatrix4x3(globalTM); // apply the rotation
                    globalTM = rotMat;
                }
            }
        }
    }


    // clone the controller
    LocalSpaceController* JacobianIKSolver::Clone(ActorInstance* targetActorInstance, bool neverActivate)
    {
        // create the clone
        JacobianIKSolver* clone = new JacobianIKSolver(targetActorInstance);

        // copy the Jacobian solver settings
        clone->mJacobian            = mJacobian;
        clone->mJacobianInv         = mJacobianInv;
        clone->mNullSpaceProjector  = mNullSpaceProjector;
        clone->mEndEffectors        = mEndEffectors;
        clone->mPrerequisitNodes    = mPrerequisitNodes;
        clone->mGradiantVector      = mGradiantVector;
        clone->mSigma               = mSigma;
        clone->mTempArray           = mTempArray;
        clone->mAI                  = mAI;
        clone->mVI                  = mVI;
        clone->mJoints              = mJoints;
        clone->mRootJoint           = mRootJoint;
        clone->mMaxGoalDist         = mMaxGoalDist;
        clone->mDistThreshold       = mDistThreshold;
        clone->mMaxIterations       = mMaxIterations;
        clone->mHasSolution         = mHasSolution;
        clone->mConstrainJoints     = mConstrainJoints;
        clone->mRootCanTranslate    = mRootCanTranslate;
        clone->mUseNSP              = mUseNSP;
        clone->mProcessRoot         = mProcessRoot;
        *(clone->mU)                = *mU;
        *(clone->mV)                = *mV;
        *(clone->mJointState)       = *mJointState;
        *(clone->mNewJointState)    = *mNewJointState;

        // copy the LocalSpaceController base class settings
        clone->CopyBaseControllerSettings(this, neverActivate);

        return clone;
    }
} // namespace EMotionFX
