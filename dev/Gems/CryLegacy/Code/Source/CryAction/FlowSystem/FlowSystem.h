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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEM_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEM_H
#pragma once

#include "IFlowSystem.h"
#include "FlowSystemCVars.h"
#include "CryListenerSet.h"
#include "IEntitySystem.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

class CFlowGraphBase;
class CFlowGraphModuleManager;
class IFlowInitManager;

struct CFlowSystemContainer
    : public IFlowSystemContainer
{
    virtual void AddItem(TFlowInputData item);
    virtual void AddItemUnique(TFlowInputData item);

    virtual void RemoveItem(TFlowInputData item);

    virtual TFlowInputData GetItem(int i);

    virtual void RemoveItemAt(int i);
    virtual int GetItemCount() const;

    virtual void Clear();

    virtual void GetMemoryUsage(ICrySizer* s) const;

    virtual void Serialize(TSerialize ser);
private:
    DynArray<TFlowInputData> m_container;
};

class CFlowSystem
    : public IFlowSystem
    , public IEntitySystemSink
    , public IEntityClassRegistryListener
    , public ISystemEventListener
{
public:
    CFlowSystem();
    ~CFlowSystem();

    // IFlowSystem
    virtual void Release();
    virtual void Update();
    virtual void Reset(bool unload);
    virtual void ReloadAllNodeTypes();
    virtual IFlowGraphPtr CreateFlowGraph();
    virtual TFlowNodeTypeId RegisterType(const char* type, IFlowNodeFactoryPtr factory);
    void RegisterNodeInfo(const char* type, IFlowNodeFactoryPtr factory);
    virtual bool UnregisterType(const char* typeName);
    virtual const char* GetTypeName(TFlowNodeTypeId typeId);
    virtual TFlowNodeTypeId GetTypeId(const char* typeName);
    virtual IFlowNodeTypeIteratorPtr CreateNodeTypeIterator();
    virtual void RegisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
    virtual void UnregisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
    virtual void EnableInspecting(bool bEnable) { m_bInspectingEnabled = bEnable; }
    virtual bool IsInspectingEnabled() const { return m_bInspectingEnabled; }
    virtual IFlowGraphInspectorPtr GetDefaultInspector() const { return m_pDefaultInspector; }
    virtual IFlowGraph* GetGraphById(TFlowGraphId graphId);
    virtual void OnEntityIdChanged(FlowEntityId oldId, FlowEntityId newId);
    virtual void GetMemoryUsage(ICrySizer* s) const;


    virtual bool CreateContainer(TFlowSystemContainerId id);
    virtual void DeleteContainer(TFlowSystemContainerId id);
    virtual IFlowSystemContainerPtr GetContainer(TFlowSystemContainerId id);

    virtual void Serialize(TSerialize ser);
    virtual TFlowGraphId RegisterGraph(IFlowGraphPtr pGraph, const char* debugName);
    virtual void UnregisterGraph(IFlowGraphPtr pGraph);

    void Uninitialize() override;
    // ~IFlowSystem

    // TODO Make a single point of entry for this and the AIProxyManager to share?
    // IEntitySystemSink
    virtual bool OnBeforeSpawn(SEntitySpawnParams& params) { return true; }
    virtual void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params);
    virtual bool OnRemove(IEntity* pEntity) { return true; }
    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event) { }
    //~IEntitySystemSink

    // IEntityClassRegistryListener
    void OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass) override;
    // ~IEntityClassRegistryListener

    // ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener
    IFlowNodePtr CreateNodeOfType(IFlowNode::SActivationInfo*, TFlowNodeTypeId typeId);
    void NotifyCriticalLoadingError() { m_criticalLoadingErrorHappened = true; }

    void PreInit();
    void Init();
    void Shutdown();
    void Enable(bool enable){m_cVars.m_enableUpdates = enable; }

    TFlowGraphId RegisterGraph(CFlowGraphBase* pGraph, const char* debugName);
    void UnregisterGraph(CFlowGraphBase* pGraph);

    CFlowGraphModuleManager* GetModuleManager();
    const CFlowGraphModuleManager* GetModuleManager() const;

    IFlowGraphModuleManager* GetIModuleManager();

    // resembles IFlowGraphInspector currently
    void NotifyFlow(CFlowGraphBase* pGraph, const SFlowAddress from, const SFlowAddress to);
    void NotifyProcessEvent(CFlowGraphBase* pGraph, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo, IFlowNode* pImpl);

    struct STypeInfo
    {
        STypeInfo()
            : name("")
            , factory(NULL) {}
        STypeInfo(const string& typeName, IFlowNodeFactoryPtr pFactory)
            : name(typeName)
            , factory(pFactory) {}
        string name;
        IFlowNodeFactoryPtr factory;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
        }
    };

