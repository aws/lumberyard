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
#include "PrefabManager.h"
#include "RuntimePrefab.h"
#include "Base64.h"


//////////////////////////////////////////////////////////////////////////
void CPrefab::ExtractTransform(XmlNodeRef& objNode, Matrix34& mat)
{
    // parse manually
    Vec3 pos(Vec3_Zero);
    Quat rot(Quat::CreateIdentity());
    Vec3 scale(Vec3_One);

    objNode->getAttr("Pos", pos);
    objNode->getAttr("Rotate", rot);
    objNode->getAttr("Scale", scale);

    // local transformation inside the prefab
    mat.Set(scale, rot, pos);
}

//////////////////////////////////////////////////////////////////////////
bool CPrefab::ExtractBrushLoadParams(XmlNodeRef& objNode, BrushParams& loadParams)
{
    ExtractTransform(objNode, loadParams.m_worldMatrix);

    const char* fileName = objNode->getAttr("Prefab");
    if (fileName)
    {
        loadParams.m_filename = string(fileName);
    }

    // flags already combined, can skip parsing lots of nodes
    objNode->getAttr("RndFlags", loadParams.m_renderNodeFlags);
    objNode->getAttr("LodRatio", loadParams.m_lodRatio);
    objNode->getAttr("ViewDistRatio", loadParams.m_viewDistRatio);

    const char* materialName = objNode->getAttr("Material");
    if (materialName && materialName[0])
    {
        loadParams.m_material = string(materialName);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefab::ExtractDecalLoadParams(XmlNodeRef& objNode, BrushParams& loadParams)
{
    ExtractTransform(objNode, loadParams.m_worldMatrix);

    const char* materialName = objNode->getAttr("Material");
    if (materialName && materialName[0])
    {
        loadParams.m_material = string(materialName);
    }

    loadParams.m_isDecal = true;

    int projectionType(SDecalProperties::ePlanar);
    objNode->getAttr("ProjectionType", projectionType);
    loadParams.m_projectionType = projectionType;

    objNode->getAttr("RndFlags", loadParams.m_renderNodeFlags);
    objNode->getAttr("Deferred", loadParams.m_deferredDecal);
    objNode->getAttr("ViewDistRatio", loadParams.m_viewDistRatio);
    objNode->getAttr("LodRatio", loadParams.m_lodRatio);
    objNode->getAttr("SortPriority", loadParams.m_sortPriority);
    objNode->getAttr("ProjectionDepth", loadParams.m_depth);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefab::ExtractPrefabLoadParams(XmlNodeRef& objNode, PrefabParams& loadParams)
{
    ExtractTransform(objNode, loadParams.m_worldMatrix);

    const char* prefabName = objNode->getAttr("PrefabName");
    if (prefabName)
    {
        loadParams.m_fullName = string(prefabName);
    }

    loadParams.m_prefab = nullptr;

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefab::ExtractEntityLoadParams(XmlNodeRef& objNode, SEntitySpawnParams& loadParams)
{
    if (!gEnv->pEntitySystem->ExtractEntityLoadParams(objNode, loadParams))
    {
        return false;
    }

    // this property is exported by Sandbox with different names in the Editor XML and in the game XML, so we need to check for it manually
    bool noStaticDecals = false;
    objNode->getAttr("NoStaticDecals", noStaticDecals);
    if (noStaticDecals)
    {
        loadParams.nFlags |= ENTITY_FLAG_NO_DECALNODE_DECALS;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefab::ExtractDesignerLoadParams(XmlNodeRef& objNode, BrushParams& loadParams)
{
    ExtractTransform(objNode, loadParams.m_worldMatrix);

    objNode->getAttr("RndFlags", loadParams.m_renderNodeFlags);
    objNode->getAttr("ViewDistRatio", loadParams.m_viewDistRatio);
    const char* materialName = objNode->getAttr("Material");
    if (materialName && materialName[0])
    {
        loadParams.m_material = string(materialName);
    }

    XmlNodeRef meshNode = objNode->findChild("Mesh");
    if (meshNode)
    {
        loadParams.m_binaryDesignerObjectVersion = 0;
        meshNode->getAttr("Version", loadParams.m_binaryDesignerObjectVersion);
        const char* encodedStr;
        if (meshNode->getAttr("BinaryData", &encodedStr))
        {
            int nLength = strlen(encodedStr);
            if (nLength > 0)
            {
                unsigned int destBufferLen = Base64::decodedsize_base64(nLength);
                loadParams.m_binaryDesignerObject.resize(destBufferLen, 0);
                Base64::decode_base64(&loadParams.m_binaryDesignerObject[0], encodedStr, nLength, false);
                return true;
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefab::Load(XmlNodeRef& itemNode)
{
    m_name = itemNode->getAttr("Name");
    XmlNodeRef objects = itemNode->findChild("Objects");
    if (objects)
    {
        int numObjects = objects->getChildCount();
        for (int i = 0; i < numObjects; i++)
        {
            XmlNodeRef objNode = objects->getChild(i);
            const char* type = objNode->getAttr("Type");

            // try to parse entities first
            SEntitySpawnParams loadParams;
            if (ExtractEntityLoadParams(objNode, loadParams))
            {
                EntityParams entityParams;
                entityParams.m_spawnParams = loadParams;
                entityParams.m_entityNode = objNode->clone();

                // local entity transformation inside the prefab
                entityParams.m_worldMatrix.Set(entityParams.m_spawnParams.vScale, entityParams.m_spawnParams.qRotation, entityParams.m_spawnParams.vPosition);

                m_entityParamsList.push_back(entityParams);
            }
            else if (azstricmp(type, "Brush") == 0)
            {
                // a brush? Parse manually
                BrushParams brushParams;
                if (ExtractBrushLoadParams(objNode, brushParams))
                {
                    m_brushParamsList.push_back(brushParams);
                }
            }
            else if (azstricmp(type, "Decal") == 0)
            {
                BrushParams brushParams;
                if (ExtractDecalLoadParams(objNode, brushParams))
                {
                    m_brushParamsList.push_back(brushParams);
                }
            }
            else if (azstricmp(type, "Prefab") == 0)
            {
                // Sometimes people are placing prefabs inside prefabs.
                // If that is the case, store a reference to the prefab and expand after the loading is done
                // (because sometimes the prefab definition is stored in the library after the declaration...)
                PrefabParams prefabParams;
                if (ExtractPrefabLoadParams(objNode, prefabParams))
                {
                    m_prefabParamsList.push_back(prefabParams);
                }
            }
            else if (azstricmp(type, "Designer") == 0)
            {
                BrushParams brushParams;
                if (ExtractDesignerLoadParams(objNode, brushParams))
                {
                    m_brushParamsList.push_back(brushParams);
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CPrefabLib::~CPrefabLib()
{
    m_prefabs.clear();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabLib::AddPrefab(const std::shared_ptr<CPrefab>& prefab)
{
    m_prefabs[prefab->GetName()] = prefab;
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CPrefab> CPrefabLib::GetPrefab(const string& name)
{
    PrefabMapIterator prefabIterator = m_prefabs.find(name);
    if (prefabIterator == m_prefabs.end())
    {
        return nullptr;
    }
    return prefabIterator->second;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CPrefabManager::CPrefabManager()
{
    CRY_ASSERT_MESSAGE(gEnv->pGame, "Game has not been initialized");
    CRY_ASSERT_MESSAGE(gEnv->pGame->GetIGameFramework(), "CryAction or other game framework has not been initialized");
    if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        CRY_ASSERT_MESSAGE(gEnv->pGame->GetIGameFramework()->GetILevelSystem(), "Unable to register as levelsystem listener!");
        if (gEnv->pGame->GetIGameFramework()->GetILevelSystem())
        {
            gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CPrefabManager::~CPrefabManager()
{
    if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        if (gEnv->pGame->GetIGameFramework()->GetILevelSystem())
        {
            gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
        }
    }
    Clear();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::Clear(bool deleteLibs)
{
    if (deleteLibs)
    {
        m_prefabLibs.clear();
    }

    m_runtimePrefabs.clear();

    m_lastPrefab.clear();
    m_instanceCounts.clear();
    m_currentGroup.clear();
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CPrefab> CPrefabManager::GetPrefab(const string& libraryName, const string& prefabName)
{
    std::shared_ptr<CPrefabLib> pLib = GetPrefabLib(libraryName);
    if (pLib)
    {
        return pLib->GetPrefab(prefabName);
    }
    else
    {
        return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CPrefabLib> CPrefabManager::GetPrefabLib(const string& libraryName)
{
    PrefabLibMapIterator prefabLibIterator = m_prefabLibs.find(libraryName);
    if (prefabLibIterator == m_prefabLibs.end())
    {
        return nullptr;
    }
    return prefabLibIterator->second;
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CRuntimePrefab> CPrefabManager::GetRuntimePrefab(EntityId id)
{
    auto prefab = m_runtimePrefabs.find(id);
    if (prefab != m_runtimePrefabs.end())
    {
        return prefab->second;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::OnLoadingStart(ILevelInfo* level)
{
    // reset existing prefabs before loading the level
    Clear(false);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::OnLoadingComplete(ILevel* level)
{
    int seed = 0;
    ICVar* cvar = gEnv->pConsole->GetCVar("g_SessionSeed");
    if (cvar)
    {
        seed = cvar->GetIVal();
    }

    // spawn prefabs
    {
        IEntityItPtr entityIteratorPtr = gEnv->pEntitySystem->GetEntityIterator();
        while (!entityIteratorPtr->IsEnd())
        {
            IEntity* entity = entityIteratorPtr->Next();
            if (strncmp(entity->GetClass()->GetName(), "ProceduralObject", 16) == 0)
            {
                const Vec3& position = entity->GetPos();

                CallOnSpawn(entity, seed);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabManager::LoadPrefabLibrary(const string& filename)
{
    if (m_prefabLibs.find(filename) != m_prefabLibs.end())
    {
        return true;
    }

    XmlNodeRef xmlLibRoot = gEnv->pSystem->LoadXmlFromFile(filename);
    if (xmlLibRoot == 0)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, 0, 0, "File %s failed to load", filename.c_str());
        return false;
    }

    const char* libraryName = xmlLibRoot->getAttr("Name");

    if (!libraryName)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, 0, 0, "Cannot find library name for %s", filename.c_str());
        return false;
    }

    auto prefabLibrary = std::make_shared<CPrefabLib>(filename, libraryName);

    for (int i = 0; i < xmlLibRoot->getChildCount(); i++)
    {
        XmlNodeRef itemNode = xmlLibRoot->getChild(i);
        // Only accept nodes with correct name.
        if (azstricmp(itemNode->getTag(), "Prefab") != 0)
        {
            continue;
        }

        std::shared_ptr<CPrefab> prefab(new CPrefab());
        prefab->Load(itemNode);

        prefabLibrary->AddPrefab(prefab);
    }

    m_prefabLibs[prefabLibrary->m_filename] = prefabLibrary;

    // verify embedded prefabs if any was found
    for (const auto& kv : prefabLibrary->m_prefabs)
    {
        std::shared_ptr<CPrefab> prefab = kv.second;
        for (PrefabParams& prefabParams : prefab->m_prefabParamsList)
        {
            // check if the prefab item exists inside the library and set the reference
            prefabParams.m_prefab = FindPrefab(prefabLibrary, prefabParams.m_fullName, true);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::SetPrefabGroup(const string& groupName)
{
    m_currentGroup = groupName;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::StripLibraryName(const string& libraryName, const string& prefabName, string& result, bool applyGroup)
{
    // strip the library name if it was added (for example it happens when automatically converting from prefab to procedural object)
    result = prefabName;

    int curPos = 0;
    string libName = prefabName.Tokenize(".", curPos);
    if (libName == libraryName)
    {
        result = string(&prefabName.c_str()[curPos]);
    }

    if (applyGroup && !m_currentGroup.empty())
    {
        curPos = 0;
        string group = prefabName.Tokenize(".", curPos);
        if (group == "Default")
        {
            // if this is in the default group, adapt to the currently selected group, if any
            result = m_currentGroup + (".") + string(&prefabName.c_str()[curPos]);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CPrefab> CPrefabManager::FindPrefabInGroup(const std::shared_ptr<CPrefabLib>& prefabLibrary, const string& prefabName)
{
    string result;

    // try with the group variation first (Voodoo etc.)
    StripLibraryName(prefabLibrary->m_name, prefabName, result, true);
    PrefabMapIterator prefabIterator = prefabLibrary->m_prefabs.find(result);
    if (prefabIterator != prefabLibrary->m_prefabs.end())
    {
        return prefabIterator->second;
    }

    // If not found try without the group variation, check if the default group has it
    StripLibraryName(prefabLibrary->m_name, prefabName, result, false);
    prefabIterator = prefabLibrary->m_prefabs.find(result);
    if (prefabIterator != prefabLibrary->m_prefabs.end())
    {
        return prefabIterator->second;
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<CPrefab> CPrefabManager::FindPrefab(const std::shared_ptr<CPrefabLib>& prefabLib, const string& prefabName, bool loadIfNotFound /*= false*/)
{
    std::shared_ptr<CPrefab> prefab = FindPrefabInGroup(prefabLib, prefabName);

    // if the prefab was not found, try to see if the prefab is referencing a prefab from a completely different library.
    if (!prefab && loadIfNotFound)
    {
        int curPos = 0;
        string libraryName = prefabName.Tokenize(".", curPos);
        if (!libraryName.empty())
        {
            // try to access this library.
            // check if the library exists
            string libraryFileName = "Prefabs/" + libraryName + ".xml";
            PrefabLibMapIterator prefabLibIterator = m_prefabLibs.find(libraryFileName);
            if (prefabLibIterator == m_prefabLibs.end())
            {
                // try to load it
                if (!LoadPrefabLibrary(libraryFileName))
                {
                    return nullptr;
                }
                prefabLibIterator = m_prefabLibs.find(libraryFileName);
                CRY_ASSERT(prefabLibIterator != m_prefabLibs.end());
            }

            // check if the item is there now in the new loaded library.
            std::shared_ptr<CPrefabLib> prefabLibrary = prefabLibIterator->second;
            prefab = FindPrefabInGroup(prefabLibrary, prefabName);
        }
    }

    return prefab;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::SpawnPrefab(const string& libraryName, const string& prefabName, EntityId id, uint32 seed, uint32 maxSpawn)
{
    // check if the library exists
    std::shared_ptr<CPrefabLib> prefabLibrary = GetPrefabLib(libraryName);
    if (prefabLibrary == nullptr)
    {
        return;
    }

    // check if the prefab item exists inside the library
    std::shared_ptr<CPrefab> prefab = FindPrefab(prefabLibrary, prefabName);
    if (prefab == nullptr)
    {
        // cannot find this prefab, something went wrong (probably during editing a prefab was deleted and the xml not updated yet)
        return;
    }

    // check if we have a limit on number of spawns
    if (maxSpawn > 0)
    {
        std::map<std::shared_ptr<CPrefab>, int>::iterator occurencesIterator = m_instanceCounts.find(prefab);
        if (occurencesIterator != m_instanceCounts.end())
        {
            if (occurencesIterator->second > maxSpawn)
            {
                return;
            }
            occurencesIterator->second = occurencesIterator->second + 1;
        }
        else
        {
            m_instanceCounts[prefab] = 1;
        }
    }

    // find the runtime prefab associated to this entity id (the entity that spawned this prefab)
    std::shared_ptr<CRuntimePrefab> runtimePrefab = GetRuntimePrefab(id);
    if (runtimePrefab == nullptr)
    {
        runtimePrefab = std::make_shared<CRuntimePrefab>(id);
        m_runtimePrefabs[id] = runtimePrefab;
    }

    runtimePrefab->Spawn(prefab);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::MovePrefab(EntityId id)
{
    auto runtimePrefab = GetRuntimePrefab(id);
    if (runtimePrefab != nullptr)
    {
        runtimePrefab->Move();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::DeletePrefab(EntityId id)
{
    RuntimePrefabMapIterator runtimePrefabIterator = m_runtimePrefabs.find(id);
    if (runtimePrefabIterator != m_runtimePrefabs.end())
    {
        const std::shared_ptr<CRuntimePrefab>& runtimePrefab = runtimePrefabIterator->second;

        std::map<std::shared_ptr<CPrefab>, int>::iterator pOccCount = m_instanceCounts.find(runtimePrefab->GetSourcePrefab());
        if (pOccCount != m_instanceCounts.end())
        {
            pOccCount->second = max(0, pOccCount->second - 1);
        }

        m_runtimePrefabs.erase(runtimePrefabIterator);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::HidePrefab(EntityId id, bool bHide)
{
    auto runtimePrefab = GetRuntimePrefab(id);
    if (runtimePrefab != nullptr)
    {
        runtimePrefab->Hide(bHide);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::CallOnSpawn(IEntity* pEntity, int nSeed)
{
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (!pScriptTable)
    {
        return;
    }

    if ((pScriptTable->GetValueType("Spawn") == svtFunction) && pScriptTable->GetScriptSystem()->BeginCall(pScriptTable, "Spawn"))
    {
        pScriptTable->GetScriptSystem()->PushFuncParam(pScriptTable);
        pScriptTable->GetScriptSystem()->PushFuncParam(nSeed);
        pScriptTable->GetScriptSystem()->EndCall();
    }
}
