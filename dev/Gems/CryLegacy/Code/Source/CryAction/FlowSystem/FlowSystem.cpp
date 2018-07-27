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

#include "FlowSystem.h"

#include <CryAction.h>

#include "FlowGraph.h"
#include "FlowInitManager.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Nodes/FlowLogNode.h"
#include "Nodes/FlowStartNode.h"
#include "Nodes/FlowTrackEventNode.h"
#include "Nodes/FlowDelayNode.h"

#include "Nodes/FlowScriptedNode.h"
#include "Nodes/FlowCompositeNode.h"
#include "Nodes/FlowTimeNode.h"
#include "Nodes/FlowEntityNode.h"

#include "Inspectors/FlowInspectorDefault.h"
#include "Modules/ModuleManager.h"

#include "PersistentDebug.h"
#include "AnimationGraph/DebugHistory.h"
#include "ITimer.h"
#include "TimeOfDayScheduler.h"
#include "VisualLog/VisualLog.h"

#include "CryPath.h"

#include <MathConversion.h>

#define BLACKLIST_FILE_PATH "Libs/FlowNodes/FlownodeBlacklist.xml"

#define GRAPH_RESERVED_CAPACITY 8

#ifndef _RELEASE
#define SHOW_FG_CRITICAL_LOADING_ERROR_ON_SCREEN
#endif

#ifndef _RELEASE
CFlowSystem::TSFGProfile CFlowSystem::FGProfile;
#endif //_RELEASE


//////////////////////////////////////////////////////////////////////////
string GetEntityFlowNodeName(const IEntityClass& entityClass)
{
    string entityPrefixStr = "entity:";
    return string(entityPrefixStr + entityClass.GetName());
}

template <class T>
class CAutoFlowFactory
    : public IFlowNodeFactory
{
public:
    CAutoFlowFactory()
        : m_refs(0) {}
    void AddRef() { m_refs++; }
    void Release()
    {
        if (0 == --m_refs)
        {
            delete this;
        }
    }
    IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) { return new T(pActInfo); }
    void GetMemoryUsage(ICrySizer* s) const
    {
        SIZER_SUBCOMPONENT_NAME(s, "CAutoFlowFactory");
        s->Add(*this);
    }

    void Reset() {}

private:
    int m_refs;
};

template <class T>
class CSingletonFlowFactory
    : public IFlowNodeFactory
{
public:
    CSingletonFlowFactory()
        : m_refs(0) {m_pInstance = new T(); }
    void AddRef() { m_refs++; }
    void Release()
    {
        if (0 == --m_refs)
        {
            delete this;
        }
    }
    void GetMemoryUsage(ICrySizer* s) const
    {
        SIZER_SUBCOMPONENT_NAME(s, "CSingletonFlowFactory");
        s->Add(*this);
        s->AddObject(m_pInstance);
    }
    IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo)
    {
        return m_pInstance;
    }

    void Reset() {}

private:
    IFlowNodePtr m_pInstance;
    int m_refs;
};

FlowEntityType GetFlowEntityTypeFromFlowEntityId(FlowEntityId id)
{
    if (id == FlowEntityId::s_invalidFlowEntityID)
    {
        return FlowEntityType::Invalid;
    }
    else
    {
        return IsLegacyEntityId(id) ? FlowEntityType::Legacy : FlowEntityType::Component;
    }
}

// FlowSystem Container
void CFlowSystemContainer::AddItem(TFlowInputData item)
{
    m_container.push_back(item);
}

void CFlowSystemContainer::AddItemUnique(TFlowInputData item)
{
    if (std::find(m_container.begin(), m_container.end(), item) == m_container.end())
    {
        m_container.push_back(item);
    }
}

void CFlowSystemContainer::RemoveItem(TFlowInputData item)
{
    m_container.erase(std::remove(m_container.begin(), m_container.end(), item), m_container.end());
}

TFlowInputData CFlowSystemContainer::GetItem(int i)
{
    return m_container[i];
}

void CFlowSystemContainer::RemoveItemAt(int i)
{
    m_container.erase(m_container.begin() + i);
}

void CFlowSystemContainer::Clear()
{
    m_container.clear();
}

int CFlowSystemContainer::GetItemCount() const
{
    return m_container.size();
}

