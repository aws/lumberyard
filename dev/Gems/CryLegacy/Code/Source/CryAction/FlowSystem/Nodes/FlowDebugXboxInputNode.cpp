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
#include "CryLegacy_precompiled.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>

// 360 keys enum // ACCEPTED_USE
enum EXBoxKey
{
    // Invalid - Not an XBox key // ACCEPTED_USE
    eXBK_Invalid = -1,

    // Keep in same order as EKeyID (KI_XINPUT_BASE+value)
    eXBK_DPadUp = 0,
    eXBK_DPadDown,
    eXBK_DPadLeft,
    eXBK_DPadRight,
    eXBK_Start,
    eXBK_Back,
    eXBK_ThumbL,
    eXBK_ThumbR,
    eXBK_ShoulderL,
    eXBK_ShoulderR,
    eXBK_A,
    eXBK_B,
    eXBK_X,
    eXBK_Y,
    eXBK_TriggerL,
    eXBK_TriggerR,
    eXBK_ThumbLX,
    eXBK_ThumbLY,
    eXBK_ThumbLUp,
    eXBK_ThumbLDown,
    eXBK_ThumbLLeft,
    eXBK_ThumbLRight,
    eXBK_ThumbRX,
    eXBK_ThumbRY,
    eXBK_ThumbRUp,
    eXBK_ThumbRDown,
    eXBK_ThumbRLeft,
    eXBK_ThumbRRight,
    eXBK_TriggerLBtn,
    eXBK_TriggerRBtn,
};


#define XBoxKeyEnum "enum_int:DPadUp=0,DPadDown=1,DPadLeft=2,DPadRight=3,Start=4,Back=5,ThumbL=6,ThumbR=7,ShoulderL=8,ShoulderR=9,A=10,B=11,X=12,Y=13,TriggerL=28,TriggerR=29" // ACCEPTED_USE
#define XBoxAnalogEnum "enum_int:ThumbL=6,ThumbR=7,TriggerL=14,TriggerR=15" // ACCEPTED_USE

//////////////////////////////////////////////////////////////////////////
class CFlowNode_DebugXBoxKey // ACCEPTED_USE
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
public:
    CFlowNode_DebugXBoxKey(SActivationInfo* pActInfo) // ACCEPTED_USE
    {
    }

    ~CFlowNode_DebugXBoxKey() // ACCEPTED_USE
    {
        Register(false);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_DebugXBoxKey(pActInfo); // ACCEPTED_USE
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_bActive", m_bActive);
        if (ser.IsReading())
        {
            m_actInfo = *pActInfo;
            Register(m_bActive);
        }
    }

    enum EInputPorts
    {
        EIP_Enable = 0,
        EIP_Disable,
        EIP_Key,
        EIP_NonDevMode,
    };

    enum EOutputPorts
    {
        EOP_Pressed = 0,
        EOP_Released,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Enable", _HELP("Enable reporting")),
            InputPortConfig_Void("Disable", _HELP("Disable reporting")),
            InputPortConfig<int>("Key", 0, _HELP("XBoxOne controller key"), NULL, _UICONFIG(XBoxKeyEnum)), // ACCEPTED_USE
            InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void("Pressed", _HELP("Called when key is pressed")),
            OutputPortConfig_Void("Released", _HELP("Called when key is released")),
            { 0 }
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get input from XBox 360 controller. EntityInput is used in multiplayer"); // ACCEPTED_USE
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (InputEntityIsLocalPlayer(pActInfo))
        {
            switch (event)
            {
            case eFE_Initialize:
            {
                m_actInfo = *pActInfo;
                Register(true);
            }
            break;

            case eFE_Activate:
            {
                m_actInfo = *pActInfo;
                if (IsPortActive(pActInfo, EIP_Enable))
                {
                    Register(true);
                }
                if (IsPortActive(pActInfo, EIP_Disable))
                {
                    Register(false);
                }
            }
            break;
            }
        }
    }

    void Register(bool bRegister)
    {
        if (IInput* pInput = gEnv->pInput)
        {
            if (true == bRegister)
            {
                const bool bAllowedInNonDevMode = GetPortBool(&m_actInfo, EIP_NonDevMode);
                if (gEnv->pSystem->IsDevMode() || bAllowedInNonDevMode)
                {
                    pInput->AddEventListener(this);
                }
            }
            else
            {
                pInput->RemoveEventListener(this);
            }

            m_bActive = bRegister;
        }
        else
        {
            m_bActive = false;
        }
    }

    int TranslateKey(int nKeyId)
    {
        if (nKeyId >= eKI_XI_DPadUp && nKeyId <= eKI_XI_Disconnect)
        {
            return nKeyId - KI_XINPUT_BASE;
        }
        return eXBK_Invalid;
    }

    // ~IInputEventListener
    virtual bool OnInputEvent(const SInputEvent& event)
    {
        if (true == m_bActive)
        {
            // Translate key, check value
            const int nThisKey = TranslateKey(event.keyId);
            const int nKey = GetPortInt(&m_actInfo, EIP_Key);
            if (nKey == nThisKey)
            {
                // Return based on state
                if (eIS_Pressed == event.state)
                {
                    ActivateOutput(&m_actInfo, EOP_Pressed, true);
                }
                else if (eIS_Released == event.state)
                {
                    ActivateOutput(&m_actInfo, EOP_Released, true);
                }
            }
        }

        // Let other listeners handle it
        return false;
    }

