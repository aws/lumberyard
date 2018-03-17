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
#include "LyShineExamples_precompiled.h"
#include <IFlowSystem.h>
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "UiDynamicContentDatabase.h"
#include "LyShineExamplesInternalBus.h"

namespace
{
    const string g_uiDynamicContentDBGetNumColorsNodePath = "UiDynamicContentDB:GetNumColors";
    const string g_uiDynamicContentDBGetColorNodePath = "UiDynamicContentDB:GetColor";
    const string g_uiDynamicContentDBGetColorNameNodePath = "UiDynamicContentDB:GetColorName";
    const string g_uiDynamicContentDBGetColorPriceNodePath = "UiDynamicContentDB:GetColorPrice";
    const string g_uiDynamicContentDBRefreshNodePath = "UiDynamicContentDB:Refresh";
}

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // CFlowUiDynamicContentDBGetNumColorsNode class
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Flow Graph node for getting the number of available colors
    class CFlowUiDynamicContentDBGetNumColorsNode
        : public CFlowBaseNode < eNCT_Instanced >
    {
    public:
        CFlowUiDynamicContentDBGetNumColorsNode(SActivationInfo* activationInfo)
            : CFlowBaseNode() {}

        //-- IFlowNode --

        IFlowNodePtr Clone(SActivationInfo* activationInfo) override
        {
            return new CFlowUiDynamicContentDBGetNumColorsNode(activationInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Trigger this node")),
                InputPortConfig<int>("ColorType", _HELP("Color type"), 0, _UICONFIG("enum_int:Free=0, Paid=1")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<int>("NumColors", _HELP("Outputs the number of colors")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Get number of colors");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
            {
                int colorTypeInt = GetPortInt(activationInfo, InputPortColorType);                               
                UiDynamicContentDatabaseInterface::ColorType colorType = static_cast<UiDynamicContentDatabaseInterface::ColorType>(colorTypeInt);

                UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
                EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);

                // Trigger output
                ActivateOutput(activationInfo, OutputPortNumColors, uiDynamicContentDB->GetNumColors(colorType));
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
            InputPortColorType
        };

        enum OutputPorts
        {
            OutputPortNumColors = 0
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // CFlowUiDynamicContentDBGetColorNode class
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Flow Graph node for getting a color
    class CFlowUiDynamicContentDBGetColorNode
        : public CFlowBaseNode < eNCT_Instanced >
    {
    public:
        CFlowUiDynamicContentDBGetColorNode(SActivationInfo* activationInfo)
            : CFlowBaseNode() {}

        //-- IFlowNode --

        IFlowNodePtr Clone(SActivationInfo* activationInfo) override
        {
            return new CFlowUiDynamicContentDBGetColorNode(activationInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Trigger this node")),
                InputPortConfig<int>("ColorType", _HELP("Color type"), 0, _UICONFIG("enum_int:Free=0, Paid=1")),
                InputPortConfig<int>("Index", 0, _HELP("The color index")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<Vec3>("Color", _HELP("Outputs a color")),
                OutputPortConfig<bool>("ColorExists", _HELP("Whether a color at the specified index exists")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Get color");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
            {
                int colorTypeInt = GetPortInt(activationInfo, InputPortColorType);
                UiDynamicContentDatabaseInterface::ColorType colorType = static_cast<UiDynamicContentDatabaseInterface::ColorType>(colorTypeInt);

                int index = GetPortInt(activationInfo, InputPortIndex);

                UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
                EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);

                int numColors = uiDynamicContentDB->GetNumColors(colorType);
                if (index < numColors)
                {
                    // Trigger output
                    AZ::Color color = uiDynamicContentDB->GetColor(colorType, index);
                    Vec3 colorVec = Vec3(color.GetR(), color.GetG(), color.GetB());
                    colorVec *= 255.0f;
                    ActivateOutput(activationInfo, OutputPortColor, colorVec);
                    ActivateOutput(activationInfo, OutputPortColorExists, true);
                }
                else
                {
                    // Trigger output
                    ActivateOutput(activationInfo, OutputPortColorExists, false);
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
            InputPortColorType,
            InputPortIndex
        };

        enum OutputPorts
        {
            OutputPortColor = 0,
            OutputPortColorExists
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // CFlowUiDynamicContentDBGetColorNameNode class
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Flow Graph node for getting a color
    class CFlowUiDynamicContentDBGetColorNameNode
        : public CFlowBaseNode < eNCT_Instanced >
    {
    public:
        CFlowUiDynamicContentDBGetColorNameNode(SActivationInfo* activationInfo)
            : CFlowBaseNode() {}

        //-- IFlowNode --

        IFlowNodePtr Clone(SActivationInfo* activationInfo) override
        {
            return new CFlowUiDynamicContentDBGetColorNameNode(activationInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Trigger this node")),
                InputPortConfig<int>("ColorType", _HELP("Color type"), 0, _UICONFIG("enum_int:Free=0, Paid=1")),
                InputPortConfig<int>("Index", 0, _HELP("The color index")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<string>("ColorName", _HELP("Outputs the name of the color")),
                OutputPortConfig<bool>("ColorExists", _HELP("Whether a color at the specified index exists")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Get color name");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
            {
                int colorTypeInt = GetPortInt(activationInfo, InputPortColorType);
                UiDynamicContentDatabaseInterface::ColorType colorType = static_cast<UiDynamicContentDatabaseInterface::ColorType>(colorTypeInt);

                int index = GetPortInt(activationInfo, InputPortIndex);

                UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
                EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);

                int numColors = uiDynamicContentDB->GetNumColors(colorType);
                if (index < numColors)
                {
                    // Trigger output
                    string colorName = uiDynamicContentDB->GetColorName(colorType, index).c_str();
                    ActivateOutput(activationInfo, OutputPortColorName, colorName);
                    ActivateOutput(activationInfo, OutputPortColorExists, true);
                }
                else
                {
                    // Trigger output
                    ActivateOutput(activationInfo, OutputPortColorExists, false);
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
            InputPortColorType,
            InputPortIndex
        };

        enum OutputPorts
        {
            OutputPortColorName = 0,
            OutputPortColorExists
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // CFlowUiDynamicContentDBGetColorPriceNode class
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Flow Graph node for getting the price of a color
    class CFlowUiDynamicContentDBGetColorPriceNode
        : public CFlowBaseNode < eNCT_Instanced >
    {
    public:
        CFlowUiDynamicContentDBGetColorPriceNode(SActivationInfo* activationInfo)
            : CFlowBaseNode() {}

        //-- IFlowNode --

        IFlowNodePtr Clone(SActivationInfo* activationInfo) override
        {
            return new CFlowUiDynamicContentDBGetColorPriceNode(activationInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Trigger this node")),
                InputPortConfig<int>("ColorType", _HELP("Color type"), 0, _UICONFIG("enum_int:Free=0, Paid=1")),
                InputPortConfig<int>("Index", 0, _HELP("The color index")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<string>("ColorPrice", _HELP("Outputs the price of the color")),
                OutputPortConfig<bool>("ColorExists", _HELP("Whether a color at the specified index exists")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Get color price");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
            {
                int colorTypeInt = GetPortInt(activationInfo, InputPortColorType);
                UiDynamicContentDatabaseInterface::ColorType colorType = static_cast<UiDynamicContentDatabaseInterface::ColorType>(colorTypeInt);

                int index = GetPortInt(activationInfo, InputPortIndex);

                UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
                EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);

                int numColors = uiDynamicContentDB->GetNumColors(colorType);
                if (index < numColors)
                {
                    // Trigger output
                    string colorPrice = uiDynamicContentDB->GetColorPrice(colorType, index).c_str();
                    ActivateOutput(activationInfo, OutputPortColorPrice, colorPrice);
                    ActivateOutput(activationInfo, OutputPortColorExists, true);
                }
                else
                {
                    // Trigger output
                    ActivateOutput(activationInfo, OutputPortColorExists, false);
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
            InputPortColorType,
            InputPortIndex
        };

        enum OutputPorts
        {
            OutputPortColorPrice = 0,
            OutputPortColorExists
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // CFlowUiDynamicContentDBRefreshNode class
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Flow Graph node for getting the number of available colors
    class CFlowUiDynamicContentDBRefreshNode
        : public CFlowBaseNode < eNCT_Instanced >
    {
    public:
        CFlowUiDynamicContentDBRefreshNode(SActivationInfo* activationInfo)
            : CFlowBaseNode() {}

        //-- IFlowNode --

        IFlowNodePtr Clone(SActivationInfo* activationInfo) override
        {
            return new CFlowUiDynamicContentDBRefreshNode(activationInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Trigger this node")),
                InputPortConfig<int>("ColorType", _HELP("Color type"), 0, _UICONFIG("enum_int:Free=0, Paid=1")),
                InputPortConfig<string>("FileName", _HELP("Name of json file")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("OnRefresh", _HELP("Sends a signal when the dynamic content database has been refreshed")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Refresh the dynamic content database");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate) && gEnv->pLyShine)
            {
                int colorTypeInt = GetPortInt(activationInfo, InputPortColorType);
                UiDynamicContentDatabaseInterface::ColorType colorType = static_cast<UiDynamicContentDatabaseInterface::ColorType>(colorTypeInt);

                string fileName = GetPortString(activationInfo, InputPortFileName);
                UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
                EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);
                uiDynamicContentDB->Refresh(colorType, fileName.c_str());

                // Trigger output
                ActivateOutput(activationInfo, OutputPortOnRefresh, 0);
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
            InputPortColorType,
            InputPortFileName
        };

        enum OutputPorts
        {
            OutputPortOnRefresh = 0
        };
    };

    REGISTER_FLOW_NODE(g_uiDynamicContentDBGetNumColorsNodePath, CFlowUiDynamicContentDBGetNumColorsNode);
    REGISTER_FLOW_NODE(g_uiDynamicContentDBGetColorNodePath, CFlowUiDynamicContentDBGetColorNode);
    REGISTER_FLOW_NODE(g_uiDynamicContentDBGetColorNameNodePath, CFlowUiDynamicContentDBGetColorNameNode);
    REGISTER_FLOW_NODE(g_uiDynamicContentDBGetColorPriceNodePath, CFlowUiDynamicContentDBGetColorPriceNode);
    REGISTER_FLOW_NODE(g_uiDynamicContentDBRefreshNodePath, CFlowUiDynamicContentDBRefreshNode);
};
