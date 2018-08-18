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
#include "SkeletalMotion.h"
#include "SkeletalSubMotion.h"
#include "Node.h"
#include "Actor.h"
#include "EventManager.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "KeyTrackLinear.h"
#include "MorphSubMotion.h"
#include "TransformData.h"
#include "AnimGraphPose.h"
#include "EMotionFXManager.h"
#include "MorphSetupInstance.h"
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SkeletalMotion, MotionAllocator, 0)


    // constructor
    SkeletalMotion::SkeletalMotion(const char* name)
        : Motion(name)
    {
        mSubMotions.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS);
        mMorphSubMotions.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS);
        mDoesInfluencePhonemes = false;
    }


    // destructor
    SkeletalMotion::~SkeletalMotion()
    {
        RemoveAllSubMotions();
    }


    // create
    SkeletalMotion* SkeletalMotion::Create(const char* name)
    {
        return aznew SkeletalMotion(name);
    }


    // returns the type ID of the motion
    uint32 SkeletalMotion::GetType() const
    {
        return SkeletalMotion::TYPE_ID;
    }


    // returns the type as string
    const char* SkeletalMotion::GetTypeString() const
    {
        return "SkeletalMotion";
    }


    // get rid of all submotions
    void SkeletalMotion::RemoveAllSubMotions()
    {
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            mSubMotions[i]->Destroy();
        }
        mSubMotions.Clear();

        // delete all morph sub motions
        const uint32 numMorphSubMotions = mMorphSubMotions.GetLength();
        for (uint32 i = 0; i < numMorphSubMotions; ++i)
        {
            mMorphSubMotions[i]->Destroy();
        }
        mMorphSubMotions.Clear();
    }


    // calculate the node transform
    void SkeletalMotion::CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting)
    {
        // get some required shortcuts
        ActorInstance*  actorInstance   = instance->GetActorInstance();
        TransformData*  transformData   = actorInstance->GetTransformData();

        const Pose* bindPose = transformData->GetBindPose();

        // get the node index/number
        const uint32 nodeIndex = node->GetNodeIndex();

        // search for the motion link
        const MotionLink* motionLink = instance->GetMotionLink(nodeIndex);
        if (motionLink->GetIsActive() == false)
        {
            *outTransform = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex);
            return;
        }

        // get the current time and the submotion
        SkeletalSubMotion* subMotion = mSubMotions[ motionLink->GetSubMotionIndex() ];

        // get position
        auto* posTrack = subMotion->GetPosTrack();
        if (posTrack)
        {
            outTransform->mPosition = AZ::Vector3(posTrack->GetValueAtTime(timeValue));
        }
        else
        {
            outTransform->mPosition = subMotion->GetPosePos();
        }

        // get rotation
        KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();
        if (rotTrack)
        {
            outTransform->mRotation = rotTrack->GetValueAtTime(timeValue);
        }
        else
        {
            outTransform->mRotation = subMotion->GetPoseRot();
        }

    #ifndef EMFX_SCALE_DISABLED
        // get scale
        auto* scaleTrack = subMotion->GetScaleTrack();
        if (scaleTrack)
        {
            outTransform->mScale = AZ::Vector3(scaleTrack->GetValueAtTime(timeValue));
        }
        else
        {
            outTransform->mScale = subMotion->GetPoseScale();
        }
    #endif

        // if we want to use retargeting
        if (enableRetargeting)
        {
            BasicRetarget(instance, subMotion, nodeIndex, *outTransform);
        }

        // mirror
        if (instance->GetMirrorMotion() && actor->GetHasMirrorInfo())
        {
            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeIndex);
            Transform mirrored = bindPose->GetLocalTransform(nodeIndex);
            AZ::Vector3 mirrorAxis(0.0f, 0.0f, 0.0f);
            mirrorAxis.SetElement(mirrorInfo.mAxis, 1.0f);
            const uint32 motionSource = actor->GetNodeMirrorInfo(nodeIndex).mSourceNode;
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalTransform(motionSource), *outTransform, mirrorAxis, mirrorInfo.mFlags);
            *outTransform = mirrored;
        }
    }


    // creates the  motion links in the nodes of the given actor
    void SkeletalMotion::CreateMotionLinks(Actor* actor, MotionInstance* instance)
    {
        Skeleton* skeleton = actor->GetSkeleton();

        // if no start node has been set
        if (instance->GetStartNodeIndex() == MCORE_INVALIDINDEX32)
        {
            const uint32 numNodes = actor->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                // find the submotion for this node
                const uint32 subMotionIndex = FindSubMotionIndexByID(skeleton->GetNode(i)->GetID());
                if (subMotionIndex != MCORE_INVALIDINDEX32) // if a submotion has been found
                {
                    instance->GetMotionLink(i)->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
                }
            }
        }
        else // if we want to use a start node
        {
            RecursiveCreateMotionLinkForNode(actor, skeleton->GetNode(instance->GetStartNodeIndex()), instance);
        }
    }


    // recursively create motion links down the hierarchy of this node
    void SkeletalMotion::RecursiveCreateMotionLinkForNode(Actor* actor, Node* node, MotionInstance* instance)
    {
        Skeleton* skeleton = actor->GetSkeleton();

        // if mirroring is disabled or there is no node motion source array (no node remapping available)
        if (instance->GetMirrorMotion() == false || actor->GetHasMirrorInfo() == false || node->GetIsAttachmentNode())
        {
            // try to find the submotion for this node
            const uint32 subMotionIndex = FindSubMotionIndexByID(node->GetID());
            if (subMotionIndex != MCORE_INVALIDINDEX32) // if it exists, set the motion link and activate it
            {
                instance->GetMotionLink(node->GetNodeIndex())->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
            }

            // process all child nodes
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RecursiveCreateMotionLinkForNode(actor, skeleton->GetNode(node->GetChildIndex(i)), instance);
            }
        }
        else
        {
            // get the source node to get the anim data from
            Node* sourceNode = node;
            uint32 sourceNodeIndex = actor->GetNodeMirrorInfo(node->GetNodeIndex()).mSourceNode;
            if (sourceNodeIndex != MCORE_INVALIDINDEX16)
            {
                sourceNode = skeleton->GetNode(sourceNodeIndex);
            }

            // try to find the submotion for this node
            const uint32 subMotionIndex = FindSubMotionIndexByID(sourceNode->GetID());
            if (subMotionIndex != MCORE_INVALIDINDEX32) // if it exists, set the motion link and activate it
            {
                instance->GetMotionLink(node->GetNodeIndex())->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
            }

            // process all child nodes
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RecursiveCreateMotionLinkForNode(actor, skeleton->GetNode(node->GetChildIndex(i)), instance);
            }
        }
    }


    // get the maximum time value and update the stored one
    void SkeletalMotion::UpdateMaxTime()
    {
        mMaxTime = 0.0f;

        // for all sub motions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SkeletalSubMotion* subMotion = mSubMotions[i];

            // check the position track
            if (subMotion->GetPosTrack() && subMotion->GetPosTrack()->GetLastTime() > mMaxTime)
            {
                mMaxTime = subMotion->GetPosTrack()->GetLastTime();
            }

            // check the rotation track
            if (subMotion->GetRotTrack() && subMotion->GetRotTrack()->GetLastTime() > mMaxTime)
            {
                mMaxTime = subMotion->GetRotTrack()->GetLastTime();
            }

            EMFX_SCALECODE
            (
                // check the scale track
                if (subMotion->GetScaleTrack() && subMotion->GetScaleTrack()->GetLastTime() > mMaxTime)
                {
                    mMaxTime = subMotion->GetScaleTrack()->GetLastTime();
                }
            )
        }


        // check key tracks for max value
        uint32 numMorphSubMotions = mMorphSubMotions.GetLength();
        for (uint32 i = 0; i < numMorphSubMotions; ++i)
        {
            MorphSubMotion* subMotion = mMorphSubMotions[i];
            KeyTrackLinear<float, MCore::Compressed16BitFloat>* keytrack = subMotion->GetKeyTrack();

            // check the two time values and return the biggest one
            mMaxTime = MCore::Max(mMaxTime, keytrack->GetLastTime());
        }
    }


    // returns if we want to overwrite nodes in non-mixing mode or not
    bool SkeletalMotion::GetDoesOverwriteInNonMixMode() const
    {
        return true;
    }


    // make the motion loopable
    void SkeletalMotion::MakeLoopable(float fadeTime)
    {
        // make all tracks of all submotions loopable
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SkeletalSubMotion* subMotion = mSubMotions[i];

            if (subMotion->GetPosTrack())
            {
                subMotion->GetPosTrack()->MakeLoopable(fadeTime);
            }

            if (subMotion->GetRotTrack())
            {
                subMotion->GetRotTrack()->MakeLoopable(fadeTime);
            }

            EMFX_SCALECODE
            (
                if (subMotion->GetScaleTrack())
                {
                    subMotion->GetScaleTrack()->MakeLoopable(fadeTime);
                }
            )
        }

        // make the key tracks loopable
        uint32 numMorphSubMotions = mMorphSubMotions.GetLength();
        for (uint32 i = 0; i < numMorphSubMotions; ++i)
        {
            mMorphSubMotions[i]->GetKeyTrack()->MakeLoopable(fadeTime);
        }

        // update the maximum time
        UpdateMaxTime();
    }

    /*
    // setup the motion bind pose values that are used for retargeting, and base them on a given actor
    void SkeletalMotion::SetRetargetSource(Actor* actor)
    {
        TransformData* transformData = actor->GetTransformData();

        // for all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i=0; i<numSubMotions; ++i)
        {
            SkeletalSubMotion* subMotion = mSubMotions[i];

            // find the node inside the actor, that is linked to this submotion
            Node* node = actor->FindNodeByName( subMotion->GetName() );
            if (node == nullptr)
                continue;

            // get the node index
            const uint32 nodeIndex = node->GetNodeIndex();

            // now get the original transformation as it was during export time
            // and set it inside the submotion's bind pose transform, so we don't need to lookup the nodes
            // inside the original actor anymore
            subMotion->SetBindPosePos( transformData->GetOrgPos(nodeIndex) );
            //subMotion->SetBindPoseRot( transformData->GetOrgRot(nodeIndex) );

            EMFX_SCALECODE
            (
                //subMotion->SetBindPoseScaleRot( transformData->GetOrgScaleRot(nodeIndex) );
                subMotion->SetBindPoseScale( transformData->GetOrgScale(nodeIndex) );
            )
        }
    }
    */

    void SkeletalMotion::BasicRetarget(const MotionInstance* motionInstance, const SkeletalSubMotion* subMotion, uint32 nodeIndex, Transform& inOutTransform)
    {
        // Special case handling on translation of root nodes.
        // Scale the translation amount based on the height difference between the bind pose height of the 
        // retarget root node and the bind pose of that node stored in the motion.
        // All other nodes get their translation data displaced based on the position difference between the
        // parent relative space positions in the actor instance's bind pose and the motion bind pose.
        const ActorInstance* actorInstance = motionInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        const uint32 retargetRootIndex = actor->GetRetargetRootNodeIndex();
        const Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
        bool needsDisplacement = true;
        if ((retargetRootIndex == nodeIndex || node->GetIsRootNode()) && retargetRootIndex != MCORE_INVALIDINDEX32)
        {
            const MotionLink* retargetRootLink = motionInstance->GetMotionLink(actor->GetRetargetRootNodeIndex());
            if (retargetRootLink->GetIsActive())
            {
                const float subMotionHeight = mSubMotions[retargetRootLink->GetSubMotionIndex()]->GetBindPosePos().GetZ();
                if (fabs(subMotionHeight) >= AZ::g_fltEps)
                {
                    const float heightFactor = bindPose->GetLocalTransform(retargetRootIndex).mPosition.GetZ() / subMotionHeight;
                    inOutTransform.mPosition *= heightFactor;
                    needsDisplacement = false;
                }
            }
        }

        const Transform& bindPoseTransform = bindPose->GetLocalTransform(nodeIndex);
        if (needsDisplacement)
        {
            const AZ::Vector3 displacement = bindPoseTransform.mPosition - subMotion->GetBindPosePos();
            inOutTransform.mPosition += displacement;
        }

        const AZ::Vector3 scaleOffset = bindPoseTransform.mScale - subMotion->GetBindPoseScale();
        inOutTransform.mScale += scaleOffset;
    }


    // the main update method
    void SkeletalMotion::Update(const Pose* inPose, Pose* outPose, MotionInstance* instance)
    {
        ActorInstance*  actorInstance   = instance->GetActorInstance();
        Actor*          actor           = actorInstance->GetActor();
        TransformData*  transformData   = actorInstance->GetTransformData();
        const Pose*     bindPose        = transformData->GetBindPose();

        // make sure the pose has the same number of transforms
        MCORE_ASSERT(outPose->GetNumTransforms() == actor->GetNumNodes());

        // get the time value
        const float timeValue = instance->GetCurrentTime();

        // reset the cache hit counters
        instance->ResetCacheHitCounters();

        // we clear the flags as optimization
        // we will use the direct local transform setting function which doesn't update the global transform flags
        // mark all flags so that it already sees the local transforms as ready
        outPose->ClearFlags(Pose::FLAG_LOCALTRANSFORMREADY);

        // process all motion links
        Transform outTransform;
        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32    nodeNumber   = actorInstance->GetEnabledNode(i);
            MotionLink*     link         = instance->GetMotionLink(nodeNumber);

            // if there is no submotion linked to this node
            if (link->GetIsActive() == false)
            {
                outTransform = inPose->GetLocalTransform(nodeNumber);
                outPose->SetLocalTransformDirect(nodeNumber, outTransform);
                continue;
            }

            // get the current time and the submotion
            SkeletalSubMotion* subMotion = mSubMotions[ link->GetSubMotionIndex() ];

            // get position
            auto* posTrack = subMotion->GetPosTrack();
            if (posTrack)
            {
                outTransform.mPosition = AZ::Vector3(posTrack->GetValueAtTime(timeValue));
            }
            else
            {
                outTransform.mPosition = subMotion->GetPosePos();
            }

            // get rotation
            KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();
            if (rotTrack)
            {
                uint8 wasHit;
                uint32 cachedKeyIndex = instance->GetCachedKey(nodeNumber);
                outTransform.mRotation = rotTrack->GetValueAtTime(timeValue, &cachedKeyIndex, &wasHit);
                instance->SetCachedKey(nodeNumber, cachedKeyIndex);
                instance->ProcessCacheHitResult(wasHit);
            }
            else
            {
                outTransform.mRotation = subMotion->GetPoseRot();
            }


        #ifndef EMFX_SCALE_DISABLED
            // get scale
            auto* scaleTrack = subMotion->GetScaleTrack();
            if (scaleTrack)
            {
                outTransform.mScale = AZ::Vector3(scaleTrack->GetValueAtTime(timeValue));
            }
            else
            {
                outTransform.mScale = subMotion->GetPoseScale();
            }
        #endif

            // perform basic retargeting
            if (instance->GetRetargetingEnabled())
            {
                BasicRetarget(instance, subMotion, nodeNumber, outTransform);
            }

            outPose->SetLocalTransformDirect(nodeNumber, outTransform);
        } // for all transforms

        // mirror
        if (instance->GetMirrorMotion() && actor->GetHasMirrorInfo())
        {
            MirrorPose(outPose, instance);
        }

        // output the morph targets
        MorphSetupInstance* morphSetup = actorInstance->GetMorphSetupInstance();
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            const uint32 morphTargetID = morphSetup->GetMorphTarget(i)->GetID();
            const uint32 subMotionIndex = FindMorphSubMotionByID(morphTargetID);
            if (subMotionIndex != MCORE_INVALIDINDEX32)
            {
                MorphSubMotion* subMotion = mMorphSubMotions[subMotionIndex];
                if (subMotion->GetKeyTrack()->GetNumKeys() > 0)
                {
                    outPose->SetMorphWeight(i, subMotion->GetKeyTrack()->GetValueAtTime(timeValue));
                }
                else
                {
                    outPose->SetMorphWeight(i, subMotion->GetPoseWeight());
                }
            }
            else
            {
                outPose->SetMorphWeight(i, inPose->GetMorphWeight(i));
            }
        }
    }

    //-----------------------------------------------------------
    /*
    // calculate the maximum error values for all of the sub-motions
    void SkeletalMotion::InitMotionLOD(Actor* actor, const float timeStepSize)
    {
        Array<Matrix> localBindPoseMatrices( actor->GetNumNodes() );    // bind pose matrices in local space
        Array<Matrix> bindPoseGlobalMatrices( actor->GetNumNodes() );   // bind pose matrices in global space
        Array<Matrix> localMatrices( actor->GetNumNodes() );            // current local space matrices
        Array<Matrix> globalMatrices( actor->GetNumNodes() );           //
        Array<Transform> localTransforms( actor->GetNumNodes() );   //

        // calculate the local space bind pose matrices
        actor->CalcBindPoseLocalMatrices( localBindPoseMatrices );

        // calculate the bind pose global space transformations using the local space matrices we just calculated
        actor->CalcGlobalMatrices(localBindPoseMatrices, bindPoseGlobalMatrices);

        // for all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 s=0; s<numSubMotions; ++s)
        {
            // find the node number linked to this sub motion
            SkeletalSubMotion* subMotion = mSubMotions[s];
            Node* node = actor->FindNodeByID( subMotion->GetID() );
            if (node == nullptr)    // the node doesn't exist in the skeleton
            {
                subMotion->SetMotionLODMaxError( FLT_MAX ); // set the error so high that it never gets disabled by automatic LOD
                continue;
            }

            // get the node number inside the actor
            const uint32 nodeIndex = node->GetNodeIndex();

            // init the matrices at bind pose matrices
            localMatrices = localBindPoseMatrices;

            // init the local space transforms
            MCore::MemCopy(localTransforms.GetPtr(), actor->GetTransformData()->GetOrgTransforms(), actor->GetNumNodes() * sizeof(Transform));

            float curTime           = 0.0f; // the current motion sampling time value (we sample transformations at this time offset in the motion)
            float maxPosError       = 0.0f; // the maximum position error for this submotion, during the whole animation

            // for all time samples inside the motion
            do
            {
                // calculate the sampled transformation for the current submotion
                localTransforms[nodeIndex].mPosition        = (subMotion->GetPosTrack()    ) ? subMotion->GetPosTrack()->GetValueAtTime( curTime ) : subMotion->GetPosePos();
                localTransforms[nodeIndex].mRotation        = (subMotion->GetRotTrack()    ) ? subMotion->GetRotTrack()->GetValueAtTime( curTime ) : subMotion->GetPoseRot();

                EMFX_SCALECODE
                (
                    localTransforms[nodeIndex].mScale           = (subMotion->GetScaleTrack()  ) ? subMotion->GetScaleTrack()->GetValueAtTime( curTime ) : subMotion->GetPoseScale();
                    localTransforms[nodeIndex].mScaleRotation   = (subMotion->GetScaleRotTrack()!= nullptr) ? subMotion->GetScaleRotTrack()->GetValueAtTime( curTime ) : subMotion->GetPoseScaleRot();
                )

                // calculate a matrix from this transformation and update it inside the local space matrix buffer
                actor->CalcLocalSpaceMatrix(nodeIndex, localTransforms.GetReadPtr(), &localMatrices[nodeIndex]);

                // calculate the transforms of this node and all child nodes, using our calculated local space matrices
                actor->CalcGlobalMatrices( localMatrices, globalMatrices ); // TODO: optimization would be to instead of calculating ALL global matrices, process only the ones needed

                // compute the maximum error in units of this + all child nodes compared to the bind pose (or motion pose maybe)
                // for all nodes from this towards all child nodes
                float posError = 0.0f;
                CalcMaxMotionLODError(nodeIndex, globalMatrices, bindPoseGlobalMatrices, actor, posError);
                maxPosError = MCore::Max<float>(posError, maxPosError);

                // increase the current sampling time
                curTime += timeStepSize;
            }
            while (curTime < GetMaxTime());

            // if there is a positional error, use that
            float maxSubMotionError = 0.0f;
            if (maxPosError > 0.001f)
                maxSubMotionError = maxPosError;
            else
            {
                // if the positions of this node or its child nodes didn't change, but we are still rotating or scaling, use the maximum error for this submotion
            #ifndef EMFX_SCALE_DISABLED
                if ((subMotion->GetRotTrack()) || (subMotion->GetScaleTrack()))
            #else
                if (subMotion->GetRotTrack())
            #endif
                {
                    //LogInfo( String("************ ") + actor->GetNode(nodeIndex)->GetName() );
                    maxSubMotionError = -1.0f;  // the -1 will be replaced by the maximum of all submotion errors later on in UpdateMotionLODMaxError().
                }
            }

            // store the maximum error in the sub motion
            mSubMotions[s]->SetMotionLODMaxError( maxSubMotionError );
            //LogInfo("Node '%s' has a maximum error of '%f' units", node->GetName(), maxSubMotionError);
        }

        // update the maximum motion LOD error
        UpdateMotionLODMaxError();
    }


    // recursive calc the max error
    void SkeletalMotion::CalcMaxMotionLODError(const uint32 nodeIndex, const MCore::Array<MCore::Matrix>& globalMatrices, const MCore::Array<MCore::Matrix>& bindPoseGlobalMatrices, Actor* actor, float& curMaxError) const
    {
        // calculate the global space distance between the bind pose and the current pose
        const float distance = (globalMatrices[nodeIndex].GetTranslation() - bindPoseGlobalMatrices[nodeIndex].GetTranslation()).SafeLength();
        curMaxError = MCore::Max<float>(curMaxError, distance);

        // process all child nodes
        Node* node = actor->GetNode(nodeIndex);
        for (uint32 i=0; i<node->GetNumChildNodes(); ++i)
            CalcMaxMotionLODError(node->GetChildIndex(i), globalMatrices, bindPoseGlobalMatrices, actor, curMaxError);
    }


    // get the maximum error from all sub-motions
    void SkeletalMotion::UpdateMotionLODMaxError()
    {
        float maxError = 0.0f;
        const float ignoreThreshold = FLT_MAX - 0.00001f;
        bool onlyNegative = true;
        bool hasNegative = false;

        // for all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 s=0; s<numSubMotions; ++s)
        {
            // don't include max float values
            if (mSubMotions[s]->GetMotionLODMaxError() < ignoreThreshold) // dont include max floats
                maxError = MCore::Max<float>(mSubMotions[s]->GetMotionLODMaxError(), maxError); // get the maximum

            // check if we got negative max error values
            if (mSubMotions[s]->GetMotionLODMaxError() >= 0.0f)
                onlyNegative = false;

            if (mSubMotions[s]->GetMotionLODMaxError() < 0.0f)
                hasNegative = true;
        }

        // use 100000 as max error if there are special case nodes and if they are most likely the only ones
        if (onlyNegative)
            maxError = 100000.0f;
        else
        {
            // if there are special case nodes and most likely all other nodes do not animate, use a high error value
            if (hasNegative && maxError <= 0.0001f)
                maxError = 100000.0f;
        }

        // set the motion lod max error for this motion
        SetMotionLODMaxError( maxError );

        // now replace all negative values with the max values
        for (uint32 i=0; i<numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetMotionLODMaxError() < 0.0f) // dont include max floats
                mSubMotions[i]->SetMotionLODMaxError( maxError );
        }

        // set the motion lod max acceptable error to the maximum error
        SetMotionLODMaxAcceptableError( maxError );
    }


    // calculate motion lod statistics for this motion with specified error settings
    void SkeletalMotion::CalcMotionLODStats(Actor* actor, const float maxError, const float blendStartFactor, const bool useMaxAcceptableError, MotionLODStats* outStats)
    {
        // find the real maximum error
        float realMaxError = maxError;
        if (useMaxAcceptableError)
            if (maxError > GetMotionLODMaxAcceptableError())
                realMaxError = GetMotionLODMaxAcceptableError();

        // for all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 s=0; s<numSubMotions; ++s)
        {
            // find the node number linked to this sub motion
            SkeletalSubMotion* subMotion = mSubMotions[s];
            Node* node = actor->FindNodeByID( subMotion->GetID() );
            if (node == nullptr)    // the node doesn't exist in the skeleton
                continue;

            // increase the number of nodes touched by this motion
            outStats->mNumNodes++;

            // get the node importance factor
            const float importanceFactor = node->GetImportanceFactor();

            // check if there are animation tracks
        #ifndef EMFX_SCALE_DISABLED
            if (subMotion->GetPosTrack() || subMotion->GetRotTrack() || subMotion->GetScaleRotTrack() || subMotion->GetScaleTrack())
        #else
            if (subMotion->GetPosTrack() || subMotion->GetRotTrack())
        #endif
            {
                // if this motion is disabled, we saved some track sampling (which is relatively slow)
                if (subMotion->IsDisabledInMotionLOD(realMaxError, importanceFactor, blendStartFactor))
                    outStats->mNumSavedSamples++;

                // increase the number of nodes affected by this motion that also have key tracks (so that contain motion data)
                outStats->mNumAnimated++;
            }

            // if this node is disabled in this LOD
            if (subMotion->IsDisabledInMotionLOD(realMaxError, importanceFactor, blendStartFactor))
                outStats->mNumDisabled++;
            else
            if (subMotion->IsFullyEnabledInMotionLOD(realMaxError, importanceFactor, blendStartFactor)) // if this node is fully enabled (so not blending)
                outStats->mNumFullyEnabled++;
            else
            {
                // if this node is blending
                MCORE_ASSERT( subMotion->IsBlendingMotionLOD(realMaxError, importanceFactor, blendStartFactor) );       // make sure its really blending in the API too
                outStats->mNumBlending++;
            }
        }

        // calculate percentages
        outStats->mNumSavedSamplesPercentage = (outStats->mNumAnimated > 0) ? (outStats->mNumSavedSamples / (float)outStats->mNumAnimated) * 100.0f : 0.0f;
        outStats->mNumDisabledPercentage     = (outStats->mNumNodes > 0) ? (outStats->mNumDisabled / (float)outStats->mNumNodes) * 100.0f : 0.0f;
    }
    */

    // are the bind pose transformations of the given actor matching the one from the current skeletal motion?
    bool SkeletalMotion::CheckIfIsMatchingActorBindPose(Actor* actor)
    {
        bool bindPosesMatching = true;
        //TransformData* transformData = actor->GetTransformData();
        //  const Transform* actorBindTransforms = actor->GetBindPoseLocalTransforms();
        const Pose& bindPose = *actor->GetBindPose();

        Skeleton* skeleton = actor->GetSkeleton();

        // get number of submotions and iterate through them
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 s = 0; s < numSubMotions; ++s)
        {
            // find the node number linked to this sub motion
            SkeletalSubMotion*  subMotion   = mSubMotions[ s ];
            const uint32        nodeID      = subMotion->GetID();
            Node*               node        = skeleton->FindNodeByID(nodeID);

            // check if there actually is a node for the current submotion
            if (node == nullptr)
            {
                continue; // TODO: will motion mirroring still work if that happens? log a warning as well?
            }
            // get the node number of the current node
            const uint32 nodeNr = node->GetNodeIndex();

            const Transform& bindTransform = bindPose.GetLocalTransform(nodeNr);

            // compare the bind pose translation of the node with the one from the submotion
            AZ::Vector3 nodeTranslation         = bindTransform.mPosition;
            AZ::Vector3 submotionTranslation    = subMotion->GetBindPosePos();
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(submotionTranslation, nodeTranslation, MCore::Math::epsilon) == false)
            {
                MCore::LogWarning("Bind pose translation of the node '%s' (%f, %f, %f) does not match the skeletal submotion bind pose translation (%f, %f, %f).",
                    node->GetName(),
                    static_cast<float>(nodeTranslation.GetX()), static_cast<float>(nodeTranslation.GetY()), static_cast<float>(nodeTranslation.GetZ()),
                    static_cast<float>(submotionTranslation.GetX()), static_cast<float>(submotionTranslation.GetY()), static_cast<float>(submotionTranslation.GetZ()));
                bindPosesMatching = false;
            }

            // compare the bind pose rotation of the node with the one from the submotion
            MCore::Quaternion nodeRotation      = bindTransform.mRotation;
            MCore::Quaternion submotionRotation = subMotion->GetBindPoseRot();
            if (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(submotionRotation, nodeRotation, MCore::Math::epsilon) == false)
            {
                MCore::LogWarning("Bind pose rotation of the node '%s' (%f, %f, %f, %f) does not match the skeletal submotion bind pose rotation (%f, %f, %f, %f).", node->GetName(), nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w, submotionRotation.x, submotionRotation.y, submotionRotation.z, submotionRotation.w);
                bindPosesMatching = false;
            }

            EMFX_SCALECODE
            (
                // compare the bind pose scale of the node with the one from the submotion
                AZ::Vector3 nodeScale        = bindTransform.mScale;
                AZ::Vector3 submotionScale   = subMotion->GetBindPoseScale();
                if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(submotionScale, nodeScale, MCore::Math::epsilon) == false)
                {
                    MCore::LogWarning("Bind pose scale of the node '%s' (%f, %f, %f) does not match the skeletal submotion bind pose scale (%f, %f, %f).",
                        node->GetName(),
                        static_cast<float>(nodeScale.GetX()), static_cast<float>(nodeScale.GetY()), static_cast<float>(nodeScale.GetZ()),
                        static_cast<float>(submotionScale.GetX()), static_cast<float>(submotionScale.GetY()), static_cast<float>(submotionScale.GetZ()));
                    bindPosesMatching = false;
                }
                /*
                            // compare the bind pose scale rotation of the node with the one from the submotion
                            MCore::Quaternion nodeScaleRot      = bindTransform.mScaleRotation;
                            MCore::Quaternion submotionScaleRot = subMotion->GetBindPoseScaleRot();
                            if (Compare<MCore::Quaternion>::CheckIfIsClose( submotionScaleRot, nodeScaleRot, Math::epsilon ) == false)
                            {
                                LogWarning("Bind pose scale rotation of the node '%s' (%f, %f, %f, %f) does not match the skeletal submotion bind pose scale rotation (%f, %f, %f, %f).", node->GetName(), nodeScaleRot.x, nodeScaleRot.y, nodeScaleRot.z, nodeScaleRot.w, submotionScaleRot.x, submotionScaleRot.y, submotionScaleRot.z, submotionScaleRot.w);
                                bindPosesMatching = false;
                            }*/
            ) // EMFX_SCALECODE
        }

        return bindPosesMatching;
    }


    // output the skeleton of an actor, using the bind pose of this motion
    void SkeletalMotion::CalcMotionBindPose(Actor* actor, MCore::Array<MCore::Matrix>& localMatrixBuffer, MCore::Array<MCore::Matrix>& outMatrices)
    {
        // make sure the size of the local matrix buffer is big enough
        const uint32 numNodes = actor->GetNumNodes();
        localMatrixBuffer.Resize(numNodes);

        // get the local transforms of the bind pose of the actor
        const Pose& actorBindPose = *actor->GetBindPose();
        const Transform* localTransforms = actorBindPose.GetLocalTransforms();

        Skeleton* skeleton = actor->GetSkeleton();

        // for all nodes in the actor
        Transform localTransform;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // check if we have a skeletal sub motion for this node
            SkeletalSubMotion* subMotion = FindSubMotionByID(node->GetID());
            if (subMotion) // if there is a submotion, build the local transform for it
            {
                // init the local transform to the motion bind pose transform
                localTransform.mPosition        = subMotion->GetBindPosePos();
                localTransform.mRotation        = subMotion->GetBindPoseRot();

                EMFX_SCALECODE
                (
                    localTransform.mScale           = subMotion->GetBindPoseScale();
                    //localTransform.mScaleRotation = subMotion->GetBindPoseScaleRot();
                )

                // calculate the local space matrix from that
                skeleton->CalcLocalSpaceMatrix(i, localTransforms, &localMatrixBuffer[i]);
            }
            else // there is no submotion, use the bind pose of the actor node instead
            {
                skeleton->CalcLocalSpaceMatrix(i, localTransforms, &localMatrixBuffer[i]);
            }
        }

        // now that we have the local space matrices, we can calculate the global space ones from those
        skeleton->CalcGlobalMatrices(localMatrixBuffer, outMatrices);
    }


    // remove a given sub-motions
    void SkeletalMotion::RemoveMorphSubMotion(uint32 nr, bool delFromMem)
    {
        if (delFromMem)
        {
            mMorphSubMotions[nr]->Destroy();
        }

        mMorphSubMotions.Remove(nr);
    }


    // find a submotion with a given id
    uint32 SkeletalMotion::FindMorphSubMotionByID(uint32 id) const
    {
        // get the number of submotions and iterate through them
        const uint32 numSubMotions = mMorphSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mMorphSubMotions[i]->GetID() == id)
            {
                return i;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    // find a submotion with a given phoneme set
    uint32 SkeletalMotion::FindMorphSubMotionByPhoneme(MorphTarget::EPhonemeSet phonemeSet) const
    {
        // check if the given phoneme set is a valid one and return if not
        if (phonemeSet == MorphTarget::PHONEMESET_NONE)
        {
            return MCORE_INVALIDINDEX32;
        }

        // get the number of submotions and iterate through them
        const uint32 numSubMotions = mMorphSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mMorphSubMotions[i]->GetPhonemeSet() == phonemeSet)
            {
                return i;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    // mirror a given pose for a given motion instance
    void SkeletalMotion::MirrorPose(Pose* inOutPose, MotionInstance* instance)
    {
        ActorInstance* actorInstance = instance->GetActorInstance();
        Actor* actor = actorInstance->GetActor();
        TransformData* transformData = actorInstance->GetTransformData();
        const Pose* bindPose = transformData->GetBindPose();

        AnimGraphPose* tempPose = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool().RequestPose(actorInstance);
        Pose& unmirroredPose = tempPose->GetPose();
        unmirroredPose = *inOutPose;

        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32    nodeNumber   = actorInstance->GetEnabledNode(i);
            MotionLink*     link         = instance->GetMotionLink(nodeNumber);

            if (link->GetIsActive() == false)
            {
                continue;
            }

            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeNumber);

            Transform mirrored = bindPose->GetLocalTransform(nodeNumber);
            AZ::Vector3 mirrorAxis(0.0f, 0.0f, 0.0f);
            mirrorAxis.SetElement(mirrorInfo.mAxis, 1.0f);
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalTransform(mirrorInfo.mSourceNode), unmirroredPose.GetLocalTransform(mirrorInfo.mSourceNode), mirrorAxis, mirrorInfo.mFlags);

            inOutPose->SetLocalTransformDirect(nodeNumber, mirrored);
        }

        GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool().FreePose(tempPose);
    }


    void SkeletalMotion::AddMorphSubMotion(MorphSubMotion* subMotion)
    {
        mMorphSubMotions.Add(subMotion);
    }


    void SkeletalMotion::SetNumMorphSubMotions(uint32 numSubMotions)
    {
        mMorphSubMotions.Resize(numSubMotions);
    }


    void SkeletalMotion::SetMorphSubMotion(uint32 index, MorphSubMotion* subMotion)
    {
        mMorphSubMotions[index] = subMotion;
    }


    bool SkeletalMotion::GetDoesInfluencePhonemes() const
    {
        return mDoesInfluencePhonemes;
    }


    void SkeletalMotion::SetDoesInfluencePhonemes(bool enabled)
    {
        mDoesInfluencePhonemes = enabled;
    }


    void SkeletalMotion::SetSubMotion(uint32 index, SkeletalSubMotion* subMotion)
    {
        mSubMotions[index] = subMotion;
    }


    bool SkeletalMotion::SetNumSubMotions(uint32 numSubMotions)
    {
        return mSubMotions.Resize(numSubMotions);
    }


    void SkeletalMotion::AddSubMotion(SkeletalSubMotion* part)
    {
        mSubMotions.Add(part);
    }


    SkeletalSubMotion* SkeletalMotion::FindSubMotionByName(const char* name) const
    {
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetNameString() == name)
            {
                return mSubMotions[i];
            }
        }

        return nullptr;
    }


    SkeletalSubMotion* SkeletalMotion::FindSubMotionByID(uint32 id) const
    {
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetID() == id)
            {
                return mSubMotions[i];
            }
        }

        return nullptr;
    }


    uint32 SkeletalMotion::FindSubMotionIndexByName(const char* name) const
    {
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find submotion index by id
    uint32 SkeletalMotion::FindSubMotionIndexByID(uint32 id) const
    {
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetID() == id)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // scale all data
    void SkeletalMotion::Scale(float scaleFactor)
    {
        // if we don't need to adjust the scale, do nothing
        if (MCore::Math::IsFloatEqual(scaleFactor, 1.0f))
        {
            return;
        }

        // modify all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SkeletalSubMotion* subMotion = mSubMotions[i];
            subMotion->SetBindPosePos(subMotion->GetBindPosePos() * scaleFactor);
            subMotion->SetPosePos(subMotion->GetPosePos() * scaleFactor);

            auto posTrack = subMotion->GetPosTrack();
            if (subMotion->GetPosTrack())
            {
                const uint32 numKeys = posTrack->GetNumKeys();
                for (uint32 k = 0; k < numKeys; ++k)
                {
                    auto key = posTrack->GetKey(k);
                    key->SetValue(AZ::PackedVector3f(AZ::Vector3(key->GetValue()) * scaleFactor));
                }
            }
        }

        // trigger the event
        GetEventManager().OnScaleMotionData(this, scaleFactor);
    }
} // namespace EMotionFX
