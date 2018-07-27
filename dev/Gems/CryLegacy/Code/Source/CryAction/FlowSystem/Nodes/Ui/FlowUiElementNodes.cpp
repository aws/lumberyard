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
#include <LyShine/Bus/UiElementBus.h>

namespace
{
    const string g_elementGetChildAtIndexNodePath = "UI:Element:GetChildAtIndex";
    const string g_elementGetIndexOfChildNodePath = "UI:Element:GetIndexOfChild";
    const string g_elementGetParentNodePath = "UI:Element:GetParent";

    const string g_entElementGetChildAtIndexNodePath = "UIe:Element:GetChildAtIndex";
    const string g_entElementGetIndexOfChildNodePath = "UIe:Element:GetIndexOfChild";
    const string g_entElementGetParentNodePath = "UIe:Element:GetParent";
    const string g_entElementGetChildByNameNodePath = "UIe:Element:GetChildByName";
}

#define FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS \
    InputPortConfig_Void("Activate", _HELP("Trigger this node"))


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiElementGetChildAtIndexBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for getting a child at index
class CFlowUiElementGetChildAtIndexBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiElementGetChildAtIndexBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiElementGetChildAtIndexBaseNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<int>("ChildIndex", 0, _HELP("The index of the child")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<int>("ChildIndex", 0, _HELP("The index of the child")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("ChildElementID", _HELP("The ID of the child element")),
            { 0 }
        };

        static const SOutputPortConfig entOutputs[] =
        {
            OutputPortConfig<string>("ChildElementName", _HELP("The name of the child element")),
            OutputPortConfig<int>("ChildElementID", _HELP("The ID of the child element")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = m_hasTargetEntity ? entOutputs : outputs;
        config.sDescription = _HELP("Get a child at index");
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

        const string& nodePath = m_hasTargetEntity ? g_entElementGetChildAtIndexNodePath : g_elementGetChildAtIndexNodePath;

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, canvasIdInputPort, elementInputPort, true,
            nodePath, "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            int inputPortChildIndex = (m_hasTargetEntity ? static_cast<int>(EntInputPortChildIndex) : static_cast<int>(InputPortChildIndex));
            int childIndex = GetPortInt(activationInfo, inputPortChildIndex);

            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_ID_RESULT(childEntity, entityId, UiElementBus, GetChildElement, childIndex);

            if (childEntity)
            {
                // Get the element ID from the entity ID
                LyShine::ElementId childElementId = 0;
                EBUS_EVENT_ID_RESULT(childElementId, childEntity->GetId(), UiElementBus, GetElementId);

                if (m_hasTargetEntity)
                {
                    AZStd::string childElementName = childEntity->GetName();
                    string childElementNameOut = childElementName.c_str();
                    ActivateOutput(activationInfo, OutputPortChildElementNameOrId, childElementNameOut);
                    ActivateOutput(activationInfo, OutputPortChildElementId, childElementId);
                }
                else
                {

                    ActivateOutput(activationInfo, OutputPortChildElementNameOrId, childElementId);
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find child at index: %d\n", nodePath.c_str(), childIndex);
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
        InputPortChildIndex
    };

    enum EntInputPorts
    {
        EntInputPortActivate = 0,
        EntInputPortElementName,
        EntInputPortElementId,
        EntInputPortChildIndex
    };

    enum OutputPorts
    {
        OutputPortChildElementNameOrId = 0,
        OutputPortChildElementId,
    };

    //! Specifies whether the node contains a canvas ID and an element ID port
    // or a target entity and an element name port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiElementGetChildAtIndexNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiElementGetChildAtIndexNode
    : public CFlowUiElementGetChildAtIndexBaseNode
{
public:
    CFlowUiElementGetChildAtIndexNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetChildAtIndexBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiElementGetChildAtIndexNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntElementGetChildAtIndexNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntElementGetChildAtIndexNode
    : public CFlowUiElementGetChildAtIndexBaseNode
{
public:
    CFlowUiEntElementGetChildAtIndexNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetChildAtIndexBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntElementGetChildAtIndexNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiElementGetIndexOfChildBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for getting the index of a child
class CFlowUiElementGetIndexOfChildBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiElementGetIndexOfChildBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiElementGetIndexOfChildBaseNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<int>("ChildElementID", 0, _HELP("The unique ID of the child element")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<string>("ChildElementName", _HELP("The name of the child element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ChildElementID", 0, _HELP("The unique ID of the child element")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("IndexOfChild", _HELP("The index of the child")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get the index of a child");
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

        const string& nodePath = m_hasTargetEntity ? g_entElementGetIndexOfChildNodePath : g_elementGetIndexOfChildNodePath;

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, canvasIdInputPort, elementInputPort, true,
            nodePath, "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            AZ::Entity* childElement = nullptr;
            if (m_hasTargetEntity)
            {
                string childElementName = GetPortString(activationInfo, EntInputPortChildElementName);
                if (!childElementName.empty())
                {
                    EBUS_EVENT_ID_RESULT(childElement, entityId, UiElementBus, FindChildByName, childElementName.c_str());

                    if (!childElement)
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find child with name: %s\n", nodePath.c_str(), childElementName.c_str());
                    }
                }
            }

            if (!childElement)
            {
                int childElementId = GetPortInt(activationInfo, m_hasTargetEntity ? static_cast<int>(EntInputPortChildElementId) : static_cast<int>(InputPortChildElementId));

                AZ::Entity* entity;
                EBUS_EVENT_ID_RESULT(entity, outCanvasEntityId, UiCanvasBus, FindElementById, childElementId);

                if (entity)
                {
                    EBUS_EVENT_ID_RESULT(childElement, entityId, UiElementBus, FindChildByEntityId, entity->GetId());

                    if (!childElement)
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find child with ID: %d\n", nodePath.c_str(), childElementId);
                    }
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find element with ID: %d\n", nodePath.c_str(), childElementId);
                }

            }

            if (childElement)
            {
                int childIndex = -1;
                EBUS_EVENT_ID_RESULT(childIndex, entityId, UiElementBus, GetIndexOfChild, childElement);
                AZ_Assert(childIndex >= 0, "FlowGraph: %s Node: invalid index for child\n", nodePath.c_str());
                if (childIndex >= 0)
                {
                    ActivateOutput(activationInfo, OutputPortChildElementIndex, childIndex);
                }
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
        EntInputPortChildElementName,
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
// CFlowUiElementGetIndexOfChildNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiElementGetIndexOfChildNode
    : public CFlowUiElementGetIndexOfChildBaseNode
{
public:
    CFlowUiElementGetIndexOfChildNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetIndexOfChildBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiElementGetIndexOfChildNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntElementGetIndexOfChildNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntElementGetIndexOfChildNode
    : public CFlowUiElementGetIndexOfChildBaseNode
{
public:
    CFlowUiEntElementGetIndexOfChildNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetIndexOfChildBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntElementGetIndexOfChildNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiElementGetParentBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for getting a child's parent
class CFlowUiElementGetParentBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiElementGetParentBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_hasTargetEntity(hasTargetEntity)
    {
    }

    ~CFlowUiElementGetParentBaseNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("ParentElementID", _HELP("The ID of the parent element")),
            { 0 }
        };

        static const SOutputPortConfig entOutputs[] =
        {
            OutputPortConfig<string>("ParentElementName", _HELP("The name of the parent element")),
            OutputPortConfig<int>("ParentElementID", _HELP("The ID of the parent element")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = m_hasTargetEntity ? entOutputs : outputs;
        config.sDescription = _HELP("Get a child's parent");
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

        const string& nodePath = m_hasTargetEntity ? g_entElementGetParentNodePath : g_elementGetParentNodePath;

        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, canvasIdInputPort, elementInputPort, true,
            nodePath, "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            AZ::Entity* parentElement = nullptr;
            EBUS_EVENT_ID_RESULT(parentElement, entityId, UiElementBus, GetParent);
            if (parentElement)
            {
                // Get the element ID from the entity ID
                LyShine::ElementId parentElementId = 0;
                EBUS_EVENT_ID_RESULT(parentElementId, parentElement->GetId(), UiElementBus, GetElementId);

                if (m_hasTargetEntity)
                {
                    AZStd::string parentElementName = parentElement->GetName();
                    string parentElementNameOut = parentElementName.c_str();
                    ActivateOutput(activationInfo, OutputPortParentElementIdOrName, parentElementNameOut);
                    ActivateOutput(activationInfo, OutputPortParentElementId, parentElementId);
                }
                else
                {

                    ActivateOutput(activationInfo, OutputPortParentElementIdOrName, parentElementId);
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find parent for UI element\n", nodePath.c_str());
            }
        }
    }

    //-- ~IFlowNode --

protected:

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

    enum OutputPorts
    {
        OutputPortParentElementIdOrName = 0,
        OutputPortParentElementId,
    };

    //! Specifies whether the node contains a canvas ID and an element ID port
    // or a target entity and an element name port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiElementGetParentNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiElementGetParentNode
    : public CFlowUiElementGetParentBaseNode
{
public:
    CFlowUiElementGetParentNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetParentBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiElementGetParentNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntElementGetParentNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntElementGetParentNode
    : public CFlowUiElementGetParentBaseNode
{
public:
    CFlowUiEntElementGetParentNode(SActivationInfo* activationInfo)
        : CFlowUiElementGetParentBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntElementGetChildAtIndexNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntElementGetChildByNameNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for getting a child element id by name
class CFlowUiEntElementGetChildByNameNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiEntElementGetChildByNameNode(SActivationInfo* activationInfo)
        : CFlowBaseNode()
    {
    }

    ~CFlowUiEntElementGetChildByNameNode() override
    {
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_ELEMENT_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<string>("ElementName", _HELP("The name of the element"), 0, _UICONFIG("dt=uiElement")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element")),
            InputPortConfig<string>("ChildElementName", _HELP("The name of the child element"), 0, _UICONFIG("dt=uiElement")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("ChildElementID", _HELP("The ID of the child element")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get the ID of a child element");
        config.SetCategory(EFLN_APPROVED);
        config.nFlags |= EFLN_TARGET_ENTITY;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        AZ::EntityId outCanvasEntityId;
        AZ::EntityId entityId = UiFlow::IfActivatedGetElementEntityId<UiElementBus>(event, activationInfo,
            InputPortActivate, -1, InputPortElementName, true,
            g_entElementGetChildByNameNodePath, "Element", outCanvasEntityId);

        if (entityId.IsValid())
        {
            string childElementName = GetPortString(activationInfo, InputPortChildElementName);

            AZ::Entity* childEntity = nullptr;
            EBUS_EVENT_ID_RESULT(childEntity, entityId, UiElementBus, FindChildByName, childElementName.c_str());

            if (childEntity)
            {
                // Get the element ID from the entity ID
                LyShine::ElementId childElementId = 0;
                EBUS_EVENT_ID_RESULT(childElementId, childEntity->GetId(), UiElementBus, GetElementId);

                ActivateOutput(activationInfo, OutputPortChildElementId, childElementId);
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find child with name: %s\n", g_entElementGetChildByNameNodePath.c_str(), childElementName.c_str());
            }
        }
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntElementGetChildByNameNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortElementName,
        InputPortElementId,
        InputPortChildElementName
    };

    enum OutputPorts
    {
        OutputPortChildElementId = 0
    };
};

REGISTER_FLOW_NODE(g_elementGetChildAtIndexNodePath, CFlowUiElementGetChildAtIndexNode);
REGISTER_FLOW_NODE(g_elementGetIndexOfChildNodePath, CFlowUiElementGetIndexOfChildNode);
REGISTER_FLOW_NODE(g_elementGetParentNodePath, CFlowUiElementGetParentNode);

REGISTER_FLOW_NODE(g_entElementGetChildAtIndexNodePath, CFlowUiEntElementGetChildAtIndexNode);
REGISTER_FLOW_NODE(g_entElementGetIndexOfChildNodePath, CFlowUiEntElementGetIndexOfChildNode);
REGISTER_FLOW_NODE(g_entElementGetParentNodePath, CFlowUiEntElementGetParentNode);
REGISTER_FLOW_NODE(g_entElementGetChildByNameNodePath, CFlowUiEntElementGetChildByNameNode);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_NODE__GET_BOOL(Element, IsEnabled,
    "Get the enabled state of the element",
    "State", "The enabled state of element [ElementID]");

UI_FLOW_NODE__SET_BOOL(Element, SetIsEnabled,
    "Set the enabled state of the element",
    "State", "The enabled state of element [ElementID]");

UI_FLOW_NODE__GET_INT(Element, GetNumChildElements,
    "Get the number of child elements",
    "NumChildElements", "The number of child elements");
