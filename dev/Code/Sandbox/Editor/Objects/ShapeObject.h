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

// Description : ShapeObject object definition.

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SHAPEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_SHAPEOBJECT_H
#pragma once


#include <INavigationSystem.h>
#include "EntityObject.h"
#include "SafeObjectsArray.h"
#include "RenderHelpers/AxisHelper.h"

#define SHAPE_CLOSE_DISTANCE            0.8f
#define SHAPE_POINT_MIN_DISTANCE    0.1f    // Set to 10 cm (this number has been found in cooperation with C2 level designers)

class ReflectedPropertiesPanel;

class CAIWaveObject;
/*!
 *  CShapeObject is an object that represent named 3d position in world.
 *
 */
class SANDBOX_API CShapeObject
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    void Done();
    bool HasMeasurementAxis() const {   return true;    }
    void Display(DisplayContext& dc);

    //////////////////////////////////////////////////////////////////////////
    void SetName(const QString& name);

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);

    bool HitTest(HitContext& hc);
    bool HitTestRect(HitContext& hc);

    void OnEvent(ObjectEvent event);
    virtual void PostLoad(CObjectArchive& ar);

    virtual void SpawnEntity();
    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////

    int GetAreaId();

    void SetClosed(bool bClosed);
    bool IsClosed() { return mv_closed; };

    //! Insert new point to shape at given index.
    //! @return index of newly inserted point.
    virtual int InsertPoint(int index, const Vec3& point, bool const bModifying);
    //! Set split point.
    int SetSplitPoint(int index, const Vec3& point, int numPoint);
    //! Split shape on two shapes.
    void Split();
    //! Remove point from shape at given index.
    virtual void RemovePoint(int index);
    //! Set edge index for merge.
    void SetMergeIndex(int index);
    //! Merge shape to this.
    void Merge(CShapeObject* shape);
    //! Remove all points.
    void ClearPoints();

    //! Get number of points in shape.
    int GetPointCount() const { return m_points.size(); };
    //! Get point at index.
    const Vec3& GetPoint(int index) const { return m_points[index]; };
    //! Set point position at specified index.
    virtual void SetPoint(int index, const Vec3& pos);

    //! Reverses the points of the path
    void    ReverseShape();

    //! Reset z-value of points
    void    ResetShape();

    void SelectPoint(int index);
    int GetSelectedPoint() const { return m_selectedPoint; };

    //! Set shape width.
    float GetWidth() const { return mv_width; };

    //! Set shape height.
    float GetHeight() const { return mv_height; };

    //! Find shape point nearest to given point.
    int GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance);

    //! Find shape edge nearest to given point.
    void GetNearestEdge(const Vec3& pos, int& p1, int& p2, float& distance, Vec3& intersectPoint);

    //! Find shape edge nearest to given ray.
    void GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint);
    //////////////////////////////////////////////////////////////////////////

    static CAxisHelper& GetSelelectedPointAxisHelper() { return m_selectedPointAxis; }
    bool    UseAxisHelper() const { return m_useAxisHelper; }
    void    SetUseAxisHelper(bool state) { m_useAxisHelper = state; }

    //////////////////////////////////////////////////////////////////////////
    virtual void SetSelected(bool bSelect);

    //////////////////////////////////////////////////////////////////////////
    // Binded entities.
    //////////////////////////////////////////////////////////////////////////
    //! Bind entity to this shape.
    void AddEntity(CBaseObject* entity);
    void RemoveEntity(int index);
    CBaseObject* GetEntity(int index);
    int GetEntityCount() { return m_entities.GetCount(); }

    virtual float GetShapeZOffset() const { return 0.1f; }

    virtual void CalcBBox();
    virtual void EndPointModify(){}

public:
    CShapeObject();

    static const GUID& GetClassID()
    {
        // {6167DD9D-73D5-4d07-9E92-CF12AF451B08}
        static const GUID guid = {
            0x6167dd9d, 0x73d5, 0x4d07, { 0x9e, 0x92, 0xcf, 0x12, 0xaf, 0x45, 0x1b, 0x8 }
        };
        return guid;
    }

protected:
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    bool RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt);
    virtual bool IsOnlyUpdateOnUnselect() const { return false; }
    virtual int GetMaxPoints() const { return 100000; };
    virtual int GetMinPoints() const { return 3; };
    ////! Calculate distance between
    //float DistanceToShape( const Vec3 &pos );
    void DrawTerrainLine(DisplayContext& dc, const Vec3& p1, const Vec3& p2);
    void AlignToGrid();

    // Ignore default draw highlight.
    void DrawHighlight(DisplayContext& dc) {};

    virtual void EndCreation();

