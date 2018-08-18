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
#include <IRenderer.h>
#include <LyShine/IDraw2d.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node rendering 2d debug circles
class CFlowNode_Draw2dCircle
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    //! Input port enum list
    enum Inputs
    {
        InputDraw = 0,
        InputScreenX,
        InputScreenY,
        InputRadius,
        InputColor,
        InputOpacity,
        InputTime,
    };

    //! "Default" constructor for the debug draw 2d circle flow graph node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowNode_Draw2dCircle(SActivationInfo* activationInfo)
        : CFlowBaseNode()
    {
    }

    //! Destructor, does nothing
    ~CFlowNode_Draw2dCircle() override {}

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    // IFlowNode

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<SFlowSystemVoid>("Draw", _HELP("Trigger to draw the 2D circle")),
            InputPortConfig<float>("ScreenX", 10.0f, _HELP("X postion in screen space")),
            InputPortConfig<float>("ScreenY", 10.0f, _HELP("Y postion in screen space")),
            InputPortConfig<float>("Radius", _HELP("Radius of circle in screen space")),
            InputPortConfig<Vec3>("clr_Color", Vec3(1.0f, 1.0f, 1.0f), _HELP("Color of the circle")),
            InputPortConfig<float>("Opacity", 1.0f, _HELP("Transparency of the circle. Range from 0 to 1")),
            InputPortConfig<float>("Time", _HELP("How much time the message will be visible. 0 = permanently visible.")),
            { 0 }
        };

        config.sDescription = _HELP("Draws a 2d circle for debugging purposes");
        config.pInputPorts = in_config;
        config.SetCategory(EFLN_DEBUG);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
#if !defined(_RELEASE)
        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputDraw))
            {
                IPersistentDebug* persistentDebug = CCryAction::GetCryAction()->GetIPersistentDebug();
                if (persistentDebug)
                {
                    const float x = GetPortFloat(activationInfo, InputScreenX);
                    const float y = GetPortFloat(activationInfo, InputScreenY);
                    const float radius = GetPortFloat(activationInfo, InputRadius);
                    const float time = GetPortFloat(activationInfo, InputTime);
                    const ColorF color = ColorF(GetPortVec3(activationInfo, InputColor), GetPortFloat(activationInfo, InputOpacity));

                    persistentDebug->Begin("FG_Circle2d", false);
                    persistentDebug->Add2DCircle(x, y, radius, color, time);
                }
            }
        }
#endif
    }

    // ~IFlowNode
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node rendering 2d debug rectangles
class CFlowNode_Draw2dRect
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    //! Input port enum list
    enum Inputs
    {
        InputDraw = 0,
        InputScreenX,
        InputScreenY,
        InputWidth,
        InputHeight,
        InputCentered,
        InputColor,
        InputOpacity,
        InputTime,
    };

    //! "Default" constructor for the debug draw 2d rectangle flow graph node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowNode_Draw2dRect(SActivationInfo* activationInfo)
        : CFlowBaseNode()
    {
    }

    ///! Destructor, does nothing
    ~CFlowNode_Draw2dRect() override {}

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    // IFlowNode

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<SFlowSystemVoid>("Draw", _HELP("Trigger to draw the 2D rectangle")),
            InputPortConfig<float>("ScreenX", 10.0f, _HELP("X postion in screen space")),
            InputPortConfig<float>("ScreenY", 10.0f, _HELP("Y postion in screen space")),
            InputPortConfig<float>("Width", 10.0f, _HELP("Width in screen space")),
            InputPortConfig<float>("Height", 10.0f, _HELP("Height in screen space")),
            InputPortConfig<bool>("Centered", false, _HELP("Center placement. false wil default to top left placement.")),
            InputPortConfig<Vec3>("clr_Color", Vec3(1.0f, 1.0f, 1.0f), _HELP("Color of the rectangle")),
            InputPortConfig<float>("Opacity", 1.0f, _HELP("Transparency of the rectangle. Range from 0 to 1")),
            InputPortConfig<float>("Time", _HELP("How much time the rectangle will be visible")),
            { 0 }
        };

        config.sDescription = _HELP("Draws a 2d rectangle for debugging purposes");
        config.pInputPorts = in_config;
        config.SetCategory(EFLN_DEBUG);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
#if !defined(_RELEASE)
        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputDraw))
            {
                IPersistentDebug* persistentDebug = CCryAction::GetCryAction()->GetIPersistentDebug();
                if (persistentDebug)
                {
                    Vec2 position(GetPortFloat(activationInfo, InputScreenX), GetPortFloat(activationInfo, InputScreenY));
                    Vec2 size(GetPortFloat(activationInfo, InputWidth), GetPortFloat(activationInfo, InputWidth));
                    const float time = GetPortFloat(activationInfo, InputTime);
                    const ColorF color = ColorF(GetPortVec3(activationInfo, InputColor), GetPortFloat(activationInfo, InputOpacity));

                    if (GetPortBool(activationInfo, InputCentered))
                    {
                        position -= (size * 0.5f);
                    }

                    persistentDebug->Begin("FG_Rectangle2d", false);
                    persistentDebug->Add2DRect(position.x, position.y, size.x, size.y, color, time);
                }
            }
        }
