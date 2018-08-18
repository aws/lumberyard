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

// include required headers
#include "Pose.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "MorphSetupInstance.h"
#include "TransformData.h"
#include "MorphSetup.h"
#include "Node.h"

namespace EMotionFX
{
    // default constructor
    Pose::Pose()
    {
        mActorInstance  = nullptr;
        mActor          = nullptr;
        mSkeleton       = nullptr;
        mLocalTransforms.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mGlobalTransforms.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mFlags.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mMorphWeights.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);

        // reset morph weights
        mMorphWeights.Reserve(32);
        mLocalTransforms.Reserve(128);
        mGlobalTransforms.Reserve(128);
        mFlags.Reserve(128);
    }


    // copy constructor
    Pose::Pose(const Pose& other)
    {
        InitFromPose(&other);
    }


    // destructor
    Pose::~Pose()
    {
        Clear(true);
    }


    // link the pose to a given actor instance
    void Pose::LinkToActorInstance(ActorInstance* actorInstance, uint8 initialFlags)
    {
        // store the pointer to the actor instance etc
        mActorInstance  = actorInstance;
        mActor          = actorInstance->GetActor();
        mSkeleton       = mActor->GetSkeleton();

        // resize the buffers
        const uint32 numTransforms = mActor->GetSkeleton()->GetNumNodes();
        mLocalTransforms.ResizeFast(numTransforms);
        mGlobalTransforms.ResizeFast(numTransforms);
        mFlags.ResizeFast(numTransforms);
        mMorphWeights.ResizeFast(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets());

        ClearFlags(initialFlags);
    }


    // link the pose to a given actor
    void Pose::LinkToActor(Actor* actor, uint8 initialFlags, bool clearAllFlags)
    {
        mActorInstance  = nullptr;
        mActor          = actor;
        mSkeleton       = actor->GetSkeleton();

        // resize the buffers
        const uint32 numTransforms = mActor->GetSkeleton()->GetNumNodes();
        mLocalTransforms.ResizeFast(numTransforms);
        mGlobalTransforms.ResizeFast(numTransforms);

        const uint32 oldSize = mFlags.GetLength();
        mFlags.ResizeFast(numTransforms);
        if (oldSize < numTransforms && clearAllFlags == false)
        {
            for (uint32 i = oldSize; i < numTransforms; ++i)
            {
                mFlags[i] = initialFlags;
            }
        }

        MorphSetup* morphSetup = mActor->GetMorphSetup(0);
        mMorphWeights.ResizeFast((morphSetup) ? morphSetup->GetNumMorphTargets() : 0);

        if (clearAllFlags)
        {
            ClearFlags(initialFlags);
        }
    }

    /*
    // link to a given skeleton
    void Pose::LinkToSkeleton(Skeleton* skeleton, uint8 initialFlags, bool clearAllFlags)
    {
        mActor          = nullptr;
        mActorInstance  = nullptr;
        mSkeleton       = skeleton;

        // resize the buffers
        const uint32 numTransforms = mSkeleton->GetNumNodes();
        mLocalTransforms.ResizeFast( numTransforms );
        mGlobalTransforms.ResizeFast( numTransforms );

        const uint32 oldSize = mFlags.GetLength();
        mFlags.ResizeFast( numTransforms );
        if (oldSize < numTransforms && clearAllFlags == false)
        {
            for (uint32 i=oldSize; i<numTransforms; ++i)
                mFlags[i] = initialFlags;
        }

        Transform identTransform;
        identTransform.Identity();
        for (uint32 i=oldSize; i<numTransforms; ++i)
            SetLocalTransform(i, identTransform);

        MorphSetup* morphSetup = mActor->GetMorphSetup(0);
        mMorphWeights.ResizeFast( (morphSetup!=nullptr) ? morphSetup->GetNumMorphTargets() : 0 );

        if (clearAllFlags)
            ClearFlags(initialFlags);
    }
    */

    //
    void Pose::SetNumTransforms(uint32 numTransforms)
    {
        // resize the buffers
        mLocalTransforms.ResizeFast(numTransforms);
        mGlobalTransforms.ResizeFast(numTransforms);

        const uint32 oldSize = mFlags.GetLength();
        mFlags.ResizeFast(numTransforms);

        Transform identTransform;
        identTransform.Identity();
        for (uint32 i = oldSize; i < numTransforms; ++i)
        {
            mFlags[i] = 0;
            SetLocalTransform(i, identTransform);
        }
    }


    // clear the pose
    void Pose::Clear(bool clearMem)
    {
        mLocalTransforms.Clear(clearMem);
        mGlobalTransforms.Clear(clearMem);
        mFlags.Clear(clearMem);
        mMorphWeights.Clear(clearMem);
    }


    // clear the pose flags
    void Pose::ClearFlags(uint8 newFlags)
    {
        MCore::MemSet((uint8*)mFlags.GetPtr(), newFlags, sizeof(uint8) * mFlags.GetLength());
    }


    /*
    // init from a set of local space transformations
    void Pose::InitFromLocalTransforms(ActorInstance* actorInstance, const Transform* localTransforms)
    {
        // link to an actor instance
        LinkToActorInstance( actorInstance, FLAG_LOCALTRANSFORMREADY );

        // reset all flags
        //MCore::MemSet( (uint8*)mFlags.GetPtr(), FLAG_LOCALTRANSFORMREADY, sizeof(uint8)*mFlags.GetLength() );

        // copy over the local transforms
        MCore::MemCopy((uint8*)mLocalTransforms.GetPtr(), (uint8*)localTransforms, sizeof(Transform)*mLocalTransforms.GetLength());
    }
    */

    // initialize this pose to the bind pose
    void Pose::InitFromBindPose(ActorInstance* actorInstance)
    {
        // init to the bind pose
        InitFromPose(actorInstance->GetTransformData()->GetBindPose());

        // compensate for motion extraction
        // we already moved our actor instance's position and rotation at this point
        // so we have to cancel/compensate this delta offset from the motion extraction node, so that we don't double-transform
        // basically this will keep the motion in-place rather than moving it away from the origin
        //CompensateForMotionExtractionDirect();
    }


    // init to the actor bind pose
    void Pose::InitFromBindPose(Actor* actor)
    {
        InitFromPose(actor->GetBindPose());
    }


    /*
    // initialize this pose from some given set of local space transformations
    void Pose::InitFromLocalBindSpaceTransforms(Actor* actor)
    {
        // link to an actor
        LinkToActor(actor);

        // reset all flags
        MCore::MemSet( (uint8*)mFlags.GetPtr(), FLAG_LOCALTRANSFORMREADY, sizeof(uint8)*mFlags.GetLength() );

        // copy over the local transforms
        MCore::MemCopy((uint8*)mLocalTransforms.GetPtr(), (uint8*)actor->GetBindPose().GetLocalTransforms(), sizeof(Transform)*mLocalTransforms.GetLength());

        // reset the morph targets
        const uint32 numMorphWeights = mMorphWeights.GetLength();
        for (uint32 i=0; i<numMorphWeights; ++i)
            mMorphWeights[i] = 0.0f;
    }
    */

    // update the full local space pose
    void Pose::ForceUpdateFullLocalPose()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                GetGlobalTransform(parentIndex, &mLocalTransforms[i]);  // calculate the global parent index transform
                mLocalTransforms[i].Inverse();
                mLocalTransforms[i].PreMultiply(mGlobalTransforms[i]);
            }
            else
            {
                mLocalTransforms[i] = mGlobalTransforms[i];
            }

            mFlags[i] |= FLAG_LOCALTRANSFORMREADY;
        }
    }


    // update the full global space pose
    void Pose::ForceUpdateFullGlobalPose()
    {
        // iterate from root towards child nodes recursively, updating all global matrices on the way
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                mGlobalTransforms[parentIndex].PreMultiply(mLocalTransforms[i], &mGlobalTransforms[i]);
            }
            else
            {
                mGlobalTransforms[i] = mLocalTransforms[i];
            }

            mFlags[i] |= FLAG_GLOBALTRANSFORMREADY;
        }
    }


    // recursively update
    void Pose::UpdateGlobalTransform(uint32 nodeIndex) const
    {
        Skeleton* skeleton = mActor->GetSkeleton();

        const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != MCORE_INVALIDINDEX32)
        {
            UpdateGlobalTransform(parentIndex);
        }

        // update the global transform if needed
        if ((mFlags[nodeIndex] & FLAG_GLOBALTRANSFORMREADY) == false)
        {
            //MCore::LogInfo("Calculating global transform for node '%s'", mActor->GetNode(nodeIndex)->GetName());
            Transform localTransform;
            GetLocalTransform(nodeIndex, &localTransform);
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                mGlobalTransforms[parentIndex].PreMultiply(localTransform, &mGlobalTransforms[nodeIndex]);
            }
            else
            {
                mGlobalTransforms[nodeIndex] = mLocalTransforms[nodeIndex];
            }

            mFlags[nodeIndex] |= FLAG_GLOBALTRANSFORMREADY;
        }
    }


    // update the local transform
    void Pose::UpdateLocalTransform(uint32 nodeIndex) const
    {
        const uint32 flags = mFlags[nodeIndex];
        if (flags & FLAG_LOCALTRANSFORMREADY)
        {
            return;
        }

        MCORE_ASSERT(flags & FLAG_GLOBALTRANSFORMREADY); // the global transform has to be updated already, otherwise we cannot possibly calculate the local space one
        //if ((flags & FLAG_GLOBALTRANSFORMREADY) == false)
        //      DebugBreak();

        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != MCORE_INVALIDINDEX32)
        {
            GetGlobalTransform(parentIndex, &mLocalTransforms[nodeIndex]);  // calculate the global parent index transform
            mLocalTransforms[nodeIndex].Inverse();
            mLocalTransforms[nodeIndex].PreMultiply(mGlobalTransforms[nodeIndex]);
        }
        else
        {
            mLocalTransforms[nodeIndex] = mGlobalTransforms[nodeIndex];
        }

        mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;
        //MCore::LogInfo("Calculated local transform for node '%s'", skeleton->GetNode(nodeIndex)->GetName());
    }


    // get the local transform
    const Transform& Pose::GetLocalTransform(uint32 nodeIndex) const
    {
        if ((mFlags[nodeIndex] & FLAG_LOCALTRANSFORMREADY))
        {
            return mLocalTransforms[nodeIndex];
        }
        else
        {
            UpdateLocalTransform(nodeIndex);
            return mLocalTransforms[nodeIndex];
        }
    }


    // get the global transform
    const Transform& Pose::GetGlobalTransform(uint32 nodeIndex) const
    {
        // make sure the transform is up to date (checks parent nodes etc)
        UpdateGlobalTransform(nodeIndex);
        return mGlobalTransforms[nodeIndex];
    }


    // get the global transform including the actor instance transform
    Transform Pose::GetGlobalTransformInclusive(uint32 nodeIndex) const
    {
        // make sure the transform is up to date (checks parent nodes etc)
        UpdateGlobalTransform(nodeIndex);
        return mGlobalTransforms[nodeIndex].Multiplied(mActorInstance->GetGlobalTransform());
    }


    // get the global transform including the actor instance transform
    void Pose::GetGlobalTransformInclusive(uint32 nodeIndex, Transform* outResult) const
    {
        // make sure the transform is up to date (checks parent nodes etc)
        UpdateGlobalTransform(nodeIndex);
        *outResult = mGlobalTransforms[nodeIndex];
        outResult->Multiply(mActorInstance->GetGlobalTransform());
    }


    // calculate a local transform
    void Pose::GetLocalTransform(uint32 nodeIndex, Transform* outResult) const
    {
        if ((mFlags[nodeIndex] & FLAG_LOCALTRANSFORMREADY) == false)
        {
            UpdateLocalTransform(nodeIndex);
        }

        *outResult = mLocalTransforms[nodeIndex];
    }


    // calculate a global transform
    void Pose::GetGlobalTransform(uint32 nodeIndex, Transform* outResult) const
    {
        // make sure the transform is up to date (checks parent nodes etc)
        UpdateGlobalTransform(nodeIndex);
        *outResult = mGlobalTransforms[nodeIndex];
    }


    // set the local transform
    void Pose::SetLocalTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateGlobalTransforms)
    {
        mLocalTransforms[nodeIndex] = newTransform;
        mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;

        // mark all child node global transforms as dirty (recursively)
        if (invalidateGlobalTransforms)
        {
            if (mFlags[nodeIndex] & FLAG_GLOBALTRANSFORMREADY)
            {
                RecursiveInvalidateGlobalTransforms(mActor, nodeIndex);
            }
        }
    }


    // mark all child nodes recursively as dirty
    void Pose::RecursiveInvalidateGlobalTransforms(Actor* actor, uint32 nodeIndex)
    {
        // if this global transform ain't ready yet assume all child nodes are also not
        if ((mFlags[nodeIndex] & FLAG_GLOBALTRANSFORMREADY) == false)
        {
            return;
        }

        // mark the global transform as invalid
        mFlags[nodeIndex] &= ~FLAG_GLOBALTRANSFORMREADY;

        // recurse through all child nodes
        Skeleton* skeleton = actor->GetSkeleton();
        Node* node = skeleton->GetNode(nodeIndex);
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveInvalidateGlobalTransforms(actor, node->GetChildIndex(i));
        }
    }


    // set the global transform
    void Pose::SetGlobalTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        mGlobalTransforms[nodeIndex] = newTransform;

        // invalidate the local transform
        mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        // recursively invalidate all global transforms of all child nodes
        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateGlobalTransforms(mActor, nodeIndex);
        }

        // mark this global transform as ready
        mFlags[nodeIndex] |= FLAG_GLOBALTRANSFORMREADY;
    }


    // set the global transform that already has the current global transform of the actor instance included
    void Pose::SetGlobalTransformInclusive(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        Transform inverseActorInstanceTransform = mActorInstance->GetGlobalTransform().Inversed();
        mGlobalTransforms[nodeIndex] = newTransform.Multiplied(inverseActorInstanceTransform);

        // invalidate the local transform
        mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        // recursively invalidate all global transforms of all child nodes
        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateGlobalTransforms(mActor, nodeIndex);
        }

        // mark this global transform as ready
        mFlags[nodeIndex] |= FLAG_GLOBALTRANSFORMREADY;
    }



    // invalidate all local transforms
    void Pose::InvalidateAllLocalTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~FLAG_LOCALTRANSFORMREADY;
        }
    }


    // invalidate all global transforms
    void Pose::InvalidateAllGlobalTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~FLAG_GLOBALTRANSFORMREADY;
        }
    }


    // invalidate all local and global transforms
    void Pose::InvalidateAllLocalAndGlobalTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~(FLAG_LOCALTRANSFORMREADY | FLAG_GLOBALTRANSFORMREADY);
        }
    }


    // Get the trajectory transform.
    Transform Pose::CalcTrajectoryTransform() const
    {
        MCORE_ASSERT( mActor );
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex == MCORE_INVALIDINDEX32)
        {
            Transform result;
            result.Identity();
            return result;
        }

        return GetGlobalTransformInclusive(motionExtractionNodeIndex).ProjectedToGroundPlane();
    }


    // update all local transforms if needed
    void Pose::UpdateAllLocalTranforms()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            UpdateLocalTransform(i);
        }
    }


    // update all global transforms
    void Pose::UpdateAllGlobalTranforms()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            UpdateGlobalTransform(i);
        }
    }


    //-----------------------------------------------------------------

    // blend two poses, non mixing
    void Pose::BlendNonMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        // make sure the number of transforms are equal
        MCORE_ASSERT(destPose);
        MCORE_ASSERT(outPose );
        MCORE_ASSERT(mLocalTransforms.GetLength() == destPose->mLocalTransforms.GetLength());
        MCORE_ASSERT(mLocalTransforms.GetLength() == outPose->mLocalTransforms.GetLength());
        MCORE_ASSERT(instance->GetIsMixing() == false);
        MCORE_ASSERT(instance->GetNumMotionLinks() == mLocalTransforms.GetLength());

        // get some motion instance properties which we use to decide the optimized blending routine
        const bool additive     = (instance->GetBlendMode() == BLENDMODE_ADDITIVE); // additively blend?
        const bool hasBlendMask = instance->GetHasBlendMask();
        ActorInstance*  actorInstance   = instance->GetActorInstance();
        //  Actor*          actor           = actorInstance->GetActor();

        // blend all transforms
        if (hasBlendMask == false) // optimized version without blendmask
        {
            // check if we want an additive blend or not
            if (additive == false)
            {
                // if the dest pose has full influence, simply copy over that pose instead of performing blending
                if (weight >= 1.0f)
                {
                    outPose->InitFromPose(destPose);
                }
                else
                {
                    if (weight > 0.0f)
                    {
                        Transform transform;
                        uint32 nodeNr;
                        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                        for (uint32 i = 0; i < numNodes; ++i)
                        {
                            nodeNr = actorInstance->GetEnabledNode(i);
                            transform = GetLocalTransform(nodeNr);
                            transform.Blend(destPose->GetLocalTransform(nodeNr), weight);
                            outPose->SetLocalTransform(nodeNr, transform, false);
                        }
                        outPose->InvalidateAllGlobalTransforms();
                    }
                    else // if the weight is 0, so the source
                    {
                        if (outPose != this)
                        {
                            outPose->InitFromPose(this); // TODO: make it use the motionInstance->GetActorInstance()?
                        }
                    }
                }

                // blend the morph weights
                const uint32 numMorphs = mMorphWeights.GetLength();
                MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
                MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
                for (uint32 i = 0; i < numMorphs; ++i)
                {
                    mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
                }
            }
            else
            {
                TransformData* transformData = instance->GetActorInstance()->GetTransformData();
                const Pose* bindPose = transformData->GetBindPose();
                uint32 nodeNr;
                Transform result;
                const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    nodeNr = actorInstance->GetEnabledNode(i);
                    const Transform& base = bindPose->GetLocalTransform(nodeNr);
                    BlendTransformAdditiveUsingBindPose(base, GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), weight, &result);
                    outPose->SetLocalTransform(nodeNr, result, false);
                }
                outPose->InvalidateAllGlobalTransforms();

                // blend the morph weights
                const uint32 numMorphs = mMorphWeights.GetLength();
                MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
                MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
                for (uint32 i = 0; i < numMorphs; ++i)
                {
                    mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
                }
            }
        }
        else // take the blendmask into account
        {
            if (additive == false)
            {
                Transform result;
                uint32 nodeNr;
                const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    nodeNr = actorInstance->GetEnabledNode(i);

                    // try to find the motion link
                    // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                    float nodeBlendWeight;
                    const MotionLink* motionLink = instance->GetMotionLink(nodeNr);
                    if (motionLink->GetIsActive() == false)
                    {
                        nodeBlendWeight = 1.0f;
                    }
                    else
                    {
                        nodeBlendWeight = instance->GetNodeWeight(nodeNr);
                    }

                    // take the node's blend weight into account
                    const float finalWeight = nodeBlendWeight * weight;

                    // blend the source into dest with the given weight and output it inside the output pose
                    BlendTransformWithWeightCheck(GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), finalWeight, &result);
                    outPose->SetLocalTransform(nodeNr, result, false);
                }

                outPose->InvalidateAllGlobalTransforms();

                // blend the morph weights
                const uint32 numMorphs = mMorphWeights.GetLength();
                MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
                MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
                for (uint32 i = 0; i < numMorphs; ++i)
                {
                    mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
                }
            }
            else
            {
                TransformData* transformData = instance->GetActorInstance()->GetTransformData();
                const Pose* bindPose = transformData->GetBindPose();
                Transform result;

                uint32 nodeNr;
                const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    nodeNr = actorInstance->GetEnabledNode(i);

                    // try to find the motion link
                    // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                    float nodeBlendWeight;
                    const MotionLink* motionLink = instance->GetMotionLink(nodeNr);
                    if (motionLink->GetIsActive() == false)
                    {
                        nodeBlendWeight = 1.0f;
                    }
                    else
                    {
                        nodeBlendWeight = instance->GetNodeWeight(nodeNr);
                    }

                    // take the node's blend weight into account
                    const float finalWeight = nodeBlendWeight * weight;

                    // blend the source into dest with the given weight and output it inside the output pose
                    BlendTransformAdditiveUsingBindPose(bindPose->GetLocalTransform(nodeNr), GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), finalWeight, &result);
                    outPose->SetLocalTransform(nodeNr, result, false);
                }
                outPose->InvalidateAllGlobalTransforms();

                // blend the morph weights
                const uint32 numMorphs = mMorphWeights.GetLength();
                MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
                MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
                for (uint32 i = 0; i < numMorphs; ++i)
                {
                    mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
                }
            } // if additive
        } // if has blend mask
    }


    // blend two poses, mixing
    void Pose::BlendMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        // make sure the number of transforms are equal
        MCORE_ASSERT(destPose);
        MCORE_ASSERT(outPose );
        MCORE_ASSERT(mLocalTransforms.GetLength() == destPose->mLocalTransforms.GetLength());
        MCORE_ASSERT(mLocalTransforms.GetLength() == outPose->mLocalTransforms.GetLength());
        MCORE_ASSERT(instance->GetIsMixing());
        MCORE_ASSERT(instance->GetNumMotionLinks() == mLocalTransforms.GetLength());

        // do we blend additively?
        const bool additive = (instance->GetBlendMode() == BLENDMODE_ADDITIVE);

        // get the actor from the instance
        ActorInstance* actorInstance = instance->GetActorInstance();
        //  Actor* actor = actorInstance->GetActor();

        // get the transform data
        TransformData* transformData = instance->GetActorInstance()->GetTransformData();
        Transform result;

        // blend all transforms
        if (additive == false)
        {
            uint32 nodeNr;
            const uint32 numNodes = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = actorInstance->GetEnabledNode(i);

                // try to find the motion link
                // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                const MotionLink* motionLink = instance->GetMotionLink(nodeNr);
                if (motionLink->GetIsActive() == false)
                {
                    continue;
                }

                // take the node's blend weight into account
                const float finalWeight = instance->GetNodeWeight(nodeNr) * weight;

                // blend the source into dest with the given weight and output it inside the output pose
                BlendTransformWithWeightCheck(GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), finalWeight, &result);
                outPose->SetLocalTransform(nodeNr, result, false);
            }

            outPose->InvalidateAllGlobalTransforms();

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }
        }
        else
        {
            Pose* bindPose = transformData->GetBindPose();
            uint32 nodeNr;
            const uint32 numNodes = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = actorInstance->GetEnabledNode(i);

                // try to find the motion link
                // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                const MotionLink* motionLink = instance->GetMotionLink(nodeNr);
                if (motionLink->GetIsActive() == false)
                {
                    continue;
                }

                // take the node's blend weight into account
                const float finalWeight = instance->GetNodeWeight(nodeNr) * weight;

                // blend the source into dest with the given weight and output it inside the output pose
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalTransform(nodeNr), GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), finalWeight, &result);
                outPose->SetLocalTransform(nodeNr, result, false);
            }
            outPose->InvalidateAllGlobalTransforms();

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }
    }


    // init from another pose
    void Pose::InitFromPose(const Pose* sourcePose)
    {
        if (sourcePose == nullptr)
        {
            if (mActorInstance)
            {
                InitFromBindPose(mActorInstance);
            }
            else
            {
                InitFromBindPose(mActor);
            }

            return;
        }

        mGlobalTransforms.MemCopyContentsFrom(sourcePose->mGlobalTransforms);
        mLocalTransforms.MemCopyContentsFrom(sourcePose->mLocalTransforms);
        mFlags.MemCopyContentsFrom(sourcePose->mFlags);
        mMorphWeights.MemCopyContentsFrom(sourcePose->mMorphWeights);
    }


    // blend, and auto-detect if it is a mixing or non mixing blend
    void Pose::Blend(const Pose* destPose, float weight, const MotionInstance* instance)
    {
        if (instance->GetIsMixing() == false)
        {
            BlendNonMixed(destPose, weight, instance, this);
        }
        else
        {
            BlendMixed(destPose, weight, instance, this);
        }

        InvalidateAllGlobalTransforms();
    }


    // blend, and auto-detect if it is a mixing or non mixing blend
    void Pose::Blend(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        if (instance->GetIsMixing() == false)
        {
            BlendNonMixed(destPose, weight, instance, outPose);
        }
        else
        {
            BlendMixed(destPose, weight, instance, outPose);
        }

        InvalidateAllGlobalTransforms();
    }


    // reset all transforms to zero
    void Pose::Zero()
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                mLocalTransforms[nodeNr].Zero();
            }

            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = 0.0f;
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mLocalTransforms[i].Zero();
            }

            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = 0.0f;
            }
        }

        InvalidateAllGlobalTransforms();
    }


    // normalize all quaternions
    void Pose::NormalizeQuaternions()
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                UpdateLocalTransform(nodeNr);
                mLocalTransforms[nodeNr].mRotation.Normalize();
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                UpdateLocalTransform(i);
                mLocalTransforms[i].mRotation.Normalize();
            }
        }
    }


    // add the transforms of another pose to this one
    void Pose::Sum(const Pose* other, float weight)
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);

                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                const Transform& otherTransform = other->GetLocalTransform(nodeNr);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += other->mMorphWeights[i] * weight;
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                const Transform& otherTransform = other->GetLocalTransform(i);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += other->mMorphWeights[i] * weight;
            }
        }

        InvalidateAllGlobalTransforms();
    }


    // blend, without motion instance
    void Pose::Blend(const Pose* destPose, float weight)
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& curTransform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                curTransform.Blend(destPose->GetLocalTransform(nodeNr), weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& curTransform = const_cast<Transform&>(GetLocalTransform(i));
                curTransform.Blend(destPose->GetLocalTransform(i), weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }
        }

        InvalidateAllGlobalTransforms();
    }


    Pose& Pose::MakeRelativeTo(const Pose& other)
    {
        AZ_Assert(mLocalTransforms.GetLength() == other.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                transform = transform.CalcRelativeTo(other.GetLocalTransform(nodeNr));
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                transform = transform.CalcRelativeTo(other.GetLocalTransform(i));
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == other.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] -= other.mMorphWeights[i];
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose, float weight)
    {
        AZ_Assert(mLocalTransforms.GetLength() == additivePose.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                const Transform& additiveTransform = additivePose.GetLocalTransform(nodeNr);
                transform.mPosition += additiveTransform.mPosition * weight;
                transform.mRotation = transform.mRotation.NLerp(transform.mRotation * additiveTransform.mRotation, weight);
                EMFX_SCALECODE
                (
                    transform.mScale += additiveTransform.mScale * weight;
                )
                transform.mRotation.Normalize();
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                const Transform& additiveTransform = additivePose.GetLocalTransform(i);
                transform.mPosition += additiveTransform.mPosition * weight;
                transform.mRotation = transform.mRotation.NLerp(transform.mRotation * additiveTransform.mRotation, weight);
                EMFX_SCALECODE
                (
                    transform.mScale += additiveTransform.mScale * weight;
                )
                transform.mRotation.Normalize();
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] += additivePose.mMorphWeights[i] * weight;
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose)
    {
        AZ_Assert(mLocalTransforms.GetLength() == additivePose.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                const Transform& additiveTransform = additivePose.GetLocalTransform(nodeNr);
                transform.mPosition += additiveTransform.mPosition;
                transform.mRotation = transform.mRotation * additiveTransform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale += additiveTransform.mScale;
                )
                transform.mRotation.Normalize();
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                const Transform& additiveTransform = additivePose.GetLocalTransform(i);
                transform.mPosition += additiveTransform.mPosition;
                transform.mRotation = transform.mRotation * additiveTransform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale += additiveTransform.mScale;
                )
                transform.mRotation.Normalize();
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] += additivePose.mMorphWeights[i];
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }


    Pose& Pose::MakeAdditive(const Pose& refPose)
    {
        AZ_Assert(mLocalTransforms.GetLength() == refPose.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                const Transform& refTransform = refPose.GetLocalTransform(nodeNr);
                transform.mPosition = transform.mPosition - refTransform.mPosition;
                transform.mRotation = refTransform.mRotation.Conjugated() * transform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale = transform.mScale - refTransform.mScale;
                )
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                const Transform& refTransform = refPose.GetLocalTransform(i);
                transform.mPosition = transform.mPosition - refTransform.mPosition;
                transform.mRotation = refTransform.mRotation.Conjugated() * transform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale = transform.mScale - refTransform.mScale;
                )
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == refPose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] -= refPose.mMorphWeights[i];
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }


    // additive blend
    void Pose::BlendAdditiveUsingBindPose(const Pose* destPose, float weight)
    {
        if (mActorInstance)
        {
            const TransformData* transformData = mActorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalTransform(nodeNr), GetLocalTransform(nodeNr), destPose->GetLocalTransform(nodeNr), weight, &result);
                SetLocalTransform(nodeNr, result, false);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }
        else
        {
            const TransformData* transformData = mActorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalTransform(i), GetLocalTransform(i), destPose->GetLocalTransform(i), weight, &result);
                SetLocalTransform(i, result, false);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }

        InvalidateAllGlobalTransforms();
    }


    // blend a transformation with weight check optimization
    void Pose::BlendTransformWithWeightCheck(const Transform& source, const Transform& dest, float weight, Transform* outTransform)
    {
        if (weight >= 1.0f)
        {
            *outTransform = dest;
        }
        else
        {
            if (weight > 0.0f)
            {
                *outTransform = source;
                outTransform->Blend(dest, weight);
            }
            else
            {
                *outTransform = source;
            }
        }
    }


    // blend two transformations additively
    void Pose::BlendTransformAdditiveUsingBindPose(const Transform& baseLocalTransform, const Transform& source, const Transform& dest, float weight, Transform* outTransform)
    {
        // get the node index
        *outTransform = source;
        outTransform->BlendAdditive(dest, baseLocalTransform, weight);
    }


    Pose& Pose::operator=(const Pose& other)
    {
        InitFromPose(&other);
        return *this;
    }


    // compensate for motion extraction, basically making it in-place
    void Pose::CompensateForMotionExtractionDirect(EMotionExtractionFlags motionExtractionFlags)
    {
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            Transform motionExtractionNodeTransform = GetLocalTransformDirect(motionExtractionNodeIndex);
            mActorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalTransformDirect(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // compensate for motion extraction, basically making it in-place
    void Pose::CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags)
    {
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            Transform motionExtractionNodeTransform = GetLocalTransform(motionExtractionNodeIndex);
            mActorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalTransform(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // apply the morph target weights to the morph setup instance of the given actor instance
    void Pose::ApplyMorphWeightsToActorInstance()
    {
        MorphSetupInstance* morphSetupInstance = mActorInstance->GetMorphSetupInstance();
        const uint32 numMorphs = morphSetupInstance->GetNumMorphTargets();
        for (uint32 m = 0; m < numMorphs; ++m)
        {
            MorphSetupInstance::MorphTarget* morphTarget = morphSetupInstance->GetMorphTarget(m);
            if (morphTarget->GetIsInManualMode() == false)
            {
                morphTarget->SetWeight(mMorphWeights[m]);
            }
        }
    }


    // zero all morph weights
    void Pose::ZeroMorphWeights()
    {
        const uint32 numMorphs = mMorphWeights.GetLength();
        for (uint32 m = 0; m < numMorphs; ++m)
        {
            mMorphWeights[m] = 0.0f;
        }
    }


    void Pose::ResizeNumMorphs(uint32 numMorphTargets)
    {
        mMorphWeights.Resize(numMorphTargets);
    }


    Pose& Pose::PreMultiply(const Pose& other)
    {
        AZ_Assert(mLocalTransforms.GetLength() == other.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                Transform otherTransform = other.GetLocalTransform(nodeNr);
                transform = otherTransform * transform;
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                Transform otherTransform = other.GetLocalTransform(i);
                transform = otherTransform * transform;
            }
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }
    

    Pose& Pose::Multiply(const Pose& other)
    {
        AZ_Assert(mLocalTransforms.GetLength() == other.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                transform.Multiply(other.GetLocalTransform(nodeNr));
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                transform.Multiply(other.GetLocalTransform(i));
            }
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }


    Pose& Pose::MultiplyInverse(const Pose& other)
    {
        AZ_Assert(mLocalTransforms.GetLength() == other.mLocalTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalTransform(nodeNr));
                Transform otherTransform = other.GetLocalTransform(nodeNr);
                otherTransform.Inverse();
                transform.PreMultiply(otherTransform);
            }
        }
        else
        {
            const uint32 numNodes = mLocalTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalTransform(i));
                Transform otherTransform = other.GetLocalTransform(i);
                otherTransform.Inverse();
                transform.PreMultiply(otherTransform);
            }
        }

        InvalidateAllGlobalTransforms();
        return *this;
    }



}   // namespace EMotionFX
