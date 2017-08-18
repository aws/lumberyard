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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVIGATION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVIGATION_H
#pragma once


#include "INavigation.h"
#include "IAISystem.h"
#include "Graph.h"
#include "CAISystem.h"
#include "CTriangulator.h"
#include "VertexList.h"
#include "BuildingIDManager.h"
#include "ShapeContainer.h"
#include "TriangularNavRegion.h"


struct SpecialArea
{
    // (MATT) Note that this must correspond exactly to the enum in CryAISystem's Navigation.h {2009/06/17}
    enum EType
    {
        TYPE_WAYPOINT_HUMAN,
        TYPE_VOLUME,
        TYPE_FLIGHT,
        TYPE_WATER,
        TYPE_WAYPOINT_3DSURFACE,
        TYPE_FREE_2D,
        TYPE_TRIANGULATION,
    };

    void SetPolygon(const ListPositions& polygon) {lstPolygon = polygon; CalcAABB(); }
    const ListPositions& GetPolygon() const {return lstPolygon; }

    const AABB& GetAABB() const {return aabb; }

    float fMinZ, fMaxZ;
    float fHeight;
    float fNodeAutoConnectDistance;
    int m_iWaypointType;
    int bVehiclesInHumanNav;
    int nBuildingID;
    SpecialArea::EType type;
    bool bCalculate3DNav;
    float f3DNavVolumeRadius;
    EAILightLevel lightLevel;
    float flyAgentWidth, flyAgentHeight;

    struct FlightNavData
    {
        float flyAgentWidth;
        float flyAgentHeight;
        float voxelOffsetX;
        float voxelOffsetY;

        FlightNavData()
            : flyAgentWidth(0.0f)
            , flyAgentHeight(0.0f)
            , voxelOffsetX(0.0f)
            , voxelOffsetY(0.0f)
        {}
    };


    FlightNavData flightNavData;

    EWaypointConnections    waypointConnections;
    bool bAltered;      // making links unpassable
    bool bCritterOnly;

    std::vector<string> vPFProperties;

    SpecialArea()
        : nBuildingID(-1)
        , type(SpecialArea::TYPE_WAYPOINT_HUMAN)
        , bCalculate3DNav(true)
        , f3DNavVolumeRadius(10.0f)
        , waypointConnections(WPCON_DESIGNER_NONE)
        , bVehiclesInHumanNav(false)
        , bAltered(false)
        , bCritterOnly(false)
        , lightLevel(AILL_NONE)
        , flyAgentWidth(0.0f)
        , flyAgentHeight(0.0f)
    {
        fMinZ = 9999.f;
        fMaxZ = -9999.f;
        fHeight = 0;
        aabb.Reset();
    }

private:
    ListPositions   lstPolygon;
    AABB aabb;

    void CalcAABB()
    {
        aabb.Reset();
        for (ListPositions::const_iterator it = lstPolygon.begin(); it != lstPolygon.end(); ++it)
        {
            aabb.Add(*it);
        }
    }
};

typedef std::map<string, SpecialArea> SpecialAreaMap;
// used for faster GetSpecialArea() lookup in game mode
typedef std::vector<SpecialArea*> SpecialAreaVector;

struct SExtraLinkCostShape
{
    SExtraLinkCostShape(const ListPositions& shape, const AABB& aabb, float costFactor)
        : shape(shape)
        , aabb(aabb)
        , costFactor(costFactor)
        , origCostFactor(costFactor) {}
    SExtraLinkCostShape(const ListPositions& shape, float costFactor)
        : shape(shape)
        , costFactor(costFactor)
        , origCostFactor(costFactor)
    {
        aabb.Reset();
        for (ListPositions::const_iterator it = shape.begin(); it != shape.end(); ++it)
        {
            aabb.Add(*it);
        }
    }
    ListPositions shape;
    AABB aabb;
    // the cost factor can get modified at run-time - it will get reset at the same time
    // as graph links get reset etc
    float costFactor;
    float origCostFactor;
};

