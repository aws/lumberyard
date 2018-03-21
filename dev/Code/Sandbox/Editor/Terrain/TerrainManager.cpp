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

// Description : Manages layer and heightfield data.


#include <StdAfx.h>
#include "TerrainManager.h"

//////////////////////////////////////////////////////////////////////////
#include "SurfaceType.h"
#include "Layer.h"
#include "TerrainTexGen.h"
#include "Util/AutoLogTime.h"
#include "ITerrain.h"
#include "Terrain/TerrainConverter.h"

namespace {
    const char* kHeightmapFile = "Heightmap.dat";
    const char* kTerrainTextureFile = "TerrainTexture.xml";

    // world data block types
    enum EWDBType
    {
        WDB_LEVELGENERAL = DMAS_GENERAL,
        WDB_TERRAIN_LAYERS = DMAS_TERRAIN_LAYERS,
        WDB_VEGETATION = DMAS_VEGETATION,
        WDB_TIMEOFDAY = DMAS_TIME_OF_DAY,
        WDB_ENVIRONMENT = DMAS_ENVIRONMENT,
        WDB_LEVELNAMEDDATA = DMAS_GENERAL_NAMED_DATA,
        WDB_XMLCOUNT,
        //WDB_LEVELPAK = WDB_XMLCOUNT,
        WDB_COUNT = WDB_XMLCOUNT,
        WDB_UNSPECIFIED = -1,
    };
};



//////////////////////////////////////////////////////////////////////////
// CTerrainManager

//////////////////////////////////////////////////////////////////////////
CTerrainManager::CTerrainManager()
{
}

//////////////////////////////////////////////////////////////////////////
CTerrainManager::~CTerrainManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveSurfaceType(CSurfaceType* pSurfaceType)
{
    for (int i = 0; i < m_surfaceTypes.size(); i++)
    {
        if (m_surfaceTypes[i] == pSurfaceType)
        {
            m_surfaceTypes.erase(m_surfaceTypes.begin() + i);
            break;
        }
    }
    ConsolidateSurfaceTypes();
}

