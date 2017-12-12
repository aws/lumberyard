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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_VOLUMENAVREGION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_VOLUMENAVREGION_H
#pragma once


#include "IAISystem.h"
#include "NavRegion.h"
#include "Graph.h"
#include "HashSpace.h"

#include "IPhysics.h"
#include <ICoverSystem.h>

#include <vector>
#include <list>
#include <map>
#include <deque>

struct IRenderer;
struct I3DEngine;
//struct IPhysicalEntity;
struct IPhysicalWorld;
struct IGeometry;
class CNavigation;

/// Helper for users of GetHideSpotsWithinRadius
struct SVolumeHideFunctor
{
    SVolumeHideFunctor(std::vector<SVolumeHideSpot>& hidePositions)
        : hidePositions(hidePositions) {}
    void operator()(SVolumeHideSpot& hs, float) {hidePositions.push_back(hs); }
    std::vector<SVolumeHideSpot>& hidePositions;
};



// Danny work in progress - enabling this works for basic navigation, but has problems if the destination
// lies inside an object (e.g. pathfinding to that object) or if the agent is carrying an object. Need
// physics API to change so we can skip entities.
//#define DYNAMIC_3D_NAVIGATION

/// The volume nav region works by loading/calculating individual navigation
/// regions, then stitching these regions together, either after loading
class CVolumeNavRegion
    : public CNavRegion
{
public:
    CVolumeNavRegion(CNavigation* pNavigation);
    ~CVolumeNavRegion(void);

    // inherited
    virtual void Clear();

    // inherited
    virtual unsigned GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "");

    /// This will write out the whole 3D navigation AND the navigation for the individual regions
    bool WriteToFile(const char* pName);
    /// This just reads in the whole 3D navigation
    bool ReadFromFile(const char* pName);

    /// Clears and generates 3D navigation based on the list of navigation modifiers that get obtained
    /// from CAISystem. Depending on the calculate flag in the nav mod then the regions will either be
    /// calculated or loaded from file.
    void Generate(const char* szLevel, const char* szMission);

    /// As IAISystem
    void Generate3DDebugVoxels();

    /// Calculate all hide spots
    void CalculateHideSpots();

    /// Gets the graph node from the index stored in the volume data (used during load/save)
    /// Returns 0 if out of bounds
    unsigned GetNodeFromIndex(int index);

    /// Returns true if a path segment capsule would intersect the world (and thus be
    /// a bad path. If intersectAtStart then is true then start represents the start
    /// sphere position. If it's false then it represents the very tip of the
    /// capsule. Similarly for intersectAtEnd.
    bool PathSegmentWorldIntersection(const Vec3& start,
        const Vec3& end,
        float radius,
        bool intersectAtStart,
        bool intersectAtEnd,
        EAICollisionEntities aiCollisionEntities) const;

    /// Returns true if the path, defined by a vector of points, would intersect the
    /// world, as in SweptSphereWorldIntersection
    bool DoesPathIntersectWorld(std::vector<Vec3>& path,
        float radius,
        EAICollisionEntities aiCollisionEntities);

    /// VOXEL_VALID means the voxel is in "good" space - i.e. where an alien might
    /// reside. INVALID means it's inside geometry. INDETERMINATE means we couldn't
    /// work out a result
    enum ECheckVoxelResult
    {
        VOXEL_VALID, VOXEL_INVALID
    };
    /// definitelyValid is set then it should be a location that is definitely valid.
    ECheckVoxelResult CheckVoxel(const Vec3& pos, const Vec3& definitelyValid) const;

    /// see GetNavigableSpacePoint
    void GetNavigableSpaceEntities(std::vector<const IEntity*>& entities);
    /// Finds the best navigable space point for the reference position and puts it in pos. Returns
    /// false if no navigable space entity is found. Entities should be obtained from
    /// GetNavigableSpaceEntities
    bool GetNavigableSpacePoint(const Vec3& refPos, Vec3& pos, const std::vector<const IEntity*>& entities);

    /// inherited
    virtual void Serialize(TSerialize ser);

    /// inherited
    virtual size_t MemStats();

    /// calls Functor::operator() with every hidespot within radius of pos
    template<typename Functor>
    void GetHideSpotsWithinRadius(const Vec3& pos, float radius, Functor& functor)
    {
        m_hideSpots.ProcessObjectsWithinRadius(pos, radius, functor);
    }

    // Returns estimated distance to geometry.
    float   GetDistanceToGeometry(size_t volumeIdx) const;

