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
#include <Gestures/GestureRecognizerPinch.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerPinchFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public IPinchListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerPinchFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_recognizer(*this)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerPinchFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerPinchFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_MinPixelsMoved,
            Input_MaxAngleDegrees
        };

        enum Outputs
        {
            Output_Initiated = 0,
            Output_Updated,
            Output_Ended,
            Output_StartX,
            Output_StartY,
            Output_StartDistance,
            Output_CurrentX,
            Output_CurrentY,
            Output_CurrentDistance,
            Output_PinchRatio
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<float>("MinPixelsMoved", m_recognizer.GetConfig().minPixelsMoved, _HELP("The min distance in pixels that must be pinched before a pinch will be recognized")),
                InputPortConfig<float>("MaxAngleDegrees", m_recognizer.GetConfig().maxAngleDegrees, _HELP("The max angle in degrees that a pinch can deviate before it will be recognized")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Initiated", _HELP("Activated when a continuous pinch gesture is initiated")),
                OutputPortConfig_Void("Updated", _HELP("Activated when a continuous pinch gesture is updated")),
                OutputPortConfig_Void("Ended", _HELP("Activated when a continuous pinch gesture is ended")),
                OutputPortConfig<float>("StartMidpointX", _HELP("X pixel position (midpoint) where the pinch started")),
                OutputPortConfig<float>("StartMidpointY", _HELP("Y pixel position (midpoint) where the pinch started")),
                OutputPortConfig<float>("StartDistance", _HELP("Pixel distance between the two touch positions when the pinch started")),
                OutputPortConfig<float>("CurrentMidpointX", _HELP("X pixel position (midpoint) where the pinch is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentMidpointY", _HELP("Y pixel position (midpoint) where the pinch is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentDistance", _HELP("Pixel distance between the two touch positions currently (or when the pinch ended)")),
                OutputPortConfig<float>("Ratio", _HELP("The ratio of the pinch (CurrentDistance / StartDistance)")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes continuous pinch gestures (the primary and secondary touches moving towards or away from each other)");
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
                if (IsPortActive(activationInfo, Input_MinPixelsMoved))
                {
                    m_recognizer.GetConfig().minPixelsMoved = GetPortFloat(activationInfo, Input_MinPixelsMoved);
                }
                if (IsPortActive(activationInfo, Input_MaxAngleDegrees))
                {
                    m_recognizer.GetConfig().maxAngleDegrees = GetPortFloat(activationInfo, Input_MaxAngleDegrees);
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
            ser.Value("minPixelsMoved", m_recognizer.GetConfig().minPixelsMoved);
            ser.Value("maxAngleDegrees", m_recognizer.GetConfig().maxAngleDegrees);

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
        void OnPinchInitiated(const RecognizerPinch& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Initiated, true);

            const Vec2 startPosition = recognizer.GetStartMidpoint();
            ActivateOutput(&m_activationInfo, Output_StartX, startPosition.x);
            ActivateOutput(&m_activationInfo, Output_StartY, startPosition.y);
            ActivateOutput(&m_activationInfo, Output_StartDistance, recognizer.GetStartDistance());

            OnPinchEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPinchUpdated(const RecognizerPinch& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Updated, true);

            OnPinchEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPinchEnded(const RecognizerPinch& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Ended, true);

            OnPinchEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPinchEvent(const RecognizerPinch& recognizer)
        {
            const Vec2 currentPosition = recognizer.GetCurrentMidpoint();
            ActivateOutput(&m_activationInfo, Output_CurrentX, currentPosition.x);
            ActivateOutput(&m_activationInfo, Output_CurrentY, currentPosition.y);
            ActivateOutput(&m_activationInfo, Output_CurrentDistance, recognizer.GetCurrentDistance());

            ActivateOutput(&m_activationInfo, Output_PinchRatio, recognizer.GetPinchRatio());
        }

    private:
        SActivationInfo m_activationInfo;
        RecognizerPinch m_recognizer;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:Pinch", RecognizerPinchFlowNode)
}
