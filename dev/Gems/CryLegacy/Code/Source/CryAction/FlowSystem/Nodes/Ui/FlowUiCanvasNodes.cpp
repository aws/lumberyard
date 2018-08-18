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
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>

namespace
{
    const string g_canvasLoadNodePath = "UI:Canvas:Load";
    const string g_canvasUnloadNodePath = "UI:Canvas:Unload";
    const string g_canvasFindLoadedNodePath = "UI:Canvas:FindLoaded";
    const string g_canvasActionListenerNodePath = "UI:Canvas:ActionListener";

    const string g_entCanvasLoadIntoEntityNodePath = "UIe:Canvas:LoadIntoEntity";
    const string g_entCanvasUnloadFromEntityNodePath = "UIe:Canvas:UnloadFromEntity";
    const string g_entCanvasActionListenerNodePath = "UIe:Canvas:ActionListener";

    const string g_entCanvasSetCanvasRefOnEntityNodePath = "UIe:Canvas:SetCanvasRefOnEntity";
    const string g_canvasCanvasEntityListenerNodePath = "UIe:Canvas:CanvasEntityListener";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for creating get/set UI canvas nodes that take a canvas ID as input
////////////////////////////////////////////////////////////////////////////////////////////////////

// Macro for the input ports
#define UI_FLOW_ADD_STD_CANVAS_INPUTS(componentName, activateStr) \
    InputPortConfig_Void("Activate", _HELP(activateStr)),         \
    InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the canvas"))

// Macro to wrap the code that gets the canvas entity IDs
#define UI_FLOW_IF_ACTIVATED_AND_CANVAS_IS_VALID(canvasPort)                        \
    if (eFE_Activate == event && IsPortActive(activationInfo, 0) && gEnv->pLyShine) \
    {                                                                               \
        AZ::EntityId canvasEntityId = UiFlow::GetCanvasEntityId(activationInfo,     \
                canvasPort, s_nodePath);                                            \
        if (canvasEntityId.IsValid())

// The preamble for a *Get* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, functionName)                     \
    UI_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)                             \
                                                                                           \
    void GetConfiguration(SFlowNodeConfig & config) override                               \
    {                                                                                      \
        static const SInputPortConfig inputs[] =                                           \
        {                                                                                  \
            UI_FLOW_ADD_STD_CANVAS_INPUTS(#componentName, "Trigger to update the output"), \
            {0}                                                                            \
        };                                                                                 \
        static const SOutputPortConfig outputs[] =                                         \
        {
// The mid section for a *Get* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                         \
    {0}                                                                            \
    };                                                                             \
    UI_FLOW_SET_CONFIG(nodeHelpText)                                               \
    }                                                                              \
                                                                                   \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override \
    {                                                                              \
        m_actInfo = *activationInfo;                                               \
        UI_FLOW_IF_ACTIVATED_AND_CANVAS_IS_VALID(1)                                \
        {
// The end section for a *Get* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, functionName) \
    }                                                                     \
    }                                                                     \
    }                                                                     \
                                                                          \
    UI_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

// The preamble for a *Set* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, functionName) \
    UI_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)         \
                                                                       \
    void GetConfiguration(SFlowNodeConfig & config) override           \
    {                                                                  \
        static const SInputPortConfig inputs[] =                       \
        {                                                              \
            UI_FLOW_ADD_STD_CANVAS_INPUTS(#componentName, "Trigger this node"),

// The mid section for a *Set* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                         \
    {0}                                                                            \
    };                                                                             \
                                                                                   \
    UI_FLOW_SET_OUTPUTS                                                            \
    UI_FLOW_SET_CONFIG(nodeHelpText)                                               \
    }                                                                              \
                                                                                   \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override \
    {                                                                              \
        UI_FLOW_IF_ACTIVATED_AND_CANVAS_IS_VALID(1)                                \
        {
// The end section for a *Set* UI canvas flow node
#define UI_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, functionName) \
    ActivateOutput(activationInfo, 0, 0);                                 \
    }                                                                     \
    }                                                                     \
    }                                                                     \
                                                                          \
    UI_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for creating get/set UI canvas nodes that take a canvas ref entity as input
////////////////////////////////////////////////////////////////////////////////////////////////////

// Macro for the input ports
#define UI_ENT_FLOW_ADD_STD_CANVAS_INPUTS(componentName, activateStr) \
    InputPortConfig_Void("Activate", _HELP(activateStr))

// The preamble for a *Get* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, functionName)                     \
    UI_ENT_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)                             \
                                                                                               \
    void GetConfiguration(SFlowNodeConfig & config) override                                   \
    {                                                                                          \
        static const SInputPortConfig inputs[] =                                               \
        {                                                                                      \
            UI_ENT_FLOW_ADD_STD_CANVAS_INPUTS(#componentName, "Trigger to update the output"), \
            {0}                                                                                \
        };                                                                                     \
        static const SOutputPortConfig outputs[] =                                             \
        {
// The mid section for a *Get* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                     \
    {0}                                                                            \
    };                                                                             \
    UI_ENT_FLOW_SET_CONFIG(nodeHelpText)                                           \
    }                                                                              \
                                                                                   \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override \
    {                                                                              \
        m_actInfo = *activationInfo;                                               \
        UI_FLOW_IF_ACTIVATED_AND_CANVAS_IS_VALID(-1)                               \
        {
// The end section for a *Get* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, functionName) \
    }                                                                         \
    }                                                                         \
    }                                                                         \
                                                                              \
    UI_ENT_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

