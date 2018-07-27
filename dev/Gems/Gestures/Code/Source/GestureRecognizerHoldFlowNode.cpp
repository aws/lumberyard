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
#include "Gestures_precompiled.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <Gestures/GestureRecognizerHold.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerHoldFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public RecognizerHold
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerHoldFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerHoldFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerHoldFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_PointerIndex,
            Input_MinSecondsHeld,
            Input_MaxPixelsMoved
        };

        enum Outputs
        {
            Output_Initiated = 0,
            Output_Updated,
            Output_Ended,
            Output_StartX,
            Output_StartY,
            Output_CurrentX,
            Output_CurrentY,
            Output_Duration
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<int>("PointerIndex", GetConfig().pointerIndex, _HELP("The pointer (button or finger) index to track")),
                InputPortConfig<float>("MinSecondsHeld", GetConfig().minSecondsHeld, _HELP("The min time in seconds after the initial press before a hold will be recognized")),
                InputPortConfig<float>("MaxPixelsMoved", GetConfig().maxPixelsMoved, _HELP("The max distance in pixels that can be moved before a hold stops being recognized")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Initiated", _HELP("Activated when a continuous hold gesture is initiated")),
                OutputPortConfig_Void("Updated", _HELP("Activated when a continuous hold gesture is updated")),
                OutputPortConfig_Void("Ended", _HELP("Activated when a continuous hold gesture is ended")),
                OutputPortConfig<float>("StartX", _HELP("X pixel position where the hold started")),
                OutputPortConfig<float>("StartY", _HELP("Y pixel position where the hold started")),
                OutputPortConfig<float>("CurrentX", _HELP("X pixel position where the hold is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentY", _HELP("Y pixel position where the hold is currently (or where it ended)")),
                OutputPortConfig<float>("Duration", _HELP("Duration of the hold in seconds")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes continuous hold gestures");
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
                    GetConfig().pointerIndex = GetPortInt(activationInfo, Input_PointerIndex);
                }
                if (IsPortActive(activationInfo, Input_MinSecondsHeld))
                {
                    GetConfig().minSecondsHeld = GetPortFloat(activationInfo, Input_MinSecondsHeld);
                }
                if (IsPortActive(activationInfo, Input_MaxPixelsMoved))
                {
                    GetConfig().maxPixelsMoved = GetPortFloat(activationInfo, Input_MaxPixelsMoved);
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
            ser.Value("pointerIndex", GetConfig().pointerIndex);
            ser.Value("minSecondsHeld", GetConfig().minSecondsHeld);
            ser.Value("maxPixelsMoved", GetConfig().maxPixelsMoved);

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
                BusConnect();
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void Disable()
        {
            if (m_enabled)
            {
                m_enabled = false;
                BusDisconnect();
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnContinuousGestureInitiated() override
        {
            ActivateOutput(&m_activationInfo, Output_Initiated, true);

            const AZ::Vector2 startPosition = GetStartPosition();
            ActivateOutput(&m_activationInfo, Output_StartX, startPosition.GetX());
            ActivateOutput(&m_activationInfo, Output_StartY, startPosition.GetY());

            OnHoldEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnContinuousGestureUpdated() override
        {
            ActivateOutput(&m_activationInfo, Output_Updated, true);

            OnHoldEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnContinuousGestureEnded() override
        {
            ActivateOutput(&m_activationInfo, Output_Ended, true);

            OnHoldEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnHoldEvent()
        {
            const AZ::Vector2 currentPosition = GetCurrentPosition();
            ActivateOutput(&m_activationInfo, Output_CurrentX, currentPosition.GetX());
            ActivateOutput(&m_activationInfo, Output_CurrentY, currentPosition.GetY());

            ActivateOutput(&m_activationInfo, Output_Duration, GetDuration());
        }

    private:
        SActivationInfo m_activationInfo;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:Hold", RecognizerHoldFlowNode)
}
