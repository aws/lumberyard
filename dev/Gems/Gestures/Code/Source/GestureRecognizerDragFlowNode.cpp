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

#include <Gestures/GestureRecognizerDrag.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerDragFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public RecognizerDrag
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerDragFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerDragFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerDragFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_PointerIndex,
            Input_MinSecondsHeld,
            Input_MinPixelsMoved
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
            Output_DeltaX,
            Output_DeltaY,
            Output_Distance
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<int>("PointerIndex", GetConfig().pointerIndex, _HELP("The pointer (button or finger) index to track")),
                InputPortConfig<float>("MinSecondsHeld", GetConfig().minSecondsHeld, _HELP("The min time in seconds after the initial press before a drag will be recognized")),
                InputPortConfig<float>("MinPixelsMoved", GetConfig().minPixelsMoved, _HELP("The min distance in pixels that must be dragged before a drag will be recognized")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Initiated", _HELP("Activated when a continuous drag gesture is initiated")),
                OutputPortConfig_Void("Updated", _HELP("Activated when a continuous drag gesture is updated")),
                OutputPortConfig_Void("Ended", _HELP("Activated when a continuous drag gesture is ended")),
                OutputPortConfig<float>("StartX", _HELP("X pixel position where the drag started")),
                OutputPortConfig<float>("StartY", _HELP("Y pixel position where the drag started")),
                OutputPortConfig<float>("CurrentX", _HELP("X pixel position where the drag is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentY", _HELP("Y pixel position where the drag is currently (or where it ended)")),
                OutputPortConfig<float>("DeltaX", _HELP("X pixels dragged (CurrentX - StartX)")),
                OutputPortConfig<float>("DeltaY", _HELP("Y pixels dragged (CurrentY - StartY)")),
                OutputPortConfig<float>("Distance", _HELP("Pixel distance from the drag's start position to its current (or end) position")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes continuous drag gestures");
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
                if (IsPortActive(activationInfo, Input_MinPixelsMoved))
                {
                    GetConfig().minPixelsMoved = GetPortFloat(activationInfo, Input_MinPixelsMoved);
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
            ser.Value("minPixelsMoved", GetConfig().minPixelsMoved);

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

            OnDragEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnContinuousGestureUpdated() override
        {
            ActivateOutput(&m_activationInfo, Output_Updated, true);

            OnDragEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnContinuousGestureEnded() override
        {
            ActivateOutput(&m_activationInfo, Output_Ended, true);

            OnDragEvent();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnDragEvent()
        {
            const AZ::Vector2 currentPosition = GetCurrentPosition();
            ActivateOutput(&m_activationInfo, Output_CurrentX, currentPosition.GetX());
            ActivateOutput(&m_activationInfo, Output_CurrentY, currentPosition.GetY());

            const AZ::Vector2 currentDelta = GetDelta();
            ActivateOutput(&m_activationInfo, Output_DeltaX, currentDelta.GetX());
            ActivateOutput(&m_activationInfo, Output_DeltaY, currentDelta.GetY());

            ActivateOutput(&m_activationInfo, Output_Distance, GetDistance());
        }

    private:
        SActivationInfo m_activationInfo;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:Drag", RecognizerDragFlowNode)
}
