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

#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// The UiFlow namespace contains helper functions for making the implementation of UI Flowgraph
// nodes less verbose. Most UI flow graph nodes end up taking a canvas ID and element ID as input
// and converting those to an entity ID and calling a function on a specific bus on that entity
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace UiFlow
{
    //! Helper function to get a canvas ID from a port, get the canvas entity and report a warning if it can't be found
    //! This requires that the canvas is specified by an entity rather than a canvas ID
    AZ::EntityId GetCanvasEntityIdFromEntityPort(IFlowNode::SActivationInfo* activationInfo, const string& nodeName);

    //! Helper function to get a canvas ID from a port, get the canvas entity and report a warning if it can't be found
    AZ::EntityId GetCanvasEntityId(IFlowNode::SActivationInfo* activationInfo, int canvasIdPort, const string& nodeName);

    //! Helper function to get a canvas ID and element ID ports, get the element entity and report a warning if it can't be found
    AZ::EntityId GetElementEntityId(IFlowNode::SActivationInfo* activationInfo,
        int canvasIdPort, int elementPortStart, bool hasOptionalElementIdPort, const string& nodeName, int& elementId, string& elementName, AZ::EntityId& outCanvasEntityId);

    //! Helper function to check if the given element supports the given Bus and report a warning if not
    template<typename Bus>
    bool CheckForComponent(AZ::EntityId elementEntityId, const string& nodeName, const char* componentName, int elementId, const string& elementName)
    {
        // Check for the component on the entity
        if (!Bus::FindFirstHandler(elementEntityId))
        {
            if (!elementName.empty())
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: couldn't find a %s component on UI element with name: %s\n",
                    nodeName.c_str(), componentName, elementName.c_str());
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: couldn't find a %s component on UI element with ID: %d\n",
                    nodeName.c_str(), componentName, elementId);
            }
            return false;
        }

        return true;
    }

    //! Helper function to get the element ID from the given ports and check that the element supports the given Bus
    //! Reports appropriate warnings if it does not
    template<typename Bus>
    AZ::EntityId GetElementEntityId(IFlowNode::SActivationInfo* activationInfo,
        int canvasIdPort, int elementPortStart, bool hasOptionalElementIdPort, const string& nodeName, const char* componentName,
        AZ::EntityId& outCanvasEntityId)
    {
        int elementId = 0;
        string elementName;
        AZ::EntityId elementEntityId = UiFlow::GetElementEntityId(activationInfo,
                canvasIdPort, elementPortStart, hasOptionalElementIdPort, nodeName, elementId, elementName, outCanvasEntityId);

        if (elementEntityId.IsValid())
        {
            if (UiFlow::CheckForComponent<Bus>(elementEntityId, nodeName, componentName, elementId, elementName))
            {
                return elementEntityId;
            }
        }

        return AZ::EntityId();     // return invalid entity ID
    }

    //! Helper function to get the element ID from the given ports and check that the element supports the given Bus
    //! Reports appropriate warnings if it does not
    template<typename Bus>
    AZ::EntityId IfActivatedGetElementEntityId(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo,
        int activatePort, int canvasIdPort, int elementPortStart, bool hasOptionalElementIdPort, const string& nodeName, const char* componentName,
        AZ::EntityId& outCanvasEntityId)
    {
        AZ::EntityId elementId;
        if (IFlowNode::eFE_Activate == event && IsPortActive(activationInfo, activatePort) && gEnv->pLyShine)
        {
            elementId = UiFlow::GetElementEntityId<Bus>(
                    activationInfo, canvasIdPort, elementPortStart, hasOptionalElementIdPort,
                    nodeName, componentName, outCanvasEntityId);
        }
        return elementId;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros that reduce some of the boilerplate code in UI flow graph node implementations
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for creating get/set UI nodes that take a canvas ID and element ID as input
////////////////////////////////////////////////////////////////////////////////////////////////////

// Create the class name from the component name and operation
// e.g. CFlowUiScrollBoxGetScrollOffset from ScrollBox and GetScrollOffset
#define UI_FLOW_NODE_CLASS_NAME(componentName, nodeName) CFlowUi##componentName##nodeName##Node

// Create the string for registering the node
// e.g. create "UI:ScrollBox:GetScrollOffset" from ScrollBox and GetScrollOffset
#define UI_FLOW_NODE_REGISTER_NAME(componentName, nodeName) "UI:" #componentName ":" #nodeName

// Wrapper for REGISTER_FLOW_NODE that builds the class name and registration name
#define UI_REGISTER_FLOW_NODE_HELPER(regName, className) REGISTER_FLOW_NODE(regName, className)
#define UI_REGISTER_FLOW_NODE(componentName, nodeName) \
    UI_REGISTER_FLOW_NODE_HELPER(UI_FLOW_NODE_CLASS_NAME(componentName, nodeName) ::s_nodePath, UI_FLOW_NODE_CLASS_NAME(componentName, nodeName));

// Macro for the input ports that most UI flow nodes share
#define UI_FLOW_ADD_STD_ELEMENT_INPUTS(componentName, activateStr)                       \
    InputPortConfig_Void("Activate", _HELP(activateStr)),                                \
    InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the element's canvas")), \
    InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the " componentName " element"))

// This macro does the boilerplate code for the start of a UI flow node class declaration
// It should always be followed by UI_FLOW_END_FLOW_NODE_CLASS with the exact same parameters
#define UI_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, nodeName)                             \
    class UI_FLOW_NODE_CLASS_NAME(componentName, nodeName)                                 \
        : public CFlowBaseNode < eNCT_Instanced >                                          \
    {                                                                                      \
    public:                                                                                \
        static const char* s_nodePath;                                                     \
                                                                                           \
        UI_FLOW_NODE_CLASS_NAME(componentName, nodeName)(SActivationInfo * activationInfo) \
        : CFlowBaseNode()                                                                  \
        , m_actInfo(*activationInfo)                                                       \
        {                                                                                  \
        }                                                                                  \
                                                                                           \
        ~UI_FLOW_NODE_CLASS_NAME(componentName, nodeName)()override                        \
        {                                                                                  \
        }                                                                                  \
                                                                                           \
        IFlowNodePtr Clone(SActivationInfo * activationInfo) override                      \
        {                                                                                  \
            return new UI_FLOW_NODE_CLASS_NAME(componentName, nodeName)(activationInfo);   \
        }                                                                                  \
                                                                                           \
        void GetMemoryUsage(ICrySizer * sizer) const override                              \
        {                                                                                  \
            sizer->Add(*this);                                                             \
        }

// This macro does the boilerplate code for the end of a UI flow node class declaration
// It ends the class and registers it
#define UI_FLOW_END_FLOW_NODE_CLASS(componentName, nodeName)                                                                         \
    SActivationInfo m_actInfo;                                                                                                       \
    };                                                                                                                               \
    const char* UI_FLOW_NODE_CLASS_NAME(componentName, nodeName) ::s_nodePath = UI_FLOW_NODE_REGISTER_NAME(componentName, nodeName); \
                                                                                                                                     \
    UI_REGISTER_FLOW_NODE(componentName, nodeName)

// Typical get nodes do not have any additional inputs other then the stc activate, canvasId, elementId
#define UI_FLOW_DECLARE_STD_GET_NODE_INPUTS(componentNameStr)                             \
    static const SInputPortConfig inputs[] =                                              \
    {                                                                                     \
        UI_FLOW_ADD_STD_ELEMENT_INPUTS(componentNameStr, "Trigger to update the output"), \
        {0}                                                                               \
    };

// Used for a Set node that typically has no outputs
#define UI_FLOW_SET_OUTPUTS                                                       \
    static const SOutputPortConfig outputs[] =                                    \
    {                                                                             \
        OutputPortConfig_Void("Done", _HELP("Fired after the node is finished")), \
        {0}                                                                       \
    };

// Macros to extract out the common 4 lines that every node has to set the config
#define UI_FLOW_SET_CONFIG(toolTipStr)       \
    config.pInputPorts = inputs;             \
    config.pOutputPorts = outputs;           \
    config.sDescription = _HELP(toolTipStr); \
    config.SetCategory(EFLN_APPROVED);

// Macro to wrap the code that gets the canvas and element entity IDs and checks the bus is supported
#define UI_FLOW_IF_ACTIVATED_AND_VALID(busname, componentNameStr)                                  \
    AZ::EntityId canvasId;                                                                         \
    AZ::EntityId elementId = UiFlow::IfActivatedGetElementEntityId<busname>(event, activationInfo, \
            0, 1, 2, false, s_nodePath, componentNameStr, canvasId);                               \
    if (elementId.IsValid())

// Macro that assumes that the bus name is a standard naming convention based of the component root name
// e.g. UI_FLOW_NODE_BUS_NAME(ScrollBox) -> UiScrollBoxBus
#define UI_FLOW_NODE_BUS_NAME(componentName) Ui##componentName##Bus

// The preamble for a *Get* UI flow node
#define UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName) \
    UI_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)  \
                                                                \
    void GetConfiguration(SFlowNodeConfig & config) override    \
    {                                                           \
        UI_FLOW_DECLARE_STD_GET_NODE_INPUTS( #componentName)    \
        static const SOutputPortConfig outputs[] =              \
        {
// The mid section for a *Get* UI flow node
#define UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                           \
    {0}                                                                                      \
    };                                                                                       \
    UI_FLOW_SET_CONFIG(nodeHelpText)                                                         \
    }                                                                                        \
                                                                                             \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override           \
    {                                                                                        \
        m_actInfo = *activationInfo;                                                         \
        UI_FLOW_IF_ACTIVATED_AND_VALID(UI_FLOW_NODE_BUS_NAME(componentName), #componentName) \
        {
// The end section for a *Get* UI flow node
#define UI_FLOW_NODE__GET_END_SECTION(componentName, functionName) \
    }                                                              \
    }                                                              \
                                                                   \
    UI_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

// The preamble for a *Set* UI flow node
#define UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName) \
    UI_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)  \
                                                                \
    void GetConfiguration(SFlowNodeConfig & config) override    \
    {                                                           \
        static const SInputPortConfig inputs[] =                \
        {                                                       \
            UI_FLOW_ADD_STD_ELEMENT_INPUTS( #componentName, "Trigger this node"),

// The mid section for a *Set* UI flow node
#define UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                           \
    {0}                                                                                      \
    };                                                                                       \
                                                                                             \
    UI_FLOW_SET_OUTPUTS                                                                      \
    UI_FLOW_SET_CONFIG(nodeHelpText)                                                         \
    }                                                                                        \
                                                                                             \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override           \
    {                                                                                        \
        UI_FLOW_IF_ACTIVATED_AND_VALID(UI_FLOW_NODE_BUS_NAME(componentName), #componentName) \
        {
// The end section for a *Set* UI flow node
#define UI_FLOW_NODE__SET_END_SECTION(componentName, functionName) \
    ActivateOutput(activationInfo, 0, 0);                          \
    }                                                              \
    }                                                              \
                                                                   \
    UI_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for creating get/set UI nodes that take a canvas ref entity and an element name as input
////////////////////////////////////////////////////////////////////////////////////////////////////

// Create the class name from the component name and operation
// e.g. CFlowUiEntScrollBoxGetScrollOffset from ScrollBox and GetScrollOffset
#define UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName) CFlowUiEnt##componentName##nodeName##Node

// Create the string for registering the node
// e.g. create "UIe:ScrollBox:GetScrollOffset" from ScrollBox and GetScrollOffset
#define UI_ENT_FLOW_NODE_REGISTER_NAME(componentName, nodeName) "UIe:" #componentName ":" #nodeName

// Wrapper for REGISTER_FLOW_NODE that builds the class name and registration name
#define UI_ENT_REGISTER_FLOW_NODE_HELPER(regName, className) REGISTER_FLOW_NODE(regName, className)
#define UI_ENT_REGISTER_FLOW_NODE(componentName, nodeName) \
    UI_ENT_REGISTER_FLOW_NODE_HELPER(UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName) ::s_nodePath, UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName));

// Macro for the input ports that most UI flow nodes share
#define UI_ENT_FLOW_ADD_STD_ELEMENT_INPUTS(componentName, activateStr)                                                        \
    InputPortConfig_Void("Activate", _HELP(activateStr)),                                                                     \
    InputPortConfig<string>("ElementName", _HELP("The name of the " componentName " element"), 0, _UICONFIG("dt=uiElement")), \
    InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the " componentName " element"))

// This macro does the boilerplate code for the start of a UI flow node class declaration
// It should always be followed by UI_ENT_FLOW_END_FLOW_NODE_CLASS with the exact same parameters
#define UI_ENT_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, nodeName)                             \
    class UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName)                                 \
        : public CFlowBaseNode < eNCT_Instanced >                                              \
    {                                                                                          \
    public:                                                                                    \
        static const char* s_nodePath;                                                         \
                                                                                               \
        UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName)(SActivationInfo * activationInfo) \
        : CFlowBaseNode()                                                                      \
        , m_actInfo(*activationInfo)                                                           \
        {                                                                                      \
        }                                                                                      \
                                                                                               \
        ~UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName)()override                        \
        {                                                                                      \
        }                                                                                      \
                                                                                               \
        IFlowNodePtr Clone(SActivationInfo * activationInfo) override                          \
        {                                                                                      \
            return new UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName)(activationInfo);   \
        }                                                                                      \
                                                                                               \
        void GetMemoryUsage(ICrySizer * sizer) const override                                  \
        {                                                                                      \
            sizer->Add(*this);                                                                 \
        }