public:
    //! Update game area.
    virtual void UpdateGameArea(bool bRemove = false);

protected:
    //overrided from CBaseObject.
    void InvalidateTM(int nWhyFlags);
    virtual bool ConvertFromObject(CBaseObject* pObject) override;

    //! Called when shape variable changes.
    virtual void OnShapeChange(IVariable* var);

    void OnSoundParamsChange(IVariable* var);

    //! Called when shape point sound obstruction variable changes.
    void OnPointChange(IVariable* var);

    void DeleteThis() { delete this; };

    void DisplayNormal(DisplayContext& dc);
    void DisplaySoundInfo(DisplayContext& dc);

private:

    void    UpdateSoundPanelParams();

protected:

    AABB m_bbox;

    std::vector<Vec3>   m_points;

    QString m_lastGameArea;

    bool    m_useAxisHelper;

    //! List of binded entities.
    //std::vector<uint32> m_entities;

    // Entities.
    CSafeObjectsArray m_entities;

    //////////////////////////////////////////////////////////////////////////
    // Shape parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<float> mv_width;
    CVariable<float> mv_height;
    CVariable<int> mv_areaId;
    CVariable<int> mv_groupId;
    CVariable<int> mv_priority;

    CVariable<bool> mv_closed;
    //! Display triangles filling closed shape.
    CVariable<bool> mv_displayFilled;
    CVariable<bool> mv_displaySoundInfo;

    // For Mesh navigation.
    CVariable<bool> m_vEnabled;
    CVariable<bool> m_vExclusion;
    CVariable<float> mv_agentWidth;
    CVariable<float> mv_agentHeight;
    CVariable<float> mv_VoxelOffsetX;
    CVariable<float> mv_VoxelOffsetY;
    CVariable<bool> mv_renderVoxelGrid;

    int m_selectedPoint;
    float m_lowestHeight;

    uint32 m_bAreaModified : 1;
    //! Forces shape to be always 2D. (all vertices lie on XY plane).
    uint32 m_bForce2D : 1;
    uint32 m_bDisplayFilledWhenSelected : 1;
    uint32 m_bIgnoreGameUpdate : 1;
    uint32 m_bPerVertexHeight : 1;
    uint32 m_bNoCulling : 1;

    static int m_rollupId;
    static class ShapeEditSplitPanel* m_panel;
    static int m_rollupMultyId;
    static class CShapeMultySelPanel* m_panelMulty;
    static CAxisHelper  m_selectedPointAxis;

    Vec3 m_splitPoints[2];
    int m_splitIndicies[2];
    int m_numSplitPoints;

    int m_mergeIndex;

    bool m_updateSucceed;

    bool m_bBeingManuallyCreated;

    // Sound specific members
    std::vector<bool>                   m_abObstructSound;
    static ReflectedPropertiesPanel*    m_pSoundPropertiesPanel;
    static int                              m_nSoundPanelID;
    static CVarBlockPtr             m_pSoundPanelVarBlock;
};

//////////////////////////////////////////////////////////////////////////
// Base class for all AI Shapes
//////////////////////////////////////////////////////////////////////////
class CAIShapeObjectBase
    : public CShapeObject
{
public:
    CAIShapeObjectBase() { m_entityClass = ""; }
    virtual bool HasMeasurementAxis() const {   return false;   }
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return 0; };
};
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Special object for AI Walkabale paths.
//////////////////////////////////////////////////////////////////////////
class CAIPathObject
    : public CAIShapeObjectBase
{
    Q_OBJECT
public:
    CAIPathObject();

    static const GUID& GetClassID()
    {
        // {06380C56-6ECB-416f-9888-60DE08F0280B}
        static const GUID guid = {
            0x6380c56, 0x6ecb, 0x416f, { 0x98, 0x88, 0x60, 0xde, 0x8, 0xf0, 0x28, 0xb }
        };
        return guid;
    }

    // Ovverride game area creation.
    virtual void UpdateGameArea(bool bRemove = false) override;
    virtual void Display(DisplayContext& dc) override;
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) override { return 0; };

    virtual void RemovePoint(int index) override;
    virtual int InsertPoint(int index, const Vec3& point, bool const bModifying) override;
    virtual void SetPoint(int index, const Vec3& pos) override;

protected:
    virtual int GetMinPoints() const override { return 2; }

private:

    bool    IsSegmentValid(const Vec3& p0, const Vec3& p1, float rad);
    void    DrawSphere(DisplayContext& dc, const Vec3& p0, float rad, int n);
    void    DrawCylinder(DisplayContext& dc, const Vec3& p0, const Vec3& p1, float rad, int n);

    CVariable<bool> m_bRoad;
    CVariableEnum<int> m_navType;
    CVariable<QString> m_anchorType;
    CVariable<bool> m_bValidatePath;
};

