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
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#undef GetObject

namespace BehaviorTree
{
    //////////////////////////////////////////////////////////////////////////
    /// Use this bus to tell the BehaviorTreeManager to start and stop behavior trees associated with an entity.
    //////////////////////////////////////////////////////////////////////////
    class BehaviorTreeManagerRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        /// Use this message to create a behavior tree and associate it with an entityId.
        /// This method will call Stop Behavior Tree on any tree already running for the entity.
        /// treeName is a path to a behavior tree asset (xml).
        /// It uses legacy serialization to load the xml.
        /// It will look in the following paths for your asset:
        ///     "Scripts/AI/BehaviorTrees/"
        ///     "libs/ai/behavior_trees/"
        ///     "@assets@/"
        /// An error will be logged and false will be returned if it could not find
        /// or serialize the requested behavior tree.
        //////////////////////////////////////////////////////////////////////////
        virtual bool StartBehaviorTree(const AZ::EntityId& entityId, const char* treeName) = 0;


        //////////////////////////////////////////////////////////////////////////
        /// Use this message to end a behavior tree associated with an entity.
        /// This will cause it to Terminate the behavior tree, and then erase it from 
        /// the list of updating behavior trees
        //////////////////////////////////////////////////////////////////////////
        virtual void StopBehaviorTree(const AZ::EntityId& entityId) = 0;


        virtual AZStd::vector<AZ::Crc32> GetVariableNameCrcs(const AZ::EntityId& entityId) = 0;
        virtual bool GetVariableValue(const AZ::EntityId& entityId, AZ::Crc32 variableNameCrc) = 0;
        virtual void SetVariableValue(const AZ::EntityId& entityId, AZ::Crc32 variableNameCrc, bool newValue) = 0;
    };

    using BehaviorTreeManagerRequestBus = AZ::EBus<BehaviorTreeManagerRequests>;
}