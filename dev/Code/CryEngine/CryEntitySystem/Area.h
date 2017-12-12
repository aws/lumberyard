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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_AREA_H
#define CRYINCLUDE_CRYENTITYSYSTEM_AREA_H
#pragma once

#include "AreaManager.h"
#include "EntitySystem.h"
#include "Components/IComponentArea.h"

#define INVALID_AREA_GROUP_ID -1

// Position type of a position compared relative to the given area
enum EAreaPosType
{
    AREA_POS_TYPE_2DINSIDE_ZABOVE,   // Above an area
    AREA_POS_TYPE_2DINSIDE_ZINSIDE,  // Inside an area
    AREA_POS_TYPE_2DINSIDE_ZBELOW,   // Below an area
    AREA_POS_TYPE_2DOUTSIDE_ZABOVE,  // Next to an area but above it
    AREA_POS_TYPE_2DOUTSIDE_ZINSIDE, // Next to an area but inside its min and max Z
    AREA_POS_TYPE_2DOUTSIDE_ZBELOW,  // Next to an area but below it

    AREA_POS_TYPE_COUNT
};

enum ECachedAreaData
{
    eCachedAreaData_None                           = 0x00000000,
    eCachedAreaData_PosTypeValid                   = BIT(0),
    eCachedAreaData_PosOnHullValid                 = BIT(1),
    eCachedAreaData_DistWithinSqValid              = BIT(2),
    eCachedAreaData_DistWithinSqValidNotObstructed = BIT(3),
    eCachedAreaData_DistNearSqValid                = BIT(4),
    eCachedAreaData_DistNearSqValidNotObstructed   = BIT(5),
    eCachedAreaData_DistNearSqValidHeightIgnored   = BIT(6),
    eCachedAreaData_PointWithinValid               = BIT(7),
    eCachedAreaData_PointWithinValidHeightIgnored  = BIT(8),
};

class CAreaSolid;

