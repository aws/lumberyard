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
#include "CryLegacy_precompiled.h"
#include "ComponentFlowGraph.h"
#include "Entity.h"
#include <IFlowSystem.h>
#include "ISerialize.h"
#include "Components/IComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentFlowGraph, IComponentFlowGraph)

//////////////////////////////////////////////////////////////////////////
CComponentFlowGraph::CComponentFlowGraph()
{
    m_pEntity = NULL;
    m_pFlowGraph = 0;
}

//////////////////////////////////////////////////////////////////////////
CComponentFlowGraph::~CComponentFlowGraph()
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentFlowGraph>(SerializationOrder::FlowGraph, *this, &CComponentFlowGraph::Serialize, &CComponentFlowGraph::SerializeXML, &CComponentFlowGraph::NeedSerialize, &CComponentFlowGraph::GetSignature);
}

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::Done()
{
    if (m_pFlowGraph)
    {
        m_pFlowGraph->Release();
    }
    m_pFlowGraph = 0;
};

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::SetFlowGraph(IFlowGraph* pFlowGraph)
{
    if (m_pFlowGraph)
    {
        m_pFlowGraph->Release();
    }
    m_pFlowGraph = pFlowGraph;
    if (m_pFlowGraph)
    {
        m_pFlowGraph->AddRef();
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowGraph* CComponentFlowGraph::GetFlowGraph()
{
    return m_pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::ProcessEvent(SEntityEvent& event)
{
    // Assumes only 1 current listener can be deleted as a result of the event.
    Listeners::iterator next;
    Listeners::iterator it = m_listeners.begin();
    while (it != m_listeners.end())
    {
        next = it;
        ++next;
        (*it)->OnEntityEvent(m_pEntity, event);
        it = next;
    }
    // respond to entity activation/deactivation. enable/disable flowgraph
    switch (event.event)
    {
    case ENTITY_EVENT_INIT:
    case ENTITY_EVENT_DONE:
    {
        if (m_pFlowGraph)
        {
            const bool bActive = (event.event == ENTITY_EVENT_INIT) ? true : false;
            const bool bIsActive = m_pFlowGraph->IsActive();
            if (bActive != bIsActive)
            {
                m_pFlowGraph->SetActive(bActive);
            }
        }
    }
    break;
    case ENTITY_EVENT_POST_SERIALIZE:
        if (m_pFlowGraph)
        {
            m_pFlowGraph->PostSerialize();
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::AddEventListener(IEntityEventListener* pListener)
{
    // Does not check uniquiness due to performance reasons.
    m_listeners.push_back(pListener);
}

void CComponentFlowGraph::RemoveEventListener(IEntityEventListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

void CComponentFlowGraph::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    m_pEntity = pEntity;

    SAFE_RELEASE(m_pFlowGraph);

    m_listeners.clear();
}

void CComponentFlowGraph::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
    // don't serialize flowgraphs from the editor
    // (editor needs more information, so it saves them specially)
    if (gEnv->IsEditor())
    {
        return;
    }

    XmlNodeRef flowGraphNode;
    if (bLoading)
    {
        flowGraphNode = entityNode->findChild("FlowGraph");
        if (!flowGraphNode)
        {
            return;
        }

        // prevents loading of disabled flowgraphs for optimization
        // only in game mode, of course
#if 1
        bool bEnabled = true;
        if (flowGraphNode->getAttr("enabled", bEnabled) && bEnabled == false)
        {
            EntityWarning("[ComponentFlowGraph] Skipped loading of FG for Entity %d '%s'. FG was disabled on export.", m_pEntity->GetId(), m_pEntity->GetName());
            return;
        }
#endif

        enum EMultiPlayerType
        {
            eMPT_ServerOnly = 0,
            eMPT_ClientOnly,
            eMPT_ClientServer
        };

        EMultiPlayerType mpType = eMPT_ServerOnly;
        const char* mpTypeAttr = flowGraphNode->getAttr("MultiPlayer");

        if (mpTypeAttr)
        {
            if (strcmp("ClientOnly", mpTypeAttr) == 0)
            {
                mpType = eMPT_ClientOnly;
            }
            else if (strcmp("ClientServer", mpTypeAttr) == 0)
            {
                mpType = eMPT_ClientServer;
            }
        }

        const bool bIsServer = gEnv->bServer;
        const bool bIsClient = gEnv->IsClient();

        if (mpType == eMPT_ServerOnly && !bIsServer)
        {
            return;
        }
        else if (mpType == eMPT_ClientOnly && !bIsClient)
        {
            return;
        }

        if (gEnv->pFlowSystem)
        {
            SetFlowGraph(gEnv->pFlowSystem->CreateFlowGraph());
        }
    }
    else
    {
        flowGraphNode = entityNode->createNode("FlowGraph");
        entityNode->addChild(flowGraphNode);
    }
    assert(!!flowGraphNode);
    if (m_pFlowGraph)
    {
        m_pFlowGraph->SetGraphEntity(FlowEntityId(m_pEntity->GetId()));
        m_pFlowGraph->SerializeXML(flowGraphNode, bLoading);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentFlowGraph::NeedSerialize()
{
    return m_pFlowGraph != 0;
};

//////////////////////////////////////////////////////////////////////////
bool CComponentFlowGraph::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentFlowGraph");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentFlowGraph::Serialize(TSerialize ser)
{
    bool hasFlowGraph = m_pFlowGraph != 0;
    if (ser.BeginOptionalGroup("FlowGraph", hasFlowGraph))
    {
        if (ser.IsReading() && m_pFlowGraph == 0)
        {
            EntityWarning("Unable to reload flowgraph here");
            return;
        }

        if (m_pFlowGraph)
        {
            m_pFlowGraph->Serialize(ser);
        }

        ser.EndGroup();
    }
}
