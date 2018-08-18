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
#include <LyShine/Bus/UiDynamicScrollBoxBus.h>

namespace
{
    const string g_dynamicScrollBoxRefreshNodePath = "UI:DynamicScrollBox:RefreshContent";
    const string g_dynamicScrollBoxGetLocationIndexOfChildNodePath = "UI:DynamicScrollBox:GetLocationIndexOfChild";

    const string g_entDynamicScrollBoxRefreshNodePath = "UIe:DynamicScrollBox:RefreshContent";
    const string g_entDynamicScrollBoxGetLocationIndexOfChildNodePath = "UIe:DynamicScrollBox:GetLocationIndexOfChild";
}

#define FLOW_UI_DYNAMICSCROLLBOX_NODE_COMMON_INPUT_PORT_CONFIGS \
    InputPortConfig_Void("Activate", _HELP("Trigger this node"))


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiDynamicScrollBoxRefreshBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for refreshing the content
class CFlowUiDynamicScrollBoxRefreshBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiDynamicScrollBoxRefreshBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiDynamicScrollBoxRefreshBaseNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_DYNAMICSCROLLBOX_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_DYNAMICSCROLLBOX_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Fired after the node is finished")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Refresh the content");
        config.SetCategory(EFLN_APPROVED);
        if (m_hasTargetEntity)
        {
            config.nFlags |= EFLN_TARGET_ENTITY;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        int canvasIdInputPort = (m_hasTargetEntity ? -1 : InputPortCanvasId);
        int elementInputPort = (m_hasTargetEntity ? static_cast<int>(EntInputPortElementName) : static_cast<int>(InputPortElementId));

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, canvasIdInputPort, elementInputPort, true,
            (m_hasTargetEntity ? g_entDynamicScrollBoxRefreshNodePath : g_dynamicScrollBoxRefreshNodePath), "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            EBUS_EVENT_ID(entityId, UiDynamicScrollBoxBus, RefreshContent);

            ActivateOutput(activationInfo, OutputPortOnComplete, 0);
        }
    }

    //-- ~IFlowNode --

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortCanvasId,
        InputPortElementId,
    };

    enum EntInputPorts
    {
        EntInputPortActivate = 0,
        EntInputPortElementName,
        EntInputPortElementId
    };

    enum OutputPorts
    {
        OutputPortOnComplete = 0
    };

    //! Specifies whether the node contains a canvas ID and an element ID port
    // or a target entity and an element name port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiDynamicScrollBoxRefreshNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiDynamicScrollBoxRefreshNode
    : public CFlowUiDynamicScrollBoxRefreshBaseNode
{
public:
    CFlowUiDynamicScrollBoxRefreshNode(SActivationInfo* activationInfo)
        : CFlowUiDynamicScrollBoxRefreshBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiDynamicScrollBoxRefreshNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntDynamicScrollBoxRefreshNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntDynamicScrollBoxRefreshNode
    : public CFlowUiDynamicScrollBoxRefreshBaseNode
{
public:
    CFlowUiEntDynamicScrollBoxRefreshNode(SActivationInfo* activationInfo)
        : CFlowUiDynamicScrollBoxRefreshBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntDynamicScrollBoxRefreshNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for getting the index of a child element
class CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_DYNAMICSCROLLBOX_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<int>("ChildElementID", 0, _HELP("The unique ID of the child element")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_DYNAMICSCROLLBOX_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<int>("ChildElementID", 0, _HELP("The unique ID of the child element")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("Index", _HELP("Location index of the child element")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get the location index of a child element");
        config.SetCategory(EFLN_APPROVED);
        if (m_hasTargetEntity)
        {
            config.nFlags |= EFLN_TARGET_ENTITY;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        int canvasIdInputPort = (m_hasTargetEntity ? -1 : InputPortCanvasId);
        int elementInputPort = (m_hasTargetEntity ? static_cast<int>(EntInputPortElementName) : static_cast<int>(InputPortElementId));

        const string& nodePath = m_hasTargetEntity ? g_entDynamicScrollBoxGetLocationIndexOfChildNodePath : g_dynamicScrollBoxGetLocationIndexOfChildNodePath;

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, canvasIdInputPort, elementInputPort, true,
            nodePath, "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            int inputPortChildElementId = (m_hasTargetEntity ? static_cast<int>(EntInputPortChildElementId) : static_cast<int>(InputPortChildElementId));
            int childElementId = GetPortInt(activationInfo, inputPortChildElementId);

            AZ::Entity* childElement = nullptr;
            EBUS_EVENT_ID_RESULT(childElement, outCanvasEntityId, UiCanvasBus, FindElementById, childElementId);

            if (childElement)
            {
                int index = -1;
                EBUS_EVENT_ID_RESULT(index, entityId, UiDynamicScrollBoxBus, GetLocationIndexOfChild, childElement->GetId());

                if (index >= 0)
                {
                    ActivateOutput(activationInfo, OutputPortChildElementIndex, index);
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find child with ID: %d\n", nodePath.c_str(), childElementId);
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find UI element with ID: %d\n", nodePath.c_str(), childElementId);
            }
        }
    }

    //-- ~IFlowNode --

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortCanvasId,
        InputPortElementId,
        InputPortChildElementId
    };

    enum EntInputPorts
    {
        EntInputPortActivate = 0,
        EntInputPortElementName,
        EntInputPortElementId,
        EntInputPortChildElementId
    };

    enum OutputPorts
    {
        OutputPortChildElementIndex = 0
    };

    //! Specifies whether the node contains a canvas ID and an element ID port
    // or a target entity and an element name port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiDynamicScrollBoxGetLocationIndexOfChildNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiDynamicScrollBoxGetLocationIndexOfChildNode
    : public CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode
{
public:
    CFlowUiDynamicScrollBoxGetLocationIndexOfChildNode(SActivationInfo* activationInfo)
        : CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiDynamicScrollBoxGetLocationIndexOfChildNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntDynamicScrollBoxGetLocationIndexOfChildNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntDynamicScrollBoxGetLocationIndexOfChildNode
    : public CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode
{
public:
    CFlowUiEntDynamicScrollBoxGetLocationIndexOfChildNode(SActivationInfo* activationInfo)
        : CFlowUiDynamicScrollBoxGetLocationIndexOfChildBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntDynamicScrollBoxGetLocationIndexOfChildNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

REGISTER_FLOW_NODE(g_dynamicScrollBoxRefreshNodePath, CFlowUiDynamicScrollBoxRefreshNode);
REGISTER_FLOW_NODE(g_dynamicScrollBoxGetLocationIndexOfChildNodePath, CFlowUiDynamicScrollBoxGetLocationIndexOfChildNode);

REGISTER_FLOW_NODE(g_entDynamicScrollBoxRefreshNodePath, CFlowUiEntDynamicScrollBoxRefreshNode);
REGISTER_FLOW_NODE(g_entDynamicScrollBoxGetLocationIndexOfChildNodePath, CFlowUiEntDynamicScrollBoxGetLocationIndexOfChildNode);
