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

#ifndef IBehaviorTreeGraft_h
#define IBehaviorTreeGraft_h

#pragma once
#include "IBehaviorTree.h"

namespace BehaviorTree
{
    struct IGraftNode
    {
        virtual ~IGraftNode() = default;
        virtual bool RunBehavior(AZ::EntityId entityId, const char* behaviorName, XmlNodeRef behaviorXmlNode) = 0;
    };

    struct IGraftModeListener
    {
        virtual void GraftModeReady(AZ::EntityId entityId) = 0;
        virtual void GraftModeInterrupted(AZ::EntityId entityId) = 0;
    };

    struct IGraftBehaviorListener
    {
        virtual void GraftBehaviorComplete(AZ::EntityId entityId) = 0;
    };

    struct IGraftManager
    {
        virtual ~IGraftManager() {}

        virtual bool RunGraftBehavior(AZ::EntityId entityId, const char* behaviorName, XmlNodeRef behaviorXml, IGraftBehaviorListener* listener) = 0;
        virtual bool RequestGraftMode(AZ::EntityId entityId, IGraftModeListener* listener) = 0;
        virtual void CancelGraftMode(AZ::EntityId entityId) = 0;
    };
}

#endif // IBehaviorTreeGraft_h