void CFlowSystemContainer::GetMemoryUsage(ICrySizer* s) const
{
    if (GetItemCount() > 0)
    {
        s->Add(&m_container[0], m_container.capacity());
    }

    s->Add(m_container);
}

void CFlowSystemContainer::Serialize(TSerialize ser)
{
    int count = m_container.size();

    ser.Value("count", count);

    if (ser.IsWriting())
    {
        for (int i = 0; i < count; i++)
        {
            ser.BeginGroup("ContainerInfo");
            m_container[i].Serialize(ser);
            ser.EndGroup();
        }
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            ser.BeginGroup("ContainerInfo");
            TFlowInputData data;
            data.Serialize(ser);
            m_container.push_back(data);
            ser.EndGroup();
        }
    }
}

CFlowSystem::CFlowSystem()
    : m_bInspectingEnabled(false)
    , m_needToUpdateForwardings(false)
    , m_criticalLoadingErrorHappened(false)
    , m_graphs(GRAPH_RESERVED_CAPACITY)
    , m_nextFlowGraphId(0)
    , m_pModuleManager(NULL)
    , m_blacklistNode(NULL)
    , m_nextNodeTypeID(InvalidFlowNodeTypeId)
{
    LoadBlacklistedFlownodeXML();

    gEnv->pEntitySystem->GetClassRegistry()->RegisterListener(this);

    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
}

void CFlowSystem::PreInit()
{
    m_flowInitManager.reset(new FlowInitManager);

    m_pModuleManager = new CFlowGraphModuleManager();
    RegisterAllNodeTypes();

    m_pDefaultInspector = new CFlowInspectorDefault(this);
    RegisterInspector(m_pDefaultInspector, 0);
    EnableInspecting(false);
}


CFlowSystem::~CFlowSystem()
{
    gEnv->pEntitySystem->GetClassRegistry()->UnregisterListener(this);

    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
    {
        itGraph->NotifyFlowSystemDestroyed();
    }

    SAFE_DELETE(m_pModuleManager);
}

void CFlowSystem::Release()
{
    UnregisterInspector(m_pDefaultInspector, 0);

    if (m_pModuleManager)
    {
        m_pModuleManager->Shutdown();
    }

    delete this;
}

