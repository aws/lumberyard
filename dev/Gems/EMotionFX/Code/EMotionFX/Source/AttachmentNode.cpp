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
#include "AttachmentNode.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttachmentNode, AttachmentAllocator, 0)

    // constructor for non-deformable attachments
    AttachmentNode::AttachmentNode(ActorInstance* attachToActorInstance, uint32 attachToNodeIndex, ActorInstance* attachment)
        : Attachment(attachToActorInstance, attachment)
    {
        // make sure the node index is valid
        MCORE_ASSERT(attachToNodeIndex < attachToActorInstance->GetNumNodes());
        mAttachedToNode = attachToNodeIndex;
    }


    // destructor
    AttachmentNode::~AttachmentNode()
    {
    }


    // create it
    AttachmentNode* AttachmentNode::Create(ActorInstance* attachToActorInstance, uint32 attachToNodeIndex, ActorInstance* attachment)
    {
        return aznew AttachmentNode(attachToActorInstance, attachToNodeIndex, attachment);
    }


    // update the attachment
    void AttachmentNode::Update()
    {
        if (mAttachment == nullptr)
        {
            return;
        }

        const Transform& globalTransform = mActorInstance->GetTransformData()->GetGlobalInclusiveMatrix(mAttachedToNode);
        mAttachment->SetParentGlobalTransform(globalTransform);
    }


    uint32 AttachmentNode::GetAttachToNodeIndex() const
    {
        return mAttachedToNode;
    }
}   // namespace EMotionFX