private:
    class CVolume;
    class CPortal;
    typedef   std::vector<CVolume*>   TVolumes;
    typedef   std::vector<CPortal>    TPortals;
    typedef   std::vector<int>        TPortalIndices;
    typedef std::vector<int>        TVectorInt;
    typedef short                   TVoxelValue;
    typedef unsigned char           TDistanceVoxelValue;
    typedef std::deque<TVoxelValue> TVoxels;
    typedef std::deque<TDistanceVoxelValue> TDistanceVoxels;

    class CVolume
    {
    public:
        const Vec3& Center(const CGraphNodeManager& nodeManager) const {return nodeManager.GetNode(m_graphNodeIndex)->GetPos(); }

        float m_radius;
        TPortalIndices m_portalIndices;
        unsigned m_graphNodeIndex;
        float          m_distanceToGeometry;///< approximate distance to the nearest geometry

        Vec3    GetConnectionPoint(const CVolumeNavRegion* VolumeNavRegion, const CVolume* otherVolume) const;
    };

    // link between volumes
    // it defines portal connecting two volumes
    class CPortal
    {
    public:
        CPortal(CVolume* one, CVolume* two)
            : m_pVolumeOne(one)
            , m_pVolumeTwo(two)
            , m_passRadius(100.0f){}
        bool    IsConnecting(const CVolume* pOne, const CVolume* pTwo) const;

        CVolume* m_pVolumeOne;
        CVolume* m_pVolumeTwo;
        Vec3        m_passPoint;
        float       m_passRadius;
    };

    // used during generation
    struct SVoxelData
    {
        Vec3    corner;///< bottom left corner
        TVoxels voxels;
        TDistanceVoxels distanceVoxels;
        float   size;
        Vec3i   dim;
    };

    struct SRegionData
    {
        /// These are derived immediately from the input data
        string        name;
        const struct SpecialArea*  sa;
        AABB          aabb;
        float                   volumeRadius;

        TVolumes            volumes;
        TPortals            portals;

        SVoxelData    voxelData;

        // Used when stitching - all the volumes at the edge
        TVolumes      edgeVolumes;

        std::vector<SVolumeHideSpot> hideSpots;
    };

    void  GeneratePortals(SRegionData& regionData);
    void  GeneratePortalsBetweenRegions(std::vector<SRegionData>& allRegionData);
    void  GeneratePortalVxl(TPortals& portals, CVolume& firstVolume, CVolume& secondVolume);
    /// Chooses a pass radius from radii for the portal and returns it.
    /// If no safe radius is found a -ve value is returned
    float CalculatePassRadius(CPortal& portal, const std::vector<float>& radii);

    bool Voxelize(SRegionData& regionData);
    bool DoesVoxelContainGeometry(SVoxelData& voxelData, int i, int j, int k, primitives::box& boxPrim);
    void UpdateVoxelisationNeighbours(SVoxelData& voxelData, int i, int j, int k, primitives::box& boxPrim);
    void VoxelizeInvert(SRegionData& regionData, const CVolume* pVolume, const Vec3i& ptIndex);
    bool FindSeedPoint(SVoxelData& voxelData, Vec3& point, Vec3i& ptIndex, int& voxelsLeft) const;

    TVoxelValue GetVoxelValue(const SVoxelData& voxelData, int xIdx, int yIdx, int zIdx) const;
    TDistanceVoxelValue GetDistanceVoxelValue(const SVoxelData& voxelData, int xIdx, int yIdx, int zIdx) const;
    void  SetVoxelValue(SVoxelData& voxelData, int xIdx, int yIdx, int zIdx, TVoxelValue value);
    bool  AdvanceVoxel(SVoxelData& voxelData, int xIdx, int yIdx, int zIdx, TVoxelValue threshold);
    int       CalculateVoxelDistances(SVoxelData& voxelData, TVoxelValue maxThreshold);
    Vec3    GetVoxelDistanceFieldGradient(SVoxelData& voxelData, int xIdx, int yIdx, int zIdx);

    Vec3  GetConnectionPoint(TVolumes& volumes, TPortals& portals, int volumeIdx1, int volumeIdx2) const;
    CVolume* GetVolume(TVolumes& volumes, int volumeIdx) const;
    int       GetVolumeIdx(const TVolumes& volumes, const CVolume* pVolume) const;
    CPortal* GetPortal(TPortals& portals, int portalIdx);
    const CPortal* GetPortal(const TPortals& portals, int portalIdx) const;
    int       GetPortalIdx(const TPortals& portals, const CPortal* pPortal) const;

    CVolume*  CreateVolume(TVolumes& volumes, const Vec3& pos, float radius, float distanceToGeometry);
    void  GenerateBasicVolumes(SRegionData& regionData);

    void CalculateHideSpots(SRegionData& regionData, std::vector<SVolumeHideSpot>& hideSpots);
    void CombineRegions(std::vector<SRegionData>& allRegions, bool adjustIndices);
    void StitchRegionsTogether(std::vector<SRegionData>& allRegions);

    void LoadRegion(const char* szLevel, const char* szMission, SRegionData& regionData);
    void SaveRegion(const char* szLevel, const char* szMission, SRegionData& regionData);
    void CalculateRegion(SRegionData& regionData);

    bool LoadNavigation(TVolumes& volumes, TPortals& portals, CCryFile& cryFile);
    bool SaveNavigation(TVolumes& volumes, TPortals& portals, CCryFile& cryFile);

    bool LoadPortal(TVolumes& volumes, TPortals& portals, CCryFile& cryFile);
    bool SavePortal(TVolumes& volumes, TPortals& portals, CPortal& portal, CCryFile& cryFile);

    bool LoadVolume(TVolumes& volumes, CCryFile& cryFile);
    bool SaveVolume(TVolumes& volumes, CVolume& volume, CCryFile& cryFile);

    /// These are just meant for the region hidespots since the main hidespots are
    /// stored in a hash space
    bool LoadHideSpots(std::vector<SVolumeHideSpot>& hidespots, CCryFile& cryFile);
    bool SaveHideSpots(const std::vector<SVolumeHideSpot>& hidespots, CCryFile& cryFile);

    //============ end of generation code ====================

    /// Sweeps a sphere from volume to pos and returns the sphere position at the intersection time
    Vec3 CalcClosestPointFromVolumeToPos(const CVolume& volume, const Vec3& pos, float passRadius) const;

    /// Checks if pos lies within vol. if extra > 0 then the test is made less strict by that amount
    /// in the radial direction.
    bool IsPointInVolume(const CVolume& vol, const Vec3& pos, float extra = 0.0f) const;

    bool AreAllLinksBlocked(unsigned nodeIndex, float passRadius);

    CNavigation* m_pNavigation;
    CGraph* m_pGraph;

    // here is the environment representation
    AABB            m_AABB;
    TVolumes        m_volumes;
    TPortals        m_portals;

    float m_maxVolumeRadius;

    CHashSpace<SVolumeHideSpot> m_hideSpots;

    /// portal index that was last checked for gravity streams
    unsigned m_lastGravityPortalIndex;

    /// Only gets populated during 3D generation if ai_DebugDrawVolumeVoxels is set
    /// Note - memory lock fails if it's a std::vector (std::vector of std::deque bad?)
    typedef std::list<SVoxelData> TDebugVoxels;
    TDebugVoxels m_debugVoxels;
};


#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_VOLUMENAVREGION_H