IFlowGraphPtr CFlowSystem::CreateFlowGraph()
{
    return new CFlowGraph(this);
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::ReloadAllNodeTypes()
{
    if (gEnv->IsEditor())
    {
        RegisterAllNodeTypes();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::RegisterAllNodeTypes()
{
    // Load UI initialization data from json
    m_flowInitManager->LoadInitData();

    // clear node types
    m_typeNameToIdMap.clear();
    m_typeRegistryVec.clear();

    // reset TypeIDs
    m_freeNodeTypeIDs.clear();
    m_nextNodeTypeID = InvalidFlowNodeTypeId;

    // register all types
    TFlowNodeTypeId typeId = RegisterType("InvalidType", 0);
    assert (typeId == InvalidFlowNodeTypeId);
    RegisterType("Debug:Log", new CSingletonFlowFactory<CFlowLogNode>());
    RegisterType("Game:Start", new CAutoFlowFactory<CFlowStartNode>());
    RegisterType("TrackEvent", new CAutoFlowFactory<CFlowTrackEventNode>());

    RegisterAutoTypes();

    LoadExtensions("Libs/FlowNodes");

    // register game specific flownodes
    gEnv->pGame->RegisterGameFlowNodes();

    // register entity type flownodes after the game
    RegisterEntityTypes();

#ifndef AZ_MONOLITHIC_BUILD

    // Last chance for engine modules and gems to register nodes before flow modules get loaded
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES, 0, 0);

#endif // AZ_MONOLITHIC_BUILD

    // this has to come after all other nodes are reloaded
    // as it will trigger the editor to reload the fg classes!
    GetModuleManager()->ScanForModules(); // reload all modules (since they need to register the start and end node types)
}

void CFlowSystem::LoadExtensions(string path)
{
    ICryPak* pCryPak = gEnv->pCryPak;
    _finddata_t fd;
    char filename[_MAX_PATH];

    path.TrimRight("/\\");
    string search = path + "/*.node";
    intptr_t handle = pCryPak->FindFirst(search.c_str(), &fd);
    if (handle != -1)
    {
        int res = 0;

        TPendingComposites pendingComposites;

        do
        {
            cry_strcpy(filename, path.c_str());
            cry_strcat(filename, "/");
            cry_strcat(filename, fd.name);

            XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename);
            if (root)
            {
                LoadExtensionFromXml(root, &pendingComposites);
            }

            res = pCryPak->FindNext(handle, &fd);
        }
        while (res >= 0);

        LoadComposites(&pendingComposites);

        pCryPak->FindClose(handle);
    }
}

void CFlowSystem::LoadExtensionFromXml(XmlNodeRef nodeParent, TPendingComposites* pComposites)
{
    XmlNodeRef node;
    int nChildren = nodeParent->getChildCount();
    int iChild;

    // run for nodeParent, and any children it contains
    for (iChild = -1, node = nodeParent;; )
    {
        const char* tag = node->getTag();
        if (!tag || !*tag)
        {
            return;
        }

        // Multiple components may be contained within a <NodeList> </NodeList> pair
        if ((0 == strcmp("NodeList", tag)) || (0 == strcmp("/NodeList", tag)))
        {
            // a list node, do nothing.
            // if root level, will advance to child below
        }
        else
        {
            const char* name = node->getAttr("name");
            if (!name || !*name)
            {
                return;
            }

            if (0 == strcmp("Script", tag))
            {
                const char* path = node->getAttr("script");
                if (!path || !*path)
                {
                    return;
                }
                CFlowScriptedNodeFactoryPtr pFactory = new CFlowScriptedNodeFactory();
                if (pFactory->Init(path, name))
                {
                    RegisterType(name, &*pFactory);
                }
            }
            else if (0 == strcmp("SimpleScript", tag))
            {
                const char* path = node->getAttr("script");
                if (!path || !*path)
                {
                    return;
                }
                CFlowSimpleScriptedNodeFactoryPtr pFactory = new CFlowSimpleScriptedNodeFactory();
                if (pFactory->Init(path, name))
                {
                    RegisterType(name, &*pFactory);
                }
            }
            else if (0 == strcmp("Composite", tag))
            {
                pComposites->push(node);
            }
        }

        if (++iChild >= nChildren)
        {
            break;
        }
        node = nodeParent->getChild(iChild);
    }
}

void CFlowSystem::LoadComposites(TPendingComposites* pComposites)
{
    size_t failCount = 0;
    while (!pComposites->empty() && failCount < pComposites->size())
    {
        CFlowCompositeNodeFactoryPtr pFactory = new CFlowCompositeNodeFactory();
        switch (pFactory->Init(pComposites->front(), this))
        {
        case CFlowCompositeNodeFactory::eIR_Failed:
            GameWarning("Failed to load composite due to invalid data: %s", pComposites->front()->getAttr("name"));
            pComposites->pop();
            failCount = 0;
            break;
        case CFlowCompositeNodeFactory::eIR_Ok:
            RegisterType(pComposites->front()->getAttr("name"), &*pFactory);
            pComposites->pop();
            failCount = 0;
            break;
        case CFlowCompositeNodeFactory::eIR_NotYet:
            pComposites->push(pComposites->front());
            pComposites->pop();
            failCount++;
            break;
        }
    }

    while (!pComposites->empty())
    {
        GameWarning("Failed to load composite due to failed dependency: %s", pComposites->front()->getAttr("name"));
        pComposites->pop();
    }
}

void CFlowSystem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
    {
        // If went to editor mode then uninitialize
        bool inGame = (wparam != 0U);
        if (!inGame)
        {
            Uninitialize();
        }
        break;
    }
    case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
    {
        // If AI/Physics turned off then uninitialize
        bool simulationEnabled = static_cast<uint32>(wparam) == 0;
        if (simulationEnabled)
        {
            Uninitialize();
        }
        break;
    }
    }
}

class CFlowSystem::CNodeTypeIterator
    : public IFlowNodeTypeIterator
{
public:
    CNodeTypeIterator(CFlowSystem* pImpl)
        : m_nRefs(0)
        , m_pImpl(pImpl)
        , m_iter(pImpl->m_typeRegistryVec.begin())
    {
        assert (m_iter != m_pImpl->m_typeRegistryVec.end());
        ++m_iter;
        m_id = InvalidFlowNodeTypeId;
        assert (m_id == 0);
    }
    void AddRef()
    {
        ++m_nRefs;
    }
    void Release()
    {
        if (0 == --m_nRefs)
        {
            delete this;
        }
    }
    bool Next(SNodeType& nodeType)
    {
        while (m_iter != m_pImpl->m_typeRegistryVec.end() && !_stricmp(m_iter->name, ""))
        {
            ++m_id;
            ++m_iter;
        }

        if (m_iter == m_pImpl->m_typeRegistryVec.end())
        {
            return false;
        }

        nodeType.typeId = ++m_id;
        nodeType.typeName = m_iter->name;
        ++m_iter;
        return true;
    }
private:
    int m_nRefs;
    TFlowNodeTypeId m_id;
    CFlowSystem* m_pImpl;
    std::vector<STypeInfo>::iterator m_iter;
};

