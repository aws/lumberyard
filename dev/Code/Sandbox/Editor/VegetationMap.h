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

#ifndef CRYINCLUDE_EDITOR_VEGETATIONMAP_H
#define CRYINCLUDE_EDITOR_VEGETATIONMAP_H

#pragma once

#include <AzToolsFramework/API/EditorVegetationRequestsBus.h>

#define RAD2BYTE(x) ((x) * 255.0f / float(g_PI2))
#define BYTE2RAD(x) ((x) * float(g_PI2) / 255.0f)

// Magic number restriction to prevent crash (https://jira.agscollab.com/browse/LY-19536)
// should eventually be able to remove this once vegetation is componentized and
// refactored to be streamed in as needed
#define MAX_VEGETATION_INSTANCES 2000000


class CVegetationObject;
struct CVegetationInstance;
//////////////////////////////////////////////////////////////////////////
/** CVegetationMap stores static objects distributed over terrain.
        It keeps a list of all allocated geometry objects and all places instances.
*/
class CVegetationMap
    : public AzToolsFramework::EditorVegetation::EditorVegetationRequestsBus::Handler
{
public:
    CVegetationMap();
    ~CVegetationMap();

    //////////////////////////////////////////////////////////////////////////
    // Map
    //////////////////////////////////////////////////////////////////////////
    //! Allocate sectors map.
    //! @param mapSize is size of terrain in meters.
    void Allocate(int nMapSize, bool bKeepData);

    //! Get number of sectors at side.
    int GetNumSectors() const { return m_numSectors; }

    //! Get total Size of vegetation map.
    int GetSize() const;

    //! Convert world coordinate to sector coordinate.
    int WorldToSector(float worldCoord) const;

    //! Place all objects in vegetation map to 3d engine terrain.
    void PlaceObjectsOnTerrain();

    //! Get total number of vegetation instances.
    int GetNumInstances() const { return m_numInstances; };

    //////////////////////////////////////////////////////////////////////////
    // Vegetation Objects
    //////////////////////////////////////////////////////////////////////////
    //! Get number of use vegetation objects.
    int GetObjectCount() const { return m_objects.size(); }
    //! Get vegetation object.
    CVegetationObject*  GetObject(int i) { return m_objects[i]; }

    //! Create new object.
    //! @param prev Source object to clone from.
    CVegetationObject* CreateObject(CVegetationObject* prev = 0);

    // Inserts an object, assigning a new ID to it.
    // Should be used only with objects which ID is not registered.
    // One such case, is while Undo.
    bool InsertObject(CVegetationObject* obj);

    //! Remove object.
    void RemoveObject(CVegetationObject* obj);
    //! Replace one vegetation object with another.
    void ReplaceObject(CVegetationObject* pOldObject, CVegetationObject* pNewObject);
    //! Remove all objects.
    void ClearObjects();

    //! Remove everything.
    void ClearAll();

    //! Hide all instances of this object.
    void HideObject(CVegetationObject* object, bool bHide);
    void HideAllObjects(bool bHide);

    //! Merge 2 object to first one
    void MergeObjects(CVegetationObject* object, CVegetationObject* objectMerged);

    //! Save vegetation to xml file
    void Save();

    //! Load vegetation from xml file
    bool Load();

    //! Export static object and all its instances.
    //! @return Number of serialized instances.
    int ExportObject(CVegetationObject* object, XmlNodeRef& node, QRect* saveRect = NULL);
    void ImportObject(XmlNodeRef node, const Vec3& offset = Vec3(0, 0, 0));

    //! Export part of vegetation map.
    void ExportBlock(const QRect& rect, CXmlArchive& ar);
    //! Import part of vegetation map.
    void ImportBlock(CXmlArchive& ar, const QPoint& placeOffset = QPoint(0, 0));

    //! Unload all rendering geometry from objects.
    void UnloadObjectsGeometry();

    //////////////////////////////////////////////////////////////////////////
    // Object Painting.
    //////////////////////////////////////////////////////////////////////////

    //! Place single object at specified location.
    CVegetationInstance* PlaceObjectInstance(const Vec3& worldPos, CVegetationObject* brush);
    void DeleteObjInstance(CVegetationInstance* obj);
    void RemoveDuplVegetation(int x1 = -1, int y1 = -1, int x2 = -1, int y2 = -1);
    //! Find object instances closest to the point within given radius.
    CVegetationInstance* GetNearestInstance(const Vec3& worldPos, float radius);
    void GetObjectInstances(float x1, float y1, float x2, float y2, std::vector<CVegetationInstance*>& instances);
    void GetAllInstances(std::vector<CVegetationInstance*>& instances);
    //! Move instance to new position.
    bool MoveInstance(CVegetationInstance* obj, const Vec3& newPos, bool bTerrainAlign = true);
    //! Remove object from 3D engine and place it back again.
    void RepositionObject(CVegetationObject* object);

    //! Update vegetation position z coordinate
    void OnHeightMapChanged();

    //! Remove objects within specified area from 3D engine and then place it back again on top of terrain.
    void RepositionArea(const AABB& box, const Vec3& offset =  Vec3(0, 0, 0), ImageRotationDegrees rotation = ImageRotationDegrees::Rotate0, bool isCopy = false);

    //! Scale all instances of this objects.
    void ScaleObjectInstances(CVegetationObject* object, float fScale, AABB* outModifiedArea = NULL);
    //! Randomally rotate all instances of this objects.
    void RandomRotateInstances(CVegetationObject* object);
    //! Clear rotation for all instances of this objects.
    void ClearRotateInstances(CVegetationObject* object);

    //! Paint objects on rectangle using given brush.
    bool PaintBrush(QRect& rc, bool bCircle, CVegetationObject* brush, Vec3* pPos = 0);

    //! Clear objects in rectangle using given brush.
    //! @param brush Object to remove, if NULL all object will be cleared.
    void ClearBrush(QRect& rc, bool bCircle, CVegetationObject* pObject);

    //! Sets this brighness to all objects within specified rectangle.
    //! x,y,w,h are specified in world units (meters).
    //! \param brightness 0..255 brightness without ground texture, with hill shadows but without object shadows
    //! \param brightness_shadowmap 0..255 brightness without ground texture, with hill shadows and with object shadows
    void PaintBrightness(float x, float y, float w, float h, uint8 brightness, uint8 brightness_shadowmap);

    //////////////////////////////////////////////////////////////////////////
    //! Serialize vegetation map to archive.
    void Serialize(CXmlArchive& xmlAr);

    //! Serialize vegetation objects
    void SerializeObjects(XmlNodeRef& vegetationNode);

    //! Serialize vegetation instances
    void SerializeInstances(CXmlArchive& xmlAr, QRect* rect = NULL);

    //! Serialize segment in memory block
    void ClearSegment(const AABB& bb);
    void ImportSegment(CMemoryBlock& mem, const Vec3& vOfs);
    void ExportSegment(CMemoryBlock& mem, const AABB& bb, const Vec3& vOfs);

    //! Generate shadows from static objects and place them in shadow map bitarray.
    void GenerateShadowMap(CByteImage& shadowmap, float shadowAmmount, const Vec3& sunVector);

    //! Draw sectors to texture.
    void DrawToTexture(uint32* texture, int textureWidth, int textureHeight, int srcX, int srcY);

    //! Record undo info for vegetation instance.
    void RecordUndo(CVegetationInstance* obj);

    //////////////////////////////////////////////////////////////////////////
    // Validate Vegetation map about errors.
    //////////////////////////////////////////////////////////////////////////
    void Validate(IErrorReport& report);

    // Calculates texture memory usage for the vegatation objects.
    int GetTexureMemoryUsage(bool bOnlySelectedObjects);
    int GetSpritesMemoryUsage(bool bOnlySelectedObjects);

    void GetMemoryUsage(ICrySizer* pSizer);

    // Set engine params for all objects
    void SetEngineObjectsParams();

    // Calculate Height for vegetation instance if it placed on the brush.
    float CalcHeightOnBrushes(const Vec3& p, const Vec3& posUpper);

    // Call this function if global config spec is changed
    void UpdateConfigSpec();

    void ReloadGeometry();

    bool IsAreaEmpty(const AABB& bbox);

    //! States for optimal storing Undo
    enum EStoreUndo
    {
        eStoreUndo_Normal,
        eStoreUndo_Begin,
        eStoreUndo_Once,
        eStoreUndo_End,
    };
    void StoreBaseUndo(EStoreUndo state = eStoreUndo_Normal);

    uint32 GetFilterLayerId() const { return m_uiFilterLayerId; }
    void SetFilterLayerId(uint32 uiFilterLayerId) { m_uiFilterLayerId = uiFilterLayerId; }

    // EditorVegetationRequestsBus implementation
    AZStd::vector<CVegetationInstance*> GetObjectInstances(const AZ::Vector2& min, const AZ::Vector2& max) override;
    void DeleteObjectInstance(CVegetationInstance* instance) override;

private:
    struct SectorInfo
    {
        CVegetationInstance* first; // First vegetation object instance in this sector.
    };

    //! Get sector by world coordinates.
    SectorInfo* GetVegSector(const Vec3& worldPos);

    //! Get sector by 2d map coordinates.
    SectorInfo* GetVegSector(int x, int y);

    //! Remove all object in vegetation map from 3d engine terrain.
    void RemoveObjectsFromTerrain();

    //! Create new object instance in map.
    CVegetationInstance* CreateObjInstance(CVegetationObject* object, const Vec3& pos, CVegetationInstance* pCopy = nullptr);
    void DeleteObjInstance(CVegetationInstance* obj, SectorInfo* sector);
    void DeleteObjInstanceNoUndo(CVegetationInstance* obj, SectorInfo* sector);

    //! Only to be used by undo/redo.
    void AddObjInstance(CVegetationInstance* obj);

    void SectorLink(CVegetationInstance* object, SectorInfo* sector);
    void SectorUnlink(CVegetationInstance* object, SectorInfo* sector);

    //! Return true if there is no specified objects within radius from given position.
    bool CanPlace(CVegetationObject* object, const Vec3& pos, float radius);

    //! Returns true if theres no any objects within radius from given position.
    bool IsPlaceEmpty(const Vec3& pos, float radius, CVegetationInstance* ignore);

    void ClearSectors();

    void LoadOldStuff(CXmlArchive& xmlAr);

    void RegisterInstance(CVegetationInstance* obj);

    float GenerateRotation(CVegetationObject* pObject, const Vec3& vPos);

private:
    //! 2D Array of sectors that store vegetation objects.
    SectorInfo* m_sectors;
    //! Size of single sector in meters.
    int m_sectorSize;
    //! Number of sectors in each dimension in map (Resolution of sectors map).
    int m_numSectors;
    //! Size of all map in meters.
    int m_mapSize;

    //! Minimal distance between two objects.
    //! Objects cannot be placed closer together that this distance.
    float m_minimalDistance;

    //! world to sector scaling ratio.
    float m_worldToSector;

    typedef std::vector<TSmartPtr<CVegetationObject> > Objects;
    Objects m_objects;

    int m_numInstances;
    int m_nSID;

    int m_nVersion;
    uint32 m_uiFilterLayerId;

    //! For optimization: Update UI once while Undo
    EStoreUndo m_storeBaseUndoState;

    friend class CUndoVegInstanceCreate;
};

#endif // CRYINCLUDE_EDITOR_VEGETATIONMAP_H
