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

    AttachmentSkin::AttachmentSkin(ActorInstance* attachToActorInstance, ActorInstance* attachment)
        : Attachment(attachToActorInstance, attachment)
    {
        InitJointMap();
    }

    AttachmentSkin::~AttachmentSkin()
    {
    }

    AttachmentSkin* AttachmentSkin::Create(ActorInstance* attachToActorInstance, ActorInstance* attachment)
    {
        return aznew AttachmentSkin(attachToActorInstance, attachment);
    }

    void AttachmentSkin::InitJointMap()
    {
        m_jointMap.clear();
        if (!m_attachment)
        {
            return;
        }

        Skeleton* attachmentSkeleton    = m_attachment->GetActor()->GetSkeleton();
        Skeleton* skeleton              = m_actorInstance->GetActor()->GetSkeleton();

        const uint32 numNodes = attachmentSkeleton->GetNumNodes();
        m_jointMap.reserve(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* attachmentNode = attachmentSkeleton->GetNode(i);

            // Try to find the same joint in the actor where we attach to.
            Node* sourceNode = skeleton->FindNodeByID(attachmentNode->GetID());
            if (sourceNode)
            {
                JointMapping mapping;
                mapping.m_sourceJoint = sourceNode->GetNodeIndex();
                mapping.m_targetJoint = i;
                m_jointMap.push_back(mapping);
            }
        }
        m_jointMap.shrink_to_fit();
    }
        
    void AttachmentSkin::Update()
    {
        if (!m_attachment)
        {
            return;
        }

        // Pass the parent's world space transform into the attachment.
        const Transform worldTransform = m_actorInstance->GetWorldSpaceTransform();
        m_attachment->SetParentWorldSpaceTransform(worldTransform);
    }
    
    void AttachmentSkin::UpdateJointTransforms(Pose& outPose)
    {
        if (!m_attachment)
        {
            return;
        }

        const Pose* actorInstancePose = m_actorInstance->GetTransformData()->GetCurrentPose();
        for (const JointMapping& mapping : m_jointMap)
        {
            outPose.SetModelSpaceTransform(mapping.m_targetJoint, actorInstancePose->GetModelSpaceTransform(mapping.m_sourceJoint));
        }
    }
}   // namespace EMotionFX
