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
#include "TransformData.h"
#include "ActorInstance.h"
#include "Actor.h"
#include <MCore/Source/Compare.h>
#include "Node.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformData, TransformDataAllocator, 0)

    // default constructor
    TransformData::TransformData()
        : BaseObject()
    {
        mGlobalMatrices     = nullptr;
        mLocalMatrices      = nullptr;
        mBindPose           = nullptr;
        mFlags              = nullptr;
        mNumTransforms      = 0;
        mHasUniqueBindPose  = false;
    }


    // destructor
    TransformData::~TransformData()
    {
        Release();
    }


    // create
    TransformData* TransformData::Create()
    {
        return aznew TransformData();
    }


    // get rid of all allocated data
    void TransformData::Release()
    {
        MCore::AlignedFree(mGlobalMatrices);
        MCore::AlignedFree(mLocalMatrices);
        MCore::AlignedFree(mFlags);

        if (mHasUniqueBindPose)
        {
            delete mBindPose;
        }

        mPose.Clear();
        mFlags          = nullptr;
        mGlobalMatrices = nullptr;
        mLocalMatrices  = nullptr;
        mBindPose       = nullptr;
        mNumTransforms  = 0;
    }


    // initialize from a given actor
    void TransformData::InitForActorInstance(ActorInstance* actorInstance)
    {
        Release();

        // link to the given actor instance
        mPose.LinkToActorInstance(actorInstance);

        // release all memory if we want to resize to zero nodes
        const uint32 numNodes = actorInstance->GetNumNodes();
        if (numNodes == 0)
        {
            Release();
            return;
        }

        mGlobalMatrices         = (MCore::Matrix*)MCore::AlignedAllocate(sizeof(MCore::Matrix) * numNodes, 16, EMFX_MEMCATEGORY_TRANSFORMDATA);
        mLocalMatrices          = (MCore::Matrix*)MCore::AlignedAllocate(sizeof(MCore::Matrix) * numNodes, 16, EMFX_MEMCATEGORY_TRANSFORMDATA);
        mFlags                  = (ENodeFlags*)MCore::AlignedAllocate(sizeof(ENodeFlags) * numNodes, 16, EMFX_MEMCATEGORY_TRANSFORMDATA);
        mNumTransforms          = numNodes;

        if (mHasUniqueBindPose)
        {
            mBindPose = new Pose();
            mBindPose->LinkToActorInstance(actorInstance);
        }
        else
        {
            mBindPose = actorInstance->GetActor()->GetBindPose();
        }

        // now initialize the data with the actor transforms
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mLocalMatrices[i].Identity();       // only reset when cloned?
            mGlobalMatrices[i].Identity();

            // use all flags
            uint32 flag = 0;
            flag |= FLAG_HASSCALE;
            //flag |= FLAG_NEEDLOCALTMUPDATE;
            //flag |= FLAG_HASSCALEROTATION;
            mFlags[i] = (ENodeFlags)flag;
        }

        // reset the flags
        //ResetFlags();
    }


    // make the bind pose transforms unique
    void TransformData::MakeBindPoseTransformsUnique()
    {
        if (mHasUniqueBindPose)
        {
            return;
        }

        ActorInstance* actorInstance = mPose.GetActorInstance();
        mHasUniqueBindPose = true;
        mBindPose = new Pose();
        mBindPose->LinkToActorInstance(actorInstance);
        *mBindPose = *actorInstance->GetActor()->GetBindPose();
    }


    EMFX_SCALECODE
    (
        // set the scaling value for the node and all child nodes
        void TransformData::SetBindPoseLocalScaleInherit(uint32 nodeIndex, const AZ::Vector3 & scale)
        {
            ActorInstance*  actorInstance   = mPose.GetActorInstance();
            Actor*          actor           = actorInstance->GetActor();

            // get the node index and the number of children of the given node
            const Node* node        = actor->GetSkeleton()->GetNode(nodeIndex);
            const uint32 numChilds  = node->GetNumChildNodes();

            // set the new scale for the given node
            SetBindPoseLocalScale(nodeIndex, scale);

            // iterate through the children and set their scale recursively
            for (uint32 i = 0; i < numChilds; ++i)
            {
                SetBindPoseLocalScaleInherit(node->GetChildIndex(i), scale);
            }
        }

        // update the local space scale
        void TransformData::SetBindPoseLocalScale(uint32 nodeIndex, const AZ::Vector3 & scale)
        {
            Transform newTransform = mBindPose->GetLocalTransform(nodeIndex);
            newTransform.mScale = scale;
            mBindPose->SetLocalTransform(nodeIndex, newTransform);
        }
    ) // EMFX_SCALECODE



    // update the flags
    void TransformData::UpdateNodeFlags()
    {
    #if !defined(EMFX_SCALE_DISABLED)
        ActorInstance* actorInstance = mPose.GetActorInstance();
        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 nodeIndex = actorInstance->GetEnabledNode(i);
            const Transform& transform = mPose.GetLocalTransform(nodeIndex);

            /*      #if defined(MCORE_SSE_ENABLED)
                        __m128 oneVec   = _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f);
                        __m128 scaleVec = _mm_set_ps(transform.mScale.x, transform.mScale.y, transform.mScale.z, 1.0f);
                        __m128 result   = _mm_cmpneq_ps(scaleVec, oneVec);
                        int mask        = _mm_movemask_ps(result);
                        if (mask != 0)
                            SetFlag( nodeIndex, FLAG_HASSCALE, true );
                        //if (mask != 0)
                            //MCore::LogInfo("Mask is %d (0x%x) for scale (%f, %f, %f)", mask, mask, transform.mScale.x, transform.mScale.y, transform.mScale.z);
                    #else*/
            if (!MCore::Compare<float>::CheckIfIsClose(transform.mScale.GetX(), 1.0f, MCore::Math::epsilon) ||
                !MCore::Compare<float>::CheckIfIsClose(transform.mScale.GetY(), 1.0f, MCore::Math::epsilon) ||
                !MCore::Compare<float>::CheckIfIsClose(transform.mScale.GetZ(), 1.0f, MCore::Math::epsilon))
            {
                SetNodeFlag(nodeIndex, FLAG_HASSCALE, true);
            }
            //#endif
        }
    #endif
    }


    // set the number of morph weights
    void TransformData::SetNumMorphWeights(uint32 numMorphWeights)
    {
        mPose.ResizeNumMorphs(numMorphWeights);
    }
}   // namespace EMotionFX
