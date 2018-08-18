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

#include "EMotionFXConfig.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/IDGenerator.h>

#include "Actor.h"
#include "ActorInstance.h"
#include "MotionLayerSystem.h"
#include "Attachment.h"
#include "ActorManager.h"
#include "Mesh.h"
#include "MeshDeformerStack.h"
#include "MorphMeshDeformer.h"
#include "TransformData.h"
#include "EventManager.h"
#include "EyeBlinker.h"
#include "Recorder.h"
#include "AnimGraphInstance.h"
#include "AttachmentNode.h"
#include "AttachmentSkin.h"
#include "NodeGroup.h"
#include "ActorUpdateScheduler.h"
#include "MorphSetup.h"
#include "Node.h"



namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorInstance, ActorInstanceAllocator, 0)

    // the constructor
    ActorInstance::ActorInstance(Actor* actor, AZ::EntityId entityId, uint32 threadIndex)
        : BaseObject()
        , m_entityId(entityId)
    {
        MCORE_ASSERT(actor);

        // set the memory categories
        mAttachments.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORINSTANCES);
        mDependencies.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORINSTANCES);
        mEnabledNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORINSTANCES);
        mEnabledNodes.Reserve(actor->GetNumNodes());

        // set the actor and create the motion system
        mBoolFlags              = 0;
        mActor                  = actor;
        mLODLevel               = 0;
        mNumAttachmentRefs      = 0;
        mThreadIndex            = threadIndex;
        mAttachedTo             = nullptr;
        mSelfAttachment         = nullptr;
        mCustomData             = nullptr;
        mID                     = MCore::GetIDGenerator().GenerateID();
        mEyeBlinker             = nullptr;
        mVisualizeScale         = 1.0f;
        mMotionSamplingRate     = 0.0f;
        mMotionSamplingTimer    = 0.0f;

        mTrajectoryDelta.ZeroWithIdentityQuaternion();
        mStaticAABB.Init();

        mAnimGraphInstance     = nullptr;

        // set the boolean defaults
        SetFlag(BOOL_ISVISIBLE, true);
        SetFlag(BOOL_BOUNDSUPDATEENABLED, true);
        SetFlag(BOOL_NORMALIZEDMOTIONLOD, true);
        SetFlag(BOOL_RENDER, true);
        SetFlag(BOOL_USEDFORVISUALIZATION, false);
        SetFlag(BOOL_ENABLED, true);
        SetFlag(BOOL_MOTIONEXTRACTION, true);

