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
#include "RetargetSetup.h"
#include "Pose.h"
#include "NodeMap.h"
#include "Actor.h"
#include "Node.h"
#include "ActorInstance.h"


namespace EMotionFX
{
    //---------------------------------------------------------------------
    // class RetargetSetup
    //---------------------------------------------------------------------

    // default constructor
    RetargetSetup::RetargetSetup()
    {
        mTargetActor = nullptr;
    }

    // constructor
    RetargetSetup::RetargetSetup(Actor* targetActor)
    {
        mTargetActor = nullptr;
        Init(targetActor);
    }


    // destructor
    RetargetSetup::~RetargetSetup()
    {
        RemoveAllRetargetInfos();
    }


    // create
    RetargetSetup* RetargetSetup::Create()
    {
        return new RetargetSetup();
    }


    // create for an actor
    RetargetSetup* RetargetSetup::Create(Actor* targetActor)
    {
        return new RetargetSetup(targetActor);
    }


    // init for a given target actor
    void RetargetSetup::Init(Actor* targetActor)
    {
        RemoveAllRetargetInfos();
        mTargetActor = targetActor;
    }


    // init from another setup
    void RetargetSetup::InitFrom(RetargetSetup* otherSetup)
    {
        RemoveAllRetargetInfos();

        // copy the retarget infos
        const uint32 numInfos = mRetargetInfos.GetLength();
        mRetargetInfos.Resize(numInfos);
        for (uint32 i = 0; i < numInfos; ++i)
        {
            mRetargetInfos[i].mNodeInfos            = otherSetup->mRetargetInfos[i].mNodeInfos;
            mRetargetInfos[i].mSourceActorInstance  = ActorInstance::Create(otherSetup->mRetargetInfos[i].mSourceActorInstance->GetActor());
        }
    }


    // remove all retarget infos
    void RetargetSetup::RemoveAllRetargetInfos()
    {
        const uint32 numInfos = mRetargetInfos.GetLength();
        for (uint32 i = 0; i < numInfos; ++i)
        {
            if (mRetargetInfos[i].mSourceActorInstance)
            {
                mRetargetInfos[i].mSourceActorInstance->Destroy();
            }
        }
        mRetargetInfos.Clear();
    }


    // get the number of registered retarget infos
    uint32 RetargetSetup::GetNumRetargetInfos() const
    {
        return mRetargetInfos.GetLength();
    }


    // get a given retarget info, in const mode
    const RetargetSetup::RetargetInfo& RetargetSetup::GetRetargetInfo(uint32 index) const
    {
        return mRetargetInfos[index];
    }


    // get a given retarget info
    RetargetSetup::RetargetInfo& RetargetSetup::GetRetargetInfo(uint32 index)
    {
        return mRetargetInfos[index];
    }