// This macro does the boilerplate code for the end of a UI flow node class declaration
// It ends the class and registers it
#define UI_ENT_FLOW_END_FLOW_NODE_CLASS(componentName, nodeName)                                                                             \
    SActivationInfo m_actInfo;                                                                                                               \
    };                                                                                                                                       \
    const char* UI_ENT_FLOW_NODE_CLASS_NAME(componentName, nodeName) ::s_nodePath = UI_ENT_FLOW_NODE_REGISTER_NAME(componentName, nodeName); \
                                                                                                                                             \
    UI_ENT_REGISTER_FLOW_NODE(componentName, nodeName)

// Typical get nodes do not have any additional inputs other then the stc activate, elementName
#define UI_ENT_FLOW_DECLARE_STD_GET_NODE_INPUTS(componentNameStr)                             \
    static const SInputPortConfig inputs[] =                                                  \
    {                                                                                         \
        UI_ENT_FLOW_ADD_STD_ELEMENT_INPUTS(componentNameStr, "Trigger to update the output"), \
        {0}                                                                                   \
    };

// Macros to extract out the common 5 lines that every node has to set the config
#define UI_ENT_FLOW_SET_CONFIG(toolTipStr) \
    UI_FLOW_SET_CONFIG(toolTipStr)         \
    config.nFlags |= EFLN_TARGET_ENTITY;