#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_OWNEDBYRUNTIME, false);
#endif // EMFX_DEVELOPMENT_BUILD

        // enable all nodes on default
        EnableAllNodes();

        // apply actor node group default states (disable groups of nodes that are disabled on default)
        const uint32 numGroups = mActor->GetNumNodeGroups();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mActor->GetNodeGroup(i)->GetIsEnabledOnDefault() == false) // if this group is disabled on default
            {
                mActor->GetNodeGroup(i)->DisableNodes(this); // disable all nodes inside this group
            }
        }
        // disable nodes that are disabled in LOD 0
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            if (skeleton->GetNode(n)->GetSkeletalLODStatus(0) == false)
            {
                DisableNode(static_cast<uint16>(n));
            }
        }

        // setup auto bounds update (it is enabled on default)
        mBoundsUpdateFrequency  = 0.0f;
        mBoundsUpdatePassedTime = 0.0f;
        mBoundsUpdateType       = BOUNDS_STATIC_BASED;
        mBoundsUpdateItemFreq   = 1;

        // initialize the actor local and global transform
        mParentGlobalTransform.Identity();
        mLocalTransform.Identity();
        mGlobalTransform.Identity();

        // init the morph setup instance
        mMorphSetup = MorphSetupInstance::Create();
        mMorphSetup->Init(actor->GetMorphSetup(0));

        // initialize the transformation data of this instance
        mTransformData = TransformData::Create();
        mTransformData->InitForActorInstance(this);

        // create the motion system
        mMotionSystem = MotionLayerSystem::Create(this);

        // update the global and local matrices
        UpdateTransformations(0.0f);

        // update the actor dependencies
        UpdateDependencies();

        // update the static based AABB dimensions
        mStaticAABB = mActor->GetStaticAABB();
        if (mStaticAABB.CheckIfIsValid() == false)
        {
            UpdateMeshDeformers(0.0f, true);      // TODO: not really thread safe because of shared meshes, although it probably will output correctly
            UpdateStaticBasedAABBDimensions();
        }

        // update the bounds
        UpdateBounds(0, mBoundsUpdateType, 1);

        // register it
        GetActorManager().RegisterActorInstance(this);

        // automatically register the actor instance
        GetEventManager().OnCreateActorInstance(this);

        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(this);
    }


    // the destructor
    ActorInstance::~ActorInstance()
    {
        // trigger the OnDeleteActorInstance event
        GetEventManager().OnDeleteActorInstance(this);

        // remove it from the recording
        GetRecorder().RemoveActorInstanceFromRecording(this);
          
        // get rid of the motion system
        if (mMotionSystem)
        {
            mMotionSystem->Destroy();
        }

        if (mAnimGraphInstance)
        {
            mAnimGraphInstance->Destroy();
        }

        if (mEyeBlinker)
        {
            mEyeBlinker->Destroy();
        }

        // delete all attachments
        // actor instances that are attached will be detached, and not deleted from memory
        const uint32 numAttachments = mAttachments.GetLength();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            ActorInstance* attachmentActorInstance = mAttachments[i]->GetAttachmentActorInstance();
            if (attachmentActorInstance)
            {
                attachmentActorInstance->SetAttachedTo(nullptr);
                attachmentActorInstance->SetSelfAttachment(nullptr);
                attachmentActorInstance->DecreaseNumAttachmentRefs();
                GetActorManager().UpdateActorInstanceStatus(attachmentActorInstance);
            }
            mAttachments[i]->Destroy();
        }
        mAttachments.Clear();

        if (mMorphSetup)
        {
            mMorphSetup->Destroy();
        }

        if (mTransformData)
        {
            mTransformData->Destroy();
        }

        // remove the attachment from the actor instance where it is attached to
        if (GetIsAttachment())
        {
            mAttachedTo->RemoveAttachment(this /*, false*/);
        }

        // automatically unregister the actor instance
        GetActorManager().UnregisterActorInstance(this);
    }


    // create an actor instance
    ActorInstance* ActorInstance::Create(Actor* actor, AZ::EntityId entityId, uint32 threadIndex)
    {
        return aznew ActorInstance(actor, entityId, threadIndex);
    }


    // update the transformation data
    void ActorInstance::UpdateTransformations(float timePassedInSeconds, bool updateMatrices, bool sampleMotions)
    {
        const Recorder& recorder = GetRecorder();
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // if we are using the recorder to playback
        if (recorder.GetIsInPlayMode() && recorder.GetHasRecorded(this))
        {
            // output the anim graph instance, this doesn't overwrite transforms, just some things internally
            if (recorder.GetRecordSettings().mRecordAnimGraphStates && mAnimGraphInstance)
            {
                mAnimGraphInstance->Update(0.0f);
                mAnimGraphInstance->Output(nullptr);
            }

            // apply the main transformation
            recorder.SampleAndApplyMainTransform(recorder.GetCurrentPlayTime(), this);

            // apply the node transforms
            if (recorder.GetRecordSettings().mRecordTransforms)
            {
                recorder.SampleAndApplyTransforms(recorder.GetCurrentPlayTime(), this);
            }

            // sample the morph targets
            if (recorder.GetRecordSettings().mRecordMorphs)
            {
                recorder.SampleAndApplyMorphs(recorder.GetCurrentPlayTime(), this);
            }

            // perform forward kinematics etc
            mTransformData->UpdateNodeFlags();
            UpdateLocalMatrices();          // update local transformation matrices
            UpdateGlobalTransform();        // update the global transformation
            UpdateGlobalMatrices();         // update global matrices (performs forward kinematics)
            mTransformData->ResetNodeFlags(); // reset node flags
            UpdateAttachments();            // update the attachment parent matrices

            // update the bounds when needed
            if (GetBoundsUpdateEnabled() && mBoundsUpdateType != BOUNDS_MESH_BASED)
            {
                mBoundsUpdatePassedTime += timePassedInSeconds;
                if (mBoundsUpdatePassedTime >= mBoundsUpdateFrequency)
                {
                    UpdateBounds(mLODLevel, BOUNDS_NODE_BASED, mBoundsUpdateItemFreq);
                    mBoundsUpdatePassedTime = 0.0f;
                }
            }

            return;
        } // if the recorder is in playback mode and we recorded this actor instance

        // check if we are an attachment
        Attachment* attachment = GetSelfAttachment();

        // if we are not an attachment, or if we are but not one influenced by multiple nodes
        if (attachment == nullptr || (attachment && attachment->GetIsInfluencedByMultipleNodes() == false))
        {
            // update the motion system, which performs all blending, and updates all local transforms (excluding the local matrices)
            if (mAnimGraphInstance)
            {
                mAnimGraphInstance->Update(timePassedInSeconds);
                UpdateGlobalTransform();
                if (updateMatrices && sampleMotions)
                {
                    mAnimGraphInstance->Output(mTransformData->GetCurrentPose());
                }
            }
            else
            if (mMotionSystem)
            {
                mMotionSystem->Update(timePassedInSeconds, (updateMatrices && sampleMotions));
            }
            else
            {
                UpdateGlobalTransform();        // update the global transformation
            }
            // when the actor instance isn't visible, we don't want to do more things
            if (updateMatrices == false)
            {
                if (GetBoundsUpdateEnabled() && mBoundsUpdateType == BOUNDS_STATIC_BASED)
                {
                    UpdateBounds(mLODLevel, mBoundsUpdateType);
                }

                return;
            }

            // update the procedural eyeblinker if we have one
            if (mEyeBlinker)
            {
                mEyeBlinker->Update(timePassedInSeconds);
            }

            // **** if you want to overwrite some local space transforms, then here is the location to do it, or after the morph setup ****
            // of course it is nicer to use the LocalSpaceController class for this though, as that takes blending etc into account
            // <insert your code here, that modifies the local transforms>

            // apply the morph setup
            mTransformData->GetCurrentPose()->ApplyMorphWeightsToActorInstance();
            ApplyMorphSetup();

            // update the local matrices (calc them based on local pos/rot/scale etc)
            mTransformData->UpdateNodeFlags();

            if (sampleMotions)
            {
                UpdateLocalMatrices();
            }

            // update global matrices (performs forward kinematics)
            UpdateGlobalMatrices();

            // reset the node flags
            mTransformData->ResetNodeFlags();

            // update the parent matrices
            UpdateAttachments();
        }
        else // we are a skin attachment
        {
            const bool allowFastUpdates = attachment->GetAllowFastUpdates();

            // update the motion system, which performs all blending, and updates all local transforms (excluding the local matrices)
            if (mAnimGraphInstance)
            {
                mAnimGraphInstance->Update(timePassedInSeconds);
                UpdateGlobalTransform();

                if (updateMatrices && sampleMotions)
                {
                    mAnimGraphInstance->Output(mTransformData->GetCurrentPose());
                }
            }
            else
            if (mMotionSystem)
            {
                mMotionSystem->Update(timePassedInSeconds, allowFastUpdates ? false : (updateMatrices && sampleMotions));
            }
            else
            {
                UpdateGlobalTransform();        // update the global transformation
            }
            // when the actor instance isn't visible, we don't want to do more things
            if (updateMatrices == false)
            {
                if (GetBoundsUpdateEnabled() && mBoundsUpdateType == BOUNDS_STATIC_BASED)
                {
                    UpdateBounds(mLODLevel, mBoundsUpdateType);
                }
                return;
            }

            // update the procedural eyeblinker if we have one
            if (mEyeBlinker)
            {
                mEyeBlinker->Update(timePassedInSeconds);
            }

            // **** if you want to overwrite some local space transforms, then here is the location to do it, or after the morph setup ****
            // of course it is nicer to use the LocalSpaceController class for this though, as that takes blending etc into account
            // <insert your code here, that modifies the local transforms>

            // apply the morph setup
            if (allowFastUpdates == false)
            {
                mTransformData->GetCurrentPose()->ApplyMorphWeightsToActorInstance();
                ApplyMorphSetup();

                // update the local matrices (calc them based on local pos/rot/scale etc)
                mTransformData->UpdateNodeFlags();
                UpdateLocalMatrices();
            }

            // update global matrices (performs forward kinematics)
            //if (allowFastUpdates == false)
            //  UpdateGlobalMatrices();

            // if this is an attachment that deforms with the actor instance where it is attached to
            // then we need to overwrite the local space transformations for the nodes
            // nodes that don't exist in the actor it deforms with will not be overwritten
            UpdateMatricesIfSkinAttachment();

            if (allowFastUpdates == false)
            {
                // if this is an attachment itself, calculate all the global matrices again
                if (mSelfAttachment)
                {
                    UpdateGlobalMatricesForNonRoots(); // don't update root nodes though, assume the root nodes are already copied over from the actor instance where we are attached to
                }

                // reset the node flags
                mTransformData->ResetNodeFlags();
            }

            // update the attachment parent matrices
            UpdateAttachments();
        }

        // update the bounds when needed
        if (GetBoundsUpdateEnabled() && mBoundsUpdateType != BOUNDS_MESH_BASED)
        {
            mBoundsUpdatePassedTime += timePassedInSeconds;
            if (mBoundsUpdatePassedTime >= mBoundsUpdateFrequency)
            {
                UpdateBounds(mLODLevel, mBoundsUpdateType, mBoundsUpdateItemFreq);
                mBoundsUpdatePassedTime = 0.0f;
            }
        }
    }


    // calculate the local space matrix for a given node
    void ActorInstance::CalcLocalTM(uint32 nodeIndex, MCore::Matrix* outMatrix) const
    {
        const Transform& localTransform = mTransformData->GetCurrentPose()->GetLocalTransform(nodeIndex);

    #ifdef EMFX_SCALE_DISABLED
        CalcLocalTM(nodeIndex, localTransform.mPosition, localTransform.mRotation, MCore::Vector3(1.0f, 1.0f, 1.0f), outMatrix);
    #else
        CalcLocalTM(nodeIndex, localTransform.mPosition, localTransform.mRotation, localTransform.mScale, outMatrix);
    #endif
    }


    // calculate the local space matrix from transformation components (pos/rot/scale) that have been specified manually
    void ActorInstance::CalcLocalTM(uint32 nodeIndex, const AZ::Vector3& pos, const MCore::Quaternion& rot, const AZ::Vector3& scale, MCore::Matrix* outMatrix) const
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(nodeIndex);
        MCORE_UNUSED(scale);
        outMatrix->InitFromPosRot(pos, rot);
    #else
        // if there is scale
        TransformData::ENodeFlags flags = mTransformData->GetNodeFlags(nodeIndex);
        if (flags & TransformData::FLAG_HASSCALE)
        {
            // check if there is a parent
            const uint32 parentIndex = mActor->GetSkeleton()->GetNode(nodeIndex)->GetParentIndex();
            if ((parentIndex != MCORE_INVALIDINDEX32) && (mTransformData->GetNodeFlags(parentIndex) & TransformData::FLAG_HASSCALE))    // if there is a parent and it has scale
            {
                AZ::Vector3 invParentScale(1.0f, 1.0f, 1.0f);
                invParentScale /= mTransformData->GetCurrentPose()->GetLocalTransform(parentIndex).mScale;  // TODO: unsafe, can get division by zero
                outMatrix->InitFromNoScaleInherit(pos, rot, scale, invParentScale);
            }
            else // there is NO parent, so its a root node
            {
                outMatrix->InitFromPosRotScale(pos, rot, scale);
            }
        }
        else    // if there is no scale in the current node
        {
            // check if there is a parent, but no scale
            const uint32 parentIndex = mActor->GetSkeleton()->GetNode(nodeIndex)->GetParentIndex();
            if ((parentIndex != MCORE_INVALIDINDEX32) && (mTransformData->GetNodeFlags(parentIndex) & TransformData::FLAG_HASSCALE))    // if there is a parent and it has scale
            {
                // calculate the inverse parent scale
                AZ::Vector3 invParentScale(1.0f, 1.0f, 1.0f);
                invParentScale /= mTransformData->GetCurrentPose()->GetLocalTransform(parentIndex).mScale;  // TODO: unsafe, can get division by zero
                outMatrix->InitFromNoScaleInherit(pos, rot, scale, invParentScale);
            }
            else    // there is no parent or parent scale, and no scale
            {
                outMatrix->InitFromPosRot(pos, rot);
            }
        }
    #endif
    }


    // updates the local matrices of all nodes and the actor instance itself
    void ActorInstance::UpdateLocalMatrices()
    {
    #ifdef EMFX_SCALE_DISABLED
        MCore::Matrix* localMatrices  = mTransformData->GetLocalMatrices();
        Pose* pose = mTransformData->GetCurrentPose();

        // process all nodes
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the index of the node from the array of enabled nodes
            const uint16 nodeNumber = GetEnabledNode(i);
            const Transform& localTransform = pose->GetLocalTransform(nodeNumber);
            localMatrices[nodeNumber].InitFromPosRot(localTransform.mPosition, localTransform.mRotation);
        }   // for loop
    #else
        MCore::Matrix*  localMatrices = mTransformData->GetLocalMatrices();

        // process all nodes
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the index of the node from the array of enabled nodes
            const uint32 nodeNumber = GetEnabledNode(i);

            // if we need to update the local TM
            TransformData::ENodeFlags flags = mTransformData->GetNodeFlags(nodeNumber);
            const Transform& localTransform = mTransformData->GetCurrentPose()->GetLocalTransform(nodeNumber);

            // if there is scale
            if (flags & TransformData::FLAG_HASSCALE)
            {
                // check if there is a parent
                const uint32 parentIndex = skeleton->GetNode(nodeNumber)->GetParentIndex();
                if ((parentIndex != MCORE_INVALIDINDEX32) && (mTransformData->GetNodeFlags(parentIndex) & TransformData::FLAG_HASSCALE))    // if there is a parent and it has scale
                {
                    AZ::Vector3 invParentScale(1.0f, 1.0f, 1.0f);
                    invParentScale /= mTransformData->GetCurrentPose()->GetLocalTransform(parentIndex).mScale;  // TODO: unsafe, can get division by zero
                    localMatrices[nodeNumber].InitFromNoScaleInherit(localTransform.mPosition, localTransform.mRotation, localTransform.mScale, invParentScale);
                }
                else // there is NO parent, so its a root node
                {
                    localMatrices[nodeNumber].InitFromPosRotScale(localTransform.mPosition, localTransform.mRotation, localTransform.mScale);
                }
            }
            else    // if there is no scale in the current node
            {
                // check if there is a parent, but no scale
                const uint32 parentIndex = skeleton->GetNode(nodeNumber)->GetParentIndex();
                if ((parentIndex != MCORE_INVALIDINDEX32) && (mTransformData->GetNodeFlags(parentIndex) & TransformData::FLAG_HASSCALE))    // if there is a parent and it has scale
                {
                    // calculate the inverse parent scale
                    AZ::Vector3 invParentScale(1.0f, 1.0f, 1.0f);
                    invParentScale /= mTransformData->GetCurrentPose()->GetLocalTransform(parentIndex).mScale;  // TODO: unsafe, can get division by zero
                    localMatrices[nodeNumber].InitFromNoScaleInherit(localTransform.mPosition, localTransform.mRotation, localTransform.mScale, invParentScale);
                }
                else    // there is no parent or parent scale, and no scale
                {
                    localMatrices[nodeNumber].InitFromPosRot(localTransform.mPosition, localTransform.mRotation);
                }
            }
        }
    #endif  // #ifdef EMFX_SCALE_DISABLED
    }


    // update global space matrices for nodes that are no root nodes
    void ActorInstance::UpdateGlobalMatricesForNonRoots()
    {
        // get the transform data and some shortcuts to specific transform buffers
        MCore::Matrix* localMatrices = mTransformData->GetLocalMatrices();
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the index of the node from the array of enabled nodes
            const uint32 nodeNumber = GetEnabledNode(i);

            // if this is not a root node
            const uint32 parentIndex = skeleton->GetNode(nodeNumber)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                globalMatrices[nodeNumber].MultMatrix4x3(localMatrices[nodeNumber], globalMatrices[parentIndex]);
            }
        }
    }


    // update the global transformation
    void ActorInstance::UpdateGlobalTransform()
    {
        mGlobalTransform = mLocalTransform;
        mGlobalTransform.Multiply(mParentGlobalTransform);
    }


    // updates the global matrices of all nodes
    void ActorInstance::UpdateGlobalMatrices()
    {
        // build the global transform matrix
        MCore::Matrix globalTM = mGlobalTransform.ToMatrix();

        // get the transform data and some shortcuts to specific transform buffers
        MCore::Matrix* localMatrices = mTransformData->GetLocalMatrices();
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the index of the node from the array of enabled nodes
            const uint32 nodeNumber = GetEnabledNode(i);

            // if this is not a root node
            const uint32 parentIndex = skeleton->GetNode(nodeNumber)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                globalMatrices[nodeNumber].MultMatrix4x3(localMatrices[nodeNumber], globalMatrices[parentIndex]);
            }
            else
            {
                globalMatrices[nodeNumber].MultMatrix4x3(localMatrices[nodeNumber], globalTM);
            }
        }
    }


    // Update the mesh deformers, which updates the vertex positions on the CPU, so performing CPU skinning and morphing etc.
    void ActorInstance::UpdateMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers)
    {
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // Update the mesh deformers.
        const Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = mEnabledNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = mEnabledNodes[i];
            Node* node = skeleton->GetNode(nodeNr);
            MeshDeformerStack* stack = mActor->GetMeshDeformerStack(mLODLevel, nodeNr);
            if (stack)
            {
                stack->Update(this, node, timePassedInSeconds, processDisabledDeformers);
            }
        }

        // Update the bounds when we are set to use mesh based bounds.
        if (GetBoundsUpdateEnabled() == BOUNDS_MESH_BASED)
        {
            mBoundsUpdatePassedTime += timePassedInSeconds;
            if (mBoundsUpdatePassedTime >= mBoundsUpdateFrequency)
            {
                UpdateBounds(mLODLevel, mBoundsUpdateType, mBoundsUpdateItemFreq);
                mBoundsUpdatePassedTime = 0.0f;
            }
        }
    }



    // Update the mesh morph deformers, which updates the vertex positions on the CPU, so performing CPU morphing.
    void ActorInstance::UpdateMorphMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers)
    {
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // Update the mesh morph deformers.
        const Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = mEnabledNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = mEnabledNodes[i];
            Node* node = skeleton->GetNode(nodeNr);
            MeshDeformerStack* stack = mActor->GetMeshDeformerStack(mLODLevel, nodeNr);
            if (stack)
            {
                stack->UpdateByModifierType(this, node, timePassedInSeconds, MorphMeshDeformer::TYPE_ID, true, processDisabledDeformers);
            }
        }

        // Update the bounds when we are set to use mesh based bounds.
        if (GetBoundsUpdateEnabled() == BOUNDS_MESH_BASED)
        {
            mBoundsUpdatePassedTime += timePassedInSeconds;
            if (mBoundsUpdatePassedTime >= mBoundsUpdateFrequency)
            {
                UpdateBounds(mLODLevel, mBoundsUpdateType, mBoundsUpdateItemFreq);
                mBoundsUpdatePassedTime = 0.0f;
            }
        }
    }

    // add an attachment
    void ActorInstance::AddAttachment(Attachment* attachment)
    {
        // first remove the current attachment tree from the scheduler
        ActorInstance* root = FindAttachmentRoot();
        GetActorManager().GetScheduler()->RecursiveRemoveActorInstance(root);

        // add the attachment
        mAttachments.Add(attachment);
        ActorInstance* attachmentActorInstance = attachment->GetAttachmentActorInstance();
        if (attachmentActorInstance)
        {
            attachmentActorInstance->IncreaseNumAttachmentRefs();
            attachmentActorInstance->SetAttachedTo(this);
            GetActorManager().UpdateActorInstanceStatus(attachmentActorInstance);
        }

        // and re-add the root to the scheduler
        //GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root, 0);
        // re-add the root if it was visible already
        //if (root->GetIsVisible())
        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root);
    }


    // try to find the attachment number for a given actor instance
    uint32 ActorInstance::FindAttachmentNr(ActorInstance* actorInstance)
    {
        // for all attachments
        const uint32 numAttachments = mAttachments.GetLength();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            if (mAttachments[i]->GetAttachmentActorInstance() == actorInstance)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // remove an attachment by actor instance pointer
    bool ActorInstance::RemoveAttachment(ActorInstance* actorInstance, bool delFromMem)
    {
        // try to find the attachment
        const uint32 attachmentNr = FindAttachmentNr(actorInstance);
        if (attachmentNr == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the actual attachment
        RemoveAttachment(attachmentNr, delFromMem);
        return true;
    }


    // remove an attachment
    void ActorInstance::RemoveAttachment(uint32 nr, bool delFromMem)
    {
        MCORE_ASSERT(nr < mAttachments.GetLength());

        // first remove the current attachment tree from the scheduler
        ActorInstance* root = FindAttachmentRoot();
        GetActorManager().GetScheduler()->RecursiveRemoveActorInstance(root);

        // get the attachment
        Attachment* attachment = mAttachments[nr];

        // its not an attachment anymore
        ActorInstance* attachmentInstance = attachment->GetAttachmentActorInstance();
        if (attachmentInstance)
        {
            attachmentInstance->SetSelfAttachment(nullptr);

            // unregister the attachment inside the actor instance that represents the attachment
            attachmentInstance->DecreaseNumAttachmentRefs();
            attachmentInstance->SetAttachedTo(nullptr);
            GetActorManager().UpdateActorInstanceStatus(attachmentInstance);

            Transform identTransform;
            identTransform.Identity();
            attachmentInstance->SetParentGlobalTransform(identTransform);
        }

        // delete it from memory when desired
        if (delFromMem)
        {
            attachment->Destroy();
        }

        // remove it from the attachment list
        mAttachments.Remove(nr);

        // and re-add the root to the scheduler
        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root, 0);

        // re-add the attachment
        if (attachmentInstance)
        {
            GetActorManager().GetScheduler()->RecursiveInsertActorInstance(attachmentInstance, 0);
        }
    }


    // remove all attachments
    void ActorInstance::RemoveAllAttachments(bool delFromMem)
    {
        // keep removing the last attachment until there are none left
        while (mAttachments.GetLength())
        {
            RemoveAttachment(mAttachments.GetLength() - 1, delFromMem);
        }
    }


    // update the dependencies
    void ActorInstance::UpdateDependencies()
    {
        // get rid of existing dependencies
        mDependencies.Clear();

        // add the main dependency
        Actor::Dependency mainDependency;
        mainDependency.mActor       = mActor;
        mainDependency.mAnimGraph  = (mAnimGraphInstance) ? mAnimGraphInstance->GetAnimGraph() : nullptr;
        mDependencies.Add(mainDependency);

        // add all dependencies stored inside the actor
        const uint32 numDependencies = mActor->GetNumDependencies();
        for (uint32 i = 0; i < numDependencies; ++i)
        {
            mDependencies.Add(*mActor->GetDependency(i));
        }
    }


    // set the attachment matrices
    void ActorInstance::UpdateAttachments()
    {
        // update all attachments
        const uint32 numAttachments = mAttachments.GetLength();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            mAttachments[i]->Update();
        }
    }


    // find the root attachment actor instance, for example if you have a
    // knight with knife attached to his hands, where the knight is attached to a horse, the horse will be the
    // attachment root
    ActorInstance* ActorInstance::FindAttachmentRoot() const
    {
        if (mAttachedTo)
        {
            return mAttachedTo->FindAttachmentRoot();
        }

        return const_cast<ActorInstance*>(this);
    }


    // update the matrices in case this is a skin attachment
    void ActorInstance::UpdateMatricesIfSkinAttachment()
    {
        // if this actor instance isn't an attachment, then there's nothing to do
        if (GetIsAttachment() == false)
        {
            return;
        }

        // copy over the matrices
        MCORE_ASSERT(mSelfAttachment);
        MCORE_ASSERT(mSelfAttachment->GetIsInfluencedByMultipleNodes());

        static_cast<AttachmentSkin*>(mSelfAttachment)->UpdateNodeTransforms();
    }


    // recursively update the globalspace matrix for a given node
    // this propagates the changes to all child nodes, using forward kinematics
    void ActorInstance::RecursiveUpdateGlobalTM(uint32 nodeIndex, const MCore::Matrix* globalTM, MCore::Matrix* outGlobalMatrixArray)
    {
        // use the global space matrix array on default
        if (outGlobalMatrixArray == nullptr)
        {
            outGlobalMatrixArray = mTransformData->GetGlobalInclusiveMatrices();
        }

        MCore::Matrix* outLocalMatrices = mTransformData->GetLocalMatrices();
        Node* node = mActor->GetSkeleton()->GetNode(nodeIndex);

        // if we have set a global matrix, then copy it, else calculate it
        if (globalTM)
        {
            outGlobalMatrixArray[nodeIndex] = *globalTM;
            const uint32 parentIndex = node->GetParentIndex();

            // calculate the local TM
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                outLocalMatrices[nodeIndex].MultMatrix4x3(*globalTM, outGlobalMatrixArray[parentIndex].Inversed());
            }
            else
            {
                outLocalMatrices[nodeIndex] = *globalTM;
            }
        }
        else
        {
            const uint32 parentIndex = node->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                outGlobalMatrixArray[nodeIndex].MultMatrix4x3(outLocalMatrices[nodeIndex], outGlobalMatrixArray[parentIndex]);
            }
        }

        // recursively update the global matrices of the childs
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveUpdateGlobalTM(node->GetChildIndex(i), nullptr, outGlobalMatrixArray);
        }
    }


    // change visibility state
    void ActorInstance::SetIsVisible(bool isVisible)
    {
        // if the state doesn't change, do nothing
        if (isVisible == GetIsVisible())
        {
            return;
        }

        // if we change visibility state
        SetFlag(BOOL_ISVISIBLE, isVisible);
    }


    // update the bounding volume
    void ActorInstance::UpdateBounds(uint32 geomLODLevel, EBoundsType boundsType, uint32 itemFrequency)
    {
        // depending on the bounding volume update type
        switch (boundsType)
        {
        // calculate the static based AABB
        case BOUNDS_STATIC_BASED:
            CalcStaticBasedAABB(&mAABB);
            break;

        // based on the global space positions of the nodes (least accurate, but fastest)
        case BOUNDS_NODE_BASED:
            CalcNodeBasedAABB(&mAABB, itemFrequency);
            break;

        // based on the global space positions of the vertices of the collision meshes (faster and more accurate than mesh based)
        case BOUNDS_COLLISIONMESH_BASED:
            CalcCollisionMeshBasedAABB(geomLODLevel, &mAABB, itemFrequency);
            break;

        // based on the global space positions of the vertices of the meshes (most accurate)
        case BOUNDS_MESH_BASED:
            CalcMeshBasedAABB(geomLODLevel, &mAABB, itemFrequency);
            break;

        // based on the global space positions of the vertices of the meshes (most accurate)
        case BOUNDS_NODEOBB_BASED:
            CalcNodeOBBBasedAABB(&mAABB, itemFrequency);
            break;

        case BOUNDS_NODEOBBFAST_BASED:
            CalcNodeOBBBasedAABBFast(&mAABB, itemFrequency);
            break;

        // when we're dealing with an unspecified bounding volume update method
        default:
            MCore::LogInfo("*** EMotionFX::ActorInstance::UpdateBounds() - Unknown boundsType specified! (%d) ***", (uint32)boundsType);
        }
        ;
    }


    // calculate the axis aligned bounding box that contains the object oriented boxes of all nodes
    void ActorInstance::CalcNodeOBBBasedAABBFast(MCore::AABB* outResult, uint32 nodeFrequency)
    {
        // init the axis aligned bounding box
        outResult->Init();

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // for all nodes, encapsulate the global space positions
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; i += nodeFrequency)
        {
            nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);
            if (node->GetIncludeInBoundsCalc())
            {
                const MCore::OBB& obb = mActor->GetNodeOBB(nodeNr);
                if (obb.CheckIfIsValid() == false)
                {
                    continue;
                }

                // calculate the corner points of the node in local space
                AZ::Vector3 minPoint, maxPoint;
                obb.CalcMinMaxPoints(&minPoint, &maxPoint);

                // encapsulate the results in the AABB box
                outResult->Encapsulate(minPoint * globalMatrices[nodeNr]);
                outResult->Encapsulate(maxPoint * globalMatrices[nodeNr]);
            }
        }
    }


    // more accurate node obb based method that uses the 8 corner points of the obb
    void ActorInstance::CalcNodeOBBBasedAABB(MCore::AABB* outResult, uint32 nodeFrequency)
    {
        // init the axis aligned bounding box
        outResult->Init();

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // the 8 corners of a given OBB
        AZ::Vector3 cornerPoints[8];

        // for all nodes, encapsulate the global space positions
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; i += nodeFrequency)
        {
            nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);
            if (node->GetIncludeInBoundsCalc())
            {
                const MCore::OBB& obb = mActor->GetNodeOBB(nodeNr);
                if (obb.CheckIfIsValid() == false)
                {
                    continue;
                }

                // calculate the 8 corner points
                obb.CalcCornerPoints(cornerPoints);

                // encapsulate all OBB global space corner points inside the AABB
                for (uint32 p = 0; p < 8; ++p)
                {
                    cornerPoints[p] *= globalMatrices[i];
                    outResult->Encapsulate(cornerPoints[p]);
                }
            }
        }
    }


    // calculate the axis aligned bounding box based on the global space positions of the nodes
    void ActorInstance::CalcNodeBasedAABB(MCore::AABB* outResult, uint32 nodeFrequency)
    {
        // init the axis aligned bounding box
        outResult->Init();

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // if it is not a skin attachment
        if (GetIsSkinAttachment() == false)
        {
            // for all nodes, encapsulate the global space positions
            uint16 nodeNr;
            const uint32 numNodes = GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; i += nodeFrequency)
            {
                nodeNr = GetEnabledNode(i);
                if (skeleton->GetNode(nodeNr)->GetIncludeInBoundsCalc())
                {
                    outResult->Encapsulate(globalMatrices[nodeNr].GetTranslation());
                }
            }
        }
        else // if we are dealing with a skin attachment here
        {
            // get the attachment
            AttachmentSkin* attachment = static_cast<AttachmentSkin*>(GetSelfAttachment());

            // process all nodes
            uint16 nodeIndex;
            const uint32 numNodes = GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeIndex = GetEnabledNode(i);
                if (attachment->GetNodeMapping(nodeIndex).mSourceNode) // if this node maps to a node in the main skeleton, otherwise skip it as then the bounds would go to the origin
                {
                    outResult->Encapsulate(globalMatrices[nodeIndex].GetTranslation());
                }
            }
        }
    }


    // calculate the AABB that contains all global space vertices of all meshes
    void ActorInstance::CalcMeshBasedAABB(uint32 geomLODLevel, MCore::AABB* outResult, uint32 vertexFrequency)
    {
        // init the axis aligned bounding box
        outResult->Init();

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // for all nodes, encapsulate the global space positions
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);

            // skip nodes without meshes
            Mesh* mesh = mActor->GetMesh(geomLODLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            // if this node should be excluded
            if (node->GetIncludeInBoundsCalc() == false)
            {
                continue;
            }

            // calculate and encapsulate the mesh bounds inside the total mesh box
            MCore::AABB meshBox;
            mesh->CalcAABB(&meshBox, globalMatrices[nodeNr], vertexFrequency);
            outResult->Encapsulate(meshBox);
        }
    }


    void ActorInstance::CalcCollisionMeshBasedAABB(uint32 geomLODLevel, MCore::AABB* outResult, uint32 vertexFrequency)
    {
        // init the axis aligned bounding box
        outResult->Init();

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // for all nodes, encapsulate the global space positions
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);

            // skip nodes without collision meshes
            Mesh* mesh = mActor->GetMesh(geomLODLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                continue;
            }

            // if this node should be excluded
            if (node->GetIncludeInBoundsCalc() == false)
            {
                continue;
            }

            // calculate and encapsulate the mesh bounds inside the total mesh box
            MCore::AABB meshBox;
            mesh->CalcAABB(&meshBox, globalMatrices[nodeNr], vertexFrequency);
            outResult->Encapsulate(meshBox);
        }
    }


    // setup bounding volume auto update settings
    void ActorInstance::SetupAutoBoundsUpdate(float updateFrequencyInSeconds, EBoundsType boundsType, uint32 itemFrequency)
    {
        MCORE_ASSERT(itemFrequency > 0); // zero would cause an infinite loop
        mBoundsUpdateFrequency  = updateFrequencyInSeconds;
        mBoundsUpdateType       = boundsType;
        mBoundsUpdateItemFreq   = itemFrequency;
        SetBoundsUpdateEnabled(true);
    }


    // apply the morph setup
    void ActorInstance::ApplyMorphSetup()
    {
        // get the morph setup instance and original setup
        MorphSetupInstance* morphSetupInstance = GetMorphSetupInstance();
        if (morphSetupInstance == nullptr)
        {
            return;
        }

        // if there is no morph setup, we have nothing to do
        MorphSetup* morphSetup = mActor->GetMorphSetup(mLODLevel);
        if (morphSetup == nullptr)
        {
            return;
        }

        // apply all morph targets
        //bool allZero = true;
        const uint32 numTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            // get the morph target
            MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
            MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
            if (morphTargetInstance == nullptr)
            {
                continue;
            }

            // get the weight for this morph target
            const float weight = morphTargetInstance->GetWeight();

            // apply the morph target if its weight is non-zero
            if (MCore::Math::Abs(weight) > 0.0001f)
            {
                //allZero = false;
                morphTarget->Apply(this, weight);
            }
        }

        /*
            // enable or disable all morph deformers if the weights are all zero
            const uint32 numNodes = mActor->GetNumNodes();
            for (uint32 n=0; n<numNodes; ++n)
            {
                Node* node = mActor->GetNode(n);
                MeshDeformerStack* stack = node->GetMeshDeformerStack( mGeometryLODLevel ).GetPointer();
                if (stack == nullptr)
                    continue;

                stack->EnableAllDeformersByType( MorphMeshDeformer::TYPE_ID, !allZero );
            }*/
    }

    //---------------------

    // check intersection with a ray, but don't get the intersection point or closest intersecting node
    Node* ActorInstance::IntersectsCollisionMesh(uint32 lodLevel, const MCore::Ray& ray) const
    {
        Skeleton* skeleton = mActor->GetSkeleton();

        // for all nodes
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if there is a mesh for this node
            Mesh* mesh = mActor->GetMesh(lodLevel, i);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                continue;
            }

            // return when there is an intersection
            if (mesh->Intersects(mTransformData->GetGlobalInclusiveMatrices()[i], ray))
            {
                return skeleton->GetNode(i);
            }
        }

        return nullptr;
    }


    Node* ActorInstance::IntersectsCollisionMesh(uint32 lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal, AZ::Vector2* outUV, float* outBaryU, float* outBaryV, uint32* outIndices) const
    {
        Node*           closestNode = nullptr;
        AZ::Vector3     point;
        AZ::Vector3     closestPoint(0.0f, 0.0f, 0.0f);
        float           dist, baryU, baryV, closestBaryU = 0, closestBaryV = 0;
        float           closestDist = FLT_MAX;
        uint32          closestIndices[3];
        uint32          triIndices[3];

        Skeleton* skeleton = mActor->GetSkeleton();

        // check all nodes
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; i++)
        {
            nodeNr = GetEnabledNode(i);
            Node* curNode = skeleton->GetNode(nodeNr);
            Mesh* mesh = mActor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                continue;
            }

            // if there is an intersection between the ray and the mesh
            if (mesh->Intersects(mTransformData->GetGlobalInclusiveMatrix(nodeNr), ray, &point, &baryU, &baryV, triIndices))
            {
                // calculate the squared distance between the intersection point and the ray origin
                dist = (point - ray.GetOrigin()).GetLengthSq();

                // if it is the closest point till now, record it as closest
                if (dist < closestDist)
                {
                    closestPoint    = point;
                    closestDist     = dist;
                    closestNode     = curNode;
                    closestBaryU    = baryU;
                    closestBaryV    = baryV;
                    closestIndices[0] = triIndices[0];
                    closestIndices[1] = triIndices[1];
                    closestIndices[2] = triIndices[2];
                }
            }
        }

        // output the closest values
        if (closestNode)
        {
            if (outIntersect)
            {
                *outIntersect = closestPoint;
            }

            if (outBaryU)
            {
                *outBaryU = closestBaryU;
            }

            if (outBaryV)
            {
                *outBaryV = closestBaryV;
            }

            if (outIndices)
            {
                outIndices[0] = closestIndices[0];
                outIndices[1] = closestIndices[1];
                outIndices[2] = closestIndices[2];
            }

            // calculate the interpolated normal
            if (outNormal || outUV)
            {
                Mesh* mesh = mActor->GetMesh(lodLevel, closestNode->GetNodeIndex());

                // calculate the normal at the intersection point
                if (outNormal)
                {
                    AZ::PackedVector3f* normals = (AZ::PackedVector3f*)mesh->FindVertexData(Mesh::ATTRIB_NORMALS);
                    AZ::Vector3  norm = MCore::BarycentricInterpolate<AZ::Vector3>(closestBaryU, closestBaryV, AZ::Vector3(normals[closestIndices[0]]), AZ::Vector3(normals[closestIndices[1]]), AZ::Vector3(normals[closestIndices[2]]));
                    norm = mTransformData->GetGlobalInclusiveMatrices()[closestNode->GetNodeIndex()].Mul3x3(norm);
                    norm.GetNormalized();
                    *outNormal = norm;
                }

                // calculate the UV coordinate at the intersection point
                if (outUV)
                {
                    // get the UV coordinates of the first layer
                    AZ::Vector2* uvData = static_cast<AZ::Vector2*>(mesh->FindVertexData(Mesh::ATTRIB_UVCOORDS, 0));
                    if (uvData)
                    {
                        // calculate the interpolated texture coordinate
                        *outUV = MCore::BarycentricInterpolate<AZ::Vector2>(closestBaryU, closestBaryV, uvData[ closestIndices[0] ],
                                uvData[ closestIndices[1] ],
                                uvData[ closestIndices[2] ]);
                    }
                }
            }
        }

        // return the result
        return closestNode;
    }


    // check intersection with a ray, but don't get the intersection point or closest intersecting node
    Node* ActorInstance::IntersectsMesh(uint32 lodLevel, const MCore::Ray& ray) const
    {
        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // for all nodes
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);

            // check if there is a mesh for this node
            Mesh* mesh = mActor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            // return when there is an intersection
            if (mesh->Intersects(globalMatrices[nodeNr], ray))
            {
                return node;
            }
        }

        // no intersection found
        return nullptr;
    }



    // intersection test that returns the closest intersection
    Node* ActorInstance::IntersectsMesh(uint32 lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal, AZ::Vector2* outUV, float* outBaryU, float* outBaryV, uint32* outIndices) const
    {
        Node*           closestNode = nullptr;
        AZ::Vector3     point;
        AZ::Vector3     closestPoint(0.0f, 0.0f, 0.0f);
        float           dist, baryU, baryV, closestBaryU = 0, closestBaryV = 0;
        float           closestDist = FLT_MAX;
        uint32          closestIndices[3];
        uint32          triIndices[3];

        // get the global space matrices
        MCore::Matrix* globalMatrices = mTransformData->GetGlobalInclusiveMatrices();
        Skeleton* skeleton = mActor->GetSkeleton();

        // check all nodes
        uint16 nodeNr;
        const uint32 numNodes = GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; i++)
        {
            nodeNr = GetEnabledNode(i);
            Node* curNode = skeleton->GetNode(nodeNr);
            Mesh* mesh = mActor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            // if there is an intersection between the ray and the mesh
            if (mesh->Intersects(globalMatrices[nodeNr], ray, &point, &baryU, &baryV, triIndices))
            {
                // calculate the squared distance between the intersection point and the ray origin
                dist = (point - ray.GetOrigin()).GetLengthSq();

                // if it is the closest point till now, record it as closest
                if (dist < closestDist)
                {
                    closestPoint    = point;
                    closestDist     = dist;
                    closestNode     = curNode;
                    closestBaryU    = baryU;
                    closestBaryV    = baryV;
                    closestIndices[0] = triIndices[0];
                    closestIndices[1] = triIndices[1];
                    closestIndices[2] = triIndices[2];
                }
            }
        }

        // output the closest values
        if (closestNode)
        {
            if (outIntersect)
            {
                *outIntersect = closestPoint;
            }

            if (outBaryU)
            {
                *outBaryU = closestBaryU;
            }

            if (outBaryV)
            {
                *outBaryV = closestBaryV;
            }

            if (outIndices)
            {
                outIndices[0] = closestIndices[0];
                outIndices[1] = closestIndices[1];
                outIndices[2] = closestIndices[2];
            }

            // calculate the interpolated normal
            if (outNormal || outUV)
            {
                Mesh* mesh = mActor->GetMesh(lodLevel, closestNode->GetNodeIndex());

                // calculate the normal at the intersection point
                if (outNormal)
                {
                    AZ::PackedVector3f* normals = (AZ::PackedVector3f*)mesh->FindVertexData(Mesh::ATTRIB_NORMALS);
                    AZ::Vector3  norm = MCore::BarycentricInterpolate<AZ::Vector3>(
                            closestBaryU, closestBaryV,
                            AZ::Vector3(normals[closestIndices[0]]), AZ::Vector3(normals[closestIndices[1]]), AZ::Vector3(normals[closestIndices[2]]));
                    norm = globalMatrices[closestNode->GetNodeIndex()].Mul3x3(norm);
                    norm.GetNormalized();
                    *outNormal = norm;
                }

                // calculate the UV coordinate at the intersection point
                if (outUV)
                {
                    // get the UV coordinates of the first layer
                    AZ::Vector2* uvData = static_cast<AZ::Vector2*>(mesh->FindVertexData(Mesh::ATTRIB_UVCOORDS, 0));
                    if (uvData)
                    {
                        // calculate the interpolated texture coordinate
                        *outUV = MCore::BarycentricInterpolate<AZ::Vector2>(closestBaryU, closestBaryV, uvData[ closestIndices[0] ],
                                uvData[ closestIndices[1] ],
                                uvData[ closestIndices[2] ]);
                    }
                }
            }
        }

        // return the result
        return closestNode;
    }

    //---------------------


    //
    void ActorInstance::EnableNode(uint16 nodeIndex)
    {
        // if this node already is at an enabled state, do nothing
        if (mEnabledNodes.Contains(nodeIndex))
        {
            return;
        }

        Skeleton* skeleton = mActor->GetSkeleton();

        // find the location where to insert (as the flattened hierarchy needs to be preserved in the array)
        bool found = false;
        uint32 curNode = nodeIndex;
        do
        {
            // get the parent of the current node
            uint32 parentIndex = skeleton->GetNode(curNode)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                const uint32 parentArrayIndex = mEnabledNodes.Find(static_cast<uint16>(parentIndex));
                if (parentArrayIndex != MCORE_INVALIDINDEX32)
                {
                    if (parentArrayIndex + 1 >= mEnabledNodes.GetLength())
                    {
                        mEnabledNodes.Add(nodeIndex);
                    }
                    else
                    {
                        mEnabledNodes.Insert(parentArrayIndex + 1, nodeIndex);
                    }
                    found = true;
                }
                else
                {
                    curNode = parentIndex;
                }
            }
            else // if we're dealing with a root node, insert it in the front of the array
            {
                mEnabledNodes.Insert(0, nodeIndex);
                found = true;
            }
        } while (found == false);
    }


    // disable a given node
    void ActorInstance::DisableNode(uint16 nodeIndex)
    {
        // try to remove the node from the array
        mEnabledNodes.RemoveByValue(nodeIndex);
    }


    // enable all nodes
    void ActorInstance::EnableAllNodes()
    {
        const uint32 numNodes = mActor->GetNumNodes();
        mEnabledNodes.Resize(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mEnabledNodes[i] = static_cast<uint16>(i);
        }
    }


    // disable all nodes
    void ActorInstance::DisableAllNodes()
    {
        mEnabledNodes.Clear();
    }


    // change the skeletal LOD level
    void ActorInstance::SetSkeletalLODLevelNodeFlags(uint32 level)
    {
        // make sure the lod level is in range of 0..31
        const uint32 newLevel = MCore::Clamp<uint32>(level, 0, 31);

        // if the lod level is the same as it currently is, do nothing
        if (newLevel == mLODLevel)
        {
            return;
        }

        Skeleton* skeleton = mActor->GetSkeleton();

        // change the state of all nodes that need state changes
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // check the curent and the new enabled state
            const bool curEnabled = node->GetSkeletalLODStatus(mLODLevel);
            const bool newEnabled = node->GetSkeletalLODStatus(newLevel);

            // if the state changed, enable or disable it
            if (curEnabled != newEnabled)
            {
                if (newEnabled) // if the new LOD says that this node should be enabled, enable it
                {
                    EnableNode(static_cast<uint16>(i));
                }
                else        // otherwise disable it
                {
                    DisableNode(static_cast<uint16>(i));
                }
            }
        }
    }


    // set the new current LOD level for the actor instance
    void ActorInstance::SetLODLevel(uint32 level)
    {
        // enable and disable all nodes accordingly (do not call this after setting the new mLODLevel)
        SetSkeletalLODLevelNodeFlags(level);

        // make sure the LOD level is valid and update it
        mLODLevel = MCore::Clamp<uint32>(level, 0, mActor->GetNumLODLevels() - 1);
        /*
            // update the transform data
            MorphSetup* morphSetup = mActor->GetMorphSetup(mLODLevel);
            if (morphSetup)
                mTransformData->SetNumMorphWeights( morphSetup->GetNumMorphTargets() );
            else
                mTransformData->SetNumMorphWeights( 0 );*/
    }


    // enable or disable nodes based on the skeletal LOD flags
    void ActorInstance::UpdateSkeletalLODFlags()
    {
        // change the state of all nodes that need state changes
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // if the new LOD says that this node should be enabled, enable it
            if (node->GetSkeletalLODStatus(mLODLevel))
            {
                EnableNode(static_cast<uint16>(i));
            }
            else        // otherwise disable it
            {
                DisableNode(static_cast<uint16>(i));
            }
        }
    }


    // calculate the number of disabled nodes for a given skeletal lod level
    uint32 ActorInstance::CalcNumDisabledNodes(uint32 skeletalLODLevel) const
    {
        uint32 numDisabledNodes = 0;

        Skeleton* skeleton = mActor->GetSkeleton();

        // get the number of nodes and iterate through them
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the current node
            Node* node = skeleton->GetNode(i);

            // check if the current node is disabled in the given skeletal lod level, if yes increase the resulting number
            if (node->GetSkeletalLODStatus(skeletalLODLevel) == false)
            {
                numDisabledNodes++;
            }
        }

        return numDisabledNodes;
    }


    // calculate the number of skeletal LOD levels
    uint32 ActorInstance::CalcNumSkeletalLODLevels() const
    {
        uint32 numSkeletalLODLevels = 0;

        // iterate over all skeletal LOD levels
        uint32 currentNumDisabledNodes = 0;
        uint32 previousNumDisabledNodes = MCORE_INVALIDINDEX32;
        for (uint32 i = 0; i < 32; ++i)
        {
            // get the number of disabled nodes in the current skeletal LOD level
            currentNumDisabledNodes = CalcNumDisabledNodes(i);

            // compare the number of disabled nodes from the current skeletal LOD level with the previous one and increase the number of
            // skeletal LOD levels in case they are not equal, if they are equal we have reached the last skeletal LOD level and we can skip the further calculations
            if (previousNumDisabledNodes != currentNumDisabledNodes)
            {
                numSkeletalLODLevels++;
                previousNumDisabledNodes = currentNumDisabledNodes;
            }
            else
            {
                break;
            }
        }

        return numSkeletalLODLevels;
    }


    // change the current motion system
    void ActorInstance::SetMotionSystem(MotionSystem* newSystem, bool delCurrentFromMem)
    {
        if (delCurrentFromMem && mMotionSystem)
        {
            mMotionSystem->Destroy();
        }

        mMotionSystem = newSystem;
    }


    // check if this actor instance is a skin attachment
    bool ActorInstance::GetIsSkinAttachment() const
    {
        if (mSelfAttachment == nullptr)
        {
            return false;
        }

        return mSelfAttachment->GetIsInfluencedByMultipleNodes();
    }


    // draw a skeleton using lines, calling the drawline callbacks in the event handlers
    void ActorInstance::DrawSkeleton(Pose& pose, uint32 color)
    {
        Skeleton* skeleton = mActor->GetSkeleton();

        // iterate over all enabled nodes
        const uint32 numNodes = mEnabledNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 nodeIndex = mEnabledNodes[i];
            const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();

            // if there is a parent node
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                const AZ::Vector3& startPos     = pose.GetGlobalTransformInclusive(nodeIndex).mPosition;
                const AZ::Vector3& endPos       = pose.GetGlobalTransformInclusive(parentIndex).mPosition;
                GetEventManager().OnDrawLine(startPos, endPos, color);
            }
        }
    }


    // Remove the trajectory transform from the input transformation.
    void ActorInstance::MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform, EMotionExtractionFlags motionExtractionFlags)
    {
        MCORE_ASSERT(mActor->GetMotionExtractionNodeIndex() != MCORE_INVALIDINDEX32);

        Transform trajectoryTransform = inOutMotionExtractionNodeTransform;

        // Make sure the z axis is really pointing up and project it onto the ground plane.
        if (trajectoryTransform.mRotation.CalcForwardAxis().GetZ() > 0.0f) // Pick the closest, so if we point more upwards already, we take 1.0, otherwise take -1.0. Sometimes Y would point up, sometimes down.
        {
            trajectoryTransform.mRotation.RotateFromTo(trajectoryTransform.mRotation.CalcForwardAxis(), AZ::Vector3(0.0f, 0.0f, 1.0f));
        }
        else
        {
            trajectoryTransform.mRotation.RotateFromTo(trajectoryTransform.mRotation.CalcForwardAxis(), AZ::Vector3(0.0f, 0.0f, -1.0f));
        }

        trajectoryTransform.ApplyMotionExtractionFlags(motionExtractionFlags);

        // Get the projected bind pose transform.
        Transform bindTransformProjected = mTransformData->GetBindPose()->GetLocalTransform(mActor->GetMotionExtractionNodeIndex());
        bindTransformProjected.ApplyMotionExtractionFlags(motionExtractionFlags);

        // Remove the projected rotation and translation from the transform to prevent the double transform.
        inOutMotionExtractionNodeTransform.mRotation = (bindTransformProjected.mRotation.Conjugated() * trajectoryTransform.mRotation).Conjugated() * inOutMotionExtractionNodeTransform.mRotation;
        inOutMotionExtractionNodeTransform.mPosition = inOutMotionExtractionNodeTransform.mPosition - (trajectoryTransform.mPosition - bindTransformProjected.mPosition);
    }


    // Remove the trajectory transform from the motion extraction node to prevent double transformation.
    void ActorInstance::MotionExtractionCompensate(EMotionExtractionFlags motionExtractionFlags)
    {
        const uint32 motionExtractIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        Pose* currentPose = mTransformData->GetCurrentPose();
        Transform transform = currentPose->GetLocalTransform(motionExtractIndex);
        MotionExtractionCompensate(transform, motionExtractionFlags);

        currentPose->SetLocalTransform(motionExtractIndex, transform);
    }


    // Apply the motion extraction delta transform to the actor instance.
    void ActorInstance::ApplyMotionExtractionDelta(const Transform& trajectoryDelta)
    {
        if (mActor->GetMotionExtractionNodeIndex() == MCORE_INVALIDINDEX32)
        {
            return;
        }

        #ifndef EMFX_SCALE_DISABLED
        SetLocalPosition(GetLocalPosition() + (trajectoryDelta.mPosition * GetLocalScale()));
        #else
        SetLocalPosition(GetLocalPosition() + trajectoryDelta.mPosition);
        #endif

        SetLocalRotation(GetLocalRotation() * trajectoryDelta.mRotation);
    }


    // apply the currently set motion extraction delta transform to the actor instance
    void ActorInstance::ApplyMotionExtractionDelta()
    {
        ApplyMotionExtractionDelta(mTrajectoryDelta);
    }


    void ActorInstance::SetMotionExtractionEnabled(bool enabled)
    {
        SetFlag(BOOL_MOTIONEXTRACTION, enabled);
    }


    bool ActorInstance::GetMotionExtractionEnabled() const
    {
        return (mBoolFlags & BOOL_MOTIONEXTRACTION) != 0;
    }


    // update the static based aabb dimensions
    void ActorInstance::UpdateStaticBasedAABBDimensions()
    {
        // backup the transform
        Transform orgTransform = GetLocalTransform();
        //-------------------------------------

        // reset position and scale
        SetLocalPosition(AZ::Vector3::CreateZero());

        EMFX_SCALECODE
        (
            SetLocalScale(AZ::Vector3(1.0f, 1.0f, 1.0f));
        )

        // rotate over x, y and z axis
        AZ::Vector3 boxMin(FLT_MAX, FLT_MAX, FLT_MAX);
        AZ::Vector3 boxMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (uint32 axis = 0; axis < 3; axis++)
        {
            for (uint32 i = 0; i < 360; i += 45) // steps of 45 degrees
            {
                // rotate a given amount of degrees over the axis we are currently testing
                AZ::Vector3 axisVector(0.0f, 0.0f, 0.0f);
                axisVector.SetElement(axis, 1.0f);
                const float angle = static_cast<float>(i);
                SetLocalRotation(MCore::Quaternion(axisVector, MCore::Math::DegreesToRadians(angle)));

                UpdateTransformations(0.0f, true);
                UpdateMeshDeformers(0.0f);

                // calculate the aabb of this
                if (mActor->CheckIfHasMeshes(0))
                {
                    CalcMeshBasedAABB(0, &mStaticAABB);
                }
                else
                {
                    CalcNodeBasedAABB(&mStaticAABB);
                }

                // find the minimum and maximum
                const AZ::Vector3& curMin = mStaticAABB.GetMin();
                const AZ::Vector3& curMax = mStaticAABB.GetMax();
                if (curMin.GetX() < boxMin.GetX())
                {
                    boxMin.SetX(curMin.GetX());
                }
                if (curMin.GetY() < boxMin.GetY())
                {
                    boxMin.SetY(curMin.GetY());
                }
                if (curMin.GetZ() < boxMin.GetZ())
                {
                    boxMin.SetZ(curMin.GetZ());
                }
                if (curMax.GetX() > boxMax.GetX())
                {
                    boxMax.SetX(curMax.GetX());
                }
                if (curMax.GetY() > boxMax.GetY())
                {
                    boxMax.SetY(curMax.GetY());
                }
                if (curMax.GetZ() > boxMax.GetZ())
                {
                    boxMax.SetZ(curMax.GetZ());
                }
            }
        }

        mStaticAABB.SetMin(boxMin);
        mStaticAABB.SetMax(boxMax);

/*        
            // calculate the center point of the box
            const AZ::Vector3 center = mStaticAABB.CalcMiddle();

            // find the maximum of the width, height and depth
            const float maxDim = MCore::Max3<float>( mStaticAABB.CalcWidth(), mStaticAABB.CalcHeight(), mStaticAABB.CalcDepth() ) * 0.5f;

            // make width, height and depth the same as its maximum
            mStaticAABB.SetMin( center + AZ::Vector3(-maxDim, -maxDim, -maxDim) );
            mStaticAABB.SetMax( center + AZ::Vector3( maxDim,  maxDim,  maxDim) );
*/        
        //-------------------------------------

        // restore the transform
        mLocalTransform = orgTransform;
    }


    // calculate the moved static based aabb
    void ActorInstance::CalcStaticBasedAABB(MCore::AABB* outResult)
    {
        *outResult = mStaticAABB;
        EMFX_SCALECODE
        (
            outResult->SetMin(mStaticAABB.GetMin() * mGlobalTransform.mScale);
            outResult->SetMax(mStaticAABB.GetMax() * mGlobalTransform.mScale);
        )
        outResult->Translate(mGlobalTransform.mPosition);
        /*
            if (GetSelfAttachment() && GetSelfAttachment()->GetIsInfluencedByMultipleNodes())
            {
                ActorInstance* parent = GetSelfAttachment()->GetAttachToActorInstance();
                outResult->Translate( parent->GetGlobalTransform().mPosition );
            }*/
    }


    // adjust the animgraph instance
    void ActorInstance::SetAnimGraphInstance(AnimGraphInstance* instance)
    {
        mAnimGraphInstance = instance;
        UpdateDependencies();
        //GetActorManager().GetScheduler()->RecursiveRemoveActorInstance(this);
        //GetActorManager().GetScheduler()->RecursiveInsertActorInstance(this);
    }


    Actor* ActorInstance::GetActor() const
    {
        return mActor;
    }


    void ActorInstance::SetID(uint32 id)
    {
        mID = id;
    }


    MotionSystem* ActorInstance::GetMotionSystem() const
    {
        return mMotionSystem;
    }


    uint32 ActorInstance::GetLODLevel() const
    {
        return mLODLevel;
    }


    void ActorInstance::SetCustomData(void* customData)
    {
        mCustomData = customData;
    }


    void* ActorInstance::GetCustomData() const
    {
        return mCustomData;
    }


    AZ::EntityId ActorInstance::GetEntityId() const
    {
        return m_entityId;
    }


    bool ActorInstance::GetBoundsUpdateEnabled() const
    {
        return (mBoolFlags & BOOL_BOUNDSUPDATEENABLED);
    }


    float ActorInstance::GetBoundsUpdateFrequency() const
    {
        return mBoundsUpdateFrequency;
    }


    float ActorInstance::GetBoundsUpdatePassedTime() const
    {
        return mBoundsUpdatePassedTime;
    }


    ActorInstance::EBoundsType ActorInstance::GetBoundsUpdateType() const
    {
        return mBoundsUpdateType;
    }


    uint32 ActorInstance::GetBoundsUpdateItemFrequency() const
    {
        return mBoundsUpdateItemFreq;
    }


    void ActorInstance::SetBoundsUpdateFrequency(float seconds)
    {
        mBoundsUpdateFrequency = seconds;
    }


    void ActorInstance::SetBoundsUpdatePassedTime(float seconds)
    {
        mBoundsUpdatePassedTime = seconds;
    }


    void ActorInstance::SetBoundsUpdateType(EBoundsType bType)
    {
        mBoundsUpdateType = bType;
    }


    void ActorInstance::SetBoundsUpdateItemFrequency(uint32 freq)
    {
        MCORE_ASSERT(freq >= 1);
        mBoundsUpdateItemFreq = freq;
    }


    void ActorInstance::SetBoundsUpdateEnabled(bool enable)
    {
        SetFlag(BOOL_BOUNDSUPDATEENABLED, enable);
    }


    void ActorInstance::SetStaticBasedAABB(const MCore::AABB& aabb)
    {
        mStaticAABB = aabb;
    }


    void ActorInstance::GetStaticBasedAABB(MCore::AABB* outAABB)
    {
        *outAABB = mStaticAABB;
    }


    const MCore::AABB& ActorInstance::GetStaticBasedAABB() const
    {
        return mStaticAABB;
    }


    const MCore::AABB& ActorInstance::GetAABB() const
    {
        return mAABB;
    }


    void ActorInstance::SetAABB(const MCore::AABB& aabb)
    {
        mAABB = aabb;
    }


    uint32 ActorInstance::GetNumAttachments() const
    {
        return mAttachments.GetLength();
    }


    Attachment* ActorInstance::GetAttachment(uint32 nr) const
    {
        return mAttachments[nr];
    }


    bool ActorInstance::GetIsAttachment() const
    {
        return (mAttachedTo != nullptr);
    }


    ActorInstance* ActorInstance::GetAttachedTo() const
    {
        return mAttachedTo;
    }


    Attachment* ActorInstance::GetSelfAttachment() const
    {
        return mSelfAttachment;
    }


    uint32 ActorInstance::GetNumDependencies() const
    {
        return mDependencies.GetLength();
    }


    Actor::Dependency* ActorInstance::GetDependency(uint32 nr)
    {
        return &mDependencies[nr];
    }


    MorphSetupInstance* ActorInstance::GetMorphSetupInstance() const
    {
        return mMorphSetup;
    }


    void ActorInstance::SetParentGlobalTransform(const Transform& transform)
    {
        mParentGlobalTransform = transform;
    }


    const Transform& ActorInstance::GetParentGlobalTransform() const
    {
        return mParentGlobalTransform;
    }


    void ActorInstance::SetRender(bool enabled)
    {
        SetFlag(BOOL_RENDER, enabled);
    }


    bool ActorInstance::GetRender() const
    {
        return (mBoolFlags & BOOL_RENDER) != 0;
    }


    void ActorInstance::SetIsUsedForVisualization(bool enabled)
    {
        SetFlag(BOOL_USEDFORVISUALIZATION, enabled);
    }


    bool ActorInstance::GetIsUsedForVisualization() const
    {
        return (mBoolFlags & BOOL_USEDFORVISUALIZATION) != 0;
    }


    void ActorInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_OWNEDBYRUNTIME, isOwnedByRuntime);
