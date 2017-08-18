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
#ifndef CRYINCLUDE_GINE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTFLOWGRAPH_H
#define CRYINCLUDE_GINE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTFLOWGRAPH_H
#pragma once

#include "ISerialize.h"
#include "Components/IComponentFlowGraph.h"

//! Implementation of the entity's flow graph component.
//! This component allows the entity to host a reference to a flow graph.
class CComponentFlowGraph
    : public IComponentFlowGraph
{
public:
    CComponentFlowGraph();
    ~CComponentFlowGraph() override;

    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void Done();
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentFlowGraph interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetFlowGraph(IFlowGraph* pFlowGraph);
    virtual IFlowGraph* GetFlowGraph();
    virtual void AddEventListener(IEntityEventListener* pListener);
    virtual void RemoveEventListener(IEntityEventListener* pListener);
    //////////////////////////////////////////////////////////////////////////

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void OnMove();

private:
    //////////////////////////////////////////////////////////////////////////
    // Private member variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;
    IFlowGraph* m_pFlowGraph;


    typedef std::list<IEntityEventListener*> Listeners;
    Listeners m_listeners;
};

#endif // CRYINCLUDE_GINE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTFLOWGRAPH_H