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
#ifndef CRYINCLUDE_CRYACTION_PREFABMANAGER_H
#define CRYINCLUDE_CRYACTION_PREFABMANAGER_H
#pragma once

#include <IPrefabManager.h>
#include "ILevelSystem.h"

class CRuntimePrefab;
class CPrefab;

typedef std::map<EntityId, std::shared_ptr<CRuntimePrefab> > RuntimePrefabMap;
typedef RuntimePrefabMap::iterator RuntimePrefabMapIterator;

//////////////////////////////////////////////////////////////////////////
struct BrushParams
{
    BrushParams()
        : m_renderNodeFlags(0)
        , m_isDecal(false)
        , m_viewDistRatio(100)
        , m_lodRatio(100)
        , m_deferredDecal(false)
        , m_sortPriority(16)
        , m_depth(1.0f)
        , m_binaryDesignerObjectVersion(0)
    {
    }

    string                      m_filename;
    string                      m_material;
    uint32                      m_renderNodeFlags;
    int                         m_lodRatio;
    int                         m_viewDistRatio;
    Matrix34                    m_worldMatrix;

    bool                        m_isDecal;
    int                         m_projectionType;
    bool                        m_deferredDecal;
    int                         m_sortPriority;
    float                       m_depth;
    int                         m_binaryDesignerObjectVersion;
    std::vector<char>           m_binaryDesignerObject;
};

//////////////////////////////////////////////////////////////////////////
struct EntityParams
{
    SEntitySpawnParams          m_spawnParams;
    XmlNodeRef                  m_entityNode;
    Matrix34                    m_worldMatrix;
};


//////////////////////////////////////////////////////////////////////////
struct PrefabParams
{
    Matrix34                    m_worldMatrix;
    string                      m_fullName;
    std::shared_ptr<CPrefab>    m_prefab; // recursive prefabs
};

//////////////////////////////////////////////////////////////////////////
class CPrefab
{
public:

    explicit CPrefab(const string& sName)
        : m_name(sName) {}
    CPrefab() = default;
    ~CPrefab() = default;

    void Load(XmlNodeRef& itemNode);

    const string& GetName() const { return m_name; }
    std::vector<EntityParams> m_entityParamsList;
    std::vector<BrushParams> m_brushParamsList;
    std::vector<PrefabParams> m_prefabParamsList;

protected:

    void ExtractTransform(XmlNodeRef& objNode, Matrix34& mat);
    bool ExtractBrushLoadParams(XmlNodeRef& objNode, BrushParams& loadParams);
    bool ExtractPrefabLoadParams(XmlNodeRef& objNode, PrefabParams& loadParams);
    bool ExtractDecalLoadParams(XmlNodeRef& objNode, BrushParams& loadParams);
    bool ExtractEntityLoadParams(XmlNodeRef& objNode, SEntitySpawnParams& loadParams);
    bool ExtractDesignerLoadParams(XmlNodeRef& objName, BrushParams& loadParams);

    string m_name;
};

typedef std::map<string, std::shared_ptr<CPrefab> > PrefabMap;
typedef PrefabMap::iterator PrefabMapIterator;

//////////////////////////////////////////////////////////////////////////
class CPrefabLib
{
public:
    CPrefabLib(const string& sFilename, const string& sName)
        : m_name(sName)
        , m_filename(sFilename)
    {
    }
    ~CPrefabLib();

    void AddPrefab(const std::shared_ptr<CPrefab>& prefab);
    std::shared_ptr<CPrefab> GetPrefab(const string& name);

    string m_name;
    string m_filename;

    PrefabMap m_prefabs;
};



//////////////////////////////////////////////////////////////////////////
class CPrefabManager
    : public IPrefabManager
    , ILevelSystemListener
{
    typedef std::map<string, std::shared_ptr<CPrefabLib> > PrefabLibMap;
    typedef PrefabLibMap::iterator PrefabLibMapIterator;
public:
    CPrefabManager();
    virtual ~CPrefabManager();

    // ILevelSystemListener
    void OnLoadingStart(ILevelInfo* level) override;
    void OnLoadingComplete(ILevel* level) override;

    // IPrefabManager
    bool LoadPrefabLibrary(const string& filename) override;
    void SpawnPrefab(const string& libraryName, const string& prefabName, EntityId id, uint32 seed, uint32 maxSpawn) override;
    void MovePrefab(EntityId id) override;
    void DeletePrefab(EntityId id) override;
    void HidePrefab(EntityId id, bool hide) override;
    void Clear(bool deleteLibs = true) override;

    // called during level generation process
    void CallOnSpawn(IEntity* entity, int seed);

    std::shared_ptr<CPrefab> FindPrefab(const std::shared_ptr<CPrefabLib>& prefabLib, const string& prefabName, bool loadIfNotFound = false);

    void SetPrefabGroup(const string& groupName);

protected:

    std::shared_ptr<CPrefab> GetPrefab(const string& libraryName, const string& prefabName);

    std::shared_ptr<CPrefabLib> GetPrefabLib(const string& libraryName);
    std::shared_ptr<CRuntimePrefab> GetRuntimePrefab(EntityId id);

    PrefabLibMap m_prefabLibs;
    RuntimePrefabMap m_runtimePrefabs;

private:

    void StripLibraryName(const string& libraryName, const string& prefabName, string& result, bool applyGroup);
    std::shared_ptr<CPrefab> FindPrefabInGroup(const std::shared_ptr<CPrefabLib>& prefabLibrary, const string& prefabName);

    std::map<std::shared_ptr<CPrefab>, int> m_instanceCounts;
    string m_lastPrefab;

    string m_currentGroup;
};

#endif //CRYINCLUDE_CRYACTION_PREFABMANAGER_H
