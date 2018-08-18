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
#include "UiFlow.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiFaderBus.h>

namespace
{
    const string g_faderNodePath = "UI:Fader:Animation";
    const string g_entFaderNodePath = "UIe:Fader:Animation";
}

#define FLOW_UI_FADER_NODE_COMMON_INPUT_PORT_CONFIGS \
    InputPortConfig_Void("Activate", _HELP("Trigger to start the fade animation"))

#define FLOW_UI_FADER_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS                                                                                                          \
    InputPortConfig<float>("StartValue", -1, _HELP("The value for the fade to start at (0 to 1). (-1 is a special value that means start from the current value)")), \
    InputPortConfig<float>("TargetValue", 1, _HELP("The value for the fade to end at (0 to 1)")),                                                                    \
    InputPortConfig<float>("Speed", 1, _HELP("1 would take one second to fade from 0 to 1, 2 would be twice as fast, 0.5 would be slow (0 is a special number that makes it instant)"))


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiFaderBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for fading an element and all its children
class CFlowUiFaderBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
    , public UiFaderNotificationBus::Handler
{
public:
    CFlowUiFaderBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , UiFaderNotificationBus::Handler()
        , m_actInfo(*activationInfo)
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiFaderBaseNode() override
    {
        UnregisterListener();
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_FADER_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the fader element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the fader element")),
            FLOW_UI_FADER_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS,
            {0}
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_FADER_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("uiElement_ElementName", _HELP("The name of the fader element")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the fader element")),
            FLOW_UI_FADER_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS,
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnComplete", _HELP("Triggers output when the fade is completed")),
            OutputPortConfig_Void("OnInterrupted", _HELP("Triggers output when the fade is interrupted by another fade starting")),
            {0}
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Animate the fade on the specified element");
        config.SetCategory(EFLN_APPROVED);
        if (m_hasTargetEntity)
        {
            config.nFlags |= EFLN_TARGET_ENTITY;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;

        int canvasIdInputPort = (m_hasTargetEntity ? -1 : InputPortCanvasId);
        int elementInputPort = (m_hasTargetEntity ? static_cast<int>(EntInputPortElementName) : static_cast<int>(InputPortElementId));

        int inputPortOffsetStart = (m_hasTargetEntity ? static_cast<int>(EntInputPortElementId) : static_cast<int>(InputPortElementId)) + 1;

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiFaderBus>(event, activationInfo,
                InputPortActivate, canvasIdInputPort, elementInputPort, true,
                (m_hasTargetEntity ? g_entFaderNodePath : g_faderNodePath), "Fader", outCanvasEntityId);

        if (entityId.IsValid())
        {
            // Set the starting value
            float startValue = GetPortFloat(activationInfo, inputPortOffsetStart + InputPortOffsetStartValue);
            if (startValue >= 0.0f)
            {
                // not the special value of -1 so set the fade value to the start value
                EBUS_EVENT_ID(entityId, UiFaderBus, SetFadeValue, startValue);
            }

            // Start the fade animation
            float targetValue = GetPortFloat(activationInfo, inputPortOffsetStart + InputPortOffsetTargetValue);
            float speed = GetPortFloat(activationInfo, inputPortOffsetStart + InputPortOffsetSpeed);
            EBUS_EVENT_ID(entityId, UiFaderBus, Fade, targetValue, speed);
            m_faderEntity = entityId;
            UiFaderNotificationBus::Handler::BusConnect(m_faderEntity);
        }
    }

    //-- ~IFlowNode --

    //-- UiFaderListener --

    void OnFadeComplete() override
    {
        ActivateOutput(&m_actInfo, OutputPortOnComplete, 0);
        UnregisterListener();
    }

    void OnFadeInterrupted() override
    {
        ActivateOutput(&m_actInfo, OutputPortOnInterrupted, 0);
        UnregisterListener();
    }

    void OnFaderDestroyed() override
    {
        UnregisterListener();
    }

    //-- ~UiFaderListener --

protected:
    //! Unregister from receiving fader events
    void UnregisterListener()
    {
        if (m_faderEntity.IsValid())
        {
            // Stop listening for this fader component's events
            UiFaderNotificationBus::Handler::BusDisconnect(m_faderEntity);
            m_faderEntity.SetInvalid();
        }
    }

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortCanvasId,
        InputPortElementId
    };

    enum EntInputPorts
    {
        EntInputPortActivate = 0,
        EntInputPortElementName,
        EntInputPortElementId
    };

    enum InputOffsetPorts
    {
        InputPortOffsetStartValue,
        InputPortOffsetTargetValue,
        InputPortOffsetSpeed
    };

    enum OutputPorts
    {
        OutputPortOnComplete = 0,
        OutputPortOnInterrupted
    };

    SActivationInfo m_actInfo;

    //! The entity this node triggered a fader component on, if any
    AZ::EntityId m_faderEntity;

    //! Specifies whether the node contains a canvas ID and an element ID port
    // or a target entity and an element name port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiFaderNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiFaderNode
    : public CFlowUiFaderBaseNode
{
public:
    CFlowUiFaderNode(SActivationInfo* activationInfo)
        : CFlowUiFaderBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiFaderNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntFaderNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntFaderNode
    : public CFlowUiFaderBaseNode
{
public:
    CFlowUiEntFaderNode(SActivationInfo* activationInfo)
        : CFlowUiFaderBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntFaderNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

REGISTER_FLOW_NODE(g_faderNodePath, CFlowUiFaderNode);
REGISTER_FLOW_NODE(g_entFaderNodePath, CFlowUiEntFaderNode);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_AND_SET_FLOAT(Fader, FadeValue,
    "Get the fade value",
    "Set the fade value",
    "Value", "The fade value of element [ElementID]");
