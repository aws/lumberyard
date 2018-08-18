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

// Description : Some Inspector filters


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWFILTERS_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWFILTERS_H
#pragma once

#include "IFlowSystem.h"

#undef GetClassName

typedef enum
{
    eFF_AIAction,
    eFF_Entity,
    eFF_Node
} TFlowInspectorFilter;


class CFlowFilterBase
    : public IFlowGraphInspector::IFilter
{
protected:
    CFlowFilterBase(TFlowInspectorFilter type)
        : m_type(type)
        , m_refs(0) {}

public:
    virtual void AddRef()
    {
        ++m_refs;
    }

    virtual void Release()
    {
        if (--m_refs <= 0)
        {
            delete this;
        }
    }

    virtual TFlowInspectorFilter GetTypeId() const
    {
        return m_type;
    }

    virtual void AddFilter(IFlowGraphInspector::IFilterPtr pFilter)
    {
        stl::push_back_unique(m_filters, pFilter);
    }

    virtual void RemoveFilter(IFlowGraphInspector::IFilterPtr pFilter)
    {
        stl::find_and_erase(m_filters, pFilter);
    }

    virtual const IFlowGraphInspector::IFilter_AutoArray& GetFilters() const
    {
        return m_filters;
    }

protected:
    TFlowInspectorFilter m_type;
    int m_refs;
    IFlowGraphInspector::IFilter_AutoArray m_filters;
public:
    // quick/dirty factory by name
    // but actual CFlowFilter instances can also be instantiated explicetly
    static CFlowFilterBase* CreateFilter(const char* filterType);
};

class CFlowFilterEntity
    : public CFlowFilterBase
{
public:
    static const char* GetClassName() { return "Entity"; }

    // filter lets only pass flows which happen in this entity's graph
    CFlowFilterEntity ();
    void SetEntity (FlowEntityId entityId) { m_entityId = entityId; }
    FlowEntityId GetEntity() const { return m_entityId; }

    virtual EFilterResult Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to);

protected:
    FlowEntityId m_entityId;
};

class CFlowFilterAIAction
    : public CFlowFilterBase
{
public:
    static const char* GetClassName() { return "AIAction"; }

    // filter lets only pass AIAction flows for a specific AIAction
    // if ActionName is empty works on all AIActions
    // optionally works on specific user class, object class, user ID, objectID
    // all can be mixed
    CFlowFilterAIAction ();
    void SetAction(const string& actionName) { m_actionName = actionName; }
    const string& GetAction() const { return m_actionName; }

    void SetParams(const string& userClass, const string& objectClass, FlowEntityId userId = FlowEntityId(), FlowEntityId objectId = FlowEntityId());
    void GetParams(string& userClass, FlowEntityId& userId, string& objectClass, FlowEntityId& objectId) const;

    virtual EFilterResult Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to);

protected:
    string m_actionName;
    string m_userClass;
    string m_objectClass;
    FlowEntityId m_userId;    // will be 0 if we run on all entity classes
    FlowEntityId m_objectId;  // will be 0 if we run on all entity classes
};

class CFlowFilterNode
    : public CFlowFilterBase
{
public:
    static const char* GetClassName() { return "Node"; }

    // filter lets only pass flows for nodes of a specific type and node id
    CFlowFilterNode ();
    void SetNodeType(const string& nodeTypeName) { m_nodeType = nodeTypeName; }
    const string& GetNodeType() const { return m_nodeType; }

    void SetParams(const TFlowNodeId id) {  m_nodeId = id; }
    void GetParams(TFlowNodeId& id) const { id = m_nodeId; }

    virtual EFilterResult Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to);

protected:
    string m_nodeType;
    TFlowNodeId m_nodeId;
};


#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWFILTERS_H