// The preamble for a *Set* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, functionName) \
    UI_ENT_FLOW_BEGIN_FLOW_NODE_CLASS(componentName, functionName)         \
                                                                           \
    void GetConfiguration(SFlowNodeConfig & config) override               \
    {                                                                      \
        static const SInputPortConfig inputs[] =                           \
        {                                                                  \
            UI_ENT_FLOW_ADD_STD_CANVAS_INPUTS(#componentName, "Trigger this node"),

// The mid section for a *Set* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                     \
    {0}                                                                            \
    };                                                                             \
                                                                                   \
    UI_FLOW_SET_OUTPUTS                                                            \
    UI_ENT_FLOW_SET_CONFIG(nodeHelpText)                                           \
    }                                                                              \
                                                                                   \
    void ProcessEvent(EFlowEvent event, SActivationInfo * activationInfo) override \
    {                                                                              \
        UI_FLOW_IF_ACTIVATED_AND_CANVAS_IS_VALID(-1)                               \
        {
// The end section for a *Set* UI canvas flow node
#define UI_ENT_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, functionName) \
    ActivateOutput(activationInfo, 0, 0);                                     \
    }                                                                         \
    }                                                                         \
    }                                                                         \
                                                                              \
    UI_ENT_FLOW_END_FLOW_NODE_CLASS(componentName, functionName)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros for processing simple canvas get/set events
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UI_FLOW_CANVAS_NODE__GET_ACTION(resultType, componentName, functionName)                      \
    resultType result;                                                                                \
    EBUS_EVENT_ID_RESULT(result, canvasEntityId, UI_FLOW_NODE_BUS_NAME(componentName), functionName); \
    ActivateOutput(&m_actInfo, 0, result);

#define UI_FLOW_CANVAS_NODE__SET_ACTION(inputType, portType, portIndex, componentName, functionName) \
    inputType typeInput = GetPort##portType(activationInfo, portIndex);                              \
    EBUS_EVENT_ID(canvasEntityId, UI_FLOW_NODE_BUS_NAME(componentName), functionName, typeInput);

////////////////////////////////////////////////////////////////////////////////////////////////////
// bool get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

// Define a complete class for a canvas flow node that uses a get function on a bus to get a bool
#define UI_FLOW_CANVAS_NODE__GET_BOOL(componentName, nodeName, functionName, nodeHelpText, \
                                      output0Name, output0HelpText)                        \
    UI_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, nodeName)                             \
    UI_FLOW_NODE__GET_OUTPUT(bool, output0Name, output0HelpText)                           \
    UI_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                                     \
    UI_FLOW_CANVAS_NODE__GET_ACTION(bool, componentName, functionName)                     \
    UI_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, nodeName)                          \
                                                                                           \
    UI_ENT_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, nodeName)                         \
    UI_FLOW_NODE__GET_OUTPUT(bool, output0Name, output0HelpText)                           \
    UI_ENT_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                                 \
    UI_FLOW_CANVAS_NODE__GET_ACTION(bool, componentName, functionName)                     \
    UI_ENT_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, nodeName)

// Define a complete class for a canvas flow node that uses a set function on a bus to set a bool
#define UI_FLOW_CANVAS_NODE__SET_BOOL(componentName, nodeName, functionName, nodeHelpText, \
                                      input1Name, input1HelpText)                          \
    UI_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, nodeName)                             \
    UI_FLOW_NODE__SET_INPUT(bool, input1Name, input1HelpText)                              \
    UI_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                                     \
    UI_FLOW_CANVAS_NODE__SET_ACTION(bool, Bool, 2, componentName, functionName)            \
    UI_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, nodeName)                          \
                                                                                           \
    UI_ENT_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, nodeName)                         \
    UI_FLOW_NODE__SET_INPUT(bool, input1Name, input1HelpText)                              \
    UI_ENT_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                                 \
    UI_FLOW_CANVAS_NODE__SET_ACTION(bool, Bool, 1, componentName, functionName)            \
    UI_ENT_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, nodeName)

// Defines two complete classes for canvas flow nodes that get and set a bool value using a bus
#define UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(nodeNameWithoutGetOrSet, functionNameWithoutGetOrSet,       \
                                              getNodeHelpText, setNodeHelpText,                           \
                                              inOut0Name, inOut0HelpText)                                 \
    UI_FLOW_CANVAS_NODE__GET_BOOL(Canvas, Get##nodeNameWithoutGetOrSet, Get##functionNameWithoutGetOrSet, \
    getNodeHelpText,                                                                                      \
    inOut0Name, inOut0HelpText);                                                                          \
                                                                                                          \
    UI_FLOW_CANVAS_NODE__SET_BOOL(Canvas, Set##nodeNameWithoutGetOrSet, Set##functionNameWithoutGetOrSet, \
    setNodeHelpText,                                                                                      \
    inOut0Name, inOut0HelpText);

////////////////////////////////////////////////////////////////////////////////////////////////////
// int get/set nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

// Define a complete class for a canvas flow node that uses a get function on a bus to get a int
#define UI_FLOW_CANVAS_NODE__GET_INT(componentName, nodeName, functionName, nodeHelpText, \
                                     output0Name, output0HelpText)                        \
    UI_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, nodeName)                            \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)                           \
    UI_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                                    \
    UI_FLOW_CANVAS_NODE__GET_ACTION(int, componentName, functionName)                     \
    UI_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, nodeName)                         \
                                                                                          \
    UI_ENT_FLOW_CANVAS_NODE__GET_PREAMBLE(componentName, nodeName)                        \
    UI_FLOW_NODE__GET_OUTPUT(int, output0Name, output0HelpText)                           \
    UI_ENT_FLOW_CANVAS_NODE__GET_MID_SECTION(nodeHelpText)                                \
    UI_FLOW_CANVAS_NODE__GET_ACTION(int, componentName, functionName)                     \
    UI_ENT_FLOW_CANVAS_NODE__GET_END_SECTION(componentName, nodeName)

// Define a complete class for a canvas flow node that uses a set function on a bus to set a int
#define UI_FLOW_CANVAS_NODE__SET_INT(componentName, nodeName, functionName, nodeHelpText, \
                                     input1Name, input1HelpText)                          \
    UI_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, nodeName)                            \
    UI_FLOW_NODE__SET_INPUT(int, input1Name, input1HelpText)                              \
    UI_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                                    \
    UI_FLOW_CANVAS_NODE__SET_ACTION(int, Int, 2, componentName, functionName)             \
    UI_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, nodeName)                         \
                                                                                          \
    UI_ENT_FLOW_CANVAS_NODE__SET_PREAMBLE(componentName, nodeName)                        \
    UI_FLOW_NODE__SET_INPUT(int, input1Name, input1HelpText)                              \
    UI_ENT_FLOW_CANVAS_NODE__SET_MID_SECTION(nodeHelpText)                                \
    UI_FLOW_CANVAS_NODE__SET_ACTION(int, Int, 1, componentName, functionName)             \
    UI_ENT_FLOW_CANVAS_NODE__SET_END_SECTION(componentName, nodeName)