//////////////////////////////////////////////////////////////////////////
// Generic AI shape
//////////////////////////////////////////////////////////////////////////
class CAIShapeObject
    : public CAIShapeObjectBase
{
    Q_OBJECT
public:
    CAIShapeObject();
    static const GUID& GetClassID()
    {
        // {2DDF910A-3D86-4f13-BF9D-2F4C1F5F6334}
        static const GUID guid = {
            0x2ddf910a, 0x3d86, 0x4f13, { 0xbf, 0x9d, 0x2f, 0x4c, 0x1f, 0x5f, 0x63, 0x34 }
        };
        return guid;
    }

    // Ovverride game area creation.
    virtual void UpdateGameArea(bool bRemove = false);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return 0; };
private:
    CVariable<QString> m_anchorType;
    CVariableEnum<int> m_lightLevel;
};

//////////////////////////////////////////////////////////////////////////
// Special object for AI Walkabale paths.
//////////////////////////////////////////////////////////////////////////
class CAIOcclusionPlaneObject
    : public CAIShapeObjectBase
{
    Q_OBJECT
public:
    CAIOcclusionPlaneObject();
    static const GUID& GetClassID()
    {
        // {85F4FEB1-1E97-4d78-BC0F-CACEA0D539C3}
        static const GUID guid =
        {
            0x85f4feb1, 0x1e97, 0x4d78, { 0xbc, 0xf, 0xca, 0xce, 0xa0, 0xd5, 0x39, 0xc3 }
        };
        return guid;
    }

    // Ovverride game area creation.
    virtual void UpdateGameArea(bool bRemove = false);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return 0; };
};


//////////////////////////////////////////////////////////////////////////
// Object for general AI perception modification
//////////////////////////////////////////////////////////////////////////
class CAIPerceptionModifierObject
    : public CAIShapeObjectBase
{
    Q_OBJECT
public:
    CAIPerceptionModifierObject();
    static const GUID& GetClassID()
    {
        // {6DF13CAA-28E9-4c16-9731-D6809FED2202}
        static const GUID guid =
        {
            0x6df13caa, 0x28e9, 0x4c16, { 0x97, 0x31, 0xd6, 0x80, 0x9f, 0xed, 0x22, 0x2 }
        };
        return guid;
    }

    // Override init variables to add our own, and not put irrelevant ones on the rollup bar
    void InitVariables();
    // Override game area creation.
    void UpdateGameArea(bool bRemove = false);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return 0; };
private:
    CVariable<float> m_fReductionPerMetre; // Value is from 0-1, UI shows percent of 0-100
    CVariable<float> m_fReductionMax; // Value is from 0-1, UI shows percent of 0-100
};


//////////////////////////////////////////////////////////////////////////
// Object for AI Territories
//////////////////////////////////////////////////////////////////////////
class CAITerritoryObject
    : public CAIShapeObjectBase
{
    Q_OBJECT
public:
    CAITerritoryObject();
    static const GUID& GetClassID()
    {
        // {EF05A2F1-BE56-4c09-82D4-C63B40DB823B}
        static const GUID guid =
        {
            0xef05a2f1, 0xbe56, 0x4c09, { 0x82, 0xd4, 0xc6, 0x3b, 0x40, 0xdb, 0x82, 0x3b }
        };
        return guid;
    }

    void BeginEditParams(IEditor* ie, int flags);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();
    void SetName(const QString& newName);
    // Override init variables to add our own, and not put irrelevant ones on the rollup bar
    void InitVariables();
    // Override game area creation.
    void UpdateGameArea(bool bRemove = false);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return CEntityObject::Export(levelPath, xmlNode); }
    void GetLinkedWaves(std::vector<CAIWaveObject*>& result);

protected:
#ifdef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
    virtual int GetMaxPoints() const { return 1; }
    virtual int GetMinPoints() const { return 1; }
#endif
};

//////////////////////////////////////////////////////////////////////////
// Object for volumes with custom game functionality
//////////////////////////////////////////////////////////////////////////
struct IGameVolumesEdit;

class SANDBOX_API CGameShapeObject
    : public CShapeObject
{
    Q_OBJECT
public:
    CGameShapeObject();
    static const GUID& GetClassID()
    {
        // {939AB121-51C1-4B00-9470-B840A181627D}
        static const GUID guid = {
            0x939ab121, 0x51c1, 0x4b00, { 0x94, 0x70, 0xb8, 0x40, 0xa1, 0x81, 0x62, 0x7d }
        };
        return guid;
    }

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void InitVariables();
    virtual bool CreateGameObject();
    virtual void UpdateGameArea(bool bRemove /* =false */);
    virtual void SetMaterial(CMaterial* mtl);
    virtual void InvalidateTM(int nWhyFlags);
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);
    virtual void SetMaterialLayersMask(uint32 nLayersMask);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    virtual void DeleteEntity();
    virtual void SpawnEntity();
    virtual void CalcBBox();

