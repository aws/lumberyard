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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWLOGNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWLOGNODE_H
#pragma once

#include "IFlowSystem.h"

class CFlowLogNode
    : public IFlowNode
{
public:
    CFlowLogNode();

    // IFlowNode
    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Clone(SActivationInfo*);
    virtual void GetConfiguration(SFlowNodeConfig&);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*);
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef&, bool);
    virtual void Serialize(SActivationInfo*, TSerialize ser);
    virtual void PostSerialize(SActivationInfo*){}

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
    // ~IFlowNode

private:
    int m_refs;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWLOGNODE_H
