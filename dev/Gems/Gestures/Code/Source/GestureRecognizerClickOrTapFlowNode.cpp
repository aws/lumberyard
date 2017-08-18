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
#include "StdAfx.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <Gestures/GesturesBus.h>
#include <Gestures/GestureRecognizerClickOrTap.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerClickOrTapFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public IClickOrTapListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerClickOrTapFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_recognizer(*this)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerClickOrTapFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerClickOrTapFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_PointerIndex,
            Input_MinClicksOrTaps,
            Input_MaxSecondsHeld,
            Input_MaxPixelsMoved,
            Input_MaxSecondsBetweenClicksOrTaps,
            Input_MaxPixelsBetweenClicksOrTaps
        };

        enum Outputs
        {
            Output_Recognized = 0,
            Output_StartX,
            Output_StartY,
            Output_EndX,
            Output_EndY
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<int>("PointerIndex", m_recognizer.GetConfig().pointerIndex, _HELP("The pointer (button or finger) index to track")),
                InputPortConfig<int>("MinClicksOrTaps", m_recognizer.GetConfig().minClicksOrTaps, _HELP("The min number of clicks or taps required for the gesture to be recognized")),
                InputPortConfig<float>("MaxSecondsHeld", m_recognizer.GetConfig().maxSecondsHeld, _HELP("The max time in seconds allowed while held before the gesture stops being recognized")),
                InputPortConfig<float>("MaxPixelsMoved", m_recognizer.GetConfig().maxPixelsMoved, _HELP("The max distance in pixels allowed to move while held before the gesture stops being recognized")),
                InputPortConfig<float>("MaxSecondsBetweenClicksOrTaps", m_recognizer.GetConfig().maxSecondsBetweenClicksOrTaps, _HELP("The max time in seconds allowed between clicks or taps (only used when MinClicksOrTaps > 1)")),
                InputPortConfig<float>("MaxPixelsBetweenClicksOrTaps", m_recognizer.GetConfig().maxPixelsBetweenClicksOrTaps, _HELP("The max distance in pixels allowed between clicks or taps (only used when MinClicksOrTaps > 1)")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Recognized", _HELP("Activated when a discrete (or series of discrete) click (or tap) gesture is recognized")),
                OutputPortConfig<float>("StartX", _HELP("X screen position of the click or tap start in pixels")),
                OutputPortConfig<float>("StartY", _HELP("Y screen position of the click or tap start in pixels")),
                OutputPortConfig<float>("EndX", _HELP("X screen position of the click or tap end in pixels")),
                OutputPortConfig<float>("EndY", _HELP("Y screen position of the click or tap end in pixels")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes a discrete (or series of discrete) click (or tap) gestures");
            config.SetCategory(EFLN_APPROVED);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            m_activationInfo = *activationInfo;
            switch (event)
            {
            case eFE_Activate:
            case eFE_Initialize:
            {
                if (IsPortActive(activationInfo, Input_PointerIndex))
                {
                    m_recognizer.GetConfig().pointerIndex = GetPortInt(activationInfo, Input_PointerIndex);
                }
                if (IsPortActive(activationInfo, Input_MinClicksOrTaps))
                {
                    m_recognizer.GetConfig().minClicksOrTaps = GetPortInt(activationInfo, Input_MinClicksOrTaps);
                }
                if (IsPortActive(activationInfo, Input_MaxSecondsHeld))
                {
                    m_recognizer.GetConfig().maxSecondsHeld = GetPortFloat(activationInfo, Input_MaxSecondsHeld);
                }
                if (IsPortActive(activationInfo, Input_MaxPixelsMoved))
                {
                    m_recognizer.GetConfig().maxPixelsMoved = GetPortFloat(activationInfo, Input_MaxPixelsMoved);
                }
                if (IsPortActive(activationInfo, Input_MaxSecondsBetweenClicksOrTaps))
                {
                    m_recognizer.GetConfig().maxSecondsBetweenClicksOrTaps = GetPortFloat(activationInfo, Input_MaxSecondsBetweenClicksOrTaps);
                }
                if (IsPortActive(activationInfo, Input_MaxPixelsBetweenClicksOrTaps))
                {
                    m_recognizer.GetConfig().maxPixelsBetweenClicksOrTaps = GetPortFloat(activationInfo, Input_MaxPixelsBetweenClicksOrTaps);
                }

                if (IsPortActive(activationInfo, Input_Disable))
                {
                    Disable();
                }
                else if (IsPortActive(activationInfo, Input_Enable))
                {
                    Enable();
                }
            }
            break;
            case eFE_Uninitialize:
            {
                Disable();
            }
            break;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
        {
            ser.Value("pointerIndex", m_recognizer.GetConfig().pointerIndex);
            ser.Value("minClicksOrTaps", m_recognizer.GetConfig().minClicksOrTaps);
            ser.Value("maxSecondsHeld", m_recognizer.GetConfig().maxSecondsHeld);
            ser.Value("maxPixelsMoved", m_recognizer.GetConfig().maxPixelsMoved);
            ser.Value("maxSecondsBetweenClicksOrTaps", m_recognizer.GetConfig().maxSecondsBetweenClicksOrTaps);
            ser.Value("maxPixelsBetweenClicksOrTaps", m_recognizer.GetConfig().maxPixelsBetweenClicksOrTaps);

            bool enabled = m_enabled;
            ser.Value("enabled", enabled);
            if (ser.IsReading())
            {
                enabled ? Enable() : Disable();
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void Enable()
        {
            if (!m_enabled)
            {
                m_enabled = true;
                EBUS_EVENT(GesturesBus, Register, m_recognizer);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void Disable()
        {
            if (m_enabled)
            {
                m_enabled = false;
                EBUS_EVENT(GesturesBus, Deregister, m_recognizer);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnClickOrTapRecognized(const RecognizerClickOrTap& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Recognized, true);

            const Vec2 startPosition = recognizer.GetStartPosition();
            ActivateOutput(&m_activationInfo, Output_StartX, startPosition.x);
            ActivateOutput(&m_activationInfo, Output_StartY, startPosition.y);

            const Vec2 endPosition = recognizer.GetEndPosition();
            ActivateOutput(&m_activationInfo, Output_EndX, endPosition.x);
            ActivateOutput(&m_activationInfo, Output_EndY, endPosition.y);
        }

    private:
        SActivationInfo m_activationInfo;
        RecognizerClickOrTap m_recognizer;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:ClickOrTap", RecognizerClickOrTapFlowNode)
}