IFlowNodeTypeIteratorPtr CFlowSystem::CreateNodeTypeIterator()
{
    return new CNodeTypeIterator(this);
}

TFlowNodeTypeId CFlowSystem::GenerateNodeTypeID()
{
    if (!m_freeNodeTypeIDs.empty())
    {
        TFlowNodeTypeId typeID = m_freeNodeTypeIDs.back();
        m_freeNodeTypeIDs.pop_back();
        return typeID;
    }

    if (m_nextNodeTypeID > CFlowData::TYPE_MAX_COUNT)
    {
        CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_ERROR, "CFlowSystem::GenerateNodeTypeID: Reached maximum amount of NodeTypes: Limit=%d", CFlowData::TYPE_MAX_COUNT);
        return InvalidFlowNodeTypeId;
    }

    return m_nextNodeTypeID++;
}

void CFlowSystem::RegisterNodeInfo(const char* type, IFlowNodeFactoryPtr factory)
{
    m_flowInitManager->RegisterNodeInfo(type);
}

TFlowNodeTypeId CFlowSystem::RegisterType(const char* type, IFlowNodeFactoryPtr factory)
{
    if (!BlacklistedFlownode(&type))
    {
        return InvalidFlowNodeTypeId;
    }

    string typeName = type;
    const TTypeNameToIdMap::const_iterator iter = m_typeNameToIdMap.find(typeName);
    if (iter == m_typeNameToIdMap.end())
    {
        const TFlowNodeTypeId nTypeId = GenerateNodeTypeID();
        if ((nTypeId == InvalidFlowNodeTypeId) && typeName.compareNoCase("InvalidType"))
        {
            return nTypeId;
        }

        m_typeNameToIdMap[typeName] = nTypeId;
        STypeInfo typeInfo(typeName, factory);

        if (nTypeId >= m_typeRegistryVec.size())
        {
            m_typeRegistryVec.push_back(typeInfo);
        }
        else
        {
            m_typeRegistryVec[nTypeId] = typeInfo;
        }

        RegisterNodeInfo(typeName, factory);
        return nTypeId;
    }
    else
    {
        // overriding
        TFlowNodeTypeId nTypeId = iter->second;

        if (!factory->AllowOverride())
        {
            CryFatalError("CFlowSystem::RegisterType: Type '%s' Id=%u already registered. Overriding not allowed by node factory.", type, nTypeId);
        }

        assert (nTypeId < m_typeRegistryVec.size());
        STypeInfo& typeInfo = m_typeRegistryVec[nTypeId];
        typeInfo.factory = factory;
        return nTypeId;
    }
}

bool CFlowSystem::UnregisterType(const char* typeName)
{
    const TTypeNameToIdMap::iterator iter = m_typeNameToIdMap.find(typeName);

    if (iter != m_typeNameToIdMap.end())
    {
        const TFlowNodeTypeId typeID = iter->second;
        m_freeNodeTypeIDs.push_back(typeID);
        m_typeRegistryVec[typeID] = STypeInfo();
        m_typeNameToIdMap.erase(iter);

        return true;
    }

    return false;
}