//////////////////////////////////////////////////////////////////////////
int CTerrainManager::AddSurfaceType(CSurfaceType* srf)
{
    GetIEditor()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    m_surfaceTypes.push_back(srf);

    return m_surfaceTypes.size() - 1;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveAllSurfaceTypes()
{
    GetIEditor()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    m_surfaceTypes.clear();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeSurfaceTypes(CXmlArchive& xmlAr, bool bUpdateEngineTerrain)
{
    if (xmlAr.bLoading)
    {
        // Clear old layers
        RemoveAllSurfaceTypes();

        // Load the layer count
        XmlNodeRef node = xmlAr.root->findChild("SurfaceTypes");
        if (!node)
        {
            return;
        }

        // Read all node
        for (int i = 0; i < node->getChildCount(); i++)
        {
            CXmlArchive ar(xmlAr);
            ar.root = node->getChild(i);
            CSurfaceType* sf = new CSurfaceType;
            sf->Serialize(ar);
            AddSurfaceType(sf);

            if (sf->GetSurfaceTypeID() == CLayer::e_undefined)  // For older levels.
            {
                sf->AssignUnusedSurfaceTypeID();
            }
        }
        ReloadSurfaceTypes(bUpdateEngineTerrain);
    }
    else
    {
        // Storing
        CLogFile::WriteLine("Storing surface types...");

        // Save the layer count

        XmlNodeRef node = xmlAr.root->newChild("SurfaceTypes");

        // Write all surface types.
        for (int i = 0; i < m_surfaceTypes.size(); i++)
        {
            CXmlArchive ar(xmlAr);
            ar.root = node->newChild("SurfaceType");
            m_surfaceTypes[i]->Serialize(ar);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ConsolidateSurfaceTypes()
{
    // We must consolidate the IDs after removing
    for (size_t i = 0; i < m_surfaceTypes.size(); ++i)
    {
        int id = i < CLayer::e_undefined ? i : CLayer::e_undefined;
        m_surfaceTypes[i]->SetSurfaceTypeID(id);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ReloadSurfaceTypes(bool bUpdateEngineTerrain, bool bUpdateHeightmap)
{
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    CXmlArchive ar;
    XmlNodeRef node = XmlHelpers::CreateXmlNode("SurfaceTypes_Editor");
    // Write all surface types.
    for (int i = 0; i < GetSurfaceTypeCount(); i++)
    {
        ar.root = node->newChild("SurfaceType");
        GetSurfaceTypePtr(i)->Serialize(ar);
    }

    gEnv->p3DEngine->LoadTerrainSurfacesFromXML(node, bUpdateEngineTerrain);

    if (bUpdateHeightmap && gEnv->p3DEngine->GetITerrain() && bUpdateEngineTerrain)
    {
        m_heightmap.UpdateEngineTerrain(false, false);
    }
}

//////////////////////////////////////////////////////////////////////////
int CTerrainManager::FindSurfaceType(const QString& name)
{
    for (int i = 0; i < m_surfaceTypes.size(); i++)
    {
        if (QString::compare(m_surfaceTypes[i]->GetName(), name, Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainManager::FindLayer(const char* sLayerName) const
{
    for (int i = 0; i < GetLayerCount(); i++)
    {
        CLayer* layer = GetLayer(i);
        if (QString::compare(layer->GetLayerName(), sLayerName) == 0)
        {
            return layer;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainManager::FindLayerByLayerId(const uint32 dwLayerId) const
{
    for (int i = 0; i < GetLayerCount(); i++)
    {
        CLayer* layer = GetLayer(i);
        if (layer->GetCurrentLayerId() == dwLayerId)
        {
            return layer;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveLayer(CLayer* layer)
{
    assert(m_layers.size() > 1 && "Removing last layer of terrain");

    if (layer && layer->GetCurrentLayerId() != CLayer::e_undefined)
    {
        uint32 id = layer->GetCurrentLayerId();

        if (id != CLayer::e_undefined)
        {
            // Find potential replacement for this layer.
            CLayer* defaultLayer = nullptr;
            for (CLayer* otherLayer : m_layers)
            {
                if (layer != otherLayer)
                {
                    defaultLayer = otherLayer;
                    break;
                }
            }
            m_heightmap.EraseLayerID(id, defaultLayer->GetCurrentLayerId());
        }
    }

    if (layer)
    {
        delete layer;
        m_layers.erase(std::remove(m_layers.begin(), m_layers.end(), layer), m_layers.end());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SwapLayers(int layer1, int layer2)
{
    assert(layer1 >= 0 && layer1 < m_layers.size());
    assert(layer2 >= 0 && layer2 < m_layers.size());
    std::swap(m_layers[layer1], m_layers[layer2]);

    // If necessary, also swap the surface types (their order is important when using masked surface type transitions)
    int st1 = m_layers[layer1]->GetEngineSurfaceTypeId();
    int st2 = m_layers[layer2]->GetEngineSurfaceTypeId();
    CSurfaceType* pSurfaceType1 = GetSurfaceTypePtr(st1);
    CSurfaceType* pSurfaceType2 = GetSurfaceTypePtr(st2);
    if (pSurfaceType1 && pSurfaceType2 && sgn(st1 - st2) == -sgn(layer1 - layer2))
    {
        CLayer TempLayer;
        TempLayer.SetSurfaceType(pSurfaceType1); // Temporarily holds the surface type so that it does not get deleted
        m_layers[layer1]->SetSurfaceType(pSurfaceType2);
        m_layers[layer2]->SetSurfaceType(pSurfaceType1);
        TempLayer.SetSurfaceType(NULL);
        CSurfaceType& surfaceType1 = *m_surfaceTypes[st1];
        CSurfaceType& surfaceType2 = *m_surfaceTypes[st2];
        std::swap(surfaceType1, surfaceType2);
        m_heightmap.UpdateEngineTerrain();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ClearLayers()
{
    ////////////////////////////////////////////////////////////////////////
    // Clear all texture layers
    ////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < GetLayerCount(); ++i)
    {
        delete GetLayer(i);
    }

    m_layers.clear();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeLayerSettings(CXmlArchive& xmlAr)
{
    ////////////////////////////////////////////////////////////////////////
    // Load or restore the layer settings from an XML
    ////////////////////////////////////////////////////////////////////////
    if (xmlAr.bLoading)
    {
        // Loading

        CLogFile::WriteLine("Loading layer settings...");
        QWaitCursor wait;

        // Clear old layers
        ClearLayers();

        // Load the layer count
        XmlNodeRef layers = xmlAr.root->findChild("Layers");
        if (!layers)
        {
            return;
        }

        // Read all layers
        int numLayers = layers->getChildCount();
        for (int i = 0; i < numLayers; i++)
        {
            // Create a new layer
            AddLayer(new CLayer);

            CXmlArchive ar(xmlAr);
            ar.root = layers->getChild(i);
            // Fill the layer with the data
            GetLayer(i)->Serialize(ar);

            CryLog("  loaded editor layer %d  name='%s' LayerID=%d", i, GetLayer(i)->GetLayerName().toUtf8().data(), GetLayer(i)->GetCurrentLayerId());
        }

        // If surface type ids are unassigned, assign them.
        for (int i = 0; i < GetSurfaceTypeCount(); i++)
        {
            if (GetSurfaceTypePtr(i)->GetSurfaceTypeID() >= CLayer::e_undefined)
            {
                GetSurfaceTypePtr(i)->AssignUnusedSurfaceTypeID();
            }
        }
    }
    else
    {
        // Storing

        CLogFile::WriteLine("Storing layer settings...");

        // Save the layer count

        XmlNodeRef layers = xmlAr.root->newChild("Layers");

        // Write all layers
        for (int i = 0; i < GetLayerCount(); i++)
        {
            CXmlArchive ar(xmlAr);
            ar.root = layers->newChild("Layer");
            ;
            GetLayer(i)->Serialize(ar);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
uint32 CTerrainManager::GetDetailIdLayerFromLayerId(const uint32 dwLayerId)
{
    for (int i = 0, num = (int)m_layers.size(); i < num; i++)
    {
        CLayer& rLayer = *(m_layers[i]);

        if (dwLayerId == rLayer.GetOrRequestLayerId())
        {
            return rLayer.GetEngineSurfaceTypeId();
        }
    }

    // recreate referenced layer
    if (!GetIEditor()->GetSystem()->GetI3DEngine()->GetITerrain())       // only if terrain loaded successfully
    {
        QString no;

        no = QString::number(dwLayerId);

        CLayer* pNewLayer = new CLayer;

        pNewLayer->SetLayerName(QString("LAYER_") + no);
        pNewLayer->SetLayerId(dwLayerId);

        AddLayer(pNewLayer);
    }

    return CLayer::e_undefined;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::MarkUsedLayerIds(bool bFree[CLayer::e_undefined]) const
{
    std::vector<CLayer*>::const_iterator it;

    for (it = m_layers.begin(); it != m_layers.end(); ++it)
    {
        CLayer* pLayer = *it;

        const uint32 id = pLayer->GetCurrentLayerId();

        if (id < CLayer::e_undefined)
        {
            bFree[id] = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::CreateDefaultLayer()
{
    // Create default layer.
    CLayer* layer = new CLayer;
    AddLayer(layer);
    layer->SetLayerName("Default");
    layer->LoadTexture("engineassets/textures/grey.dds");
    layer->SetLayerId(0);

    // Create default surface type.
    CSurfaceType* sfType = new CSurfaceType;
    sfType->SetName("Materials/material_terrain_default");
    uint32 dwDetailLayerId = AddSurfaceType(sfType);
    sfType->SetMaterial("Materials/material_terrain_default");
    sfType->AssignUnusedSurfaceTypeID();

    layer->SetSurfaceType(sfType);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeTerrain(TDocMultiArchive& arrXmlAr)
{
    bool bLoading = IsLoadingXmlArArray(arrXmlAr);

    if (bLoading)
    {
        m_heightmap.Serialize((*arrXmlAr[WDB_LEVELGENERAL]));

        // Surface Types ///////////////////////////////////////////////////////
        {
            CAutoLogTime logtime("Loading Surface Types");
            SerializeSurfaceTypes((*arrXmlAr[WDB_TERRAIN_LAYERS]));
        }

        // Terrain texture /////////////////////////////////////////////////////
        {
            CAutoLogTime logtime("Loading Terrain Layers Info");
            SerializeLayerSettings((*arrXmlAr[WDB_TERRAIN_LAYERS]));
        }

        {
            CAutoLogTime logtime("Load Terrain");

            m_heightmap.SerializeTerrain((*arrXmlAr[WDB_LEVELGENERAL]));
        }

        if (!m_heightmap.IsAllocated())
        {
            gEnv->pSystem->ShowMessage("Heightmap data wasn't properly loaded. The file is missing or corrupted. Using this level is not recommended. Update level data from your backup.", "Error", MB_OK | MB_ICONERROR);
        }
        else
        {
            CAutoLogTime logtime("Process RGB Terrain Layers");
            ConvertLayersToRGBLayer();
        }
    }
    else
    {
        if (arrXmlAr[WDB_LEVELGENERAL] != NULL)
        {
            // save terrain heightmap data
            m_heightmap.Serialize((*arrXmlAr[WDB_LEVELGENERAL]));

            m_heightmap.SerializeTerrain((*arrXmlAr[WDB_LEVELGENERAL]));
        }

        if (arrXmlAr[WDB_TERRAIN_LAYERS] != NULL)
        {
            // Surface Types
            SerializeSurfaceTypes((*arrXmlAr[WDB_TERRAIN_LAYERS]));

            SerializeLayerSettings((*arrXmlAr[WDB_TERRAIN_LAYERS]));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeTerrain(CXmlArchive& xmlAr)
{
    if (xmlAr.bLoading)
    {
        m_heightmap.Serialize(xmlAr);

        // Surface Types ///////////////////////////////////////////////////////
        {
            CAutoLogTime logtime("Loading Surface Types");
            SerializeSurfaceTypes(xmlAr);
        }

        // Terrain texture /////////////////////////////////////////////////////
        {
            CAutoLogTime logtime("Loading Terrain Layers Info");
            SerializeLayerSettings(xmlAr);
        }

        {
            CAutoLogTime logtime("Load Terrain");
            m_heightmap.SerializeTerrain(xmlAr);
        }

        if (!m_heightmap.IsAllocated())
        {
            gEnv->pSystem->ShowMessage("Heightmap data wasn't properly loaded. The file is missing or corrupted. Using this level is not recommended. Update level data from your backup.", "Error", MB_OK | MB_ICONERROR);
        }
        else
        {
            CAutoLogTime logtime("Process RGB Terrain Layers");
            ConvertLayersToRGBLayer();
        }
    }
    else
    {
        // save terrain heightmap data
        m_heightmap.Serialize(xmlAr);

        m_heightmap.SerializeTerrain(xmlAr);

        // Surface Types
        SerializeSurfaceTypes(xmlAr);

        SerializeLayerSettings(xmlAr);
    }
}


//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeTexture(CXmlArchive& xmlAr)
{
    GetRGBLayer()->Serialize(xmlAr.root, xmlAr.bLoading);
}


//////////////////////////////////////////////////////////////////////////
void    CTerrainManager::GetTerrainMemoryUsage(ICrySizer* pSizer)
{
    {
        SIZER_COMPONENT_NAME(pSizer, "Layers");

        std::vector<CLayer*>::iterator it;

        for (it = m_layers.begin(); it != m_layers.end(); ++it)
        {
            CLayer* pLayer = *it;

            pLayer->GetMemoryUsage(pSizer);
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "CHeightmap");

        m_heightmap.GetMemoryUsage(pSizer);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "RGBLayer");

        GetRGBLayer()->GetMemoryUsage(pSizer);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::ConvertLayersToRGBLayer()
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SetTerrainSize(int resolution, int unitSize)
{
    m_heightmap.Resize(resolution, resolution, unitSize);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SetUseTerrain(bool useTerrain)
{
    m_heightmap.SetUseTerrain(useTerrain);
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::GetUseTerrain()
{
    return m_heightmap.GetUseTerrain();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ResetHeightMap()
{
    const int resolution = 1024;
    const int unitSize = 2;
    m_heightmap.Reset(resolution, unitSize);
}

//////////////////////////////////////////////////////////////////////////
CRGBLayer* CTerrainManager::GetRGBLayer()
{
    return m_heightmap.GetRGBLayer();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::Save()
{
    CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kHeightmapFile).toUtf8().data());

    CXmlArchive xmlAr;
    SerializeTerrain(xmlAr);
    xmlAr.Save(helper.GetTempFilePath());

    helper.UpdateFile(false);
}


//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::Load()
{
    QString filename = GetIEditor()->GetLevelDataFolder() + kHeightmapFile;
    CXmlArchive xmlAr;
    xmlAr.bLoading = true;
    if (!xmlAr.Load(filename))
    {
        return false;
    }

    // Check terrain files and convert if necessary
    TerrainConverter::Config config;
    config.macroTextureXMLFilename = kTerrainTextureFile;
    config.macroTexturePakFilename = "TerrainTexture.pak";
    config.macroTextureQuadtreeFilename = COMPILED_TERRAIN_TEXTURE_FILE_NAME;
    TerrainConverter::ConvertTerrain(config);

    SerializeTerrain(xmlAr);
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SaveTexture()
{
    if (!GetIEditor()->GetTerrainManager()->GetRGBLayer()->WouldSaveSucceed())
    {
        CLogFile::WriteLine("Terrain texture can not be saved!");
        return;
    }

    CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kTerrainTextureFile).toUtf8().data());

    XmlNodeRef root;
    GetRGBLayer()->Serialize(root, false);
    if (root)
    {
        root->saveToFile(helper.GetTempFilePath().toUtf8().data());
    }

    helper.UpdateFile(false);
}


//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::LoadTexture()
{
    QString filename = GetIEditor()->GetLevelDataFolder() + kTerrainTextureFile;
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
    if (root)
    {
        GetRGBLayer()->Serialize(root, true);
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SetModified(int x1, int y1, int x2, int y2)
{
    if (!gEnv->p3DEngine->GetITerrain())
    {
        return;
    }

    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    AABB bounds;
    bounds.Reset();
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
    {
        bounds.Reset();
    }
    else
    {
        // Here we are making sure that we will update the whole sectors where the heightmap was changed.
        int nTerrainSectorSize(gEnv->p3DEngine->GetTerrainSectorSize());
        assert(nTerrainSectorSize > 0);

        x1 *= m_heightmap.GetUnitSize();
        y1 *= m_heightmap.GetUnitSize();

        x2 *= m_heightmap.GetUnitSize();
        y2 *= m_heightmap.GetUnitSize();

        x1 /= nTerrainSectorSize;
        y1 /= nTerrainSectorSize;
        x2 /= nTerrainSectorSize;
        y2 /= nTerrainSectorSize;

        // Y and X switched by historical reasons.
        bounds.Add(Vec3((y1 - 1) * nTerrainSectorSize, (x1 - 1) * nTerrainSectorSize, -32000.0f));
        bounds.Add(Vec3((y2 + 1) * nTerrainSectorSize, (x2 + 1) * nTerrainSectorSize, +32000.0f));
    }
}

QString CTerrainManager::GenerateUniqueLayerName(const QString& name) const
{
    const auto LayerNameExists = [this](const QString& name)
    {
        auto it = std::find_if(std::begin(m_layers), std::end(m_layers),
                               [&name](const CLayer* layer) { return layer->GetLayerName() == name; });
        return it != std::end(m_layers);
    };

    if (!LayerNameExists(name))
    {
        return name;
    }

    // remove digits at the end
    auto lastDigit = std::find_if(name.rbegin(), name.rend(),
                                  [](const QChar ch) { return !ch.isDigit(); });
    const QString baseName = name.left(std::distance(name.begin(), lastDigit.base()));

    // eventually we'll stop
    int lastNumber = 1;
    while (true) {
        QString newName = QStringLiteral("%1%2").arg(baseName).arg(lastNumber);
        if (!LayerNameExists(newName))
        {
            return newName;
        }
        ++lastNumber;
    }
}
