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


#ifndef CRYINCLUDE_EDITOR_TERRAIN_TERRAINMANAGER_H
#define CRYINCLUDE_EDITOR_TERRAIN_TERRAINMANAGER_H
#pragma once


//////////////////////////////////////////////////////////////////////////
// Dependencies
#include <Terrain/Bus/LegacyTerrainBus.h>
#include "Terrain/Heightmap.h"
#include "DocMultiArchive.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
class CLayer;
class CSurfaceType;
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Class
class CTerrainManager
    : public LegacyTerrain::LegacyTerrainEditorDataRequestBus::Handler
{
public:
    CTerrainManager();
    virtual ~CTerrainManager();

    //////////////////////////////////////////////////////////////////////////
    // Surface Types.
    //////////////////////////////////////////////////////////////////////////
    // Returns:
    //   can be 0 if the id does not exit
    CSurfaceType* GetSurfaceTypePtr(int i) const
    {
        if (i >= 0 && i < m_surfaceTypes.size())
        {
            return m_surfaceTypes[i];
        }
        return 0;
    }
    int  GetSurfaceTypeCount() const { return m_surfaceTypes.size(); }
    //! Find surface type by name, return -1 if name not found.
    int FindSurfaceType(const QString& name);
    void RemoveSurfaceType(CSurfaceType* pSrfType);
    int  AddSurfaceType(CSurfaceType* srf);
    void RemoveAllSurfaceTypes();
    void SerializeSurfaceTypes(CXmlArchive& xmlAr, bool bUpdateEngineTerrain = true);
    void ConsolidateSurfaceTypes();

    /** Put surface types from Editor to game.
    */
    void ReloadSurfaceTypes(bool bUpdateEngineTerrain = true, bool bUpdateHeightmap = true);

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Layers
    int GetLayerCount() const { return m_layers.size(); };
    CLayer* GetLayer(int layer) const { return layer < GetLayerCount() ? m_layers[layer] : nullptr; };
    //! Find layer by name.
    CLayer* FindLayer(const char* sLayerName) const;
    CLayer* FindLayerByLayerId(const uint32 dwLayerId) const;
    void SwapLayers(int layer1, int layer2);
    void MoveLayer(int oldIndex, int newIndex);
    void AddLayer(CLayer* layer);
    void RemoveLayer(CLayer* layer);
    void ClearLayers();
    void SerializeLayerSettings(CXmlArchive& xmlAr);
    void MarkUsedLayerIds(bool bFree[CLayer::e_undefined]) const;

    void CreateDefaultLayer();

    // slow
    // Returns:
    //   0xffffffff if the method failed (internal error)
    uint32 GetDetailIdLayerFromLayerId(const uint32 dwLayerId);

    // needed to convert from old layer structure to the new one
    bool ConvertLayersToRGBLayer();

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Heightmap
    CHeightmap* GetHeightmap() { return &m_heightmap; };
    CRGBLayer* GetRGBLayer();

    void SetTerrainSize(int resolution, int unitSize);
    void ResetHeightMap();

    void SetUseTerrain(bool useTerrain);
    bool GetUseTerrain();

    void Save();
    bool Load();

    void SaveTexture();
    bool LoadTexture();

    void SerializeTerrain(TDocMultiArchive& arrXmlAr);
    void SerializeTerrain(CXmlArchive& xmlAr);
    void SerializeTexture(CXmlArchive& xmlAr);

    void SetModified(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0);

    void GetTerrainMemoryUsage(ICrySizer* pSizer);

    QString GenerateUniqueLayerName(const QString& name) const;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // LegacyTerrain::LegacyTerrainEditorDataRequestBus
    //////////////////////////////////////////////////////////////////////////
    bool CreateTerrainSystemFromEditorData() override;
    void DestroyTerrainSystem() override;
    void RefreshEngineMacroTexture() override;
    int GetTerrainSurfaceIdFromSurfaceTag(AZ::Crc32 tag) override;
    //////////////////////////////////////////////////////////////////////////

protected:
    std::vector<_smart_ptr<CSurfaceType> > m_surfaceTypes;
    std::vector<CLayer*> m_layers;
    CHeightmap m_heightmap;
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_TERRAINMANAGER_H