private:
    bool m_bActive; // TRUE when node is enabled
    SActivationInfo m_actInfo; // Activation info instance
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_DebugXBoxAnalog // ACCEPTED_USE
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
public:
    CFlowNode_DebugXBoxAnalog(SActivationInfo* pActInfo) // ACCEPTED_USE
    {
    }

    ~CFlowNode_DebugXBoxAnalog() // ACCEPTED_USE
    {
        Register(false);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_DebugXBoxAnalog(pActInfo); // ACCEPTED_USE
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_bActive", m_bActive);
        if (ser.IsReading())
        {
            m_actInfo = *pActInfo;
            Register(m_bActive);
        }
    }

    enum EInputPorts
    {
        EIP_Enable = 0,
        EIP_Disable,
        EIP_Key,
        EIP_NonDevMode,
    };

    enum EOutputPorts
    {
        EOP_ChangedX = 0,
        EOP_ChangedY,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Enable", _HELP("Enable reporting")),
            InputPortConfig_Void("Disable", _HELP("Disable reporting")),
            InputPortConfig<int>("Key", 0, _HELP("XBoxOne controller key"), NULL, _UICONFIG(XBoxAnalogEnum)), // ACCEPTED_USE
            InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
            { 0 }
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<float>("ChangedX", _HELP("Called when analog changes in X (trigger info sent out here as well)")),
            OutputPortConfig<float>("ChangedY", _HELP("Called when analog changes in Y")),
            { 0 }
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get analog input from XBox 360 controller. Note: Expensive!  Note2: entity input is used in multiplayer"); // ACCEPTED_USE
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (InputEntityIsLocalPlayer(pActInfo))
        {
            switch (event)
            {
            case eFE_Initialize:
            {
                m_actInfo = *pActInfo;
                Register(true);
            }
            break;

            case eFE_Activate:
            {
                m_actInfo = *pActInfo;
                if (IsPortActive(pActInfo, EIP_Enable))
                {
                    Register(true);
                }
                if (IsPortActive(pActInfo, EIP_Disable))
                {
                    Register(false);
                }
            }
            break;
            }
        }
    }

    void Register(bool bRegister)
    {
        if (IInput* pInput = gEnv->pInput)
        {
            if (true == bRegister)
            {
                const bool bAllowedInNonDevMode = GetPortBool(&m_actInfo, EIP_NonDevMode);
                if (gEnv->pSystem->IsDevMode() || bAllowedInNonDevMode)
                {
                    pInput->AddEventListener(this);
                }
            }
            else
            {
                pInput->RemoveEventListener(this);
            }

            m_bActive = bRegister;
        }
        else
        {
            m_bActive = false;
        }
    }

    EXBoxKey TranslateKeyIntoFGInputEnum(int nKeyId, bool& isXAxis)
    {
        isXAxis = true;
        switch (nKeyId)
        {
        case eKI_XI_ThumbLY:
            isXAxis = false;
        case eKI_XI_ThumbLX:
            return eXBK_ThumbL;
            break;

        case eKI_XI_ThumbRY:
            isXAxis = false;
        case eKI_XI_ThumbRX:
            return eXBK_ThumbR;
            break;

        case eKI_XI_TriggerL:
            return eXBK_TriggerL;
            break;

        case eKI_XI_TriggerR:
            return eXBK_TriggerR;
            break;
        }
        return eXBK_Invalid;
    }

    // ~IInputEventListener
    virtual bool OnInputEvent(const SInputEvent& event)
    {
        if (true == m_bActive)
        {
            bool isXAxis = false;
            const EXBoxKey nPressedKey = TranslateKeyIntoFGInputEnum(event.keyId, isXAxis);
            const EXBoxKey nExpectedKey = EXBoxKey(GetPortInt(&m_actInfo, EIP_Key));
            if (nExpectedKey == nPressedKey)
            {
                if (isXAxis)
                {
                    ActivateOutput(&m_actInfo, EOP_ChangedX, event.value);
                }
                else
                {
                    ActivateOutput(&m_actInfo, EOP_ChangedY, event.value);
                }
            }
        }

        // Let other listeners handle it
        return false;
    }

private:
    bool m_bActive; // TRUE when node is enabled
    SActivationInfo m_actInfo; // Activation info instance
};


REGISTER_FLOW_NODE("Debug:XBoxKey", CFlowNode_DebugXBoxKey); // ACCEPTED_USE
REGISTER_FLOW_NODE("Debug:XBoxAnalog", CFlowNode_DebugXBoxAnalog); // ACCEPTED_USE
