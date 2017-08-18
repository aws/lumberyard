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

#ifndef CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEMANAGER_H
#pragma once

#include <IFlowSystem.h>
#include "Sequence.h"

namespace AIActionSequence
{
    class SequenceManager
        : public ISequenceManager
    {
    public:
        SequenceManager()
            : m_sequenceIdCounter(1)
        {
        }

        ~SequenceManager()
        {
        }

        void Reset();

        // AIActionSequence::ISequenceManager
        virtual bool RegisterSequence(EntityId entityId, TFlowNodeId startNodeId, SequenceProperties sequenceProperties, IFlowGraph* flowGraph);
        virtual void UnregisterSequence(SequenceId sequenceId);

        virtual void StartSequence(SequenceId sequenceId);
        virtual void CancelSequence(SequenceId sequenceId);
        virtual bool IsSequenceActive(SequenceId sequenceId);

        virtual void SequenceBehaviorReady(EntityId entityId);
        virtual void SequenceInterruptibleBehaviorLeft(EntityId entityId);
        virtual void SequenceNonInterruptibleBehaviorLeft(EntityId entityId);
        virtual void AgentDisabled(EntityId entityId);

        virtual void RequestActionStart(SequenceId sequenceId, TFlowNodeId actionNodeId);
        virtual void ActionCompleted(SequenceId sequenceId);
        virtual void SetBookmark(SequenceId sequenceId, TFlowNodeId bookmarkNodeId);
        // ~AIActionSequence::ISequenceManager

    private:
        SequenceId GenerateUniqueSequenceId();
        Sequence* GetSequence(SequenceId sequenceId);
        void CancelActiveSequencesForThisEntity(EntityId entityId);

        typedef std::map<SequenceId, Sequence> SequenceMap;
        SequenceMap  m_registeredSequences;
        SequenceId   m_sequenceIdCounter;
    };
} // namespace AIActionSequence

#endif // CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEMANAGER_H
