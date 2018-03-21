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

#include "StdAfx.h"
#include "AiBehaviorLibrary.h"
#include "AiBehavior.h"

#include "IScriptSystem.h"

#define AI_CONFIG_FILE "Scripts\\AI\\aiconfig.lua"


//====================================================================
// DirectoryExplorer
//====================================================================
struct DirectoryExplorer
{
    template<typename Action>
    uint32 operator()(const char* rootFolderName, const char* extension, Action& action)
    {
        stack_string folder = rootFolderName;
        stack_string search = folder;
        search.append("/*.*");

        uint32 fileCount = 0;

        if (folder[folder.length() - 1] != '/')
        {
            folder += "/";
        }

        ICryPak* pak = gEnv->pCryPak;

        _finddata_t fd;
        intptr_t handle = pak->FindFirst(search.c_str(), &fd);

        if (handle > -1)
        {
            do
            {
                if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
                {
                    continue;
                }

                if (fd.attrib & _A_SUBDIR)
                {
                    stack_string subName = folder + fd.name;

                    fileCount += DirectoryExplorer()(subName.c_str(), extension, action);

                    continue;
                }

                if (_stricmp(PathUtil::GetExt(fd.name), PathUtil::GetExt(extension)))
                {
                    continue;
                }

                stack_string luaFile = folder + stack_string(fd.name);
                action(luaFile);

                ++fileCount;
            } while (pak->FindNext(handle, &fd) >= 0);
        }

        return fileCount;
    }
};


struct BehaviorExplorer
{
    BehaviorExplorer(const char* _createFuncName)
        : createFuncName(_createFuncName)
    {
        globalBehaviors.Create(gEnv->pScriptSystem, false);

        string fakeCreateCode =
            "local name, baseName = select(1, ...)\n"
            "if (type(baseName) ~= \"string\") then baseName = true end\n"
            "__DiscoveredBehavior[name] = baseName\n"
            "return {}\n";

        fakeCreateBehavior = SmartScriptFunction(gEnv->pScriptSystem,   gEnv->pScriptSystem->CompileBuffer(fakeCreateCode,
                    strlen(fakeCreateCode), "Fake CreateBehaviorCode"));

        assert(fakeCreateBehavior != 0);
    }

    struct BehaviorInfo
    {
        BehaviorInfo(const char* base, const char* file)
            : baseName(base)
            , fileName(file)
            , loaded(false)
        {
        }

        string baseName;
        string fileName;

        bool loaded;
    };

    typedef std::map<string, BehaviorInfo> Behaviors;
    Behaviors behaviors;

    void operator()(const char* fileName)
    {
        HSCRIPTFUNCTION createBehavior = gEnv->pScriptSystem->GetFunctionPtr(createFuncName.c_str());
        if (!createBehavior)
        {
            // error
            return;
        }

        gEnv->pScriptSystem->SetGlobalValue(createFuncName.c_str(), fakeCreateBehavior);

        const char* discoveredBehaviorName = "__DiscoveredBehavior";
        gEnv->pScriptSystem->SetGlobalValue(discoveredBehaviorName, globalBehaviors);

        globalBehaviors->Clear();

        if (gEnv->pScriptSystem->ExecuteFile(fileName, true, gEnv->IsEditor())) // Force reload if in editor mode
        {
            IScriptTable::Iterator it = globalBehaviors->BeginIteration();

            while (globalBehaviors->MoveNext(it))
            {
                if (it.key.type != ANY_TSTRING)
                {
                    continue;
                }

                const char* behaviorName = 0;
                it.key.CopyTo(behaviorName);

                const char* baseName = 0;
                if (it.value.type == ANY_TSTRING)
                {
                    it.value.CopyTo(baseName);
                }

                std::pair<Behaviors::iterator, bool> iresult = behaviors.insert(
                        Behaviors::value_type(behaviorName, BehaviorInfo(baseName, fileName)));

                if (!iresult.second)
                {
                    gEnv->pLog->LogError("[AI] Duplicate behavior definition '%s' in file '%s'. First seen in file '%s'.",
                        behaviorName, fileName, iresult.first->second.fileName.c_str());

                    continue;
                }
            }

            globalBehaviors->EndIteration(it);
        }

        gEnv->pScriptSystem->SetGlobalToNull(discoveredBehaviorName);
        gEnv->pScriptSystem->SetGlobalValue(createFuncName.c_str(), createBehavior);
        gEnv->pScriptSystem->ReleaseFunc(createBehavior);
    }

private:
    string createFuncName;
    SmartScriptTable globalBehaviors;
    SmartScriptFunction fakeCreateBehavior;
};


//////////////////////////////////////////////////////////////////////////
struct CAIBehaviorsDump
    : public IScriptTableDumpSink
{
    CAIBehaviorsDump(CAIBehaviorLibrary* lib, IScriptTable* obj)
    {
        m_pLibrary = lib;
        m_pScriptObject = obj;
    }

    virtual void OnElementFound(int nIdx, ScriptVarType type) {}
    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (type == svtString)
        {
            // New behavior.
            const char* scriptFile;
            if (m_pScriptObject->GetValue(sName, scriptFile))
            {
                CAIBehavior* behavior = new CAIBehavior;
                behavior->SetName(sName);
                QString file = scriptFile;
                file.replace('/', '\\');
                behavior->SetScript(file);
                behavior->SetDescription("");
                m_pLibrary->AddBehavior(behavior);
            }
        }
    }