// Macro to wrap the code that gets the canvas and element entity IDs and checks the bus is supported
#define UI_ENT_FLOW_IF_ACTIVATED_AND_VALID(busname, componentNameStr)                              \
    AZ::EntityId canvasId;                                                                         \
    AZ::EntityId elementId = UiFlow::IfActivatedGetElementEntityId<busname>(event, activationInfo, \
            0, -1, 1, true, s_nodePath, componentNameStr, canvasId);                               \
    if (elementId.IsValid())

// The preamble for a *Get* UI flow node
#define UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName) \
    UI_ENT_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)  \
                                                                    \
    void GetConfiguration(SFlowNodeConfig & config) override        \
    {                                                               \
        UI_ENT_FLOW_DECLARE_STD_GET_NODE_INPUTS( #componentName)    \
        static const SOutputPortConfig outputs[] =                  \
        {
// The mid section for a *Get* UI flow node
#define UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                           \
    {0}                                                                                          \
    };                                                                                           \
    UI_ENT_FLOW_SET_CONFIG(nodeHelpText)                                                         \
    }                                                                                            \
                                                                                                 \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override               \
    {                                                                                            \
        m_actInfo = *activationInfo;                                                             \
        UI_ENT_FLOW_IF_ACTIVATED_AND_VALID(UI_FLOW_NODE_BUS_NAME(componentName), #componentName) \
        {
// The end section for a *Get* UI flow node
#define UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName) \
    }                                                                  \
    }                                                                  \
                                                                       \
    UI_ENT_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)


// The preamble for a *Set* UI flow node
#define UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName) \
    UI_ENT_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)  \
                                                                    \
    void GetConfiguration(SFlowNodeConfig & config) override        \
    {                                                               \
        static const SInputPortConfig inputs[] =                    \
        {                                                           \
            UI_ENT_FLOW_ADD_STD_ELEMENT_INPUTS( #componentName, "Trigger this node"),

// The mid section for a *Set* UI flow node
#define UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                           \
    {0}                                                                                          \
    };                                                                                           \
                                                                                                 \
    UI_FLOW_SET_OUTPUTS                                                                          \
    UI_ENT_FLOW_SET_CONFIG(nodeHelpText)                                                         \
    }                                                                                            \
                                                                                                 \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override               \
    {                                                                                            \
        UI_ENT_FLOW_IF_ACTIVATED_AND_VALID(UI_FLOW_NODE_BUS_NAME(componentName), #componentName) \
        {
// The end section for a *Set* UI flow node
#define UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName) \
    ActivateOutput(activationInfo, 0, 0);                              \
    }                                                                  \
    }                                                                  \
                                                                       \
    UI_ENT_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for adding simple input/output ports
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_OUTPUT(outputType, outputName, outputHelpText) \
    OutputPortConfig<outputType>(outputName, _HELP(outputHelpText)),

#define UI_FLOW_NODE__SET_INPUT(inputType, inputName, inputHelpText) \
    InputPortConfig<inputType>(inputName, _HELP(inputHelpText)),

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for processing simple get/set events
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_ACTION(resultType, componentName, functionName)                        \
    resultType result;                                                                           \
    EBUS_EVENT_ID_RESULT(result, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, result);

#define UI_FLOW_NODE__SET_ACTION(inputType, portType, portIndex, componentName, functionName) \
    inputType typeInput = GetPort##portType(activationInfo, portIndex);                       \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, typeInput);

////////////////////////////////////////////////////////////////////////////////////////////////////
// bool get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

// Define a complete class for a flow node that uses a get function on a bus to get a bool
#define UI_FLOW_NODE__GET_BOOL(componentName, functionName, nodeHelpText, \
                               output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)               \
    UI_FLOW_NODE__GET_OUTPUT(bool, output0Name, output0HelpText)          \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)            \
    UI_FLOW_NODE__GET_ACTION(bool, componentName, functionName)           \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)            \
                                                                          \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)           \
    UI_FLOW_NODE__GET_OUTPUT(bool, output0Name, output0HelpText)          \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)        \
    UI_FLOW_NODE__GET_ACTION(bool, componentName, functionName)           \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a bool
#define UI_FLOW_NODE__SET_BOOL(componentName, functionName, nodeHelpText, \
                               input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)               \
    UI_FLOW_NODE__SET_INPUT(bool, input1Name, input1HelpText)             \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)            \
    UI_FLOW_NODE__SET_ACTION(bool, Bool, 3, componentName, functionName)  \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)            \
                                                                          \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)           \
    UI_FLOW_NODE__SET_INPUT(bool, input1Name, input1HelpText)             \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)        \
    UI_FLOW_NODE__SET_ACTION(bool, Bool, 3, componentName, functionName)  \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a bool value using a bus
#define UI_FLOW_NODE__GET_AND_SET_BOOL(componentName, functionNameWithoutGetOrSet, \
                                       getNodeHelpText, setNodeHelpText,           \
                                       inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_BOOL(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                               \
    inOut0Name, inOut0HelpText);                                                   \
                                                                                   \
    UI_FLOW_NODE__SET_BOOL(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                               \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// int get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

// Define a complete class for a flow node that uses a get function on a bus to get a int
#define UI_FLOW_NODE__GET_INT(componentName, functionName, nodeHelpText, \
                              output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)              \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)          \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)           \
    UI_FLOW_NODE__GET_ACTION(int, componentName, functionName)           \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)           \
                                                                         \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)          \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)          \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)       \
    UI_FLOW_NODE__GET_ACTION(int, componentName, functionName)           \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a int
#define UI_FLOW_NODE__SET_INT(componentName, functionName, nodeHelpText, \
                              input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)              \
    UI_FLOW_NODE__SET_INPUT(int, input1Name, input1HelpText)             \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)           \
    UI_FLOW_NODE__SET_ACTION(int, Int, 3, componentName, functionName)   \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)           \
                                                                         \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)          \
    UI_FLOW_NODE__SET_INPUT(int, input1Name, input1HelpText)             \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)       \
    UI_FLOW_NODE__SET_ACTION(int, Int, 3, componentName, functionName)   \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a int value using a bus
#define UI_FLOW_NODE__GET_AND_SET_INT(componentName, functionNameWithoutGetOrSet, \
                                      getNodeHelpText, setNodeHelpText,           \
                                      inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_INT(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                              \
    inOut0Name, inOut0HelpText);                                                  \
                                                                                  \
    UI_FLOW_NODE__SET_INT(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                              \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// float get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

// Define a complete class for a flow node that uses a get function on a bus to get a float
#define UI_FLOW_NODE__GET_FLOAT(componentName, functionName, nodeHelpText, \
                                output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__GET_OUTPUT(float, output0Name, output0HelpText)          \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__GET_ACTION(float, componentName, functionName)           \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)             \
                                                                           \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)            \
    UI_FLOW_NODE__GET_OUTPUT(float, output0Name, output0HelpText)          \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)         \
    UI_FLOW_NODE__GET_ACTION(float, componentName, functionName)           \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a float
