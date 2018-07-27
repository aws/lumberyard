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

#include <platform.h>
#include <IGameFramework.h>
#include "Nodes/FlowBaseNode.h"

CAutoRegFlowNodeBase* CAutoRegFlowNodeBase::m_pFirst = nullptr;
CAutoRegFlowNodeBase* CAutoRegFlowNodeBase::m_pLast = nullptr;

void RegisterExternalFlowNodes()
{
#ifndef AZ_MONOLITHIC_BUILD
    if (IFlowSystem * flowSystem = gEnv->pGame->GetIGameFramework()->GetIFlowSystem())
    {
        CAutoRegFlowNodeBase* flowNodeFactory = CAutoRegFlowNodeBase::m_pFirst;
        while (flowNodeFactory)
        {
            flowSystem->RegisterType(flowNodeFactory->m_sClassName, flowNodeFactory);
            flowNodeFactory = flowNodeFactory->m_pNext;
        }
    }
#endif // AZ_MONOLITHIC_BUILD
}
