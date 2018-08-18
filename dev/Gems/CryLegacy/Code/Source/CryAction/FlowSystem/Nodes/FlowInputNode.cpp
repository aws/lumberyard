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
#include <CryAction.h>
#include <CryActionCVars.h>
#include <IGameObjectSystem.h>
#include <StringUtils.h>
#include <IInput.h>
#include "ForceFeedbackSystem/ForceFeedbackSystem.h"
#include "ActionMap.h"

static const char* kInputDeviceTypeUIString = "enum_int:Keyboard=0,Mouse=1,Joystick=2,Gamepad=3,TouchScreen=4,MotionSensor=5,MotionController=6";

class CFlowNode_InputKey
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
    enum InputPorts
    {
        Enable = 0,
        Disable,
        Key,
        NoDevMode,
        KeyboardOnly
    };

    enum OutputPorts
    {
        Pressed,
        Released,
    };

public:
    CFlowNode_InputKey(SActivationInfo* pActInfo)
        : m_bActive(false)
        , m_bAllowedInNonDevMode(false)
        , m_bKeyboardOnly(false)
    {
        m_eventKeyName.resize(64); // avoid delete/re-newing during execution - assuming no action keys are longer than 64 char
    }

    ~CFlowNode_InputKey()
    {
        Register(false, 0);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Activate to enable")),
            InputPortConfig_Void("Disable", _HELP("Activate to disable")),
            InputPortConfig<string>("Key", _HELP("Key name. Leave empty for any")),
            InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
            InputPortConfig<bool>("KeyboardOnly", false, _HELP("If set to true, will ignore any non-keyboard related input")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<string>("Pressed", _HELP("Key was pressed")),
            OutputPortConfig<string>("Released", _HELP("Key was released")),
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Debugging only FlowNode to catch key inputs. It is disabled by default. Entity Inputs should be used in multiplayer.");
        config.SetCategory(EFLN_DEBUG);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_InputKey(pActInfo);
    }

    void Register(bool bRegister, SActivationInfo* pActInfo)
    {
        IInput* pInput = gEnv->pInput;
        if (pInput)
        {
            if (bRegister)
            {
                assert(pActInfo != 0);
                m_bAllowedInNonDevMode = GetPortBool(pActInfo, InputPorts::NoDevMode);
                m_bKeyboardOnly = GetPortBool(pActInfo, InputPorts::KeyboardOnly);

                if (gEnv->pSystem->IsDevMode() || m_bAllowedInNonDevMode)
                {
                    pInput->AddEventListener(this);
                }
            }
            else
            {
                pInput->RemoveEventListener(this);
            }
        }

        m_bActive = bRegister;
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("m_bActive", m_bActive);

        if (ser.IsReading())
        {
            Register(m_bActive, pActInfo);
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (InputEntityIsLocalPlayer(pActInfo))
        {
            switch (event)
            {
            case eFE_Initialize:
            {
                m_actInfo = *pActInfo;
                Register(true, pActInfo);
                break;
            }
            case eFE_Activate:
            {
                m_actInfo = *pActInfo;
                if (IsPortActive(pActInfo, InputPorts::Disable))
                {
                    Register(false, pActInfo);
                }
                if (IsPortActive(pActInfo, InputPorts::Enable))
                {
                    Register(true, pActInfo);
                }
                break;
            }
            }
        }
    }

    bool OnInputEvent(const SInputEvent& event) override
    {
        // Kevin - Ignore if console is up
        if (gEnv->pConsole->IsOpened())
        {
            return false;
        }

        if (m_bKeyboardOnly && event.deviceType != eIDT_Keyboard)
        {
            return false;
        }

        m_eventKeyName = event.keyName.c_str();

        const string& keyName = GetPortString(&m_actInfo, InputPorts::Key);

        if (keyName.empty() == false)
        {
            if (azstricmp(keyName.c_str(), m_eventKeyName.c_str()) != 0)
            {
                return false;
            }
        }

        if (gEnv->pSystem->IsDevMode()
            && !gEnv->IsEditor()
            && CCryActionCVars::Get().g_disableInputKeyFlowNodeInDevMode != 0
            && !m_bAllowedInNonDevMode)
        {
            return false;
        }

        const bool nullInputValue = (fabs_tpl(event.value) < 0.02f);

        if ((event.state == eIS_Pressed) || ((event.state == eIS_Changed) && !nullInputValue))
        {
            ActivateOutput(&m_actInfo, OutputPorts::Pressed, m_eventKeyName);
        }
        else if ((event.state == eIS_Released) || ((event.state == eIS_Changed) && !nullInputValue))
        {
            ActivateOutput(&m_actInfo, OutputPorts::Released, m_eventKeyName);
        }

        // return false, so other listeners get notification as well
        return false;
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:

    SActivationInfo m_actInfo;
    bool m_bActive;
    bool m_bAllowedInNonDevMode;
    bool m_bKeyboardOnly;
    string m_eventKeyName; // string is necessary for ActivateOutput, and it allocates in heap - so we use a member var for it
};

/////////////////////////////////////////////////////////////////////////////////
class CFlowNode_InputActionListener
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
    enum InputPorts
    {
        Enable = 0,
        Disable,
        Action,
        ActionMap,
        NonDevMode,
    };

    enum OutputPorts
    {
        Pressed = 0,
        Released,
    };

public:
    CFlowNode_InputActionListener(SActivationInfo* pActInfo)
        : m_bActive(false)
    {
        m_eventKeyName.resize(64); // avoid delete/re-newing during execution - assuming no action keys are longer than 64 char
    }

    ~CFlowNode_InputActionListener()
    {
        Register(false, 0);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Activate to Enable")),
            InputPortConfig_Void("Disable", _HELP("Activate to Disable")),
            InputPortConfig<string>("Action", _HELP("Action to trigger"), _HELP("Action"), _UICONFIG("enum_global:input_actions")),
            InputPortConfig<string>("ActionMap", _HELP("Action Map to use"), _HELP("Action Map"), _UICONFIG("enum_global:action_maps")),
            InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<string>("Pressed", _HELP("Action trigger pressed.")),
            OutputPortConfig<string>("Released", _HELP("Action trigger released.")),
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Catches input events via their action name. Disabled by default. Entity Inputs should be used in multiplayer.");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_InputActionListener(pActInfo);
    }

    void Register(bool bRegister, SActivationInfo* pActInfo)
    {
        IInput* pInput = gEnv->pInput;
        if (pInput)
        {
            if (bRegister)
            {
                assert(pActInfo != 0);
                pInput->AddEventListener(this);
            }
            else
            {
                pInput->RemoveEventListener(this);
            }
        }

        m_bActive = bRegister;
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("m_bActive", m_bActive);
        ser.Value("m_bAnyAction", m_bAnyAction);

        if (ser.IsReading())
        {
            Register(m_bActive, pActInfo);
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (InputEntityIsLocalPlayer(pActInfo))
        {
            switch (event)
            {
            case eFE_Initialize:
            {
                m_actInfo = *pActInfo;
                Register(false, pActInfo);
                break;
            }
            case eFE_Activate:
            {
                m_actInfo = *pActInfo;
                if (IsPortActive(pActInfo, InputPorts::Disable))
                {
                    Register(false, pActInfo);
                }
                if (IsPortActive(pActInfo, InputPorts::Enable))
                {
                    Register(true, pActInfo);
                }
                break;
            }
            }
        }
    }

    virtual bool OnInputEvent(const SInputEvent& event) override
    {
        // Kevin - Ignore if console is up
        if (gEnv->pConsole->IsOpened())
        {
            return false;
        }

        m_eventKeyName = event.keyName.c_str();

        // Removed caching of actions to avoid problems when rebinding keys if node is already active
        // Only a very slight performance hit without caching
        // TODO: Should be refactored to handle onAction instead of directly comparing inputs, caching won't be needed
        const string& actionName = GetPortString(&m_actInfo, InputPorts::Action);
        const string& actionMapName = GetPortString(&m_actInfo, InputPorts::ActionMap);
        bool bActionFound = actionName.empty();

        m_bAnyAction = bActionFound;
        if (!bActionFound)
        {
            IActionMapManager* pAMMgr = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
            if (pAMMgr)
            {
                IActionMap* pAM = pAMMgr->GetActionMap(actionMapName);
                if (pAM)
                {
                    const IActionMapAction* pAction = pAM->GetAction(ActionId(actionName));
                    if (pAction)
                    {
                        const int iNumInputData = pAction->GetNumActionInputs();
                        for (int i = 0; i < iNumInputData; ++i)
                        {
                            const SActionInput* pActionInput = pAction->GetActionInput(i);
                            CRY_ASSERT(pActionInput != nullptr);
                            bActionFound = azstricmp(pActionInput->input, m_eventKeyName.c_str()) == 0;

                            if (bActionFound)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (bActionFound)
        {
            bool nullInputValue = (fabs_tpl(event.value) < 0.02f);

            if ((event.state == eIS_Pressed) || ((event.state == eIS_Changed) && !nullInputValue))
            {
                ActivateOutput(&m_actInfo, OutputPorts::Pressed, m_eventKeyName);
            }
            else if ((event.state == eIS_Released) || ((event.state == eIS_Changed) && nullInputValue))
            {
                ActivateOutput(&m_actInfo, OutputPorts::Released, m_eventKeyName);
            }
        }

        // return false, so other listeners get notification as well
        return false;
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    SActivationInfo m_actInfo;
    bool m_bActive;
    bool m_bAnyAction;
    string m_eventKeyName; // string is necessary for ActivateOutput, and it allocates in heap - so we use a member var for it
};

class CFlowNode_InputActionHandler
    : public CFlowBaseNode <eNCT_Instanced>
    , public IActionListener
{
public:
    enum InputPorts
    {
        Enable = 0,
        Disable,
        ActionMapName,
        ActionName,
    };

    enum OutputPorts
    {
        ActionInvoked = 0,
        ActionPressed,
        ActionHeld,
        ActionReleased,
    };

    CFlowNode_InputActionHandler(SActivationInfo* pActInfo)
    {
    }

    ~CFlowNode_InputActionHandler()
    {
        if (m_actionMapManager)
        {
            m_actionMapManager->RemoveExtraActionListener(this, m_actionMapName);
        }
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_InputActionHandler(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Activate to enable listening to the action map specified")),
            InputPortConfig_Void("Disable", _HELP("Activate to disable listening to the action map specified")),
            InputPortConfig<string>("ActionMapName", _HELP("This is the name of the ActionMap that the Action you want to listen to lives on"),
                _HELP("Action Map"), _UICONFIG("enum_global:action_maps")),
            InputPortConfig<string>("ActionName",
                _HELP("This is the name of the action you would like to listen for.  When this happens the output port will activate")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("ActionInvoked", _HELP("Action Invoked")),
            OutputPortConfig_Void("ActionPressed", _HELP("Action pressed (First)")),
            OutputPortConfig_Void("ActionHeld", _HELP("Action held (Sustain)")),
            OutputPortConfig_Void("ActionReleased", _HELP("Action released (Last)")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("With this you can respond to Actions listed in ActionMaps");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    virtual void OnAction(const ActionId& action, int activationMode, float value) override
    {
        if (azstricmp(action.c_str(), m_actionName) == 0)
        {
            m_shouldActivate = true;
            m_currentInputState = static_cast<EInputState>(activationMode);
            m_value = value;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
            m_actionMapManager = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;


            if (m_actionMapManager)
            {
                m_actionMapManager->RemoveExtraActionListener(this, m_actionMapName);
            }

            m_actionMapName = GetPortString(pActInfo, InputPorts::ActionMapName);
            m_actionName = GetPortString(pActInfo, InputPorts::ActionName);

            if (m_actionMapManager)
            {
                m_actionMapManager->AddExtraActionListener(this, m_actionMapName);
            }
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            else if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }

            if (IsPortActive(pActInfo, InputPorts::ActionMapName))
            {
                if (m_actionMapManager)
                {
                    m_actionMapManager->RemoveExtraActionListener(this, m_actionMapName);
                }

                m_actionMapName = GetPortString(pActInfo, InputPorts::ActionMapName);

                if (m_actionMapManager)
                {
                    m_actionMapManager->AddExtraActionListener(this, m_actionMapName);
                }
            }
            if (IsPortActive(pActInfo, InputPorts::ActionName))
            {
                m_actionName = GetPortString(pActInfo, InputPorts::ActionName);
            }
            break;
        }
        case eFE_Update:
        {
            if (m_shouldActivate)
            {
                ActivateOutput(pActInfo, OutputPorts::ActionInvoked, m_value);
                if (m_currentInputState == eIS_Down)
                {
                    ActivateOutput(pActInfo, OutputPorts::ActionHeld, m_value);
                }
                else if (m_currentInputState == eIS_Pressed)
                {
                    ActivateOutput(pActInfo, OutputPorts::ActionPressed, m_value);
                    m_currentInputState = eIS_Down;
                }
                else if (m_currentInputState == eIS_Released)
                {
                    m_shouldActivate = false;
                    ActivateOutput(pActInfo, OutputPorts::ActionReleased, m_value);
                }
            }
            break;
        }
        case eFE_Uninitialize:
        {
            if (m_actionMapManager)
            {
                m_actionMapManager->RemoveExtraActionListener(this, m_actionMapName);
            }
        }
        }
    }
private:
    string m_actionMapName = "";
    string m_actionName = "";
    IActionMapManager* m_actionMapManager = nullptr;
    float m_value = 0.0f;
    bool m_shouldActivate = false;
    EInputState m_currentInputState = eIS_Unknown;
};

//////////////////////////////////////////////////////////////////////
class CFlowNode_InputActionFilter
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        Enable = 0,
        Disable,
        Filter
    };

    enum OutputPorts
    {
        Enabled,
        Disabled,
    };

    void EnableFilter(bool bEnable, bool bActivateOutput = true)
    {
        if (m_filterName.empty())
        {
            return;
        }

        m_bEnabled = bEnable;

        IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
        IActionMapManager* pAMMgr = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;

        if (pAMMgr)
        {
            pAMMgr->EnableFilter(m_filterName, bEnable);

            if (bActivateOutput && m_pActInfo && m_pActInfo->pGraph)
            {
                ActivateOutput(m_pActInfo, bEnable ? OutputPorts::Enabled : OutputPorts::Disabled, 1);
            }
        }
    }

public:
    CFlowNode_InputActionFilter(SActivationInfo* pActInfo)
        : m_bEnabled(false)
        , m_pActInfo(pActInfo)
    {
    }

    ~CFlowNode_InputActionFilter()
    {
        EnableFilter(false, false);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Activate to enable ActionFilter")),
            InputPortConfig_Void("Disable", _HELP("Activate to disable ActionFilter")),
            InputPortConfig<string>("Filter", _HELP("Action Filter"), 0, _UICONFIG("enum_global:action_filter")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Enabled", _HELP("Activate when enabled.")),
            OutputPortConfig_Void("Disabled", _HELP("Activate when disabled.")),
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Enables & Disables ActionFilters \r If a player is not proved as input entity, the filter will affect all players");
        config.SetCategory(EFLN_ADVANCED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_InputActionFilter(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        m_pActInfo = pActInfo;
        // disable on reading
        if (ser.IsReading())
        {
            EnableFilter(false);
        }

        ser.Value("m_filterName", m_filterName);
        // on saving we ask the ActionMapManager if the filter is enabled
        // so, all basically all nodes referring to the same filter will write the correct (current) value
        // on quickload, all of them or none of them will enable/disable the filter again
        // maybe use a more central location for this
        if (ser.IsWriting())
        {
            bool bWroteIt = false;
            IActionMapManager* pAMMgr = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
            if (pAMMgr && m_filterName.empty() == false)
            {
                IActionFilter* pFilter = pAMMgr->GetActionFilter(m_filterName.c_str());

                if (pFilter)
                {
                    bool bIsEnabled = pAMMgr->IsFilterEnabled(m_filterName.c_str());
                    ser.Value("m_bEnabled", bIsEnabled);
                    bWroteIt = true;
                }
            }
            if (!bWroteIt)
            {
                ser.Value("m_bEnabled", m_bEnabled);
            }
        }
        else
        {
            ser.Value("m_bEnabled", m_bEnabled);
        }

        // re-enable
        if (ser.IsReading())
        {
            EnableFilter(m_bEnabled);
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        m_pActInfo = pActInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            EnableFilter(false);
            m_filterName = GetPortString(pActInfo, InputPorts::Filter);
            break;
        }
        case eFE_Activate:
        {
            if (InputEntityIsLocalPlayer(pActInfo))
            {
                if (IsPortActive(pActInfo, InputPorts::Filter))
                {
                    m_filterName = GetPortString(pActInfo, InputPorts::Filter);
                }
                else
                {
                    EnableFilter(IsPortActive(pActInfo, InputPorts::Enable));
                }
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

protected:

    bool m_bEnabled;
    string m_filterName; // we need to store this name, as it the d'tor we need to disable the filter, but don't have access to port names
    SActivationInfo* m_pActInfo;
};

//////////////////////////////////////////////////////////////////////
class CFlowNode_ActionMapManager
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum INPUTS
    {
        EIP_ENABLE = 0,
        EIP_DISABLE,
        EIP_ACTIONMAP,
    };
public:
    CFlowNode_ActionMapManager(SActivationInfo* pActInfo)
        : m_pActInfo(pActInfo)
    {
    }

    ~CFlowNode_ActionMapManager()
    {
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Trigger to enable (default: disabled).")),
            InputPortConfig_Void("Disable", _HELP("Trigger to disable")),
            InputPortConfig<string>("ActionMap", _HELP("Action Map to use"), _HELP("Action Map"), _UICONFIG("enum_global:action_maps")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.sDescription = _HELP("FlowNode to enable/disable actionmaps");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_ActionMapManager(pActInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        m_pActInfo = pActInfo;
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, EIP_ENABLE) || IsPortActive(pActInfo, EIP_DISABLE))
            {
                const bool bEnable = IsPortActive(pActInfo, EIP_ENABLE);
                const string actionMapName = GetPortString(pActInfo, EIP_ACTIONMAP);

                IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : NULL;
                IActionMapManager* pAMMgr = pGameFramework ? pGameFramework->GetIActionMapManager() : NULL;
                if (pAMMgr && !actionMapName.empty())
                {
                    pAMMgr->EnableActionMap(actionMapName, bEnable);
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:
    SActivationInfo* m_pActInfo;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedbackSetDeviceIndex
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum INPUTS
    {
        eIP_Index = 0,
    };

public:
    CFlowNode_ForceFeedbackSetDeviceIndex(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<int>("DeviceIndex", 0,  _HELP("The 0 based index of the controller you want feedback to go to")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("FlowNode to set the receiving device id for force feedback effects");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eIP_Index))
            {
                if (gEnv->pInput)
                {
                    gEnv->pInput->ForceFeedbackSetDeviceIndex(GetPortInt(pActInfo, eIP_Index));
                }
            }
            break;
        }
        default:
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedback
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum INPUTS
    {
        eIP_EffectName = 0,
        eIP_Play,
        eIP_Intensity,
        eIP_Delay,
        eIP_Stop,
        eIP_StopAll,
        eIP_InputDeviceType
    };

public:
    CFlowNode_ForceFeedback(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        struct FeedbackEffectNames : public IFFSPopulateCallBack
        {
            void AddFFSEffectName(const char* const pName) override { m_effectNames = AZStd::string::format("%s,%s", m_effectNames.c_str(), pName); }
            AZStd::string m_effectNames;
        };
        static FeedbackEffectNames s_feedbackEffectNames;
        if (s_feedbackEffectNames.m_effectNames.empty())
        {
            s_feedbackEffectNames.m_effectNames = "enum_string:";
            IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();
            pForceFeedback->EnumerateEffects(&s_feedbackEffectNames);
            size_t commaOffset = s_feedbackEffectNames.m_effectNames.find_first_of(',');
            if (commaOffset != AZStd::string::npos)
            {
                s_feedbackEffectNames.m_effectNames.erase(commaOffset, 1);
            }
        }

        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("EffectName", _HELP("Name of the force feedback effect to be played/stopped"), _HELP("Effect Name"), _UICONFIG(s_feedbackEffectNames.m_effectNames.c_str())),
            InputPortConfig_Void("Play", _HELP("Play the specified effect")),
            InputPortConfig<float>("Intensity", 1.f, _HELP("Intensity factor applied to the effect being played")),
            InputPortConfig<float>("Delay", 0.f, _HELP("Time delay to start the effect")),
            InputPortConfig_Void("Stop", _HELP("Stop the specified effect")),
            InputPortConfig_Void("StopAll", _HELP("Stop all currently playing force feedback effects")),
            InputPortConfig<int>("InputDeviceType", static_cast<int>(eIDT_Gamepad), _HELP("The Device type to play the force feedback"), _HELP("Input Device Type"), _UICONFIG(kInputDeviceTypeUIString)),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("FlowNode to play/stop force feedback effects");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (InputEntityIsLocalPlayer(pActInfo))
            {
                IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();

                if (IsPortActive(pActInfo, eIP_Play))
                {
                    const char* effectName = GetPortString(pActInfo, eIP_EffectName).c_str();
                    ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(effectName);
                    const float intensity = GetPortFloat(pActInfo, eIP_Intensity);
                    const float delay = GetPortFloat(pActInfo, eIP_Delay);
                    pForceFeedback->PlayForceFeedbackEffect(fxId, SForceFeedbackRuntimeParams(intensity, delay), static_cast<EInputDeviceType>(GetPortInt(pActInfo, eIP_InputDeviceType)));
                }

                if (IsPortActive(pActInfo, eIP_Stop))
                {
                    const char* effectName = GetPortString(pActInfo, eIP_EffectName).c_str();
                    ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(effectName);
                    pForceFeedback->StopForceFeedbackEffect(fxId);
                }

                if (IsPortActive(pActInfo, eIP_StopAll))
                {
                    pForceFeedback->StopAllEffects();
                }
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedbackTweaker
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum INPUTS
    {
        eIP_LowPassMultiplier,
        eIP_HighPassMultiplier,
        eIP_Start,
        eIP_Stop,
        eIP_InputDeviceType,
    };

public:
    CFlowNode_ForceFeedbackTweaker(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<float>("LowPass", 1.f, _HELP("Multiplier applied to the low frequency effect being played")),
            InputPortConfig<float>("HighPass", 1.f, _HELP("Multiplier applied to the high frequency effect being played")),
            InputPortConfig_AnyType("Activate", _HELP("activate the forcefeedback tweaker")),
            InputPortConfig_AnyType("Deactivate", _HELP("deactivate the force feedback tweaker")),
            InputPortConfig<int>("InputDeviceType", static_cast<int>(eIDT_Gamepad), _HELP("The Device type to play the force feedback"), _HELP("Input Device Type"), _UICONFIG(kInputDeviceTypeUIString)),
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.sDescription = _HELP("FlowNode to control individual high and low frequency force feedback effect");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eIP_Start))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            if (IsPortActive(pActInfo, eIP_Stop))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            break;
        }

        case eFE_Update:
        {
            IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();
            if (pForceFeedback)
            {
                pForceFeedback->AddFrameCustomForceFeedback(GetPortFloat(pActInfo, eIP_HighPassMultiplier), GetPortFloat(pActInfo, eIP_LowPassMultiplier), static_cast<EInputDeviceType>(GetPortInt(pActInfo, eIP_InputDeviceType)));
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedbackTriggerTweaker
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum INPUTS
    {
        eIP_LeftTouchToActivate,
        eIP_LeftGain,
        eIP_LeftEnvelope,

        eIP_RightTouchToActivate,
        eIP_RightGain,
        eIP_RightEnvelope,

        eIP_Activate,
        eIP_Deactivate,

        eIP_InputDeviceType,
    };

public:
    CFlowNode_ForceFeedbackTriggerTweaker(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // declare input ports
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<bool>("LeftTouchToActivate", true, _HELP("If true the left trigger's gain will be modulated by how much the trigger is pressed")),
            InputPortConfig<float>("LeftGain", 1.f, _HELP("Gain sent to the left trigger's motor")),
            InputPortConfig<int>("LeftEnvelope", 1.f, _HELP("Envelope sent to the left trigger's motor (multiple of 4, from 0 to 2000)")),
            InputPortConfig<bool>("RightTouchToActivate", true, _HELP("If true the right trigger's gain will be modulated by how much the trigger is pressed")),
            InputPortConfig<float>("RightGain", 1.f, _HELP("Gain sent to the right trigger's motor")),
            InputPortConfig<int>("RightEnvelope", 1.f, _HELP("Envelope sent to the right trigger's motor (multiple of 4, from 0 to 2000)")),
            InputPortConfig_AnyType("Activate", _HELP("activate the forcefeedback tweaker")),
            InputPortConfig_AnyType("Deactivate", _HELP("deactivate the force feedback tweaker")),
            InputPortConfig<int>("InputDeviceType", static_cast<int>(eIDT_Gamepad), _HELP("The Device type to play the force feedback"), _HELP("Input Device Type"), _UICONFIG(kInputDeviceTypeUIString)),
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.sDescription = _HELP("FlowNode to control force feedback effect on left and right triggers");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eIP_Activate))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            if (IsPortActive(pActInfo, eIP_Deactivate))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            break;
        }

        case eFE_Update:
        {
            IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();
            if (pForceFeedback)
            {
                pForceFeedback->AddCustomTriggerForceFeedback(SFFTriggerOutputData(
                        GetPortBool(pActInfo, eIP_LeftTouchToActivate),
                        GetPortBool(pActInfo, eIP_RightTouchToActivate),
                        GetPortFloat(pActInfo, eIP_LeftGain),
                        GetPortFloat(pActInfo, eIP_RightGain),
                        GetPortInt(pActInfo, eIP_LeftEnvelope),
                        GetPortInt(pActInfo, eIP_RightEnvelope)),
                        static_cast<EInputDeviceType>(GetPortInt(pActInfo, eIP_InputDeviceType)));
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Input:ActionMapManager", CFlowNode_ActionMapManager);
REGISTER_FLOW_NODE("Input:ActionListener", CFlowNode_InputActionListener);
REGISTER_FLOW_NODE("Input:ActionFilter", CFlowNode_InputActionFilter);
REGISTER_FLOW_NODE("Debug:InputKey", CFlowNode_InputKey);
REGISTER_FLOW_NODE("Game:ForceFeedbackSetDeviceIndex", CFlowNode_ForceFeedbackSetDeviceIndex);
REGISTER_FLOW_NODE("Game:ForceFeedback", CFlowNode_ForceFeedback);
REGISTER_FLOW_NODE("Game:ForceFeedbackTweaker", CFlowNode_ForceFeedbackTweaker);
REGISTER_FLOW_NODE("Game:ForceFeedbackTriggerTweaker", CFlowNode_ForceFeedbackTriggerTweaker);
REGISTER_FLOW_NODE("Input:ActionHandler", CFlowNode_InputActionHandler);
