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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef BehaviorTreeGraft_h
#define BehaviorTreeGraft_h

#pragma once

#include <BehaviorTree/IBehaviorTreeGraft.h>

namespace BehaviorTree
{
    class GraftManager
        : public IGraftManager
    {
    public:

        GraftManager() {}
        ~GraftManager() {}

        void Reset();

        void GraftNodeReady(AZ::EntityId entityId, IGraftNode* graftNode);
        void GraftNodeTerminated(AZ::EntityId entityId);
        void GraftBehaviorComplete(AZ::EntityId entityId);

        // IGraftManager
        virtual bool RunGraftBehavior(AZ::EntityId entityId, const char* behaviorName, XmlNodeRef behaviorXml, IGraftBehaviorListener* listener) override;
        virtual bool RequestGraftMode(AZ::EntityId entityId, IGraftModeListener* listener) override;
        virtual void CancelGraftMode(AZ::EntityId entityId) override;
        // ~IGraftManager

    private:

        typedef VectorMap<AZ::EntityId, IGraftModeListener*> GraftModeRequestsContainer;
        GraftModeRequestsContainer m_graftModeRequests;

        typedef VectorMap<AZ::EntityId, IGraftBehaviorListener*> GraftBehaviorRequestsContainer;
        GraftBehaviorRequestsContainer m_graftBehaviorRequests;

        typedef VectorMap<AZ::EntityId, IGraftNode*> ActiveGraftNodesContainer;
        ActiveGraftNodesContainer m_activeGraftNodes;
    };
}



#endif // BehaviorTreeGraft_h