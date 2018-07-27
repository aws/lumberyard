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


#include "CryLegacy_precompiled.h"

#include "FlowFilters.h"
#include <IFlowSystem.h>
#include <IAIAction.h>
#include <IEntity.h>
#include <IEntityClass.h>

namespace
{
    std::map<string, CFlowFilterBase* (*)()>* g_pCreatorMap = NULL;
    template <typename T, typename Base>
    Base* Create() { return new T(); };

    std::map<string, CFlowFilterBase* (*)()>& GetCreatorMap()
    {
        if (!g_pCreatorMap)
        {
            g_pCreatorMap = new std::map<string, CFlowFilterBase* (*)()>();
            (*g_pCreatorMap)[CFlowFilterAIAction::GetClassName()] = &Create<CFlowFilterAIAction, CFlowFilterBase>;
            (*g_pCreatorMap)[CFlowFilterEntity::GetClassName()] = &Create<CFlowFilterEntity, CFlowFilterBase>;
            (*g_pCreatorMap)[CFlowFilterNode::GetClassName()] = &Create<CFlowFilterNode, CFlowFilterBase>;
        }
        return *g_pCreatorMap;
    }
}

/* static */ CFlowFilterBase*
CFlowFilterBase::CreateFilter(const char* filterType)
{
    std::map<string, CFlowFilterBase* (*)()>::iterator iter = GetCreatorMap().find(filterType);
    if (iter != GetCreatorMap().end())
    {
        return ((*iter).second)();
    }
    return 0;
}

CFlowFilterEntity::CFlowFilterEntity()
    : CFlowFilterBase(eFF_Entity)
    , m_entityId(0)
{
}

/* virtual */ IFlowGraphInspector::IFilter::EFilterResult
CFlowFilterEntity::Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to)
{
    if (m_entityId == 0 || m_entityId == pGraph->GetGraphEntity(0) || m_entityId == pGraph->GetGraphEntity(1))
    {
        return eFR_Pass;
    }

    return eFR_Block;
}

CFlowFilterAIAction::CFlowFilterAIAction ()
    : CFlowFilterBase(eFF_AIAction)
{
    m_userId = 0;
    m_objectId = 0;
}

void
CFlowFilterAIAction::SetParams(const string& userClass, const string& objectClass, FlowEntityId userId, FlowEntityId objectId)
{
    m_userClass   = userClass;
    m_userId      = userId;
    m_objectClass = objectClass;
    m_objectId    = objectId;
}

void
CFlowFilterAIAction::GetParams(string& userClass, FlowEntityId& userId, string& objectClass, FlowEntityId& objectId) const
{
    userClass   = m_userClass;
    userId      = m_userId;
    objectClass = m_objectClass;
    objectId    = m_objectId;
}


/* virtual */ IFlowGraphInspector::IFilter::EFilterResult
CFlowFilterAIAction::Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to)
{
    IAIAction* pAction = pGraph->GetAIAction();

    // no AI action
    if (!pAction)
    {
        return eFR_Block;
    }

    // it's an AI action, but not the one we're interested in -> block
    if (m_actionName.empty() == false && m_actionName.compare(pAction->GetName()) != 0)
    {
        return eFR_Block;
    }

    IEntity* pUser   = pAction->GetUserEntity();
    // check for specific user entity
    if (m_userId > 0)
    {
        if (!pUser)
        {
            return eFR_Block;         // there is no user at all
        }
        if (pUser->GetId() != m_userId)
        {
            return eFR_Block;                             // user entity does not match
        }
    }
    // otherwise check for the user's class if applicable
    else if (!m_userClass.empty())
    {
        if (!pUser)
        {
            return eFR_Block;         // no user at all
        }
        if (m_userClass.compare(pUser->GetClass()->GetName()) != 0)
        {
            return eFR_Block;
        }
    }

    IEntity* pObject = pAction->GetObjectEntity();
    // check for specific object entity
    if (m_objectId > 0)
    {
        if (!pObject)
        {
            return eFR_Block;
        }
        if (pObject->GetId() != m_objectId)
        {
            return eFR_Block;
        }
    }
    // otherwise check for the object's class if applicable
    else if (!m_objectClass.empty())
    {
        if (!pObject)
        {
            return eFR_Block;           // no object at all
        }
        if (m_objectClass.compare(pObject->GetClass()->GetName()) != 0)
        {
            return eFR_Block;
        }
    }

    // it passed all tests and no subfilters -> great
    if (m_filters.empty())
    {
        return eFR_Pass;
    }

    // ask sub-filters: if anyone says yes, so do we (basically ORing the sub-filter results)
    // otherwise we block
    IFilter::EFilterResult res = eFR_Block;
    IFlowGraphInspector::IFilter_AutoArray::iterator iter (m_filters.begin());
    IFlowGraphInspector::IFilter_AutoArray::iterator end (m_filters.end());
    while (iter != end)
    {
        res = (*iter)->Apply(pGraph, from, to);
        if (res == eFR_Pass)
        {
            break;
        }
        ++iter;
    }
    return res;
}

CFlowFilterNode::CFlowFilterNode ()
    : CFlowFilterBase(eFF_Node)
{
    m_nodeId = InvalidFlowNodeId;
}

/* virtual */ IFlowGraphInspector::IFilter::EFilterResult
CFlowFilterNode::Apply (IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to)
{
    if (from.node == m_nodeId || to.node == m_nodeId)
    {
        if (m_nodeType.compare(pGraph->GetNodeTypeName(m_nodeId)) != 0)
        {
            return eFR_Block;
        }
    }
    return eFR_Pass;
}