typedef std::map<string, SExtraLinkCostShape> ExtraLinkCostShapeMap;


struct CutEdgeIdx
{
    int idx1;
    int idx2;

    CutEdgeIdx(int i1, int i2)
    {
        idx1 = i1;
        idx2 = i2;
    }

    // default ctor to allow std::vector::resize(0)
    CutEdgeIdx() {}
};

typedef std::vector<CutEdgeIdx> NewCutsVector;

class CCryFile;
class CAIShape;

class CNavRegion;
class CVolumeNavRegion;
class CWaypointHumanNavRegion;
class CWaypoint3DSurfaceNavRegion;
class CFlightNavRegion;
class CRoadNavRegion;
class CFree2DNavRegion;
class CSmartObjectNavRegion;

class CNavigation
{
public:
    explicit CNavigation(ISystem* pSystem);
    ~CNavigation();

    const SpecialAreaMap& GetSpecialAreas() const {return m_mapSpecialAreas; }
    const SpecialArea* GetSpecialArea(const Vec3& pos, SpecialArea::EType areaType);
    // Gets the special area with a particular building ID - may return 0 if it cannot be found
    const SpecialArea* GetSpecialArea(int buildingID) const;

    typedef std::vector< std::pair<string, const SpecialArea*> > VolumeRegions;
    /// Fills in the container of special areas that relate to 3D volumes
    void GetVolumeRegions(VolumeRegions& volumeRegions);
    const ShapeMap& GetDesignerPaths() const { return m_mapDesignerPaths; }

    // copies a designer path into provided list if a path of such name is found
    bool GetDesignerPath(const char* szName, SShape& path) const;

    bool Init();
    void Reset(IAISystem::EResetReason reason);
    void ShutDown();
    void FlushSystemNavigation();
    // Gets called after loading the mission
    void OnMissionLoaded();
    void ValidateNavigation();

    void Serialize(TSerialize ser);

    // writes areas into file
    void WritePolygonArea(CCryFile& file, const string& name, const ListPositions& pts);
    void WriteAreasIntoFile(const char* fileName);

    // parses ai information into file
    // FIXME Mrz 13, 2008: <pvl> come up with a better name - I don't think this
    // function parses anything into a file anymore
    void ParseIntoFile(CGraph* pGraph, bool bForbidden = false);

    // Prompt the exporting of data from AI - these should require not-too-much processing - the volume
    // stuff requires more processing so isn't done for a normal export
    void ExportData(const char* navFileName, const char* areasFileName, const char* roadsFileName,
        const char* vertsFileName, const char* volumeFileName,
        const char* flightFileName);

    enum ENavDataState
    {
        NDS_UNSET, NDS_OK, NDS_BAD
    };
    ENavDataState GetNavDataState () const { return m_navDataState; }

    void FlushAllAreas();
    void FlushSpecialAreas();
    /// Check all the forbidden areas are sensible size etc
    bool ValidateAreas();
    bool ValidateBigObstacles();

    bool ForbiddenAreasOverlap();
    bool ForbiddenAreaOverlap(const CAIShape* pShape);
    void EraseSpecialArea(const string& name);
    /// rebuilding the QuadTrees will not be super-quick so they simply get
    /// cleared when they are invalidated.
    void ClearForbiddenQuadTrees();
    /// QuadTrees will be built on (a) triangulation (b) loading a level
    void RebuildForbiddenQuadTrees();
    /// This is just for debugging
    const char* GetNavigationShapeName(int nBuildingID) const;
    /// Checks if navigation shape exists - called by editor
    bool DoesNavigationShapeExists(const char* szName, EnumAreaType areaType, bool road = false);
    /// Creates a shape object - called by editor
    /// Returns true if a shape is completely inside forbidden regions. Pass in the AABB for the shape
    template<typename VecContainer>
    bool IsShapeCompletelyInForbiddenRegion(VecContainer& shape, bool checkAutoGenRegions) const;
    bool CreateNavigationShape(const SNavigationShapeParams& params);
    /// Deletes designer created path/shape - called by editor
    void DeleteNavigationShape(const char* szName);