#endif
    }


    bool ActorInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return (mBoolFlags & BOOL_OWNEDBYRUNTIME) != 0;
#else
        return true;
#endif
    }


    uint32 ActorInstance::GetThreadIndex() const
    {
        return mThreadIndex;
    }


    void ActorInstance::SetThreadIndex(uint32 index)
    {
        mThreadIndex = index;
    }


    void ActorInstance::SetTrajectoryDeltaTransform(const Transform& transform)
    {
        mTrajectoryDelta = transform;
    }


    const Transform& ActorInstance::GetTrajectoryDeltaTransform() const
    {
        return mTrajectoryDelta;
    }


    void ActorInstance::SetEyeBlinker(EyeBlinker* blinker)
    {
        mEyeBlinker = blinker;
    }


    EyeBlinker* ActorInstance::GetEyeBlinker() const
    {
        return mEyeBlinker;
    }


    AnimGraphPose* ActorInstance::RequestPose(uint32 threadIndex)
    {
        return GetEMotionFX().GetThreadData(threadIndex)->GetPosePool().RequestPose(this);
    }


    void ActorInstance::FreePose(uint32 threadIndex, AnimGraphPose* pose)
    {
        GetEMotionFX().GetThreadData(threadIndex)->GetPosePool().FreePose(pose);
    }


    void ActorInstance::SetMotionSamplingTimer(float timeInSeconds)
    {
        mMotionSamplingTimer = timeInSeconds;
    }


    void ActorInstance::SetMotionSamplingRate(float updateRateInSeconds)
    {
        mMotionSamplingRate = updateRateInSeconds;
    }


    float ActorInstance::GetMotionSamplingTimer() const
    {
        return mMotionSamplingTimer;
    }


    float ActorInstance::GetMotionSamplingRate() const
    {
        return mMotionSamplingRate;
    }


    void ActorInstance::IncreaseNumAttachmentRefs(uint8 numToIncreaseWith)
    {
        mNumAttachmentRefs += numToIncreaseWith;
        MCORE_ASSERT(mNumAttachmentRefs == 0 || mNumAttachmentRefs == 1);
    }

    void ActorInstance::DecreaseNumAttachmentRefs(uint8 numToDecreaseWith)
    {
        mNumAttachmentRefs -= numToDecreaseWith;
        MCORE_ASSERT(mNumAttachmentRefs == 0 || mNumAttachmentRefs == 1);
    }


    uint8 ActorInstance::GetNumAttachmentRefs() const
    {
        return mNumAttachmentRefs;
    }


    void ActorInstance::SetAttachedTo(ActorInstance* actorInstance)
    {
        mAttachedTo = actorInstance;
    }


    void ActorInstance::SetSelfAttachment(Attachment* selfAttachment)
    {
        mSelfAttachment = selfAttachment;
    }


    void ActorInstance::EnableFlag(uint8 flag)
    {
        mBoolFlags |= flag;
    }


    void ActorInstance::DisableFlag(uint8 flag)
    {
        mBoolFlags &= ~flag;
    }


    void ActorInstance::SetFlag(uint8 flag, bool enabled)
    {
        if (enabled)
        {
            mBoolFlags |= flag;
        }
        else
        {
            mBoolFlags &= ~flag;
        }
    }


    void ActorInstance::RecursiveSetIsVisible(bool isVisible)
    {
        SetIsVisible(isVisible);

        // recurse to all child attachments
        const uint32 numAttachments = mAttachments.GetLength();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            mAttachments[i]->GetAttachmentActorInstance()->RecursiveSetIsVisible(isVisible);
        }
    }


    void ActorInstance::RecursiveSetIsVisibleTowardsRoot(bool isVisible)
    {
        SetIsVisible(isVisible);
        if (mSelfAttachment)
        {
            mSelfAttachment->GetAttachToActorInstance()->RecursiveSetIsVisibleTowardsRoot(isVisible);
        }
    }


    void ActorInstance::SetIsEnabled(bool enabled)
    {
        SetFlag(BOOL_ENABLED, enabled);
    }


    // update the normal scale factor based on the bounds
    void ActorInstance::UpdateVisualizeScale()
    {
        mVisualizeScale = 0.0f;
        UpdateMeshDeformers(0.0f);

        MCore::AABB box;
        CalcCollisionMeshBasedAABB(0, &box);
        if (box.CheckIfIsValid())
        {
            mVisualizeScale = MCore::Max<float>(mVisualizeScale, box.CalcRadius());
        }

        CalcNodeBasedAABB(&box);
        if (box.CheckIfIsValid())
        {
            mVisualizeScale = MCore::Max<float>(mVisualizeScale, box.CalcRadius());
        }

        CalcMeshBasedAABB(0, &box);
        if (box.CheckIfIsValid())
        {
            mVisualizeScale = MCore::Max<float>(mVisualizeScale, box.CalcRadius());
        }

        mVisualizeScale *= 0.01f;
    }


    // get the normal scale factor
    float ActorInstance::GetVisualizeScale() const
    {
        return mVisualizeScale;
    }


    // manually set the visualize scale factor
    void ActorInstance::SetVisualizeScale(float factor)
    {
        mVisualizeScale = factor;
    }


    // Recursively check if we have a given attachment in the hierarchy going downwards.
    bool ActorInstance::RecursiveHasAttachment(const ActorInstance* attachmentInstance) const
    {
        if (attachmentInstance == this)
        {
            return true;
        }

        // Iterate down the chain of attachments.
        const AZ::u32 numAttachments = GetNumAttachments();
        for (AZ::u32 i = 0; i < numAttachments; ++i)
        {
            if (GetAttachment(i)->GetAttachmentActorInstance()->RecursiveHasAttachment(attachmentInstance))
            {
                return true;
            }
        }

        return false;
    }


    // Check if we can safely attach an attachment that uses the specified actor instance.
    // This will check for infinite recursion/circular chains.
    bool ActorInstance::CheckIfCanHandleAttachment(const ActorInstance* attachmentInstance) const
    {
        if (RecursiveHasAttachment(attachmentInstance) || attachmentInstance->RecursiveHasAttachment(this))
        {
            return false;
        }

        return true;
    }

}   // namespace EMotionFX
