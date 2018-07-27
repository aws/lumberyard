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
#include "FlowStartNode.h"
#include "CryAction.h"

// if this is defined we use the old behaviour.
// means: In Editor switching from Game mode back to Editor
//        the node outputs an 'Output event (which is bogus...)
#define VS2_SAFE_PLAY_USE_OLD_BEHAVIOUR
#undef  VS2_SAFE_PLAY_USE_OLD_BEHAVIOUR  // default: use new behaviour

CFlowStartNode::CFlowStartNode(SActivationInfo* pActInfo)
{
    m_refs = 0;
    m_bActivated = true;
    SetActivation(pActInfo, false);
}

void CFlowStartNode::AddRef()
{
    ++m_refs;
}

void CFlowStartNode::Release()
{
    if (0 == --m_refs)
    {
        delete this;
    }
}

IFlowNodePtr CFlowStartNode::Clone(SActivationInfo* pActInfo)
{
    CFlowStartNode* pClone = new CFlowStartNode(pActInfo);
    pClone->SetActivation(pActInfo, m_bActivated);
    return pClone;
}

void CFlowStartNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig<bool> ("InGame", true, _HELP("Start will Trigger in PureGameMode")),
        InputPortConfig<bool> ("InEditor", true, _HELP("Start will Trigger in EditorGameMode")),
        {0}
    };
    static const SOutputPortConfig outputs[] = {
        OutputPortConfig<bool>("output"),
        {0}
    };

    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.SetCategory(EFLN_APPROVED);
}

bool CFlowStartNode::MayTrigger(SActivationInfo* pActInfo)
{
    const bool isEditor = gEnv->IsEditor();
    const bool canTriggerInGame   = *(pActInfo->pInputPorts[0].GetPtr<bool>());
    const bool canTriggerInEditor = *(pActInfo->pInputPorts[1].GetPtr<bool>());
    const bool canTrigger = (isEditor && canTriggerInEditor) || (!isEditor && canTriggerInGame);
    return canTrigger;
}

void CFlowStartNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    SFlowAddress addr(pActInfo->myID, 0, true);

#ifdef VS2_SAFE_PLAY_USE_OLD_BEHAVIOUR
    switch (event)
    {
    case eFE_Update:
    {
        if (!CCryAction::GetCryAction()->IsGameStarted())
        {
            return;
        }

        pActInfo->pGraph->ActivatePort(addr, true);
        SetActivation(pActInfo, true);
    }
    break;
    case eFE_Initialize:
        pActInfo->pGraph->ActivatePort(addr, false);
        SetActivation(pActInfo, false);
        break;
    }
#else   // new behaviour
    switch (event)
    {
    case eFE_Update:
    {
        // when we're in pure game mode
        if (!gEnv->IsEditor())
        {
            if (!CCryAction::GetCryAction()->IsGameStarted())
            {
                return;     // not yet started
            }
        }
        // else: in editor mode or game has been started
        // in editor mode we regard the game as initialized as soon as
        // we receive the first update...
        if (MayTrigger(pActInfo))
        {
            pActInfo->pGraph->ActivatePort(addr, true);
        }

        SetActivation(pActInfo, true);
    }
    break;
    case eFE_Initialize:
        if (MayTrigger(pActInfo))
        {
            pActInfo->pGraph->ActivatePort(addr, false);
        }
        if (!gEnv->IsEditor())
        {
            //check whether this Start node is in a deactivated entity (and should not trigger)
            //this is necessary for deactivated layers, because the flowgraphs are reset and should not run
            bool bSkipActivation = false;
            FlowEntityId graphEntity = pActInfo->pGraph->GetGraphEntity(0);
            IEntity* pEntity = gEnv->pEntitySystem->GetEntity(graphEntity);
            if (pEntity && gEnv->pEntitySystem->ShouldSerializedEntity(pEntity) == false)
            {
                bSkipActivation = true;
            }

            if (bSkipActivation)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            else
            {
                // we're in pure game mode
                SetActivation(pActInfo, false);
            }
        }
        else
        {
            // we're in editor mode and we're currently editing,
            // (-> not in editor/game mode), so we schedule ourself to be updated
            if (CCryAction::GetCryAction()->IsEditing() == true)
            {
                SetActivation(pActInfo, false);
            }
        }
        break;
    }
#endif
}

bool CFlowStartNode::SerializeXML(SActivationInfo* pActInfo, const XmlNodeRef& root, bool reading)
{
    if (reading)
    {
        bool activated;
        if (!root->getAttr("ACTIVATED", activated))
        {
            return false;
        }
        SetActivation(pActInfo, activated);
    }
    else
    {
        root->setAttr("ACTIVATED", m_bActivated);
    }
    return true;
}

void CFlowStartNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
    if (ser.IsWriting())
    {
        ser.Value("activated", m_bActivated);
    }
    else
    {
        bool activated;
        ser.Value("activated", activated);
        SetActivation(pActInfo, activated);
    }
}

void CFlowStartNode::SetActivation(SActivationInfo* pActInfo, bool value)
{
    if (value == m_bActivated)
    {
        return;
    }

    m_bActivated = value;
    if (pActInfo->pGraph)
    {
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, !m_bActivated);
    }
}