    /// generate the triangulation for this level and mission
    void GenerateTriangulation(const char* szLevel, const char* szMission);
    /// Find points on the land/water boundary and add them to the triangulator
    void AddBeachPointsToTriangulator(const AABB& worldAABB);

    /// Returns true if a point is in a forbidden region. When two forbidden regions
    /// are nested then it is just the region between them that is forbidden. This
    /// only checks the forbidden areas, not the boundaries.
    bool IsPointInForbiddenRegion(const Vec3& pos, bool checkAutoGenRegions = true) const;
    /// Optionally returns the forbidden region the point is in (if it is in a forbidden region)
    bool IsPointInForbiddenRegion(const Vec3& pos, const CAIShape** ppShape, bool checkAutoGenRegions) const;  // internal use
    /// Returns true if the point is inside the nav modifiers marked out as containing water
    bool IsPointInWaterAreas(const Vec3& pt) const;
    /// Indicates if a point is in a special area, checking the height too
    static bool IsPointInSpecialArea(const Vec3& pos, const SpecialArea& sa);
    /// returns true if pos is inside a TRIANGULATION nav modifier, or if there are no
    /// such modifiers.
    /// NOTE this must only be called in editor (since it shouldn't be needed in game) - will assert that
    /// this is the case!
    bool IsPointInTriangulationAreas(const Vec3& pos);
    /// if there's intersection vClosestPoint indicates the intersection point, and the edge normal
    /// is optionally returned. If bForceNormalOutwards is set then in the case of forbidden
    /// boundaries this normal is chosen to point (partly) towards vStart.
    /// nameToSkip can optionally point to a string indicating a forbidden area area to not check
    /// mode indicates if areas and/or boundaries should be checked
    bool IntersectsForbidden(const Vec3& vStart, const Vec3& vEnd, Vec3& vClosestPoint, const string* nameToSkip = 0, Vec3* pNormal = NULL,
        INavigation::EIFMode mode = INavigation::IF_AREASBOUNDARIES, bool bForceNormalOutwards = false) const;
    bool IntersectsSpecialArea(const Vec3& vStart, const Vec3& vEnd, Vec3& vClosestPoint);
    /// Returns true if a point is on/close to a forbidden boundary/area edge
    bool IsPointOnForbiddenEdge(const Vec3& pos, float tol = 0.0001f, Vec3* pNormal = 0, const CAIShape** ppPolygon = 0, bool checkAutoGenRegions = true);
    /// Returns true if a point is within a forbidden boundary. This
    /// only checks the forbidden boundaries, not the areas.
    bool IsPointInForbiddenBoundary(const Vec3& pos, const CAIShape** ppShape = 0) const;
    /// Returns true if it is impossible (assuming path finding is OK) to get from start
    /// to end without crossing a forbidden boundary (except for moving out of a
    /// forbidden region).
    bool IsPathForbidden(const Vec3& start, const Vec3& end);
    /// Returns true if a point is inside forbidden boundary/area edge or close to its edge
    bool IsPointForbidden(const Vec3& pos, float tol, Vec3* pNormal = 0, const CAIShape** ppShape = 0);

    bool GetBuildingInfo(int nBuildingID, IAISystem::SBuildingInfo& info);
    bool IsPointInBuilding(const Vec3& pos, int nBuildingID);

    /// Returns the extra link cost associated with the link by intersecting against the appropriate
    /// shapes. 0.0 means no shapes were found.
    float GetExtraLinkCost(const Vec3& pos1, const Vec3& pos2) const;
    /// Internal helper - returns the extra cost associated with traversing a single shape. Initial AABB is
    /// assumed to have already been done and passed
    float GetExtraLinkCost(const Vec3& pos1, const Vec3& pos2, const AABB& linkAABB, const SExtraLinkCostShape& shape) const;

