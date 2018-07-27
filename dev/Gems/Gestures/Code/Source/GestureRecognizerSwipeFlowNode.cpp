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

#include <Gestures/GestureRecognizerSwipe.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerSwipeFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public RecognizerSwipe
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerSwipeFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerSwipeFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerSwipeFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_PointerIndex,
            Input_MaxSecondsHeld,
            Input_MinPixelsMoved
        };

        enum Outputs
        {
            Output_Recognized = 0,
            Output_StartX,
            Output_StartY,
            Output_EndX,
            Output_EndY,
            Output_DeltaX,
            Output_DeltaY,
            Output_DirectionX,
            Output_DirectionY,
            Output_Distance,
            Output_Duration,
            Output_Velocity
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<int>("PointerIndex", GetConfig().pointerIndex, _HELP("The pointer (button or finger) index to track")),
                InputPortConfig<float>("MaxSecondsHeld", GetConfig().maxSecondsHeld, _HELP("The max time in seconds after the initial press for a swipe to be recognized")),
                InputPortConfig<float>("MinPixelsMoved", GetConfig().minPixelsMoved, _HELP("The min distance in pixels that must be moved before a swipe will be recognized")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Recognized", _HELP("Activated when a discrete swipe gesture is recognized")),
                OutputPortConfig<float>("StartX", _HELP("X pixel position where the swipe started")),
                OutputPortConfig<float>("StartY", _HELP("Y pixel position where the swipe started")),
                OutputPortConfig<float>("EndX", _HELP("X pixel position where the swipe ended")),
                OutputPortConfig<float>("EndY", _HELP("Y pixel position where the swipe ended")),
                OutputPortConfig<float>("DeltaX", _HELP("X pixels swiped (EndX - StartX)")),
                OutputPortConfig<float>("DeltaY", _HELP("Y pixels swiped (EndY - StartY)")),
                OutputPortConfig<float>("DirectionX", _HELP("X direction of the swipe (normalized DeltaX, DeltaY)")),
                OutputPortConfig<float>("DirectionY", _HELP("Y direction of the swipe (normalized DeltaX, DeltaY)")),
                OutputPortConfig<float>("Distance", _HELP("Distance of the swipe in pixels")),
                OutputPortConfig<float>("Duration", _HELP("Duration of the swipe in seconds")),
                OutputPortConfig<float>("Velocity", _HELP("Velocity of the swipe in pixels per second")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes discrete swipe gestures");
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
                if (IsPortActive(activationInfo, Input_MaxSecondsHeld))
                {
                    GetConfig().maxSecondsHeld = GetPortFloat(activationInfo, Input_MaxSecondsHeld);
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
            ser.Value("maxSecondsHeld", GetConfig().maxSecondsHeld);
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
        void OnDiscreteGestureRecognized() override
        {
            ActivateOutput(&m_activationInfo, Output_Recognized, true);

            const AZ::Vector2 startPosition = GetStartPosition();
            ActivateOutput(&m_activationInfo, Output_StartX, startPosition.GetX());
            ActivateOutput(&m_activationInfo, Output_StartY, startPosition.GetY());

            const AZ::Vector2 endPosition = GetEndPosition();
            ActivateOutput(&m_activationInfo, Output_EndX, endPosition.GetX());
            ActivateOutput(&m_activationInfo, Output_EndY, endPosition.GetY());

            const AZ::Vector2 delta = GetDelta();
            ActivateOutput(&m_activationInfo, Output_DeltaX, delta.GetX());
            ActivateOutput(&m_activationInfo, Output_DeltaY, delta.GetY());

            const AZ::Vector2 direction = GetDirection();
            ActivateOutput(&m_activationInfo, Output_DirectionX, direction.GetX());
            ActivateOutput(&m_activationInfo, Output_DirectionY, direction.GetY());

            ActivateOutput(&m_activationInfo, Output_Distance, GetDistance());
            ActivateOutput(&m_activationInfo, Output_Duration, GetDuration());
            ActivateOutput(&m_activationInfo, Output_Velocity, GetVelocity());
        }

    private:
        SActivationInfo m_activationInfo;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:Swipe", RecognizerSwipeFlowNode)
}