class CArea
    : public IArea
{
public:

    struct a2DPoint
    {
        float x, y;
        a2DPoint(void)
            : x(0.0f)
            , y(0.0f) { }
        a2DPoint(const Vec3& pos3D) { x = pos3D.x; y = pos3D.y; }
        a2DPoint(float _x, float _y)
            : x(_x)
            , y(_y){}
        float   DistSqr(const struct a2DPoint& point) const
        {
            float xx = x - point.x;
            float yy = y - point.y;
            return (xx * xx + yy * yy);
        }
        float   DistSqr(const float px, const float py) const
        {
            float xx = x - px;
            float yy = y - py;
            return (xx * xx + yy * yy);
        }
    };

    struct a2DBBox
    {
        a2DPoint    min;    // 2D BBox min
        a2DPoint    max;    // 2D BBox max
        bool    PointOutBBox2D (const a2DPoint& point) const
        {
            return (point.x < min.x || point.x > max.x || point.y < min.y || point.y > max.y);
        }
        bool    PointOutBBox2DVertical (const a2DPoint& point) const
        {
            return (point.y <= min.y || point.y > max.y || point.x > max.x);
        }
        bool    BBoxOutBBox2D (const a2DBBox& box) const
        {
            return (box.max.x < min.x || box.min.x > max.x || box.max.y < min.y || box.min.y > max.y);
        }
        ILINE bool Overlaps2D(const AABB& bb) const
        {
            if  (min.x > bb.max.x)
            {
                return false;
            }
            if  (min.y > bb.max.y)
            {
                return false;
            }
            if  (max.x < bb.min.x)
            {
                return false;
            }
            if  (max.y < bb.min.y)
            {
                return false;
            }
            return true; //the aabb's overlap
        }
    };

    struct a2DSegment
    {
        a2DSegment()
            :   isHorizontal(false)
            , bObstructSound(false)
            , k(0.0f)
            , b(0.0f){}

        bool        bObstructSound; // Does it obstruct sounds?
        bool        isHorizontal;       //horizontal flag
        float       k, b;                       //line parameters y=kx+b
        a2DBBox bbox;                       // segment's BBox

        bool IntersectsXPos(const a2DPoint& point) const
        {
            return (point.x < (point.y - b) / k);
        }

        float GetIntersectX(const a2DPoint& point) const
        {
            return (point.y - b) / k;
        }

        bool IntersectsXPosVertical(const a2DPoint& point) const
        {
            if (k == 0.0f)
            {
                return (point.x <= b);
            }
            return false;
        }

        a2DPoint const GetStart() const
        {
            return a2DPoint(bbox.min.x, (float)fsel(k, bbox.min.y, bbox.max.y));
        }

        a2DPoint const GetEnd() const
        {
            return a2DPoint(bbox.max.x, (float)fsel(k, bbox.max.y, bbox.min.y));
        }
    };

    struct SBoxHolder
    {
        a2DBBox box;
        CArea* area;
    };

    typedef std::vector<SBoxHolder> TAreaBoxes;

    static const TAreaBoxes& GetBoxHolders();

    //////////////////////////////////////////////////////////////////////////
    CArea(CAreaManager* pManager);

    //IArea
    virtual size_t GetEntityAmount() const { return m_vEntityID.size(); }
    virtual const EntityId GetEntityByIdx(int index) const { return m_vEntityID[index]; }
    virtual void GetMinMax(Vec3** min, Vec3** max) const
    {
        (*min)->x = m_areaBBox.min.x;
        (*min)->y = m_areaBBox.min.y;
        (*min)->z = m_VOrigin;
        (*max)->x = m_areaBBox.max.x;
        (*max)->y = m_areaBBox.max.y;
        (*max)->z = m_VOrigin + m_VSize;
    }
    //~IArea


    // Releases area.
    void Release();

    void    SetID(const int id) { m_AreaID = id; }
    int     GetID() const { return m_AreaID; }

    void     SetEntityID(const EntityId id) { m_EntityID = id; }
    EntityId GetEntityID() const { return m_EntityID; }

    void    SetGroup(const int id) { m_AreaGroupID = id; }
    int     GetGroup() const { return m_AreaGroupID; }

    void SetPriority (const int nPriority) { m_nPriority = nPriority; }
    int  GetPriority () const { return m_nPriority; }

    // Description:
    //    Sets sound obstruction depending on area type
    void    SetSoundObstructionOnAreaFace(int unsigned const nFaceIndex, bool const bObstructs);

    void SetAreaType(EEntityAreaType type);
    EEntityAreaType GetAreaType() const { return m_AreaType; }

    //////////////////////////////////////////////////////////////////////////
    // These functions also switch area type.
    //////////////////////////////////////////////////////////////////////////
    void    SetPoints(const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, const int nPointsCount);
    void    SetBox(const Vec3& min, const Vec3& max, const Matrix34& tm);
    void    SetSphere(const Vec3& vCenter, float fRadius);
    void    BeginSettingSolid(const Matrix34& worldTM);
    void    AddConvexHullToSolid(const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices);
    void    EndSettingSolid();
    //////////////////////////////////////////////////////////////////////////

    void SetMatrix(const Matrix34& tm);
    void GetMatrix(Matrix34& tm) const;

    void GetBox(Vec3& min, Vec3& max) const { min = m_BoxMin; max = m_BoxMax; }
    void GetWorldBox(Vec3& rMin, Vec3& rMax) const;
    void GetSphere(Vec3& vCenter, float& fRadius) const { vCenter = m_SphereCenter; fRadius = m_SphereRadius; }

    void  SetHeight(float fHeight) { m_VSize = fHeight; }
    float GetHeight() const { return m_VSize; }

    void    SetFade(const float fFade) { m_PrevFade = fFade; }
    float GetFade() const { return m_PrevFade; }

    void    AddEntity(const EntityId entId);
    void AddEntity(const EntityGUID entGuid);
    void    AddEntites(const std::vector<EntityId>& entIDs);
    const std::vector<EntityId>* GetEntities() const { return &m_vEntityID; }
    const std::vector<EntityGUID>* GetEntitiesGuid() const { return &m_vEntityGuid; }

    void    ClearEntities();
    void ResolveEntityIds();
    void    ReleaseCachedAreaData();

    void    SetProximity(float prx) { m_fProximity = prx; }
    float   GetProximity() { return m_fProximity; }

    float GetGreatestFadeDistance();
    float GetGreatestEnvironmentFadeDistance();

    // Invalidations
    void    InvalidateCachedAreaData(EntityId const nEntityID);

    // Calculations
    //  Methods can have set the bCacheResult parameter which indicates, if set to true,
    //  that data will be cached and available through the GetCached... methods.
    //  If bCacheResult is set to false no data is cached and just the result is returned,
    //  this allows for calculating positions or distances without trashing previously cached results.
    EAreaPosType    CalcPosType(EntityId const nEntityID, Vec3 const& rPos, bool const bCacheResult = true);
    float           CalcPointWithinDist(EntityId const nEntityID, Vec3 const& point3d, bool const bIgnoreSoundObstruction = true, bool const bCacheResult = true);
    bool            CalcPointWithin(EntityId const nEntityID, Vec3 const& point3d, bool const bIgnoreHeight = false, bool const bCacheResult = true);
    float           CalcDistToPoint(a2DPoint const& point) const;

    // Squared-distance returned works only if point32 is not within the area
    float           CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d, bool const bIgnoreSoundObstruction = true, bool const bCacheResult = true);
    float           CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, bool const bIgnoreHeight = false, bool const bIgnoreSoundObstruction = true, bool const bCacheResult = true);
    float           ClosestPointOnHullDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d, bool const bIgnoreSoundObstruction = true, bool const bCacheResult = true);

    // Get cached data
    float           GetCachedPointWithinDistSq(EntityId const nEntityID) const;
    bool            GetCachedPointWithin(EntityId const nEntityID) const;
    EAreaPosType    GetCachedPointPosTypeWithin(EntityId const nEntityID) const;
    float           GetCachedPointNearDistSq(EntityId const nEntityID) const;
    Vec3 const&     GetCachedPointOnHull(EntityId const nEntityID) const;

    // Events
    void AddCachedEvent(const SEntityEvent& pNewEvent);
    void ClearCachedEventsFor(EntityId const nEntityID);
    void ClearCachedEvents();
    void SendCachedEventsFor(EntityId const nEntityID);

    void SendEvent(SEntityEvent& newEvent, bool bClearCachedEvents = true);

    // Inside
    void    EnterArea(IEntity const* const __restrict pEntity);
    void    LeaveArea(IEntity const* const __restrict pEntity);
    void    UpdateArea(const Vec3& vPos, IEntity const* const pEntity);
    void    UpdateAreaInside(IEntity const* const __restrict pEntity);
    void    ExclusiveUpdateAreaInside(IEntity const* const __restrict pEntity, EntityId const AreaHighID, float const fadeValue, float const environmentFadeValue);
    void    ExclusiveUpdateAreaNear(IEntity const* const __restrict pEntity, EntityId const AreaHighID, float const fadeValue);
    float   CalculateFade(const Vec3& pos3D);
    void    ProceedFade(IEntity const* const __restrict pEntity, float const fadeValue);
    void    OnAreaCrossing(IEntity const* const __restrict pEntity);

    // Near
    void    EnterNearArea(IEntity const* const __restrict pEntity);
    void    LeaveNearArea(IEntity const* const __restrict pEntity);

    // Far
    void    OnAddedToAreaCache(IEntity const* const pEntity);
    void    OnRemovedFromAreaCache(IEntity const* const pEntity);

    void    Draw(size_t const idx);

    unsigned MemStat();

    void    SetActive(bool bActive) { m_bIsActive = bActive; }
    bool    IsActive() const { return m_bIsActive; }
    bool    HasSoundAttached();

    void    GetBBox(Vec2& vMin, Vec2& vMax) const;
    void    GetSolidBoundBox(AABB& outBoundBox) const;