    void SetUseAutoNavigation(const char* szPathName, EWaypointConnections waypointConnections);


    //====================================================================
    //  Functions for graph creation/loading etc
    //====================================================================
    //  GraphNode* FindMarkNodeBy2Vertex(CGraphLinkManager& linkManager, int vIdx1, int vIdx2, GraphNode* exclude, ListNodes &lstNewNodes);
    /// make and tags a list of nodes that intersect the segment from start to end.
    void    CreatePossibleCutList(const Vec3& vStart, const Vec3& vEnd, ListNodeIds& lstNodes);
    void RefineTriangle(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, unsigned nodeIndex, const Vec3r& start, const Vec3r& end, ListNodeIds& lstNewNodes, ListNodeIds& lstOldNodes, NewCutsVector& newCutsVector);
    bool TriangleLineIntersection(GraphNode* pNode, const Vec3& vStart, const Vec3& vEnd);
    bool SegmentInTriangle(GraphNode* pNode, const Vec3& vStart, const Vec3& vEnd);
    void MarkForbiddenTriangles();
    void DisableThinNodesNearForbidden();
    void CalculateLinkExposure(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link);
    void CalculateLinkWater(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link);
    void CalculateLinkRadius(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link);
    void CalculateNoncollidableLinks();
    float CalculateDistanceToCollidable(const Vec3& pos, float maxDist) const;
    void CreateNewTriangle(const ObstacleData& od1, const ObstacleData& od2, const ObstacleData& od3, bool tag, ListNodeIds& lstNewNodes);
    void AddForbiddenArea(CAIShape* pShape);
    /// Combines designer and code forbidden areas - returns false if there's a problem
    bool CalculateForbiddenAreas();
    /// combines overlapping areas. Returns false if there's a problem
    bool CombineForbiddenAreas(CAIShapeContainer& areas);
    CVertexList& GetVertexList () { return m_VertexList; }

    void AddForbiddenAreas();
    void CalculateLinkProperties();

    /// Returns the base-class nav region - needs the graph only if type is waypoint (so you can
    /// pass in 0 if you know it's not... but be careful!)
    CNavRegion* GetNavRegion(IAISystem::ENavigationType type, const CGraph* pGraph);

    /// Triangular nav region may be associated with more than one graph
    CTriangularNavRegion* GetTriangularNavRegion() {return m_pTriangularNavRegion; }
    CWaypointHumanNavRegion* GetWaypointHumanNavRegion() {return m_pWaypointHumanNavRegion; }
    CFlightNavRegion* GetFlightNavRegion() {return m_pFlightNavRegion; }
    CVolumeNavRegion* GetVolumeNavRegion() {return m_pVolumeNavRegion; }
    CRoadNavRegion* GetRoadNavRegion() {return m_pRoadNavRegion; }
    CSmartObjectNavRegion* GetSmartObjectsNavRegion() {return m_pSmartObjectNavRegion; }

    /// generate 3d nav volumes for this level and mission
    void Generate3DVolumes(const char* szLevel, const char* szMission);
    const std::vector<float>& Get3DPassRadii() const {return m_3DPassRadii; }
    /// Generates voxels for 3D nav regions and stores them (can cause memory problems) for subsequent
    /// debug viewing using ai_debugdrawvolumevoxels
    void Generate3DDebugVoxels();

    /// generate flight navigation for this level and mission
    void GenerateFlightNavigation(const char* szLevel, const char* szMission);
    Vec3 IsFlightSpaceVoid(const Vec3& vPos, const Vec3& vFwd, const Vec3& vWng, const Vec3& vUp);

    /// The AI system will automatically reconnect nodes in this building ID
    /// and recreate area/vertex information (only affects automatically
    /// generated links)
    void ReconnectWaypointNodesInBuilding(int nBuildingID);
    /// Automatically reconnect all waypoint nodes in all buildings,
    /// and disconnects nodes not in buildings (only affects automatically
    /// generated links)
    void ReconnectAllWaypointNodes();