#ifndef _RELEASE
    struct TSFGProfile
    {
        int graphsUpdated;
        int nodeActivations;
        int nodeUpdates;

        void Reset()
        {
            memset(this, 0, sizeof(*this));
        }
    };
    static TSFGProfile FGProfile;
#endif //_RELEASE

    const STypeInfo& GetTypeInfo(TFlowNodeTypeId typeId) const;

    virtual string GetUINameFromClassTag(const string& classTag) const;

    virtual string GetClassTagFromUIName(const string& uiName) const;
    void SetClassTagForUIName(const char* uiName, const char* className);
private:
    typedef std::queue<XmlNodeRef> TPendingComposites;

    void LoadExtensions(string path);
    void LoadExtensionFromXml(XmlNodeRef xml, TPendingComposites* pComposites);
    void LoadComposites(TPendingComposites* pComposites);

    void RegisterAllNodeTypes();
    void RegisterAutoTypes();
    void RegisterEntityTypes();
    void TryToRegisterEntityClass(IEntityClass* entityClass);
    TFlowNodeTypeId GenerateNodeTypeID();

    void LoadBlacklistedFlownodeXML();
    bool BlacklistedFlownode(const char** editorName);

    void UpdateGraphs();

private:
    // Inspecting enabled/disabled
    bool m_bInspectingEnabled;
    bool m_needToUpdateForwardings;
    bool m_criticalLoadingErrorHappened;

    class CNodeTypeIterator;
    typedef std::map<string, TFlowNodeTypeId> TTypeNameToIdMap;
    TTypeNameToIdMap m_typeNameToIdMap;
    std::vector<STypeInfo> m_typeRegistryVec; // 0 is invalid
    typedef CListenerSet<CFlowGraphBase*> TGraphs;
    TGraphs m_graphs;
    std::vector<IFlowGraphInspectorPtr> m_systemInspectors; // only inspectors which watch all graphs

    std::vector<TFlowNodeTypeId> m_freeNodeTypeIDs;
    TFlowNodeTypeId m_nextNodeTypeID;
    IFlowGraphInspectorPtr m_pDefaultInspector;

    CFlowSystemCVars m_cVars;

    TFlowGraphId m_nextFlowGraphId;

    CFlowGraphModuleManager* m_pModuleManager;

    XmlNodeRef m_blacklistNode;

    typedef std::map<TFlowSystemContainerId, IFlowSystemContainerPtr> TFlowSystemContainerMap;
    TFlowSystemContainerMap m_FlowSystemContainers;

    AZStd::unique_ptr<IFlowInitManager> m_flowInitManager;
};

inline CFlowGraphModuleManager* CFlowSystem::GetModuleManager()
{
    return m_pModuleManager;
}

////////////////////////////////////////////////////
inline const CFlowGraphModuleManager* CFlowSystem::GetModuleManager() const
{
    return m_pModuleManager;
}

/*!
Describes the different types of entities this node may work with
*/
enum class FlowEntityType
{
    Invalid,
    Legacy,
    Component
};

/*!
Identifies the type of an entity according to a given Id
@param id An entity id
@return The type of the entity
*/
FlowEntityType GetFlowEntityTypeFromFlowEntityId(FlowEntityId id);

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEM_H
