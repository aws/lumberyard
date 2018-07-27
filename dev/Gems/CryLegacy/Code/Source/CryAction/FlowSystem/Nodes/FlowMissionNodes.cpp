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
#include <IConsole.h>

#include <ITimeDemoRecorder.h>
#include <LoadScreenBus.h>

//////////////////////////////////////////////////////////////////////////
class CFlowNode_EndLevel
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        NextLevel,
        ClearToBlack,
#if AZ_LOADSCREENCOMPONENT_ENABLED
        LoadScreenUiCanvasPath,
        LoadScreenSequenceToAutoPlay,
        LoadScreenSequenceFixedFps,
        LoadScreenMaxFps,
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
    };

public:
    CFlowNode_EndLevel(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig<string>("NextLevel", _HELP("The next level to advance to")),
            InputPortConfig<bool>("ClearToBlack", true, _HELP("Should we clear to black or maintain the last rendered frame while loading")),
#if AZ_LOADSCREENCOMPONENT_ENABLED
            InputPortConfig<string>("LoadScreenUiCanvasPath", _HELP("The optional path to the UiCanvas file to use for the load screen")),
            InputPortConfig<string>("LoadScreenSequenceToAutoPlay", _HELP("The optional load screen sequence name to play automatically")),
            InputPortConfig<float>("LoadScreenSequenceFixedFps", 60.0f, _HELP("The optional fixed frame rate fed to updates of the load screen sequence\nTo let the system use the real time delta, leave it unset or set it to -1")),
            InputPortConfig<float>("LoadScreenMaxFps", 30.0f, _HELP("The optional max frame rate to update the load screen\nTo avoid capping the maximum update frequency, set it to -1. The default is 30.")),
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
            {0}
        };
        config.sDescription = _HELP("Ends the current level and advances to a new level");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts  = in_ports;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        CCryAction* pCryAction = CCryAction::GetCryAction();
        bool isTimeDemoActive = false;
        TimeDemoRecorderBus::BroadcastResult(isTimeDemoActive, &TimeDemoRecorderBus::Events::IsTimeDemoActive);

        if (!isTimeDemoActive && event == eFE_Activate && IsPortActive(pActInfo, InputPorts::Activate))
        {
#if AZ_LOADSCREENCOMPONENT_ENABLED
            gEnv->pConsole->GetCVar("level_load_screen_uicanvas_path")->Set(GetPortString(pActInfo, InputPorts::LoadScreenUiCanvasPath));
            gEnv->pConsole->GetCVar("level_load_screen_sequence_to_auto_play")->Set(GetPortString(pActInfo, InputPorts::LoadScreenSequenceToAutoPlay));
            gEnv->pConsole->GetCVar("level_load_screen_sequence_fixed_fps")->Set(GetPortFloat(pActInfo, InputPorts::LoadScreenSequenceFixedFps));
            gEnv->pConsole->GetCVar("level_load_screen_max_fps")->Set(GetPortFloat(pActInfo, InputPorts::LoadScreenMaxFps));
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
            pCryAction->ScheduleEndLevel(GetPortString(pActInfo, InputPorts::NextLevel), GetPortBool(pActInfo, InputPorts::ClearToBlack));
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Mission Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Mission:LoadNextLevel", CFlowNode_EndLevel);
