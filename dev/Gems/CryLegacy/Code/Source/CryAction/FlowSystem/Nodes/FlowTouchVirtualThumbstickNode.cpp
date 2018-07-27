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
#include "IInput.h"

namespace
{
    const float g_DefaultVirtualThumbstickRadius = 0.1f;
}

namespace LYGame
{
    /*!
     * A FlowGraph node that provides a virtual thumbstick
     */
    class CFlowNode_TouchVirtualThumbstick
        : public CFlowBaseNode <eNCT_Instanced>
        , IInputEventListener
    {
    public:

        CFlowNode_TouchVirtualThumbstick(SActivationInfo* pActInfo)
            : m_touchDownPosition(ZERO)
            , m_touchDelta(ZERO)
            , m_touchFingerId(eKI_Unknown)
            , m_radius(g_DefaultVirtualThumbstickRadius)
            , m_sideOfScreen(SideOfScreen::Side_Any)
        {
        }

        virtual ~CFlowNode_TouchVirtualThumbstick()
        {
            if (gEnv->pInput)
            {
                gEnv->pInput->RemoveEventListener(this);
            }
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_TouchVirtualThumbstick(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable this node")),
                InputPortConfig_Void("Disable", _HELP("Disable this node")),
                InputPortConfig<float>("Radius", g_DefaultVirtualThumbstickRadius, "Radius of thumbstick pad as a percentage of screen width."),
                InputPortConfig<int>("ScreenInputArea", 1, _HELP("What side of the screen should this thumnbstick accept input on?"), _HELP("ScreenInputArea"), _UICONFIG("enum_int:Any=0,Left=1,Right=2")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<float>("OutX", _HELP("X value in range -1.0 and 1.0")),
                OutputPortConfig<float>("OutY", _HELP("Y value in range -1.0 and 1.0")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Implements a virtual thumbstick");
            config.SetCategory(EFLN_ADVANCED);
        }

        void GetMemoryUsage(ICrySizer* s) const override
        {
            s->Add(*this);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            m_activationInfo = *activationInfo;

            switch (event)
            {
            case eFE_Activate:
            {
                if (gEnv->pInput)
                {
                    if (IsPortActive(activationInfo, InputPorts::Input_Enable))
                    {
                        gEnv->pInput->AddEventListener(this);
                        m_radius = GetPortFloat(activationInfo, InputPorts::Input_Radius);
                        m_sideOfScreen = GetPortInt(activationInfo, InputPorts::Input_SideOfScreen);
                    }
                    else if (IsPortActive(activationInfo, InputPorts::Input_Disable))
                    {
                        gEnv->pInput->RemoveEventListener(this);
                    }
                }
                break;
            }
            case eFE_Update:
            {
                ActivateOutput(&m_activationInfo, OutputPorts::Output_X, m_touchDelta.x);
                ActivateOutput(&m_activationInfo, OutputPorts::Output_Y, m_touchDelta.y);
                break;
            }
            }
        }

        bool OnInputEvent(const SInputEvent& inputEvent) override
        {
            if (ShouldHandleInputEvent(inputEvent))
            {
                switch (inputEvent.deviceType)
                {
                case eIDT_Mouse:
                {
                    OnMouseEvent(inputEvent);
                }
                break;
                case eIDT_TouchScreen:
                {
                    OnTouchEvent(inputEvent);
                }
                break;
                }
            }

            return false;
        }

        void OnMouseEvent(const SInputEvent& inputEvent)
        {
            if (inputEvent.keyId == eKI_Mouse1)
            {
                if (inputEvent.state == eIS_Pressed)
                {
                    StartTracking(inputEvent);
                }
                else if (inputEvent.state == eIS_Released)
                {
                    StopTracking();
                }
                else if (inputEvent.state == eIS_Down)
                {
                    UpdateTracking(inputEvent.screenPosition);
                }
            }
        }

        void OnTouchEvent(const SInputEvent& inputEvent)
        {
            switch (inputEvent.state)
            {
            case eIS_Pressed:
            {
                if (m_touchFingerId == eKI_Unknown)
                {
                    m_touchFingerId = inputEvent.keyId;
                    StartTracking(inputEvent);
                }
            }
            break;

            case eIS_Released:
            {
                if (inputEvent.keyId == m_touchFingerId)
                {
                    m_touchFingerId = eKI_Unknown;
                    StopTracking();
                }
            }
            break;

            case eIS_Changed:
            {
                if (inputEvent.keyId == m_touchFingerId)
                {
                    UpdateTracking(inputEvent.screenPosition);
                }
            }
            break;
            }
        }

        void StartTracking(const SInputEvent& inputEvent)
        {
            m_touchDownPosition = inputEvent.screenPosition;
            m_touchDelta = ZERO;

            if (m_activationInfo.pGraph)
            {
                m_activationInfo.pGraph->SetRegularlyUpdated(m_activationInfo.myID, true);
            }
        }

        void StopTracking()
        {
            m_touchDelta = ZERO;

            ActivateOutput(&m_activationInfo, OutputPorts::Output_X, m_touchDelta.x);
            ActivateOutput(&m_activationInfo, OutputPorts::Output_Y, m_touchDelta.y);

            if (m_activationInfo.pGraph)
            {
                m_activationInfo.pGraph->SetRegularlyUpdated(m_activationInfo.myID, false);
            }
        }

        void UpdateTracking(const Vec2& inputPosition)
        {
            const float discRadius = gEnv->pRenderer->GetWidth() * m_radius;
            const float distScalar = 1.0f / discRadius;

            Vec2 dist = inputPosition - m_touchDownPosition;
            dist *= distScalar;

            m_touchDelta.x = CLAMP(dist.x, -1.0f, 1.0f);
            m_touchDelta.y = CLAMP(-dist.y, -1.0f, 1.0f);
        }

        bool ShouldHandleInputEvent(const SInputEvent& inputEvent)
        {
            // only accept input from mouse or touchscreen
            if (inputEvent.deviceType != eIDT_Mouse && inputEvent.deviceType != eIDT_TouchScreen)
            {
                return false;
            }

            const float screenHalf = gEnv->pRenderer->GetWidth() * 0.5f;

            return (m_sideOfScreen == SideOfScreen::Side_Any) ||
                   (m_sideOfScreen == SideOfScreen::Side_Left && inputEvent.screenPosition.x <= screenHalf) ||
                   (m_sideOfScreen == SideOfScreen::Side_Right && inputEvent.screenPosition.x >= screenHalf);
        }


    private:
        enum InputPorts
        {
            Input_Enable = 0,
            Input_Disable,
            Input_Radius,
            Input_SideOfScreen
        };

        enum OutputPorts
        {
            Output_X = 0,
            Output_Y
        };

        enum SideOfScreen
        {
            Side_Any = 0,
            Side_Left,
            Side_Right
        };

        SActivationInfo m_activationInfo;

        Vec2 m_touchDownPosition;
        Vec2 m_touchDelta;
        EKeyId m_touchFingerId;
        int m_sideOfScreen;
        float m_radius;
    };

    REGISTER_FLOW_NODE("Input:Touch:VirtualThumbstick", CFlowNode_TouchVirtualThumbstick);
};