IFlowNodePtr CFlowSystem::CreateNodeOfType(IFlowNode::SActivationInfo* pActInfo, TFlowNodeTypeId typeId)
{
    assert (typeId < m_typeRegistryVec.size());
    if (typeId >= m_typeRegistryVec.size())
    {
        return 0;
    }

    const STypeInfo& type = m_typeRegistryVec[typeId];
    IFlowNodeFactory* pFactory = type.factory;
    if (pFactory)
    {
        return pFactory->Create(pActInfo);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::Update()
{
    #ifdef SHOW_FG_CRITICAL_LOADING_ERROR_ON_SCREEN
    if (m_criticalLoadingErrorHappened && !gEnv->IsEditor())
    {
        gEnv->pRenderer->Draw2dLabel(10, 30, 2, Col_Red, false, "CRITICAL ERROR. SOME FLOWGRAPHS HAVE BEEN DISCARDED");
        gEnv->pRenderer->Draw2dLabel(10, 50, 2, Col_Red, false, "LEVEL LOGIC COULD BE DAMAGED. check log for more info.");
    }
    #endif

    {
        FRAME_PROFILER("CFlowSystem::ForeignSystemUpdate", gEnv->pSystem, PROFILE_ACTION);
        // These things need to be updated in game mode and ai/physics mode
        CCryAction::GetCryAction()->GetPersistentDebug()->Update(gEnv->pTimer->GetFrameTime());
        CDebugHistoryManager::RenderAll();
        CCryAction::GetCryAction()->GetTimeOfDayScheduler()->Update();
        CCryAction::GetCryAction()->GetVisualLog()->Update();
    }

    {
        FRAME_PROFILER("CFlowSystem::Update()", gEnv->pSystem, PROFILE_ACTION);
        if (m_cVars.m_enableUpdates == 0)
        {
            /*
                IRenderer * pRend = gEnv->pRenderer;
                float white[4] = {1,1,1,1};
                pRend->Draw2dLabel( 10, 100, 2, white, false, "FlowGraphSystem Disabled");
            */
            return;
        }

        if (m_bInspectingEnabled)
        {
            // call pre updates

            // 1. system inspectors
            std::for_each (m_systemInspectors.begin(), m_systemInspectors.end(), std::bind2nd (std::mem_fun(&IFlowGraphInspector::PreUpdate), (IFlowGraph*) 0));

            // 2. graph inspectors TODO: optimize not to go over all graphs ;-)
            for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
            {
                const std::vector<IFlowGraphInspectorPtr>& graphInspectors (itGraph->GetInspectors());
                std::for_each (graphInspectors.begin(), graphInspectors.end(), std::bind2nd (std::mem_fun(&IFlowGraphInspector::PreUpdate), *itGraph));
            }
        }

        UpdateGraphs();

        if (m_bInspectingEnabled)
        {
            // call post updates

            // 1. system inspectors
            std::for_each (m_systemInspectors.begin(), m_systemInspectors.end(), std::bind2nd (std::mem_fun(&IFlowGraphInspector::PostUpdate), (IFlowGraph*) 0));

            // 2. graph inspectors TODO: optimize not to go over all graphs ;-)
            for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
            {
                const std::vector<IFlowGraphInspectorPtr>& graphInspectors (itGraph->GetInspectors());
                std::for_each (graphInspectors.begin(), graphInspectors.end(), std::bind2nd (std::mem_fun(&IFlowGraphInspector::PostUpdate), *itGraph));
            }
        }
    }

    // end of flow system update: remove module instances which are no longer needed
    m_pModuleManager->RemoveCompletedModuleInstances();
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::UpdateGraphs()
{
    // Determine if graphs should be updated (Debug control)
    bool bUpdateGraphs = true;
    PREFAST_SUPPRESS_WARNING(6237);
    if (gEnv->IsEditing() && CCryAction::GetCryAction()->IsGameStarted())
    {
        if (m_cVars.m_enableFlowgraphNodeDebugging == 2)
        {
            if (m_cVars.m_debugNextStep == 0)
            {
                bUpdateGraphs = false;
            }
            else
            {
                m_cVars.m_debugNextStep = 0;
            }
        }
    }

    if (bUpdateGraphs)
    {
        if (m_needToUpdateForwardings)
        {
            for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
            {
                itGraph->UpdateForwardings();
            }
            m_needToUpdateForwardings = false;
        }


        for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
        {
            itGraph->Update();
        }
    }

#ifndef _RELEASE
    if (m_cVars.m_profile != 0)
    {
        IRenderer* pRend = gEnv->pRenderer;
        float white[4] = {1, 1, 1, 1};
        pRend->Draw2dLabel(10, 100, 2, white, false, "Number of Flow Graphs Updated: %d", FGProfile.graphsUpdated);
        pRend->Draw2dLabel(10, 120, 2, white, false, "Number of Flow Graph Nodes Updated: %d", FGProfile.nodeUpdates);
        pRend->Draw2dLabel(10, 140, 2, white, false, "Number of Flow Graph Nodes Activated: %d", FGProfile.nodeActivations);
    }
    FGProfile.Reset();
#endif //_RELEASE
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::Reset(bool unload)
{
    m_needToUpdateForwardings = true; // does not hurt, and it prevents a potentially bad situation in save/load.
    if (!unload)
    {
        for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
        {
            itGraph->InitializeValues();
        }
    }
    else
    {
        m_pModuleManager->ClearModules();
        if (!m_graphs.Empty())
        {
            m_graphs.Clear(true);
        }
        for (std::vector<STypeInfo>::iterator it = m_typeRegistryVec.begin(), itEnd = m_typeRegistryVec.end(); it != itEnd; ++it)
        {
            if (it->factory.get())
            {
                it->factory->Reset();
            }
        }
    }

    // Clean up the containers
    m_FlowSystemContainers.clear();
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::Init()
{
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->AddSink(this, IEntitySystem::OnReused | IEntitySystem::OnSpawn, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::Shutdown()
{
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->RemoveSink(this);
    }
}

void CFlowSystem::Uninitialize()
{
    for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
    {
        itGraph->Uninitialize();
    }
}

//////////////////////////////////////////////////////////////////////////
TFlowGraphId CFlowSystem::RegisterGraph(IFlowGraphPtr pGraph, const char* debugName)
{
    return RegisterGraph(static_cast<CFlowGraph*>(pGraph.get()), debugName);
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::UnregisterGraph(IFlowGraphPtr pGraph)
{
    UnregisterGraph(static_cast<CFlowGraph*>(pGraph.get()));
}

//////////////////////////////////////////////////////////////////////////
TFlowGraphId CFlowSystem::RegisterGraph(CFlowGraphBase* pGraph, const char* debugName)
{
    m_graphs.Add(pGraph, debugName);
    return m_nextFlowGraphId++;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::UnregisterGraph(CFlowGraphBase* pGraph)
{
    m_graphs.Remove(pGraph);
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::LoadBlacklistedFlownodeXML()
{
    if (!m_blacklistNode)
    {
        const string filename = BLACKLIST_FILE_PATH;
        m_blacklistNode = gEnv->pSystem->LoadXmlFromFile(filename);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowSystem::BlacklistedFlownode(const char** nodeName)
{
    if (m_blacklistNode)
    {
        int childCount = m_blacklistNode->getChildCount();
        const char* attrKey;
        const char* attrValue;
        for (int n = 0; n < childCount; ++n)
        {
            XmlNodeRef child = m_blacklistNode->getChild(n);

            if (!child->getAttributeByIndex(0, &attrKey, &attrValue))
            {
                continue;
            }

            if (!_stricmp(attrValue, *nodeName))
            {
                //replace name
                int numAttr = child->getNumAttributes();
                if (numAttr == 2 && child->getAttributeByIndex(1, &attrKey, &attrValue))
                {
                    *nodeName = attrValue;
                    break;
                }

                //remove class
                return false;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::RegisterAutoTypes()
{
    //////////////////////////////////////////////////////////////////////////
    CAutoRegFlowNodeBase* pFactory = CAutoRegFlowNodeBase::m_pFirst;
    while (pFactory)
    {
        RegisterType(pFactory->m_sClassName, pFactory);

        if (pFactory->m_sAlias1)
        {
            RegisterType(pFactory->m_sAlias1, pFactory);
        }

        if (pFactory->m_sAlias2)
        {
            RegisterType(pFactory->m_sAlias2, pFactory);
        }

        pFactory = pFactory->m_pNext;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::RegisterEntityTypes()
{
    // Register all entity class from entities registry.
    // Each entity class is registered as a flow type entity:ClassName, ex: entity:ProximityTrigger
    IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
    IEntityClass* pEntityClass = 0;
    pClassRegistry->IteratorMoveFirst();
    while (pEntityClass = pClassRegistry->IteratorNext())
    {
        TryToRegisterEntityClass(pEntityClass);
    }
}
//////////////////////////////////////////////////////////////////////////
void CFlowSystem::TryToRegisterEntityClass(IEntityClass* entityClass)
{
    if (!entityClass)
    {
        return;
    }
    string classname = GetEntityFlowNodeName(*entityClass);

    INDENT_LOG_DURING_SCOPE(true, "Flow system is registering entity type '%s'", classname.c_str());

    // if the entity lua script does not have input/outputs defined, and there is already an FG node defined for that entity in c++, do not register the empty lua one
    // GetEventCount initializes the script system.
    if (entityClass->GetEventCount() == 0 && GetTypeId(classname) != InvalidFlowNodeTypeId)
    {
        return;
    }

    RegisterType(classname, new CFlowEntityClass(entityClass));
}
//////////////////////////////////////////////////////////////////////////
const CFlowSystem::STypeInfo& CFlowSystem::GetTypeInfo(TFlowNodeTypeId typeId) const
{
    assert (typeId < m_typeRegistryVec.size());
    if (typeId < m_typeRegistryVec.size())
    {
        return m_typeRegistryVec[typeId];
    }
    return m_typeRegistryVec[InvalidFlowNodeTypeId];
}

//////////////////////////////////////////////////////////////////////////
string CFlowSystem::GetClassTagFromUIName(const string& uiName) const
{
    return m_flowInitManager->GetClassTagFromUIName(AZStd::string(uiName.c_str())).c_str();
}

string CFlowSystem::GetUINameFromClassTag(const string& classTag) const
{
    return m_flowInitManager->GetUINameFromClassTag(AZStd::string(classTag.c_str())).c_str();
}
//////////////////////////////////////////////////////////////////////////
const char* CFlowSystem::GetTypeName(TFlowNodeTypeId typeId)
{
    assert (typeId < m_typeRegistryVec.size());
    if (typeId < m_typeRegistryVec.size())
    {
        return m_typeRegistryVec[typeId].name.c_str();
    }
    return "";
}

//////////////////////////////////////////////////////////////////////////
TFlowNodeTypeId CFlowSystem::GetTypeId(const char* typeName)
{
    return stl::find_in_map(m_typeNameToIdMap, GetClassTagFromUIName(CONST_TEMP_STRING(typeName)), InvalidFlowNodeTypeId);
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::RegisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph)
{
    if (pGraph == 0)
    {
        stl::push_back_unique(m_systemInspectors, pInspector);
    }
    else
    {
        CFlowGraphBase* pCGraph = (CFlowGraphBase*)pGraph.get();
        if (pCGraph && m_graphs.Contains(pCGraph))
        {
            pCGraph->RegisterInspector(pInspector);
        }
        else
        {
            GameWarning("CFlowSystem::RegisterInspector: Unknown graph 0x%p", (IFlowGraph*)pGraph);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::UnregisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph)
{
    if (pGraph == 0)
    {
        stl::find_and_erase(m_systemInspectors, pInspector);
    }
    else
    {
        CFlowGraphBase* pCGraph = (CFlowGraphBase*)pGraph.get();
        if (pCGraph && m_graphs.Contains(pCGraph))
        {
            pCGraph->UnregisterInspector(pInspector);
        }
        else
        {
            GameWarning("CFlowSystem::UnregisterInspector: Unknown graph 0x%p", (IFlowGraph*)pGraph);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::NotifyFlow(CFlowGraphBase* pGraph, const SFlowAddress from, const SFlowAddress to)
{
    if (!m_bInspectingEnabled)
    {
        return;
    }

    if (!m_systemInspectors.empty())
    {
        std::vector<IFlowGraphInspectorPtr>::iterator iter (m_systemInspectors.begin());
        while (iter != m_systemInspectors.end())
        {
            (*iter)->NotifyFlow(pGraph, from, to);
            ++iter;
        }
    }
    const std::vector<IFlowGraphInspectorPtr>& graphInspectors (pGraph->GetInspectors());
    if (!graphInspectors.empty())
    {
        std::vector<IFlowGraphInspectorPtr>::const_iterator iter (graphInspectors.begin());
        while (iter != graphInspectors.end())
        {
            (*iter)->NotifyFlow(pGraph, from, to);
            ++iter;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::NotifyProcessEvent(CFlowGraphBase* pGraph, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo, IFlowNode* pImpl)
{
    if (!m_bInspectingEnabled)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowGraph* CFlowSystem::GetGraphById(TFlowGraphId graphId)
{
    for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
    {
        CFlowGraphBase* pFlowGraph = *itGraph;

        if (pFlowGraph->GetGraphId() == graphId)
        {
            return pFlowGraph;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_SUBCOMPONENT_NAME(pSizer, "FlowSystem");
    pSizer->AddObject(this, sizeof(*this));

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "Factories");
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "FactoriesLookup");
            pSizer->AddObject(m_typeNameToIdMap);
            pSizer->AddObject(m_typeRegistryVec);
        }
    }

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "Inspectors");
        pSizer->AddObject(m_systemInspectors);
    }

    if (!m_graphs.Empty())
    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "Graphs");
        pSizer->AddObject(&m_graphs, m_graphs.MemSize());
    }

    if (!m_FlowSystemContainers.empty())
    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "Containers");
        for (TFlowSystemContainerMap::const_iterator it = m_FlowSystemContainers.begin(); it != m_FlowSystemContainers.end(); ++it)
        {
            (*it).second->GetMemoryUsage(pSizer);
        }

        pSizer->AddObject(&(*m_FlowSystemContainers.begin()), m_FlowSystemContainers.size() * (sizeof(CFlowSystemContainer) + sizeof(TFlowSystemContainerId)));
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::OnReused(IEntity* pEntity, SEntitySpawnParams& params)
{
    for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
    {
        itGraph->OnEntityReused(pEntity, params);
    }
}
//////////////////////////////////////////////////////////////////////////
void CFlowSystem::OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass)
{
    // IEntityClass has some getter functions (ex: IEntityClass::GetEventCount())
    // that trigger a lazy load; therefore the functions are non-const.
    // The flow system needs to call these getters so we need to cast
    // away the const here.
    // A better solution would be to make the getters const by marking
    // the lazily loaded data in IEntityClass as mutable.
    IEntityClass* pNonConstEntityClass = const_cast<IEntityClass*>(pEntityClass);
    TryToRegisterEntityClass(pNonConstEntityClass);
}
//////////////////////////////////////////////////////////////////////////
void CFlowSystem::OnEntityIdChanged(FlowEntityId oldId, FlowEntityId newId)
{
    for (TGraphs::Notifier itGraph(m_graphs); itGraph.IsValid(); itGraph.Next())
    {
        itGraph->OnEntityIdChanged(oldId, newId);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowSystem::CreateContainer(TFlowSystemContainerId id)
{
    if (m_FlowSystemContainers.find(id) != m_FlowSystemContainers.end())
    {
        return false;
    }

    IFlowSystemContainerPtr container(new CFlowSystemContainer);
    m_FlowSystemContainers.insert(std::make_pair(id, container));
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::DeleteContainer(TFlowSystemContainerId id)
{
    std::map<TFlowSystemContainerId, IFlowSystemContainerPtr>::iterator it = m_FlowSystemContainers.find(id);
    if (it != m_FlowSystemContainers.end())
    {
        m_FlowSystemContainers.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowSystemContainerPtr CFlowSystem::GetContainer(TFlowSystemContainerId id)
{
    return m_FlowSystemContainers[id];
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::OnSpawn(IEntity* pEntity, SEntitySpawnParams& params)
{
    // this should be a more generic test: maybe an entity flag "CAN_FORWARD_FLOWGRAPHS", or something similar
    if ((pEntity->GetFlags() & ENTITY_FLAG_HAS_AI) != 0)
    {
        m_needToUpdateForwardings = true;
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphModuleManager* CFlowSystem::GetIModuleManager()
{
    return m_pModuleManager;
}

//////////////////////////////////////////////////////////////////////////
void CFlowSystem::Serialize(TSerialize ser)
{
    int count = m_FlowSystemContainers.size();
    ser.Value("ContainerCount", count);

    if (ser.IsWriting())
    {
        TFlowSystemContainerMap::iterator it = m_FlowSystemContainers.begin();
        while (it != m_FlowSystemContainers.end())
        {
            ser.BeginGroup("Containers");
            TFlowSystemContainerId id = (*it).first;
            ser.Value("key", id);
            (*it).second->Serialize(ser);
            ser.EndGroup();
            ++it;
        }
    }
    else
    {
        int id;
        for (int i = 0; i < count; i++)
        {
            ser.BeginGroup("Containers");
            ser.Value("key", id);
            if (CreateContainer(id))
            {
                IFlowSystemContainerPtr container = GetContainer(id);
                if (container)
                {
                    container->Serialize(ser);
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_ERROR, "CFlowSystem::Serialize: Could not create or get container with ID from save file - container not restored");
                }
            }
            ser.EndGroup();
        }
    }
}

