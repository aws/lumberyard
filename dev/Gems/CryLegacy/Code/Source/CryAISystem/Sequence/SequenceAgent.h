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

#ifndef CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEAGENT_H
#define CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEAGENT_H
#pragma once

namespace AIActionSequence
{
    class SequenceAgent
    {
    public:
        SequenceAgent(EntityId entityID)
            : m_aiActor(NULL)
        {
            IEntity* entity = gEnv->pEntitySystem->GetEntity(entityID);
            if (!entity)
            {
                return;
            }

            IAIObject* aiObject = entity->GetAI();
            if (!aiObject)
            {
                return;
            }

            m_aiActor = aiObject->CastToCAIActor();
        }

        bool ValidateAgent() const
        {
            assert(m_aiActor);
            return m_aiActor != NULL;
        }

        void SetSequenceBehavior(bool interruptible)
        {
            if (!ValidateAgent())
            {
                return;
            }

            if (interruptible)
            {
                m_aiActor->SetBehaviorVariable("ExecuteInterruptibleSequence", true);
            }
            else
            {
                m_aiActor->SetBehaviorVariable("ExecuteSequence", true);
            }
        }

        bool IsRunningSequenceBehavior(bool interruptible)
        {
            if (!ValidateAgent())
            {
                return false;
            }

            IAIActorProxy* aiActorProxy = m_aiActor->GetProxy();
            assert(aiActorProxy);
            if (!aiActorProxy)
            {
                return false;
            }

            const char* currentBehavior = aiActorProxy->GetCurrentBehaviorName();
            if (!currentBehavior)
            {
                return false;
            }

            if (interruptible)
            {
                return !strcmp(currentBehavior, "ExecuteInterruptibleSequence");
            }
            else
            {
                return !strcmp(currentBehavior, "ExecuteSequence");
            }
        }

        void ClearSequenceBehavior() const
        {
            if (!ValidateAgent() && m_aiActor->IsEnabled())
            {
                return;
            }

            m_aiActor->SetBehaviorVariable("ExecuteSequence", false);
            m_aiActor->SetBehaviorVariable("ExecuteInterruptibleSequence", false);
        }

        void ClearGoalPipe() const
        {
            if (!ValidateAgent())
            {
                return;
            }

            if (IPipeUser* pipeUser = m_aiActor->CastToIPipeUser())
            {
                pipeUser->SelectPipe(0, "Empty", 0, 0, true);
            }
        }

        IPipeUser* GetPipeUser()
        {
            if (!ValidateAgent())
            {
                return NULL;
            }

            return m_aiActor->CastToIPipeUser();
        }

        IEntity* GetEntity()
        {
            if (!ValidateAgent())
            {
                return NULL;
            }

            return m_aiActor->GetEntity();
        }

        EntityId GetEntityId() const
        {
            if (!ValidateAgent())
            {
                return 0;
            }

            return m_aiActor->GetEntityID();
        }

        void SendSignal(const char* signalName, IEntity* pSender)
        {
            if (!ValidateAgent())
            {
                return;
            }

            IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
            const int goalPipeId = gEnv->pAISystem->AllocGoalPipeId();
            pData->iValue = goalPipeId;
            m_aiActor->SetSignal(10, signalName, pSender, pData);
        }

    private:
        SequenceAgent()
            : m_aiActor(NULL)
        {
        }

        CAIActor* m_aiActor;
    };
} // namespace AIActionSequence

#endif // CRYINCLUDE_CRYAISYSTEM_SEQUENCE_SEQUENCEAGENT_H
