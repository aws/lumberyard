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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

class CMovieManager;

class CMovieInstance
{
public:
    void AddRef() { ++m_nRefs; }
    void Release();

private:
    int m_nRefs;
    CMovieManager* m_pManager;
};

class CMovieManager
{
public:
    void ReleaseInstance(CMovieInstance* pInstance)
    {
    }
};

void CMovieInstance::Release()
{
    if (0 == --m_nRefs)
    {
        m_pManager->ReleaseInstance(this);
    }
}

class CFlowNode_Movie
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_Movie(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_TimeOfDay = 0,
    };

    enum EOutputPorts
    {
        eOP_SunDirection = 0,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:
    void Reload(const char* filename);
};

// REGISTER_FLOW_NODE("Environment:Movie", CFlowNode_Movie);
