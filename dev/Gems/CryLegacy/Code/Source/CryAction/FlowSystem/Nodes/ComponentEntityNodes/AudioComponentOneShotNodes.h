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

#include <ISystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IFlowSystem.h>

/*
This node executes the indicated audio trigger as a one shot on the audio component attached to
the entity on this node
*/
class AudioComponentExecuteOneShotNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    //////////////////////////////////////////////////////////////////////////
    // AudioComponentExecuteOneShotNode
    //////////////////////////////////////////////////////////////////////////

public:

    // This constructor only exists for compatibility with construction code outside of this node, it does not need the Activation info
    AudioComponentExecuteOneShotNode(SActivationInfo* /*activationInformation*/)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new AudioComponentExecuteOneShotNode(activationInformation);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

private:

    enum InputPorts
    {
        Activate = 0,
        TriggerName
    };
};

/*
This node stops the indicated audio trigger as a one shot on the audio component attached to
the entity on this node
*/
class AudioComponentStopOneShotNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    //////////////////////////////////////////////////////////////////////////
    // AudioComponentStopOneShotNode
    //////////////////////////////////////////////////////////////////////////

public:

    // This constructor only exists for compatibility with construction code outside of this node, it does not need the Activation info
    AudioComponentStopOneShotNode(SActivationInfo* /*activationInformation*/)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new AudioComponentStopOneShotNode(activationInformation);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

private:

    enum InputPorts
    {
        Activate = 0,
        TriggerName
    };
};