#define UI_FLOW_NODE__SET_FLOAT(componentName, functionName, nodeHelpText, \
                                input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__SET_INPUT(float, input1Name, input1HelpText)             \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__SET_ACTION(float, Float, 3, componentName, functionName) \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)             \
                                                                           \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)            \
    UI_FLOW_NODE__SET_INPUT(float, input1Name, input1HelpText)             \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)         \
    UI_FLOW_NODE__SET_ACTION(float, Float, 3, componentName, functionName) \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a float value using a bus
#define UI_FLOW_NODE__GET_AND_SET_FLOAT(componentName, functionNameWithoutGetOrSet, \
                                        getNodeHelpText, setNodeHelpText,           \
                                        inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_FLOAT(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                \
    inOut0Name, inOut0HelpText);                                                    \
                                                                                    \
    UI_FLOW_NODE__SET_FLOAT(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// enum as int get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_ENUM_ACTION(resultType, componentName, functionName)                   \
    resultType result;                                                                           \
    EBUS_EVENT_ID_RESULT(result, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, static_cast<int>(result));

#define UI_FLOW_NODE__SET_ENUM_INPUT(inputName, inputHelpText) \
    InputPortConfig<int>(inputName, _HELP(inputHelpText), 0, _UICONFIG("enum_int:" inputHelpText))

#define UI_FLOW_NODE__SET_ENUM_ACTION(typeName, portIndex, componentName, functionName) \
    int intInput = GetPortInt(activationInfo, portIndex);                               \
    typeName enumInput = static_cast<typeName>(intInput);                               \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, enumInput);

// Define a complete class for a flow node that uses a get function on a bus to get a enum value as in int
#define UI_FLOW_NODE__GET_ENUM_AS_INT(componentName, functionName, typeName, nodeHelpText, \
                                      output0Name, output0HelpText)                        \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)                            \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                             \
    UI_FLOW_NODE__GET_ENUM_ACTION(typeName, componentName, functionName)                   \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                             \
                                                                                           \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                            \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)                            \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                         \
    UI_FLOW_NODE__GET_ENUM_ACTION(typeName, componentName, functionName)                   \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a enum as an int
#define UI_FLOW_NODE__SET_ENUM_AS_INT(componentName, functionName, typeName, nodeHelpText, \
                                      input1Name, input1HelpText)                          \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                \
    UI_FLOW_NODE__SET_ENUM_INPUT(input1Name, input1HelpText),                              \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                             \
    UI_FLOW_NODE__SET_ENUM_ACTION(typeName, 3, componentName, functionName)                \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                             \
                                                                                           \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                            \
    UI_FLOW_NODE__SET_ENUM_INPUT(input1Name, input1HelpText),                              \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                         \
    UI_FLOW_NODE__SET_ENUM_ACTION(typeName, 3, componentName, functionName)                \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a enum value using a bus
#define UI_FLOW_NODE__GET_AND_SET_ENUM_AS_INT(componentName, functionNameWithoutGetOrSet, typeName, \
                                              getNodeHelpText, setNodeHelpText,                     \
                                              inOut0Name, inOut0HelpText)                           \
    UI_FLOW_NODE__GET_ENUM_AS_INT(componentName, Get##functionNameWithoutGetOrSet, typeName,        \
    getNodeHelpText,                                                                                \
    inOut0Name, inOut0HelpText);                                                                    \
                                                                                                    \
    UI_FLOW_NODE__SET_ENUM_AS_INT(componentName, Set##functionNameWithoutGetOrSet, typeName,        \
    setNodeHelpText,                                                                                \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// string get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_AZ_STRING_ACTION(componentName, functionName)                          \
    AZStd::string result;                                                                        \
    EBUS_EVENT_ID_RESULT(result, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    string fgResult(result.c_str());                                                             \
    ActivateOutput(&m_actInfo, 0, fgResult);

#define UI_FLOW_NODE__SET_AZ_STRING_ACTION(portIndex, componentName, functionName) \
    string stringInput = GetPortString(activationInfo, portIndex);                 \
    AZStd::string azStringInput(stringInput.c_str());                              \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, azStringInput);

// Define a complete class for a flow node that uses a get function on a bus to get a string
#define UI_FLOW_NODE__GET_AZ_STRING(componentName, functionName, nodeHelpText, \
                                    output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                    \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)             \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                 \
    UI_FLOW_NODE__GET_AZ_STRING_ACTION(componentName, functionName)            \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                 \
                                                                               \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)             \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__GET_AZ_STRING_ACTION(componentName, functionName)            \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a string
