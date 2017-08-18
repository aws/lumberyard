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

// Description : This is the public interface for AI Action Sequence system.


#ifndef CRYINCLUDE_CRYCOMMON_IAIACTIONSEQUENCE_H
#define CRYINCLUDE_CRYCOMMON_IAIACTIONSEQUENCE_H
#pragma once

#include <IFlowSystem.h>    // for TFlowNodeId typedef and IFlowGraph


namespace AIActionSequence
{
    typedef uint32 SequenceId;

    //////////////////////////////////////////////////////////////////////////

    enum SequenceEvent
    {
        StartSequence,
        StartAction,
        BookmarkSet,
        TriggerBookmark,
        SequenceStopped,
        SequenceEventCount
    };

    //////////////////////////////////////////////////////////////////////////

    class SequenceActionBase
    {
    public:
        SequenceActionBase()
            : m_assignedSequenceId(0)
            , m_assignedEntityId(0)
        {}

        virtual void HandleSequenceEvent(SequenceEvent event) {}

        void AssignSequence(SequenceId sequenceId, EntityId entityId)
        {
            m_assignedSequenceId = sequenceId;
            m_assignedEntityId = entityId;
        }

        void ClearSequence()
        {
            m_assignedSequenceId = 0;
            m_assignedEntityId = 0;
        }

        SequenceId GetAssignedSequenceId() const
        {
            return m_assignedSequenceId;
        }

        EntityId GetAssignedEntityId() const
        {
            return m_assignedEntityId;
        }

    private:
        SequenceId m_assignedSequenceId;
        EntityId m_assignedEntityId;
    };

    //////////////////////////////////////////////////////////////////////////

    struct SequenceProperties
    {
        SequenceProperties() {}
        SequenceProperties(bool _interruptible, bool _resumeAfterInterrupt)
            : interruptible(_interruptible)
            , resumeAfterInterrupt(_resumeAfterInterrupt)
        {}

        bool interruptible;
        bool resumeAfterInterrupt;
    };

    //////////////////////////////////////////////////////////////////////////

    struct ISequenceManager
    {
        virtual ~ISequenceManager() {}

        virtual bool RegisterSequence(EntityId entityId, TFlowNodeId startNodeId, SequenceProperties sequenceProperties, IFlowGraph* flowGraph) = 0;
        virtual void UnregisterSequence(SequenceId sequenceId) = 0;

        virtual void StartSequence(SequenceId sequenceId) = 0;
        virtual void CancelSequence(SequenceId sequenceId) = 0;
        virtual bool IsSequenceActive(SequenceId sequenceId) = 0;

        virtual void SequenceBehaviorReady(EntityId entityId) = 0;
        virtual void SequenceInterruptibleBehaviorLeft(EntityId entityId) = 0;
        virtual void SequenceNonInterruptibleBehaviorLeft(EntityId entityId) = 0;
        virtual void AgentDisabled(EntityId entityId) = 0;

        virtual void RequestActionStart(SequenceId sequenceId, TFlowNodeId actionNodeId) = 0;
        virtual void ActionCompleted(SequenceId sequenceId) = 0;
        virtual void SetBookmark(SequenceId sequenceId, TFlowNodeId bookmarkNodeId) = 0;
    };
} // namespace AIActionSequence


#endif // CRYINCLUDE_CRYCOMMON_IAIACTIONSEQUENCE_H
