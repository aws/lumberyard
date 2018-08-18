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
#include "FlowUIButtonNodes.h"
#include <LyShine/ILyShine.h>
#include <LyShine/IUiCanvas.h>

namespace
{
    const string NODE_PATH = "UI:Component:Button:Event";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUIButtonEventNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------------------
CFlowUIButtonEventNode::CFlowUIButtonEventNode(SActivationInfo* activationInfo)
    : CFlowBaseNode()
    , IUiButtonListener()
    , m_actInfo(*activationInfo)
    , m_button(nullptr)
{
}

//-------------------------------------------------------------------------------------------------------------
CFlowUIButtonEventNode::~CFlowUIButtonEventNode()
{
    UnregisterListener();
}

//-- IFlowNode --

//-------------------------------------------------------------------------------------------------------------
IFlowNodePtr CFlowUIButtonEventNode::Clone(SActivationInfo* activationInfo)
{
    return new CFlowUIButtonEventNode(activationInfo);
}

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig<int>("ElementID", 0, _HELP("The unique ID of the button's element")),
        {0}
    };

    static const SOutputPortConfig outputs[] =
    {
        OutputPortConfig_Void("OnClick", _HELP("Sends a signal when the button is clicked")),
        {0}
    };

    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Triggers output when events are received from the specified button");
    config.SetCategory(EFLN_APPROVED);
}

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo)
{
    m_actInfo = *activationInfo;

    if (eFE_Initialize == event ||
        eFE_Activate == event)
    {
        // Get the canvas
        auto canvas = gEnv->pLyShine->GetCanvas();
        if (!canvas)
        {
            CryLogAlways("WARNING: Flow Graph node %s couldn't find UI canvas\n", NODE_PATH.c_str());
            return;
        }

        // Get the element
        int elementID = GetPortInt(activationInfo, INPUT_ELEMENT_ID);
        auto element = canvas->FindElementById(elementID);
        if (!element)
        {
            CryLogAlways("WARNING: Flow Graph node %s couldn't find UI element with ID: %d\n", NODE_PATH.c_str(), elementID);
            UnregisterListener();
            return;
        }

        // Get the button component
        auto interactableComponent = element->GetInteractableComponent();
        if (!interactableComponent ||
            strcmp(interactableComponent->GetComponentTypeName(), "Button") != 0)
        {
            CryLogAlways("WARNING: Flow Graph node %s couldn't find a button component on UI element with ID: %d\n", NODE_PATH.c_str(), elementID);
            UnregisterListener();
            return;
        }

        auto button = static_cast<IUiButtonComponent*>(interactableComponent);
        RegisterListener(button);
    }
    if (eFE_DisconnectInputPort == event)
    {
        UnregisterListener();
    }
}

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::GetMemoryUsage(ICrySizer* sizer) const
{
    sizer->Add(*this);
}

//-- ~IFlowNode --

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::OnClick(IUiButtonComponent* button)
{
    ActivateOutput(&m_actInfo, OUTPUT_ON_CLICK, 0);
}

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::RegisterListener(IUiButtonComponent* button)
{
    if (IsRegistered())
    {
        if (m_button == button)
        {
            // Already registered for this button's events
            return;
        }
        else
        {
            UnregisterListener();
        }
    }

    // Start listening for this button's events
    button->AddListener(this);
    m_button = button;
}

//-------------------------------------------------------------------------------------------------------------
void CFlowUIButtonEventNode::UnregisterListener()
{
    if (IsRegistered())
    {
        // Stop listening for this button's events
        m_button->RemoveListener(this);
        m_button = nullptr;
    }
}

//-------------------------------------------------------------------------------------------------------------
bool CFlowUIButtonEventNode::IsRegistered() const
{
    return m_button ? true : false;
}

REGISTER_FLOW_NODE(NODE_PATH, CFlowUIButtonEventNode);