#define UI_FLOW_NODE__SET_AZ_STRING(componentName, functionName, nodeHelpText, \
                                    input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                    \
    UI_FLOW_NODE__SET_INPUT(string, input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                 \
    UI_FLOW_NODE__SET_AZ_STRING_ACTION(3, componentName, functionName)         \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                 \
                                                                               \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__SET_INPUT(string, input1Name, input1HelpText)                \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__SET_AZ_STRING_ACTION(3, componentName, functionName)         \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a string value using a bus
#define UI_FLOW_NODE__GET_AND_SET_AZ_STRING(componentName, functionNameWithoutGetOrSet, \
                                            getNodeHelpText, setNodeHelpText,           \
                                            inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_AZ_STRING(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                    \
    inOut0Name, inOut0HelpText);                                                        \
                                                                                        \
    UI_FLOW_NODE__SET_AZ_STRING(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                    \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// LyShine string get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_LYSHINE_STRING_ACTION(componentName, functionName)                     \
    LyShine::StringType result;                                                                  \
    EBUS_EVENT_ID_RESULT(result, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, result);

#define UI_FLOW_NODE__SET_LYSHINE_STRING_ACTION(portIndex, componentName, functionName) \
    LyShine::StringType stringInput = GetPortString(activationInfo, portIndex);         \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, stringInput);

// Define a complete class for a flow node that uses a get function on a bus to get a LyShine string
#define UI_FLOW_NODE__GET_LYSHINE_STRING(componentName, functionName, nodeHelpText, \
                                         output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                         \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                  \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                      \
    UI_FLOW_NODE__GET_LYSHINE_STRING_ACTION(componentName, functionName)            \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                      \
                                                                                    \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                     \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                  \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                  \
    UI_FLOW_NODE__GET_LYSHINE_STRING_ACTION(componentName, functionName)            \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a LyShine string
#define UI_FLOW_NODE__SET_LYSHINE_STRING(componentName, functionName, nodeHelpText, \
                                         input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                         \
    UI_FLOW_NODE__SET_INPUT(string, input1Name, input1HelpText)                     \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                      \
    UI_FLOW_NODE__SET_LYSHINE_STRING_ACTION(3, componentName, functionName)         \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                      \
                                                                                    \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                     \
    UI_FLOW_NODE__SET_INPUT(string, input1Name, input1HelpText)                     \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                  \
    UI_FLOW_NODE__SET_LYSHINE_STRING_ACTION(3, componentName, functionName)         \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a LyShine string value using a bus
#define UI_FLOW_NODE__GET_AND_SET_LYSHINE_STRING(componentName, functionNameWithoutGetOrSet, \
                                                 getNodeHelpText, setNodeHelpText,           \
                                                 inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_LYSHINE_STRING(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                         \
    inOut0Name, inOut0HelpText);                                                             \
                                                                                             \
    UI_FLOW_NODE__SET_LYSHINE_STRING(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                         \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// LyShine path get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__SET_LYSHINE_PATH_INPUT(inputName, inputHelpText) \
    InputPortConfig<string>(inputName, _HELP(inputHelpText), 0, _UICONFIG("dt=file")),

// Define a complete class for a flow node that uses a get function on a bus to get a LyShine string
#define UI_FLOW_NODE__GET_LYSHINE_PATH(componentName, functionName, nodeHelpText, \
                                       output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                       \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                    \
    UI_FLOW_NODE__GET_AZ_STRING_ACTION(componentName, functionName)               \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                    \
                                                                                  \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                   \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                \
    UI_FLOW_NODE__GET_AZ_STRING_ACTION(componentName, functionName)               \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a LyShine string
#define UI_FLOW_NODE__SET_LYSHINE_PATH(componentName, functionName, nodeHelpText, \
                                       input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                       \
    UI_FLOW_NODE__SET_LYSHINE_PATH_INPUT(input1Name, input1HelpText)              \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                    \
    UI_FLOW_NODE__SET_AZ_STRING_ACTION(3, componentName, functionName)            \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                    \
                                                                                  \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                   \
    UI_FLOW_NODE__SET_LYSHINE_PATH_INPUT(input1Name, input1HelpText)              \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                \
    UI_FLOW_NODE__SET_AZ_STRING_ACTION(3, componentName, functionName)            \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a LyShine path value using a bus
#define UI_FLOW_NODE__GET_AND_SET_LYSHINE_PATH(componentName, functionNameWithoutGetOrSet, \
                                               getNodeHelpText, setNodeHelpText,           \
                                               inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_LYSHINE_PATH(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                       \
    inOut0Name, inOut0HelpText);                                                           \
                                                                                           \
    UI_FLOW_NODE__SET_LYSHINE_PATH(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                       \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Entity ID as Element Id get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////


// Define a complete class for a flow node that uses a get function on a bus to get an element Id
#define UI_FLOW_NODE__GET_ENTITY_ID_AS_ELEMENT_ID(componentName, functionName, nodeHelpText,       \
                                                  output0Name, output0HelpText)                    \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                        \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)                                    \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                     \
    AZ::EntityId entityId;                                                                         \
    EBUS_EVENT_ID_RESULT(entityId, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    LyShine::ElementId elementIdResult = 0;                                                        \
    EBUS_EVENT_ID_RESULT(elementIdResult, entityId, UiElementBus, GetElementId);                   \
    ActivateOutput(&m_actInfo, 0, elementIdResult);                                                \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                     \
                                                                                                   \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                    \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                                 \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                 \
    AZ::EntityId entityId;                                                                         \
    EBUS_EVENT_ID_RESULT(entityId, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    LyShine::NameType entityName;                                                                  \
    EBUS_EVENT_ID_RESULT(entityName, entityId, UiElementBus, GetName);                             \
    string entityNameOut = entityName.c_str();                                                     \
    ActivateOutput(&m_actInfo, 0, entityNameOut);                                                  \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set an element Id
#define UI_FLOW_NODE__SET_ENTITY_ID_AS_ELEMENT_ID(componentName, functionName, nodeHelpText,                 \
                                                  input1Name, input1HelpText)                                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                                  \
    UI_FLOW_NODE__SET_INPUT(int, input1Name, input1HelpText)                                                 \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                                               \
    int elementIdInput = GetPortInt(activationInfo, 3);                                                      \
    AZ::Entity* elementInput = nullptr;                                                                      \
    EBUS_EVENT_ID_RESULT(elementInput, canvasId, UiCanvasBus, FindElementById, elementIdInput);              \
    if (elementInput)                                                                                        \
    {                                                                                                        \
        EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, elementInput->GetId()); \
    }                                                                                                        \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                                               \
                                                                                                             \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                              \
    InputPortConfig<string>(input1Name, _HELP(input1HelpText), 0, _UICONFIG("dt=uiElement")),                \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                                           \
    string elementName = GetPortString(activationInfo, 3);                                                   \
    AZ::Entity* elementInput = nullptr;                                                                      \
    EBUS_EVENT_ID_RESULT(elementInput, canvasId, UiCanvasBus, FindElementByName, elementName.c_str());       \
    if (elementInput)                                                                                        \
    {                                                                                                        \
        EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, elementInput->GetId()); \
    }                                                                                                        \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set an element Id using a bus
#define UI_FLOW_NODE__GET_AND_SET_ENTITY_ID_AS_ELEMENT_ID(componentName, functionNameWithoutGetOrSet, \
                                                          getNodeHelpText, setNodeHelpText,           \
                                                          inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_ENTITY_ID_AS_ELEMENT_ID(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                                  \
    inOut0Name, inOut0HelpText);                                                                      \
                                                                                                      \
    UI_FLOW_NODE__SET_ENTITY_ID_AS_ELEMENT_ID(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                                  \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Vec2 as Float get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_VEC2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    OutputPortConfig<float>(output0Name, _HELP(output0HelpText)),                                 \
    OutputPortConfig<float>(output1Name, _HELP(output1HelpText)),

#define UI_FLOW_NODE__GET_VEC2_ACTION(componentName, functionName)                            \
    Vec2 vec;                                                                                 \
    EBUS_EVENT_ID_RESULT(vec, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, vec.x);                                                     \
    ActivateOutput(&m_actInfo, 1, vec.y);

#define UI_FLOW_NODE__SET_VEC2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    InputPortConfig<float>(input1Name, _HELP(input1HelpText)),                               \
    InputPortConfig<float>(input2Name, _HELP(input2HelpText)),

#define UI_FLOW_NODE__SET_VEC2_ACTION(portIndex, componentName, functionName) \
    float x = GetPortFloat(activationInfo, portIndex);                        \
    float y = GetPortFloat(activationInfo, portIndex + 1);                    \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, Vec2(x, y));

// Define a complete class for a flow node that uses a get function on a bus to get a Vec2
// and expose the result as two floats
#define UI_FLOW_NODE__GET_VEC2_AS_FLOATS(componentName, functionName, nodeHelpText,           \
                                         output0Name, output0HelpText,                        \
                                         output1Name, output1HelpText)                        \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                   \
    UI_FLOW_NODE__GET_VEC2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                \
    UI_FLOW_NODE__GET_VEC2_ACTION(componentName, functionName)                                \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                \
                                                                                              \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                               \
    UI_FLOW_NODE__GET_VEC2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                            \
    UI_FLOW_NODE__GET_VEC2_ACTION(componentName, functionName)                                \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a Vec2
// and expose the inputs as two floats
#define UI_FLOW_NODE__SET_VEC2_AS_FLOATS(componentName, functionName, nodeHelpText,      \
                                         input1Name, input1HelpText,                     \
                                         input2Name, input2HelpText)                     \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                              \
    UI_FLOW_NODE__SET_VEC2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                           \
    UI_FLOW_NODE__SET_VEC2_ACTION(3, componentName, functionName)                        \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                           \
                                                                                         \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                          \
    UI_FLOW_NODE__SET_VEC2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                       \
    UI_FLOW_NODE__SET_VEC2_ACTION(3, componentName, functionName)                        \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is a Vec2 represented as 2 floats in flow graph
