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

// Description : Module container


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Modules/Module.h>
#include <FlowSystem/Modules/FlowModuleNodes.h>
#include <ILevelSystem.h>

IModuleInstance* CFlowGraphModule::CInstanceIterator::Next()
{
    if (m_cur == m_pModule->m_instances.end())
    {
        return NULL;
    }
    IModuleInstance* pCur = &(*m_cur);
    ++m_cur;
    return pCur;
}

////////////////////////////////////////////////////
CFlowGraphModule::CFlowGraphModule(TModuleId moduleId)
{
    m_pRootGraph = NULL;
    m_Id = moduleId;
    m_nextInstanceId = 0;
    m_type = IFlowGraphModule::eT_Global;

    m_startNodeTypeId = InvalidFlowNodeTypeId;
    m_returnNodeTypeId = InvalidFlowNodeTypeId;
    m_callNodeTypeId = InvalidFlowNodeTypeId;
}

////////////////////////////////////////////////////
CFlowGraphModule::~CFlowGraphModule()
{
    Destroy();
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModule::PreLoadModule(const char* fileName)
{
    m_fileName = fileName;

    XmlNodeRef moduleRef = gEnv->pSystem->LoadXmlFromFile(fileName);

    if (!moduleRef)
    {
        CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_WARNING, "Unable to preload Flowgraph Module: %s", PathUtil::GetFileName(fileName).c_str());
        return false;
    }

    assert(!_stricmp(moduleRef->getTag(), "Graph"));

    bool module = false;
    moduleRef->getAttr("isModule", module);
    assert(module);

    XmlString tempName;
    if (moduleRef->getAttr("moduleName", tempName))
    {
        m_name = tempName;
    }

    bool bResult = (m_pRootGraph != NULL);
    assert(m_pRootGraph == NULL);

    // first handle module ports
    XmlNodeRef modulePorts = moduleRef->findChild("ModuleInputsOutputs");
    RemoveModulePorts();
    if (modulePorts)
    {
        int nPorts = modulePorts->getChildCount();
        for (int i = 0; i < nPorts; ++i)
        {
            XmlString portName;
            int portType;
            bool isInput;

            XmlNodeRef port = modulePorts->getChild(i);
            port->getAttr("Name", portName);
            port->getAttr("Type", portType);
            port->getAttr("Input", isInput);

            IFlowGraphModule::SModulePortConfig portConfig;
            portConfig.name = portName.c_str();
            portConfig.type = (EFlowDataTypes)portType;
            portConfig.input = isInput;

            AddModulePort(portConfig);
        }
    }

    // and create nodes for this module (needs to be done before actual graph load, so that the
    //  nodes can be created there)
    RegisterNodes();

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModule::LoadModuleGraph(const char* moduleName, const char* fileName)
{
    assert(m_name == moduleName);
    assert(m_fileName == fileName);

    AZStd::string fullFilename;
    if (gEnv->IsEditor())
    {
        fullFilename = gEnv->pFilePathManager->PrepareAssetIDForWriting(fileName);
    }
    else
    {
        fullFilename = fileName;
    }

    XmlNodeRef moduleRef = gEnv->pSystem->LoadXmlFromFile(fullFilename.c_str());

    if (!moduleRef)
    {
        CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_WARNING, "Unable to load Flowgraph Module Graph: %s", moduleName);
        return false;
    }

    assert(!_stricmp(moduleRef->getTag(), "Graph"));

    bool module = false;
    moduleRef->getAttr("isModule", module);
    assert(module);

    bool bResult = (m_pRootGraph != NULL);
    assert(m_pRootGraph == NULL);

    if (!m_pRootGraph)
    {
        IFlowSystem* pSystem = gEnv->pFlowSystem;
        assert(pSystem);

        // Create graph
        m_pRootGraph = pSystem->CreateFlowGraph();
        if (m_pRootGraph)
        {
            m_pRootGraph->SerializeXML(moduleRef, true);

            // Root graph is for cloning, so not active!
            m_pRootGraph->UnregisterFromFlowSystem();
            m_pRootGraph->SetEnabled(false);
            m_pRootGraph->SetActive(false);
            bResult = true;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModule::SaveModuleXml(XmlNodeRef saveTo)
{
    if (!m_pRootGraph || !saveTo)
    {
        return false;
    }

    saveTo->setAttr("isModule", true);
    saveTo->setAttr("moduleName", m_name);

    // NB: don't save our graph here, just the module ports (graph is saved
    //  by the calling code)

    if (m_modulePorts.size() > 0)
    {
        XmlNodeRef inputs = saveTo->newChild("ModuleInputsOutputs");
        for (size_t i = 0; i < m_modulePorts.size(); ++i)
        {
            XmlNodeRef ioChild = inputs->newChild("Port");
            ioChild->setAttr("Name", m_modulePorts[i].name);
            ioChild->setAttr("Type", m_modulePorts[i].type);
            ioChild->setAttr("Input", m_modulePorts[i].input);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::Destroy()
{
    m_pRootGraph = NULL;

    TInstanceList::iterator i = m_instances.begin();
    TInstanceList::iterator end = m_instances.end();
    for (; i != end; ++i)
    {
        i->pGraph = NULL;
    }
    m_instances.clear();

    UnregisterNodes();
}

//////////////////////////////////////////////////////////////////////////
TModuleInstanceId CFlowGraphModule::CreateInstance(TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, TModuleParams const& params, const ModuleInstanceReturnCallback& returnCallback)
{
    TModuleInstanceId result = MODULEINSTANCE_INVALID;

    if (m_pRootGraph)
    {
        IFlowSystem* pSystem = gEnv->pFlowSystem;
        assert(pSystem);

        // Clone and create instance
        IFlowGraphPtr pClone = m_pRootGraph->Clone();
        if (pClone)
        {
            // Start up
            pClone->SetEnabled(true);
            pClone->SetActive(true);
            pClone->InitializeValues();
            pClone->SetType(IFlowGraph::eFGT_Module);

            // Store instance
            SInstance instance(m_nextInstanceId++);
            instance.pGraph = pClone;
            instance.callerGraph = callerGraphId;
            instance.callerNode = callerNodeId;
            instance.returnCallback = returnCallback;
            instance.bUsed = true;
            m_instances.push_back(instance);

            result = instance.instanceId;
        }
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::RefreshInstance(TModuleInstanceId instanceId, TModuleParams const& params)
{
    // Find the instance, and refresh with updated parameters
    TInstanceList::iterator i = m_instances.begin();
    TInstanceList::iterator end = m_instances.end();
    for (; i != end; ++i)
    {
        if (i->instanceId == instanceId)
        {
            ActivateGraph(i->pGraph, instanceId, params);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::CancelInstance(TModuleInstanceId instanceId)
{
    // Find the instance, and request cancel
    TInstanceList::iterator i = m_instances.begin();
    TInstanceList::iterator end = m_instances.end();
    for (; i != end; ++i)
    {
        if (i->instanceId == instanceId)
        {
            IFlowGraph* pGraph = i->pGraph;
            if (pGraph)
            {
                IFlowNodeIteratorPtr pNodeIter = pGraph->CreateNodeIterator();
                if (pNodeIter)
                {
                    TFlowNodeId id = InvalidFlowNodeId;
                    IFlowNodeData* pData;
                    while (pData = pNodeIter->Next(id))
                    {
                        // Check if its the starting node
                        if (m_startNodeTypeId == pData->GetNodeTypeId())
                        {
                            CFlowModuleStartNode* pNode = (CFlowModuleStartNode*)pData->GetNode();
                            pNode->OnCancel();

                            break;
                        }
                    }
                }
            }

            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModule::DestroyInstance(TModuleInstanceId instanceId, bool bSuccess, TModuleParams const& params)
{
    bool bResult = false;

    // Find the instance
    TInstanceList::iterator i = m_instances.begin();
    TInstanceList::iterator end = m_instances.end();
    for (; i != end; ++i)
    {
        if (i->instanceId == instanceId)
        {
            DeactivateGraph(*i, bSuccess, params);

            // mark as unused. Can't delete graph at this point as it is still
            //  in the middle of an update. Will be deleted at end of flow system update.
            i->bUsed = false;
            bResult  = true;

            break;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::ActivateGraph(IFlowGraph* pGraph, TModuleInstanceId instanceId, TModuleParams const& params) const
{
    if (pGraph)
    {
        IFlowNodeIteratorPtr pNodeIter = pGraph->CreateNodeIterator();
        if (pNodeIter)
        {
            TFlowNodeId id = InvalidFlowNodeId;
            IFlowNodeData* pData;
            while (pData = pNodeIter->Next(id))
            {
                // Check if its the starting node
                if (m_startNodeTypeId == pData->GetNodeTypeId())
                {
                    CFlowModuleStartNode* pNode = (CFlowModuleStartNode*)pData->GetNode();
                    pNode->OnActivate(params);
                }

                // Check if its the returning node
                if (m_returnNodeTypeId == pData->GetNodeTypeId())
                {
                    CFlowModuleReturnNode* pNode = (CFlowModuleReturnNode*)pData->GetNode();
                    pNode->SetModuleId(m_Id, instanceId);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::DeactivateGraph(SInstance& instance, bool bSuccess, TModuleParams const& params) const
{
    IFlowGraph* pModuleGraph = instance.pGraph;
    if (pModuleGraph)
    {
        pModuleGraph->SetEnabled(false);
        pModuleGraph->SetActive(false);

        // Deactivate all nodes
        IFlowNodeIteratorPtr pNodeIter = pModuleGraph->CreateNodeIterator();
        if (pNodeIter)
        {
            TFlowNodeId id = InvalidFlowNodeId;
            while (pNodeIter->Next(id))
            {
                pModuleGraph->SetRegularlyUpdated(id, false);
            }
        }
    }

    if (instance.returnCallback)
    {
        instance.returnCallback(bSuccess, params);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::RegisterNodes()
{
    IFlowGraphModuleManager* pMgr = gEnv->pFlowSystem ? gEnv->pFlowSystem->GetIModuleManager() : NULL;

    if (pMgr)
    {
        switch (m_type)
        {
        case IFlowGraphModule::eT_Global:
        case IFlowGraphModule::eT_Level:
            // Intentional fall-through since these both behave the same way here.
            m_startNodeTypeId = gEnv->pFlowSystem->RegisterType(pMgr->GetStartNodeName(m_name), new CFlowModuleStartNodeFactory(m_Id));
            m_returnNodeTypeId = gEnv->pFlowSystem->RegisterType(pMgr->GetReturnNodeName(m_name), new CFlowModuleReturnNodeFactory(m_Id));
            m_callNodeTypeId = gEnv->pFlowSystem->RegisterType(pMgr->GetCallerNodeName(m_name), new CFlowModuleCallNodeFactory(m_Id));
            break;
        default:
            CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
            break;
        }
    }
}

void CFlowGraphModule::UnregisterNodes()
{
    IFlowGraphModuleManager* pMgr = gEnv->pFlowSystem ? gEnv->pFlowSystem->GetIModuleManager() : NULL;

    if (pMgr)
    {
        gEnv->pFlowSystem->UnregisterType(pMgr->GetStartNodeName(m_name));
        gEnv->pFlowSystem->UnregisterType(pMgr->GetReturnNodeName(m_name));
        gEnv->pFlowSystem->UnregisterType(pMgr->GetCallerNodeName(m_name));
    }
}


//////////////////////////////////////////////////////////////////////////

void CFlowGraphModule::RemoveModulePorts()
{
    stl::free_container(m_modulePorts);
}

//////////////////////////////////////////////////////////////////////////
size_t CFlowGraphModule::GetModulePortCount() const
{
    return m_modulePorts.size();
}

//////////////////////////////////////////////////////////////////////////
const IFlowGraphModule::SModulePortConfig* CFlowGraphModule::GetModulePort(size_t index) const
{
    if (index < m_modulePorts.size())
    {
        return &m_modulePorts[index];
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModule::AddModulePort(const IFlowGraphModule::SModulePortConfig& port)
{
    m_modulePorts.push_back(port);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModule::RemoveCompletedInstances()
{
    for (TInstanceList::iterator it = m_instances.begin(), end = m_instances.end(); it != end; ++it)
    {
        if (!it->bUsed)
        {
            *it = SInstance(MODULEINSTANCE_INVALID);
        }
    }

    stl::find_and_erase_all(m_instances, SInstance(MODULEINSTANCE_INVALID));

    if (m_instances.size() == 0)
    {
        m_nextInstanceId = 0;
    }
}

void CFlowGraphModule::RemoveAllInstances()
{
    for (TInstanceList::iterator it = m_instances.begin(), end = m_instances.end(); it != end; ++it)
    {
        IFlowGraph* pModuleGraph = it->pGraph;
        if (pModuleGraph)
        {
            pModuleGraph->SetEnabled(false);
            pModuleGraph->SetActive(false);

            IFlowNodeIteratorPtr pNodeIter = pModuleGraph->CreateNodeIterator();
            if (pNodeIter)
            {
                TFlowNodeId id = InvalidFlowNodeId;
                while (pNodeIter->Next(id))
                {
                    pModuleGraph->SetRegularlyUpdated(id, false);
                }
            }
        }
        it->bUsed = false;
    }
}

IFlowGraphPtr CFlowGraphModule::GetInstanceGraph(TModuleInstanceId instanceID)
{
    for (TInstanceList::iterator it = m_instances.begin(), end = m_instances.end(); it != end; ++it)
    {
        if (it->instanceId == instanceID)
        {
            return it->pGraph;
        }
    }

    return NULL;
}

bool CFlowGraphModule::HasInstanceGraph(IFlowGraphPtr pGraph)
{
    for (TInstanceList::iterator it = m_instances.begin(), end = m_instances.end(); it != end; ++it)
    {
        if (it->pGraph == pGraph)
        {
            return true;
        }
    }

    return false;
}

IModuleInstanceIteratorPtr CFlowGraphModule::CreateInstanceIterator()
{
    if (m_iteratorPool.empty())
    {
        return new CInstanceIterator(this);
    }
    else
    {
        CInstanceIterator* pIter = m_iteratorPool.back();
        m_iteratorPool.pop_back();
        new (pIter) CInstanceIterator(this);
        return pIter;
    }
}