// Defines two complete classes for canvas flow nodes that get and set a int value using a bus
#define UI_FLOW_CANVAS_NODE__GET_AND_SET_INT(nodeNameWithoutGetOrSet, functionNameWithoutGetOrSet,       \
                                             getNodeHelpText, setNodeHelpText,                           \
                                             inOut0Name, inOut0HelpText)                                 \
    UI_FLOW_CANVAS_NODE__GET_INT(Canvas, Get##nodeNameWithoutGetOrSet, Get##functionNameWithoutGetOrSet, \
    getNodeHelpText,                                                                                     \
    inOut0Name, inOut0HelpText);                                                                         \
                                                                                                         \
    UI_FLOW_CANVAS_NODE__SET_INT(Canvas, Set##nodeNameWithoutGetOrSet, Set##functionNameWithoutGetOrSet, \
    setNodeHelpText,                                                                                     \
    inOut0Name, inOut0HelpText);


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasLoadNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for loading a UI canvas
class CFlowUiCanvasLoadNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiCanvasLoadNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiCanvasLoadNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Load the canvas")),
            InputPortConfig<string>("CanvasPathname", _HELP("The pathname of the canvas to load"), 0, _UICONFIG("dt=file")),
            InputPortConfig<bool>("Disabled", false, _HELP("Whether the canvas should be disabled initially. If disabled, the canvas won't be updated or rendered.")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnLoad", _HELP("Sends a signal when the canvas is loaded")),
            OutputPortConfig<int>("CanvasID", _HELP("Outputs the unique canvas ID when the canvas is loaded")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Load the specified UI canvas");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            // Load the canvas
            string canvasName = GetPortString(activationInfo, InputPortCanvasPathname);
            AZ::EntityId canvasEntityId = gEnv->pLyShine->LoadCanvas(canvasName);
            if (!canvasEntityId.IsValid())
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't load UI canvas with path: %s\n", g_canvasLoadNodePath.c_str(), canvasName.c_str());
                return;
            }

            // Until flowgraph supports ports for 64-bit EntityIds we need a separate UI canvas ID.
            LyShine::CanvasId canvasId = 0;
            EBUS_EVENT_ID_RESULT(canvasId, canvasEntityId, UiCanvasBus, GetCanvasId);

            bool disabled = GetPortBool(activationInfo, InputPortDisabled);
            EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetEnabled, !disabled);

            // Trigger output
            ActivateOutput(activationInfo, OutputPortOnLoad, 0);
            ActivateOutput(activationInfo, OutputPortCanvasId, canvasId);
        }
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
        InputPortCanvasPathname,
        InputPortDisabled,
    };

    enum OutputPorts
    {
        OutputPortOnLoad = 0,
        OutputPortCanvasId,
    };
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasUnloadNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for unloading a UI canvas
class CFlowUiCanvasUnloadNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiCanvasUnloadNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiCanvasUnloadNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Unload the canvas")),
            InputPortConfig<int>("CanvasId", 0, _HELP("The unique ID of the canvas to unload")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Fired after the node is finished")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Unload the specified UI canvas");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            // Unload the canvas
            int canvasID = GetPortInt(activationInfo, InputPortCanvasId);
            AZ::EntityId canvasEntityId = gEnv->pLyShine->FindCanvasById(canvasID);
            if (canvasEntityId.IsValid())
            {
                gEnv->pLyShine->ReleaseCanvas(canvasEntityId, false);

                // Trigger output
                ActivateOutput(activationInfo, OutputPortDone, 0);
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find UI canvas with ID: %d\n", g_canvasUnloadNodePath.c_str(), canvasID);
            }
        }
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
        InputPortCanvasId,
    };

    enum OutputPorts
    {
        OutputPortDone = 0
    };
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasFindLoadedNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for finding a UI canvas that has already been loaded
class CFlowUiCanvasFindLoadedNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiCanvasFindLoadedNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiCanvasFindLoadedNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Find the loaded canvas")),
            InputPortConfig<string>("CanvasPathname", _HELP("The pathname of the loaded canvas to find"), 0, _UICONFIG("dt=file")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("CanvasID", _HELP("Outputs the unique canvas ID of the loaded canvas")),
            OutputPortConfig<bool>("Found", _HELP("True if a loaded instance of the canvas was found, false otherwise")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Find the specified UI canvas if it has already been loaded");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            // Find the loaded canvas
            string canvasName = GetPortString(activationInfo, InputPortCanvasPathname);
            AZ::EntityId canvasEntityId = gEnv->pLyShine->FindLoadedCanvasByPathName(canvasName);
            if (!canvasEntityId.IsValid())
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FlowGraph: %s Node: couldn't find loaded UI canvas with path: %s\n", g_canvasLoadNodePath.c_str(), canvasName.c_str());
                ActivateOutput(activationInfo, OutputPortFound, false);
                return;
            }

            // Until flowgraph supports ports for 64-bit EntityIds we need a separate UI canvas ID.
            LyShine::CanvasId canvasId = 0;
            EBUS_EVENT_ID_RESULT(canvasId, canvasEntityId, UiCanvasBus, GetCanvasId);

            // Trigger output
            ActivateOutput(activationInfo, OutputPortCanvasId, canvasId);
            ActivateOutput(activationInfo, OutputPortFound, true);
        }
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
        InputPortCanvasPathname,
    };

    enum OutputPorts
    {
        OutputPortCanvasId = 0,
        OutputPortFound,
    };
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasActionListenerBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for handling UI canvas events
class CFlowUiCanvasActionListenerBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
    , public UiCanvasNotificationBus::Handler
{
public:
    CFlowUiCanvasActionListenerBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , UiCanvasNotificationBus::Handler()
        , m_actInfo(*activationInfo)
        , m_actionName("")
        , m_hasTargetEntity(hasTargetEntity)
        , m_entityId()
    {
    }

    ~CFlowUiCanvasActionListenerBaseNode() override
    {
        UnregisterListener();
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Trigger to start listening for the specified action")),
            InputPortConfig<int>("CanvasID", 0, _HELP("The unique ID of the canvas to listen to")),
            InputPortConfig<string>("ActionName", _HELP("The name of the action to listen for")),
            InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the element sending the action. Leave at 0 to get notified for any element.")),
            { 0 }
        };

        static const SInputPortConfig entInputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Trigger to start listening for the specified action")),
            InputPortConfig<string>("ActionName", _HELP("The name of the action to listen for")),
            InputPortConfig<string>("ElementName", _HELP("The name of the element sending the action. Leave empty to get notified for any element."), 0, _UICONFIG("dt=uiElement")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnAction", _HELP("Triggers output when the canvas sends the action")),
            OutputPortConfig<int>("ElementID", _HELP("The ID of the UI element that triggered the action")),
            { 0 }
        };

        static const SOutputPortConfig entOutputs[] =
        {
            OutputPortConfig_Void("OnAction", _HELP("Triggers output when the canvas sends the action")),
            OutputPortConfig<string>("ElementName", _HELP("The name of the UI element that triggered the action")),
            OutputPortConfig<int>("ElementID", _HELP("The ID of the UI element that triggered the action")),
            { 0 }
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = m_hasTargetEntity ? entOutputs : outputs;
        config.sDescription = _HELP("Listen for a specified UI canvas action");
        config.SetCategory(EFLN_APPROVED);
        if (m_hasTargetEntity)
        {
            config.nFlags |= EFLN_TARGET_ENTITY;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            // If this node was already triggered, remove it as a listener
            // so it doesn't get other random callbacks
            UnregisterListener();

            // Get canvas entity Id
            m_canvasEntityId = UiFlow::GetCanvasEntityId(activationInfo,
                    (m_hasTargetEntity ? -1 : InputPortCanvasId),
                    (m_hasTargetEntity ? g_entCanvasActionListenerNodePath : g_canvasActionListenerNodePath));
            if (m_canvasEntityId.IsValid())
            {
                int inputPortOffsetStart = (m_hasTargetEntity ? InputPortCanvasId : InputPortCanvasId + 1);

                // Get the action name
                m_actionName = GetPortString(activationInfo, inputPortOffsetStart + InputPortOffsetActionName);

                // Get the entity id
                if (m_hasTargetEntity)
                {
                    // Get the element name
                    string elementName = GetPortString(activationInfo, InputPortElementId);
                    if (!elementName.empty())
                    {
                        AZ::Entity* element = nullptr;
                        EBUS_EVENT_ID_RESULT(element, m_canvasEntityId, UiCanvasBus, FindElementByName, elementName.c_str());

                        if (element)
                        {
                            m_entityId = element->GetId();
                        }
                    }
                }
                else
                {
                    // Get the element ID
                    int elementId = GetPortInt(activationInfo, InputPortElementId);
                    if (elementId)
                    {
                        AZ::Entity* element = nullptr;
                        EBUS_EVENT_ID_RESULT(element, m_canvasEntityId, UiCanvasBus, FindElementById, elementId);

                        if (element)
                        {
                            m_entityId = element->GetId();
                        }
                    }
                }

                // Start listening for the action
                UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);
            }
        }
    }

    //-- ~IFlowNode --

    //-- UiCanvasActionNotification --

    void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override
    {
        if (actionName == m_actionName.c_str() &&
            (!m_entityId.IsValid() || (m_entityId == entityId)))
        {
            ActivateOutput(&m_actInfo, OutputPortOnAction, 0);

            // Get the element ID from the entity ID
            LyShine::ElementId elementId = 0;
            EBUS_EVENT_ID_RESULT(elementId, entityId, UiElementBus, GetElementId);

            if (m_hasTargetEntity)
            {
                // Get the element name from the entity ID
                LyShine::NameType elementName;
                EBUS_EVENT_ID_RESULT(elementName, entityId, UiElementBus, GetName);
                string elementNameOut = elementName.c_str();
                ActivateOutput(&m_actInfo, OutputPortElementNameOrId, elementNameOut);
                ActivateOutput(&m_actInfo, OutputPortElementId, elementId);
            }
            else
            {
                ActivateOutput(&m_actInfo, OutputPortElementNameOrId, elementId);
            }
        }
    }

    //-- ~UiCanvasActionNotification --