private:
    CAIBehaviorLibrary* m_pLibrary;
    IScriptTable* m_pScriptObject;
};

//////////////////////////////////////////////////////////////////////////
struct CAICharactersDump
    : public IScriptTableDumpSink
{
    CAICharactersDump(CAIBehaviorLibrary* lib, IScriptTable* obj)
    {
        m_pLibrary = lib;
        m_pScriptObject = obj;
    }

    virtual void OnElementFound(int nIdx, ScriptVarType type) {}
    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (type == svtString)
        {
            // New behavior.
            const char* scriptFile;
            if (m_pScriptObject->GetValue(sName, scriptFile))
            {
                CAICharacter* c = new CAICharacter;
                c->SetName(sName);
                QString file = scriptFile;
                file.replace('/', '\\');
                c->SetScript(file);
                c->SetDescription("");
                m_pLibrary->AddCharacter(c);
            }
        }
    }
private:
    CAIBehaviorLibrary* m_pLibrary;
    IScriptTable* m_pScriptObject;
};

//////////////////////////////////////////////////////////////////////////
// CAIBehaviorLibrary implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CAIBehaviorLibrary::CAIBehaviorLibrary()
{
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::AddBehavior(CAIBehavior* behavior)
{
    CAIBehaviorPtr g;
    if (m_behaviors.Find(behavior->GetName(), g))
    {
        // Behavior with this name already exist in the library.
    }
    m_behaviors[behavior->GetName()] = behavior;
}


//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::RemoveBehavior(CAIBehavior* behavior)
{
    m_behaviors.Erase(behavior->GetName());
}

//////////////////////////////////////////////////////////////////////////
CAIBehavior* CAIBehaviorLibrary::FindBehavior(const QString& name) const
{
    CAIBehaviorPtr behavior = 0;
    m_behaviors.Find(name, behavior);
    return behavior;
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::ClearBehaviors()
{
    m_behaviors.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::LoadBehaviors(const QString& path)
{
    m_scriptsPath = path;
    ClearBehaviors();

    BehaviorExplorer behaviorExplorer("CreateAIBehavior");
    DirectoryExplorer()(path.toUtf8().data(), "*.lua", behaviorExplorer);

    BehaviorExplorer::Behaviors::iterator it = behaviorExplorer.behaviors.begin();
    BehaviorExplorer::Behaviors::iterator end = behaviorExplorer.behaviors.end();

    for (; it != end; ++it)
    {
        CAIBehavior* behavior = new CAIBehavior;
        behavior->SetName(it->first.c_str());
        behavior->SetScript(it->second.fileName.c_str());
        behavior->SetDescription(it->second.baseName.c_str());
        AddBehavior(behavior);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::GetBehaviors(std::vector<CAIBehaviorPtr>& behaviors)
{
    // Load behaviours again.
    LoadBehaviors(m_scriptsPath);
    m_behaviors.GetAsVector(behaviors);
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::ReloadScripts()
{
    LoadBehaviors(m_scriptsPath);

    int i;
    // Generate uniq set of script files used by behaviors.
    std::set<QString> scriptFiles;

    std::vector<CAIBehaviorPtr> behaviors;
    GetBehaviors(behaviors);
    for (i = 0; i < behaviors.size(); i++)
    {
        scriptFiles.insert(behaviors[i]->GetScript());
    }

    IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
    // Load script files to script system.
    for (std::set<QString>::iterator it = scriptFiles.begin(); it != scriptFiles.end(); it++)
    {
        QString file = *it;
        CLogFile::FormatLine("Loading AI Behavior Script: %s", file.toUtf8().data());
        scriptSystem->ExecuteFile(file.toUtf8().data());
    }

    // Reload main AI script.
    //  IScriptSystem *scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
    scriptSystem->ExecuteFile(AI_CONFIG_FILE, true, true);
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::LoadCharacters()
{
    m_characters.Clear();

    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable pAIBehaviorTable(pScriptSystem, true);
    if (pScriptSystem->GetGlobalValue("AICharacter", pAIBehaviorTable))
    {
        SmartScriptTable pAvailableTable(pScriptSystem, true);
        if (pAIBehaviorTable->GetValue("AVAILABLE", pAvailableTable))
        {
            // Enumerate all characters.
            CAICharactersDump bdump(this, pAvailableTable);
            pAvailableTable->Dump(&bdump);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::GetCharacters(std::vector<CAICharacterPtr>& characters)
{
    LoadCharacters();
    m_characters.GetAsVector(characters);
}

//////////////////////////////////////////////////////////////////////////
CAICharacter* CAIBehaviorLibrary::FindCharacter(const QString& name) const
{
    CAICharacterPtr chr = 0;
    m_characters.Find(name, chr);
    return chr;
}

//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::AddCharacter(CAICharacter* chr)
{
    CAICharacterPtr g;
    if (m_characters.Find(chr->GetName(), g))
    {
        // Behavior with this name already exist in the library.
    }
    m_characters[chr->GetName()] = chr;
}


//////////////////////////////////////////////////////////////////////////
void CAIBehaviorLibrary::RemoveCharacter(CAICharacter* chr)
{
    m_characters.Erase(chr->GetName());
}