#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
    size_t GetCacheEntityCount() const {return m_mapEntityCachedAreaData.size(); }
    char const* const GetAreaEntityName() const;
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:

    struct SCachedAreaData
    {
        SCachedAreaData()
            : eFlags(eCachedAreaData_None)
            , ePosType(AREA_POS_TYPE_COUNT)
            , oPos(ZERO)
            , fDistanceWithinSq(FLT_MAX)
            , fDistanceNearSq(FLT_MAX)
            , bPointWithin(false){}

        ECachedAreaData eFlags;
        EAreaPosType        ePosType;
        Vec3                        oPos;
        float                       fDistanceWithinSq;
        float                       fDistanceNearSq;
        bool                        bPointWithin;
    };

    ~CArea();

    void    AddSegment(const a2DPoint& p0, const a2DPoint& p1, bool const nObstructSound);
    void    CalcBBox();
    const a2DBBox& GetBBox() const;
    a2DBBox& GetBBox();
    void    ClearPoints();
    IEntitySystem* GetEntitySystem() { return m_pAreaManager->GetEntitySystem(); }
    void    CalcClosestPointToObstructedShape(EntityId const nEntityID, Vec3& rv3ClosestPos, float& rfClosestDistSq, Vec3 const& rv3SourcePos);
    void    CalcClosestPointToObstructedBox(Vec3& rv3ClosestPos, float& rfClosestDistSq, Vec3 const& rv3SourcePos) const;
    void    CalcClosestPointToSolid(Vec3 const& rv3SourcePos, bool bIgnoreSoundObstruction, float& rfClosestDistSq, Vec3* rv3ClosestPos) const;

    void    ReleaseAreaData();

