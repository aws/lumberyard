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
#include "VegetationMap.h"
#include "Terrain/Heightmap.h"
#include "Terrain/Layer.h"
#include "VegetationBrush.h"
#include "VegetationObject.h"

#include "VegetationTool.h"
#include "DataBaseDialog.h"
#include "VegetationDataBasePage.h"
#include "QtViewPaneManager.h"

#include <I3DEngine.h>
#include <ITerrain.h>
#include <IEntityRenderState.h>
#include "Material/Material.h"
#include "GameEngine.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include "../Plugins/EditorCommon/QtViewPane.h"

#include <QMessageBox>

//////////////////////////////////////////////////////////////////////////
// CVegetationMap implementation.
//////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)
// Structure of vegetation object instance in file.
struct SVegInst_V1_0
{
    Vec3    pos;
    float scale;
    uint8 objectIndex;
    uint8 brightness;
    uint8 angle;
};
#pragma pack(pop)


#pragma pack(push,1)
// Structure of vegetation object instance in file.
// Deprecated (11/07/2014)
struct SVegInst_V2_0
{
    Vec3    pos;
    float scale;
    uint8 objectIndex;
    uint8 brightness;
    float angle;
    float angleX;
    float angleY;
};
#pragma pack(pop)


#pragma pack(push,1)
// Structure of vegetation object instance in file.
struct SVegInst
{
    Vec3    pos;
    float scale;
    int objectIndex;
    uint8 brightness;
    float angle;
    float angleX;
    float angleY;
};
#pragma pack(pop)


namespace
{
    const char* kVegetationMapFile = "VegetationMap.dat";
    const char* kVegetationMapFileOld = "VegetationMap.xml"; //TODO: Remove support after January 2012 (support old extension at least for convertion levels time)
    const int kMaxMapSize = 1024;
    const int kMinMaskValue = 32;
};


#define SAFE_RELEASE_NODE(node)     if (node) { (node)->ReleaseNode(); node = 0; }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for vegetation instance operations.
class CUndoVegInstance
    : public IUndoObject
{
public:
    CUndoVegInstance(CVegetationInstance* obj)
    {
        // Stores the current state of this object.
        assert(obj != 0);
        m_obj = obj;
        m_obj->AddRef();

        ZeroStruct(m_redo);
        m_undo.pos = m_obj->pos;
        m_undo.scale = m_obj->scale;
        m_undo.objectIndex = m_obj->object->GetIndex();
        m_undo.brightness = m_obj->brightness;
        m_undo.angle = m_obj->angle;
        m_undo.angleX = m_obj->angleX;
        m_undo.angleY = m_obj->angleY;
    }
    ~CUndoVegInstance()
    {
        m_obj->Release();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Vegetation Modify"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo.pos = m_obj->pos;
            m_redo.scale = m_obj->scale;
            m_redo.objectIndex = m_obj->object->GetIndex();
            m_redo.brightness = m_obj->brightness;
            m_redo.angle = m_obj->angle;
            m_redo.angleX = m_obj->angleX;
            m_redo.angleY = m_obj->angleY;
        }
        m_obj->scale = m_undo.scale;
        m_obj->brightness = m_undo.brightness;
        m_obj->angle = m_undo.angle;
        m_obj->angleX = m_undo.angleX;
        m_obj->angleY = m_undo.angleY;

        // move instance to new position
        GetIEditor()->GetVegetationMap()->MoveInstance(m_obj, m_undo.pos, false);

        Update();
    }
    virtual void Redo()
    {
        m_obj->scale = m_redo.scale;
        m_obj->brightness = m_redo.brightness;
        m_obj->angle = m_redo.angle;
        m_obj->angleX = m_redo.angleX;
        m_obj->angleY = m_redo.angleY;

        // move instance to new position
        GetIEditor()->GetVegetationMap()->MoveInstance(m_obj, m_redo.pos, false);

        Update();
    }
    void Update()
    {
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            CVegetationTool* pVegetationTool = static_cast<CVegetationTool*>(pTool);
            pVegetationTool->UpdateTransformManipulator();
        }
    }


protected:
    CVegetationInstance* m_obj;

private:
    SVegInst m_undo;
    SVegInst m_redo;
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for vegetation instance operations with vegetation object relationships.
class CUndoVegInstanceEx
    : public CUndoVegInstance
{
public:
    CUndoVegInstanceEx(CVegetationInstance* obj)
        : CUndoVegInstance(obj)
    {
        m_undo = obj->object;
    }
protected:

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo = m_obj->object;
        }
        m_obj->object = m_undo;

        CUndoVegInstance::Undo(bUndo);
    }
    virtual void Redo()
    {
        m_obj->object = m_redo;
        CUndoVegInstance::Redo();
    }
private:
    TSmartPtr<CVegetationObject> m_undo;
    TSmartPtr<CVegetationObject> m_redo;
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject.
class CUndoVegInstanceCreate
    : public IUndoObject
{
public:
    CUndoVegInstanceCreate(CVegetationInstance* obj, bool bDeleted)
    {
        // Stores the current state of this object.
        assert(obj != 0);
        m_obj = obj;
        m_obj->AddRef();
        m_bDeleted = bDeleted;
    }

    ~CUndoVegInstanceCreate()
    {
        m_obj->Release();
    }

protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Vegetation Create"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            ClearThingSelection();
        }

        CVegetationMap* vegMap = GetIEditor()->GetVegetationMap();
        if (m_bDeleted)
        {
            vegMap->AddObjInstance(m_obj);
            vegMap->MoveInstance(m_obj, m_obj->pos, false);
        }
        else
        {
            vegMap->DeleteObjInstance(m_obj);
        }
    }

    virtual void Redo()
    {
        ClearThingSelection();
        CVegetationMap* vegMap = GetIEditor()->GetVegetationMap();
        if (!m_bDeleted)
        {
            vegMap->AddObjInstance(m_obj);
            vegMap->MoveInstance(m_obj, m_obj->pos, false);
        }
        else
        {
            vegMap->DeleteObjInstance(m_obj);
        }
    }

    void ClearThingSelection()
    {
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            CVegetationTool* pVegetationTool = static_cast<CVegetationTool*>(pTool);
            pVegetationTool->ClearThingSelection(false);
        }
    }

private:
    CVegetationInstance* m_obj;
    bool m_bDeleted;
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Base Undo object for CVegetationObject.
class CUndoVegetationBase
    : public IUndoObject
{
public:
    CUndoVegetationBase(bool isReloadObjectsInPanel)
        : m_isReloadObjectsInPanel(isReloadObjectsInPanel)
    {
    }

protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Vegetation Modify"; }

    virtual void Undo(bool bUndo)
    {
        NotifyListeners();
    }

    virtual void Redo()
    {
        NotifyListeners();
    }

    void NotifyListeners()
    {
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            CVegetationTool* pVegetationTool = (CVegetationTool*)pTool;
            pVegetationTool->RefreshPanel(m_isReloadObjectsInPanel);
        }

        auto pDatabaseDialog = FindViewPane<CDataBaseDialog>(LyViewPane::DatabaseView);
        if (pDatabaseDialog)
        {
            if (auto pVegetationDatabase = qobject_cast<CVegetationDataBasePage*>(pDatabaseDialog->GetCurrent()))
            {
                pVegetationDatabase->ReloadObjects();
            }
        }

        CVegetationDataBasePage* vegetationObjects = FindViewPane<CVegetationDataBasePage>(LyViewPane::VegetationEditor);
        if (vegetationObjects)
        {
            vegetationObjects->ReloadObjects();
        }
    }

private:
    bool m_isReloadObjectsInPanel;
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for vegetation object.
class CUndoVegetationObject
    : public CUndoVegetationBase
{
public:
    CUndoVegetationObject(CVegetationObject* pObj)
        : CUndoVegetationBase(true)
    {
        m_pObj = pObj;
        m_undo = m_pObj->GetNumInstances();
    }

protected:
    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo = m_pObj->GetNumInstances();
        }
        m_pObj->SetNumInstances(m_undo);
        // Set hidden flag
        m_pObj->SetHidden(true);
        // Show instancies
        GetIEditor()->GetVegetationMap()->HideObject(m_pObj, false);
        CUndoVegetationBase::Undo(bUndo);
    }
    virtual void Redo()
    {
        m_pObj->SetNumInstances(m_redo);
        CUndoVegetationBase::Redo();
    }