#define UI_FLOW_NODE__GET_AND_SET_VEC2_AS_FLOATS(componentName, functionNameWithoutGetOrSet, \
                                                 getNodeHelpText, setNodeHelpText,           \
                                                 inOut0Name, inOut0HelpText,                 \
                                                 inOut1Name, inOut1HelpText)                 \
    UI_FLOW_NODE__GET_VEC2_AS_FLOATS(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                         \
    inOut0Name, inOut0HelpText,                                                              \
    inOut1Name, inOut1HelpText);                                                             \
                                                                                             \
    UI_FLOW_NODE__SET_VEC2_AS_FLOATS(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                         \
    inOut0Name, inOut0HelpText,                                                              \
    inOut1Name, inOut1HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// ColorF as Vec3 and Float get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_COLOR_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    OutputPortConfig<Vec3>(output0Name, _HELP(output0HelpText)),                                   \
    OutputPortConfig<float>(output1Name, _HELP(output1HelpText)),

#define UI_FLOW_NODE__GET_COLOR_ACTION(componentName, functionName)                             \
    ColorF color;                                                                               \
    EBUS_EVENT_ID_RESULT(color, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    Vec3 colorVec = color.toVec3();                                                             \
    colorVec *= 255.0f;                                                                         \
    float alpha = color.a * 255.0f;                                                             \
    ActivateOutput(&m_actInfo, 0, colorVec);                                                    \
    ActivateOutput(&m_actInfo, 1, alpha);

#define UI_FLOW_NODE__SET_COLOR_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    InputPortConfig<Vec3>(input1Name, _HELP(input1HelpText)),                                 \
    InputPortConfig<float>(input2Name, 255, _HELP(input2HelpText), 0, _UICONFIG("v_min=0, v_max=255")),

#define UI_FLOW_NODE__SET_COLOR_ACTION(portIndex, componentName, functionName) \
    Vec3 vec = GetPortVec3(activationInfo, portIndex) / 255.0f;                \
    float a = GetPortFloat(activationInfo, portIndex + 1) / 255.0f;            \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, ColorF(vec, a));

// Define a complete class for a flow node that uses a get function on a bus to get a ColorF
// and expose the result as a Vec3 and a float
#define UI_FLOW_NODE__GET_COLORF_AS_VEC3_AND_FLOAT(componentName, functionName, nodeHelpText,  \
                                                   output0Name, output0HelpText,               \
                                                   output1Name, output1HelpText)               \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                    \
    UI_FLOW_NODE__GET_COLOR_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                 \
    UI_FLOW_NODE__GET_COLOR_ACTION(componentName, functionName)                                \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                 \
                                                                                               \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                \
    UI_FLOW_NODE__GET_COLOR_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                             \
    UI_FLOW_NODE__GET_COLOR_ACTION(componentName, functionName)                                \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a ColorF
// and expose the inputs as a Vec3 and a float
#define UI_FLOW_NODE__SET_COLORF_AS_VEC3_AND_FLOAT(componentName, functionName, nodeHelpText, \
                                                   input1Name, input1HelpText,                \
                                                   input2Name, input2HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                   \
    UI_FLOW_NODE__SET_COLOR_INPUT(input1Name, input1HelpText, input2Name, input2HelpText)     \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                                \
    UI_FLOW_NODE__SET_COLOR_ACTION(3, componentName, functionName)                            \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                                \
                                                                                              \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                               \
    UI_FLOW_NODE__SET_COLOR_INPUT(input1Name, input1HelpText, input2Name, input2HelpText)     \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                            \
    UI_FLOW_NODE__SET_COLOR_ACTION(3, componentName, functionName)                            \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is a ColorF represented as a Vec3 and a float in flow graph
#define UI_FLOW_NODE__GET_AND_SET_COLORF_AS_VEC3_AND_FLOAT(componentName, functionNameWithoutGetOrSet, \
                                                           getNodeHelpText, setNodeHelpText,           \
                                                           inOut0Name, inOut0HelpText,                 \
                                                           inOut1Name, inOut1HelpText)                 \
    UI_FLOW_NODE__GET_COLORF_AS_VEC3_AND_FLOAT(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                                   \
    inOut0Name, inOut0HelpText,                                                                        \
    inOut1Name, inOut1HelpText);                                                                       \
                                                                                                       \
    UI_FLOW_NODE__SET_COLORF_AS_VEC3_AND_FLOAT(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                                   \
    inOut0Name, inOut0HelpText,                                                                        \
    inOut1Name, inOut1HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// AZ::Color as Vec3 and Float get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_AZ_COLOR_ACTION(componentName, functionName)                          \
    AZ::Color color;                                                                            \
    EBUS_EVENT_ID_RESULT(color, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    Vec3 colorVec(color.GetR(), color.GetG(), color.GetB());                                    \
    colorVec *= 255.0f;                                                                         \
    float alpha = color.GetA() * 255.0f;                                                        \
    ActivateOutput(&m_actInfo, 0, colorVec);                                                    \
    ActivateOutput(&m_actInfo, 1, alpha);

#define UI_FLOW_NODE__SET_AZ_COLOR_ACTION(portIndex, componentName, functionName) \
    Vec3 vec = GetPortVec3(activationInfo, portIndex) / 255.0f;                   \
    float a = GetPortFloat(activationInfo, portIndex + 1) / 255.0f;               \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, AZ::Color(vec.x, vec.y, vec.z, a));

// Define a complete class for a flow node that uses a get function on a bus to get a AZ::Color
// and expose the result as a Vec3 and a float
#define UI_FLOW_NODE__GET_COLOR_AS_VEC3_AND_FLOAT(componentName, functionName, nodeHelpText,   \
                                                  output0Name, output0HelpText,                \
                                                  output1Name, output1HelpText)                \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                    \
    UI_FLOW_NODE__GET_COLOR_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                 \
    UI_FLOW_NODE__GET_AZ_COLOR_ACTION(componentName, functionName)                             \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                 \
                                                                                               \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                \
    UI_FLOW_NODE__GET_COLOR_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                             \
    UI_FLOW_NODE__GET_AZ_COLOR_ACTION(componentName, functionName)                             \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a AZ::Color
// and expose the inputs as a Vec3 and a float
#define UI_FLOW_NODE__SET_COLOR_AS_VEC3_AND_FLOAT(componentName, functionName, nodeHelpText, \
                                                  input1Name, input1HelpText,                \
                                                  input2Name, input2HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                  \
    UI_FLOW_NODE__SET_COLOR_INPUT(input1Name, input1HelpText, input2Name, input2HelpText)    \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                               \
    UI_FLOW_NODE__SET_AZ_COLOR_ACTION(3, componentName, functionName)                        \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                               \
                                                                                             \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                              \
    UI_FLOW_NODE__SET_COLOR_INPUT(input1Name, input1HelpText, input2Name, input2HelpText)    \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                           \
    UI_FLOW_NODE__SET_AZ_COLOR_ACTION(3, componentName, functionName)                        \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is a AZ::Color represented as a Vec3 and a float in flow graph
