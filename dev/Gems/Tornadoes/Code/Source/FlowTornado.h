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
#ifndef CRYINCLUDE_GAMEDLL_ENVIRONMENT_FLOWTORNADO_H
#define CRYINCLUDE_GAMEDLL_ENVIRONMENT_FLOWTORNADO_H
#pragma once

#include "Tornado.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"

class CFlowTornadoWander
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowTornadoWander(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowTornadoWander(pActInfo);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Activate", _HELP("Tell the tornado to actually wander to the new [Target]")),
            InputPortConfig<FlowEntityId>("Target", _HELP("Set a new wandering target for the tornado")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void("Done", _HELP("Triggered when target has been reached")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Tells a tornado entity to wander in the direction of the [Target] entity");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            m_actInfo = *pActInfo;
            break;
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0) && pActInfo->pEntity)
            {
                CTornado* pTornado = (CTornado*)GetISystem()->GetIGame()->GetIGameFramework()->QueryGameObjectExtension(pActInfo->pEntity->GetId(), "Tornado");

                if (pTornado)
                {
                    IEntity* pTarget = gEnv->pEntitySystem->GetEntity(GetPortEntityId(pActInfo, 1));
                    if (pTarget)
                    {
                        pTornado->SetTarget(pTarget, this);
                    }
                }
            }
            break;
        }
    }

    void Done()
    {
        if (m_actInfo.pGraph)
        {
            ActivateOutput(&m_actInfo, FlowEntityId(), true);
        }
    }
private:
    SActivationInfo m_actInfo;
};

#endif // CRYINCLUDE_GAMEDLL_ENVIRONMENT_FLOWTORNADO_H