private:
    TSmartPtr<CVegetationObject> m_pObj;
    int m_undo;
    int m_redo;
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for creation and deleting CVegetationObject
class CUndoVegetationObjectCreate
    : public CUndoVegetationBase
{
public:
    CUndoVegetationObjectCreate(CVegetationObject* obj, bool bDeleted)
        : CUndoVegetationBase(true)
    {
        // Stores the current state of this object.
        assert(obj != 0);
        m_obj = obj;
        m_obj->AddRef();
        m_bDeleted = bDeleted;
    }
    ~CUndoVegetationObjectCreate()
    {
        m_obj->Release();
    }
protected:
    virtual QString GetDescription() { return "Vegetation Object Create"; }

    virtual void Undo(bool bUndo)
    {
        CVegetationMap* vegMap = GetIEditor()->GetVegetationMap();
        if (m_bDeleted)
        {
            vegMap->InsertObject(m_obj);
        }
        else
        {
            vegMap->RemoveObject(m_obj);
        }
        CUndoVegetationBase::Undo(bUndo);
    }
    virtual void Redo()
    {
        CVegetationMap* vegMap = GetIEditor()->GetVegetationMap();
        if (!m_bDeleted)
        {
            vegMap->InsertObject(m_obj);
        }
        else
        {
            vegMap->RemoveObject(m_obj);
        }
        CUndoVegetationBase::Redo();
    }

private:
    CVegetationObject* m_obj;
    bool m_bDeleted;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// class CVegetationMap
//////////////////////////////////////////////////////////////////////////
CVegetationMap::CVegetationMap()
{
    m_sectors = 0;
    m_sectorSize = 0;
    m_numSectors = 0;
    m_mapSize = 0;
    m_minimalDistance = 0.1f;
    m_worldToSector = 0;
    m_numInstances = 0;
    m_nSID = 0;
    m_nVersion = 1;
    m_uiFilterLayerId = -1;
    m_storeBaseUndoState = eStoreUndo_Normal;

    // Initialize the random number generator
    srand(GetTickCount());

    AzToolsFramework::EditorVegetation::EditorVegetationRequestsBus::Handler::BusConnect(this);
}


//////////////////////////////////////////////////////////////////////////
CVegetationMap::~CVegetationMap()
{
    AzToolsFramework::EditorVegetation::EditorVegetationRequestsBus::Handler::BusDisconnect();
    ClearAll();
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ClearObjects()
{
    ClearSectors();
    m_objects.clear();
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ClearAll()
{
    ClearObjects();

    if (m_sectors)
    {
        free(m_sectors);
        m_sectors = 0;
    }

    m_usedIds.clear();

    m_sectors = 0;
    m_sectorSize = 0;
    m_numSectors = 0;
    m_mapSize = 0;
    m_minimalDistance = 0.1f;
    m_worldToSector = 0;
    m_numInstances = 0;
    m_nSID = 0;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RegisterInstance(CVegetationInstance* obj)
{
    // re-create vegetation render node
    SAFE_RELEASE_NODE(obj->pRenderNode);

    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();

    if (obj->object && !obj->object->IsHidden())
    {
        obj->pRenderNode = p3DEngine->GetITerrain()->AddVegetationInstance(obj->object->GetId(), obj->pos, obj->scale, obj->brightness, RAD2BYTE(obj->angle), RAD2BYTE(obj->angleX), RAD2BYTE(obj->angleY));
    }

    if (obj->pRenderNode && !obj->object->IsAutoMerged())
    {
        GetIEditor()->GetGameEngine()->OnAreaModified(obj->pRenderNode->GetBBox());
    }

    // re-create ground decal render node
    SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);
    if (obj->object && (obj->object->m_pMaterialGroundDecal != NULL) && obj->pRenderNode)
    {
        obj->pRenderNodeGroundDecal = (IDecalRenderNode*)p3DEngine->CreateRenderNode(eERType_Decal);

        bool    boIsSelected(0);

        // update basic entity render flags
        unsigned int renderFlags = 0;
        obj->pRenderNodeGroundDecal->SetRndFlags(renderFlags);

        // set properties
        SDecalProperties decalProperties;
        decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrain;

        Matrix34 wtm;
        wtm.SetIdentity();
        Vec3 vSize = obj->pRenderNode->GetBBox().GetSize();
        float fRadiusXY = max(vSize.x, vSize.y) * .125f;
        wtm.SetScale(Vec3(fRadiusXY, fRadiusXY, fRadiusXY));
        wtm.SetTranslation(obj->pos);

        // get normalized rotation (remove scaling)
        Matrix33 rotation(wtm);
        rotation.SetRow(0, rotation.GetRow(0).GetNormalized());
        rotation.SetRow(1, rotation.GetRow(1).GetNormalized());
        rotation.SetRow(2, rotation.GetRow(2).GetNormalized());

        decalProperties.m_pos = wtm.TransformPoint(Vec3(0, 0, 0));
        decalProperties.m_normal = wtm.TransformVector(Vec3(0, 0, 1));
        QByteArray name = obj->object->m_pMaterialGroundDecal->GetName().toUtf8();
        decalProperties.m_pMaterialName = name.data();
        decalProperties.m_radius = decalProperties.m_normal.GetLength();
        decalProperties.m_explicitRightUpFront = rotation;
        decalProperties.m_sortPrio = 10;
        obj->pRenderNodeGroundDecal->SetDecalProperties(decalProperties);

        obj->pRenderNodeGroundDecal->SetMatrix(wtm);
        obj->pRenderNodeGroundDecal->SetViewDistanceMultiplier(2.0f);
    }
}

//////////////////////////////////////////////////////////////////////////
float CVegetationMap::GenerateRotation(CVegetationObject* pObject, const Vec3& vPos)
{
    const bool bRandomRotation = pObject->IsRandomRotation();
    const int nRotationRange = pObject->GetRotationRangeToTerrainNormal();

    if (bRandomRotation || nRotationRange >= 360)
    {
        return (float)rand();
    }
    else if (nRotationRange > 0)
    {
        const Vec3 vTerrainNormal = GetIEditor()->Get3DEngine()->GetTerrainSurfaceNormal(vPos);

        if (abs(vTerrainNormal.x) == 0.f && abs(vTerrainNormal.y) == 0.f)
        {
            return (float)rand();
        }
        else
        {
            const float rndDegree = (float)-nRotationRange * 0.5f + (float)(rand() % nRotationRange);
            const float finaleDegree = RAD2DEG(atan2f(vTerrainNormal.y, vTerrainNormal.x)) + rndDegree;
            return DEG2RAD(finaleDegree);
        }
    }
    return 0.f;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ClearSectors()
{
    RemoveObjectsFromTerrain();
    // Delete all objects in sectors.
    // Iterator over all sectors.
    CVegetationInstance* next;
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = next)
        {
            next = obj->next;
            obj->pRenderNode = 0;
            SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);
            obj->Release();
        }
        si->first = 0;
    }
    m_numInstances = 0;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::Allocate(int nMapSize, bool bKeepData)
{
    CXmlArchive ar("Temp");
    if (bKeepData)
    {
        Serialize(ar);
    }

    ClearAll();

    m_mapSize = nMapSize;
    m_sectorSize = m_mapSize < kMaxMapSize ? 1 : m_mapSize / kMaxMapSize;
    m_numSectors = m_mapSize / m_sectorSize;
    m_worldToSector = 1.0f / m_sectorSize;

    // allocate sectors map.
    int sz = sizeof(SectorInfo) * m_numSectors * m_numSectors;
    m_sectors = (SectorInfo*)malloc(sz);
    memset(m_sectors, 0, sz);

    if (bKeepData)
    {
        ar.bLoading = true;
        Serialize(ar);
    }
}


//////////////////////////////////////////////////////////////////////////
int CVegetationMap::GetSize() const
{
    return m_numSectors * m_sectorSize;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::PlaceObjectsOnTerrain()
{
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
    if (!p3DEngine)
    {
        return;
    }

    // Clear all objects from 3d Engine.
    RemoveObjectsFromTerrain();

    // Iterator over all sectors.
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (!obj->object->IsHidden())
            {
                // Stick vegetation to terrain.
                if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
                {
                    obj->pos.z = p3DEngine->GetTerrainElevation(obj->pos.x, obj->pos.y);
                }
                obj->pRenderNode = 0;
                SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);
                RegisterInstance(obj);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RemoveObjectsFromTerrain()
{
    if (GetIEditor()->Get3DEngine())
    {
        GetIEditor()->Get3DEngine()->RemoveAllStaticObjects(m_nSID);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::HideObject(CVegetationObject* object, bool bHide)
{
    if (object->IsHidden() == bHide)
    {
        return;
    }

    object->SetHidden(bHide);

    if (object->GetNumInstances() > 0)
    {
        I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
        // Iterate over all sectors.
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
            {
                if (obj->object == object)
                {
                    // Remove/Add to terrain.
                    if (bHide)
                    {
                        SAFE_RELEASE_NODE(obj->pRenderNode);
                        SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);
                    }
                    else
                    {
                        // Stick vegetation to terrain.
                        if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
                        {
                            obj->pos.z = p3DEngine->GetTerrainElevation(obj->pos.x, obj->pos.y);
                        }
                        RegisterInstance(obj);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::HideAllObjects(bool bHide)
{
    for (int i = 0; i < m_objects.size(); i++)
    {
        HideObject(m_objects[i], bHide);
    }
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::MergeObjects(CVegetationObject* object, CVegetationObject* objectMerged)
{
    if (objectMerged->GetNumInstances() > 0)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoVegetationObject(object));
            CUndo::Record(new CUndoVegetationObject(objectMerged));
        }
        HideObject(object, true);
        HideObject(objectMerged, true);
        // Iterate over all sectors.
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
            {
                if (obj->object == objectMerged)
                {
                    if (CUndo::IsRecording())
                    {
                        CUndo::Record(new CUndoVegInstanceEx(obj));
                    }
                    obj->object = object;
                    object->SetNumInstances(object->GetNumInstances() + 1);
                    objectMerged->SetNumInstances(objectMerged->GetNumInstances() - 1);
                }
            }
        }
        HideObject(object, false);
        HideObject(objectMerged, false);
    }
    assert(objectMerged->GetNumInstances() == 0);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::SectorLink(CVegetationInstance* obj, SectorInfo* sector)
{
    /*
    if (sector->first)
    {
        // Add to the end of current list.
        CVegetationInstance *head = sector->first;
        CVegetationInstance *tail = head->prev;
        obj->prev = tail;
        obj->next = 0;

        tail->next = obj;
        head->prev = obj; // obj is now last object.
    }
    else
    {
        sector->first = obj;
        obj->prev = obj;
        obj->next = 0;
    }
    */
    if (sector->first)
    {
        // Add to the end of current list.
        CVegetationInstance* head = sector->first;
        obj->prev = 0;
        obj->next = head;
        head->prev = obj;
        sector->first = obj;
    }
    else
    {
        sector->first = obj;
        obj->prev = 0;
        obj->next = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::SectorUnlink(CVegetationInstance* obj, SectorInfo* sector)
{
    if (obj == sector->first) // if head of list.
    {
        sector->first = obj->next;
        if (sector->first)
        {
            sector->first->prev = 0;
        }
    }
    else
    {
        //assert( obj->prev != 0 );
        if (obj->prev)
        {
            obj->prev->next = obj->next;
        }
        if (obj->next)
        {
            obj->next->prev = obj->prev;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
inline CVegetationMap::SectorInfo* CVegetationMap::GetVegSector(int x, int y)
{
    assert(x >= 0 && x < m_numSectors && y >= 0 && y < m_numSectors);
    return &m_sectors[y * m_numSectors + x];
}


//////////////////////////////////////////////////////////////////////////
inline CVegetationMap::SectorInfo* CVegetationMap::GetVegSector(const Vec3& worldPos)
{
    int x = ftoi(worldPos.x * m_worldToSector);
    int y = ftoi(worldPos.y * m_worldToSector);
    if (x >= 0 && x < m_numSectors && y >= 0 && y < m_numSectors)
    {
        return &m_sectors[y * m_numSectors + x];
    }
    else
    {
        return 0;
    }
}


void CVegetationMap::AddObjInstance(CVegetationInstance* obj)
{
    SectorInfo* si = GetVegSector(obj->pos);
    if (!si)
    {
        obj->next = obj->prev = 0;
        return;
    }
    obj->AddRef();

    CVegetationObject* object = obj->object;
    // Add object to end of the list of instances in sector.
    // Increase number of instances.
    object->SetNumInstances(object->GetNumInstances() + 1);
    m_numInstances++;

    SectorLink(obj, si);
}

//////////////////////////////////////////////////////////////////////////
CVegetationInstance* CVegetationMap::CreateObjInstance(CVegetationObject* object, const Vec3& pos, CVegetationInstance* pCopy)
{
    SectorInfo* si = GetVegSector(pos);
    if (!si)
    {
        return 0;
    }

    if (m_numInstances >= MAX_VEGETATION_INSTANCES)
    {
        return nullptr;
    }

    CVegetationInstance* obj = new CVegetationInstance();
    obj->m_refCount = 1; // Starts with 1 reference.
    obj->object = object;
    obj->pos = pos;
    obj->pRenderNode = 0;

    if (pCopy)
    {
        obj->scale = pCopy->scale;
        obj->brightness = pCopy->brightness;
        obj->angleX = pCopy->angleX;
        obj->angleY = pCopy->angleY;
        obj->angle = pCopy->angle;
        obj->pRenderNodeGroundDecal = pCopy->pRenderNodeGroundDecal;
        obj->m_boIsSelected = pCopy->m_boIsSelected;
    }
    else
    {
        obj->scale = 1;
        obj->brightness = 255;
        obj->angleX = 0;
        obj->angleY = 0;
        obj->angle = 0;
        obj->pRenderNodeGroundDecal = 0;
        obj->m_boIsSelected = false;
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegInstanceCreate(obj, false));
    }

    // Add object to end of the list of instances in sector.
    // Increase number of instances.
    object->SetNumInstances(object->GetNumInstances() + 1);
    m_numInstances++;

    SectorLink(obj, si);
    return obj;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::DeleteObjInstance(CVegetationInstance* obj, SectorInfo* sector)
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegInstanceCreate(obj, true));
    }

    if (obj->pRenderNode)
    {
        GetIEditor()->GetGameEngine()->OnAreaModified(obj->pRenderNode->GetBBox());
    }

    SAFE_RELEASE_NODE(obj->pRenderNode);
    SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);

    SectorUnlink(obj, sector);
    obj->object->SetNumInstances(obj->object->GetNumInstances() - 1);
    m_numInstances--;
    assert(m_numInstances >= 0);
    obj->Release();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::DeleteObjInstance(CVegetationInstance* obj)
{
    SectorInfo* sector = GetVegSector(obj->pos);
    if (sector)
    {
        DeleteObjInstance(obj, sector);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RemoveDuplVegetation(int x1, int y1, int x2, int y2)
{
    if (x1 == -1)
    {
        x1 = 0;
        y1 = 0;
        x2 = m_numSectors - 1;
        y2 = m_numSectors - 1;
    }
    for (int j = y1; j <= y2; j++)
    {
        for (int i = x1; i <= x2; i++)
        {
            SectorInfo* si = &m_sectors[i + j * m_numSectors];

            for (CVegetationInstance* objChk = si->first; objChk; objChk = objChk->next)
            {
                for (CVegetationInstance* objRem = objChk; objRem && objRem->next; objRem = objRem->next)
                {
                    if (fabs(objChk->pos.x - objRem->next->pos.x) < m_minimalDistance &&
                        fabs(objChk->pos.y - objRem->next->pos.y) < m_minimalDistance)
                    {
                        DeleteObjInstance(objRem->next, si);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CVegetationInstance* CVegetationMap::GetNearestInstance(const Vec3& pos, float radius)
{
    // check all sectors intersected by radius.
    float r = radius * m_worldToSector;
    float px = pos.x * m_worldToSector;
    float py = pos.y * m_worldToSector;
    int sx1 = ftoi(px - r);
    sx1 = max(sx1, 0);
    int sx2 = ftoi(px + r);
    sx2 = min(sx2, m_numSectors - 1);
    int sy1 = ftoi(py - r);
    sy1 = max(sy1, 0);
    int sy2 = ftoi(py + r);
    sy2 = min(sy2, m_numSectors - 1);

    CVegetationInstance* nearest = 0;
    float minDist = FLT_MAX;

    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is nearby.
            SectorInfo* si = GetVegSector(x, y);
            if (si->first)
            {
                for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
                {
                    if (fabs(obj->pos.x - pos.x) < radius && fabs(obj->pos.y - pos.y) < radius)
                    {
                        float dist = pos.GetSquaredDistance(obj->pos);
                        if (dist < minDist)
                        {
                            minDist = dist;
                            nearest = obj;
                        }
                    }
                }
            }
        }
    }
    return nearest;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::GetObjectInstances(float x1, float y1, float x2, float y2, std::vector<CVegetationInstance*>& instances)
{
    instances.reserve(100);
    // check all sectors intersected by radius.
    int sx1 = ftoi(x1 * m_worldToSector);
    sx1 = max(sx1, 0);
    int sx2 = ftoi(x2 * m_worldToSector);
    sx2 = min(sx2, m_numSectors - 1);
    int sy1 = ftoi(y1 * m_worldToSector);
    sy1 = max(sy1, 0);
    int sy2 = ftoi(y2 * m_worldToSector);
    sy2 = min(sy2, m_numSectors - 1);
    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is nearby.
            SectorInfo* si = GetVegSector(x, y);
            if (si->first)
            {
                for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
                {
                    if (obj->pos.x >= x1 && obj->pos.x <= x2 && obj->pos.y >= y1 && obj->pos.y <= y2)
                    {
                        instances.push_back(obj);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::GetAllInstances(std::vector<CVegetationInstance*>& instances)
{
    int k = 0;
    instances.resize(m_numInstances);
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = m_sectors[i].first; obj; obj = obj->next)
        {
            instances[k++] = obj;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::IsPlaceEmpty(const Vec3& pos, float radius, CVegetationInstance* ignore)
{
    // check all sectors intersected by radius.
    if (pos.x < 0 || pos.y < 0 || pos.x > m_mapSize || pos.y > m_mapSize)
    {
        return false;
    }

    // check all sectors intersected by radius.
    float r = radius * m_worldToSector;
    float px = pos.x * m_worldToSector;
    float py = pos.y * m_worldToSector;
    int sx1 = ftoi(px - r);
    sx1 = max(sx1, 0);
    int sx2 = ftoi(px + r);
    sx2 = min(sx2, m_numSectors - 1);
    int sy1 = ftoi(py - r);
    sy1 = max(sy1, 0);
    int sy2 = ftoi(py + r);
    sy2 = min(sy2, m_numSectors - 1);
    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is within this radius.
            SectorInfo* si = GetVegSector(x, y);
            if (si->first)
            {
                for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
                {
                    if (obj != ignore && fabs(obj->pos.x - pos.x) < radius && fabs(obj->pos.y - pos.y) < radius)
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::MoveInstance(CVegetationInstance* obj, const Vec3& newPos, bool bTerrainAlign)
{
    if (!IsPlaceEmpty(newPos, m_minimalDistance, obj))
    {
        return false;
    }

    if (obj->pos != newPos)
    {
        RecordUndo(obj);
    }

    // Then delete object.
    DWORD dwOldFlags = 0;
    if (obj->pRenderNode)
    {
        dwOldFlags = obj->pRenderNode->GetRndFlags();
    }

    SAFE_RELEASE_NODE(obj->pRenderNode);
    SAFE_RELEASE_NODE(obj->pRenderNodeGroundDecal);

    SectorInfo* from = GetVegSector(obj->pos);
    SectorInfo* to = GetVegSector(newPos);

    if (!from || !to)
    {
        return true;
    }

    if (from != to)
    {
        // Relink object between sectors.
        SectorUnlink(obj, from);
        if (to)
        {
            SectorLink(obj, to);
        }
    }

    obj->pos = newPos;

    // Stick vegetation to terrain.
    if (bTerrainAlign)
    {
        if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
        {
            obj->pos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(obj->pos.x, obj->pos.y);
        }
    }
    //GetIEditor()->Get3DEngine()->AddStaticObject( obj->object->GetId(),newPos,obj->scale,obj->brightness );

    RegisterInstance(obj);

    if (obj->pRenderNode && (dwOldFlags & ERF_SELECTED))
    {
        obj->pRenderNode->SetRndFlags(ERF_SELECTED, true);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::CanPlace(CVegetationObject* object, const Vec3& pos, float radius)
{
    // check all sectors intersected by radius.
    if (pos.x < 0 || pos.y < 0 || pos.x > m_mapSize || pos.y > m_mapSize)
    {
        return false;
    }

    float r = radius * m_worldToSector;
    float px = pos.x * m_worldToSector;
    float py = pos.y * m_worldToSector;
    int sx1 = ftoi(px - r);
    sx1 = max(sx1, 0);
    int sx2 = ftoi(px + r);
    sx2 = min(sx2, m_numSectors - 1);
    int sy1 = ftoi(py - r);
    sy1 = max(sy1, 0);
    int sy2 = ftoi(py + r);
    sy2 = min(sy2, m_numSectors - 1);
    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is within this radius.
            SectorInfo* si = GetVegSector(x, y);
            if (si->first)
            {
                for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
                {
                    // Only check objects that we need.
                    if (obj->object == object)
                    {
                        if (fabs(obj->pos.x - pos.x) < radius && fabs(obj->pos.y - pos.y) < radius)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (fabs(obj->pos.x - pos.x) < m_minimalDistance && fabs(obj->pos.y - pos.y) < m_minimalDistance)
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
CVegetationInstance* CVegetationMap::PlaceObjectInstance(const Vec3& worldPos, CVegetationObject* object)
{
    float fScale = object->CalcVariableSize();
    // Check if this place is empty.
    if (CanPlace(object, worldPos, m_minimalDistance))
    {
        CVegetationInstance* obj = CreateObjInstance(object, worldPos);
        if (obj)
        {
            obj->angle = GenerateRotation(object, worldPos);
            obj->scale = fScale;

            // Stick vegetation to terrain.
            if (!obj->object->IsAffectedByBrushes())
            {
                obj->pos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(obj->pos.x, obj->pos.y);
            }

            RegisterInstance(obj);
        }
        return obj;
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::PaintBrush(QRect& rc, bool bCircle, CVegetationObject* object, Vec3* pPos)
{
    assert(object != 0);

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();

    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    Vec3 p(0, 0, 0);

    int unitSize = pHeightmap->GetUnitSize();

    int mapSize = m_numSectors * m_sectorSize;

    bool bProgress = rc.width() >= mapSize;
    CWaitProgress wait(QObject::tr("Distributing objects"), bProgress);

    // Intersect with map rectangle.
    // Offset by 1 from each side.
    const int accountForQtBottomRightOffset = -1;
    float brushRadius2 = rc.width() * rc.width() / 4;
    rc &= QRect(1, 1, mapSize - 2 - accountForQtBottomRightOffset, mapSize - 2 - accountForQtBottomRightOffset);

    float AltMin = object->GetElevationMin();
    float AltMax = object->GetElevationMax();
    float SlopeMin = object->GetSlopeMin();
    float SlopeMax = object->GetSlopeMax();

    float density = object->GetDensity();
    if (density <= 0)
    {
        density = m_minimalDistance;
    }

    int area = rc.width() * rc.height();
    int count = area / (density * density);

    // Limit from too much objects.
    if (count > area)
    {
        count = area;
    }

    bool bUndoStored = false;
    int j = 0;

    float x1 = rc.left();
    float y1 = rc.top();
    float width2 = rc.width() / 2.0f;
    float height2 = rc.height() / 2.0f;

    float cx = (rc.right() + 1 + rc.left()) / 2.0f;
    float cy = (rc.bottom() + 1 + rc.top()) / 2.0f;

    // Calculate the vegetation for every point in the area marked by the brush
    bool success = true;
    for (int i = 0; i < count; i++)
    {
        if (bProgress)
        {
            j++;
            if (j > 200)
            {
                if (!wait.Step(100 * i / count))
                {
                    break;
                }
            }
        }

        float x = x1 + cry_random(0.0f, 2.0f) * width2;
        float y = y1 + cry_random(0.0f, 2.0f) * height2;

        // Skip all coordinates outside the brush circle
        if (bCircle)
        {
            if (((x - cx) * (x - cx) + (y - cy) * (y - cy)) > brushRadius2)
            {
                continue;
            }
        }

        // Get the height of the current point
        // swap x/y
        int hy = ftoi(x / unitSize);
        int hx = ftoi(y / unitSize);

        // Use the safe method for retrieving the height, or else the Editor
        // will crash if the x/y values are out of range
        float currHeight = pHeightmap->GetSafeXY(hx, hy);

        // Check if height value is within brush min/max altitude.
        if (currHeight < AltMin || currHeight > AltMax)
        {
            continue;
        }

        // Calculate the slope for this spot
        float slope = pHeightmap->GetSlope(hx, hy);

        // Check if slope is within brush min/max slope.
        if (slope < SlopeMin || slope > SlopeMax)
        {
            continue;
        }

        p.x = x;
        p.y = y;
        float fScale = object->CalcVariableSize();
        float placeRadius = fScale * object->GetObjectSize() * 0.5f;
        // Check if this place is empty.
        if (!CanPlace(object, p, placeRadius))
        {
            continue;
        }

        if (pPos && object->IsAffectedByBrushes())
        {
            p.z = pPos->z;
            float brushRadius = float(rc.right() - rc.left()) / 2;
            Vec3 pointerPos(p.x, p.y, pPos->z);
            Vec3 posUpper = pointerPos + Vec3(0, 0, 0.1f + brushRadius / 2);
            p.z = CalcHeightOnBrushes(p, posUpper);
        }
        else if (object->IsAffectedByTerrain())
        {
            p.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(p.x, p.y);
        }
        else
        {
            continue;
        }

        if (!bUndoStored)
        {
            StoreBaseUndo();
            bUndoStored = true;
        }

        CVegetationInstance* obj = CreateObjInstance(object, p);
        if (obj)
        {
            obj->angle = GenerateRotation(object, p);
            obj->scale = fScale;
            RegisterInstance(obj);
        }
        else
        {
            // Stop trying to create more instances if we failed (e.g. reached max vegetation instance limit)
            success = false;
            break;
        }
    }

    // if undo stored, store info for redo
    if (bUndoStored)
    {
        StoreBaseUndo();
    }

    return success;
}


//////////////////////////////////////////////////////////////////////////
float CVegetationMap::CalcHeightOnBrushes(const Vec3& p, const Vec3& posUpper)
{
    IPhysicalWorld* world = GetIEditor()->GetSystem()->GetIPhysicalWorld();
    ray_hit hit;

    int numSkipEnts = 0;
    typedef IPhysicalEntity* PIPhysicalEntity;
    PIPhysicalEntity pSkipEnts[3];
    int col = 0;
    Vec3 vDir(0, 0, -1);
    int flags = rwi_stop_at_pierceable | rwi_ignore_terrain_holes;
    for (int chcnt = 0; chcnt < 3; chcnt++)
    {
        hit.pCollider = 0;
        col = world->RayWorldIntersection(posUpper, vDir * 1000, ent_all, flags, &hit, 1, pSkipEnts, numSkipEnts);
        IRenderNode* pVegNode = 0;
        if (hit.pCollider &&
            hit.pCollider->GetiForeignData() == PHYS_FOREIGN_ID_STATIC &&
            (pVegNode = (IRenderNode*) hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC)) &&
            pVegNode->GetRenderNodeType() == eERType_Vegetation)
        {
            // skip vegetation
        }
        else
        {
            break;
        }

        pSkipEnts[numSkipEnts++] = hit.pCollider;
    }
    if (col && !hit.pt.IsZero())
    {
        return hit.pt.z;
    }

    return p.z;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::PaintBrightness(float x, float y, float w, float h, uint8 brightness, uint8 brightness_shadowmap)
{
    // Find sector range from world positions.
    int startSectorX = ftoi(x * m_worldToSector);
    int startSectorY = ftoi(y * m_worldToSector);
    int endSectorX = ftoi((x + w) * m_worldToSector);
    int endSectorY = ftoi((y + h) * m_worldToSector);

    // Clamp start and end sectors to valid range.
    if (startSectorX < 0)
    {
        startSectorX = 0;
    }
    if (startSectorY < 0)
    {
        startSectorY = 0;
    }
    if (endSectorX >= m_numSectors)
    {
        endSectorX = m_numSectors - 1;
    }
    if (endSectorY >= m_numSectors)
    {
        endSectorY = m_numSectors - 1;
    }

    float x2 = x + w;
    float y2 = y + h;

    // Iterate all sectors in range.
    for (int sy = startSectorY; sy <= endSectorY; sy++)
    {
        for (int sx = startSectorX; sx <= endSectorX; sx++)
        {
            // Iterate all objects in sector.
            SectorInfo* si = &m_sectors[sy * m_numSectors + sx];
            if (!si->first)
            {
                continue;
            }
            for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
            {
                if (obj->pos.x >= x && obj->pos.x < x2 && obj->pos.y >= y && obj->pos.y <= y2)
                {
                    bool bNeedUpdate = false;
                    if (!obj->object->IsPrecalcShadow())
                    {
                        if (obj->brightness != brightness_shadowmap)
                        {
                            bNeedUpdate = true;
                        }
                        // If object is not casting precalculated shadow (small grass etc..) affect it by shadow map.
                        obj->brightness = brightness_shadowmap;
                    }
                    else
                    {
                        if (obj->brightness != brightness)
                        {
                            bNeedUpdate = true;
                        }
                        obj->brightness = brightness;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ClearBrush(QRect& rc, bool bCircle, CVegetationObject* pObject)
{
    StoreBaseUndo();

    //GetISystem()->VTuneResume();
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    Vec3 p(0, 0, 0);

    int unitSize = GetIEditor()->GetHeightmap()->GetUnitSize();

    int mapSize = m_numSectors * m_sectorSize;

    // Intersect with map rectangle.
    // Offset by 1 from each side.
    float brushRadius2 = rc.width() * rc.width() / 4;
    float cx = (rc.right() + 1 + rc.left()) / 2;
    float cy = (rc.bottom() + 1 + rc.top()) / 2;

    float x1 = rc.left();
    float y1 = rc.top();
    float x2 = rc.right();
    float y2 = rc.bottom();

    // check all sectors intersected by radius.
    int sx1 = ftoi(x1 * m_worldToSector);
    int sx2 = ftoi(x2 * m_worldToSector);
    int sy1 = ftoi(y1 * m_worldToSector);
    int sy2 = ftoi(y2 * m_worldToSector);
    sx1 = max(sx1, 0);
    sx2 = min(sx2, m_numSectors - 1);
    sy1 = max(sy1, 0);
    sy2 = min(sy2, m_numSectors - 1);

    CVegetationInstance* next = 0;
    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is within this radius.
            SectorInfo* si = GetVegSector(x, y);
            if (si->first)
            {
                for (CVegetationInstance* obj = si->first; obj; obj = next)
                {
                    next = obj->next;
                    if (obj->object != pObject && pObject != NULL)
                    {
                        continue;
                    }

                    if (bCircle)
                    {
                        // Skip objects outside the brush circle
                        if (((obj->pos.x - cx) * (obj->pos.x - cx) + (obj->pos.y - cy) * (obj->pos.y - cy)) > brushRadius2)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        // Within rectangle.
                        if (obj->pos.x < x1 || obj->pos.x > x2 || obj->pos.y < y1 || obj->pos.y > y2)
                        {
                            continue;
                        }
                    }

                    // Then delete object.
                    StoreBaseUndo(eStoreUndo_Once);
                    DeleteObjInstance(obj, si);
                }
            }
        }
    }

    StoreBaseUndo();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CVegetationObject* CVegetationMap::CreateObject(CVegetationObject* prev)
{
    int id(GenerateVegetationObjectId());

    if (id < 0)
    {
        // Free id not found
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Vegetation objects limit is reached."));
        return 0;
    }

    // Mark id as used.
    m_usedIds.insert(id);

    CVegetationObject* obj = new CVegetationObject(id);
    if (prev)
    {
        obj->CopyFrom(*prev);
    }

    m_objects.push_back(obj);

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegetationObjectCreate(obj, false));
    }

    return obj;
}
//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::InsertObject(CVegetationObject* obj)
{
    int id(GenerateVegetationObjectId());

    if (id < 0)
    {
        // Free id not found, created more then 256 objects
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Vegetation objects limit is reached."));
        return false;
    }

    // Mark id as used.
    m_usedIds.insert(id);

    // Assign the new Id to the vegetation object.
    obj->SetId(id);

    // Add it to the list.
    m_objects.push_back(obj);

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegetationObjectCreate(obj, false));
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RemoveObject(CVegetationObject* object)
{
    // Free id for this object.
    m_usedIds.erase(object->GetId());

    // First delete instances
    // Undo will be stored in DeleteObjInstance()
    if (object->GetNumInstances() > 0)
    {
        CVegetationInstance* next = 0;
        // Delete this object in sectors.
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = next)
            {
                next = obj->next;
                if (obj->object == object)
                {
                    DeleteObjInstance(obj, si);
                }
            }
        }
    }

    // Store undo for object after deleting of instances. For correct queue in undo stack.
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegetationObjectCreate(object, true));
    }

    Objects::iterator it = std::find(m_objects.begin(), m_objects.end(), object);
    if (it != m_objects.end())
    {
        m_objects.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ReplaceObject(CVegetationObject* pOldObject, CVegetationObject* pNewObject)
{
    if (pOldObject->GetNumInstances() > 0)
    {
        pNewObject->SetNumInstances(pNewObject->GetNumInstances() + pOldObject->GetNumInstances());
        CVegetationInstance* next = 0;
        // Delete this object in sectors.
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = next)
            {
                next = obj->next;
                if (obj->object == pOldObject)
                {
                    obj->object = pNewObject;
                }
            }
        }
    }
    RemoveObject(pOldObject);
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RepositionObject(CVegetationObject* object)
{
    // Iterator over all sectors.
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->object == object)
            {
                RegisterInstance(obj);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::OnHeightMapChanged()
{
    // Iterator over all sectors.
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
            {
                obj->pos.z = p3DEngine->GetTerrainElevation(obj->pos.x, obj->pos.y);
            }

            if (obj->pRenderNode && !obj->object->IsAutoMerged())
            {
                // fix vegetation position
                {
                    p3DEngine->UnRegisterEntityAsJob(obj->pRenderNode);
                    Matrix34 mat;
                    mat.SetIdentity();
                    mat.SetTranslation(obj->pos);
                    obj->pRenderNode->SetMatrix(mat);
                    p3DEngine->RegisterEntity(obj->pRenderNode);
                }

                // fix ground decal position
                if (obj->pRenderNodeGroundDecal)
                {
                    p3DEngine->UnRegisterEntityAsJob(obj->pRenderNodeGroundDecal);
                    Matrix34 wtm;
                    wtm.SetIdentity();
                    Vec3 vSize = obj->pRenderNode->GetBBox().GetSize();
                    float fRadiusXY = max(vSize.x, vSize.y) * .125f;
                    wtm.SetScale(Vec3(fRadiusXY, fRadiusXY, fRadiusXY));
                    wtm.SetTranslation(obj->pos);
                    obj->pRenderNodeGroundDecal->SetMatrix(wtm);
                    p3DEngine->RegisterEntity(obj->pRenderNodeGroundDecal);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ScaleObjectInstances(CVegetationObject* object, float fScale, AABB* outModifiedArea)
{
    // Iterator over all sectors.
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->object == object)
            {
                // add the box before instance scaling
                if (outModifiedArea != nullptr && obj->pRenderNode != nullptr)
                {
                    outModifiedArea->Add(obj->pRenderNode->GetBBox());
                }

                // scale instance
                obj->scale *= fScale;
                RegisterInstance(obj);

                // add the box after the instance scaling
                if (outModifiedArea != nullptr && obj->pRenderNode != nullptr)
                {
                    outModifiedArea->Add(obj->pRenderNode->GetBBox());
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RandomRotateInstances(CVegetationObject* object)
{
    bool bUndo = CUndo::IsRecording();
    // Iterator over all sectors.
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->object == object)
            {
                if (bUndo)
                {
                    RecordUndo(obj);
                }
                obj->angle = rand();
                RegisterInstance(obj);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ClearRotateInstances(CVegetationObject* object)
{
    bool bUndo = CUndo::IsRecording();
    // Iterator over all sectors.
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->object == object && obj->angle != 0)
            {
                if (bUndo)
                {
                    RecordUndo(obj);
                }
                obj->angle = 0;
                RegisterInstance(obj);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::Serialize(CXmlArchive& xmlAr)
{
    if (xmlAr.bLoading)
    {
        CLogFile::WriteLine("Loading Vegetation Map...");

        ClearObjects();

        CWaitProgress progress(QObject::tr("Loading Static Objects"));

        //if (!progress.Step( 100*i/numObjects ))
        //break;

        XmlNodeRef mainNode = xmlAr.root;

        if (mainNode && !_stricmp(mainNode->getTag(), "VegetationMap"))
        {
            m_nVersion = 1;
            mainNode->getAttr("Version", m_nVersion);

            XmlNodeRef objects = mainNode->findChild("Objects");
            if (objects)
            {
                int numObjects = objects->getChildCount();
                for (int i = 0; i < numObjects; i++)
                {
                    CVegetationObject* obj = CreateObject();
                    if (obj)
                    {
                        auto c = objects->getChild(i);
                        obj->Serialize(c, xmlAr.bLoading);
                    }
                }
            }
        }

        SerializeInstances(xmlAr);


        LoadOldStuff(xmlAr);

        // Now display all objects on terrain.
        PlaceObjectsOnTerrain();
    }
    else
    {
        // Storing
        CLogFile::WriteLine("Saving Vegetation Map...");

        xmlAr.root = XmlHelpers::CreateXmlNode("VegetationMap");
        XmlNodeRef mainNode = xmlAr.root;

        m_nVersion = 3;
        mainNode->setAttr("Version", m_nVersion);

        XmlNodeRef objects = mainNode->newChild("Objects");
        SerializeObjects(objects);

        // Store objects.
        SerializeInstances(xmlAr);
    }
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::SerializeObjects(XmlNodeRef& vegetationNode)
{
    for (size_t i = 0, cnt = m_objects.size(); i < cnt; ++i)
    {
        XmlNodeRef node = vegetationNode->newChild("Object");
        m_objects[i]->Serialize(node, false);
    }
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::SerializeInstances(CXmlArchive& xmlAr, QRect* saveRect)
{
    if (xmlAr.bLoading)
    {
        Vec3 posofs(0, 0, 0);
        // Loading.
        if (!saveRect)
        {
            ClearSectors();
        }
        else
        {
            posofs.x = saveRect->left();
            posofs.y = saveRect->top();
        }

        int numObjects = m_objects.size();
        int arraySize;
        void* pData = 0;
        if (!xmlAr.pNamedData->GetDataBlock("VegetationInstancesArray", pData, arraySize))
        {
            return;
        }

        if (m_nVersion == 1)
        {
            SVegInst_V1_0* pVegInst = (SVegInst_V1_0*)pData;
            int numInst = arraySize / sizeof(SVegInst_V1_0);
            for (int i = 0; i < numInst; i++)
            {
                if (pVegInst[i].objectIndex >= numObjects)
                {
                    continue;
                }
                CVegetationObject* object = m_objects[pVegInst[i].objectIndex];
                CVegetationInstance* obj = CreateObjInstance(object, pVegInst[i].pos + posofs);
                if (obj)
                {
                    obj->scale = pVegInst[i].scale;
                    obj->brightness = pVegInst[i].brightness;
                    obj->angle = BYTE2RAD(pVegInst[i].angle);
                    obj->angleX = 0;
                    obj->angleY = 0;
                }
            }
        }
        else if (m_nVersion == 2)
        {
            SVegInst_V2_0* pVegInst = (SVegInst_V2_0*)pData;
            int numInst = arraySize / sizeof(SVegInst_V2_0);
            for (int i = 0; i < numInst; i++)
            {
                if (pVegInst[i].objectIndex >= numObjects)
                {
                    continue;
                }
                CVegetationObject* object = m_objects[pVegInst[i].objectIndex];
                CVegetationInstance* obj = CreateObjInstance(object, pVegInst[i].pos + posofs);
                if (obj)
                {
                    obj->scale = pVegInst[i].scale;
                    obj->brightness = pVegInst[i].brightness;
                    obj->angle = pVegInst[i].angle;
                    obj->angleX = pVegInst[i].angleX;
                    obj->angleY = pVegInst[i].angleY;
                }
            }
        }
        else
        {
            SVegInst* pVegInst = (SVegInst*)pData;
            int numInst = arraySize / sizeof(SVegInst);
            for (int i = 0; i < numInst; i++)
            {
                if (pVegInst[i].objectIndex < numObjects)
                {
                    CVegetationObject* object = m_objects[pVegInst[i].objectIndex];
                    CVegetationInstance* obj = CreateObjInstance(object, pVegInst[i].pos + posofs);
                    if (obj)
                    {
                        obj->scale = pVegInst[i].scale;
                        obj->brightness = pVegInst[i].brightness;
                        obj->angle = pVegInst[i].angle;
                        obj->angleX = pVegInst[i].angleX;
                        obj->angleY = pVegInst[i].angleY;
                    }
                }
            }
        }
    }
    else
    {
        // Saving.
        if (m_numInstances == 0)
        {
            return;
        }
        int arraySize = sizeof(SVegInst) * m_numInstances;
        SVegInst* array = (SVegInst*)malloc(arraySize);

        // Assign indices to objects.
        for (int i = 0; i < GetObjectCount(); ++i)
        {
            GetObject(i)->SetIndex(i);
        }

        float x1, y1, x2, y2;
        if (saveRect)
        {
            x1 = saveRect->left();
            y1 = saveRect->top();
            x2 = saveRect->right() + 1;
            y2 = saveRect->bottom() + 1;
        }

        // Fill array.
        int k = 0;
        for (int i = 0; i < m_numSectors * m_numSectors; ++i)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
            {
                if (saveRect)
                {
                    if (obj->pos.x < x1 || obj->pos.x > x2 || obj->pos.y < y1 || obj->pos.y > y2)
                    {
                        continue;
                    }
                }
                array[k].pos = obj->pos;
                array[k].scale = obj->scale;
                array[k].brightness = obj->brightness;
                array[k].angle = obj->angle;
                array[k].angleX = obj->angleX;
                array[k].angleY = obj->angleY;
                array[k].objectIndex = obj->object->GetIndex();
                k++;
            }
        }
        // Save this array.
        if (k > 0)
        {
            xmlAr.pNamedData->AddDataBlock("VegetationInstancesArray", array, k * sizeof(SVegInst));
        }
        free(array);
    }
}

void CVegetationMap::ClearSegment(const AABB& bb)
{
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        PodArray<CVegetationInstance*> lstDel;
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->pos.x < bb.min.x || obj->pos.x >= bb.max.x || obj->pos.y < bb.min.y || obj->pos.y >= bb.max.y)
            {
                continue;
            }
            lstDel.push_back(obj);
        }
        for (int j = 0; j < lstDel.size(); ++j)
        {
            DeleteObjInstance(lstDel[j], si);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ImportSegment(CMemoryBlock& mem, const Vec3& vOfs)
{
    //TODO: load vegetation objects info, update m_objects, remap indicies
    int iInstances = 0;

    typedef std::map<int, GUID> TIntGuidMap;
    typedef std::pair<int, GUID> TIntGuidPair;

    typedef std::map<int, int> TIntIntMap;
    typedef std::pair<int, int> TIntIntPair;

    bool bIsVegetationObjectInfoSaved = false;
    unsigned char* p = (unsigned char*)mem.GetBuffer();
    int curr = 0;

    if (mem.GetSize() > sizeof(int))
    {
        int bckwCompatibiliyFlag = 0;// = 0xFFFFFFFF;
        // flag for backward compatibility
        memcpy(&bckwCompatibiliyFlag, &p[curr], sizeof(int));
        curr += sizeof(int);

        if (bckwCompatibiliyFlag == 0xFFFFFFFF)
        {
            bIsVegetationObjectInfoSaved = true;
        }
    }

    TIntIntMap tIndexMap;
    if (bIsVegetationObjectInfoSaved)
    {
        // IntGuidMap
        TIntGuidMap VegetationIndexGuidMap;

        int numMapObjects = 0;
        // num objects
        memcpy(&numMapObjects, &p[curr], sizeof(int));
        curr += sizeof(int);

        for (int i = 0; i < numMapObjects; i++)
        {
            TIntGuidPair tPair;
            //object index
            memcpy(&tPair.first, &p[curr], sizeof(int));
            curr += sizeof(int);

            // GUID;
            memcpy(&tPair.second, &p[curr], sizeof(GUID));
            curr += sizeof(GUID);

            VegetationIndexGuidMap.insert(tPair);
        }

        //correct mapping
        TIntGuidMap::iterator it;

        for (it = VegetationIndexGuidMap.begin(); it != VegetationIndexGuidMap.end(); ++it)
        {
            for (int i = 0; i < GetObjectCount(); i++)
            {
                if (GetObject(i)->GetGUID() == it->second)
                {
                    TIntIntPair tIndexPair;
                    tIndexPair.first = it->first;
                    tIndexPair.second = i;
                    tIndexMap.insert(tIndexPair);
                    break;
                }
            }
        }

        // instances
        memcpy(&iInstances, &p[curr], sizeof(int)); // num of instances
        curr += sizeof(int);
    }

    SVegInst* array = bIsVegetationObjectInfoSaved ? (SVegInst*) &p[curr] : (SVegInst*) mem.GetBuffer();
    int numInst = bIsVegetationObjectInfoSaved ? iInstances : mem.GetSize() / sizeof(SVegInst);
    int numObjects = m_objects.size();
    for (int i = 0; i < numInst; i++)
    {
        int objectIndex = array[i].objectIndex;

        if (bIsVegetationObjectInfoSaved)
        {
            TIntIntMap::iterator it;
            it = tIndexMap.find(objectIndex);
            if (it == tIndexMap.end())
            {
                continue;
            }
            else
            {
                objectIndex = it->second;
            }
        }

        if (objectIndex >= numObjects)
        {
            continue;
        }
        CVegetationObject* object = m_objects[objectIndex];
        CVegetationInstance* obj = CreateObjInstance(object, array[i].pos + vOfs);
        if (obj)
        {
            obj->scale = array[i].scale;
            obj->brightness = array[i].brightness;
            obj->angle = array[i].angle;
            if (!object->IsHidden())
            {
                RegisterInstance(obj);
            }
        }
    }
}

void CVegetationMap::ExportSegment(CMemoryBlock& mem, const AABB& bb, const Vec3& vOfs)
{
    //TODO: save vegetation objects info with number of instances per object
    int i;

    // Assign indices to objects.
    for (i = 0; i < GetObjectCount(); i++)
    {
        GetObject(i)->SetIndex(i);
    }

    // count instances
    int iInstances = 0;

    typedef std::map<int, GUID> TIntGuidMap;
    typedef std::pair<int, GUID> TIntGuidPair;

    TIntGuidMap VegetationIndexGuidMap;

    for (i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->pos.x < bb.min.x || obj->pos.x >= bb.max.x || obj->pos.y < bb.min.y || obj->pos.y >= bb.max.y)
            {
                continue;
            }
            iInstances++;

            if (VegetationIndexGuidMap.find(obj->object->GetIndex()) == VegetationIndexGuidMap.end())
            {
                TIntGuidPair tPair;
                tPair.first = obj->object->GetIndex();
                tPair.second = obj->object->GetGUID();
                VegetationIndexGuidMap.insert(tPair);
            }
        }
    }
    if (!iInstances)
    {
        mem.Free();
        return;
    }

    //calculating size of map
    int sizeOfMem = 0;
    sizeOfMem += sizeof(int); // flag for backward compatibility
    sizeOfMem += sizeof(int); // num of objects

    TIntGuidMap::iterator it;

    for (it = VegetationIndexGuidMap.begin(); it != VegetationIndexGuidMap.end(); ++it)
    {
        sizeOfMem += sizeof(int); //object index
        sizeOfMem += sizeof(GUID);  // size of GUID;
    }

    sizeOfMem += sizeof(int); // num of instances
    sizeOfMem += sizeof(SVegInst) * iInstances; // instances

    int numElems = VegetationIndexGuidMap.size();

    CMemoryBlock tmp;
    tmp.Allocate(sizeOfMem);
    unsigned char* p = (unsigned char*)tmp.GetBuffer();
    int curr = 0;

    int bckwCompatibiliyFlag = 0xFFFFFFFF;
    memcpy(&p[curr], &bckwCompatibiliyFlag, sizeof(int)); // flag for backward compatibility
    curr += sizeof(int);

    memcpy(&p[curr], &numElems, sizeof(int)); // num of objects
    curr += sizeof(int);

    for (it = VegetationIndexGuidMap.begin(); it != VegetationIndexGuidMap.end(); ++it)
    {
        memcpy(&p[curr], &it->first, sizeof(int));
        curr += sizeof(int); //object index

        memcpy(&p[curr], &it->second, sizeof(GUID));
        curr += sizeof(GUID);   // GUID;
    }

    // save instances
    memcpy(&p[curr], &iInstances, sizeof(int)); // num of instances
    curr += sizeof(int);

    SVegInst* array = (SVegInst*) &p[curr];
    int k = 0;
    for (i = 0; i < m_numSectors * m_numSectors; i++)
    {
        SectorInfo* si = &m_sectors[i];
        // Iterate on every object in sector.
        for (CVegetationInstance* obj = si->first; obj; obj = obj->next)
        {
            if (obj->pos.x < bb.min.x || obj->pos.x >= bb.max.x || obj->pos.y < bb.min.y || obj->pos.y >= bb.max.y)
            {
                continue;
            }
            array[k].pos = obj->pos + vOfs;
            array[k].scale = obj->scale;
            array[k].brightness = obj->brightness;
            array[k].angle = obj->angle;
            array[k].objectIndex = obj->object->GetIndex();
            k++;
        }
    }
    assert(k == iInstances);
    tmp.Compress(mem);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::LoadOldStuff(CXmlArchive& xmlAr)
{
    if (!xmlAr.root)
    {
        return;
    }

    {
        XmlNodeRef mainNode = xmlAr.root->findChild("VegetationMap");
        if (mainNode)
        {
            XmlNodeRef objects = mainNode->findChild("Objects");
            if (objects)
            {
                int numObjects = objects->getChildCount();
                for (int i = 0; i < numObjects; i++)
                {
                    CVegetationObject* obj = CreateObject();
                    if (obj)
                    {
                        auto c = objects->getChild(i);
                        obj->Serialize(c, xmlAr.bLoading);
                    }
                }
            }
            SerializeInstances(xmlAr);
        }
    }

    XmlNodeRef mainNode = xmlAr.root->findChild("VegetationMap");
    if (!mainNode)
    {
        return;
    }

    // Backward compatibility loading of brushes.
    XmlNodeRef brushes = mainNode->findChild("Brushes");
    if (brushes)
    {
        for (int i = 0; i < brushes->getChildCount(); ++i)
        {
            CVegetationBrush br;
            auto b = brushes->getChild(i);
            br.Serialize(b, xmlAr.bLoading);
        }
    }

    void* pData = 0;
    int nSize;
    if (xmlAr.pNamedData->GetDataBlock("StatObjectsArray", pData, nSize))
    {
        if (nSize != 2048 * 2048)
        {
            return;
        }

        int numObjects = m_objects.size();
        CVegetationObject* usedObjects[256];
        ZeroStruct(usedObjects);

        for (int i = 0; i < m_objects.size(); ++i)
        {
            usedObjects[m_objects[i]->GetIndex()] = m_objects[i];
        }

        Vec3 pos;
        uint8* pMap = (uint8*)pData;
        for (int y = 0; y < 2048; ++y)
        {
            for (int x = 0; x < 2048; ++x)
            {
                int i = x + 2048 * y;
                if (!pMap[i])
                {
                    continue;
                }

                unsigned int objIndex = pMap[i] - 1;
                if (!usedObjects[objIndex])
                {
                    continue;
                }

                //Swap x/y
                pos.x = y;
                pos.y = x;
                pos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(pos.x, pos.y);
                CVegetationInstance* obj = CreateObjInstance(usedObjects[objIndex], pos);
                if (obj)
                {
                    obj->scale = usedObjects[objIndex]->CalcVariableSize();
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//! Generate shadows from static objects and place them in shadow map bitarray.
void CVegetationMap::GenerateShadowMap(CByteImage& shadowmap, float shadowAmmount, const Vec3& sunVector)
{
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();

    int width = shadowmap.GetWidth();
    int height = shadowmap.GetHeight();

    //@FIXME: Hardcoded.
    int sectorSizeInMeters = 64;

    int unitSize = pHeightmap->GetUnitSize();
    int numSectors = (pHeightmap->GetWidth() * unitSize) / sectorSizeInMeters;

    int sectorSize = shadowmap.GetWidth() / numSectors;
    int sectorSize2 = sectorSize * 2;
    assert(sectorSize > 0);

    uint32 shadowValue = shadowAmmount;

    bool bProgress = width >= 2048;
    CWaitProgress wait("Calculating Objects Shadows", bProgress);

    unsigned char* sectorImage = (unsigned char*)malloc(sectorSize * sectorSize * 3);
    unsigned char* sectorImage2 = (unsigned char*)malloc(sectorSize2 * sectorSize2 * 3);

    for (int y = 0; y < numSectors; y++)
    {
        if (bProgress)
        {
            if (!wait.Step(y * 100 / numSectors))
            {
                break;
            }
        }

        for (int x = 0; x < numSectors; x++)
        {
            //          GetIEditor()->Get3DEngine()->MakeTerrainLightMap( y*sectorSizeInMeters,x*sectorSizeInMeters,sectorSizeInMeters,sectorImage2,sectorSize2 );
            memset(sectorImage2, 255, sectorSize2 * sectorSize2 * 3);

            // Scale sector image down and store into the shadow map.
            {
                int pos;
                uint32 color;
                int x1 = x * sectorSize;
                int y1 = y * sectorSize;
                for (int j = 0; j < sectorSize; j++)
                {
                    int sx1 = x1 + (sectorSize - j - 1);
                    for (int i = 0; i < sectorSize; i++)
                    {
                        pos = (i + j * sectorSize2) * 2 * 3;
                        color = (shadowValue *
                                 ((uint32)
                                  (255 - sectorImage2[pos]) +
                                  (255 - sectorImage2[pos + 3]) +
                                  (255 - sectorImage2[pos + sectorSize2 * 3]) +
                                  (255 - sectorImage2[pos + sectorSize2 * 3 + 3])
                                 )) >> 10;
                        //                      color = color*shadowValue >> 8;
                        // swap x/y
                        //color = (255-sectorImage2[(i+j*sectorSize)*3]);
                        shadowmap.ValueAt(sx1, y1 + i) = color;
                    }
                }
            }
        }
    }
    free(sectorImage);
    free(sectorImage2);
}

//////////////////////////////////////////////////////////////////////////
int CVegetationMap::ExportObject(CVegetationObject* object, XmlNodeRef& node, QRect* saveRect)
{
    int numSaved = 0;
    assert(object != 0);
    object->Serialize(node, false);
    if (object->GetNumInstances() > 0)
    {
        float x1, y1, x2, y2;
        if (saveRect)
        {
            x1 = saveRect->left();
            y1 = saveRect->top();
            x2 = saveRect->right();
            y2 = saveRect->bottom();
        }
        // Export instances.
        XmlNodeRef instancesNode = node->newChild("Instances");
        // Iterator over all sectors.
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = m_sectors[i].first; obj; obj = obj->next)
            {
                if (obj->object == object)
                {
                    if (saveRect)
                    {
                        if (obj->pos.x < x1 || obj->pos.x > x2 || obj->pos.y < y1 || obj->pos.y > y2)
                        {
                            continue;
                        }
                    }
                    numSaved++;
                    XmlNodeRef inst = instancesNode->newChild("Instance");
                    inst->setAttr("Pos", obj->pos);
                    if (obj->scale != 1)
                    {
                        inst->setAttr("Scale", obj->scale);
                    }
                    if (obj->brightness != 255)
                    {
                        inst->setAttr("Brightness", (int)obj->brightness);
                    }
                    if (obj->angle != 0)
                    {
                        inst->setAttr("Angle", (int)obj->angle);
                    }
                }
            }
        }
    }
    return numSaved;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ImportObject(XmlNodeRef node, const Vec3& offset)
{
    CVegetationObject* object = CreateObject();
    if (!object)
    {
        return;
    }
    object->Serialize(node, true);

    // Check if object with this GUID. already exists.
    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject* pOldObject = GetObject(i);
        if (pOldObject == object)
        {
            continue;
        }
        if (pOldObject->GetGUID() == object->GetGUID())
        {
            ReplaceObject(pOldObject, object);
            /*
            // 2 objects have same GUID;
            CString msg;
            msg.Format( _T("Vegetation Object %s %s already exists in the level!.\r\nOverride existing object?"),
                                        GuidUtil::ToString(pOldObject->GetGUID()),(const char*)pOldObject->GetFileName() );

            if (QMessageBox( msg,MB_YESNO|MB_ICONWARNING ) == IDYES)
            {
                ReplaceObject( pOldObject,object );
            }
            else
            {
                RemoveObject( object );
                object = pOldObject;
            }
            */
            break;
        }
    }

    XmlNodeRef instancesNode = node->findChild("Instances");
    if (instancesNode)
    {
        int numChilds = instancesNode->getChildCount();
        for (int i = 0; i < numChilds; i++)
        {
            Vec3 pos(0, 0, 0);
            float scale = 1;
            int brightness = 255;
            int angle = 0;

            XmlNodeRef inst = instancesNode->getChild(i);
            inst->getAttr("Pos", pos);
            inst->getAttr("Scale", scale);
            inst->getAttr("Brightness", brightness);
            inst->getAttr("Angle", angle);

            pos += offset;
            CVegetationInstance* obj = GetNearestInstance(pos, 0.01f);
            if (obj && obj->pos == pos)
            {
                // Delete pevious object at same position.
                DeleteObjInstance(obj);
            }
            obj = CreateObjInstance(object, pos);
            if (obj)
            {
                obj->scale = scale;
                obj->brightness = brightness;
                obj->angle = angle;
                // Stick vegetation to terrain.
                if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
                {
                    obj->pos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(obj->pos.x, obj->pos.y);
                }
            }
        }
    }
    RepositionObject(object);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::DrawToTexture(uint32* texture, int texWidth, int texHeight, int srcX, int srcY)
{
    assert(texture != 0);

    for (int y = 0; y < texHeight; y++)
    {
        int trgYOfs = y * texWidth;
        for (int x = 0; x < texWidth; x++)
        {
            int sx = x + srcX;
            int sy = y + srcY;
            // Swap X/Y
            SectorInfo* si = &m_sectors[sy + sx * m_numSectors];
            if (si->first)
            {
                texture[x + trgYOfs] = 0xFFFFFFFF;
            }
            else
            {
                texture[x + trgYOfs] = 0;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CVegetationMap::WorldToSector(float worldCoord) const
{
    return ftoi(worldCoord * m_worldToSector);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::UnloadObjectsGeometry()
{
    for (int i = 0; i < GetObjectCount(); i++)
    {
        GetObject(i)->UnloadObject();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ExportBlock(const QRect& subRc, CXmlArchive& ar)
{
    XmlNodeRef mainNode = ar.root->newChild("VegetationMap");
    mainNode->setAttr("X1", subRc.left());
    mainNode->setAttr("Y1", subRc.top());
    mainNode->setAttr("X2", subRc.right() + 1);
    mainNode->setAttr("Y2", subRc.bottom() + 1);

    QRect rect = subRc;
    for (int i = 0; i < GetObjectCount(); i++)
    {
        XmlNodeRef vegObjNode = mainNode->createNode("VegetationObject");
        CVegetationObject* pObject = GetObject(i);
        int numSaved = ExportObject(pObject, vegObjNode, &rect);
        if (numSaved > 0)
        {
            mainNode->addChild(vegObjNode);
        }
    }

    //SerializeInstances( ar,&rect );
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ImportBlock(CXmlArchive& ar, const QPoint& placeOffset)
{
    XmlNodeRef mainNode = ar.root->findChild("VegetationMap");
    if (!mainNode)
    {
        return;
    }

    int left = 0, top = 0, right = 1, bottom = 1;
    mainNode->getAttr("X1", left);
    mainNode->getAttr("Y1", top);
    mainNode->getAttr("X2", right);
    mainNode->getAttr("Y2", bottom);
    QRect subRc(QPoint(left, top), QPoint(right, bottom) - QPoint(1, 1));

    subRc.translate(placeOffset);

    // Clear all vegetation instances in this rectangle.
    ClearBrush(subRc, false, NULL);

    // Serialize vegitation objects.
    for (int i = 0; i < mainNode->getChildCount(); i++)
    {
        XmlNodeRef vegObjNode = mainNode->getChild(i);
        ImportObject(vegObjNode, Vec3(placeOffset.x(), placeOffset.y(), 0));
    }
    // Clear object in this rectangle.
    QRect rect(placeOffset, placeOffset);
    SerializeInstances(ar, &rect);

    // Now display all objects on terrain.
    PlaceObjectsOnTerrain();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RepositionArea(const AABB& box, const Vec3& offset, ImageRotationDegrees rotation, bool isCopy)
{
    Vec3 src = (box.min + box.max) / 2;
    Vec3 dst = src + offset;
    float alpha = 0.0f;
    switch (rotation)
    {
        case ImageRotationDegrees::Rotate90:
            alpha = gf_halfPI;
            break;
        case ImageRotationDegrees::Rotate180:
            alpha = gf_PI;
            break;
        case ImageRotationDegrees::Rotate270:
            alpha = gf_PI + gf_halfPI;
            break;
        default:
            break;
    }
    float cosa = cos(alpha);
    float sina = sin(alpha);

    float x1 = min(box.min.x, box.max.x);
    float y1 = min(box.min.y, box.max.y);
    float x2 = max(box.min.x, box.max.x);
    float y2 = max(box.min.y, box.max.y);

    // check all sectors intersected by radius.
    int sx1 = ftoi(x1 * m_worldToSector);
    int sx2 = ftoi(x2 * m_worldToSector);
    int sy1 = ftoi(y1 * m_worldToSector);
    int sy2 = ftoi(y2 * m_worldToSector);
    sx1 = max(sx1, 0);
    sx2 = min(sx2, m_numSectors - 1);
    sy1 = max(sy1, 0);
    sy2 = min(sy2, m_numSectors - 1);

    int px1 = ftoi((x1 + offset.x) * m_worldToSector);
    int px2 = ftoi((x2 + offset.x) * m_worldToSector);
    int py1 = ftoi((y1 + offset.y) * m_worldToSector);
    int py2 = ftoi((y2 + offset.y) * m_worldToSector);

    if (rotation == ImageRotationDegrees::Rotate90 || rotation == ImageRotationDegrees::Rotate270)
    {
        px1 = ftoi((dst.x - (box.max.y - box.min.y)) * m_worldToSector);
        px2 = ftoi((dst.x + (box.max.y - box.min.y)) * m_worldToSector);
        py1 = ftoi((dst.y - (box.max.x - box.min.x)) * m_worldToSector);
        py2 = ftoi((dst.y + (box.max.x - box.min.x)) * m_worldToSector);
    }

    px1 = max(px1, 0);
    px2 = min(px2, m_numSectors - 1);
    py1 = max(py1, 0);
    py2 = min(py2, m_numSectors - 1);

    CVegetationInstance* next = 0;

    // Change sector
    // cycle trough sectors under source
    for (int y = sy1; y <= sy2; y++)
    {
        for (int x = sx1; x <= sx2; x++)
        {
            // For each sector check if any object is within this radius.
            SectorInfo* si = GetVegSector(x, y);
            CVegetationInstance* obj = si->first;
            while (obj)
            {
                next = obj->next;
                bool isUnlink = false;

                if (obj->object &&
                    x1 <= obj->pos.x && obj->pos.x <= x2 &&
                    y1 <= obj->pos.y && obj->pos.y <= y2)
                {
                    Vec3 pos = obj->pos - src;
                    Vec3 newPos = Vec3(cosa * pos.x - sina * pos.y, sina * pos.x + cosa * pos.y, pos.z) + dst;

                    if (isCopy)
                    {
                        if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
                        {
                            newPos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(newPos.x, newPos.y);
                        }

                        if (CanPlace(obj->object, newPos, m_minimalDistance))
                        {
                            CVegetationInstance* pInst = CreateObjInstance(obj->object, newPos, obj);
                            RegisterInstance(pInst);
                        }
                    }
                    else // if move instances
                    {
                        SectorInfo* to = GetVegSector(newPos);

                        if (to && si != to)
                        {
                            // Relink object between sectors.
                            SectorUnlink(obj, si);
                            if (to)
                            {
                                SectorLink(obj, to);
                            }
                            isUnlink = true;
                        }
                    }
                }

                if (isUnlink)
                {
                    obj = si->first;
                }
                else
                {
                    obj = next;
                }
            }
        }
    }

    if (!isCopy) // if move instances
    {
        // Change position
        for (int y = py1; y <= py2; y++)
        {
            for (int x = px1; x <= px2; x++)
            {
                // For each sector check if any object is within this radius.
                SectorInfo* si = GetVegSector(x, y);
                CVegetationInstance* obj = si->first;
                while (obj)
                {
                    next = obj->next;

                    if (obj->object &&
                        x1 <= obj->pos.x && obj->pos.x <= x2 &&
                        y1 <= obj->pos.y && obj->pos.y <= y2)
                    {
                        Vec3 pos = obj->pos - src;
                        Vec3 newPos = Vec3(cosa * pos.x - sina * pos.y, sina * pos.x + cosa * pos.y, pos.z) + dst;

                        float newz = newPos.z;
                        if (!obj->object->IsAffectedByBrushes() && obj->object->IsAffectedByTerrain())
                        {
                            newz = GetIEditor()->Get3DEngine()->GetTerrainElevation(newPos.x, newPos.y);
                        }
                        newPos.z = newz;
                        if (newPos.z != newz)
                        {
                            RecordUndo(obj);
                        }

                        obj->pos = newPos;
                        if (!obj->object->IsHidden())
                        {
                            RegisterInstance(obj);
                        }
                    }

                    obj = next;
                }
            }
        }
    }

    RemoveDuplVegetation(px1, py1, px2, py2);

    if (offset.x != 0 || offset.y != 0)
    {
        AABB destBox = box;
        destBox.min.x = min(box.min.x, box.max.x) + offset.x;
        destBox.min.y = min(box.min.y, box.max.y) + offset.y;
        destBox.min.z = min(box.min.z, box.max.z) + offset.z;

        destBox.max.x = max(box.min.x, box.max.x) + offset.x;
        destBox.max.y = max(box.min.y, box.max.y) + offset.y;
        destBox.max.z = max(box.min.z, box.max.z) + offset.z;
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::RecordUndo(CVegetationInstance* obj)
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoVegInstance(obj));
    }
}
//////////////////////////////////////////////////////////////////////////
void CVegetationMap::Validate(IErrorReport& report)
{
    for (int i = 0; i < GetObjectCount(); i++)
    {
        GetObject(i)->Validate(report);
    }
}

//////////////////////////////////////////////////////////////////////////
int CVegetationMap::GetTexureMemoryUsage(bool bOnlySelectedObjects)
{
    ICrySizer* pSizer = GetISystem()->CreateSizer();

    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject* pObject = GetObject(i);
        if (!pObject->IsSelected() && bOnlySelectedObjects)
        {
            continue;
        }

        pObject->GetTextureMemUsage(pSizer);
    }
    int nSize = pSizer->GetTotalSize();
    pSizer->Release();
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
int CVegetationMap::GetSpritesMemoryUsage(bool bOnlySelectedObjects)
{
    int nSize = 0;
    std::set<IStatObj*> objset;

    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject* pObject = GetObject(i);
        if (!pObject->IsSelected() && bOnlySelectedObjects)
        {
            continue;
        }

        if (!pObject->IsUseSprites())
        {
            continue;
        }

        IStatObj* pStatObj = pObject->GetObject();
        if (objset.find(pStatObj) != objset.end())
        {
            continue;
        }

        objset.insert(pStatObj);

        //      nSize += pStatObj->GetSpritesTexMemoryUsage(); // statobj does not contain sprites in it
    }
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationMap::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(*this);

    pSizer->Add(m_usedIds);

    {
        pSizer->Add(m_objects);

        Objects::iterator it, end = m_objects.end();

        for (it = m_objects.begin(); it != end; ++it)
        {
            TSmartPtr<CVegetationObject>& ref = *it;

            pSizer->Add(*ref);
        }
    }

    {
        pSizer->Add(m_sectors, sizeof(SectorInfo) * m_numSectors * m_numSectors);

        CVegetationInstance* next;
        for (int i = 0; i < m_numSectors * m_numSectors; i++)
        {
            SectorInfo* si = &m_sectors[i];
            // Iterate on every object in sector.
            for (CVegetationInstance* obj = si->first; obj; obj = next)
            {
                next = obj->next;

                pSizer->Add(*obj);
            }
        }
    }


    // todo : finish m_objects
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::SetEngineObjectsParams()
{
    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject* obj = GetObject(i);
        obj->SetEngineParams();
    }
}


//////////////////////////////////////////////////////////////////////////
int CVegetationMap::GenerateVegetationObjectId()
{
    int id = -1;
    // Generate New id.
#ifdef max
#undef max
#endif
    for (int i = 0; i < std::numeric_limits<int>::max(); ++i)
    {
        if (m_usedIds.find(i) == m_usedIds.end())
        {
            id = i;
            break;
        }
    }

    return id;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::UpdateConfigSpec()
{
    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject* obj = GetObject(i);
        obj->OnConfigSpecChange();
    }
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::Save()
{
    CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kVegetationMapFile).toUtf8());

    CXmlArchive xmlAr;
    Serialize(xmlAr);
    xmlAr.Save(helper.GetTempFilePath());

    helper.UpdateFile(false);
}


//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::Load()
{
    QString filename = GetIEditor()->GetLevelDataFolder() + kVegetationMapFile;
    CXmlArchive xmlAr;
    xmlAr.bLoading = true;
    if (!xmlAr.Load(filename))
    {
        QString filename = GetIEditor()->GetLevelDataFolder() + kVegetationMapFileOld;
        if (!xmlAr.Load(filename))
        {
            return false;
        }
    }

    Serialize(xmlAr);
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationMap::ReloadGeometry()
{
    for (int i = 0; i < GetObjectCount(); i++)
    {
        IStatObj* pObject = GetObject(i)->GetObject();
        if (pObject)
        {
            pObject->Refresh(FRO_SHADERS | FRO_TEXTURES | FRO_GEOMETRY);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
bool CVegetationMap::IsAreaEmpty(const AABB& bbox)
{
    std::vector<CVegetationInstance*> foundVegetationIstances;
    foundVegetationIstances.clear();
    bool bHasVeg = false;

    GetObjectInstances(bbox.min.x, bbox.min.y, bbox.max.x, bbox.max.y, foundVegetationIstances);

    for (int vegNo = 0; vegNo < foundVegetationIstances.size(); ++vegNo)
    {
        CVegetationInstance* pVegInstance = (CVegetationInstance*)foundVegetationIstances[vegNo];
        if (!pVegInstance)
        {
            continue;
        }

        if (!pVegInstance->object)
        {
            continue;
        }

        IStatObj* pStatObj = pVegInstance->object->GetObject();

        if (!pStatObj)
        {
            continue;
        }

        Vec3 expandMin = pVegInstance->pos + pStatObj->GetBoxMin() * pVegInstance->scale;
        Vec3 expandMax = pVegInstance->pos + pStatObj->GetBoxMax() * pVegInstance->scale;

        AABB vegBBox(expandMin, expandMax);

        if (bbox.ContainsBox(vegBBox))
        {
            bHasVeg = true;
        }

        if (pVegInstance->pRenderNode && bHasVeg)
        {
            pVegInstance->object->SetSelected(true);
        }
    }

    return !(foundVegetationIstances.size() > 0 && bHasVeg);
}

void CVegetationMap::StoreBaseUndo(EStoreUndo state)
{
    bool bRecordUndo = false;

    switch (state)
    {
    case eStoreUndo_Normal:
        if (eStoreUndo_Normal == m_storeBaseUndoState)
        {
            bRecordUndo = true;
        }
        break;

    case eStoreUndo_Begin:
        m_storeBaseUndoState = eStoreUndo_Begin;
        break;

    case eStoreUndo_Once:
        if (eStoreUndo_Begin == m_storeBaseUndoState)
        {
            bRecordUndo = true;
            m_storeBaseUndoState = eStoreUndo_Once;
        }
        break;

    case eStoreUndo_End:
        if (eStoreUndo_Once == m_storeBaseUndoState)
        {
            bRecordUndo = true;
        }
        m_storeBaseUndoState = eStoreUndo_Normal;
        break;
    }
    ;


    if (bRecordUndo)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoVegetationBase(false));
        }
    }
}

AZStd::vector<CVegetationInstance*> CVegetationMap::GetObjectInstances(const AZ::Vector2& min, const AZ::Vector2& max)
{
    AZStd::vector<CVegetationInstance*> result;
    std::vector<CVegetationInstance*> vegetationInstances;

    GetObjectInstances(min.GetX(), min.GetY(), max.GetX(), max.GetY(), vegetationInstances);
    result.reserve(vegetationInstances.size());

    for (const auto& vegetation : vegetationInstances)
    {
        result.push_back(vegetation);
    }

    return result;
}

void CVegetationMap::DeleteObjectInstance(CVegetationInstance* instance)
{
    DeleteObjInstance(instance);
}