#define UI_FLOW_NODE__GET_AND_SET_COLOR_AS_VEC3_AND_FLOAT(componentName, functionNameWithoutGetOrSet, \
                                                          getNodeHelpText, setNodeHelpText,           \
                                                          inOut0Name, inOut0HelpText,                 \
                                                          inOut1Name, inOut1HelpText)                 \
    UI_FLOW_NODE__GET_COLOR_AS_VEC3_AND_FLOAT(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                                  \
    inOut0Name, inOut0HelpText,                                                                       \
    inOut1Name, inOut1HelpText);                                                                      \
                                                                                                      \
    UI_FLOW_NODE__SET_COLOR_AS_VEC3_AND_FLOAT(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                                  \
    inOut0Name, inOut0HelpText,                                                                       \
    inOut1Name, inOut1HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Padding as Int get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_PADDING_OUTPUT                                                       \
    OutputPortConfig<int>("Left", _HELP("The padding inside the left edge of the element")),   \
    OutputPortConfig<int>("Right", _HELP("The padding inside the right edge of the element")), \
    OutputPortConfig<int>("Top", _HELP("The padding inside the top edge of the element")),     \
    OutputPortConfig<int>("Bottom", _HELP("The padding inside the bottom edge of the element")),

#define UI_FLOW_NODE__GET_PADDING_ACTION(componentName, functionName)                             \
    UiLayoutInterface::Padding padding;                                                           \
    EBUS_EVENT_ID_RESULT(padding, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, padding.m_left);                                                \
    ActivateOutput(&m_actInfo, 1, padding.m_right);                                               \
    ActivateOutput(&m_actInfo, 2, padding.m_top);                                                 \
    ActivateOutput(&m_actInfo, 3, padding.m_bottom);

#define UI_FLOW_NODE__SET_PADDING_INPUT                                                       \
    InputPortConfig<int>("Left", _HELP("The padding inside the left edge of the element")),   \
    InputPortConfig<int>("Right", _HELP("The padding inside the right edge of the element")), \
    InputPortConfig<int>("Top", _HELP("The padding inside the top edge of the element")),     \
    InputPortConfig<int>("Bottom", _HELP("The padding inside the bottom edge of the element")),

#define UI_FLOW_NODE__SET_PADDING_ACTION(portIndex, componentName, functionName) \
    UiLayoutInterface::Padding padding;                                          \
    padding.m_left = GetPortInt(activationInfo, portIndex);                      \
    padding.m_right = GetPortInt(activationInfo, portIndex + 1);                 \
    padding.m_top = GetPortInt(activationInfo, portIndex + 2);                   \
    padding.m_bottom = GetPortInt(activationInfo, portIndex + 3);                \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, padding);

// Define a complete class for a flow node that uses a get function on a bus to get Padding
// and expose the result as four ints
#define UI_FLOW_NODE__GET_PADDING_AS_INTS(componentName, functionName)                                              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                                         \
    UI_FLOW_NODE__GET_PADDING_OUTPUT                                                                                \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, "Get the padding (in pixels) inside the edges of the element")     \
    UI_FLOW_NODE__GET_PADDING_ACTION(componentName, functionName)                                                   \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                                      \
                                                                                                                    \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                                     \
    UI_FLOW_NODE__GET_PADDING_OUTPUT                                                                                \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, "Get the padding (in pixels) inside the edges of the element") \
    UI_FLOW_NODE__GET_PADDING_ACTION(componentName, functionName)                                                   \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set Padding
// and expose the inputs as four ints
#define UI_FLOW_NODE__SET_PADDING_AS_INTS(componentName, functionName)                                              \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                                         \
    UI_FLOW_NODE__SET_PADDING_INPUT                                                                                 \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, "Set the padding (in pixels) inside the edges of the element")     \
    UI_FLOW_NODE__SET_PADDING_ACTION(3, componentName, functionName)                                                \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                                                      \
                                                                                                                    \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                                     \
    UI_FLOW_NODE__SET_PADDING_INPUT                                                                                 \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, "Set the padding (in pixels) inside the edges of the element") \
    UI_FLOW_NODE__SET_PADDING_ACTION(3, componentName, functionName)                                                \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is Padding represented as 4 ints in flow graph
#define UI_FLOW_NODE__GET_AND_SET_PADDING_AS_INTS(componentName, functionNameWithoutGetOrSet); \
    UI_FLOW_NODE__GET_PADDING_AS_INTS(componentName, Get##functionNameWithoutGetOrSet);        \
                                                                                               \
    UI_FLOW_NODE__SET_PADDING_AS_INTS(componentName, Set##functionNameWithoutGetOrSet);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_ALIGNMENT_OUTPUT                                     \
    OutputPortConfig<int>("HorizAlignment", _HELP("Left=0,Center=1,Right=2")), \
    OutputPortConfig<int>("VertAlignment", _HELP("Top=0,Center=1,Bottom=2")),

#define UI_FLOW_NODE__GET_ALIGNMENT_ACTION(componentName, functionName)                                          \
    IDraw2d::HAlign horizAlignment;                                                                              \
    IDraw2d::VAlign vertAlignment;                                                                               \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, horizAlignment, vertAlignment); \
    ActivateOutput(&m_actInfo, 0, static_cast<int>(horizAlignment));                                             \
    ActivateOutput(&m_actInfo, 1, static_cast<int>(vertAlignment));

#define UI_FLOW_NODE__SET_ALIGNMENT_INPUT                                                                                                            \
    InputPortConfig<int>("HorizAlignment", _HELP("The horizontal text alignment of the element"), 0, _UICONFIG("enum_int:Left=0,Center=1,Right=2")), \
    InputPortConfig<int>("VertAlignment", _HELP("The vertical text alignment of the element"), 0, _UICONFIG("enum_int:Top=0,Center=1,Bottom=2")),

#define UI_FLOW_NODE__SET_ALIGNMENT_ACTION(portIndex, componentName, functionName)      \
    int horizAlignmentIndex = GetPortInt(activationInfo, portIndex);                    \
    IDraw2d::HAlign horizAlignment = static_cast<IDraw2d::HAlign>(horizAlignmentIndex); \
    int vertAlignmentIndex = GetPortInt(activationInfo, portIndex + 1);                 \
    IDraw2d::VAlign vertAlignment = static_cast<IDraw2d::VAlign>(vertAlignmentIndex);   \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, horizAlignment, vertAlignment);

// Define a complete class for a flow node that uses a get function on a bus to get alignment
#define UI_FLOW_NODE__GET_ALIGNMENT(componentName, functionName, nodeHelpText) \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                    \
    UI_FLOW_NODE__GET_ALIGNMENT_OUTPUT                                         \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                 \
    UI_FLOW_NODE__GET_ALIGNMENT_ACTION(componentName, functionName)            \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                 \
                                                                               \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__GET_ALIGNMENT_OUTPUT                                         \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__GET_ALIGNMENT_ACTION(componentName, functionName)            \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set alignment
#define UI_FLOW_NODE__SET_ALIGNMENT(componentName, functionName, nodeHelpText) \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                    \
    UI_FLOW_NODE__SET_ALIGNMENT_INPUT                                          \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                 \
    UI_FLOW_NODE__SET_ALIGNMENT_ACTION(3, componentName, functionName)         \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                 \
                                                                               \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                \
    UI_FLOW_NODE__SET_ALIGNMENT_INPUT                                          \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)             \
    UI_FLOW_NODE__SET_ALIGNMENT_ACTION(3, componentName, functionName)         \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is alignment
