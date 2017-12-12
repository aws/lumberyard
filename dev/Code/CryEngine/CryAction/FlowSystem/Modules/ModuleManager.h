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

// Description : Manages module loading and application


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_MODULES_MODULEMANAGER_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_MODULES_MODULEMANAGER_H
#pragma once

#include <IFlowGraphModuleManager.h>
#include <IGameFramework.h>
#include "CryListenerSet.h"

struct SActivationInfo;
class CFlowGraphModule;
class CFlowModuleCallNode;

class CFlowGraphModuleManager
    : public IFlowGraphModuleManager
    , public ISystemEventListener
    , public IGameFrameworkListener
{
public:
    ////////////////////////////////////////////////////
    CFlowGraphModuleManager();
    virtual ~CFlowGraphModuleManager();

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

    // IFlowGraphModuleManager
    virtual bool RegisterListener(IFlowGraphModuleListener* pListener, const char* name);
    virtual void UnregisterListener(IFlowGraphModuleListener* pListener);

    virtual void OnDeleteModuleXML(const char* moduleName, const char* fileName);
    virtual void OnRenameModuleXML(const char* moduleName, const char* newName);

    virtual bool SaveModule(const char* moduleName, XmlNodeRef saveTo);
    IFlowGraphModule* LoadModuleFile(const char* moduleName, const char* fileName, IFlowGraphModule::EType type);

    virtual IModuleIteratorPtr CreateModuleIterator();

    virtual const char* GetStartNodeName(const char* moduleName) const;
    virtual const char* GetReturnNodeName(const char* moduleName) const;
    virtual const char* GetCallerNodeName(const char* moduleName) const;
    virtual void ScanForModules();
    virtual const char* GetModulePath(const char* name);
    virtual bool CreateModuleNodes(const char* moduleName, TModuleId moduleId);

    virtual IFlowGraphModule* GetModule(IFlowGraphPtr pFlowgraph) const;
    virtual IFlowGraphModule* GetModule(const char* moduleName) const;
    virtual IFlowGraphModule* GetModule(TModuleId id) const;
    virtual TModuleInstanceId CreateModuleInstance(TModuleId moduleId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback);
    virtual TModuleInstanceId CreateModuleInstance(TModuleId moduleId, TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback);
    // ~IFlowGraphModuleManager

    // IGameFrameworkListener
    virtual void OnPostUpdate(float fDeltaTime);
    virtual void OnSaveGame(ISaveGame* pSaveGame) {}
    virtual void OnLoadGame(ILoadGame* pLoadGame) {}
    virtual void OnLevelEnd(const char* nextLevel) {}
    virtual void OnActionEvent(const SActionEvent& event) {}
    // ~IGameFrameworkListener

    void Shutdown();

    void ClearModules(); // Unload all loaded modules
    void RemoveCompletedModuleInstances();  // cleanup at end of update

    bool AddModulePathInfo(const char* moduleName, const char* path);

    void RefreshModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId, TModuleParams const& params);
    void CancelModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId);

    // OnModuleFinished: Called when a module is done executing
    void OnModuleFinished(TModuleId const& moduleId, TModuleInstanceId instanceId, bool bSuccess, TModuleParams const& params);


    DeclareStaticConstIntCVar(CV_fg_debugmodules, 0);
    ICVar* fg_debugmodules_filter;

private:
    CFlowGraphModuleManager(CFlowGraphModuleManager const&)
        : m_listeners(1) {}
    CFlowGraphModuleManager& operator =(CFlowGraphModuleManager const&) {return *this; }
    void RescanModuleNames(IFlowGraphModule::EType type);
    void ScanFolder(const string& folderName, IFlowGraphModule::EType type);

    CFlowGraphModule* PreLoadModuleFile(const char* moduleName, const char* fileName, IFlowGraphModule::EType type);
    void LoadModuleGraph(const char* moduleName, const char* fileName);

    void DestroyActiveModuleInstances();

    // Module Id map
    typedef std::map<string, TModuleId> TModuleIdMap;
    TModuleIdMap m_ModuleIds;

    // Loaded modules
    typedef std::map<TModuleId, CFlowGraphModule*> TModuleMap;
    TModuleMap m_Modules;
    TModuleId m_moduleIdMaker;

    typedef std::map<string, string> TModulesPathInfo;
    TModulesPathInfo m_ModulesPathInfo;

    CListenerSet<IFlowGraphModuleListener*> m_listeners;

    class CModuleIterator
        : public IFlowGraphModuleIterator
    {
    public:
        CModuleIterator(CFlowGraphModuleManager* pMM)
        {
            m_pModuleManager = pMM;
            m_cur = m_pModuleManager->m_Modules.begin();
            m_nRefs = 0;
        }
        void AddRef()
        {
            ++m_nRefs;
        }
        void Release()
        {
            if (--m_nRefs <= 0)
            {
                this->~CModuleIterator();
                m_pModuleManager->m_iteratorPool.push_back(this);
            }
        }
        IFlowGraphModule* Next();
        size_t Count()
        {
            return m_pModuleManager->m_Modules.size();
        }
        CFlowGraphModuleManager* m_pModuleManager;
        TModuleMap::iterator m_cur;
        int m_nRefs;
    };

    std::vector<CModuleIterator*> m_iteratorPool;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_MODULES_MODULEMANAGER_H
