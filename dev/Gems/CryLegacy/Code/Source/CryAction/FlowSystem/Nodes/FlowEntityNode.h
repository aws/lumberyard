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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWENTITYNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWENTITYNODE_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "IEntityPoolManager.h"
#include <MathConversion.h>

//////////////////////////////////////////////////////////////////////////
class CFlowEntityClass
    : public IFlowNodeFactory
{
public:
    CFlowEntityClass(IEntityClass* pEntityClass);
    ~CFlowEntityClass();
    virtual void AddRef() { m_nRefCount++; }
    virtual void Release()
    {
        if (0 == --m_nRefCount)
        {
            delete this;
        }
    }
    virtual IFlowNodePtr Create(IFlowNode::SActivationInfo*);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        SIZER_SUBCOMPONENT_NAME(s, "CFlowEntityClass");
        s->AddObject(this, sizeof(*this));
        s->AddObject(m_inputs);
        s->AddObject(m_outputs);
    }

    void GetConfiguration(SFlowNodeConfig&);

    const char* CopyStr(const char* src);
    void FreeStr(const char* src);

    size_t GetNumOutputs() { return m_outputs.size() - 1; }

    void Reset();

private:
    void GetInputsOutputs(IEntityClass* pEntityClass);
    friend class CFlowEntityNode;

    int m_nRefCount;
    //string m_classname;
    IEntityClass* m_pEntityClass;
    std::vector<SInputPortConfig> m_inputs;
    std::vector<SOutputPortConfig> m_outputs;
};

//////////////////////////////////////////////////////////////////////////
class CFlowEntityNodeBase
    : public CFlowBaseNode<eNCT_Instanced>
    , public IEntityEventListener
{
public:
    CFlowEntityNodeBase()
    {
        m_entityId = EntityId();
        m_event = ENTITY_EVENT_LAST;
    };
    ~CFlowEntityNodeBase()
    {
        UnregisterEvent(m_event);
    }

    // deliberately don't implement Clone(), make sure derived classes do.

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_SetEntityId)
        {
            return;
        }

        EntityId newEntityId = GetEntityId(pActInfo);

        if (m_entityId && newEntityId)
        {
            UnregisterEvent(m_event);
        }

        if (newEntityId)
        {
            m_entityId = newEntityId;
            RegisterEvent(m_event);
        }
    }

    // Return entity of this node.
    // Note: Will not work if the entity is pooled and not yet prepared.
    ILINE IEntity* GetEntity() const
    {
        return gEnv->pEntitySystem->GetEntity(m_entityId);
    }

    // Return entityId of this node (works with entity pools)
    FlowEntityId GetEntityId(SActivationInfo* pActInfo) const
    {
        assert(pActInfo);

        if (pActInfo->pEntity)
        {
            return FlowEntityId(pActInfo->pEntity->GetId());
        }
        else if (!IsLegacyEntityId(pActInfo->entityId))
        {
            return pActInfo->entityId;
        }

        const FlowEntityId graphEntityId = pActInfo->pGraph->GetEntityId(pActInfo->myID);
        if (gEnv->pEntitySystem->GetIEntityPoolManager()->IsEntityBookmarked(graphEntityId))
        {
            return graphEntityId;
        }

        return FlowEntityId();
    }

    void RegisterEvent(EEntityEvent event)
    {
        if (event != ENTITY_EVENT_LAST)
        {
            gEnv->pEntitySystem->AddEntityEventListener(m_entityId, event, this);
            gEnv->pEntitySystem->AddEntityEventListener(m_entityId, ENTITY_EVENT_DONE, this);
        }
    }
    void UnregisterEvent(EEntityEvent event)
    {
        if (m_entityId && event != ENTITY_EVENT_LAST)
        {
            IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
            if (pEntitySystem)
            {
                pEntitySystem->RemoveEntityEventListener(m_entityId, event, this);
                pEntitySystem->RemoveEntityEventListener(m_entityId, ENTITY_EVENT_DONE, this);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:
    EEntityEvent m_event; // This member must be set in derived class constructor.
    EntityId m_entityId;
};

//////////////////////////////////////////////////////////////////////////
// Flow node for entity.
//////////////////////////////////////////////////////////////////////////
class CFlowEntityNode
    : public CFlowEntityNodeBase
{
public:
    CFlowEntityNode(CFlowEntityClass* pClass, SActivationInfo* pActInfo);
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
    virtual void GetConfiguration(SFlowNodeConfig&);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
    virtual bool SerializeXML(SActivationInfo*, const XmlNodeRef&, bool);

    //////////////////////////////////////////////////////////////////////////
    // IEntityEventListener
    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event);
    //////////////////////////////////////////////////////////////////////////

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:
    void Serialize(SActivationInfo* pActInfo, TSerialize ser);

    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;
    _smart_ptr<CFlowEntityClass> m_pClass;
    int m_lastInitializeFrameId;
};


#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWENTITYNODE_H
