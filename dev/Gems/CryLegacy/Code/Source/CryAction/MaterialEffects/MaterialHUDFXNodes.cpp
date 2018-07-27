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
#include "MaterialEffects.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Components/IComponentRender.h"

//CHUDStartFXNode
//Special node that let us trigger a FlowGraph HUD effect related to
//some kind of material, impact, explosion...
class CHUDStartFXNode
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CHUDStartFXNode(SActivationInfo* pActInfo)
    {
    }

    ~CHUDStartFXNode()
    {
    }

    enum
    {
        EIP_Trigger = 0,
    };

    enum
    {
        EOP_Started = 0,
        EOP_Distance,
        EOP_Param1,
        EOP_Param2,
        EOP_Intensity,
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void  ("Start", _HELP("Triggered automatically by MaterialEffects")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void   ("Started", _HELP("Triggered when Effect is started.")),
            OutputPortConfig<float> ("Distance", _HELP("Distance to player")),
            OutputPortConfig<float> ("Param1", _HELP("MFX Custom Param 1")),
            OutputPortConfig<float> ("Param2", _HELP("MFX Custom Param 2")),
            OutputPortConfig<float> ("Intensity", _HELP("Dynamic value set from game code (0.0 - 1.0)")),
            OutputPortConfig<float> ("BlendOutTime", _HELP("Outputs time value in seconds; request to blend out FX in that time")),
            {0}
        };
        config.sDescription = _HELP("MaterialFX StartNode");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    //Just activate the output when the input is activated
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:
            if (IsPortActive(pActInfo, EIP_Trigger))
            {
                ActivateOutput(pActInfo, EOP_Started, true);
            }
            break;
        }
    }
};

//CHUDEndFXNode
//Special node that let us know when the FlowGraph HUD Effect has finish
class CHUDEndFXNode
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CHUDEndFXNode(SActivationInfo* pActInfo)
    {
    }

    ~CHUDEndFXNode()
    {
    }

    enum
    {
        EIP_Trigger = 0,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void     ("Trigger", _HELP("Trigger to mark end of effect.")),
            {0}
        };
        config.sDescription = _HELP("MaterialFX EndNode");
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }

    //When activated, just notify the MaterialFGManager that the effect ended
    //See MaterialFGManager.cpp
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:
            if (IsPortActive(pActInfo, EIP_Trigger))
            {
                CMaterialEffects* pMatFX = static_cast<CMaterialEffects*> (gEnv->pGame->GetIGameFramework()->GetIMaterialEffects());
                if (pMatFX)
                {
                    //const string& name = GetPortString(pActInfo, EIP_Name);
                    pMatFX->NotifyFGHudEffectEnd(pActInfo->pGraph);
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("MaterialFX:HUDStartFX", CHUDStartFXNode);
REGISTER_FLOW_NODE("MaterialFX:HUDEndFX", CHUDEndFXNode);