protected:
    //! Unregister from receiving actions
    void UnregisterListener()
    {
        if (m_canvasEntityId.IsValid())
        {
            // Stop listening for actions
            UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
            m_canvasEntityId.SetInvalid();
        }
    }

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortCanvasId,
        InputPortElementId
    };

    enum InputOffsetPorts
    {
        InputPortOffsetActionName = 0
    };

    enum OutputPorts
    {
        OutputPortOnAction = 0,
        OutputPortElementNameOrId,
        OutputPortElementId
    };

    SActivationInfo m_actInfo;

    //! The canvas this node is registered with, if any
    AZ::EntityId m_canvasEntityId;

    //! The action this node is registered to listen for, if any
    string m_actionName;

    //! Specifies whether the node contains a canvas ID port or a target entity port
    bool m_hasTargetEntity;

    //! The entity that triggered the action
    AZ::EntityId m_entityId;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasActionListenerNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiCanvasActionListenerNode
    : public CFlowUiCanvasActionListenerBaseNode
{
public:
    CFlowUiCanvasActionListenerNode(SActivationInfo* activationInfo)
        : CFlowUiCanvasActionListenerBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiCanvasActionListenerNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntCanvasActionListenerNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntCanvasActionListenerNode
    : public CFlowUiCanvasActionListenerBaseNode
{
public:
    CFlowUiEntCanvasActionListenerNode(SActivationInfo* activationInfo)
        : CFlowUiCanvasActionListenerBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntCanvasActionListenerNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntCanvasLoadIntoEntityNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for loading a UI canvas
class CFlowUiEntCanvasLoadIntoEntityNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiEntCanvasLoadIntoEntityNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntCanvasLoadIntoEntityNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Load the canvas")),
            InputPortConfig<bool>("Disabled", false, _HELP("Whether the canvas should be disabled initially. If disabled, the canvas won't be updated or rendered.")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnLoad", _HELP("Sends a signal when the canvas is loaded")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Load the UI canvas defined by the assigned UiCanvasRefEntity");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            AZ::EntityId canvasEntityId;

            FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);
            if (flowEntityType == FlowEntityType::Legacy)
            {
                IEntity* entity = activationInfo->pEntity;
                AZ_Assert(entity, "FlowGraph: %s Node: couldn't find entity\n", g_entCanvasLoadIntoEntityNodePath.c_str());
                if (entity)
                {
                    canvasEntityId = ProcessCanvasLoadForLegacyEntity(entity);
                }
            }
            else if (flowEntityType == FlowEntityType::Component)
            {
                AZ::EntityId entityId = activationInfo->entityId;

                canvasEntityId = ProcessCanvasLoadForComponentEntity(entityId);
            }
            else if (flowEntityType == FlowEntityType::Invalid)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: entity not assigned\n",
                    g_entCanvasLoadIntoEntityNodePath.c_str());
            }
            else
            {
                AZ_Assert(false, "FlowGraph: %s Node: invalid flowEntityType\n", g_entCanvasLoadIntoEntityNodePath.c_str());
            }

            // if we succesfully loaded a canvas then set the disabled flag if required and activate the output port
            if (canvasEntityId.IsValid())
            {
                bool disabled = GetPortBool(activationInfo, InputPortDisabled);
                EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, SetEnabled, !disabled);

                // Trigger output
                ActivateOutput(activationInfo, OutputPortOnLoad, 0);
            }
        }
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --

    AZ::EntityId ProcessCanvasLoadForLegacyEntity(IEntity* entity)
    {
        AZ::EntityId canvasEntityId;

        // Get the canvas name property from the entity
        string canvasPathname;
        bool foundProperty = false;
        IScriptTable* scriptTable = entity->GetScriptTable();
        if (scriptTable)
        {
            SmartScriptTable scriptProps;
            if (scriptTable->GetValue("Properties", scriptProps))
            {
                if (scriptProps->GetValue("fileCanvasPath", canvasPathname))
                {
                    foundProperty = true;

                    if (canvasPathname.empty())
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                            "FlowGraph: %s Node: CanvasPath property of assigned UiCanvasRefEntity is empty\n",
                            g_entCanvasLoadIntoEntityNodePath.c_str());
                        return canvasEntityId;
                    }
                    else
                    {
                        // Load the canvas
                        canvasEntityId = gEnv->pLyShine->LoadCanvas(canvasPathname);
                        if (canvasEntityId.IsValid())
                        {
                            // Set canvas ID on the entity
                            LyShine::CanvasId canvasId = 0;
                            EBUS_EVENT_ID_RESULT(canvasId, canvasEntityId, UiCanvasBus, GetCanvasId);

                            scriptTable->SetValue("canvasID", canvasId);
                        }
                        else
                        {
                            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                                "FlowGraph: %s Node: couldn't load UI canvas with path: %s\n",
                                g_entCanvasLoadIntoEntityNodePath.c_str(), canvasPathname.c_str());
                            return canvasEntityId;
                        }
                    }
                }
            }
        }
        if (!foundProperty)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: assigned entity needs to be a UiCanvasRefEntity\n",
                g_entCanvasLoadIntoEntityNodePath.c_str());
        }

        return canvasEntityId;
    }

    AZ::EntityId ProcessCanvasLoadForComponentEntity(AZ::EntityId entityId)
    {
        AZ::EntityId canvasEntityId;

        if (!UiCanvasAssetRefBus::FindFirstHandler(entityId))
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasAssetRef component\n",
                g_entCanvasLoadIntoEntityNodePath.c_str());
        }
        else
        {
            EBUS_EVENT_ID_RESULT(canvasEntityId, entityId, UiCanvasAssetRefBus, LoadCanvas);
            if (!canvasEntityId.IsValid())
            {
                AZStd::string canvasPathname;
                EBUS_EVENT_ID_RESULT(canvasPathname, entityId, UiCanvasAssetRefBus, GetCanvasPathname);

                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: couldn't load UI canvas with path: %s\n",
                    g_entCanvasLoadIntoEntityNodePath.c_str(), canvasPathname.c_str());
            }
        }

        return canvasEntityId;
    }

