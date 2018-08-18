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


#include "CryLegacy_precompiled.h"
#include "ModuleManager.h"
#include "Module.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "FlowModuleNodes.h"
#include "ILevelSystem.h"

#define MODULE_FOLDER_NAME ("\\FlowgraphModules\\")

#if !defined (_RELEASE)
void RenderModuleDebugInfo();
#endif

AllocateConstIntCVar(CFlowGraphModuleManager, CV_fg_debugmodules);

//////////////////////////////////////////////////////////////////////////
IFlowGraphModule* CFlowGraphModuleManager::CModuleIterator::Next()
{
    if (m_cur == m_pModuleManager->m_Modules.end())
    {
        return NULL;
    }
    IFlowGraphModule* pCur = m_cur->second;
    ++m_cur;
    return pCur;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphModuleManager::CFlowGraphModuleManager()
    : m_listeners(1)
{
    m_moduleIdMaker = 0;
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    DefineConstIntCVarName("fg_debugmodules", CV_fg_debugmodules, 0, VF_NULL,   "Display Module debug info.\n" \
        "0=Disabled"                                                                                           \
        "1=Modules only"                                                                                       \
        "2=Modules + Module Instances");
    fg_debugmodules_filter = REGISTER_STRING("fg_debugmodules_filter", "",  VF_NULL, "Only debug modules with this name");

#if !defined (_RELEASE)
    CRY_ASSERT_MESSAGE(gEnv->pGame->GetIGameFramework(), "Unable to register as Framework listener!");
    if (gEnv->pGame->GetIGameFramework())
    {
        gEnv->pGame->GetIGameFramework()->RegisterListener(this, "FlowGraphModuleManager", FRAMEWORKLISTENERPRIORITY_GAME);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphModuleManager::~CFlowGraphModuleManager()
{
    Shutdown();
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

    gEnv->pConsole->UnregisterVariable("fg_debugmodules_filter", true);

#if !defined (_RELEASE)
    if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::Shutdown()
{
    ClearModules();
    m_listeners.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::ClearModules()
{
    TModuleMap::iterator i = m_Modules.begin();
    TModuleMap::iterator end = m_Modules.end();
    for (; i != end; ++i)
    {
        if (i->second)
        {
            for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
            {
                notifier->OnModuleDestroyed(i->second);
            }

            i->second->Destroy();
            SAFE_DELETE(i->second);

            for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
            {
                notifier->OnPostModuleDestroyed();
            }
        }
    }
    m_Modules.clear();
    m_ModuleIds.clear();
    m_ModulesPathInfo.clear();
    m_moduleIdMaker = 0;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphModule* CFlowGraphModuleManager::PreLoadModuleFile(const char* moduleName, const char* fileName, IFlowGraphModule::EType type)
{
    // NB: the module name passed in might be a best guess based on the filename. The actual name
    // comes from within the module xml.

    // first check for existing module
    CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));

    if (pModule)
    {
        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnModuleDestroyed(pModule);
        }
        // exists, reload
        pModule->Destroy();
        pModule->PreLoadModule(fileName);

        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnPostModuleDestroyed();
        }
    }
    else
    {
        // not found, create

        pModule = new CFlowGraphModule(m_moduleIdMaker++);
        pModule->SetType(type);

        TModuleId id = pModule->GetId();
        m_Modules[id] = pModule;

        pModule->PreLoadModule(fileName);
        AddModulePathInfo(pModule->GetName(), fileName);

        m_ModuleIds[pModule->GetName()] = id;
    }

    return pModule;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::LoadModuleGraph(const char* moduleName, const char* fileName)
{
    // first check for existing module - must exist by this point
    CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));

    assert(pModule);

    if (pModule)
    {
        if (pModule->LoadModuleGraph(moduleName, fileName))
        {
            for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
            {
                notifier->OnRootGraphChanged(pModule);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphModule* CFlowGraphModuleManager::LoadModuleFile(const char* moduleName, const char* fileName, IFlowGraphModule::EType type)
{
    // first check for existing module
    CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));

    if (pModule)
    {
        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnModuleDestroyed(pModule);
        }

        pModule->Destroy();

        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnPostModuleDestroyed();
        }
    }

    pModule = PreLoadModuleFile(moduleName, fileName, type);
    LoadModuleGraph(moduleName, fileName);

    return pModule;
}

TModuleInstanceId CFlowGraphModuleManager::CreateModuleInstance(TModuleId moduleId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback)
{
    return CreateModuleInstance(moduleId, InvalidFlowGraphId, InvalidFlowNodeId, params, returnCallback);
}

//////////////////////////////////////////////////////////////////////////
TModuleInstanceId CFlowGraphModuleManager::CreateModuleInstance(TModuleId moduleId, TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback)
{
    TModuleInstanceId result = MODULEINSTANCE_INVALID;

    // Get module container
    CFlowGraphModule* pModule = NULL;
    if (moduleId != MODULEID_INVALID)
    {
        TModuleMap::iterator moduleEntry = m_Modules.find(moduleId);
        if (m_Modules.end() != moduleEntry)
        {
            pModule = moduleEntry->second;
        }
    }
    if (!pModule)
    {
        assert(false);
    }

    // Make instance
    if (pModule)
    {
        result = pModule->CreateInstance(callerGraphId, callerNodeId, params, returnCallback);

        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnModuleInstanceCreated(pModule, result);
        }

        pModule->ActivateGraph(pModule->GetInstanceGraph(result), result, params);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::RefreshModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId, TModuleParams const& params)
{
    // Get module container
    CFlowGraphModule* pModule = NULL;
    if (moduleId != MODULEID_INVALID)
    {
        TModuleMap::iterator moduleEntry = m_Modules.find(moduleId);
        if (m_Modules.end() != moduleEntry)
        {
            pModule = moduleEntry->second;
        }
    }
    if (!pModule)
    {
        assert(false);
    }

    // Refresh instance
    if (pModule)
    {
        pModule->RefreshInstance(instanceId, params);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::CancelModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId)
{
    // Get module container
    CFlowGraphModule* pModule = NULL;
    if (moduleId != MODULEID_INVALID)
    {
        TModuleMap::iterator moduleEntry = m_Modules.find(moduleId);
        if (m_Modules.end() != moduleEntry)
        {
            pModule = moduleEntry->second;
        }
    }
    if (!pModule)
    {
        assert(false);
    }

    // Cancel instance
    if (pModule)
    {
        pModule->CancelInstance(instanceId);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::OnModuleFinished(TModuleId const& moduleId, TModuleInstanceId instanceId, bool bSuccess, TModuleParams const& params)
{
    TModuleMap::iterator moduleEntry = m_Modules.find(moduleId);
    if (m_Modules.end() != moduleEntry)
    {
        CFlowGraphModule* pModule = moduleEntry->second;
        if (pModule)
        {
            for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
            {
                notifier->OnModuleInstanceDestroyed(GetModule(pModule->GetId()), instanceId);
            }
            pModule->DestroyInstance(instanceId, bSuccess, params);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_START:
    {
        ScanForModules();
    }
    break;
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
    {
        if (gEnv->pGame->GetIGameFramework()->GetILevelSystem()->IsLevelLoaded())
        {
            ClearModules();
        }
    }
    break;
    case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
    case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
    {
        if (wparam == 0)
        {
            DestroyActiveModuleInstances();
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::OnDeleteModuleXML(const char* moduleName, const char* fileName)
{
    TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
    if (m_ModulesPathInfo.end() == modulePathEntry)
    {
        return;
    }

    m_ModulesPathInfo.erase(moduleName);

    // Remove the module itself
    TModuleIdMap::iterator idIt = m_ModuleIds.find(moduleName);
    assert(idIt != m_ModuleIds.end());
    TModuleId id = idIt->second;
    TModuleMap::iterator modIt = m_Modules.find(id);
    assert(modIt != m_Modules.end());

    if (modIt != m_Modules.end() && idIt != m_ModuleIds.end())
    {
        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnModuleDestroyed(GetModule(id));
        }

        m_Modules[id]->Destroy();
        delete m_Modules[id];
        m_Modules[id] = nullptr;

        m_ModuleIds.erase(idIt);
        m_Modules.erase(modIt);

        if (m_Modules.empty())
        {
            m_moduleIdMaker = 0;
        }

        for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnPostModuleDestroyed();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::OnRenameModuleXML(const char* moduleName, const char* newName)
{
    TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
    if (m_ModulesPathInfo.end() != modulePathEntry)
    {
        string newNameWithPath = PathUtil::GetPath(modulePathEntry->second);
        newNameWithPath += newName;
        newNameWithPath += ".xml";

        m_ModulesPathInfo.insert(TModulesPathInfo::value_type(PathUtil::GetFileName(newNameWithPath), newNameWithPath));
        m_ModulesPathInfo.erase(moduleName);
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CFlowGraphModuleManager::GetStartNodeName(const char* moduleName) const
{
    static CryFixedStringT<64> temp;
    temp.Format("Module:Start_%s", moduleName);
    return temp.c_str();
}

//////////////////////////////////////////////////////////////////////////
const char* CFlowGraphModuleManager::GetReturnNodeName(const char* moduleName) const
{
    static CryFixedStringT<64> temp;
    temp.Format("Module:End_%s", moduleName);
    return temp.c_str();
}

//////////////////////////////////////////////////////////////////////////
const char* CFlowGraphModuleManager::GetCallerNodeName(const char* moduleName) const
{
    static CryFixedStringT<64> temp;
    temp.Format("Module:Call_%s", moduleName);
    return temp.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::ScanFolder(const string& folderName, IFlowGraphModule::EType type)
{
    _finddata_t fd;
    intptr_t handle = 0;
    ICryPak* pPak = gEnv->pCryPak;

    CryFixedStringT<512> searchString = folderName.c_str();
    searchString.append("*.*");

    handle = pPak->FindFirst(searchString.c_str(), &fd);

    CryFixedStringT<512> moduleName("");
    string newFolder("");

    if (handle > -1)
    {
        do
        {
            if (!strcmp(fd.name, ".") || !strcmp(fd.name, "..") || (fd.attrib & _A_HIDDEN))
            {
                continue;
            }

            if (fd.attrib & _A_SUBDIR)
            {
                newFolder = folderName;
                newFolder = newFolder + fd.name;
                newFolder = newFolder + "\\";
                ScanFolder(newFolder, type);
            }
            else
            {
                moduleName = fd.name;
                if (!azstricmp(PathUtil::GetExt(moduleName.c_str()), "xml"))
                {
                    PathUtil::RemoveExtension(moduleName);
                    PathUtil::MakeGamePath(folderName);

                    // initial load: creates module, registers nodes
                    CFlowGraphModule* pModule = PreLoadModuleFile(moduleName.c_str(), PathUtil::GetPath(folderName) + fd.name, type);
                    // important: the module should be added using its internal name rather than the filename
                    m_ModulesPathInfo.insert(TModulesPathInfo::value_type(pModule->GetName(), PathUtil::GetPath(folderName) + fd.name));
                }
            }
        } while (pPak->FindNext(handle, &fd) >= 0);

        pPak->FindClose(handle);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::RescanModuleNames(IFlowGraphModule::EType type)
{
    CryFixedStringT<512> path = "";

    switch (type)
    {
    case IFlowGraphModule::eT_Global:
        path = "Libs\\";
        break;
    case IFlowGraphModule::eT_Level:
        if (gEnv->IsEditor())
        {
            char* levelName;
            char* levelPath;
            gEnv->pGame->GetIGameFramework()->GetEditorLevel(&levelName, &levelPath);
            path = levelPath;
        }
        else
        {
            ILevel* pLevel = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();
            if (pLevel)
            {
                path = pLevel->GetLevelInfo()->GetPath();
            }
        }
        break;
    default:
        CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
        break;
    }

    if (false == path.empty())
    {
        switch (type)
        {
        case IFlowGraphModule::eT_Global:
        case IFlowGraphModule::eT_Level:
            // Intentional fall-through since these both behave the same way here.
            path += MODULE_FOLDER_NAME;
            break;
        default:
            CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
            break;
        }

        ScanFolder(path, type);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::ScanForModules()
{
    // first remove any existing modules
    ClearModules();

    // during scanning, modules will be PreLoaded - created, and nodes registered. This ensures that when we actually
    //  load the graphs, the nodes already exist - even if one module contains the nodes relating to another module.
    RescanModuleNames(IFlowGraphModule::eT_Global);
    RescanModuleNames(IFlowGraphModule::eT_Level);

    // Second pass: loading the graphs, now all nodes should exist.
    for (TModulesPathInfo::const_iterator it = m_ModulesPathInfo.begin(), end = m_ModulesPathInfo.end(); it != end; ++it)
    {
        LoadModuleGraph(it->first, it->second);
    }

    for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnScannedForModules();
    }
}

//////////////////////////////////////////////////////////////////////////
IModuleIteratorPtr CFlowGraphModuleManager::CreateModuleIterator()
{
    if (m_iteratorPool.empty())
    {
        return new CModuleIterator(this);
    }
    else
    {
        CModuleIterator* pIter = m_iteratorPool.back();
        m_iteratorPool.pop_back();
        new (pIter) CModuleIterator(this);
        return pIter;
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CFlowGraphModuleManager::GetModulePath(const char* name)
{
    if (m_ModulesPathInfo.empty())
    {
        return NULL;
    }

    TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(name);
    if (m_ModulesPathInfo.end() != modulePathEntry)
    {
        return modulePathEntry->second;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModuleManager::AddModulePathInfo(const char* moduleName, const char* path)
{
    TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
    if (m_ModulesPathInfo.end() != modulePathEntry)
    {
        return false;
    }

    m_ModulesPathInfo.insert(TModulesPathInfo::value_type(moduleName, path));
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModuleManager::SaveModule(const char* moduleName, XmlNodeRef saveTo)
{
    TModuleId moduleId = stl::find_in_map(m_ModuleIds, moduleName, MODULEID_INVALID);
    if (moduleId != MODULEID_INVALID)
    {
        CFlowGraphModule* pModule = stl::find_in_map(m_Modules, moduleId, NULL);
        if (pModule)
        {
            pModule->SaveModuleXml(saveTo);
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphModuleManager::CreateModuleNodes(const char* moduleName, TModuleId moduleId)
{
    assert(moduleId != MODULEID_INVALID);

    CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleId));
    if (pModule)
    {
        pModule->RegisterNodes();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphModule* CFlowGraphModuleManager::GetModule(TModuleId moduleId) const
{
    return stl::find_in_map(m_Modules, moduleId, NULL);
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphModule* CFlowGraphModuleManager::GetModule(const char* moduleName) const
{
    TModuleId id = stl::find_in_map(m_ModuleIds, moduleName, MODULEID_INVALID);
    return GetModule(id);
}

IFlowGraphModule* CFlowGraphModuleManager::GetModule(IFlowGraphPtr pFlowgraph) const
{
    for (TModuleMap::const_iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
    {
        CFlowGraphModule* pModule = it->second;

        if (pModule && pModule->HasInstanceGraph(pFlowgraph))
        {
            return pModule;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphModuleManager::RemoveCompletedModuleInstances()
{
    for (TModuleMap::iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
    {
        it->second->RemoveCompletedInstances();
    }
}

bool CFlowGraphModuleManager::RegisterListener(IFlowGraphModuleListener* pListener, const char* name)
{
    return m_listeners.Add(pListener, name);
}

void CFlowGraphModuleManager::UnregisterListener(IFlowGraphModuleListener* pListener)
{
    m_listeners.Remove(pListener);
}

void CFlowGraphModuleManager::OnPostUpdate(float fDeltaTime)
{
#if !defined (_RELEASE)
    if (CV_fg_debugmodules > 0)
    {
        RenderModuleDebugInfo();
    }
#endif
}

void CFlowGraphModuleManager::DestroyActiveModuleInstances()
{
    for (TModuleMap::const_iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
    {
        if (CFlowGraphModule* pModule = it->second)
        {
            pModule->RemoveAllInstances();
            pModule->RemoveCompletedInstances();
        }
    }
}

#if !defined (_RELEASE)
void DrawModule2dLabel(float x, float y, float fontSize, const float* pColor, const char* pText)
{
    SDrawTextInfo ti;
    ti.xscale = ti.yscale = fontSize;
    ti.flags = eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
    if (pColor)
    {
        ti.color[0] = pColor[0];
        ti.color[1] = pColor[1];
        ti.color[2] = pColor[2];
        ti.color[3] = pColor[3];
    }
    gEnv->pRenderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, pText);
}


void DrawModuleTextLabel(float x, float y, const float* pColor, const char* pFormat, ...)
{
    char buffer[512];
    const size_t cnt = sizeof(buffer);

    va_list args;
    va_start(args, pFormat);
    int written = azvsnprintf(buffer, cnt, pFormat, args);
    if (written < 0 || written == cnt)
    {
        buffer[cnt - 1] = '\0';
    }
    va_end(args);

    DrawModule2dLabel(x, y, 1.4f, pColor, buffer);
}

void RenderModuleDebugInfo()
{
    CRY_ASSERT_MESSAGE(gEnv->pFlowSystem, "No Flowsystem available!");

    if (!gEnv->pFlowSystem || !gEnv->pFlowSystem->GetIModuleManager())
    {
        return;
    }

    IModuleIteratorPtr pIter = gEnv->pFlowSystem->GetIModuleManager()->CreateModuleIterator();
    if (!pIter)
    {
        return;
    }

    const int count = pIter->Count();

    if (count == 0)
    {
        static const float colorRed[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        DrawModuleTextLabel(10, 20, colorRed, "NO FLOWGRAPH MODULES AVAILABLE!");
        return;
    }

    if (IFlowGraphModule* pModule = pIter->Next())
    {
        float py = 15;
        const float dy = 15;
        const float dy_space = 5;
        static const float colorWhite[4] =    {1.0f, 1.0f, 1.0f, 1.0f};
        static const float colorBlue[4] =     {0.3f, 0.8f, 1.0f, 1.0f};
        static const float colorGray[4] =     {0.3f, 0.3f, 0.3f, 1.0f};
        static const float colorGreen[4] =    {0.3f, 1.0f, 0.8f, 1.0f};

        const float col1 = 10;
        const float col2 = 320;
        const float col3 = 380;
        const float col4 = 500;
        const float col5 = 600;
        const float col6 = 850;

        DrawModuleTextLabel(col1, py, colorWhite, "Module");
        DrawModuleTextLabel(col2, py, colorWhite, "ID");
        DrawModuleTextLabel(col3, py, colorWhite, "Num Instances");
        DrawModuleTextLabel(col4, py, colorWhite, "Instance ID");
        DrawModuleTextLabel(col5, py, colorWhite, "Caller Graph - Node");
        DrawModuleTextLabel(col6, py, colorWhite, "Type");

        py += dy + dy_space;

        for (int i = 0; i < count; ++i)
        {
            string filter = gEnv->pConsole->GetCVar("fg_debugmodules_filter")->GetString();
            string moduleName = pModule->GetName();
            moduleName.MakeLower();
            filter.MakeLower();

            if (!filter.empty() && moduleName.find(filter) == string::npos)
            {
                pModule = pIter->Next();
                continue;
            }

            if (IModuleInstanceIteratorPtr instanceIter = pModule->CreateInstanceIterator())
            {
                IModuleInstance* pInstance = instanceIter->Next();

                DrawModuleTextLabel(col1, py, pInstance ? colorBlue : colorGray, "%s", pModule->GetName());
                DrawModuleTextLabel(col2, py, pInstance ? colorGreen : colorGray, "%d", pModule->GetId());
                DrawModuleTextLabel(col6, py, colorGreen, "%s", IFlowGraphModule::GetTypeName(pModule->GetType()));

                if (pInstance)
                {
                    const int numInstances = instanceIter->Count();

                    DrawModuleTextLabel(col3, py, colorGreen, "%d", numInstances);

                    if (CFlowGraphModuleManager::CV_fg_debugmodules == 2)
                    {
                        for (int j = 0; j < numInstances; ++j)
                        {
                            py += dy;
                            DrawModuleTextLabel(col4, py, colorGreen, "%d", pInstance->instanceId);
                            DrawModuleTextLabel(col5, py, colorGreen, "Graph ID: %d - Node ID: %d", pInstance->callerGraph, pInstance->callerNode);

                            pInstance = instanceIter->Next();
                        }
                    }
                }
                else
                {
                    DrawModuleTextLabel(col3, py, colorGray, "0");
                    DrawModuleTextLabel(col4, py, colorGray, "-");
                }
            }

            py += dy;
            pModule = pIter->Next();
        }
    }
}
#endif
