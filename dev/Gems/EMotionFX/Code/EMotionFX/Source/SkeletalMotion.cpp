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
    // constructor
    SkeletalMotion::SkeletalMotion(const char* name)
        : Motion(name)
        , mDoesInfluencePhonemes(false)
    {
        GetEMotionFX().GetEventManager()->AddEventHandler(&m_subMotionIndexCache);
    }


    // destructor
    SkeletalMotion::~SkeletalMotion()
    {
        GetEMotionFX().GetEventManager()->RemoveEventHandler(&m_subMotionIndexCache);

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
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
            subMotion->Destroy();
        }
        mSubMotions.clear();

        // delete all morph sub motions
        for (MorphSubMotion* subMotion : mMorphSubMotions)
        {
            subMotion->Destroy();
        }
        mMorphSubMotions.clear();
    }


    // calculate the node transform
    void SkeletalMotion::CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting)
    {
        // get some required shortcuts
        ActorInstance*  actorInstance = instance->GetActorInstance();
        TransformData*  transformData = actorInstance->GetTransformData();

        const Pose* bindPose = transformData->GetBindPose();

        // get the node index/number
        const uint32 nodeIndex = node->GetNodeIndex();

        // search for the motion link
        const MotionLink* motionLink = instance->GetMotionLink(nodeIndex);
        if (motionLink->GetIsActive() == false)
        {
            *outTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex);
            return;
        }

        const Skeleton* skeleton = actor->GetSkeleton();
        const bool inPlace = instance->GetIsInPlace();

        SkeletalSubMotion* subMotion = mSubMotions[motionLink->GetSubMotionIndex()];
        if (!(inPlace && skeleton->GetNode(nodeIndex)->GetIsRootNode()))
        {
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
        }
        else
        {
            *outTransform = actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(nodeIndex);
        }

        // if we want to use retargeting
        if (enableRetargeting)
        {
            BasicRetarget(instance, subMotion, nodeIndex, *outTransform);
        }

        // mirror
        if (instance->GetMirrorMotion() && actor->GetHasMirrorInfo())
        {
            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeIndex);
            Transform mirrored = bindPose->GetLocalSpaceTransform(nodeIndex);
            AZ::Vector3 mirrorAxis(0.0f, 0.0f, 0.0f);
            mirrorAxis.SetElement(mirrorInfo.mAxis, 1.0f);
            const uint32 motionSource = actor->GetNodeMirrorInfo(nodeIndex).mSourceNode;
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(motionSource), *outTransform, mirrorAxis, mirrorInfo.mFlags);
            *outTransform = mirrored;
        }
    }


    // creates the  motion links in the nodes of the given actor
    void SkeletalMotion::CreateMotionLinks(Actor* actor, MotionInstance* instance)
    {
        m_subMotionIndexCache.CreateMotionLinks(actor, *this, instance);
    }


    // get the maximum time value and update the stored one
    void SkeletalMotion::UpdateMaxTime()
    {
        mMaxTime = 0.0f;

        // for all sub motions
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
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
        for (MorphSubMotion* subMotion : mMorphSubMotions)
        {
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
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
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
        for (MorphSubMotion* subMotion : mMorphSubMotions)
        {
            subMotion->GetKeyTrack()->MakeLoopable(fadeTime);
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
                    const float heightFactor = bindPose->GetLocalSpaceTransform(retargetRootIndex).mPosition.GetZ() / subMotionHeight;
                    inOutTransform.mPosition *= heightFactor;
                    needsDisplacement = false;
                }
            }
        }

        const Transform& bindPoseTransform = bindPose->GetLocalSpaceTransform(nodeIndex);
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

        const Skeleton* skeleton = actor->GetSkeleton();
        const bool inPlace = instance->GetIsInPlace();

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
                outTransform = inPose->GetLocalSpaceTransform(nodeNumber);
                outPose->SetLocalSpaceTransformDirect(nodeNumber, outTransform);
                continue;
            }

            SkeletalSubMotion* subMotion = mSubMotions[ link->GetSubMotionIndex() ];
            if (!(inPlace && skeleton->GetNode(nodeNumber)->GetIsRootNode()))
            {
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
            }
            else // we are in place
            {
                outTransform = actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(nodeNumber);
            }

            // perform basic retargeting
            if (instance->GetRetargetingEnabled())
            {
                BasicRetarget(instance, subMotion, nodeNumber, outTransform);
            }

            outPose->SetLocalSpaceTransformDirect(nodeNumber, outTransform);
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


    // are the bind pose transformations of the given actor matching the one from the current skeletal motion?
    bool SkeletalMotion::CheckIfIsMatchingActorBindPose(Actor* actor)
    {
        bool bindPosesMatching = true;
        const Pose& bindPose = *actor->GetBindPose();

        Skeleton* skeleton = actor->GetSkeleton();

        // get number of submotions and iterate through them
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
            // find the node number linked to this sub motion
            const uint32        nodeID      = subMotion->GetID();
            Node*               node        = skeleton->FindNodeByID(nodeID);

            // check if there actually is a node for the current submotion
            if (node == nullptr)
            {
                continue; // TODO: will motion mirroring still work if that happens? log a warning as well?
            }
            // get the node number of the current node
            const uint32 nodeNr = node->GetNodeIndex();

            const Transform& bindTransform = bindPose.GetLocalSpaceTransform(nodeNr);

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
    void SkeletalMotion::CalcMotionBindPose(Actor* actor, Pose* outPose)
    {
        outPose->InitFromBindPose(actor);

        Skeleton* skeleton = actor->GetSkeleton();

        // for all nodes in the actor
        Transform localTransform;
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // check if we have a skeletal sub motion for this node
            SkeletalSubMotion* subMotion = FindSubMotionByID(node->GetID());
            if (subMotion) // if there is a submotion, build the local transform for it
            {
                // init the local transform to the motion bind pose transform
                localTransform.mPosition = subMotion->GetBindPosePos();
                localTransform.mRotation = subMotion->GetBindPoseRot();

                EMFX_SCALECODE
                (
                    localTransform.mScale  = subMotion->GetBindPoseScale();
                    //localTransform.mScaleRotation = subMotion->GetBindPoseScaleRot();
                )

                outPose->SetLocalSpaceTransform(i, localTransform);
            }
        }
    }


    // remove a given sub-motions
    void SkeletalMotion::RemoveMorphSubMotion(uint32 nr, bool delFromMem)
    {
        if (delFromMem)
        {
            mMorphSubMotions[nr]->Destroy();
        }

        mMorphSubMotions.erase(mMorphSubMotions.begin() + nr);
    }


    // find a submotion with a given id
    uint32 SkeletalMotion::FindMorphSubMotionByID(uint32 id) const
    {
        // get the number of submotions and iterate through them
        const size_t numSubMotions = mMorphSubMotions.size();
        for (size_t i = 0; i < numSubMotions; ++i)
        {
            if (mMorphSubMotions[i]->GetID() == id)
            {
                return static_cast<uint32>(i);
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
        const size_t numSubMotions = mMorphSubMotions.size();
        for (size_t i = 0; i < numSubMotions; ++i)
        {
            if (mMorphSubMotions[i]->GetPhonemeSet() == phonemeSet)
            {
                return static_cast<uint32>(i);
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

            Transform mirrored = bindPose->GetLocalSpaceTransform(nodeNumber);
            AZ::Vector3 mirrorAxis(0.0f, 0.0f, 0.0f);
            mirrorAxis.SetElement(mirrorInfo.mAxis, 1.0f);
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(mirrorInfo.mSourceNode), unmirroredPose.GetLocalSpaceTransform(mirrorInfo.mSourceNode), mirrorAxis, mirrorInfo.mFlags);

            inOutPose->SetLocalSpaceTransformDirect(nodeNumber, mirrored);
        }

        GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool().FreePose(tempPose);
    }


    void SkeletalMotion::AddMorphSubMotion(MorphSubMotion* subMotion)
    {
        mMorphSubMotions.emplace_back(subMotion);
    }


    void SkeletalMotion::SetNumMorphSubMotions(uint32 numSubMotions)
    {
        mMorphSubMotions.resize_no_construct(numSubMotions);
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
        mSubMotions.resize_no_construct(numSubMotions);
        return true;
    }


    void SkeletalMotion::AddSubMotion(SkeletalSubMotion* part)
    {
        mSubMotions.emplace_back(part);
    }


    SkeletalSubMotion* SkeletalMotion::FindSubMotionByName(const char* name) const
    {
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
            if (subMotion->GetNameString() == name)
            {
                return subMotion;
            }
        }

        return nullptr;
    }


    SkeletalSubMotion* SkeletalMotion::FindSubMotionByID(uint32 id) const
    {
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
            if (subMotion->GetID() == id)
            {
                return subMotion;
            }
        }

        return nullptr;
    }


    uint32 SkeletalMotion::FindSubMotionIndexByName(const char* name) const
    {
        const size_t numSubMotions = mSubMotions.size();
        for (size_t i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetNameString() == name)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find submotion index by id
    uint32 SkeletalMotion::FindSubMotionIndexByID(uint32 id) const
    {
        const size_t numSubMotions = mSubMotions.size();
        for (size_t i = 0; i < numSubMotions; ++i)
        {
            if (mSubMotions[i]->GetID() == id)
            {
                return static_cast<uint32>(i);
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
        for (SkeletalSubMotion* subMotion : mSubMotions)
        {
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


    void SkeletalMotion::SubMotionIndexCache::CreateMotionLinks(Actor* actor, SkeletalMotion& skeletalMotion, MotionInstance* instance)
    {
        AZStd::shared_lock<AZStd::shared_mutex> sharedLock(m_mutex);

        auto it = m_subMotionIndexByActorNode.find(actor);
        if (it == m_subMotionIndexByActorNode.end())
        {
            m_mutex.unlock_shared(); // we need an exclusive lock to modify the container
            {
                AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex); // exclusive lock
                // Check that another thread didn't already insert the entry
                it = m_subMotionIndexByActorNode.find(actor);
                if (it == m_subMotionIndexByActorNode.end())
                {
                    const auto itInserted = m_subMotionIndexByActorNode.emplace(actor, IndexVector());
                    AZ_Assert(itInserted.second, "Expected successful insert");
                    it = itInserted.first;
                    IndexVector& indexVector = it->second;
                    Skeleton* skeleton = actor->GetSkeleton();

                    const uint32 numNodes = actor->GetNumNodes();
                    indexVector.resize_no_construct(numNodes);
                    for (uint32 i = 0; i < numNodes; ++i)
                    {
                        indexVector[i] = skeletalMotion.FindSubMotionIndexByID(skeleton->GetNode(i)->GetID());
                    }
                }
            }
            m_mutex.lock_shared(); // restore the shared lock
        }

        const IndexVector& indexVector = it->second;

        // if no start node has been set
        if (instance->GetStartNodeIndex() == MCORE_INVALIDINDEX32)
        {
            // fill it up with all the data
            const uint32 numNodes = actor->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                const uint32 subMotionIndex = indexVector[i];
                if (subMotionIndex != MCORE_INVALIDINDEX32) // if a submotion has been found
                {
                    instance->GetMotionLink(i)->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
                }
            }
        }
        else // if we want to use a start node
        {
            RecursiveAddIndices(indexVector, actor, instance->GetStartNodeIndex(), instance);
        }
    }

    void SkeletalMotion::SubMotionIndexCache::OnDeleteActor(Actor* actor)
    {
        AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex);
        m_subMotionIndexByActorNode.erase(actor);
    }

    void SkeletalMotion::SubMotionIndexCache::RecursiveAddIndices(const IndexVector& indicesVector, Actor* actor, uint32 nodeIndex, MotionInstance* instance)
    {
        Node* node = actor->GetSkeleton()->GetNode(nodeIndex);

        // if mirroring is disabled or there is no node motion source array (no node remapping available)
        if (instance->GetMirrorMotion() == false || actor->GetHasMirrorInfo() == false || node->GetIsAttachmentNode())
        {
            // try to find the submotion for this node
            const uint32 subMotionIndex = indicesVector[nodeIndex];
            if (subMotionIndex != MCORE_INVALIDINDEX32) // if it exists, set the motion link and activate it
            {
                instance->GetMotionLink(nodeIndex)->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
            }

            // process all child nodes
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RecursiveAddIndices(indicesVector, actor, node->GetChildIndex(i), instance);
            }
        }
        else
        {
            // get the source node to get the anim data from
            uint32 sourceNodeIndex = actor->GetNodeMirrorInfo(nodeIndex).mSourceNode;
            if (sourceNodeIndex == MCORE_INVALIDINDEX16)
            {
                sourceNodeIndex = nodeIndex;
            }

            // try to find the submotion for this node
            const uint32 subMotionIndex = indicesVector[sourceNodeIndex];
            if (subMotionIndex != MCORE_INVALIDINDEX32) // if it exists, set the motion link and activate it
            {
                instance->GetMotionLink(nodeIndex)->SetSubMotionIndex(static_cast<uint16>(subMotionIndex));
            }

            // process all child nodes
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RecursiveAddIndices(indicesVector, actor, node->GetChildIndex(i), instance);
            }
        }
    }
} // namespace EMotionFX