#endif
    }

    // ~IFlowNode
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node rendering 2d debug line
class CFlowNode_Draw2dLine
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    //! Input port enum list
    enum Inputs
    {
        InputDraw = 0,
        InputStartX,
        InputStartY,
        InputEndX,
        InputEndY,
        InputColor,
        InputOpacity,
        InputTime,
    };

    //! "Default" constructor for the debug draw 2d line flow graph node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowNode_Draw2dLine(SActivationInfo* activationInfo)
        : CFlowBaseNode()
    {
    }

    //! Destructor, does nothing
    ~CFlowNode_Draw2dLine() override {}

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    // IFlowNode

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<SFlowSystemVoid>("Draw", _HELP("Trigger to draw the 2D rectangle")),
            InputPortConfig<float>("StartX", 0.0f, _HELP("Staring x point in screen space")),
            InputPortConfig<float>("StartY", 0.0f, _HELP("Staring y point in screen space")),
            InputPortConfig<float>("EndX", 10.0f, _HELP("Ending x point in screen space")),
            InputPortConfig<float>("EndY", 10.0f, _HELP("Ending y point in screen space")),
            InputPortConfig<Vec3>("clr_Color", Vec3(1.0f, 1.0f, 1.0f), _HELP("Color of the line")),
            InputPortConfig<float>("Opacity", 1.0f, _HELP("Transparency of the line. Range from 0 to 1")),
            InputPortConfig<float>("Time", _HELP("How much time the line will be visible")),
            { 0 }
        };

        config.sDescription = _HELP("Draws a 2d line for debugging purposes");
        config.pInputPorts = in_config;
        config.SetCategory(EFLN_DEBUG);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
#if !defined(_RELEASE)
        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputDraw))
            {
                IPersistentDebug* persistentDebug = CCryAction::GetCryAction()->GetIPersistentDebug();
                if (persistentDebug)
                {
                    const Vec2 start(GetPortFloat(activationInfo, InputStartX), GetPortFloat(activationInfo, InputStartY));
                    const Vec2 end(GetPortFloat(activationInfo, InputEndX), GetPortFloat(activationInfo, InputEndY));
                    const float time = GetPortFloat(activationInfo, InputTime);
                    const ColorF color = ColorF(GetPortVec3(activationInfo, InputColor), GetPortFloat(activationInfo, InputOpacity));

                    persistentDebug->Begin("FG_Line2d", false);
                    persistentDebug->Add2DLine(start.x, start.y, end.x, end.y, color, time);
                }
            }
        }
#endif
    }

    // ~IFlowNode
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node rendering 2d debug text
class CFlowNode_Draw2dText
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    //! Input port enum list
    enum Inputs
    {
        InputDraw = 0,
        InputText,
        InputScreenX,
        InputScreenY,
        InputFontSize,
        InputColor,
        InputOpacity,
        InputTime,
    };

    //! "Default" constructor for the debug draw 2d text flow graph node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowNode_Draw2dText(SActivationInfo* activationInfo)
        : CFlowBaseNode()
    {
    }

    //! Destructor, does nothing
    ~CFlowNode_Draw2dText() override {}

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    // IFlowNode

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<SFlowSystemVoid>("Draw", _HELP("Trigger to draw the 2D text")),
            InputPortConfig<string>("Text", _HELP("Text that should be rendered to the screen")),
            InputPortConfig<float>("ScreenX", 10.0f, _HELP("X postion in screen space")),
            InputPortConfig<float>("ScreenY", 10.0f, _HELP("Y postion in screen space")),
            InputPortConfig<float>("FontSize", 16.0f, _HELP("Size of the font in pixels")),
            InputPortConfig<Vec3>("clr_Color", Vec3(1.0f, 1.0f, 1.0f), _HELP("Color of the line")),
            InputPortConfig<float>("Opacity", 1.0f, _HELP("Transparency of the line. Range from 0 to 1")),
            InputPortConfig<float>("Time", _HELP("How much time the line will be visible")),
            { 0 }
        };

        config.sDescription = _HELP("Draws a 2d rectangle for debugging purposes");
        config.pInputPorts = in_config;
        config.SetCategory(EFLN_DEBUG);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
#if !defined(_RELEASE)
        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputDraw))
            {
                IPersistentDebug* persistentDebug = CCryAction::GetCryAction()->GetIPersistentDebug();
                if (persistentDebug)
                {
                    const string text = GetPortString(activationInfo, InputText);
                    const Vec2 position(GetPortFloat(activationInfo, InputScreenX), GetPortFloat(activationInfo, InputScreenY));
                    const ColorF color = ColorF(GetPortVec3(activationInfo, InputColor), GetPortFloat(activationInfo, InputOpacity));
                    const float fontSize = GetPortFloat(activationInfo, InputFontSize);
                    const float time = GetPortFloat(activationInfo, InputTime);

                    persistentDebug->Begin("FG_Line2d", false);
                    persistentDebug->AddText(position.x, position.y, fontSize, color, time, "%s", text.c_str());
                }
            }
        }
#endif
    }

    // ~IFlowNode
};


REGISTER_FLOW_NODE("Debug:Draw2d:Circle", CFlowNode_Draw2dCircle);
REGISTER_FLOW_NODE("Debug:Draw2d:Rectangle", CFlowNode_Draw2dRect);
REGISTER_FLOW_NODE("Debug:Draw2d:Line", CFlowNode_Draw2dLine);
REGISTER_FLOW_NODE("Debug:Draw2d:Text", CFlowNode_Draw2dText);