    CGraph* GetGraph () { return m_pGraph; }

    IAISystem::ENavigationType CheckNavigationType(const Vec3& pos, int& nBuildingID, IVisArea*& pArea, IAISystem::tNavCapMask navCapMask);

    /// Returns true if all the links leading out of the node have radius < fRadius
    bool ExitNodeImpossible(CGraphLinkManager& linkManager, const GraphNode* pNode, float fRadius) const;

    void GetMemoryStatistics(ICrySizer* pSizer);

    /// Meant for debug draws, there shouldn't be anything else to update here.
    void Update () const;

private:
    std::vector <const SpecialArea*> GetExclusionVolumes (const SpecialArea&, const string& agentType);

    /// Indicates if the navigation data is sufficiently valid after loading that
    /// we should continue
    ENavDataState m_navDataState;

    CTriangularNavRegion* m_pTriangularNavRegion;
    CWaypointHumanNavRegion* m_pWaypointHumanNavRegion;
    CVolumeNavRegion* m_pVolumeNavRegion;
    CFlightNavRegion* m_pFlightNavRegion;
    CRoadNavRegion* m_pRoadNavRegion;
    CSmartObjectNavRegion*      m_pSmartObjectNavRegion;

    /// used during 3D nav generation - the pass radii of the entities that will use the navigation
    std::vector<float> m_3DPassRadii;

    ShapeMap m_mapDesignerPaths;

    CAIShapeContainer m_forbiddenAreas;
    CAIShapeContainer m_designerForbiddenAreas;
    CAIShapeContainer m_forbiddenBoundaries;

    ExtraLinkCostShapeMap m_mapExtraLinkCosts;

    SpecialAreaMap  m_mapSpecialAreas;  // define where to disable automatic AI processing.
    SpecialAreaVector m_vectorSpecialAreas; // to speed up GetSpecialArea() in game mode

    // NOTE Mrz 14, 2008: <pvl> the following 3 data structures live in AISystem
    // on the run-time side - in the editor we want to have them here though, we
    // need to support adding/removing and loading/saving them.
    PerceptionModifierShapeMap m_mapPerceptionModifiers;
    ShapeMap m_mapOcclusionPlanes;
    ShapeMap m_mapGenericShapes;


    unsigned int m_nNumBuildings;
    CBuildingIDManager  m_BuildingIDManager;

    CGraph* m_pGraph;

    CVertexList m_VertexList;

    struct SValidationErrorMarker
    {
        SValidationErrorMarker(const string& msg, const Vec3& pos, const OBB& obb, ColorB col)
            : msg(msg)
            , pos(pos)
            , obb(obb)
            , col(col) {}
        SValidationErrorMarker(const string& msg, const Vec3& pos, ColorB col)
            : msg(msg)
            , pos(pos)
            , col(col)
        {
            obb.SetOBBfromAABB(Matrix33(IDENTITY), AABB(Vec3 (-0.1f, -0.1f, -0.1f), Vec3 (0.1f, 0.1f, 0.1f)));
        }
        Vec3 pos;
        OBB obb;
        string msg;
        ColorB col;
    };

    // NOTE Jun 9, 2007: <pvl> needs to access m_validationErrorMarkers
    friend bool DoesShapeSelfIntersect(const ShapePointContainer&, Vec3&, std::vector<SValidationErrorMarker>&);

    std::vector<SValidationErrorMarker> m_validationErrorMarkers;

    ISystem* m_pSystem;

    ICVar* m_cvBigBrushLimitSize;   // to be used for finding big objects not enclosed into forbidden areas
    ICVar* m_cvIncludeNonColEntitiesInNavigation;
    ICVar* m_cvRadiusForAutoForbidden;
    ICVar* m_cvShowNavAreas;

    //====================================================================
    // Used during graph generation - should subsequently be empty
    //====================================================================
    CTriangulator* m_pTriangulator;
    std::vector<Tri*>   m_vTriangles;
    VARRAY  m_vVertices;
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVIGATION_H