protected:
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    virtual int GetMaxPoints() const;
    virtual int GetMinPoints() const;

    virtual void OnPropertyChangeDone(IVariable* var);

    void  SetCustomMinAndMaxPoints();

    void OnClassChanged(IVariable* var);

    void ReloadGameObject(const QString& newClass);
    void UpdateGameShape();

    void RefreshVariables();

    IGameVolumesEdit* GetGameVolumesEdit() const;

    bool IsShapeOnly() const;
    bool GetIsClosedShape(bool& isClosed) const;
    bool NeedExportToGame() const;

    void NotifyPropertyChange();

private:

    CVariableEnum<int> m_shapeClasses;
    int                m_minPoints;
    int                m_maxPoints;
};

class CGameShapeLedgeObject
    : public CGameShapeObject
{
    Q_OBJECT
public:
    CGameShapeLedgeObject();
    static const GUID& GetClassID()
    {
        // {1C6118E2-A8C6-4AE6-8EB5-7E4F4C2B6BF3}
        static const GUID guid = {
            0x1c6118e2, 0xa8c6, 0x4ae6, { 0x8e, 0xb5, 0x7e, 0x4f, 0x4c, 0x2b, 0x6b, 0xf3 }
        };
        return guid;
    }

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void InitVariables();
    virtual void UpdateGameArea(bool bRemove);
    virtual void SetPoint(int index, const Vec3& pos);
};

class CGameShapeLedgeStaticObject
    : public CGameShapeLedgeObject
{
    Q_OBJECT
public:
    CGameShapeLedgeStaticObject();
    static const GUID& GetClassID()
    {
        // {48272E81-61F3-4D51-8743-54F4C516FD95}
        static const GUID guid = {
            0x48272e81, 0x61f3, 0x4d51, { 0x87, 0x43, 0x54, 0xf4, 0xc5, 0x16, 0xfd, 0x95 }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
// Object for Navigation Surfaces and Volumes
//////////////////////////////////////////////////////////////////////////
class CNavigationAreaObject
    : public CAIShapeObjectBase
    , public INavigationSystem::INavigationSystemListener
{
    Q_OBJECT
public:
    CNavigationAreaObject();
    virtual ~CNavigationAreaObject();

    static const GUID& GetClassID()
    {
        // {EF05A2F1-BE56-4c09-82D4-C63B40DB823C}
        static const GUID guid =
        {
            0xef05a2f1, 0xbe56, 0x4c09, { 0x82, 0xd4, 0xc6, 0x3b, 0x40, 0xdb, 0x82, 0x3c }
        };
        return guid;
    }

    virtual void Serialize(CObjectArchive& ar);

    void BeginEditParams(IEditor* pEditor, int flags);
    virtual void Display(DisplayContext& dc);
    virtual void PostLoad(CObjectArchive& ar);
    virtual void Done();
    // Override init variables to add our own, and not put irrelevant ones on the rollup bar
    virtual void InitVariables();
    // Override game area creation.
    virtual void UpdateGameArea(bool bRemove = false);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return CEntityObject::Export(levelPath, xmlNode); }
    virtual void SetName(const QString& name);

    virtual void RemovePoint(int index);
    virtual int InsertPoint(int index, const Vec3& point, bool const bModifying);
    virtual void SetPoint(int index, const Vec3& pos);

    //////////////////////////////////////////////////////////////////////////
    virtual void ChangeColor(COLORREF color);

    virtual void OnShapeTypeChange(IVariable* var);
    virtual void OnShapeAgentTypeChange(IVariable* var);

    virtual void OnNavigationEvent(const INavigationSystem::ENavigationEvent event);

    void UpdateMeshes();
    void ApplyExclusion(bool apply);
    void RelinkWithMesh(bool bUpdateGameArea = true);
    void CreateVolume(Vec3* points, size_t pointsSize, NavigationVolumeID requestedID = NavigationVolumeID(0));
    void DestroyVolume();

protected:
    CVariable<bool> mv_exclusion;
    CVariableArray mv_agentTypes;

    std::vector<CVariable<bool> > mv_agentTypeVars;
    std::vector<NavigationMeshID> m_meshes;
    NavigationVolumeID m_volume;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SHAPEOBJECT_H