private:

    CAreaManager* m_pAreaManager;
    float   m_fProximity;
    float m_fFadeDistance;
    float m_fEnvironmentFadeDistance;

    //  attached entities IDs list
    std::vector<EntityId> m_vEntityID;
    std::vector<EntityGUID> m_vEntityGuid;

    typedef std::vector<SEntityEvent> CachedEvents;
    CachedEvents m_cachedEvents;

    float   m_PrevFade;

    int             m_AreaID;
    int             m_AreaGroupID;
    EntityId    m_EntityID;
    int             m_nPriority;

    Matrix34    m_WorldTM;
    Matrix34    m_InvMatrix;

    EEntityAreaType m_AreaType;
    typedef VectorMap<EntityId, SCachedAreaData> TEntityCachedAreaDataMap;
    TEntityCachedAreaDataMap m_mapEntityCachedAreaData;
    Vec3 const m_oNULLVec;

    // for shape areas ----------------------------------------------------------------------
    // area's bbox
    a2DBBox m_areaBBox;
    size_t m_bbox_holder;
    // the area segments
    std::vector<a2DSegment*>    m_vpSegments;

    // for sector areas ----------------------------------------------------------------------
    //  int m_Building;
    //  int m_Sector;
    //  IVisArea *m_Sector;

    // for box areas ----------------------------------------------------------------------
    Vec3    m_BoxMin;
    Vec3    m_BoxMax;
    bool    m_bAllObstructed : 1;
    unsigned int m_nObstructedCount;

    struct SBoxSideSoundObstruction
    {
        bool bObstructed : 1;
    } m_abBoxSideObstruction[6];

    struct SBoxSide
    {
        SBoxSide(Vec3 const& rv3MinValues, Vec3 const& rv3MaxValues)
            :   v3MinValues(rv3MinValues)
            , v3MaxValues(rv3MaxValues){}
        Vec3 const v3MinValues;
        Vec3 const v3MaxValues;
    };

    // for sphere areas ----------------------------------------------------------------------
    Vec3 m_SphereCenter;
    float   m_SphereRadius;
    float   m_SphereRadius2;

    // for solid areas -----------------------------------------------------------------------
    _smart_ptr<CAreaSolid> m_AreaSolid;

    //  area vertical origin - the lowest point of area
    float   m_VOrigin;
    //  area height (vertical size). If (m_VSize<=0) - not used, only 2D check is done. Otherwise
    //  additional check for Z to be in [m_VOrigin, m_VOrigin + m_VSize] range is done
    float   m_VSize;

    bool        m_bIsActive : 1;
    bool        m_bInitialized : 1;
    bool        m_bHasSoundAttached : 1;
    bool        m_bAttachedSoundTested : 1; // can be replaced later with an OnAfterLoad
    bool        m_bObstructRoof : 1;
    bool        m_bObstructFloor : 1;
    bool        m_bEntityIdsResolved : 1;
};




#endif // CRYINCLUDE_CRYENTITYSYSTEM_AREA_H