protected:
    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortDisabled,
    };

    enum OutputPorts
    {
        OutputPortOnLoad = 0
    };
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntCanvasUnloadFromEntityNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for unloading a UI canvas
class CFlowUiEntCanvasUnloadFromEntityNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiEntCanvasUnloadFromEntityNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntCanvasUnloadFromEntityNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Unload the canvas")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Fired after the node is finished")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Unload the specified UI canvas");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);
            if (flowEntityType == FlowEntityType::Component)
            {
                // get the main entity for this node. It should be a component entity with a
                // UiCanvasRef component on it.
                AZ::EntityId entityId = activationInfo->entityId;

                if (!UiCanvasAssetRefBus::FindFirstHandler(entityId))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasRef component\n",
                        g_entCanvasUnloadFromEntityNodePath.c_str());
                    return;
                }

                EBUS_EVENT_ID(entityId, UiCanvasAssetRefBus, UnloadCanvas);

                // Trigger output
                ActivateOutput(activationInfo, OutputPortDone, 0);
            }
            else
            {
                // Get canvas entity Id
                AZ::EntityId canvasEntityId = UiFlow::GetCanvasEntityId(activationInfo,
                        -1, g_entCanvasUnloadFromEntityNodePath);
                if (canvasEntityId.IsValid())
                {
                    // Unload the canvas
                    gEnv->pLyShine->ReleaseCanvas(canvasEntityId, false);

                    // Clear canvas ID on the entity
                    IEntity* entity = activationInfo->pEntity;
                    if (entity)
                    {
                        IScriptTable* scriptTable = entity->GetScriptTable();
                        if (scriptTable)
                        {
                            scriptTable->SetValue("canvasID", 0);
                        }
                    }

                    // Trigger output
                    ActivateOutput(activationInfo, OutputPortDone, 0);
                }
            }
        }
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --

