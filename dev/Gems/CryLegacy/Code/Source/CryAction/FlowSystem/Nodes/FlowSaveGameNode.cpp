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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "CryAction.h"
#include "IGame.h"
#include "ICheckPointSystem.h"

//////////////////////////////////////////////////////////////////////////

const static int NUM_SAVE_LOAD_ENTITIES = 5;

class CFlowNodeCheckpoint
    : public CFlowBaseNode<eNCT_Instanced>
    , ICheckpointListener
{
public:
    CFlowNodeCheckpoint(SActivationInfo* pActInfo)
    {
        m_iSaveId = m_iLoadId = 0;
        memset(m_saveLoadEntities, 0, sizeof(m_saveLoadEntities));
        CCryAction::GetCryAction()->GetICheckpointSystem()->RegisterListener(this);
    }

    virtual ~CFlowNodeCheckpoint()
    {
        CCryAction::GetCryAction()->GetICheckpointSystem()->RemoveListener(this);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNodeCheckpoint(pActInfo); }

    // ICheckpointListener
    void OnSave(SCheckpointData* pCheckpoint, ICheckpointSystem* pSystem)
    {
        CRY_ASSERT(pSystem);

        //output onSave
        m_iSaveId = (int)(pCheckpoint->m_checkPointId);

        //this saves designer-controlled entities (breakables, destructables ...)
        for (int i = 0; i < NUM_SAVE_LOAD_ENTITIES; ++i)
        {
            //this comes with the ICheckpointListener
            pSystem->SaveExternalEntity(m_saveLoadEntities[i]);
        }
    }
    void OnLoad(SCheckpointData* pCheckpoint, ICheckpointSystem* pSystem)
    {
        //output onLoad
        m_iLoadId = (int)(pCheckpoint->m_checkPointId);

        //loading external entities happens inside the CheckpointSystem
    }
    //~ICheckpointListener

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    enum
    {
        EIP_LoadLastCheckpoint = 0,
        EIP_SaveLoadEntityStart
    };

    enum
    {
        EOP_OnSave = 0,
        EOP_OnLoad,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("LoadLastCheckpoint", _HELP("Load the last checkpoint which was saved during the current session.")),
            InputPortConfig<FlowEntityId>("SaveLoadEntity1", _HELP("Save and load this entity when a checkpoint is triggered.")),
            InputPortConfig<FlowEntityId>("SaveLoadEntity2", _HELP("Save and load this entity when a checkpoint is triggered.")),
            InputPortConfig<FlowEntityId>("SaveLoadEntity3", _HELP("Save and load this entity when a checkpoint is triggered.")),
            InputPortConfig<FlowEntityId>("SaveLoadEntity4", _HELP("Save and load this entity when a checkpoint is triggered.")),
            InputPortConfig<FlowEntityId>("SaveLoadEntity5", _HELP("Save and load this entity when a checkpoint is triggered.")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int> ("SaveDone", _HELP("Triggered when saved")),
            OutputPortConfig<int> ("LoadDone", _HELP("Triggered when loaded")),
            {0}
        };
        config.sDescription = _HELP("Checkpoint System Output");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        //initialization event
        switch (event)
        {
        case eFE_Initialize:
            //since we cannot send data to the flowgraph at any time, we need to wait for updates ..
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            break;
        case eFE_Activate:
            if (IsPortActive(pActInfo, EIP_LoadLastCheckpoint))
            {
                CCryAction::GetCryAction()->GetICheckpointSystem()->LoadLastCheckpoint();
            }
            break;
        case eFE_Update:
            if (m_iSaveId > 0)
            {
                ActivateOutput(pActInfo, EOP_OnSave, m_iSaveId);
                m_iSaveId = 0;
            }
            if (m_iLoadId > 0)
            {
                ActivateOutput(pActInfo, EOP_OnLoad, m_iLoadId);
                m_iLoadId = 0;
            }

            for (int entityIndex = 0, portIndex = EIP_SaveLoadEntityStart; portIndex < (EIP_SaveLoadEntityStart + NUM_SAVE_LOAD_ENTITIES); ++portIndex, ++entityIndex)
            {
                m_saveLoadEntities[entityIndex] = GetPortEntityId(pActInfo, portIndex);
            }
            /*
                        m_saveLoadEntities[0] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity1);
                        m_saveLoadEntities[1] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity2);
                        m_saveLoadEntities[2] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity3);
                        m_saveLoadEntities[3] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity4);
                        m_saveLoadEntities[4] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity5);
            */
            break;
        }
    }

private:

    int             m_iSaveId, m_iLoadId;
    FlowEntityId    m_saveLoadEntities[NUM_SAVE_LOAD_ENTITIES];
};
//////////////////////////////////////////////////////////////////////////
// Register nodes

FLOW_NODE_BLACKLISTED("System:CheckpointSystem", CFlowNodeCheckpoint);

