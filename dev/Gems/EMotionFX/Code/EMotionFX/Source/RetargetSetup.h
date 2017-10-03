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

#pragma once

// include the core system
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class Pose;
    class NodeMap;
    class Actor;
    class ActorInstance;


    /**
     *
     *
     */
    class EMFX_API RetargetSetup
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(RetargetSetup, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_RETARGET);
    public:
        // per node retarget info
        struct EMFX_API RetargetNodeInfo
        {
            uint16  mSourceNodeIndex;           // the node this node points to inside the source actor
            uint16  mSourceParentNodeIndex;     // the parent inside the source actor (start of the retarget chain)
        };

        // retarget info
        struct EMFX_API RetargetInfo
        {
            ActorInstance*                  mSourceActorInstance;   // the source actor instance
            MCore::Array<RetargetNodeInfo>  mNodeInfos;             // per node retarget info
        };

        static RetargetSetup* Create();
        static RetargetSetup* Create(Actor* targetActor);

        void Init(Actor* targetActor);
        void InitFrom(RetargetSetup* otherSetup);

        void RemoveAllRetargetInfos();
        RetargetInfo& RegisterEmptyRetargetInfo();
        RetargetInfo& RegisterRetargetInfo(Actor* sourceActor, NodeMap* nodeMap);
        RetargetInfo& RegisterRetargetInfoByNodeMap(NodeMap* nodeMap);
        uint32 GetNumRetargetInfos() const;
        const RetargetInfo& GetRetargetInfo(uint32 index) const;
        RetargetInfo& GetRetargetInfo(uint32 index);
        uint32 FindRetargetInfoIndex(Actor* sourceActor) const;
        void RemoveRetargetInfoByIndex(uint32 index);
        void RemoveRetargetInfoByActor(Actor* sourceActor);
        void InitRetargetInfo(RetargetInfo& retargetInfo, Actor* sourceActor, NodeMap* nodeMap);
        void InitRetargetInfo(uint32 index, Actor* sourceActor, NodeMap* nodeMap);

        void RetargetPose(const Pose* sourcePose, Pose* targetPose, uint32 retargetInfoIndex) const;
        void RetargetPoseAuto(const Pose* sourcePose, Pose* targetPose) const;

    private:
        MCore::Array<RetargetInfo>  mRetargetInfos;         /**< The retarget infos. */
        Actor*                      mTargetActor;           /**< The actor to retarget onto. */

        void UpdateNodeParentIndices(RetargetInfo& retargetInfo);

        RetargetSetup();
        RetargetSetup(Actor* targetActor);
        ~RetargetSetup();
    };
}   // namespace EMotionFX
