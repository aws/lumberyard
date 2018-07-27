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
#include "AttachmentSkin.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "Node.h"
#include "TransformData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttachmentSkin, AttachmentAllocator, 0)

    // constructor for a skin attachment
    AttachmentSkin::AttachmentSkin(ActorInstance* attachToActorInstance, ActorInstance* attachment)
        : Attachment(attachToActorInstance, attachment)
    {
        InitNodeMap();
    }


    // destructor
    AttachmentSkin::~AttachmentSkin()
    {
    }


    // create a skin attachment
    AttachmentSkin* AttachmentSkin::Create(ActorInstance* attachToActorInstance, ActorInstance* attachment)
    {
        return aznew AttachmentSkin(attachToActorInstance, attachment);
    }


    // initialize the node map that links nodes in this attachment to the parent actor where it is attached to
    void AttachmentSkin::InitNodeMap()
    {
        if (mAttachment == nullptr)
        {
            return;
        }

        Skeleton* attachmentSkeleton    = mAttachment->GetActor()->GetSkeleton();
        Skeleton* skeleton              = mActorInstance->GetActor()->GetSkeleton();

        // precalculate for each node in the attachment, the node where
        // to get the transformation from inside the actor where it will be attached to
        // so if the "upper arm" bone of the main/parent actor rotates, the node with the same name
        // inside the attachment will also be rotated the same way
        const uint32 numNodes = attachmentSkeleton->GetNumNodes();
        mNodeMap.Resize(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the node inside the attachment
            Node* attachmentNode = attachmentSkeleton->GetNode(i);

            // try to find the same node in the actor where we attach to
            Node* sourceNode = skeleton->FindNodeByID(attachmentNode->GetID());
            mNodeMap[i].mSourceNode = sourceNode; // NOTE: this can also be nullptr!

            // init the global matrices on identity
            mNodeMap[i].mLocalMatrix.Identity();
            mNodeMap[i].mGlobalMatrix.Identity();
            mNodeMap[i].mLocalTransform.Identity();
        }
    }


    // update the node transformations
    void AttachmentSkin::UpdateNodeTransforms()
    {
        // when we have no nodes to update, there is nothing to do
        if (mNodeMap.GetLength() == 0)
        {
            return;
        }

        // get the number of nodes in this attachment (and so also in the map)
        const uint32 numNodes = mNodeMap.GetLength();
        MCORE_ASSERT(numNodes == mAttachment->GetActor()->GetNumNodes());

        // update all the node matrices
        TransformData* attachmentTransformData  = mAttachment->GetTransformData();
        MCore::Matrix* localMatrices            = attachmentTransformData->GetLocalMatrices();
        MCore::Matrix* globalMatrices           = attachmentTransformData->GetGlobalInclusiveMatrices();
        Pose* attachmentPose                    = attachmentTransformData->GetCurrentPose();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // skip nodes that don't exist inside the actor where we are attached to
            if (mNodeMap[i].mSourceNode == nullptr)
            {
                continue;
            }

            // copy the matrix
            localMatrices[i]    = mNodeMap[i].mLocalMatrix;
            globalMatrices[i]   = mNodeMap[i].mGlobalMatrix;
            attachmentPose->SetLocalTransform(i, mNodeMap[i].mLocalTransform);
        }
    }


    // default attachment update
    void AttachmentSkin::Update()
    {
        if (mAttachment == nullptr || mNodeMap.GetLength() == 0)
        {
            return;
        }

        const Transform& globalTransform = mActorInstance->GetGlobalTransform();
        mAttachment->SetParentGlobalTransform(globalTransform);

        const TransformData* transformData = mActorInstance->GetTransformData();

        // set all transform matrices in the attachment
        const MCore::Matrix* localMatrices  = transformData->GetLocalMatrices();
        const MCore::Matrix* globalMatrices = transformData->GetGlobalInclusiveMatrices();
        const Pose* currentPose             = transformData->GetCurrentPose();
        const uint32 numAttachmentNodes     = GetAttachmentActorInstance()->GetActor()->GetNumNodes();
        for (uint32 a = 0; a < numAttachmentNodes; ++a)
        {
            NodeMapping& nodeMap = mNodeMap[a];
            Node* sourceNode = nodeMap.mSourceNode;
            if (sourceNode == nullptr) // skip nodes that aren't used
            {
                continue;
            }

            const uint32 nodeIndex = sourceNode->GetNodeIndex();
            nodeMap.mLocalMatrix    = localMatrices[nodeIndex];
            nodeMap.mGlobalMatrix   = globalMatrices[nodeIndex];
            nodeMap.mLocalTransform = currentPose->GetLocalTransform(nodeIndex);
        }
    }
}   // namespace EMotionFX