protected:
    enum InputPorts
    {
        InputPortActivate = 0
    };

    enum OutputPorts
    {
        OutputPortDone = 0
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntCanvasSetCanvasRefOnEntityNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for setting the canvas on a UiCanvasRef component
//! This is only needed if you want to use the same canvas instance on multiple in-world entities
//! or if you are loading the canvas in some other way than via the UiCanvasRef component.
class CFlowUiEntCanvasSetCanvasRefOnEntityNode
    : public CFlowBaseNode < eNCT_Instanced >
{
public:
    CFlowUiEntCanvasSetCanvasRefOnEntityNode(SActivationInfo* activationInfo)
        : CFlowBaseNode() {}

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntCanvasSetCanvasRefOnEntityNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Set the UI Canvas Reference")),
            InputPortConfig<FlowEntityId>("CanvasEntityId", _HELP("An entity that has the desired canvas already loaded into it")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Fired after the node is finished")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Set the UI Canvas Reference to an already loaded canvas");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);
            if (flowEntityType == FlowEntityType::Component)
            {
                // get the main entity for this node. It should be a component entity with a
                // UiCanvasRef component on it.
                AZ::EntityId entityId = activationInfo->entityId;

                if (!UiCanvasProxyRefBus::FindFirstHandler(entityId))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasProxyRef component\n",
                        g_entCanvasSetCanvasRefOnEntityNodePath.c_str());
                    return;
                }

                AZ::EntityId canvasRefEntityId;
                FlowEntityId fromCanvasFlowEntityId = FlowEntityId(GetPortEntityId(activationInfo, InputPortFromCanvasEntity));
                FlowEntityType fromCanvasFlowEntityType = GetFlowEntityTypeFromFlowEntityId(fromCanvasFlowEntityId);
                if (fromCanvasFlowEntityType == FlowEntityType::Component)
                {
                    canvasRefEntityId = fromCanvasFlowEntityId;
                    if (!UiCanvasRefBus::FindFirstHandler(canvasRefEntityId))
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                            "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasRef component\n",
                            g_canvasCanvasEntityListenerNodePath.c_str());
                        return;
                    }
                }

                // Get the UI canvas entity from the input port
                if (canvasRefEntityId.IsValid())
                {
                    // Set the canvas
                    EBUS_EVENT_ID(entityId, UiCanvasProxyRefBus, SetCanvasRefEntity, canvasRefEntityId);

                    // Trigger output
                    ActivateOutput(activationInfo, OutputPortDone, 0);
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: assigned entity must be a component entity\n",
                    g_entCanvasSetCanvasRefOnEntityNodePath.c_str());
            }
        }
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
        InputPortFromCanvasEntity = 1
    };

    enum OutputPorts
    {
        OutputPortDone = 0
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiCanvasEntityListenerNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for listing for changes on an entity in the level that loads a canvas
class CFlowUiCanvasEntityListenerNode
    : public CFlowBaseNode < eNCT_Instanced >
    , public UiCanvasRefNotificationBus::Handler
    , public UiCanvasAssetRefNotificationBus::Handler
{
public:
    CFlowUiCanvasEntityListenerNode(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , UiCanvasRefNotificationBus::Handler()
        , UiCanvasAssetRefNotificationBus::Handler()
        , m_actInfo(*activationInfo)
    {
    }

    ~CFlowUiCanvasEntityListenerNode() override
    {
        UnregisterListener();
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiCanvasEntityListenerNode(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Trigger to start listening for the canvas entity events")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnLoad", _HELP("Triggers output when a canvas is loaded into the entity")),
            OutputPortConfig<int>("OnCanvasChanged", _HELP("Triggers output when then entity canvas reference changes (via load, set, unload etc)")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Listen for changes on an entity with a UI Canvas Ref component on it");
        config.SetCategory(EFLN_APPROVED);
        config.nFlags |= EFLN_TARGET_ENTITY;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
        {
            // If this node was already triggered, remove it as a listener
            // so it doesn't get other random callbacks
            UnregisterListener();

            FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInfo->entityId);
            if (flowEntityType == FlowEntityType::Component)
            {
                // get the main entity for this node. It should be a component entity with a
                // UiCanvasRef component on it.
                AZ::EntityId entityId = activationInfo->entityId;

                if (!UiCanvasRefBus::FindFirstHandler(entityId))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasRef component\n",
                        g_canvasCanvasEntityListenerNodePath.c_str());
                    return;
                }

                if (entityId.IsValid())
                {
                    // Start listening for the action
                    UiCanvasRefNotificationBus::Handler::BusConnect(entityId);
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: assigned entity must be a component entity\n",
                    g_canvasCanvasEntityListenerNodePath.c_str());
            }
        }
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --

    //-- UiCanvasRefListener --

    void OnCanvasLoadedIntoEntity(AZ::EntityId entityId) override
    {
        ActivateOutput(&m_actInfo, OutputPortOnLoad, 0);
    }

    void OnCanvasRefChanged(AZ::EntityId uiCanvasRefEntity, AZ::EntityId uiCanvasEntity) override
    {
        ActivateOutput(&m_actInfo, OutputPortOnCanvasChanged, 0);
    }

    //-- ~UiCanvasRefListener --

protected:
    //! Unregister from receiving actions
    void UnregisterListener()
    {
        if (m_canvasEntityId.IsValid())
        {
            // Stop listening for actions
            UiCanvasRefNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
            m_canvasEntityId.SetInvalid();
        }
    }

    enum InputPorts
    {
        InputPortActivate = 0,
    };

    enum OutputPorts
    {
        OutputPortOnLoad = 0,
        OutputPortOnCanvasChanged = 1,
    };

    SActivationInfo m_actInfo;

    //! The canvas this node is registered with, if any
    AZ::EntityId m_canvasEntityId;
};

REGISTER_FLOW_NODE(g_canvasLoadNodePath, CFlowUiCanvasLoadNode);
REGISTER_FLOW_NODE(g_canvasUnloadNodePath, CFlowUiCanvasUnloadNode);
REGISTER_FLOW_NODE(g_canvasFindLoadedNodePath, CFlowUiCanvasFindLoadedNode);
REGISTER_FLOW_NODE(g_canvasActionListenerNodePath, CFlowUiCanvasActionListenerNode);

REGISTER_FLOW_NODE(g_entCanvasLoadIntoEntityNodePath, CFlowUiEntCanvasLoadIntoEntityNode);
REGISTER_FLOW_NODE(g_entCanvasUnloadFromEntityNodePath, CFlowUiEntCanvasUnloadFromEntityNode);
REGISTER_FLOW_NODE(g_entCanvasActionListenerNodePath, CFlowUiEntCanvasActionListenerNode);

REGISTER_FLOW_NODE(g_entCanvasSetCanvasRefOnEntityNodePath, CFlowUiEntCanvasSetCanvasRefOnEntityNode);
REGISTER_FLOW_NODE(g_canvasCanvasEntityListenerNodePath, CFlowUiCanvasEntityListenerNode);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get/Set flow nodes
////////////////////////////////////////////////////////////////////////////////////////////////////

UI_FLOW_CANVAS_NODE__GET_AND_SET_INT(DrawOrder, DrawOrder,
    "Get the draw order on the canvas",
    "Set the draw order on the canvas",
    "DrawOrder", "The draw order number for the canvas. Higher numbers draw in front of lower numbers.");

UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(KeepLoaded, KeepLoadedOnLevelUnload,
    "Get whether the canvas stays loaded on level unload",
    "Set whether the canvas stays loaded on level unload",
    "KeepLoaded", "If true the canvas will stay loaded on level unload");

UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(Enabled, Enabled,
    "Get the enabled flag of the canvas. Enabled canvases are updated and rendered each frame.",
    "Set whether the canvas is enabled. Enabled canvases are updated and rendered each frame.",
    "Enabled", "The enabled flag of the canvas (true if enabled, false otherwise)");

UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(IsPixelAligned, IsPixelAligned,
    "Get whether visual element's vertices should snap to the nearest pixel",
    "Set whether visual element's vertices should snap to the nearest pixel",
    "IsPixelAligned", "Whether visual element's vertices should snap to the nearest pixel");

UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(IsPositionalInputSupported, IsPositionalInputSupported,
    "Get whether the canvas accepts input from mouse/touch",
    "Set whether the canvas accepts input from mouse/touch",
    "IsPositionalInputSupported", "Whether the canvas accepts input from mouse/touch");

UI_FLOW_CANVAS_NODE__GET_AND_SET_BOOL(IsNavigationSupported, IsNavigationSupported,
    "Get whether the canvas accepts navigation input",
    "Set whether the canvas accepts navigation input",
    "IsNavigationSupported", "Whether the canvas accepts navigation input");