#define UI_FLOW_NODE__GET_AND_SET_ALIGNMENT(componentName, functionNameWithoutGetOrSet, \
                                            getNodeHelpText, setNodeHelpText)           \
    UI_FLOW_NODE__GET_ALIGNMENT(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText);                                                                   \
                                                                                        \
    UI_FLOW_NODE__SET_ALIGNMENT(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Image pathname to ISprite get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_SPRITE_ACTION(componentName, functionName)                                \
    ISprite * imgSprite;                                                                            \
    EBUS_EVENT_ID_RESULT(imgSprite, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, imgSprite->GetTexturePathname());

#define UI_FLOW_NODE__SET_SPRITE_INPUT(inputName, inputHelpText) \
    InputPortConfig<string>(inputName, _HELP(inputHelpText), 0, _UICONFIG("dt=file")),

#define UI_FLOW_NODE__SET_SPRITE_ACTION(portIndex, componentName, functionName)                          \
    string imgSrc = GetPortString(activationInfo, portIndex);                                            \
    ISprite* sprite = gEnv->pLyShine->LoadSprite(imgSrc);                                                \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, sprite);                \
    if (!sprite)                                                                                         \
    {                                                                                                    \
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,                                           \
            "FlowGraph: %s Node: couldn't find UI Image with source: %s\n", s_nodePath, imgSrc.c_str()); \
    }                                                                                                    \
    else                                                                                                 \
    {                                                                                                    \
        sprite->Release();                                                                               \
    }

// Define a complete class for a flow node that uses a get function on a bus to get a sprite
#define UI_FLOW_NODE__GET_IMAGE_PATH_AS_SPRITE(componentName, functionName, nodeHelpText, \
                                               output0Name, output0HelpText)              \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                               \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                        \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                            \
    UI_FLOW_NODE__GET_SPRITE_ACTION(componentName, functionName)                          \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                            \
                                                                                          \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                           \
    UI_FLOW_NODE__GET_OUTPUT(string, output0Name, output0HelpText)                        \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                        \
    UI_FLOW_NODE__GET_SPRITE_ACTION(componentName, functionName)                          \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a sprite
#define UI_FLOW_NODE__SET_IMAGE_PATH_AS_SPRITE(componentName, functionName, nodeHelpText, \
                                               input1Name, input1HelpText)                \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                               \
    UI_FLOW_NODE__SET_SPRITE_INPUT(input1Name, input1HelpText)                            \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                            \
    UI_FLOW_NODE__SET_SPRITE_ACTION(3, componentName, functionName)                       \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                            \
                                                                                          \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                           \
    UI_FLOW_NODE__SET_SPRITE_INPUT(input1Name, input1HelpText)                            \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                        \
    UI_FLOW_NODE__SET_SPRITE_ACTION(3, componentName, functionName)                       \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set sprite using a bus
#define UI_FLOW_NODE__GET_AND_SET_IMAGE_PATH_AS_SPRITE(componentName, functionNameWithoutGetOrSet, \
                                                       getNodeHelpText, setNodeHelpText,           \
                                                       inOut0Name, inOut0HelpText)                 \
    UI_FLOW_NODE__GET_IMAGE_PATH_AS_SPRITE(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                               \
    inOut0Name, inOut0HelpText);                                                                   \
                                                                                                   \
    UI_FLOW_NODE__SET_IMAGE_PATH_AS_SPRITE(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                               \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector2 as Float get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_NODE__GET_VECTOR2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    OutputPortConfig<float>(output0Name, _HELP(output0HelpText)),                                    \
    OutputPortConfig<float>(output1Name, _HELP(output1HelpText)),

#define UI_FLOW_NODE__GET_VECTOR2_ACTION(componentName, functionName)                             \
    AZ::Vector2 vector2;                                                                          \
    EBUS_EVENT_ID_RESULT(vector2, elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, vector2.GetX());                                                \
    ActivateOutput(&m_actInfo, 1, vector2.GetY());

#define UI_FLOW_NODE__SET_VECTOR2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    InputPortConfig<float>(input1Name, _HELP(input1HelpText)),                                  \
    InputPortConfig<float>(input2Name, _HELP(input2HelpText)),

#define UI_FLOW_NODE__SET_VECTOR2_ACTION(portIndex, componentName, functionName) \
    float x = GetPortFloat(activationInfo, portIndex);                           \
    float y = GetPortFloat(activationInfo, portIndex + 1);                       \
    EBUS_EVENT_ID(elementId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, AZ::Vector2(x, y));

// Define a complete class for a flow node that uses a get function on a bus to get a Vector2
// and expose the result as two floats
#define UI_FLOW_NODE__GET_VECTOR2_AS_FLOATS(componentName, functionName, nodeHelpText,           \
                                            output0Name, output0HelpText,                        \
                                            output1Name, output1HelpText)                        \
    UI_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                      \
    UI_FLOW_NODE__GET_VECTOR2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                                   \
    UI_FLOW_NODE__GET_VECTOR2_ACTION(componentName, functionName)                                \
    UI_FLOW_NODE__GET_END_SECTION(componentName, functionName)                                   \
                                                                                                 \
    UI_ENT_FLOW_NODE__GET_PREAMBLE(componentName, functionName)                                  \
    UI_FLOW_NODE__GET_VECTOR2_OUTPUT(output0Name, output0HelpText, output1Name, output1HelpText) \
    UI_ENT_FLOW_NODE__GET_MID_SECTION(componentName, nodeHelpText)                               \
    UI_FLOW_NODE__GET_VECTOR2_ACTION(componentName, functionName)                                \
    UI_ENT_FLOW_NODE__GET_END_SECTION(componentName, functionName)

// Define a complete class for a flow node that uses a set function on a bus to set a Vector2
// and expose the inputs as two floats
#define UI_FLOW_NODE__SET_VECTOR2_AS_FLOATS(componentName, functionName, nodeHelpText,      \
                                            input1Name, input1HelpText,                     \
                                            input2Name, input2HelpText)                     \
    UI_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                                 \
    UI_FLOW_NODE__SET_VECTOR2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    UI_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                              \
    UI_FLOW_NODE__SET_VECTOR2_ACTION(3, componentName, functionName)                        \
    UI_FLOW_NODE__SET_END_SECTION(componentName, functionName)                              \
                                                                                            \
    UI_ENT_FLOW_NODE__SET_PREAMBLE(componentName, functionName)                             \
    UI_FLOW_NODE__SET_VECTOR2_INPUT(input1Name, input1HelpText, input2Name, input2HelpText) \
    UI_ENT_FLOW_NODE__SET_MID_SECTION(componentName, nodeHelpText)                          \
    UI_FLOW_NODE__SET_VECTOR2_ACTION(3, componentName, functionName)                        \
    UI_ENT_FLOW_NODE__SET_END_SECTION(componentName, functionName)

// Defines two complete classes for flow nodes that get and set a value using a bus
// where the value is a Vector2 represented as 2 floats in flow graph
#define UI_FLOW_NODE__GET_AND_SET_VECTOR2_AS_FLOATS(componentName, functionNameWithoutGetOrSet, \
                                                    getNodeHelpText, setNodeHelpText,           \
                                                    inOut0Name, inOut0HelpText,                 \
                                                    inOut1Name, inOut1HelpText)                 \
    UI_FLOW_NODE__GET_VECTOR2_AS_FLOATS(componentName, Get##functionNameWithoutGetOrSet,        \
    getNodeHelpText,                                                                            \
    inOut0Name, inOut0HelpText,                                                                 \
    inOut1Name, inOut1HelpText);                                                                \
                                                                                                \
    UI_FLOW_NODE__SET_VECTOR2_AS_FLOATS(componentName, Set##functionNameWithoutGetOrSet,        \
    setNodeHelpText,                                                                            \
    inOut0Name, inOut0HelpText,                                                                 \
    inOut1Name, inOut1HelpText);
