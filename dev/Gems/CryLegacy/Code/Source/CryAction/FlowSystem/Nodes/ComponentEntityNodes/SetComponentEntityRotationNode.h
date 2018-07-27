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
#include <IFlowSystem.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

class ComponentEntitySetRotationNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    //////////////////////////////////////////////////////////////////////////
    // ComponentEntitySetRotationNode
    //////////////////////////////////////////////////////////////////////////

public:

    // This constructor only exists for compatibility with construction code outside of this node, it does not need the Activation info
    ComponentEntitySetRotationNode(SActivationInfo* /*activationInformation*/)
    {
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new ComponentEntitySetRotationNode(activationInformation);
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

    enum class CoordSys
    {
        Parent = 0,
        World
    };

private:

    enum InputPorts
    {
        Activate = 0,
        Rotation,
        CoordSystem
    };
};
