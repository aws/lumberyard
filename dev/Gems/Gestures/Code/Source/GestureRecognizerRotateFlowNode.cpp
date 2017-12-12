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
#include <Gestures/GestureRecognizerRotate.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerRotateFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
        , public IRotateListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        RecognizerRotateFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
            , m_activationInfo(*activationInfo)
            , m_recognizer(*this)
            , m_enabled(false)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~RecognizerRotateFlowNode() override
        {
            Disable();
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new RecognizerRotateFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum Inputs
        {
            Input_Enable = 0,
            Input_Disable,
            Input_MaxPixelsMoved,
            Input_MinAngleDegrees
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
            Output_RotationDegrees
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Enable", _HELP("Enable gesture recognizer")),
                InputPortConfig_Void("Disable", _HELP("Disable gesture recognizer")),
                InputPortConfig<float>("MaxPixelsMoved", m_recognizer.GetConfig().maxPixelsMoved, _HELP("The max distance in pixels that the touches can move towards or away from each other before a rotate will be recognized")),
                InputPortConfig<float>("MinAngleDegrees", m_recognizer.GetConfig().minAngleDegrees, _HELP("The min angle in degrees that must be rotated before the gesture will be recognized")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Initiated", _HELP("Activated when a continuous rotate gesture is initiated")),
                OutputPortConfig_Void("Updated", _HELP("Activated when a continuous rotate gesture is updated")),
                OutputPortConfig_Void("Ended", _HELP("Activated when a continuous rotate gesture is ended")),
                OutputPortConfig<float>("StartMidpointX", _HELP("X pixel position (midpoint) where the rotate started")),
                OutputPortConfig<float>("StartMidpointY", _HELP("Y pixel position (midpoint) where the rotate started")),
                OutputPortConfig<float>("StartDistance", _HELP("Pixel distance between the two touch positions when the rotate started")),
                OutputPortConfig<float>("CurrentMidpointX", _HELP("X pixel position (midpoint) where the rotate is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentMidpointY", _HELP("Y pixel position (midpoint) where the rotate is currently (or where it ended)")),
                OutputPortConfig<float>("CurrentDistance", _HELP("Pixel distance between the two touch positions currently (or when the rotate ended)")),
                OutputPortConfig<float>("RotationDegrees", _HELP("The current rotation in degress in the range [-180, 180]")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Recognizes continuous rotate gestures (the primary and/or secondary touches moving in a circular motion around the other)");
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
                if (IsPortActive(activationInfo, Input_MaxPixelsMoved))
                {
                    m_recognizer.GetConfig().maxPixelsMoved = GetPortFloat(activationInfo, Input_MaxPixelsMoved);
                }
                if (IsPortActive(activationInfo, Input_MinAngleDegrees))
                {
                    m_recognizer.GetConfig().minAngleDegrees = GetPortFloat(activationInfo, Input_MinAngleDegrees);
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
            ser.Value("maxPixelsMoved", m_recognizer.GetConfig().maxPixelsMoved);
            ser.Value("minAngleDegrees", m_recognizer.GetConfig().minAngleDegrees);

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
        void OnRotateInitiated(const RecognizerRotate& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Initiated, true);

            const Vec2 startPosition = recognizer.GetStartMidpoint();
            ActivateOutput(&m_activationInfo, Output_StartX, startPosition.x);
            ActivateOutput(&m_activationInfo, Output_StartY, startPosition.y);
            ActivateOutput(&m_activationInfo, Output_StartDistance, recognizer.GetStartDistance());

            OnRotateEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnRotateUpdated(const RecognizerRotate& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Updated, true);

            OnRotateEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnRotateEnded(const RecognizerRotate& recognizer) override
        {
            ActivateOutput(&m_activationInfo, Output_Ended, true);

            OnRotateEvent(recognizer);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnRotateEvent(const RecognizerRotate& recognizer)
        {
            const Vec2 currentPosition = recognizer.GetCurrentMidpoint();
            ActivateOutput(&m_activationInfo, Output_CurrentX, currentPosition.x);
            ActivateOutput(&m_activationInfo, Output_CurrentY, currentPosition.y);
            ActivateOutput(&m_activationInfo, Output_CurrentDistance, recognizer.GetCurrentDistance());

            ActivateOutput(&m_activationInfo, Output_RotationDegrees, recognizer.GetSignedRotationInDegrees());
        }

    private:
        SActivationInfo m_activationInfo;
        RecognizerRotate m_recognizer;
        bool m_enabled;
    };

    REGISTER_FLOW_NODE("Input:Gestures:Rotate", RecognizerRotateFlowNode)
}
