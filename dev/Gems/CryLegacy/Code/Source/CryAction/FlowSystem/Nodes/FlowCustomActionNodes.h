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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : All nodes related to the Custom Action flow graphs


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWCUSTOMACTIONNODES_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWCUSTOMACTIONNODES_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "IEntityPoolManager.h"
#include <ICustomActions.h>

// Forward declarations
struct ICustomAction;

//////////////////////////////////////////////////////////////////////////
// CustomAction:Control node.
// This node is used to control a custom action from a specific instance
//////////////////////////////////////////////////////////////////////////
class CFlowNode_CustomActionControl
    : public CFlowBaseNode<eNCT_Instanced>
    , ICustomActionListener
{
public:
    enum INPUTS
    {
        EIP_Start = 0,
        EIP_Succeed,
        EIP_SucceedWait,
        EIP_SucceedWaitComplete,
        EIP_Abort,
        EIP_EndSuccess,
        EIP_EndFailure,
    };

    enum OUTPUTS
    {
        EOP_Started = 0,
        EOP_Succeeded,
        EOP_SucceededWait,
        EOP_SucceededWaitComplete,
        EOP_Aborted,
        EOP_EndedSuccess,
        EOP_EndedFailure,
    };

    CFlowNode_CustomActionControl(SActivationInfo* pActInfo)
    {
    }

    ~CFlowNode_CustomActionControl();

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_CustomActionControl(pActInfo); }

    void GetConfiguration(SFlowNodeConfig& config);
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

    // ICustomActionListener
    virtual void OnCustomActionEvent(ECustomActionEvent event, ICustomAction& customAction);
    // ~ICustomActionListener

private:
    SActivationInfo m_actInfo;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWCUSTOMACTIONNODES_H
