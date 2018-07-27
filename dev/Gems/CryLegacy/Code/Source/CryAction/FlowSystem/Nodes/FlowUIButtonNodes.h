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

// Includes
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <LyShine/IUiButtonComponent.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// FlowUIButtonEventNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for catching a specified UI button's events
class CFlowUIButtonEventNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IUiButtonListener
{
public:
    //! Input node array order
    enum INPUTS
    {
        INPUT_ELEMENT_ID = 0,
    };

    //! Output node array order
    enum OUTPUTS
    {
        OUTPUT_ON_CLICK = 0,
    };

    //! Constructor for a UI button event node
    //! \param activationInfo   The basic information needed for initializing a flowgraph node
    CFlowUIButtonEventNode(SActivationInfo* activationInfo);

    //! Removes the event listener from the associated button
    ~CFlowUIButtonEventNode() override;

    //-- IFlowNode --

    //! Clones the flowgraph node.  Clone is called instead of creating a new node if the node type is set to eNCT_Cloned
    //! \param activationInfo       The activation info from the flowgraph node that is being cloned from
    //! \return                     The newly cloned flowgraph node
    IFlowNodePtr Clone(SActivationInfo* activationInfo) override;

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override;

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override;

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override;

    //-- ~IFlowNode --

    //-- IUiButtonListener --

    //! Called when the button this node has registered with gets clicked
    void OnClick(IUiButtonComponent* button) override;

    //-- ~IUiButtonListener --

protected:
    //! Register with a button to receive its events
    void RegisterListener(IUiButtonComponent* button);

    //! Unregister from receiving button events
    void UnregisterListener();

    //! Check to see if this node is currently registered to receive events
    bool IsRegistered() const;

private:
    //!< A copy of the activation info used for construction
    SActivationInfo m_actInfo;

    //! The button this node is registered with, if any
    IUiButtonComponent* m_button;
};
