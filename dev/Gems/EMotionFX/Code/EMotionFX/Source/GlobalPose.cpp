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
#include "GlobalPose.h"
#include "ActorInstance.h"
#include "Attachment.h"
#include "TransformData.h"
#include "Node.h"


namespace EMotionFX
{
    // constructor
    GlobalPose::GlobalPose()
        : BaseObject()
    {
        mGlobalMatrices.SetMemoryCategory(EMFX_MEMCATEGORY_GLOBALPOSES);
        mScales.SetMemoryCategory(EMFX_MEMCATEGORY_GLOBALPOSES);
    }


    // extended constructor
    GlobalPose::GlobalPose(uint32 numTransforms)
        : BaseObject()
    {
        mGlobalMatrices.SetMemoryCategory(EMFX_MEMCATEGORY_GLOBALPOSES);
        mScales.SetMemoryCategory(EMFX_MEMCATEGORY_GLOBALPOSES);

        SetNumTransforms(numTransforms);
    }


    // destructor
    GlobalPose::~GlobalPose()
    {
        Release();
    }


    // create
    GlobalPose* GlobalPose::Create()
    {
        return new GlobalPose();
    }


    // extended create
    GlobalPose* GlobalPose::Create(uint32 numTransforms)
    {
        return new GlobalPose(numTransforms);
    }


    // release the allocated memory
    void GlobalPose::Release()
    {
        mGlobalMatrices.Clear();
        mScales.Clear();
    }


    // set the number of transforms
    void GlobalPose::SetNumTransforms(uint32 numTransforms)
    {
        if (numTransforms)
        {
            mGlobalMatrices.Resize(numTransforms);
            mScales.Resize(numTransforms);
        }
        else
        {
            Release();
        }
    }


    // copy the pose data to another pose
    void GlobalPose::CopyTo(GlobalPose* destPose)
    {
        destPose->mGlobalMatrices   = mGlobalMatrices;
        destPose->mScales           = mScales;
    }


    // init from the actor instance
    void GlobalPose::InitFromActorInstance(ActorInstance* actorInstance)
    {
        // make sure we have enough transforms
        const uint32 numTransforms = actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
        if (numTransforms != GetNumTransforms())
        {
            SetNumTransforms(numTransforms);
        }

        // get the array of globalspace matrices
        MCore::Matrix* globalMatrices = actorInstance->GetTransformData()->GetGlobalInclusiveMatrices();

        // copy over the matrices
        //for (uint32 i=0; i<numTransforms; ++i)
        //mGlobalMatrices[i] = globalMatrices[i];

        // only copy over the matrices for the enabled nodes
        uint16 nodeNr;
        const uint32 numEnabledNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numEnabledNodes; ++i)
        {
            nodeNr = actorInstance->GetEnabledNode(i);
            mGlobalMatrices[nodeNr] = globalMatrices[nodeNr];
        }
    }


    // blend into the given dest pose, with a weight betwen 0 and 1
    void GlobalPose::BlendAndUpdateActorMatrices(ActorInstance* actorInstance, float weight, const MCore::Array<bool>* nodeMask)
    {
        // make sure the numbers of transforms are equal
        MCORE_ASSERT(weight >= 0.0f && weight <= 1.0f); // make sure the weight is in range

        // when we just want to use the source (current) pose, return as there is nothing to do
        if (weight <= 0.001f)
        {
            return;
        }

        // some data we need
        TransformData*  transformData           = actorInstance->GetTransformData();
        Skeleton*       skeleton                = actorInstance->GetActor()->GetSkeleton();
        MCore::Matrix*  actorGlobalMatrices     = transformData->GetGlobalInclusiveMatrices();
        MCore::Matrix*  actorLocalMatrices      = transformData->GetLocalMatrices();
        bool    needUpdateGlobalMatrices = false;

        // only copy over the matrices for the enabled nodes
        uint32 nodeNr;
        Transform localTransform;
        const uint32 numEnabledNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numEnabledNodes; ++i)
        {
            nodeNr = actorInstance->GetEnabledNode(i);

            // check if this transform has to be processed or not
            if (nodeMask->GetItem(nodeNr) == false)
            {
                continue;
            }

            // if the weight isn't "fully active", we need to blend
            if (weight < 0.999f)
            {
                // first decompose the source matrix in local space
                MCore::Matrix sourceMatrix = actorGlobalMatrices[nodeNr];
                uint32 parentIndex = skeleton->GetNode(nodeNr)->GetParentIndex();
                if (parentIndex != MCORE_INVALIDINDEX32)
                {
                    sourceMatrix = sourceMatrix * actorGlobalMatrices[parentIndex].Inversed();
                }

                // decompose the source matrix
                MCore::Vector3 sourcePos;
                MCore::Vector3 sourceScale;
                MCore::Quaternion sourceRot;
                sourceMatrix.DecomposeQRGramSchmidt(sourcePos, sourceRot, sourceScale);

                // then decompose the destination matrix in local space
                MCore::Matrix destMatrix = mGlobalMatrices[nodeNr];
                if (parentIndex != MCORE_INVALIDINDEX32)
                {
                    destMatrix = destMatrix * mGlobalMatrices[parentIndex].Inversed();
                }

                // now perform blending towards the dest pose
                // decompose the dest matrix
                MCore::Vector3 destPos;
                MCore::Vector3 destScale;
                MCore::Quaternion destRot;
                destMatrix.DecomposeQRGramSchmidt(destPos, destRot, destScale);

                const MCore::Vector3    finalPos        = MCore::LinearInterpolate<MCore::Vector3>(sourcePos,   destPos,   weight);
                const MCore::Quaternion finalRot        = sourceRot.NLerp(destRot, weight);
                const MCore::Vector3    finalScale      = MCore::LinearInterpolate<MCore::Vector3>(sourceScale, destScale, weight);

                // update the local transform (required to prevent a fade out issue when using CLONE_ALL)
                localTransform.Set(finalPos, finalRot, finalScale);
                transformData->GetCurrentPose()->SetLocalTransform(nodeNr, localTransform);

                // update the local matrix
                actorInstance->CalcLocalTM(nodeNr, finalPos, finalRot, finalScale, &actorLocalMatrices[nodeNr]);

                // mark that we need to update the global space matrices of the actor
                needUpdateGlobalMatrices = true;
            }
            else
            {
                actorGlobalMatrices[nodeNr] = mGlobalMatrices[nodeNr];
            }
        }

        // update the global space matrices when needed (using forward kinematics)
        if (needUpdateGlobalMatrices)
        {
            if (actorInstance->GetIsAttachment() && actorInstance->GetSelfAttachment()->GetIsInfluencedByMultipleNodes())
            {
                actorInstance->UpdateGlobalMatricesForNonRoots();
            }
            else
            {
                actorInstance->UpdateGlobalMatrices();
            }
        }
    }
}   // namespace EMotionFX