    // find the retarget info for a given source actor
    uint32 RetargetSetup::FindRetargetInfoIndex(Actor* sourceActor) const
    {
        const uint32 numInfos = mRetargetInfos.GetLength();
        for (uint32 i = 0; i < numInfos; ++i)
        {
            if (mRetargetInfos[i].mSourceActorInstance->GetActor() == sourceActor)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // remove a given retarget info
    void RetargetSetup::RemoveRetargetInfoByIndex(uint32 index)
    {
        if (mRetargetInfos[index].mSourceActorInstance)
        {
            mRetargetInfos[index].mSourceActorInstance->Destroy();
        }

        mRetargetInfos.Remove(index);
    }


    // register an empty retarget info
    RetargetSetup::RetargetInfo& RetargetSetup::RegisterEmptyRetargetInfo()
    {
        // create the new retarget info
        mRetargetInfos.AddEmpty();
        RetargetInfo& retargetInfo = mRetargetInfos.GetLast();
        retargetInfo.mSourceActorInstance = nullptr;
        return retargetInfo;
    }


    // remove a given retarget info by source actor pointer
    void RetargetSetup::RemoveRetargetInfoByActor(Actor* sourceActor)
    {
        const uint32 index = FindRetargetInfoIndex(sourceActor);
        if (index != MCORE_INVALIDINDEX32)
        {
            RemoveRetargetInfoByIndex(index);
        }
    }


    // register a given retarget info using a node map and a source actor
    RetargetSetup::RetargetInfo& RetargetSetup::RegisterRetargetInfo(Actor* sourceActor, NodeMap* nodeMap)
    {
        // create the new retarget info
        mRetargetInfos.AddEmpty();
        RetargetInfo& retargetInfo = mRetargetInfos.GetLast();
        retargetInfo.mSourceActorInstance = nullptr;

        InitRetargetInfo(mRetargetInfos.GetLength() - 1, sourceActor, nodeMap);

        return retargetInfo;
    }


    // init a given retarget info by reference
    void RetargetSetup::InitRetargetInfo(RetargetInfo& retargetInfo, Actor* sourceActor, NodeMap* nodeMap)
    {
        uint32 index = MCORE_INVALIDINDEX32;
        for (uint32 i = 0; i < mRetargetInfos.GetLength() && index == MCORE_INVALIDINDEX32; ++i)
        {
            if (&mRetargetInfos[i] == &retargetInfo)
            {
                index = i;
            }
        }

        // if we didnt find a match
        if (index == MCORE_INVALIDINDEX32) // this shouldn't happen
        {
            MCORE_ASSERT(false);
            return;
        }

        InitRetargetInfo(index, sourceActor, nodeMap);
    }


    // init a given retarget info
    void RetargetSetup::InitRetargetInfo(uint32 index, Actor* sourceActor, NodeMap* nodeMap)
    {
        RetargetInfo& retargetInfo = mRetargetInfos[index];

        // clean existing data
        if (retargetInfo.mSourceActorInstance)
        {
            retargetInfo.mSourceActorInstance->Destroy();
        }

        retargetInfo.mNodeInfos.Clear(false);

        // create the actor instance so we can sample transforms etc
        retargetInfo.mSourceActorInstance = ActorInstance::Create(sourceActor);
        retargetInfo.mSourceActorInstance->SetRender(false);
        //retargetInfo.mSourceActorInstance->SetVisible(false); // TODO: should we force it to invisible?

        if (nodeMap)
        {
            // for every node in the target actor
            const uint32 numTargetNodes = mTargetActor->GetNumNodes();
            retargetInfo.mNodeInfos.Resize(numTargetNodes);
            for (uint32 i = 0; i < numTargetNodes; ++i)
            {
                // try to find the matching node name for this current node
                const char* sourceActorNodeName = nodeMap->FindSecondName(mTargetActor->GetSkeleton()->GetNode(i)->GetName());
                if (sourceActorNodeName == nullptr)
                {
                    retargetInfo.mNodeInfos[i].mSourceNodeIndex = MCORE_INVALIDINDEX16;
                    continue;
                }

                // find the source node and store it in the map
                // TODO: we could probably use a name ID find here, instead of by name string
                const Node* sourceNode = sourceActor->GetSkeleton()->FindNodeByName(sourceActorNodeName);
                if (sourceNode)
                {
                    retargetInfo.mNodeInfos[i].mSourceNodeIndex = static_cast<uint16>(sourceNode->GetNodeIndex());
                }
                else
                {
                    retargetInfo.mNodeInfos[i].mSourceNodeIndex = MCORE_INVALIDINDEX16;
                }
            }
        }

        // update the parent indices of the node infos
        UpdateNodeParentIndices(retargetInfo);
    }


    // register a given retarget info using a node map
    RetargetSetup::RetargetInfo& RetargetSetup::RegisterRetargetInfoByNodeMap(NodeMap* nodeMap)
    {
        Actor* sourceActor = nodeMap->GetSourceActor();
        MCORE_ASSERT(sourceActor);
        return RegisterRetargetInfo(sourceActor, nodeMap);
    }


    // update the parent indices
    void RetargetSetup::UpdateNodeParentIndices(RetargetInfo& retargetInfo)
    {
        // for all nodes in the target actor
        const uint32 numTargetNodes = retargetInfo.mNodeInfos.GetLength();
        for (uint32 i = 0; i < numTargetNodes; ++i)
        {
            RetargetNodeInfo& retargetNodeInfo = retargetInfo.mNodeInfos[i];

            // if the current node isn't targeting another node
            const uint32 retargetNodeIndex = retargetNodeInfo.mSourceNodeIndex;
            if (retargetNodeIndex == MCORE_INVALIDINDEX16)
            {
                retargetNodeInfo.mSourceParentNodeIndex = MCORE_INVALIDINDEX16;
                continue;
            }

            // try to find a parent in the source actor
            Node* targetNode = mTargetActor->GetSkeleton()->GetNode(i);
            bool parentFound = false;
            while (parentFound == false)
            {
                // if the current parent is nullptr, so if the current node is a root node
                Node* parentNode = targetNode->GetParentNode();
                if (parentNode == nullptr)
                {
                    const uint16 currentParentIndex = retargetInfo.mNodeInfos[ targetNode->GetNodeIndex() ].mSourceNodeIndex;
                    if (currentParentIndex != MCORE_INVALIDINDEX16)
                    {
                        retargetNodeInfo.mSourceParentNodeIndex = currentParentIndex;
                        parentFound = true;
                        break;
                    }

                    retargetNodeInfo.mSourceParentNodeIndex = MCORE_INVALIDINDEX16;
                    break;
                }

                const uint32 targetParentIndex = parentNode->GetNodeIndex();
                uint16 currentParentIndex = retargetInfo.mNodeInfos[ targetParentIndex ].mSourceNodeIndex;
                if (currentParentIndex != MCORE_INVALIDINDEX16)
                {
                    retargetNodeInfo.mSourceParentNodeIndex = currentParentIndex;
                    parentFound = true;
                    break;
                }

                // move up the hierarchy
                targetNode = parentNode;
            }
        }
    }


    // retarget a given pose, auto detect what retarget info to use based on the source pose's actor
    void RetargetSetup::RetargetPoseAuto(const Pose* sourcePose, Pose* targetPose) const
    {
        const uint32 index = FindRetargetInfoIndex(sourcePose->GetActorInstance()->GetActor());
        MCORE_ASSERT(index != MCORE_INVALIDINDEX32); // the actor cannot be found, we have not yet registered a retarget info for retargeting between the target actor and this source actor
        RetargetPose(sourcePose, targetPose, index);
    }


    // retarget a given pose
    void RetargetSetup::RetargetPose(const Pose* sourcePose, Pose* targetPose, uint32 retargetInfoIndex) const
    {
        MCORE_UNUSED(sourcePose);
        MCORE_UNUSED(retargetInfoIndex);
        targetPose->InitFromBindPose(targetPose->GetActorInstance());

        MCORE_ASSERT(targetPose->GetNumTransforms() == mTargetActor->GetNumNodes());
        const RetargetInfo& retargetInfo = mRetargetInfos[ retargetInfoIndex ];
        const uint32 numNodes = mTargetActor->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            const RetargetNodeInfo& retargetNodeInfo = retargetInfo.mNodeInfos[n];

            // if we cannot possibly know the transform, just keep the bind pose transform
            if (retargetNodeInfo.mSourceNodeIndex == MCORE_INVALIDINDEX16 || retargetNodeInfo.mSourceParentNodeIndex == MCORE_INVALIDINDEX16)
            {
                continue;
            }

            // TODO: perform the actual retargeting
        }
    }
}   // namespace EMotionFX
