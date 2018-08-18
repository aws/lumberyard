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

#include "Navigation.h"
#include "PolygonSetOps/Polygon2d.h"
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "VolumeNavRegion.h"
#include "FlightNavRegion.h"
#include "RoadNavRegion.h"
#include "SmartObjectNavRegion.h"

#include "TypeInfo_impl.h"
#include "AutoTypeStructs_info.h"
#include <INavigationSystem.h>

#include "Util/GeometryUtil.h"
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>


static const int maxForbiddenNameLen = 1024;

// flag used for debugging/profiling the improvement obtained from using a
// QuadTree for the forbidden shapes. Currently it's actually faster not
// using quadtree - need to experiment more
// kirill - enabling quadtree - is faster on low-spec
bool useForbiddenQuadTree = true;//false;
int debugForbiddenCounter = 0;

// counter used to ensure automatic forbidden names are unique
int forbiddenNameCounter = 0;

template <class F>
inline bool IsEquivalent2D(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, f32 epsilon = VEC_EPSILON)
{
    return fabs_tpl(v0.x - v1.x) <= epsilon && fabs_tpl(v0.y - v1.y) <= epsilon;
}

//====================================================================
// DoesRegionAContainRegionB
// assume that if all points from one region are in the other then
// that's sufficient - only true if regions are convex
//====================================================================
bool DoesRegionAContainRegionB(const CGraph::CRegion& regionA, const CGraph::CRegion& regionB)
{
    for (std::list<Vec3>::const_iterator it = regionB.m_outline.begin(); it != regionB.m_outline.end(); ++it)
    {
        if (!Overlap::Point_Polygon2D(*it, regionA.m_outline))
        {
            return false;
        }
    }
    return true;
}

//====================================================================
// CombineRegions
//====================================================================
static void CombineRegions(CGraph::CRegions& regions)
{
    CGraph::CRegions origRegions = regions;
    regions.m_regions.clear();

    for (std::list<CGraph::CRegion>::const_iterator it = origRegions.m_regions.begin();
         it != origRegions.m_regions.end();
         ++it)
    {
        // skip *it if it's completely contained in any of regions
        bool skip = false;
        for (std::list<CGraph::CRegion>::const_iterator existingIt = regions.m_regions.begin();
             existingIt != regions.m_regions.end();
             ++existingIt)
        {
            if (DoesRegionAContainRegionB(*existingIt, *it))
            {
                skip = true;
                break;
            }
        }
        if (skip)
        {
            continue;
        }

        // remove any of regions that are contained in *it
        for (std::list<CGraph::CRegion>::iterator existingIt = regions.m_regions.begin();
             existingIt != regions.m_regions.end(); )
        {
            if (DoesRegionAContainRegionB(*it, *existingIt))
            {
                existingIt = regions.m_regions.erase(existingIt);
            }
            else
            {
                ++existingIt;
            }
        }
        // now add
        regions.m_regions.push_front(*it);
    }

    // update the bounding box;
    regions.m_AABB.Reset();
    for (std::list<CGraph::CRegion>::const_iterator it = regions.m_regions.begin();
         it != regions.m_regions.end();
         ++it)
    {
        regions.m_AABB.Add(it->m_AABB);
    }
}


CNavigation::CNavigation(ISystem* pSystem)
    : m_navDataState(NDS_UNSET)
    , m_pTriangularNavRegion(0)
    , m_pWaypointHumanNavRegion(0)
    , m_pVolumeNavRegion(0)
    , m_pFlightNavRegion(0)
    , m_pRoadNavRegion(0)
    , m_pSmartObjectNavRegion(0)
    , m_nNumBuildings(0)
    , m_pGraph(0)
    , m_pSystem(pSystem)
    , m_cvBigBrushLimitSize(0)
    , m_cvIncludeNonColEntitiesInNavigation(0)
    , m_cvRadiusForAutoForbidden(0)
    , m_cvShowNavAreas(0)
    , m_pTriangulator(0)
{
    m_pGraph = new CGraph(this);
    m_pTriangularNavRegion = new CTriangularNavRegion(m_pGraph, &m_VertexList);
    m_pWaypointHumanNavRegion = new CWaypointHumanNavRegion(this);
    m_pFlightNavRegion = new CFlightNavRegion(pSystem->GetIPhysicalWorld(), m_pGraph);
    m_pVolumeNavRegion = new CVolumeNavRegion(this);
    m_pRoadNavRegion = new CRoadNavRegion(m_pGraph);
    m_pSmartObjectNavRegion = new CSmartObjectNavRegion(m_pGraph);

    AIInitLog (pSystem);
}

CNavigation::~CNavigation ()
{
    delete m_pTriangularNavRegion;
    m_pTriangularNavRegion = 0;
    delete m_pWaypointHumanNavRegion;
    m_pWaypointHumanNavRegion = 0;
    delete m_pFlightNavRegion;
    m_pFlightNavRegion = 0;

    delete m_pVolumeNavRegion;
    m_pVolumeNavRegion = 0;
    delete m_pRoadNavRegion;
    m_pRoadNavRegion = 0;
    delete m_pSmartObjectNavRegion;
    m_pSmartObjectNavRegion = 0;

    delete m_pGraph;
    m_pGraph = 0;
}

bool CNavigation::Init()
{
    m_nNumBuildings = 0;

    m_cvBigBrushLimitSize = REGISTER_FLOAT("ai_BigBrushCheckLimitSize", 15.f, VF_CHEAT | VF_DUMPTODISK,
            "to be used for finding big objects not enclosed into forbidden areas");
    m_cvIncludeNonColEntitiesInNavigation = REGISTER_INT("ai_IncludeNonColEntitiesInNavigation", 1, 0,
            "Includes/Excludes noncolliding objects from navigation.");
    m_cvRadiusForAutoForbidden = REGISTER_FLOAT("ai_RadiusForAutoForbidden",
            1.0f, VF_CHEAT,
            "If object/vegetation radius is more than this then an automatic forbidden area is created during triangulation.");
    m_cvShowNavAreas = REGISTER_INT("ai_ShowNavAreas",
            0, VF_CHEAT, "Show Navigation Surfaces & Volumes.");

    return true;
}

void CNavigation::ShutDown()
{
    FlushAllAreas();

    if (m_pTriangulator)
    {
        delete m_pTriangulator;
        m_pTriangulator = 0;
    }
}

//====================================================================
// OnMissionLoaded
//====================================================================
void CNavigation::OnMissionLoaded()
{
    m_pWaypointHumanNavRegion->OnMissionLoaded();
    m_pFlightNavRegion->OnMissionLoaded();
    m_pVolumeNavRegion->OnMissionLoaded();
    m_pRoadNavRegion->OnMissionLoaded();
    m_pTriangularNavRegion->OnMissionLoaded();
}

void CNavigation::Serialize(TSerialize ser)
{
    m_pTriangularNavRegion->Serialize(ser);
    m_pWaypointHumanNavRegion->Serialize(ser);
    m_pFlightNavRegion->Serialize(ser);
    m_pVolumeNavRegion->Serialize(ser);
}

//====================================================================
// ExportData
//====================================================================
void CNavigation::ExportData(const char* navFileName, const char* areasFileName,
    const char* roadsFileName, const char* vertsFileName,
    const char* volumeFileName, const char* flightFileName)
{
    if (navFileName)
    {
        AILogProgress("[AISYSTEM] Exporting AI Graph to %s", navFileName);
        m_pGraph->WriteToFile(navFileName);
    }
    if (areasFileName)
    {
        AILogProgress("[AISYSTEM] Exporting Areas to %s", areasFileName);
        WriteAreasIntoFile(areasFileName);
    }

    if (roadsFileName && m_pRoadNavRegion)
    {
        AILogProgress("[AISYSTEM] Exporting Roads to %s", roadsFileName);
        m_pRoadNavRegion->WriteToFile(roadsFileName);
    }

    if (volumeFileName && m_pVolumeNavRegion)
    {
        AILogProgress("[AISYSTEM] Exporting Volume to %s", volumeFileName);
        m_pVolumeNavRegion->WriteToFile(volumeFileName);
    }

    if (flightFileName && m_pFlightNavRegion)
    {
        AILogProgress("[AISYSTEM] Exporting Flight to %s", flightFileName);
        m_pFlightNavRegion->WriteToFile(flightFileName);
    }

    if (vertsFileName)
    {
        AILogProgress("[AISYSTEM] Exporting Vertices to %s", vertsFileName);
        GetVertexList().WriteToFile(vertsFileName);
    }
    AILogProgress("[AISYSTEM] Finished Exporting data");
}

static void CheckShapeNameClashes(const CAIShapeContainer& shapeCont, const char* szContainer)
{
    AILogComment("Dumping names in %s", szContainer);
    const CAIShapeContainer::ShapeVector& shapes = shapeCont.GetShapes();
    for (unsigned i = 0; i < shapes.size(); ++i)
    {
        unsigned n = 1;
        AILogComment(" '%s'", shapes[i]->GetName().c_str());
        for (unsigned j = i + 1; j < shapes.size(); ++j)
        {
            if (shapes[i]->GetName().compare(shapes[j]->GetName()) == 0)
            {
                ++n;
            }
        }
        if (n > 1)
        {
            AIError("%s containes %d duplicates of shapes called '%s'", szContainer, n, shapes[i]->GetName().c_str());
        }
    }
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::FlushSystemNavigation(void)
{
    // clear areas id's
    SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
    while (si != m_mapSpecialAreas.end())
    {
        m_BuildingIDManager.FreeId((si->second).nBuildingID);
        (si->second).nBuildingID = -1;
        (si->second).bAltered = false;
        ++si;
    }
    m_BuildingIDManager.FreeAll();

    ClearForbiddenQuadTrees();
    GetVertexList().Clear();

    m_pTriangularNavRegion->Clear();
    m_pWaypointHumanNavRegion->Clear();
    m_pFlightNavRegion->Clear();
    m_pVolumeNavRegion->Clear();
    m_pRoadNavRegion->Clear();
    m_pSmartObjectNavRegion->Clear();

    if (m_pGraph)
    {
        m_pGraph->Clear(IAISystem::NAVMASK_ALL);
    }
}

void CNavigation::ValidateNavigation()
{
    m_validationErrorMarkers.clear();
    ValidateBigObstacles();
}

// NOTE Jun 7, 2007: <pvl> not const anymore because ForbiddenAreasOverlap()
// isn't const anymore
//===================================================================
// ValidateAreas
//===================================================================
bool CNavigation::ValidateAreas()
{
    bool result = true;
    for (ShapeMap::const_iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        if (it->second.shape.size() < 2)
        {
            AIWarning("AI Path %s has only %lu points", it->first.c_str(), it->second.shape.size());
            result = false;
        }
    }
    for (unsigned i = 0, ni = m_designerForbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_designerForbiddenAreas.GetShapes()[i];
        if (pShape->GetPoints().size() < 3)
        {
            AIWarning("AI Designer Forbidden Area %s has only %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
            result = false;
        }
    }
    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
        if (pShape->GetPoints().size() < 3)
        {
            AIWarning("AI Forbidden Area %s has only %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
            result = false;
        }
    }
    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
        if (pShape->GetPoints().size() < 3)
        {
            AIWarning("AI Forbidden Boundary %s has only %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
            result = false;
        }
    }
    for (SpecialAreaMap::const_iterator it = m_mapSpecialAreas.begin(); it != m_mapSpecialAreas.end(); ++it)
    {
        if (it->second.GetPolygon().size() < 3)
        {
            AIWarning("AI Area %s has only %lu points", it->first.c_str(), it->second.GetPolygon().size());
            result = false;
        }
    }
    for (ShapeMap::const_iterator it = m_mapOcclusionPlanes.begin(); it != m_mapOcclusionPlanes.end(); ++it)
    {
        if (it->second.shape.size() < 2)
        {
            AIWarning("AI Occlusion Plane %s has only %lu points", it->first.c_str(), it->second.shape.size());
            result = false;
        }
    }
    for (PerceptionModifierShapeMap::const_iterator it = m_mapPerceptionModifiers.begin(); it != m_mapPerceptionModifiers.end(); ++it)
    {
        if (it->second.shape.size() < 2)
        {
            AIWarning("AI Perception Modifier %s has only %lu points", it->first.c_str(), it->second.shape.size());
            result = false;
        }
    }

    if (ForbiddenAreasOverlap())
    {
        result = false;
    }

    // Check name clashes.
    CheckShapeNameClashes(m_forbiddenBoundaries, "Forbidden Boundaries");
    CheckShapeNameClashes(m_designerForbiddenAreas, "Forbidden Areas (editor)");
    CheckShapeNameClashes(m_forbiddenAreas, "Forbidden Areas (automatic)");

    return result;
}

//
//-----------------------------------------------------------------------------------------------------------
//
bool CNavigation::ValidateBigObstacles()
{
    if (gEnv->pAISystem->GetNavigationSystem()->IsInUse())
    {
        return true;
    }

    float   trhSize(m_cvBigBrushLimitSize->GetFVal());
    Vec3 min, max;
    float fTSize = (float) m_pSystem->GetI3DEngine()->GetTerrainSize();
    AILogProgress(" Checking for big obstacles out of forbidden areas. Terrain size = %.0f", fTSize);

    min.Set(0, 0, -5000);
    max.Set(fTSize, fTSize, 5000.0f);

    // get only static physical entities (trees, buildings etc...)
    IPhysicalEntity** pObstacles;
    int flags = ent_static | ent_ignore_noncolliding;
    int count = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(min, max, pObstacles, flags);
    for (int i = 0; i < count; ++i)
    {
        IPhysicalEntity* pCurrent = pObstacles[i];

        // don't add entities (only triangulate brush-like objects, and brush-like
        // objects should not be entities!)
        IEntity* pEntity = (IEntity*) pCurrent->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
        if (pEntity)
        {
            continue;
        }

        pe_params_foreign_data pfd;
        if (pCurrent->GetParams(&pfd) == 0)
        {
            continue;
        }

        // skip trees
        IRenderNode* pRenderNode = (IRenderNode*) pCurrent->GetForeignData(PHYS_FOREIGN_ID_STATIC);
        if (!pRenderNode)
        {
            continue;
        }
        if (pRenderNode && pRenderNode->GetRenderNodeType() == eERType_Vegetation)
        {
            continue;
        }
        if (pCurrent->GetForeignData() || (pfd.iForeignFlags & PFF_EXCLUDE_FROM_STATIC))
        {
            continue;
        }

        pe_status_pos sp;
        sp.ipart = -1;
        sp.partid = -1;
        if (pCurrent->GetStatus(&sp) == 0)
        {
            continue;
        }

        pe_status_pos status;
        if (pCurrent->GetStatus(&status) == 0)
        {
            continue;
        }

        Vec3 calc_pos = status.pos;

        IVisArea* pArea;
        int buildingID;
        if (CheckNavigationType(calc_pos, buildingID, pArea, IAISystem::NAV_WAYPOINT_HUMAN) == IAISystem::NAV_WAYPOINT_HUMAN)
        {
            continue;
        }

        if (!IsPointInTriangulationAreas(calc_pos))
        {
            continue;
        }

        if (IsPointInForbiddenRegion(calc_pos, true))
        {
            continue;
        }

        if (status.BBox[1].x - status.BBox[0].x > trhSize
            || status.BBox[1].y - status.BBox[0].y > trhSize)
        {
            Matrix34A TM;
            IStatObj* pStatObj = pRenderNode->GetEntityStatObj(0, 0, &TM);
            if (pStatObj)
            {
                char msg[256];
                azsnprintf(msg, 256, "Big object\n(%.f, %.f, %.f).", calc_pos.x, calc_pos.y, calc_pos.z);
                OBB obb;
                obb.SetOBBfromAABB(Matrix33(TM), pStatObj->GetAABB());
                m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, TM.GetTranslation(), obb, ColorB(255, 196, 0)));

                AIWarning("BIG object not enclosed in forbidden area. Pos: (%.f   %.f   %.f). Make sure it has NoTrinagulate flag (or is within forbidden area)", calc_pos.x, calc_pos.y, calc_pos.z);
            }
            else
            {
                AIWarning("BIG object not enclosed in forbidden area. Pos: (%.f   %.f   %.f). Make sure it has NoTrinagulate flag (or is within forbidden area). The geometry does not have stat object, maybe a solid?", calc_pos.x, calc_pos.y, calc_pos.z);
            }
            //          AIError("Forbidden area/boundary ");
        }
        //      static float edgeTol = 0.3f;
        //      if (IsPointOnForbiddenEdge(calc_pos, edgeTol))
        //          continue;
    }
    return true;
}

void CNavigation::Reset(IAISystem::EResetReason reason)
{
    if (reason == IAISystem::RESET_ENTER_GAME)
    {
        m_validationErrorMarkers.clear();
    }

    for (ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.begin(); it != m_mapExtraLinkCosts.end(); ++it)
    {
        const string& name = it->first;
        SExtraLinkCostShape& shape = it->second;
        shape.costFactor = shape.origCostFactor;
    }

    // enable all nav modifiers
    SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
    while (si != m_mapSpecialAreas.end())
    {
        (si->second).bAltered = false;
        ++si;
    }

    // Reset path devalue stuff.
    for (ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        it->second.devalueTime = 0;
    }

    GetTriangularNavRegion()->Reset(reason);
    GetVolumeNavRegion()->Reset(reason);
    GetWaypointHumanNavRegion()->Reset(reason);
    GetFlightNavRegion()->Reset(reason);
    GetRoadNavRegion()->Reset(reason);
    GetSmartObjectsNavRegion()->Reset(reason);
}

// copies a designer path into provided list if a path of such name is found
//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::GetDesignerPath(const char* szName, SShape& path) const
{
    ShapeMap::const_iterator di = m_mapDesignerPaths.find(szName);
    if (di == m_mapDesignerPaths.end())
    {
        return false;
    }
    path = di->second;
    return true;
}

//====================================================================
// WriteArea
//====================================================================
//void WriteArea(CCryFile & file, const string & name, const SpecialArea & sa)
//{
//  unsigned nameLen = name.length();
//  file.Write(&nameLen, sizeof(nameLen));
//  file.Write((char *) name.c_str(), nameLen * sizeof(char));
//  int64 type = sa.type;
//  file.Write((void *) &type, sizeof(type));
//  int64 waypointConnections = sa.waypointConnections;
//  file.Write((void *) &waypointConnections, sizeof(waypointConnections));
//  char altered = sa.bAltered;
//  file.Write((void *) &altered, sizeof(altered));
//  file.Write((void *) &sa.fHeight, sizeof(sa.fHeight));
//  file.Write((void *) &sa.fNodeAutoConnectDistance, sizeof(sa.fNodeAutoConnectDistance));
//  file.Write((void *) &sa.fMaxZ, sizeof(sa.fMaxZ));
//  file.Write((void *) &sa.fMinZ, sizeof(sa.fMinZ));
//  file.Write((void *) &sa.nBuildingID, sizeof(sa.nBuildingID));
//  unsigned char lightLevel = (int)sa.lightLevel;
//  file.Write((void *) &lightLevel, sizeof(lightLevel));
//  char critterOnly = sa.bCritterOnly;
//  file.Write((void *) &critterOnly, sizeof(critterOnly));
//
//  // now the area itself
//  const ListPositions & pts = sa.GetPolygon();
//  unsigned ptsSize = pts.size();
//  file.Write(&ptsSize, sizeof(ptsSize));
//  ListPositions::const_iterator it;
//  for (it = pts.begin() ; it != pts.end() ; ++it)
//  {
//      // only works so long as *it is a contiguous object
//      const Vec3 & pt = *it;
//      file.Write((void *) &pt, sizeof(pt));
//  }
//}

//====================================================================
// WriteForbiddenArea
//====================================================================
//void WriteForbiddenArea(CCryFile & file, const CAIShape* shape)
//{
//  unsigned nameLen = shape->GetName().length();
//  if (nameLen >= maxForbiddenNameLen)
//      nameLen = maxForbiddenNameLen - 1;
//  file.Write(&nameLen, sizeof(nameLen));
//  file.Write((char*)shape->GetName().c_str(), nameLen * sizeof(char));
//
//  const ShapePointContainer& pts = shape->GetPoints();
//  unsigned ptsSize = pts.size();
//  file.Write(&ptsSize, sizeof(ptsSize));
//  for (unsigned i = 0; i < ptsSize; ++i)
//  {
//      // only works so long as *it is a contiguous object
//      const Vec3& pt = pts[i];
//      file.Write((void *) &pt, sizeof(pt));
//  }
//}

//====================================================================
// WriteExtraLinkCostArea
//====================================================================
//void WriteExtraLinkCostArea(CCryFile & file, const string & name, const SExtraLinkCostShape &shape)
//{
//  unsigned nameLen = name.length();
//  file.Write(&nameLen, sizeof(nameLen));
//  file.Write((char *) name.c_str(), nameLen * sizeof(char));
//
//  file.Write(&shape.origCostFactor, sizeof(shape.origCostFactor));
//  file.Write(&shape.aabb.min, sizeof(shape.aabb.min));
//  file.Write(&shape.aabb.max, sizeof(shape.aabb.max));
//
//  unsigned ptsSize = shape.shape.size();
//  file.Write(&ptsSize, sizeof(ptsSize));
//  ListPositions::const_iterator it;
//  for (it = shape.shape.begin() ; it != shape.shape.end() ; ++it)
//  {
//      // only works so long as *it is a contiguous object
//      const Vec3 & pt = *it;
//      file.Write((void *) &pt, sizeof(pt));
//  }
//}

//====================================================================
// WritePolygonArea
//====================================================================
void CNavigation::WritePolygonArea(CCryFile& file, const string& name, const ListPositions& pts)
{
    unsigned nameLen = name.length();
    if (nameLen >= maxForbiddenNameLen)
    {
        nameLen = maxForbiddenNameLen - 1;
    }
    file.Write(&nameLen, sizeof(nameLen));
    file.Write((char*) name.c_str(), nameLen * sizeof(char));
    unsigned ptsSize = pts.size();
    file.Write(&ptsSize, sizeof(ptsSize));
    ListPositions::const_iterator it;
    for (it = pts.begin(); it != pts.end(); ++it)
    {
        // only works so long as *it is a contiguous object
        const Vec3& pt = *it;
        file.Write((void*) &pt, sizeof(pt));
    }
}

//====================================================================
// WriteAreasIntoFile
// SEG_WORLD: Checks bounds using segmentAABB. Use an empty AABB to bypass bounds check.
//====================================================================
void CNavigation::WriteAreasIntoFile(const char* fileName)
{
    CCryFile file;
    if (false != file.Open(fileName, "wb"))
    {
        int fileVersion = BAI_AREA_FILE_VERSION_WRITE;
        file.Write(&fileVersion, sizeof(fileVersion));

        // Write designer paths
        {
            unsigned numDesignerPaths = m_mapDesignerPaths.size();
            file.Write(&numDesignerPaths, sizeof(numDesignerPaths));
            ShapeMap::const_iterator it;
            for (it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
            {
                WritePolygonArea(file, it->first, it->second.shape);
                int navType = it->second.navType;
                file.Write(&navType, sizeof(navType));
                file.Write(&it->second.type, sizeof(int));
                file.Write(&it->second.closed, sizeof(bool));
            }
        }


        //----------------------------------------------------------
        // End of navigation-related shapes, start of other shapes
        //----------------------------------------------------------
        // Write generic shapes
        {
            unsigned numGenericShapes = m_mapGenericShapes.size();
            file.Write(&numGenericShapes, sizeof(numGenericShapes));
            ShapeMap::const_iterator it;
            for (it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
            {
                WritePolygonArea(file, it->first, it->second.shape);
                int navType = it->second.navType;
                file.Write(&navType, sizeof(navType));
                file.Write(&it->second.type, sizeof(int));
                float height = it->second.height;
                int lightLevel = (int)it->second.lightLevel;
                file.Write(&height, sizeof(height));
                file.Write(&lightLevel, sizeof(lightLevel));
            }
        }
    }
    else
    {
        AIWarning("Unable to open areas file %s", fileName);
    }
}

//====================================================================
// ReadArea
//====================================================================
//void ReadArea(CCryFile & file, int version, string & name, SpecialArea & sa)
//{
//  unsigned nameLen;
//  file.ReadType(&nameLen);
//  char tmpName[1024];
//  file.ReadType(&tmpName[0], nameLen);
//  tmpName[nameLen] = '\0';
//  name = tmpName;
//
//  int64 type = 0;
//  file.ReadType(&type);
//  sa.type = (SpecialArea::EType) type;
//
//  int64 waypointConnections = 0;
//  file.ReadType(&waypointConnections);
//  sa.waypointConnections = (EWaypointConnections) waypointConnections;
//
//  char altered = 0;
//  file.ReadType(&altered);
//  sa.bAltered = altered != 0;
//  file.ReadType(&sa.fHeight);
//  if (version <= 16)
//  {
//      float junk;
//      file.ReadType(&junk);
//  }
//  file.ReadType(&sa.fNodeAutoConnectDistance);
//  file.ReadType(&sa.fMaxZ);
//  file.ReadType(&sa.fMinZ);
//  file.ReadType(&sa.nBuildingID);
//  if (version >= 18)
//  {
//      unsigned char lightLevel = 0;
//      file.ReadType(&lightLevel);
//      sa.lightLevel = (EAILightLevel)lightLevel;
//  }
//
//  // now the area itself
//  ListPositions pts;
//  unsigned ptsSize;
//  file.ReadType(&ptsSize);
//
//  for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
//  {
//      Vec3 pt;
//      file.ReadType(&pt);
//      pts.push_back(pt);
//  }
//  sa.SetPolygon(pts);
//}

//====================================================================
// ReadPolygonArea
//====================================================================
//bool ReadPolygonArea(CCryFile & file, int version, string & name, ListPositions & pts)
//{
//  unsigned nameLen = maxForbiddenNameLen;
//  file.ReadType(&nameLen);
//  if (nameLen >= maxForbiddenNameLen)
//  {
//      AIWarning("Excessive forbidden area name length - AI loading failed");
//      return false;
//  }
//  char tmpName[maxForbiddenNameLen];
//  file.ReadRaw(&tmpName[0], nameLen);
//  tmpName[nameLen] = '\0';
//  name = tmpName;
//
//  unsigned ptsSize;
//  file.ReadType(&ptsSize);
//
//  for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
//  {
//      Vec3 pt;
//      file.ReadType(&pt);
//      pts.push_back(pt);
//  }
//  return true;
//}

//====================================================================
// ReadForbiddenArea
//====================================================================
//bool ReadForbiddenArea(CCryFile & file, int version, CAIShape* shape)
//{
//  unsigned nameLen = maxForbiddenNameLen;
//  file.ReadType(&nameLen);
//  if (nameLen >= maxForbiddenNameLen)
//  {
//      AIWarning("Excessive forbidden area name length - AI loading failed");
//      return false;
//  }
//  char tmpName[maxForbiddenNameLen];
//  file.ReadRaw(&tmpName[0], nameLen);
//  tmpName[nameLen] = '\0';
//  shape->SetName(tmpName);
//
//  unsigned ptsSize;
//  file.ReadType(&ptsSize);
//
//  ShapePointContainer& pts = shape->GetPoints();
//  pts.resize(ptsSize);
//
//  for (unsigned i = 0; i < ptsSize; ++i)
//      file.ReadType(&pts[i]);
//
//  // Call build AABB since the point container was filled directly.
//  shape->BuildAABB();
//
//  return true;
//}

//====================================================================
// ReadExtraLinkCostArea
//====================================================================
//void ReadExtraLinkCostArea(CCryFile & file, int version, string & name, SExtraLinkCostShape &shape)
//{
//  unsigned nameLen;
//  file.ReadType(&nameLen);
//  char tmpName[1024];
//  file.ReadRaw(&tmpName[0], nameLen);
//  tmpName[nameLen] = '\0';
//  name = tmpName;
//
//  file.ReadType(&shape.origCostFactor);
//  shape.costFactor = shape.origCostFactor;
//  file.ReadType(&shape.aabb.min);
//  file.ReadType(&shape.aabb.max);
//
//  unsigned ptsSize;
//  file.ReadType(&ptsSize);
//
//  for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
//  {
//      Vec3 pt;
//      file.ReadType(&pt);
//      shape.shape.push_back(pt);
//  }
//}


//====================================================================
// ReadAreasFromFile
// only actually reads if not editor - in editor just checks the version
//====================================================================
//void CNavigation::ReadAreasFromFile(CCryFile & file, int fileVersion)
//{
//  // (MATT) This function is really pointless in Editor now, surely? Have just preserved sideeffects {2008/07/20}
//
//  m_forbiddenAreas.Clear();
//
//  unsigned numAreas;
//  {
//      file.ReadType(&numAreas);
//      // vague sanity check
//      AIAssert(numAreas < 1000000);
//      for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
//      {
//          CAIShape* pShape = new CAIShape;
//          if (!ReadForbiddenArea(file, fileVersion, pShape))
//              return;
//          if (m_forbiddenAreas.FindShape(pShape->GetName()) != 0)
//          {
//              AIError("CAISystem::ReadAreasFromFile: Forbidden area '%s' already exists, please rename the shape and reexport.", pShape->GetName().c_str());
//              delete pShape;
//          }
//          else
//              m_forbiddenAreas.InsertShape(pShape);
//      }
//  }
//
//  return;
//}

//====================================================================
// GetSpecialArea
//====================================================================
const SpecialArea* CNavigation::GetSpecialArea(const Vec3& pos, SpecialArea::EType areaType)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    // make sure each area has a building id
    // Flight/water navigation modifiers are only used to limit the initial navigation
    // area during preprocessing - give them a building id anyway
    for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        SpecialArea& sa = si->second;
        if (sa.nBuildingID < 0)
        {
            (si->second).nBuildingID = m_BuildingIDManager.GetId();
        }
    }

    for (SpecialAreaMap::const_iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        const SpecialArea& sa = si->second;
        if (sa.type == areaType)
        {
            if (IsPointInSpecialArea(pos, sa))
            {
                return &sa;
            }
        }
    }
    return 0;
}

//====================================================================
// GetSpecialArea
//====================================================================
const SpecialArea* CNavigation::GetSpecialArea(int buildingID) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
    if (!gEnv->IsEditor())
    {
        return m_vectorSpecialAreas[buildingID];
    }

    for (SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin();
         di != m_mapSpecialAreas.end(); ++di)
    {
        if (di->second.nBuildingID == buildingID)
        {
            return &(di->second);
        }
    }
    return 0;
}

//void CNavigation::InsertSpecialArea(const string & name, SpecialArea & sa)
//{
//  std::pair <SpecialAreaMap::iterator, bool> result =
//      m_mapSpecialAreas.insert(SpecialAreaMap::iterator::value_type(name,sa));
//
//  if (! gEnv->IsEditor())
//  {
//      if (m_vectorSpecialAreas.size() <= sa.nBuildingID)
//          m_vectorSpecialAreas.resize (2*m_vectorSpecialAreas.size());
//      m_vectorSpecialAreas[sa.nBuildingID] = & result.first->second;
//  }
//}

void CNavigation::EraseSpecialArea(const string& name)
{
    SpecialAreaMap::iterator si;
    si = m_mapSpecialAreas.find(name);
    if (si == m_mapSpecialAreas.end())
    {
        return;
    }

    if (!gEnv->IsEditor())
    {
        m_vectorSpecialAreas[si->second.nBuildingID] = 0;
    }

    if (si->second.nBuildingID >= 0)
    {
        m_BuildingIDManager.FreeId(si->second.nBuildingID);
    }
    m_mapSpecialAreas.erase(si);
}

void CNavigation::FlushSpecialAreas()
{
    m_mapSpecialAreas.clear();
    if (!gEnv->IsEditor())
    {
        m_vectorSpecialAreas.resize(0);
        // NOTE Mai 6, 2007: <pvl> 64 should be enough for most levels I guess. If
        // it's not for some level, InsertSpecialArea() will grow the vector.
        m_vectorSpecialAreas.resize(64, 0);
    }
}


//====================================================================
// GetVolumeRegions
//====================================================================
void CNavigation::GetVolumeRegions(VolumeRegions& volumeRegions)
{
    volumeRegions.resize(0);
    for (SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin(); di != m_mapSpecialAreas.end(); ++di)
    {
        if (di->second.type == SpecialArea::TYPE_VOLUME)
        {
            volumeRegions.push_back(std::make_pair(di->first, &di->second));
        }
    }
}

//====================================================================
// GetBuildingInfo
//====================================================================
bool CNavigation::GetBuildingInfo(int nBuildingID, IAISystem::SBuildingInfo& info)
{
    const SpecialArea* sa = GetSpecialArea(nBuildingID);
    if (sa)
    {
        info.fNodeAutoConnectDistance = sa->fNodeAutoConnectDistance;
        info.waypointConnections = sa->waypointConnections;
        return true;
    }
    else
    {
        return false;
    }
}

bool CNavigation::IsPointInBuilding(const Vec3& pos, int nBuildingID)
{
    const SpecialArea* sa = GetSpecialArea(nBuildingID);
    if (!sa)
    {
        return false;
    }
    return IsPointInSpecialArea(pos, *sa);
}

void CNavigation::FlushAllAreas()
{
    FlushSpecialAreas();

    m_designerForbiddenAreas.Clear();
    m_forbiddenAreas.Clear();
    m_forbiddenBoundaries.Clear();

    m_mapGenericShapes.clear();
    m_mapOcclusionPlanes.clear();
    m_mapPerceptionModifiers.clear();

    m_mapDesignerPaths.clear();
    m_mapExtraLinkCosts.clear();
    m_BuildingIDManager.FreeAll();                  // Mikko/Martin - id manager should be reset
}

//====================================================================
// GetNavigationShapeName
//====================================================================
const char* CNavigation::GetNavigationShapeName(int nBuildingID) const
{
    for (SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin(); di != m_mapSpecialAreas.end(); ++di)
    {
        int bID = di->second.nBuildingID;
        const char* name = di->first.c_str();
        if (bID == nBuildingID)
        {
            return name;
        }
    }
    return "Cannot convert building id to name";
}

//===================================================================
// IsShapeCompletelyInForbiddenRegion
//===================================================================
template<typename VecContainer>
bool CNavigation::IsShapeCompletelyInForbiddenRegion(VecContainer& shape, bool checkAutoGenRegions) const
{
    const typename VecContainer::const_iterator itEnd = shape.end();
    for (typename VecContainer::const_iterator it = shape.begin(); it != itEnd; ++it)
    {
        if (!IsPointInForbiddenRegion(*it, checkAutoGenRegions))
        {
            return false;
        }
    }
    return true;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::DoesNavigationShapeExists(const char* szName, EnumAreaType areaType, bool road)
{
    if (areaType == AREATYPE_PATH)
    {
        if (road)
        {
            return m_pRoadNavRegion->DoesRoadExists(szName);
        }
        else
        {
            return m_mapDesignerPaths.find(szName) != m_mapDesignerPaths.end();
        }
    }
    else if (areaType == AREATYPE_FORBIDDEN)
    {
        return m_designerForbiddenAreas.FindShape(szName) != 0;
    }
    else if (areaType == AREATYPE_FORBIDDENBOUNDARY)
    {
        return m_forbiddenBoundaries.FindShape(szName) != 0;
    }
    else if (areaType == AREATYPE_NAVIGATIONMODIFIER)
    {
        return m_mapSpecialAreas.find(szName) != m_mapSpecialAreas.end() ||
               m_mapExtraLinkCosts.find(szName) != m_mapExtraLinkCosts.end();
    }
    else if (areaType == AREATYPE_OCCLUSION_PLANE)
    {
        return m_mapOcclusionPlanes.find(szName) != m_mapOcclusionPlanes.end();
    }
    else if (areaType == AREATYPE_GENERIC)
    {
        return m_mapGenericShapes.find(szName) != m_mapGenericShapes.end();
    }

    return false;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::CreateNavigationShape(const SNavigationShapeParams& params)
{
    // need at least one point in a path. Some paths need more than one (areas need 3)
    if (params.nPoints == 0)
    {
        return true; // Do not report too few points as errors.
    }
    std::vector<Vec3> vecPts(params.points, params.points + params.nPoints);

    if (params.areaType == AREATYPE_PATH && params.pathIsRoad == false)
    {
        //designer path need to preserve directions
    }
    else
    {
        if (params.closed)
        {
            EnsureShapeIsWoundAnticlockwise<std::vector<Vec3>, float>(vecPts);
        }
    }

    ListPositions listPts(vecPts.begin(), vecPts.end());

    if (params.areaType == AREATYPE_PATH)
    {
        if (listPts.size() < 2)
        {
            return true; // Do not report too few points as errors.
        }
        if (params.pathIsRoad)
        {
            return m_pRoadNavRegion->CreateRoad(params.szPathName, vecPts, 10.0f, 2.5f);
        }
        else
        {
            if (m_mapDesignerPaths.find(params.szPathName) != m_mapDesignerPaths.end())
            {
                AIError("CNavigation::CreateNavigationShape: Designer path '%s' already exists, please rename the path.", params.szPathName);
                return false;
            }

            if (params.closed)
            {
                if (Distance::Point_Point(listPts.front(), listPts.back()) > 0.1f)
                {
                    listPts.push_back(listPts.front());
                }
            }
            m_mapDesignerPaths.insert(ShapeMap::iterator::value_type(params.szPathName, SShape(listPts, false, (IAISystem::ENavigationType)params.nNavType, params.nAuxType, params.closed)));
        }
    }
    else if (params.areaType == AREATYPE_OCCLUSION_PLANE)
    {
        if (listPts.size() < 3)
        {
            return true; // Do not report too few points as errors.
        }
        if (m_mapOcclusionPlanes.find(params.szPathName) != m_mapOcclusionPlanes.end())
        {
            AIError("CNavigation::CreateNavigationShape: Occlusion plane '%s' already exists, please rename the shape.", params.szPathName);
            return false;
        }

        m_mapOcclusionPlanes.insert(ShapeMap::iterator::value_type(params.szPathName, SShape(listPts)));
    }
    else if (params.areaType == AREATYPE_PERCEPTION_MODIFIER)
    {
        if (listPts.size() < 2)
        {
            return false;
        }

        PerceptionModifierShapeMap::iterator di;
        di = m_mapPerceptionModifiers.find(params.szPathName);

        if (di == m_mapPerceptionModifiers.end())
        {
            SPerceptionModifierShape pms(listPts, params.fReductionPerMetre, params.fReductionMax, params.fHeight, params.closed);
            m_mapPerceptionModifiers.insert(PerceptionModifierShapeMap::iterator::value_type(params.szPathName, pms));
        }
        else
        {
            return false;
        }
    }
    else if (params.areaType == AREATYPE_GENERIC)
    {
        if (listPts.size() < 3)
        {
            return true; // Do not report too few points as errors.
        }
        if (m_mapGenericShapes.find(params.szPathName) != m_mapGenericShapes.end())
        {
            AIError("CNavigation::CreateNavigationShape: Shape '%s' already exists, please rename the shape.", params.szPathName);
            return false;
        }

        m_mapGenericShapes.insert(ShapeMap::iterator::value_type(params.szPathName,
                SShape(listPts, false, IAISystem::NAV_UNSET, params.nAuxType, params.closed, params.fHeight, params.lightLevel)));
    }

    return true;
}

// deletes designer created path
//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DeleteNavigationShape(const char* szName)
{
    ShapeMap::iterator di;

    m_pRoadNavRegion->DeleteRoad(szName);

    di = m_mapDesignerPaths.find(szName);
    if (di != m_mapDesignerPaths.end())
    {
        m_mapDesignerPaths.erase(di);
    }

    if (CAIShape* pShape = m_designerForbiddenAreas.FindShape(szName))
    {
        ClearForbiddenQuadTrees();
        m_forbiddenAreas.Clear();   // The designer forbidden areas will be duplicated into the forbidden areas when regenerating navigation.
        m_designerForbiddenAreas.DeleteShape(pShape);
    }

    if (CAIShape* pShape = m_forbiddenBoundaries.FindShape(szName))
    {
        ClearForbiddenQuadTrees();
        m_forbiddenBoundaries.DeleteShape(pShape);
    }

    EraseSpecialArea(szName);

    ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.find(szName);
    if (it != m_mapExtraLinkCosts.end())
    {
        m_mapExtraLinkCosts.erase(it);
    }

    di = m_mapGenericShapes.find(szName);
    if (di != m_mapGenericShapes.end())
    {
        m_mapGenericShapes.erase(di);
    }

    di = m_mapOcclusionPlanes.find(szName);
    if (di != m_mapOcclusionPlanes.end())
    {
        m_mapOcclusionPlanes.erase(di);
    }

    PerceptionModifierShapeMap::iterator pmsi = m_mapPerceptionModifiers.find(szName);
    if (pmsi != m_mapPerceptionModifiers.end())
    {
        m_mapPerceptionModifiers.erase(pmsi);
    }

    // FIXME Mrz 14, 2008: <pvl> how to handle this???
    // Light manager might have pointers to shapes and areas, update it.
    //m_lightManager.Update(true);
}

// NOTE Jun 7, 2007: <pvl> not const any more, needs to write to m_validationErrorMarkers
//===================================================================
// ForbiddenAreasOverlap
//===================================================================
bool CNavigation::ForbiddenAreasOverlap()
{
    bool res = false;
    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        if (ForbiddenAreaOverlap(m_forbiddenAreas.GetShapes()[i]))
        {
            res = true;
        }
    }
    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        if (ForbiddenAreaOverlap(m_forbiddenBoundaries.GetShapes()[i]))
        {
            res = true;
        }
    }
    return res;
}

//===================================================================
// ForbiddenAreasOverlap
//===================================================================
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
bool CNavigation::ForbiddenAreaOverlap(const CAIShape* pShape)
{
    const ShapePointContainer& pts = pShape->GetPoints();
    const string& name = pShape->GetName();

    if (pts.size() < 3)
    {
        AIError("Forbidden area/boundary %s has less than 3 points - you must delete it and rerun triangulation [Design bug]", name.c_str());
        return true;
    }

    for (unsigned i = 0, ni = pts.size(); i < ni; ++i)
    {
        const Vec3& v0 = pts[i];
        const Vec3& v1 = pts[(i + 1) % ni];

        if (IsEquivalent2D(v0, v1, 0.001f))
        {
            AIError("Forbidden area/boundary %s contains one or more identical points (%5.2f, %5.2f, %5.2f) - Fix (e.g. adjust position of forbidden areas/objects) and run triangulation again [Design bug... possibly]",
                name.c_str(), v0.x, v0.y, v0.z);

            char msg[256];
            azsnprintf(msg, 256, "Identical points\n(%.f, %.f, %.f).", v0.x, v0.y, v0.z);
            m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, v0, ColorB(255, 0, 0)));

            return true;
        }
    }

    for (unsigned i = 0, ni = pts.size(); i < ni; ++i)
    {
        unsigned ine = (i + 1) % ni;
        const Vec3& vi0 = pts[i];
        const Vec3& vi1 = pts[ine];

        // check for self-intersection with all remaining segments
        for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
        {
            unsigned jne = (j + 1) % nj;
            if (j == ine || jne == i || i == j)
            {
                continue;
            }

            const Vec3& vj0 = pts[j];
            const Vec3& vj1 = pts[jne];

            real s, t;
            if (Intersect::Lineseg_Lineseg2D(Linesegr(vi0, vi1), Linesegr(vj0, vj1), s, t))
            {
                Vec3 pos = Lineseg(vi0, vi1).GetPoint(s);
                AIError("Forbidden area/boundary %s self-intersects. Pos: (%5.2f, %5.2f, %5.2f). Please fix (e.g. adjust position of forbidden areas/objects) and re-triangulate [Design bug... possibly]",
                    name.c_str(), pos.x, pos.y, pos.z);

                char msg[256];
                azsnprintf(msg, 256, "Self-intersection\n(%.f, %.f, %.f).", pos.x, pos.y, pos.z);
                m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, pos, ColorB(255, 0, 0)));

                return true;
            }
        }
        Vec3 pt;
        if (IntersectsForbidden(vi0, vi1, pt, &name))
        {
            AIError("!Forbidden area %s intersects with another forbidden area. Pos: (%5.2f, %5.2f, %5.2f) . Please fix (e.g. adjust position of forbidden areas/objects) and re-triangulate [Design bug... possibly]",
                name.c_str(), pt.x, pt.y, pt.z);

            char msg[256];
            azsnprintf(msg, 256, "Mutual-intersection\n(%.f, %.f, %.f).", pt.x, pt.y, pt.z);
            m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, pt, ColorB(255, 0, 0)));

            return true;
        }
    }

    return false;
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif

//====================================================================
// IsPointInForbiddenRegion
// Note that although forbidden regions aren't allowed to cross they can
// be nested, so we assume that in/out alternates with nesting...
//====================================================================
bool CNavigation::IsPointInForbiddenRegion(const Vec3& pos, bool checkAutoGenRegions) const
{
    return IsPointInForbiddenRegion(pos, 0, checkAutoGenRegions);
}

//===================================================================
// IsPointInForbiddenRegion
//===================================================================
bool CNavigation::IsPointInForbiddenRegion(const Vec3& pos, const CAIShape** ppShape, bool checkAutoGenRegions) const
{
    FUNCTION_PROFILER(m_pSystem, PROFILE_AI);

    if (checkAutoGenRegions)
    {
        return m_forbiddenAreas.IsPointInside(pos, ppShape);
    }
    return m_designerForbiddenAreas.IsPointInside(pos, ppShape);
}

//====================================================================
// IsPointInWaterAreas
//====================================================================
inline bool IsPointInWaterAreas(const Vec3& pt, const std::vector<ListPositions>& areas)
{
    unsigned nAreas = areas.size();
    for (unsigned i = 0; i < nAreas; ++i)
    {
        const ListPositions& area = areas[i];
        if (Overlap::Point_Polygon2D(pt, area))
        {
            return true;
        }
    }
    return false;
}

//===================================================================
// IsPointInWaterAreas
//===================================================================
bool CNavigation::IsPointInWaterAreas(const Vec3& pt) const
{
    for (SpecialAreaMap::const_iterator it = m_mapSpecialAreas.begin(); it != m_mapSpecialAreas.end(); ++it)
    {
        const string& name = it->first;
        const SpecialArea& sa = it->second;
        if (sa.type == SpecialArea::TYPE_WATER)
        {
            if (Overlap::Point_AABB2D(pt, sa.GetAABB()))
            {
                if (Overlap::Point_Polygon2D(pt, sa.GetPolygon()))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

//====================================================================
// IsPointInSpecialArea
//====================================================================
bool CNavigation::IsPointInSpecialArea(const Vec3& pos, const SpecialArea& sa)
{
    if (sa.fHeight > 0.00001f && (pos.z < sa.fMinZ || pos.z > sa.fMaxZ))
    {
        return false;
    }
    if (Overlap::Point_Polygon2D(pos, sa.GetPolygon(), &sa.GetAABB()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//===================================================================
// IsPointInTriangulationAreas
//===================================================================
bool CNavigation::IsPointInTriangulationAreas(const Vec3& pos)
{
    AIAssert(gEnv->IsEditor());
    bool foundOne = false;
    for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        SpecialArea& sa = si->second;
        if (sa.type == SpecialArea::TYPE_TRIANGULATION)
        {
            if (Overlap::Point_Polygon2D(pos, sa.GetPolygon(), &sa.GetAABB()))
            {
                return true;
            }
            foundOne = true;
        }
    }
    return !foundOne;
}


//===================================================================
// DoesShapeSelfIntersect
//===================================================================
bool DoesShapeSelfIntersect(const ShapePointContainer& shape, Vec3& badPt,
    std::vector<CNavigation::SValidationErrorMarker>& validationErrorMarkers)
{
    ShapePointContainer::const_iterator liend = shape.end();

    // Check for degenerate edges.
    for (ShapePointContainer::const_iterator li = shape.begin(); li != liend; ++li)
    {
        ShapePointContainer::const_iterator linext = li;
        ++linext;
        if (linext == liend)
        {
            linext = shape.begin();
        }

        if (IsEquivalent2D((*li), (*linext), 0.001f))
        {
            badPt = *li;

            char msg[256];
            azsnprintf(msg, 256, "Degenerate edge\n(%.f, %.f, %.f).", badPt.x, badPt.y, badPt.z);
            validationErrorMarkers.push_back(CNavigation::SValidationErrorMarker(msg, badPt, ColorB(255, 0, 0)));

            return true;
        }
    }

    for (ShapePointContainer::const_iterator li = shape.begin(); li != liend; ++li)
    {
        ShapePointContainer::const_iterator linext = li;
        ++linext;
        if (linext == liend)
        {
            linext = shape.begin();
        }

        // check for self-intersection with all remaining segments
        int posIndex1 = 0;
        ShapePointContainer::const_iterator li1, linext1;
        for (li1 = shape.begin(); li1 != liend; ++li1, ++posIndex1)
        {
            linext1 = li1;
            ++linext1;
            if (linext1 == liend)
            {
                linext1 = shape.begin();
            }

            if (li1 == linext)
            {
                continue;
            }
            if (linext1 == li)
            {
                continue;
            }
            if (li1 == li)
            {
                continue;
            }
            real s, t;
            if (Intersect::Lineseg_Lineseg2D(Linesegr(*li, *linext), Linesegr(*li1, *linext1), s, t))
            {
                badPt = Lineseg(*li, *linext).GetPoint(s);

                char msg[256];
                azsnprintf(msg, 256, "Self-intersection\n(%.f, %.f, %.f)", badPt.x, badPt.y, badPt.z);
                validationErrorMarkers.push_back(CNavigation::SValidationErrorMarker(msg, badPt, ColorB(255, 0, 0)));

                return true;
            }
        }
    }
    return false;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::IntersectsForbidden(const Vec3& start, const Vec3& end, Vec3& closestPoint,
    const string* nameToSkip, Vec3* pNormal, INavigation::EIFMode mode, bool bForceNormalOutwards) const
{
    FUNCTION_PROFILER(m_pSystem, PROFILE_AI);

    if (m_forbiddenAreas.GetShapes().empty() && m_forbiddenBoundaries.GetShapes().empty())
    {
        return false;
    }

    switch (mode)
    {
    case INavigation::IF_AREASBOUNDARIES:
        if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal))
        {
            return true;
        }
        if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
        {
            return true;
        }
        break;
    case INavigation::IF_AREAS: // areas only
        if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal))
        {
            return true;
        }
        break;
    case INavigation::IF_BOUNDARIES: // boundaries only
        if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
        {
            return true;
        }
        break;
    default:
        AIError("Bad mode %d passed to CAISystem::IntersectsForbidden", mode);
        if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
        {
            return true;
        }
        if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
        {
            return true;
        }
        break;
    }
    return false;
}

// TODO Jan 31, 2008: <pvl> looks unused
//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::IntersectsSpecialArea(const Vec3& vStart, const Vec3& vEnd, Vec3& vClosestPoint)
{
    if (m_mapSpecialAreas.empty())
    {
        return false;
    }

    Lineseg lineseg(vStart, vEnd);
    SpecialAreaMap::iterator iend = m_mapSpecialAreas.end();
    for (SpecialAreaMap::iterator fi = m_mapSpecialAreas.begin(); fi != iend; ++fi)
    {
        Vec3 result;
        if (Overlap::Lineseg_AABB2D(lineseg, fi->second.GetAABB()))
        {
            if (Intersect::Lineseg_Polygon2D(lineseg, fi->second.GetPolygon(), result))
            {
                vClosestPoint = result;
                return true;
            }
        }
    }
    return false;
}

//===================================================================
// ClearForbiddenQuadTrees
//===================================================================
void CNavigation::ClearForbiddenQuadTrees()
{
    m_forbiddenAreas.ClearQuadTree();
    m_forbiddenBoundaries.ClearQuadTree();
    m_designerForbiddenAreas.ClearQuadTree();
}

//===================================================================
// RebuildForbiddenQuadTrees
//===================================================================
void CNavigation::RebuildForbiddenQuadTrees()
{
    ClearForbiddenQuadTrees();

    m_forbiddenAreas.BuildBins();
    m_forbiddenBoundaries.BuildBins();
    m_designerForbiddenAreas.BuildBins();

    if (useForbiddenQuadTree)
    {
        m_forbiddenAreas.BuildQuadTree(8, 20.0f);
        m_forbiddenBoundaries.BuildQuadTree(8, 20.0f);
        m_designerForbiddenAreas.BuildQuadTree(8, 20.0f);
    }
}

//-----------------------------------------------------------------------------------------------------------
bool CNavigation::IsPointOnForbiddenEdge(const Vec3& pos, float tol, Vec3* pNormal,
    const CAIShape** ppShape, bool checkAutoGenRegions)
{
    FUNCTION_PROFILER(m_pSystem, PROFILE_AI);

    if (checkAutoGenRegions)
    {
        if (m_forbiddenAreas.IsPointOnEdge(pos, tol, ppShape))
        {
            return true;
        }
    }
    else
    {
        if (m_designerForbiddenAreas.IsPointOnEdge(pos, tol, ppShape))
        {
            return true;
        }
    }

    if (m_forbiddenBoundaries.IsPointOnEdge(pos, tol, ppShape))
    {
        return true;
    }

    return false;
}

//====================================================================
// IsPointInForbiddenBoundary
//====================================================================
bool CNavigation::IsPointInForbiddenBoundary(const Vec3& pos, const CAIShape** ppShape) const
{
    FUNCTION_PROFILER(m_pSystem, PROFILE_AI);

    const CAIShape* pHitShape = 0;
    unsigned hits = m_forbiddenBoundaries.IsPointInsideCount(pos, &pHitShape);

    if ((hits % 2) != 0)
    {
        if (ppShape)
        {
            *ppShape = pHitShape;
        }
        return true;
    }
    return false;
}

//====================================================================
// IsPointForbidden
//====================================================================
bool CNavigation::IsPointForbidden(const Vec3& pos, float tol, Vec3* pNormal, const CAIShape** ppShape)
{
    if (IsPointInForbiddenRegion(pos, ppShape, true))
    {
        return true;
    }
    return IsPointOnForbiddenEdge(pos, tol, pNormal, ppShape);
}

//====================================================================
// IsPathForbidden
//====================================================================
bool CNavigation::IsPathForbidden(const Vec3& start, const Vec3& end)
{
    FUNCTION_PROFILER(m_pSystem, PROFILE_AI);

    // do areas and boundaries separately

    // for boundaries get a list of each of the boundaries the start and end
    // are in. Path is forbidden if the lists are different.
    std::set<const void*> startBoundaries;
    std::set<const void*> endBoundaries;

    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
        if (pShape->IsPointInside(start))
        {
            startBoundaries.insert(pShape);
        }
    }

    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
        if (pShape->IsPointInside(end))
        {
            endBoundaries.insert(pShape);
        }
    }

    if (startBoundaries != endBoundaries)
    {
        return true;
    }

    // boundaries are not a problem. Areas are trikier because we can
    // always go from a forbidden area into a not-forbidden area, but not
    // the other way. Simplify things by assuming they are only nested a bit
    // - i.e. there are no more than two forbidden areas outside a point.
    std::set<const void*> startAreas;
    std::set<const void*> endAreas;

    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
        if (pShape->IsPointInside(start))
        {
            startAreas.insert(pShape);
        }
    }

    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
        if (pShape->IsPointInside(end))
        {
            endAreas.insert(pShape);
        }
    }

    // start and end completely clear.
    if (startAreas.empty() && endAreas.empty())
    {
        return false;
    }

    // start and end in the same
    if (startAreas == endAreas)
    {
        return false;
    }

    // going from nested-but-valid to not-nested-but-valid is forbidden
    if (startAreas.size() > endAreas.size() + 1)
    {
        return true;
    }

    // check for transition from valid to forbidden
    bool startInForbidden = (startAreas.size() % 2) != 0;
    bool endInForbidden = (endAreas.size() % 2) != 0;
    if (!startInForbidden && endInForbidden)
    {
        return true;
    }

    // This isn't foolproof but it is OK in most sensible situations!
    return false;
}


// parses ai information into file
//
//-----------------------------------------------------------------------------------------------------------
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void CNavigation::ParseIntoFile(CGraph* pGraph, bool bForbidden)
{
    CTimeValue absoluteStartTime = gEnv->pTimer->GetAsyncCurTime();

    for (unsigned int index = 0; index < (int)m_vTriangles.size(); ++index)
    {
        Tri* tri = m_vTriangles[index];
        // make vertices know which triangles contain them
        m_vVertices[tri->v[0]].m_lstTris.push_back(index);
        m_vVertices[tri->v[1]].m_lstTris.push_back(index);
        m_vVertices[tri->v[2]].m_lstTris.push_back(index);
    }

    Vec3 tbbmin, tbbmax;
    tbbmin(m_pTriangulator->m_vtxBBoxMin.x, m_pTriangulator->m_vtxBBoxMin.y, m_pTriangulator->m_vtxBBoxMin.z);
    tbbmax(m_pTriangulator->m_vtxBBoxMax.x, m_pTriangulator->m_vtxBBoxMax.y, m_pTriangulator->m_vtxBBoxMax.z);

    pGraph->SetBBox(tbbmin, tbbmax);

    //I3DEngine *pEngine = m_pSystem->GetI3DEngine();
    unsigned int cnt = 0;
    std::vector<Tri*>::iterator ti;
    //for ( unsigned int i=0;i<m_vTriangles.size();++i)

    for (ti = m_vTriangles.begin(); ti != m_vTriangles.end(); ++ti, ++cnt)
    {
        //      Tri *tri = m_vTriangles[i];
        Tri* tri = (*ti);

        if (!tri->graphNodeIndex)
        {
            // create node for this tri
            unsigned nodeIndex = pGraph->CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3(ZERO));
            tri->graphNodeIndex = nodeIndex;
        }

        Vtx* v1, * v2, * v3;
        v1 = &m_vVertices[tri->v[0]];
        v2 = &m_vVertices[tri->v[1]];
        v3 = &m_vVertices[tri->v[2]];

        GraphNode* pNode = pGraph->GetNodeManager().GetNode(tri->graphNodeIndex);

        // add the triangle vertices... for outdoors only
        ObstacleData odata;
        odata.vPos = Vec3(v1->x, v1->y, v1->z);
        odata.SetCollidable(v1->bCollidable);
        odata.SetHideable(v1->bHideable);
        pNode->GetTriangularNavData()->vertices.push_back(GetVertexList().AddVertex(odata));
        odata.vPos = Vec3(v2->x, v2->y, v2->z);
        odata.SetCollidable(v2->bCollidable);
        odata.SetHideable(v2->bHideable);
        pNode->GetTriangularNavData()->vertices.push_back(GetVertexList().AddVertex(odata));
        odata.vPos = Vec3(v3->x, v3->y, v3->z);
        odata.SetCollidable(v3->bCollidable);
        odata.SetHideable(v3->bHideable);
        pNode->GetTriangularNavData()->vertices.push_back(GetVertexList().AddVertex(odata));

        pGraph->FillGraphNodeData(tri->graphNodeIndex);

        // test first edge
        std::vector<int>::iterator u, v;
        for (u = v1->m_lstTris.begin(); u != v1->m_lstTris.end(); ++u)
        {
            for (v = v2->m_lstTris.begin(); v != v2->m_lstTris.end(); ++v)
            {
                int au, av;
                au = (*u);
                av = (*v);
                if (au == av && au != cnt)
                {
                    Tri* other = m_vTriangles[au];
                    if (!other->graphNodeIndex)
                    {
                        // create node for this tri
                        unsigned nodeIndex = pGraph->CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3(ZERO));
                        other->graphNodeIndex = nodeIndex;
                    }
                    if (-1 == pGraph->GetNodeManager().GetNode(tri->graphNodeIndex)->GetLinkIndex(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(other->graphNodeIndex)))
                    {
                        pGraph->Connect(tri->graphNodeIndex, other->graphNodeIndex);
                    }
                    pGraph->ResolveLinkData(pGraph->GetNodeManager().GetNode(tri->graphNodeIndex), pGraph->GetNodeManager().GetNode(other->graphNodeIndex));
                }
            }
        }

        for (u = v2->m_lstTris.begin(); u != v2->m_lstTris.end(); ++u)
        {
            for (v = v3->m_lstTris.begin(); v != v3->m_lstTris.end(); ++v)
            {
                int au, av;
                au = (*u);
                av = (*v);
                if (au == av && au != cnt)
                {
                    Tri* other = m_vTriangles[au];
                    if (!other->graphNodeIndex)
                    {
                        // create node for this tri
                        unsigned nodeIndex = pGraph->CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3(ZERO));
                        other->graphNodeIndex = nodeIndex;
                    }
                    if (-1 == pGraph->GetNodeManager().GetNode(tri->graphNodeIndex)->GetLinkIndex(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(other->graphNodeIndex)))
                    {
                        pGraph->Connect(tri->graphNodeIndex, other->graphNodeIndex);
                    }
                    pGraph->ResolveLinkData(pGraph->GetNodeManager().GetNode(tri->graphNodeIndex), pGraph->GetNodeManager().GetNode(other->graphNodeIndex));
                }
            }
        }

        for (u = v3->m_lstTris.begin(); u != v3->m_lstTris.end(); ++u)
        {
            for (v = v1->m_lstTris.begin(); v != v1->m_lstTris.end(); ++v)
            {
                int au, av;
                au = (*u);
                av = (*v);
                if (au == av && au != cnt)
                {
                    Tri* other = m_vTriangles[au];
                    if (!other->graphNodeIndex)
                    {
                        // create node for this tri
                        unsigned nodeIndex = pGraph->CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3(ZERO));
                        other->graphNodeIndex = nodeIndex;
                    }
                    if (-1 == pGraph->GetNodeManager().GetNode(tri->graphNodeIndex)->GetLinkIndex(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(other->graphNodeIndex)))
                    {
                        pGraph->Connect(tri->graphNodeIndex, other->graphNodeIndex);
                    }
                    pGraph->ResolveLinkData(pGraph->GetNodeManager().GetNode(tri->graphNodeIndex), pGraph->GetNodeManager().GetNode(other->graphNodeIndex));
                }
            }
        }
    }

    for (ti = m_vTriangles.begin(); ti != m_vTriangles.end(); ++ti)
    {
        GraphNode* pCurrent = (GraphNode*) pGraph->GetNodeManager().GetNode((*ti)->graphNodeIndex);
        for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
        {
            pGraph->ResolveLinkData(pCurrent, pGraph->GetNodeManager().GetNode(pGraph->GetLinkManager().GetNextNode(link)));
        }
    }

    pGraph->Validate("CAISystem::ParseIntoFile before adding forbidden", false);

    if (bForbidden)
    {
        AILogProgress(" Checking forbidden area validity.");
        if (!ValidateAreas())
        {
            AIError("Problems with AI areas - aborting triangulation [Design bug]");
            return;
        }

        AILogProgress("Adding forbidden areas.");
        AddForbiddenAreas();

        AILogProgress("Calculating link properties.");
        CalculateLinkProperties();

        AILogProgress("Disabling thin triangles near forbidden");
        DisableThinNodesNearForbidden();

        AILogProgress("Marking forbidden triangles");
        MarkForbiddenTriangles();
    }

    // Get the vertex/obstacle radius from vegetation data for soft cover objects.
    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();
    for (int i = 0; i < GetVertexList().GetSize(); ++i)
    {
        ObstacleData&   vert = GetVertexList().ModifyVertex(i);
        Vec3 vPos = vert.vPos;
        vPos.z = p3DEngine->GetTerrainElevation(vPos.x, vPos.y);

        Vec3 bboxsize(1.f, 1.f, 1.f);
        IPhysicalEntity* pPhys = 0;

        IPhysicalEntity** pEntityList;
        int nCount = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(vPos - bboxsize, vPos + bboxsize, pEntityList, ent_static);

        int j = 0;
        while (j < nCount)
        {
            pe_status_pos ppos;
            pEntityList[j]->GetStatus(&ppos);
            ppos.pos.z = vPos.z;
            if (IsEquivalent(ppos.pos, vPos, 0.001f))
            {
                pPhys = pEntityList[j];
                break;
            }
            ++j;
        }

        if (!pPhys)
        {
            continue;
        }

        float overrideRadius = -1.0f;
        IRenderNode* pRN = (IRenderNode*)pPhys->GetForeignData(PHYS_FOREIGN_ID_STATIC);
        if (pRN)
        {
            IStatObj* pStatObj = pRN->GetEntityStatObj();
            if (pStatObj)
            {
                pe_status_pos status;
                status.ipart = 0;
                pPhys->GetStatus(&status);
                float r = pStatObj->GetAIVegetationRadius();
                if (r > 0.0f)
                {
                    overrideRadius = r * status.scale;
                }
                else
                {
                    overrideRadius = -2.0f;
                }
                vert.SetApproxHeight((status.pos.z + status.BBox[1].z) - vert.vPos.z);
            }
        }
        vert.fApproxRadius = max(vert.fApproxRadius, overrideRadius);
    }

    pGraph->Validate("End of CAISystem::ParseIntoFile", false);

    CTimeValue absoluteEndTime = gEnv->pTimer->GetAsyncCurTime();

    AILogProgress("Finished CAISystem::ParseIntoFile in %f seconds", (absoluteEndTime - absoluteStartTime).GetSeconds());
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif

//====================================================================
// AddBeachPointsToTriangulator
//====================================================================
void CNavigation::AddBeachPointsToTriangulator(const AABB& worldAABB)
{
    std::vector<ListPositions> areas;
    AABB aabb(AABB::RESET);
    for (SpecialAreaMap::iterator it = m_mapSpecialAreas.begin(); it != m_mapSpecialAreas.end(); ++it)
    {
        if (it->second.type == SpecialArea::TYPE_WATER)
        {
            const ListPositions& pts = it->second.GetPolygon();
            areas.push_back(pts);
            for (ListPositions::const_iterator ptIt = pts.begin(); ptIt != pts.end(); ++ptIt)
            {
                Vec3 pt = *ptIt;
                aabb.Add(pt);
            }
        }
    }

    if (areas.empty())
    {
        return;
    }

    int terrainMinX = (int) aabb.min.x;
    int terrainMinY = (int) aabb.min.y;
    int terrainMaxX = (int) aabb.max.x;
    int terrainMaxY = (int) aabb.max.y;

    static float criticalDepth = 0.1f;
    static float criticalDeepDepth = 5.0f;
    static unsigned delta = 4;
    I3DEngine* pEngine = m_pSystem->GetI3DEngine();

    int terrainArraySize = pEngine->GetTerrainSize();
    unsigned edge = 16;

    Limit(terrainMinX, 0, terrainArraySize);
    Limit(terrainMinY, 0, terrainArraySize);
    Limit(terrainMaxX, 0, terrainArraySize);
    Limit(terrainMaxY, 0, terrainArraySize);

    for (int ix = terrainMinX; ix + delta < terrainMaxX; ix += delta)
    {
        for (int iy = terrainMinY; iy + delta < terrainMaxY; iy += delta)
        {
            Vec3 v00(ix, iy, 0.0f);
            Vec3 v10(ix + delta, iy, 0.0f);
            Vec3 v01(ix, iy + delta, 0.0f);
            v00.z = pEngine->GetTerrainElevation(v00.x, v00.y);
            v10.z = pEngine->GetTerrainElevation(v10.x, v00.y);
            v01.z = pEngine->GetTerrainElevation(v01.x, v00.y);
            float water00Z = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(v00) : pEngine->GetWaterLevel(&v00);
            float water10Z = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(v10) : pEngine->GetWaterLevel(&v10);
            float water01Z = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(v01) : pEngine->GetWaterLevel(&v01);
            float depth00 = water00Z - v00.z;
            float depth10 = water10Z - v10.z;
            float depth01 = water01Z - v01.z;
            if ((depth00 > criticalDepth) != (depth10 > criticalDepth))
            {
                Vec3 pt = 0.5f * (v00 + v10);
                if (worldAABB.IsContainSphere(pt, 1.0f) && ::IsPointInWaterAreas(pt, areas) && IsPointInTriangulationAreas(pt))
                {
                    m_pTriangulator->AddVertex(pt.x, pt.y, pt.z, false, false);
                }
            }
            if ((depth00 > criticalDepth) != (depth01 > criticalDepth))
            {
                Vec3 pt = 0.5f * (v00 + v01);
                if (worldAABB.IsContainSphere(pt, 1.0f) && ::IsPointInWaterAreas(pt, areas) && IsPointInTriangulationAreas(pt))
                {
                    m_pTriangulator->AddVertex(pt.x, pt.y, pt.z, false, false);
                }
            }
        }
    }

    // also add points in the main water body
    static int waterDelta = 100;
    for (unsigned ix = terrainMinX; ix + waterDelta < terrainMaxX; ix += waterDelta)
    {
        for (unsigned iy = terrainMinY; iy + waterDelta < terrainMaxY; iy += waterDelta)
        {
            Vec3 v(ix, iy, 0.0f);
            v.z = pEngine->GetTerrainElevation(v.x, v.y);
            float waterZ = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(v) : pEngine->GetWaterLevel(&v);
            float depth = waterZ - v.z;
            if (depth > criticalDeepDepth)
            {
                if (worldAABB.IsContainSphere(v, 1.0f) && ::IsPointInWaterAreas(v, areas) && IsPointInTriangulationAreas(v))
                {
                    m_pTriangulator->AddVertex(v.x, v.y, v.z, false, false);
                }
            }
        }
    }
}


//====================================================================
// GenerateTriangulation
// generate the triangulation for this level and mission
//====================================================================
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void CNavigation::GenerateTriangulation(const char* szLevel, const char* szMission)
{
    if (!szLevel || !szMission)
    {
        return;
    }

    CGraph* pGraph = GetGraph();

    float absoluteStartTime = gEnv->pTimer->GetAsyncCurTime();

    char fileNameAreas[1024], fileNameVerts[1024];
    sprintf_s(fileNameAreas, "%s/areas%s.bai", szLevel, szMission);
    sprintf_s(fileNameVerts, "%s/verts%s.bai", szLevel, szMission);

    AILogProgress("Clearing old graph data...");
    pGraph->Clear(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE);
    pGraph->CheckForEmpty(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE);

    ClearForbiddenQuadTrees();

    AILogProgress("Generating triangulation: forbidden boundaries:");
    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
        AILogProgress("%s, %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
    }

    AILogProgress("Generating triangulation: designer-created forbidden areas:");
    for (unsigned i = 0, ni = m_designerForbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_designerForbiddenAreas.GetShapes()[i];
        AILogProgress("%s, %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
    }

    if (!CalculateForbiddenAreas())
    {
        return;
    }

    AILogProgress("Generating triangulation: forbidden areas:");
    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
        AILogProgress("%s, %lu points", pShape->GetName().c_str(), pShape->GetPoints().size());
    }

    AILogProgress("Building forbidden area QuadTrees");
    RebuildForbiddenQuadTrees();
    //  m_forbiddenAreasQuadTree.Dump("m_forbiddenAreasQuadTree");
    //  m_designerForbiddenAreasQuadTree.Dump("m_designerForbiddenAreasQuadTree");
    //  m_forbiddenBoundariesQuadTree.Dump("m_forbiddenBoundariesQuadTree");
    AILogProgress("Finished building forbidden area QuadTrees");

    GetVertexList().Clear();

    Vec3 min, max;
    float fTSize = (float) m_pSystem->GetI3DEngine()->GetTerrainSize();
    AILogProgress(" Triangulation started. Terrain size = %.0f", fTSize);

    min.Set(0, 0, -5000);
    max.Set(fTSize, fTSize, 5000.0f);

    if (m_pTriangulator)
    {
        delete m_pTriangulator;
    }
    m_pTriangulator = new CTriangulator();

    // get only static physical entities (trees, buildings etc...)
    IPhysicalEntity** pObstacles;
    int flags = ent_static;
    if (m_cvIncludeNonColEntitiesInNavigation->GetIVal() == 0)
    {
        flags |= ent_ignore_noncolliding;
    }
    int count = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(min, max, pObstacles, flags);

    m_pTriangulator->m_vVertices.reserve(count);

    AILogProgress("Processing >>> %d objects >>> box size [%.0f x %.0f %.0f]", count, max.x, max.y, max.z);

    AABB worldAABB(min, max);

    m_pTriangulator->AddVertex(0.0f, 0.0f, 0.1f, false, false);
    m_pTriangulator->AddVertex(fTSize, 0.0f, 0.1f, false, false);
    m_pTriangulator->AddVertex(fTSize, fTSize, 0.1f, false, false);
    m_pTriangulator->AddVertex(0.0f, fTSize, 0.1f, false, false);

    if (!m_pTriangulator->PrepForTriangulation())
    {
        return;
    }

    I3DEngine* pEngine = m_pSystem->GetI3DEngine();
    int vertexCounter(0);

    for (int i = 0; i < count; ++i)
    {
        IPhysicalEntity* pCurrent = pObstacles[i];

        // don't add entities (only triangulate brush-like objects, and brush-like
        // objects should not be entities!)
        IEntity* pEntity = (IEntity*) pCurrent->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
        if (pEntity)
        {
            continue;
        }

        pe_params_foreign_data pfd;
        if (pCurrent->GetParams(&pfd) == 0)
        {
            continue;
        }

        pe_status_pos sp;
        sp.ipart = -1;
        sp.partid = -1;
        if (pCurrent->GetStatus(&sp) == 0)
        {
            continue;
        }

        pe_status_pos status;
        if (pCurrent->GetStatus(&status) == 0)
        {
            continue;
        }

        Vec3 obstaclePos = status.pos;

        if (!worldAABB.IsContainSphere(obstaclePos, 1.0f))
        {
            continue;
        }

        // if too flat and too close to the terrain
        float zpos = pEngine->GetTerrainElevation(obstaclePos.x, obstaclePos.y);
        float ftop = status.pos.z + status.BBox[1].z;

        if (zpos > ftop)  // skip underground stuff
        {
            continue;
        }
        if (fabs(ftop - zpos) < 0.4f) // skip stuff too close to the terrain
        {
            continue;
        }

        bool collideable = true;
        bool hideable = (pfd.iForeignFlags & PFF_HIDABLE) != 0 || (pfd.iForeignFlags & PFF_HIDABLE_SECONDARY) != 0;

        // Only include high non-collidables.
        if ((sp.flagsOR & geom_colltype_solid) == 0)
        {
            static float criticalZ = 0.9f;
            if (fabs(ftop - zpos) < criticalZ)
            {
                continue;
            }

            IRenderNode* pRN = (IRenderNode*)pCurrent->GetForeignData(PHYS_FOREIGN_ID_STATIC);
            if (pRN)
            {
                IStatObj* pStatObj = pRN->GetEntityStatObj();
                if (pStatObj)
                {
                    float r = pStatObj->GetAIVegetationRadius();
                    if (r < 0.0f)
                    {
                        continue;
                    }
                }
            }
            collideable = false;
        }

        // If the object should be excluded but it is hideable, add the object is non-collidable cover.
        if ((pfd.iForeignFlags & PFF_EXCLUDE_FROM_STATIC) != 0)
        {
            if (!hideable)
            {
                continue;
            }
            collideable = false;
        }

        // don't triangulate objects that your feet wouldn't touch when walking in water
        float waterZ = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(obstaclePos) : pEngine->GetWaterLevel(&obstaclePos, 0);
        if (ftop < (waterZ - 1.5f))
        {
            continue;
        }

        IVisArea* pArea;
        int buildingID;
        if (CheckNavigationType(obstaclePos, buildingID, pArea, IAISystem::NAV_WAYPOINT_HUMAN) == IAISystem::NAV_WAYPOINT_HUMAN)
        {
            continue;
        }

        if (!IsPointInTriangulationAreas(obstaclePos))
        {
            continue;
        }

        // [Mikko] Looser forbidden area check for hideable obstacles.
        // Don't discard hide points if they're inside auto-generated forbidden...
        // Note that this might cause problems finding hide spots
        if (IsPointInForbiddenRegion(obstaclePos, !hideable))
        {
            continue;
        }

        static float edgeTol = 0.3f;
        if (IsPointOnForbiddenEdge(obstaclePos, edgeTol))
        {
            continue;
        }

        m_pTriangulator->AddVertex(obstaclePos.x, obstaclePos.y, obstaclePos.z, collideable, hideable);
        ++vertexCounter;
    }

    AILogProgress("Processing >>> %d vertises added  >>> ", vertexCounter);
    vertexCounter = 0;

    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
        const ShapePointContainer& pts = pShape->GetPoints();
        for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
        {
            const Vec3& pos = pts[j];
            if (worldAABB.IsContainSphere(pos, 1.0f))
            {
                m_pTriangulator->AddVertex(pos.x, pos.y, pos.z, true, false);
            }
        }
    }

    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
        const ShapePointContainer& pts = pShape->GetPoints();
        for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
        {
            const Vec3& pos = pts[j];
            if (worldAABB.IsContainSphere(pos, 1.0f))
            {
                m_pTriangulator->AddVertex(pos.x, pos.y, pos.z, true, false);
            }
        }
    }

    // Add some extra points along the beaches to help divide land from sea
    AILogProgress("Adding beach points");
    AddBeachPointsToTriangulator(worldAABB);

    AILogProgress("Starting basic triangulation");

    if (!m_pTriangulator->TriangulateNew())
    {
        return;
    }

    TARRAY lstTriangles = m_pTriangulator->GetTriangles();
    m_vVertices = m_pTriangulator->GetVertices();
    m_vTriangles.clear();
    // INTEGRATION Kire: Replaced PushFront with explicit loop:
    // (MATT) Should be due to STL bug that was fixed. Try switching this back. {2008/07/20}
    for (TARRAY::iterator itx = lstTriangles.begin(); itx != lstTriangles.end(); ++itx)
    {
        Tri* pTri = *itx;
        m_vTriangles.push_back(pTri);
    }
    //PushFront(m_vTriangles,lstTriangles);

    AILogProgress(" Creation of graph and parsing into file.");
    ParseIntoFile(pGraph, true);

    if (!m_vTriangles.empty())
    {
        std::vector<Tri*>::iterator ti;
        for (ti = m_vTriangles.begin(); ti != m_vTriangles.end(); ++ti)
        {
            Tri* tri = (*ti);
            delete tri;
        }
        m_vTriangles.clear();
    }

    m_vVertices.clear();

    pGraph->Validate("CAISystem::GenerateTriangulation after writing nav graph to file", true);
    if (!pGraph->mBadGraphData.empty())
    {
        AIWarning("CAISystem: To remove \"invalid passable\" warnings in nav graph, enable \"ai_DebugDraw 72\", disable vegetation etc, and tweak the forbidden region areas close to where you see yellow circles");
    }

    if (!m_vTriangles.empty())
    {
        std::vector<Tri*>::iterator ti;
        for (ti = m_vTriangles.begin(); ti != m_vTriangles.end(); ++ti)
        {
            Tri* tri = (*ti);
            delete tri;
        }
        m_vTriangles.clear();
    }
    m_vVertices.clear();

    GetVertexList().WriteToFile(fileNameVerts);

    delete m_pTriangulator;
    m_pTriangulator = 0;

    m_nNumBuildings = 0;

    ValidateBigObstacles();

    float endTime = gEnv->pTimer->GetAsyncCurTime();
    AILogProgress(" Triangulation finished in %6.1f seconds.", endTime - absoluteStartTime);

    m_navDataState = NDS_OK;
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif

bool doForbiddenDebug = false;

//
//-----------------------------------------------------------------------------------------------------------
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void CNavigation::AddForbiddenArea(CAIShape* pShape)
{
    CGraph* pGraph = GetGraph();

    ListNodeIds lstNewNodes;
    ListNodeIds lstOldNodes;
    NewCutsVector   newCutsVector;

    const ShapePointContainer& pts = pShape->GetPoints();
    for (unsigned i = 0, ni = pts.size(); i < ni; ++i)
    {
        ++debugForbiddenCounter;
        Vec3r vStart = pts[i];

        lstNewNodes.clear();
        lstOldNodes.clear();

        newCutsVector.resize(0);
        // create edge
        Vec3r vEnd = pts[(i + 1) % ni];
        vEnd.z = vStart.z = 0;

        ListNodeIds nodesToRefine;
        CreatePossibleCutList(vStart, vEnd, nodesToRefine);

        if (doForbiddenDebug)
        {
            AILogAlways("%d: CreatePossibleCutList generates %lu", debugForbiddenCounter, nodesToRefine.size());
        }

        for (ListNodeIds::iterator nI = nodesToRefine.begin(); nI != nodesToRefine.end(); ++nI)
        {
            RefineTriangle(pGraph->GetNodeManager(), pGraph->GetLinkManager(), (*nI), vStart, vEnd, lstNewNodes, lstOldNodes, newCutsVector);
        }

        if (doForbiddenDebug)
        {
            AILogAlways("%d: RefineTriangle generates %lu %lu %lu", debugForbiddenCounter, lstNewNodes.size(), lstOldNodes.size(), newCutsVector.size());
        }

        static int cutOff = 1;
        if (newCutsVector.size() == 1 && lstNewNodes.size() < cutOff)  // nothing was really changed
        {
            continue;
        }

        pGraph->ClearTags();

        ListNodeIds::iterator di;

        // Danny - to be honest I don't understand exactly how these lists
        // are used, but I found that sometimes the same node could be in both
        // the old and new lists. During Disconnect the node could get invalidated,
        // resulting in much badness - crashes and incorrect graph. So, if it's
        // in both lists removing it from the "old" list seems to work and generates
        // a correct graph
        unsigned origOldSize = lstOldNodes.size();
        for (di = lstNewNodes.begin(); di != lstNewNodes.end(); ++di)
        {
            lstOldNodes.remove(*di);
        }
        unsigned newOldSize = lstOldNodes.size();

        for (di = lstNewNodes.begin(); di != lstNewNodes.end(); )
        {
            GraphNode* pNode = pGraph->GetNodeManager().GetNode(*di);
            if (pNode->navType != IAISystem::NAV_TRIANGULAR)
            {
                di = lstNewNodes.erase(di); // should never happen - but the safe first node (NAV_UNSET) was getting in?
            }
            else
            {
                ++di;
            }
        }

        // delete old triangles
        for (di = lstOldNodes.begin(); di != lstOldNodes.end(); ++di)
        {
            pGraph->Disconnect((*di));
        }

        pGraph->ConnectNodes(lstNewNodes);
    }
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif


// for point sorting in CalculateForbiddenAreas
static const float vec3SortTol = 0.001f;
static inline bool operator<(const Vec3& lhs, const Vec3& rhs)
{
    if (lhs.x < rhs.x - vec3SortTol)
    {
        return true;
    }
    else if (lhs.x > rhs.x + vec3SortTol)
    {
        return false;
    }
    if (lhs.y < rhs.y - vec3SortTol)
    {
        return true;
    }
    else if (lhs.y > rhs.y + vec3SortTol)
    {
        return false;
    }
    return false; // equal
}
static inline bool Vec3Equivalent2D(const Vec3& lhs, const Vec3& rhs)
{
    return IsEquivalent2D(lhs, rhs, vec3SortTol);
}

//===================================================================
// CalculateForbiddenAreas
//===================================================================
bool CNavigation::CalculateForbiddenAreas()
{
    float criticalRadius = m_cvRadiusForAutoForbidden->GetFVal();

    m_forbiddenAreas.Copy(m_designerForbiddenAreas);
    forbiddenNameCounter = 0;

    if (ForbiddenAreasOverlap())
    {
        AIError("Designer forbidden areas overlap - aborting triangulation");
        return false;
    }

    if (criticalRadius >= 100.0f)
    {
        return true;
    }

    // get all the objects that need code-generated forbidden areas
    std::vector<Sphere> extraObjects;

    Vec3 min, max;
    float fTSize = (float) m_pSystem->GetI3DEngine()->GetTerrainSize();
    min.Set(0, 0, -5000);
    max.Set(fTSize, fTSize, 5000.0f);

    IPhysicalEntity** pObstacles;
    int count = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(min, max, pObstacles, ent_static | ent_ignore_noncolliding);

    I3DEngine* pEngine = m_pSystem->GetI3DEngine();

    CAIShapeContainer extraAreas;

    int iEO = 0;
    for (int i = 0; i < count; ++i)
    {
        IPhysicalEntity* pCurrent = pObstacles[i];

        IRenderNode* pRN = (IRenderNode*)pCurrent->GetForeignData(PHYS_FOREIGN_ID_STATIC);
        if (pRN)
        {
            IStatObj* pStatObj = pRN->GetEntityStatObj();
            if (pStatObj)
            {
                float vegRad = pStatObj->GetAIVegetationRadius();
                pe_status_pos status;
                status.ipart = 0;
                pCurrent->GetStatus(&status);

                if (!IsPointInTriangulationAreas(status.pos))
                {
                    continue;
                }

                if (vegRad > 0.0f)
                {
                    vegRad *= status.scale;
                }

                if (vegRad >= criticalRadius)
                {
                    // use the convex hull - first get the points
                    Vec3 obstPos = status.pos;
                    Matrix33 obstMat = Matrix33(status.q);
                    obstMat *= status.scale;

                    static float criticalMinAlt = 0.5f;
                    static float criticalMaxAlt = 1.8f;

                    static std::vector<Vec3>    vertices;
                    vertices.resize(0);

                    // add all parts
                    pe_status_nparts statusNParts;
                    int nParts = pCurrent->GetStatus(&statusNParts);

                    pe_status_pos statusPos;
                    pe_params_part paramsPart;
                    for (statusPos.ipart = 0, paramsPart.ipart = 0; statusPos.ipart < nParts; ++statusPos.ipart, ++paramsPart.ipart)
                    {
                        if (!pCurrent->GetParams(&paramsPart) || !pCurrent->GetStatus(&statusPos))
                        {
                            continue;
                        }

                        Vec3 partPos = statusPos.pos;
                        Matrix33 partMat = Matrix33(statusPos.q);
                        partMat *= statusPos.scale;

                        IGeometry* geom = statusPos.pGeom;
                        const primitives::primitive* prim = geom->GetData();
                        int type = geom->GetType();
                        switch (type)
                        {
                        case GEOM_BOX:
                        {
                            const primitives::box* bbox = static_cast<const primitives::box*>(prim);
                            Matrix33 rot = bbox->Basis.T();

                            Vec3    boxVerts[8];
                            boxVerts[0] = bbox->center + rot * Vec3(-bbox->size.x, -bbox->size.y, -bbox->size.z);
                            boxVerts[1] = bbox->center + rot * Vec3(bbox->size.x, -bbox->size.y, -bbox->size.z);
                            boxVerts[2] = bbox->center + rot * Vec3(bbox->size.x, bbox->size.y, -bbox->size.z);
                            boxVerts[3] = bbox->center + rot * Vec3(-bbox->size.x, bbox->size.y, -bbox->size.z);
                            boxVerts[4] = bbox->center + rot * Vec3(-bbox->size.x, -bbox->size.y, bbox->size.z);
                            boxVerts[5] = bbox->center + rot * Vec3(bbox->size.x, -bbox->size.y, bbox->size.z);
                            boxVerts[6] = bbox->center + rot * Vec3(bbox->size.x, bbox->size.y, bbox->size.z);
                            boxVerts[7] = bbox->center + rot * Vec3(-bbox->size.x, bbox->size.y, bbox->size.z);

                            for (int j = 0; j < 8; j++)
                            {
                                boxVerts[j] = partPos + partMat * boxVerts[j];
                            }

                            int edges[12][2] =
                            {
                                {0, 1},
                                {1, 2},
                                {2, 3},
                                {3, 0},
                                {4, 5},
                                {5, 6},
                                {6, 7},
                                {7, 4},
                                {0, 4},
                                {1, 5},
                                {2, 6},
                                {3, 7}
                            };

                            for (int iEdge = 0; iEdge < 12; ++iEdge)
                            {
                                const Vec3& v0 = boxVerts[edges[iEdge][0]];
                                const Vec3& v1 = boxVerts[edges[iEdge][1]];

                                //                                  AddDebugLine(v0, v1, 255,255,255, 30.0f);

                                // use the terrain z at the edge mid-point. This means that duplicate edges (from the adjacent triangle)
                                // will be eliminated at the end
                                Vec3 midPt = 0.5f * (v0 + v1);
                                float terrainZ = pEngine->GetTerrainElevation(midPt.x, midPt.y);
                                // if within range add v0 and up to 2 clip points. v1 might get added on the next edge
                                if (v0.z > terrainZ + criticalMinAlt && v0.z < terrainZ + criticalMaxAlt)
                                {
                                    vertices.push_back(v0);
                                }
                                const float clipAlts[2] = {terrainZ + criticalMinAlt, terrainZ + criticalMaxAlt};
                                for (int iClip = 0; iClip < 2; ++iClip)
                                {
                                    const float clipAlt = clipAlts[iClip];
                                    float d0 = v0.z - clipAlt;
                                    float d1 = v1.z - clipAlt;
                                    if (d0 * d1 < 0.0f)
                                    {
                                        float d = d0 - d1;
                                        Vec3 clipPos = (v1 * d0 - v0 * d1) / d;
                                        vertices.push_back(clipPos);
                                        //                                          AddDebugSphere(clipPos, 0.02f, 255,255,255, 30.0f);
                                    }
                                }
                            }
                        }     // box case
                        break;
                        case GEOM_TRIMESH:
                        {
                            const mesh_data* mesh = static_cast<const mesh_data*>(prim);

                            int numVerts = mesh->nVertices;
                            int numTris = mesh->nTris;

                            static std::vector<Vec3>    worldVertices;
                            worldVertices.resize(numVerts);
                            for (int j = 0; j < numVerts; j++)
                            {
                                worldVertices[j] = partPos + partMat * mesh->pVertices[j];
                            }

                            for (int j = 0; j < numTris; ++j)
                            {
                                int indices[3] = {mesh->pIndices[j * 3 + 0], mesh->pIndices[j * 3 + 1], mesh->pIndices[j * 3 + 2]};
                                const Vec3 worldV[3] = {worldVertices[indices[0]], worldVertices[indices[1]], worldVertices[indices[2]]};

                                for (int iEdge = 0; iEdge < 3; ++iEdge)
                                {
                                    const Vec3& v0 = worldV[iEdge];
                                    const Vec3& v1 = worldV[(iEdge + 1) % 3];
                                    // use the terrain z at the edge mid-point. This means that duplicate edges (from the adjacent triangle)
                                    // will be eliminated at the end
                                    Vec3 midPt = 0.5f * (v0 + v1);
                                    float terrainZ = pEngine->GetTerrainElevation(midPt.x, midPt.y);
                                    // if within range add v0 and up to 2 clip points. v1 might get added on the next edge
                                    if (v0.z > terrainZ + criticalMinAlt && v0.z < terrainZ + criticalMaxAlt)
                                    {
                                        vertices.push_back(v0);
                                    }
                                    const float clipAlts[2] = {terrainZ + criticalMinAlt, terrainZ + criticalMaxAlt};
                                    for (int iClip = 0; iClip < 2; ++iClip)
                                    {
                                        const float clipAlt = clipAlts[iClip];
                                        float d0 = v0.z - clipAlt;
                                        float d1 = v1.z - clipAlt;
                                        if (d0 * d1 < 0.0f)
                                        {
                                            float d = d0 - d1;
                                            Vec3 clipPos = (v1 * d0 - v0 * d1) / d;
                                            vertices.push_back(clipPos);
                                        }
                                    }
                                }     // loop over edges
                            }     // loop over triangles
                        }     // trimesh
                        break;
                        case GEOM_SPHERE:
                        {
                            const primitives::sphere* sphere = static_cast<const primitives::sphere*>(prim);
                            Vec3 center = partPos + sphere->center;
                            float terrainZ = pEngine->GetTerrainElevation(center.x, center.y);
                            float top = center.z + sphere->r;
                            float bot = center.z - sphere->r;
                            if (top > terrainZ && bot < terrainZ + criticalMaxAlt)
                            {
                                static int nPts = 8;
                                vertices.reserve(nPts);
                                for (int j = 0; j < nPts; ++j)
                                {
                                    float angle = j * gf_PI2 / nPts;
                                    Vec3 pos = center + sphere->r * Vec3(cos(angle), sin(angle), 0.0f);
                                    vertices.push_back(pos);
                                }
                            }
                        }
                        break;
                        case GEOM_CAPSULE:
                        case GEOM_CYLINDER:
                        {
                            // rely on capsule being the same struct as a cylinder
                            Vec3 pts[2];
                            const primitives::cylinder* capsule = static_cast<const primitives::cylinder*> (prim);
                            pts[0] = partPos + partMat * (capsule->center - capsule->hh * capsule->axis);
                            pts[1] = partPos + partMat * (capsule->center + capsule->hh * capsule->axis);
                            float r = capsule->r;

                            static int nPts = 8;
                            vertices.reserve(nPts * 2);
                            for (unsigned iC = 0; iC != 2; ++iC)
                            {
                                float terrainZ = pEngine->GetTerrainElevation(pts[iC].x, pts[iC].y);
                                float top = pts[iC].z + capsule->r;
                                float bot = pts[iC].z - capsule->r;
                                if (top > terrainZ && bot < terrainZ + criticalMaxAlt)
                                {
                                    for (int j = 0; j < nPts; ++j)
                                    {
                                        float angle = j * gf_PI2 / nPts;
                                        Vec3 pos = pts[iC] + r * Vec3(cos(angle), sin(angle), 0.0f);
                                        vertices.push_back(pos);
                                    }
                                }
                            }
                            //              if (!vertices.empty())
                            //                AIWarning("cylinder at (%5.2f %5.2f %5.2f)", pts[0].x, pts[0].y, pts[0].z);
                        }
                        break;
                        default:
                            AIWarning("CAISystem::CalculateForbiddenAreas unhandled geom type %d", type);
                            break;
                        } // switch over geometry type
                    } // all parts

                    if (vertices.empty())
                    {
                        continue;
                    }

                    // remove duplicates
                    std::sort(vertices.begin(), vertices.end());
                    vertices.erase(std::unique(vertices.begin(), vertices.end(), Vec3Equivalent2D), vertices.end());

                    if (vertices.size() < 3)
                    {
                        continue;
                    }

                    static std::vector<Vec3>    hullVertices;
                    hullVertices.resize(0);

                    Vec3 offset = std::accumulate(vertices.begin(), vertices.end(), Vec3(ZERO)) / vertices.size();
                    for (std::vector<Vec3>::iterator it = vertices.begin(); it != vertices.end(); ++it)
                    {
                        *it -= offset;
                    }

                    AABB pointsBounds(AABB::RESET);
                    for (std::vector<Vec3>::iterator it = vertices.begin(); it != vertices.end(); ++it)
                    {
                        pointsBounds.Add(*it);
                    }

                    ConvexHull2D(hullVertices, vertices);
                    if (hullVertices.size() < 3)
                    {
                        continue;
                    }

                    AABB hullBounds(&vertices[0], vertices.size());

                    Vec3 pointsSize = pointsBounds.GetSize();
                    pointsSize.z = 0;
                    Vec3 hullSize = hullBounds.GetSize();
                    hullSize.z = 0;
                    if (fabsf(pointsSize.GetLength() - hullSize.GetLength()) > 0.01f)
                    {
                        // Size of the bounds are different
                        AIWarning("The bounds of automatic forbidden area convex hull differs from the input point bounds.");

                        Vec3 center = pointsBounds.GetCenter();
                        pointsBounds.Move(-center);
                        OBB obb;
                        obb.SetOBBfromAABB(Matrix33::CreateIdentity(), pointsBounds);

                        char msg[256];
                        azsnprintf(msg, 256, "New area bounds mismatch\n(%.f, %.f, %.f).", center.x, center.y, center.z);
                        m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, center, obb, ColorB(255, 0, 196)));
                    }

                    // remove duplicates
                    hullVertices.erase(std::unique(hullVertices.begin(), hullVertices.end(), Vec3Equivalent2D), hullVertices.end());
                    while (hullVertices.size() > 1 && Vec3Equivalent2D(hullVertices.front(), hullVertices.back()))
                    {
                        hullVertices.pop_back();
                    }

                    // Check if the shape self-intersects
                    Vec3 badPt;
                    if (DoesShapeSelfIntersect(hullVertices, badPt, m_validationErrorMarkers))
                    {
                        AIWarning("(%5.2f, %5.2f) New automatic forbidden area self intersects: dropping it (triangulation will not be perfect).", badPt.x, badPt.y);
                        continue;
                    }

                    // Check if the shape is degenerate
                    float area = CalcPolygonArea<std::vector<Vec3>, float>(hullVertices);
                    if (area < 0.01f)
                    {
                        if (area < 0.0f)
                        {
                            // Clockwise area
                            // Size of the bounds are different
                            AIWarning("The area of the automatic forbidden area is negative %f.", area);

                            Vec3 center = pointsBounds.GetCenter();
                            pointsBounds.Move(-center);
                            OBB obb;
                            obb.SetOBBfromAABB(Matrix33::CreateIdentity(), pointsBounds);

                            char msg[256];
                            azsnprintf(msg, 256, "Negative Area\n(%.f, %.f, %.f).", center.x, center.y, center.z);
                            m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, center, obb, ColorB(255, 0, 196)));
                        }
                        else
                        {
                            // Degenerate or badly wound shape.
                            AILogProgress("Area of automatic forbidden areas less than 0.01 (%.5f), skipping.", area);
                        }
                        continue;
                    }

                    if (hullVertices.size() < 3)
                    {
                        continue;
                    }

                    for (std::vector<Vec3>::iterator it = hullVertices.begin(); it != hullVertices.end(); ++it)
                    {
                        *it += offset;
                    }

                    if (!IsShapeCompletelyInForbiddenRegion(hullVertices, false))
                    {
                        // should already be correctly wound
                        string name;
                        name.Format("CFA%04d", forbiddenNameCounter++);

                        CAIShape* pShape = new CAIShape;
                        pShape->SetName(name);
                        pShape->SetPoints(hullVertices);
                        extraAreas.InsertShape(pShape);
                        AILogProgress("Created forbidden area %s with %lu points", name.c_str(), hullVertices.size());
                    }
                } // r >= criticalRadius - i.e. using geometry
                else if (false && vegRad > 0.0f)
                {
                    // Use a circle
                    pe_status_pos status;
                    pCurrent->GetStatus(&status);

                    const unsigned nPts = 8;
                    ShapePointContainer pts(nPts);
                    for (unsigned iPt = 0; iPt < nPts; ++iPt)
                    {
                        Vec3 pt = status.pos;
                        float angle = iPt * gf_PI2 / nPts;
                        pt += vegRad * Vec3(cosf(angle), sinf(angle), 0.0f);
                        pts[iPt] = pt;
                    }

                    if (!IsShapeCompletelyInForbiddenRegion(pts, false))
                    {
                        string name;
                        name.Format("CFA%04d", forbiddenNameCounter++);

                        CAIShape* pShape = new CAIShape;
                        pShape->SetName(name);
                        pShape->SetPoints(pts);
                        extraAreas.InsertShape(pShape);
                        AILogProgress("Created forbidden area %s with %lu points", name.c_str(), pts.size());
                    }
                }
                else
                {
                    // this object radius will be applied only to the links
                }
            } // stat obj
        } // render node
    } // loop over entities

    AILogProgress("Combining %lu extra areas", extraAreas.GetShapes().size());
    if (!CombineForbiddenAreas(extraAreas))
    {
        m_forbiddenAreas.Copy(extraAreas);
        return false;
    }

    m_forbiddenAreas.Insert(extraAreas);

    AILogProgress("Combining %lu forbidden areas", m_forbiddenAreas.GetShapes().size());
    if (!CombineForbiddenAreas(m_forbiddenAreas))
    {
        return false;
    }

    return true;
}

//===================================================================
// RemoveExtraPointsFromShape
//===================================================================
void RemoveExtraPointsFromShape(ShapePointContainer& pts)
{
    // Don't remove a point if the resulting segment would be longer than this
    static float maxSegDistSq = square(10.0f);
    // Don't remove a point(s) if the removed point(s) would be more than this
    // from the new segment. Differentiate between in and out
    static float criticalOutSideDist = 0.01f;
    static float criticalInSideDist = 0.01f;
    // Don't remove points if the angle between the prev/next segments has a
    // dot-product smaller than this
    static float criticalDot = 0.9f;

    // number of points that will be cut each time (gets zerod after the cut)
    int numCutPoints = -1;

    // it is our current segment start
    for (ShapePointContainer::iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const Vec3& itPos = *it;
        // itSegEnd is our potential segment end. We keep increasing it until cutting an
        // intermediate point would diverge too much. If this happens then we move itSegEnd
        // back one place (i.e. back to the last safe segment end) and cut the intervening
        // points
        ShapePointContainer::iterator itSegEnd;
        for (itSegEnd = it; itSegEnd != pts.end(); ++itSegEnd, ++numCutPoints)
        {
            if (itSegEnd == it)
            {
                continue;
            }

            const Vec3& itSegEndPos = *itSegEnd;

            float potentialSegmentLenSq = Distance::Point_Point2DSq(itPos, itSegEndPos);
            if (potentialSegmentLenSq > maxSegDistSq)
            {
                goto TryToCut;
            }

            const Lineseg potentialSegment(itPos, itSegEndPos);

            // if the potential segment would result in self-intersection then skip it
            for (ShapePointContainer::iterator itRest = itSegEnd; itRest != it; ++itRest)
            {
                if (itRest == pts.end())
                {
                    itRest = pts.begin();
                }
                if (itRest == it)
                {
                    break;
                }
                if (itRest == itSegEnd)
                {
                    continue;
                }

                ShapePointContainer::iterator itRestNext = itRest;
                ++itRestNext;
                if (itRestNext == pts.end())
                {
                    itRestNext = pts.begin();
                }
                if (itRestNext == it || itRestNext == itSegEnd)
                {
                    continue;
                }

                if (Overlap::Lineseg_Lineseg2D(potentialSegment, Lineseg(*itRest, *itRestNext)))
                {
                    goto TryToCut;
                }
            }

            Vec3 potentialSegmentDir = itSegEndPos - itPos;
            potentialSegmentDir.z = 0.0f;
            potentialSegmentDir.NormalizeSafe();
            // assume shape is wound anti-clockwise
            Vec3 potentialSegmentOutsideDir(potentialSegmentDir.y, -potentialSegmentDir.x, 0.0f);

            // itCut is a potential point to cut
            for (ShapePointContainer::iterator itCut = it; itCut != itSegEnd; ++itCut)
            {
                if (itCut == it)
                {
                    continue;
                }
                const Vec3& itCutPos = *itCut;
                // check sideways distance
                Vec3 delta = itCutPos - potentialSegment.start;
                float outsideDist = potentialSegmentOutsideDir.Dot(delta - delta.Dot(potentialSegmentDir) * potentialSegmentDir);
                if (outsideDist > criticalOutSideDist)
                {
                    goto TryToCut;
                }
                if (outsideDist < -criticalInSideDist)
                {
                    goto TryToCut;
                }

                // check dot product
                ShapePointContainer::iterator itCutPrev = itCut;
                --itCutPrev;
                ShapePointContainer::iterator itCutNext = itCut;
                ++itCutNext;
                if (itCutNext == itSegEnd)
                {
                    continue;
                }
                Vec3 deltaPrev = (itCutPos - *itCutPrev);
                deltaPrev.z = 0.0f;
                deltaPrev.NormalizeSafe();
                Vec3 deltaNext = (*itCutNext - itCutPos);
                deltaNext.z = 0.0f;
                deltaNext.NormalizeSafe();
                float dot = deltaPrev.Dot(deltaNext);
                if (dot < criticalDot)
                {
                    goto TryToCut;
                }
            } // iterate over potential points to cut
        } // iterate over segment end points
TryToCut:
        // Either itSegEnd reached the end, or else it has gone just one step too far.
        // Either way we move it back one place and if that's successful then we remove
        // points between it and the new location.
        if (pts.size() - numCutPoints < 3)
        {
            return;
        }
        if (itSegEnd != it && --itSegEnd != it)
        {
            ShapePointContainer::iterator itNext = it;
            ++itNext;
            if (itNext != itSegEnd && itNext != pts.end())
            {
                pts.erase(itNext, itSegEnd);
            }
        }
        numCutPoints = -1;
    }
}

//===================================================================
// CombineForbiddenAreas
//===================================================================
bool CNavigation::CombineForbiddenAreas(CAIShapeContainer& areasContainer)
{
    typedef std::list<CAIShape*> ShapeList;

    // when we find an area that doesn't intersect anything else we put it in here
    ShapeList areas;
    ShapeList processedAreas;

    // simplify all shapes

    CAIShapeContainer::ShapeVector& vShapes = areasContainer.GetShapes();
    for (CAIShapeContainer::ShapeVector::iterator it = vShapes.begin(), itEnd = vShapes.end(); it != itEnd; ++it)
    {
        areas.push_back(*it);
    }

    for (ShapeList::iterator it = areas.begin(), itEnd = areas.end(); it != itEnd; ++it)
    {
        CAIShape* pAIShape = *it;

        areasContainer.DetachShape(pAIShape); // Detach the shape from the container
        RemoveExtraPointsFromShape(pAIShape->GetPoints());
        pAIShape->BuildAABB();
    }

    areasContainer.Clear();

    // remove all the non-overlapping shapes
    for (ShapeList::iterator it = areas.begin(); it != areas.end(); )
    {
        bool overlap = false;
        // Find if the shape overlaps with other shapes.
        for (ShapeList::iterator it2 = areas.begin(); it2 != areas.end(); ++it2)
        {
            if (it == it2)
            {
                continue;            // Skipt self.
            }
            const CAIShape* shapes[2] = { *it, *it2 };
            if (Overlap::Polygon_Polygon2D<ShapePointContainer>(shapes[0]->GetPoints(), shapes[1]->GetPoints(),
                    &shapes[0]->GetAABB(), &shapes[1]->GetAABB()))
            {
                overlap = true;
                break;
            }
        }

        if (!overlap)
        {
            // Move into processedAreas
            processedAreas.push_back(*it);
            it = areas.erase(it);
        }
        else
        {
            ++it;
        }
    }

    unsigned unionCounter = 0;
    I3DEngine* pEngine = m_pSystem->GetI3DEngine();

    while (!areas.empty())
    {
        CAIShape* pCurrentShape = areas.front();
        areas.pop_front();

        // Find the first shape that overlaps the current shape.
        CAIShape* pNextShape = 0;
        for (ShapeList::iterator it = areas.begin(); it != areas.end(); ++it)
        {
            if (Overlap::Polygon_Polygon2D<ShapePointContainer>(pCurrentShape->GetPoints(), (*it)->GetPoints(),
                    &pCurrentShape->GetAABB(), &(*it)->GetAABB()))
            {
                pNextShape = *it;
                areas.erase(it);
                break;
            }
        }

        if (!pNextShape)
        {
            // Could not find overlapping shape!
            AIWarning("Forbidden area %s was reported to intersect with another shape, but the intersecting shape was not found! (%lu areas left)", pCurrentShape->GetName().c_str(), areas.size());
            processedAreas.push_back(pCurrentShape);

            AABB aabb = pCurrentShape->GetAABB();
            Vec3 center = aabb.GetCenter();
            aabb.Move(-center);
            OBB obb;
            obb.SetOBBfromAABB(Matrix33::CreateIdentity(), aabb);

            char msg[256];
            azsnprintf(msg, 256, "Intersect mismatch\n(%.f, %.f, %.f).", center.x, center.y, center.z);
            m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, center, obb, ColorB(255, 0, 196)));
        }
        else
        {
            const string* names[2] = { &pCurrentShape->GetName(), &pNextShape->GetName() };
            const CAIShape* shapes[2] = { pCurrentShape, pNextShape };

            AABB originalCombinedAABB(shapes[0]->GetAABB());
            originalCombinedAABB.Add(shapes[1]->GetAABB());

            // this is subtracted from the polygons before combining, then added afterwards to
            // reduce rounding errors during the geometric union
            Vec3 offset = std::accumulate(shapes[0]->GetPoints().begin(), shapes[0]->GetPoints().end(), Vec3(ZERO));
            offset += std::accumulate(shapes[1]->GetPoints().begin(), shapes[1]->GetPoints().end(), Vec3(ZERO));
            offset /= (shapes[0]->GetPoints().size() + shapes[1]->GetPoints().size());
            offset.z = 0.0f;

            string newName;
            newName.Format("%s+%s", names[0]->c_str(), names[1]->c_str());
            if (newName.length() > 1024)
            {
                newName.Format("CombinedArea%d", forbiddenNameCounter++);
            }

            // combine the shapes
            Polygon2d polygons[2];
            for (unsigned iPoly = 0; iPoly < 2; ++iPoly)
            {
                const ShapePointContainer& pts = shapes[iPoly]->GetPoints();
                //        Vec3 badPt;
                //        if (DoesShapeSelfIntersect(shape, badPt))
                //          AIWarning("Input polygon self intersects at (%5.2f, %5.2f) %s", badPt.x, badPt.y, names[iPoly]->c_str());

                unsigned nPts = pts.size();
                //        for (ListPositions::const_iterator it = shape.begin() ; it != shape.end() ; ++it, ++i)
                for (unsigned k = 0; k < nPts; ++k)
                {
                    Vector2d pt(pts[k].x - offset.x, pts[k].y - offset.y);
                    polygons[iPoly].AddVertex(pt);
                    polygons[iPoly].AddEdge(Polygon2d::Edge((k + nPts - 1) % nPts, k));
                }
                polygons[iPoly].CollapseVertices(0.001f);
                //polygons[iPoly].ComputeBspTree();

                //        if (!polygons[iPoly].IsWoundAnticlockwise())
                //          AIWarning("Input polygon not wound anti-clockwise %s", names[iPoly]->c_str());
            }

            //      polygons[0].Print("PolyP.txt");
            //      polygons[1].Print("PolyQ.txt");

            AILogProgress("Running polygon union %d", unionCounter++);

            Polygon2d polyUnion = polygons[0] | polygons[1];

            //      if (!polyUnion.IsWoundAnticlockwise())
            //        AIWarning("Union polygon not wound anti-clockwise %s", newName.c_str());

            polyUnion.CollapseVertices(0.001f);

            //      polyUnion.Print("PolyUnion.txt");

            polyUnion.CalculateContours(true);
            unsigned nContours = polyUnion.GetContourQuantity();
            if (nContours != 1)
            {
                AILogComment("Got %d contours out of the union - using them all but expect problems. From %s",
                    nContours, newName.c_str());
            }

            AILogEvent("Finished Running polygon union");

            // Remove the merged shapes.
            delete shapes[0];
            shapes[0] = 0;
            delete shapes[1];
            shapes[1] = 0;

            // it's possible that the union will result in two or more just-touching contours
            // so need to add them all.
            AABB combinedTotalAABB(AABB::RESET);
            for (unsigned iContour = 0; iContour < nContours; ++iContour)
            {
                const Vector2d* pPts;
                unsigned nPts = polyUnion.GetContour(iContour, &pPts); // guaranteed anti-clockwise wound
                if (nPts < 3)
                {
                    continue;
                }

                ShapePointContainer newPts(nPts);
                for (unsigned iPt = 0; iPt < nPts; ++iPt)
                {
                    // Sanity check
                    if (pPts[iPt].x < -1000.0f || pPts[iPt].x > 10000.0f || pPts[iPt].y < -1000.0f || pPts[iPt].y > 10000.0f)
                    {
                        AIWarning("Contour %d/%d has bad point (%f, %f)", iContour + 1, nContours, pPts[iPt].x, pPts[iPt].y);
                    }

                    Vec3 pt(pPts[iPt].x + offset.x, pPts[iPt].y + offset.y, 0.0f);
                    pt.z = pEngine->GetTerrainElevation(pt.x, pt.y);
                    newPts[iPt] = pt;
                    combinedTotalAABB.Add(pt);
                }

                string thisName;
                if (nContours > 1)
                {
                    thisName.Format("%s-%d", newName.c_str(), iContour);
                }
                else
                {
                    thisName = newName;
                }

                // Make sure the name is unique
                for (ShapeList::iterator itn = areas.begin(); itn != areas.end(); ++itn)
                {
                    if ((*itn)->GetName() == thisName)
                    {
                        thisName.Format("CFA-%d", forbiddenNameCounter++);
                        break;
                    }
                }
                for (ShapeList::iterator itn = processedAreas.begin(); itn != processedAreas.end(); ++itn)
                {
                    if ((*itn)->GetName() == thisName)
                    {
                        thisName.Format("CFA-%d", forbiddenNameCounter++);
                        break;
                    }
                }

                RemoveExtraPointsFromShape(newPts);

                Vec3 badPt;
                if (DoesShapeSelfIntersect(newPts, badPt, m_validationErrorMarkers))
                {
                    AIWarning("(%5.2f, %5.2f) Combined area self intersects: dropping it (triangulation will not be perfect). Check/modify objects nearby: %s",
                        badPt.x, badPt.y, thisName.c_str());
                    continue;
                }

                if (newPts.size() < 3)
                {
                    continue;
                }

                CAIShape* pNewShape = new CAIShape;
                pNewShape->SetName(thisName);
                pNewShape->SetPoints(newPts);

                // if this is free of overlap then don't process it any more
                bool isFreeOfOverlap = true;
                for (ShapeList::iterator ito = areas.begin(); ito != areas.end(); ++ito)
                {
                    const CAIShape* pTestShape = *ito;
                    if (Overlap::Polygon_Polygon2D<ShapePointContainer>(pNewShape->GetPoints(), pTestShape->GetPoints(),
                            &pNewShape->GetAABB(), &pTestShape->GetAABB()))
                    {
                        isFreeOfOverlap = false;
                        break;
                    }
                }

                if (isFreeOfOverlap)
                {
                    processedAreas.push_back(pNewShape);
                }
                else
                {
                    areas.push_back(pNewShape);

                    /*          if (!areas.insert(ShapeMap::iterator::value_type(thisName, thisShape)).second)
                    {
                    AIError("CAISystem::CombineForbiddenAreas Duplicate auto-forbidden area name after processing %s",
                    thisName.c_str());
                    return false;
                    }*/
                }

                Vec3 origSize = originalCombinedAABB.GetSize();
                origSize.z = 0;
                Vec3 combinedSize = combinedTotalAABB.GetSize();
                combinedSize.z = 0;

                if (fabsf(origSize.GetLength() - combinedSize.GetLength()) > 0.1f)
                {
                    // Size of the bounds are different
                    AIWarning("The bounds of the combined forbidden area %s is different than expected from the combined results.", pNewShape->GetName().c_str());

                    Vec3 center = originalCombinedAABB.GetCenter();
                    originalCombinedAABB.Move(-center);
                    OBB obb;
                    obb.SetOBBfromAABB(Matrix33::CreateIdentity(), originalCombinedAABB);

                    char msg[256];
                    azsnprintf(msg, 256, "Combined bounds mismatch\n(%.f, %.f, %.f).", center.x, center.y, center.z);
                    m_validationErrorMarkers.push_back(SValidationErrorMarker(msg, center, obb, ColorB(255, 0, 196)));
                }
            }
        }
    }

    for (ShapeList::iterator it = processedAreas.begin(); it != processedAreas.end(); ++it)
    {
        areasContainer.InsertShape(*it);
    }

    return true;
}

//===================================================================
// CreatePossibleCutList
//===================================================================
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void    CNavigation::CreatePossibleCutList(const Vec3& vStart, const Vec3& vEnd, ListNodeIds& lstNodes)
{
    CGraph* pGraph = GetGraph();

    if (doForbiddenDebug)
    {
        AILogAlways("%d: CreatePossibleCutList start (%7.4f, %7.4f) end (%7.4f, %7.4f)",
            debugForbiddenCounter, vStart.x, vStart.y, vEnd.x, vEnd.y);
    }

    float       offset = .5f;
    Vec3 pos = vStart + offset * (vEnd - vStart);
    unsigned currentIndex = GetTriangularNavRegion()->GetEnclosing(pos);
    GraphNode* pCurNode = pGraph->GetNodeManager().GetNode(currentIndex);

    if (!pCurNode)
    {
        AIError("Unable to find graph node enclosing. Pos: (%5.2f, %5.2f, %5.2f) [Code bug]", pos.x, pos.y, pos.z);
        return;
    }

    ListNodeIds queueList;
    lstNodes.clear();
    queueList.clear();
    pGraph->ClearTags();

    while (!SegmentInTriangle(pCurNode, vStart, vEnd))
    {
        offset *= .5f;
        currentIndex = GetTriangularNavRegion()->GetEnclosing(vStart + offset * (vEnd - vStart));
        pCurNode = pGraph->GetNode(currentIndex);
        if (offset < .0001f)
        {
            if (doForbiddenDebug)
            {
                AILogAlways("%d: failed seg in triangle part", debugForbiddenCounter);
            }
            // can't find  node on the edge
            //          AIAssert( 0 );
            return;
        }
    }
    if (doForbiddenDebug)
    {
        AILogAlways("%d: CreatePossibleCutList offset = %7.4f, node at (%7.4f, %7.4f)",
            debugForbiddenCounter, offset, pCurNode->GetPos().x, pCurNode->GetPos().y);
    }

    pGraph->TagNode(currentIndex);
    queueList.push_back(currentIndex);

    while (!queueList.empty())
    {
        currentIndex = queueList.front();
        pCurNode = pGraph->GetNodeManager().GetNode(currentIndex);
        ;
        queueList.pop_front();
        lstNodes.push_back(currentIndex);

        for (unsigned link = pCurNode->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
        {
            unsigned candidateIndex = pGraph->GetLinkManager().GetNextNode(link);
            GraphNode* pCandidate = pGraph->GetNodeManager().GetNode(candidateIndex);
            if (pCandidate->tag)
            {
                continue;
            }
            pGraph->TagNode(candidateIndex);
            ++debugForbiddenCounter;
            if (!SegmentInTriangle(pCandidate, vStart, vEnd))
            {
                if (doForbiddenDebug)
                {
                    AILogAlways("%d:          reject node at (%7.4f, %7.4f)", debugForbiddenCounter, pCandidate->GetPos().x, pCandidate->GetPos().y);
                }
                continue;
            }
            else
            {
                if (doForbiddenDebug)
                {
                    AILogAlways("%d:       -> accept node at (%7.4f, %7.4f)", debugForbiddenCounter, pCandidate->GetPos().x, pCandidate->GetPos().y);
                }
                queueList.push_back(candidateIndex);
            }
        }
    }
    pGraph->ClearTags();
    for (ListNodeIds::iterator nI = lstNodes.begin(); nI != lstNodes.end(); ++nI)
    {
        pGraph->TagNode(*nI);
    }
    return;
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif


//===================================================================
// AddTheCut
//===================================================================
static void AddTheCut(int vIdx1, int vIdx2, NewCutsVector& newCutsVector)
{
    AIAssert(vIdx1 >= 0 && vIdx2 >= 0);
    if (vIdx1 == vIdx2)
    {
        return;
    }
    newCutsVector.push_back(CutEdgeIdx(vIdx1, vIdx2));
}


static const real epsilon = 0.0000001;
//const float epsilon = 0.001f;

//
//-----------------------------------------------------------------------------------------------------------
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void CNavigation::RefineTriangle(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, unsigned nodeIndex, const Vec3r& start, const Vec3r& end, ListNodeIds& lstNewNodes, ListNodeIds& lstOldNodes, NewCutsVector& newCutsVector)
{
    CGraph* pGraph = GetGraph();

    GraphNode* pNode = pGraph->GetNodeManager().GetNode(nodeIndex);

    if (doForbiddenDebug)
    {
        AILogAlways("%d: RefineTriangle start (%7.4f, %7.4f) end (%7.4f, %7.4f) for tri at (%5.2f %5.2f)",
            debugForbiddenCounter, start.x, start.y, end.x, end.y, pNode->GetPos().x, pNode->GetPos().y);
    }

    int newNodesCount = lstNewNodes.size();

    Vec3r D0, P0;

    Vec3r newStart = start;
    real maximum_s = -epsilon;
    real aux_maximum_s = -epsilon;

    // parametric equation of new edge
    P0 = start;
    D0 = end - start;

    Vec3r vCut1(ZERO);
    Vec3r   vCut2(ZERO);

    int FirstCutStart = -1, FirstCutEnd = -1;
    int SecondCutStart = -1, SecondCutEnd = -1;
    int StartCutStart = -1, StartCutEnd = -1;
    int EndCutStart = -1, EndCutEnd = -1;
    int TouchStart = -1, TouchEnd = -1;

    unsigned int index, next_index;
    for (index = 0; index < pNode->GetTriangularNavData()->vertices.size(); ++index)
    {
        Vec3r P1, D1;
        next_index = index + 1;
        if (next_index == pNode->GetTriangularNavData()->vertices.size())       // make sure we wrap around correctly
        {
            next_index = 0;
        }

        //get the triangle edge
        P1 = GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[index]).vPos;
        D1 = GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[next_index]).vPos - P1;

        // calculate intersection parameters
        real s = -1;  // parameter on new edge
        real t = -1;  // parameter on triangle edge
        {
            Vec3r delta = P1 - P0;
            real crossD = D0.x * D1.y - D0.y * D1.x;
            real crossDelta1 = delta.x * D1.y - delta.y * D1.x;
            real crossDelta2 = delta.x * D0.y - delta.y * D0.x;

            if (fabs(crossD) > epsilon)
            {
                // intersection
                s = crossDelta1 / crossD;
                t = crossDelta2 / crossD;
            }
            else
            {
                TouchStart = index;
                TouchEnd = next_index;
            }
        }

        if (doForbiddenDebug)
        {
            AILogAlways("%d: index = %d, s = %5.2f t = %5.2f", debugForbiddenCounter, index, s, t);
        }

        // for any reasonable calculation both s and t must be between 0 and 1
        // everything else is a non desired situation
        if ((t > -epsilon) && (t < 1.f + epsilon))
        {
            // calculate the point of intersection
            Vec3r result = P0 + D0 * s;
            if (s > maximum_s)
            {
                maximum_s = s;
            }

            // s < 0
            if (s < -epsilon)
            {
                if (doForbiddenDebug)
                {
                    AILogAlways("%d: start clean triangle", debugForbiddenCounter);
                }
                // a clean start triangle
                StartCutStart = index;
                StartCutEnd   = next_index;
            }

            // s == 0  or s == 1
            if ((fabs(s) < epsilon) || ((s > 1.f - epsilon) && (s < 1.f + epsilon)))
            {
                if (doForbiddenDebug)
                {
                    AILogAlways("%d: s == 0 or s == 1", debugForbiddenCounter);
                }
                // the start coincides with a triangle vertex or lies on a triangle side
                if ((t > epsilon) && (t < 1.f - epsilon))
                {
                    // the start lies on a triangle side
                    if (FirstCutStart < 0)
                    {
                        FirstCutStart = index;
                        FirstCutEnd   = next_index;
                        vCut1 = result;
                    }
                    else
                    {
                        SecondCutStart = index;
                        SecondCutEnd    = next_index;
                        vCut2 = result;
                    }
                }
                // if its in the triangle vertex then just skip
            }


            // s between 0 and 1
            if ((s > epsilon) && (s < 1.f - epsilon))
            {
                // a normal cut or new edge coincides with a vertex on the triangle
                if ((fabs(t) < epsilon) || (t > 1.f - epsilon))
                {
                    //the edge coincides with a triangle vertex
                    // skip this case
                    int a = 5;
                }
                else
                {
                    // a normal cut
                    if (FirstCutStart < 0)
                    {
                        FirstCutStart = index;
                        FirstCutEnd   = next_index;
                        vCut1 = result;
                    }
                    else
                    {
                        SecondCutStart = index;
                        SecondCutEnd    = next_index;
                        vCut2 = result;
                    }
                }
            }

            // s bigger then 1
            if (s > 1.f + epsilon)
            {
                // a clear end situation
                EndCutStart = index;
                EndCutEnd       = next_index;
            }
        }

        aux_maximum_s = s;
    } // end for

    ObstacleData od1, od2;

    if (doForbiddenDebug)
    {
        AILogAlways("%d: creating new triangles StartCutStart = %d EndCutStart = %d", debugForbiddenCounter, StartCutStart, EndCutStart);
    }


    // now create the new triangles
    if (StartCutStart >= 0)
    {
        ++debugForbiddenCounter;
        // start is in this triangle
        // triangle: start vertex and edge that it cuts
        od1.vPos = start;
        od2.vPos = end;
        CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[StartCutStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[StartCutEnd]), false, lstNewNodes);

        int notStart    = 3 - (StartCutStart + StartCutEnd);

        if (EndCutStart >= 0)
        {
            ++debugForbiddenCounter;
            // and end also
            // triangle: end vertex and edge that it cuts
            CreateNewTriangle(od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutEnd]), false, lstNewNodes);
            // triangle: start vertex end vertex and both end vertices
            CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutStart]), true, lstNewNodes);
            CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutEnd]), true, lstNewNodes);
            AddTheCut(GetVertexList().FindVertex(od1), GetVertexList().FindVertex(od2), newCutsVector);

            int notEnd      = 3 - (EndCutStart + EndCutEnd);
            CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notEnd]), false, lstNewNodes);
        }
        else
        {
            ++debugForbiddenCounter;
            if (FirstCutStart >= 0)
            {
                ++debugForbiddenCounter;
                // simple start-cut case
                od2.vPos = vCut1;
                // triangle: start vertex, cut pos and both cut vertices
                CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutStart]), true, lstNewNodes);
                CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutEnd]), true, lstNewNodes);
                AddTheCut(GetVertexList().FindVertex(od1), GetVertexList().FindVertex(od2), newCutsVector);
                // find index of vertex that does not lie on cut edge
                int notCut = 3 - (FirstCutEnd + FirstCutStart);
                // triangle: start vertex, notcut and notstart
                CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut]), false, lstNewNodes);
            }
            else
            {
                ++debugForbiddenCounter;
                // nasty: start ok but end or cut into a vertex
                // two more triangles
                if (od1.vPos != GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notStart]).vPos)
                {
                    ++debugForbiddenCounter;
                    // od1 and notStart
                    CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[StartCutStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notStart]), true, lstNewNodes);
                    CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[StartCutEnd]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notStart]), true, lstNewNodes);
                    AddTheCut(GetVertexList().FindVertex(od1), pNode->GetTriangularNavData()->vertices[notStart], newCutsVector);
                }
            }
        }
    }
    else
    {
        ++debugForbiddenCounter;

        // start is not in this triangle

        if (EndCutStart >= 0)
        {
            ++debugForbiddenCounter;
            // but end is
            od1.vPos = end;
            od2.vPos = vCut1;

            CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutEnd]), false, lstNewNodes);
            int notEnd = 3 - (EndCutStart + EndCutEnd);
            if (FirstCutStart >= 0)
            {
                ++debugForbiddenCounter;
                // simple cut-end case
                CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutStart]), true, lstNewNodes);
                CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutEnd]), true, lstNewNodes);
                AddTheCut(GetVertexList().FindVertex(od1), GetVertexList().FindVertex(od2), newCutsVector);
                int notCut = 3 - (FirstCutStart + FirstCutEnd);
                CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notEnd]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut]), false, lstNewNodes);
            }
            else
            {
                ++debugForbiddenCounter;
                // od1 and notEnd
                // end ok but cut in vertex
                CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notEnd]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutStart]), true, lstNewNodes);
                CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notEnd]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[EndCutEnd]), true, lstNewNodes);
                AddTheCut(GetVertexList().FindVertex(od1), pNode->GetTriangularNavData()->vertices[notEnd], newCutsVector);
            }
        }
        else
        {
            ++debugForbiddenCounter;
            // this triangle contains no start and no end
            if (FirstCutStart >= 0)
            {
                ++debugForbiddenCounter;
                od1.vPos = vCut1;
                int notCut1 = 3 - (FirstCutStart + FirstCutEnd);
                if (SecondCutStart >= 0)
                {
                    ++debugForbiddenCounter;
                    od2.vPos = vCut2;
                    // simple cut-cut case
                    // find shared vertex
                    int SHARED;
                    if (FirstCutStart == SecondCutStart)
                    {
                        SHARED = FirstCutStart;
                    }
                    else
                    {
                        if (FirstCutStart == SecondCutEnd)
                        {
                            SHARED = FirstCutStart;
                        }
                        else
                        {
                            SHARED = FirstCutEnd;
                        }
                    }

                    int notCut2 = 3 - (SecondCutStart + SecondCutEnd);

                    CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[SHARED]), true, lstNewNodes);
                    CreateNewTriangle(od1, od2, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut1]), true, lstNewNodes);
                    AddTheCut(GetVertexList().FindVertex(od1), GetVertexList().FindVertex(od2), newCutsVector);
                    CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut1]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut2]), false, lstNewNodes);
                }
                else
                {
                    ++debugForbiddenCounter;
                    // od and NotCut
                    // a cut is ok but otherwise the other edge hits a vertex
                    CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutStart]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut1]), true, lstNewNodes);
                    CreateNewTriangle(od1, GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[FirstCutEnd]), GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[notCut1]), true, lstNewNodes);
                    AddTheCut(GetVertexList().FindVertex(od1), pNode->GetTriangularNavData()->vertices[notCut1], newCutsVector);
                }
            }
            else
            {
                ++debugForbiddenCounter;
                // nasty case when nothing was cut... possibly the edge touches a triangle
                // skip - no new triangles added.
                if (TouchStart >= 0)
                {
                    ++debugForbiddenCounter;
                    Vec3r junk;
                    real d1 = Distance::Point_Line<real>(GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[TouchStart]).vPos, start, end, junk);
                    real d2 = Distance::Point_Line<real>(GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[TouchEnd]).vPos, start, end, junk);
                    //                  AIWarning("!TOUCHED EDGE. Distances are %.6f and %.6f and aux_s %.6f and maximum_s:%.6f",d1,d2,aux_maximum_s,maximum_s);
                    if ((d1 < epsilon && d2 < epsilon) &&
                        (aux_maximum_s > epsilon && aux_maximum_s < 1 - epsilon))
                    {
                        // an edge has been touched
                        //  AIWarning("!Nasty case. Plugged %.4f into maximum_s.",aux_maximum_s);
                        maximum_s = aux_maximum_s;
                    }
                    else
                    {
                        maximum_s = -1;
                    }

                    AddTheCut(pNode->GetTriangularNavData()->vertices[TouchStart], pNode->GetTriangularNavData()->vertices[TouchEnd], newCutsVector);
                    // we don't create new triangle - just add it to new nodes list
                    lstNewNodes.push_back(nodeIndex);
                }
                else
                {
                    ++debugForbiddenCounter;
                    //                  AIWarning("!TOUCHED vertices. SKIP! Aux_s %.6f and maximum :%.6f",aux_maximum_s,maximum_s);
                    ///                 if (aux_maximum_s>epsilon && aux_maximum_s<1-epsilon)
                    //                      maximum_s = aux_maximum_s;
                    //                  else
                    maximum_s = 0.01f;
                }
            }
        }
    }

    bool bAddedTriangles = (newNodesCount != lstNewNodes.size());
    if (bAddedTriangles)
    {
        // now push old triangle for disconnection later
        lstOldNodes.push_back(nodeIndex);
        for (unsigned linkId = pNode->firstLinkIndex; linkId; linkId = linkManager.GetNextLink(linkId))
        {
            unsigned nextNodeIndex = linkManager.GetNextNode(linkId);
            GraphNode* pNextNode = nodeManager.GetNode(nextNodeIndex);
            if (pNextNode->tag || pNextNode->navType != IAISystem::NAV_TRIANGULAR)  // this one is in candidatesForCutting list (nodes2refine)
            {
                continue;
            }
            if ((std::find(lstNewNodes.begin(), lstNewNodes.end(), nextNodeIndex) == lstNewNodes.end())) // only push if its not the next triangle for refinement
            {
                lstNewNodes.push_back(nextNodeIndex);
            }
        }
    }
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif


//===================================================================
// TriangleLineIntersection
//===================================================================
bool CNavigation::TriangleLineIntersection(GraphNode* pNode, const Vec3& vStart, const Vec3& vEnd)
{
    if (!pNode || pNode->navType != IAISystem::NAV_TRIANGULAR || pNode->GetTriangularNavData()->vertices.empty())
    {
        return false;
    }
    unsigned nVerts = pNode->GetTriangularNavData()->vertices.size();
    Vec3r P0 = vStart;
    Vec3r Q0 = vEnd;
    unsigned int index, next_index;
    for (index = 0; index != nVerts; ++index)
    {
        Vec3r P1, Q1;
        next_index = (index + 1) % nVerts;

        //get the triangle edge
        P1 = GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[index]).vPos;
        Q1 = GetVertexList().GetVertex(pNode->GetTriangularNavData()->vertices[next_index]).vPos;

        real s = -1, t = -1;
        if (Intersect::Lineseg_Lineseg2D(Linesegr(P0, Q0), Linesegr(P1, Q1), s, t))
        {
            // For some reason just using the intersection result as is generates errors
            // later on - it seems that it detects intersections maybe between two edges of
            // the same triangle or something... Anyway, limiting the intersection test to
            // not include the segment ends makes this problem go away (epsilon can actually
            // be 0 here... but I don't like comparisons that close to 0)
            static const real eps = 0.0000000001;
            if ((s > 0.f - eps) && (s < 1.f + eps) && (t > 0.f + eps) && (t < 1.f - eps))
            {
                return true;
            }
        }
    }
    return false;
}

//===================================================================
// SegmentInTriangle
//===================================================================
bool CNavigation::SegmentInTriangle(GraphNode* pNode, const Vec3& vStart, const Vec3& vEnd)
{
    if (m_pGraph->PointInTriangle((vStart + vEnd) * .5f, pNode))
    {
        return true;
    }
    return TriangleLineIntersection(pNode, vStart, vEnd);
}

//
//-----------------------------------------------------------------------------------------------------------
#ifdef AI_FP_FAST
#pragma float_control(precise, on)
#pragma fenv_access(on)
#endif
void CNavigation::AddForbiddenAreas()
{
    debugForbiddenCounter = 0;

    for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
    {
        ++debugForbiddenCounter;
        AddForbiddenArea(m_forbiddenAreas.GetShapes()[i]);
    }

    for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
    {
        ++debugForbiddenCounter;
        AddForbiddenArea(m_forbiddenBoundaries.GetShapes()[i]);
    }
}
#ifdef AI_FP_FAST
#pragma fenv_access(off)
#pragma float_control(precise, off)
#endif

//====================================================================
// GetIntermediatePosition
// Calculates ptOut which is (ptNum/(numPts-1)) of the way between start
// and end via mid. If distTotal < 0 then it gets calculated, otherwise
// it and distToMid are assumed to be correct
//====================================================================
static void GetIntermediatePosition(Vec3& ptOut,
    const Vec3& start, const Vec3& mid, const Vec3& end,
    float& distTotal, float& distToMid,
    int ptNum, int numPts)
{
    if (distTotal < 0.0f || distToMid < 0.0f)
    {
        distToMid = (mid - start).GetLength();
        distTotal = distToMid + (end - mid).GetLength();
    }
    if (distToMid <= 0.0f)
    {
        AIWarning("Got bad distance in GetIntermediatePosition - distToMid = %6.3f (start = %5.2f, %5.2f, %5.2f, mid = %5.2f, %5.2f, %5.2f, end = %5.2f, %5.2f, %5.2f)",
            distToMid,
            start.x, start.y, start.z,
            mid.x, mid.y, mid.z,
            end.x, end.y, end.z);
        ptOut = start;
        return;
    }

    if (numPts <= 1)
    {
        ptOut = start;
    }
    else
    {
        float distFromStart = (distTotal * ptNum) / (numPts - 1);
        if (distFromStart < distToMid)
        {
            ptOut = start + (mid - start) * distFromStart / distToMid;
        }
        else
        {
            ptOut = mid + (end - mid) * (distFromStart - distToMid) / (distTotal - distToMid);
        }
    }
}

//====================================================================
// CalculateLinkProperties
//====================================================================
void CNavigation::CalculateLinkProperties()
{
    CGraph* pGraph = GetGraph();
    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();

    /// keep track of all tri-tri links that we've calculated so we can avoid doing
    /// reciprocal calculations
    std::set<unsigned> processedTriTriLinks;

    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAVMASK_ALL);
    while (unsigned currentNodeIndex = it.Increment())
    {
        GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

        if (pCurrent->navType == IAISystem::NAV_TRIANGULAR)
        {
            // clamp the vertices to the ground
            for (unsigned iVert = 0; iVert < pCurrent->GetTriangularNavData()->vertices.size(); ++iVert)
            {
                ObstacleData& od = GetVertexList().ModifyVertex(pCurrent->GetTriangularNavData()->vertices[iVert]);
                od.vPos.z = p3DEngine->GetTerrainElevation(od.vPos.x, od.vPos.y);
            }

            // find max passing radius between this node and all neighboors
            for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
            {
                GraphNode* pNextNode = pGraph->GetNodeManager().GetNode(pGraph->GetLinkManager().GetNextNode(link));

                if (pGraph->GetLinkManager().GetNextNode(link) == pGraph->m_safeFirstIndex)
                {
                    pGraph->GetLinkManager().SetRadius(link, -1.0f);
                    continue;
                }

                if (pNextNode->navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE))
                {
                    pGraph->GetLinkManager().SetRadius(link, 100.0f);
                    pGraph->GetLinkManager().SetMaxWaterDepth(link, 0.0f);
                    pGraph->GetLinkManager().SetMinWaterDepth(link, 0.0f);
                    pGraph->GetLinkManager().GetEdgeCenter(link) = 0.5f * (pCurrent->GetPos() + pNextNode->GetPos());
                    CalculateLinkExposure(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pCurrent, link);
                    continue;
                }

                if (pNextNode->navType == IAISystem::NAV_TRIANGULAR)
                {
                    if (processedTriTriLinks.insert(link).second)
                    {
                        CalculateLinkRadius(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pCurrent, link);
                        CalculateLinkExposure(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pCurrent, link);
                        CalculateLinkWater(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pCurrent, link);

                        // if this was a non-forbidden link, use the result for the reciprocal link, unless
                        // that is forbidden
                        if (pGraph->GetLinkManager().GetRadius(link) > 0.0f)
                        {
                            for (unsigned incomingLink = pNextNode->firstLinkIndex; incomingLink; incomingLink = pGraph->GetLinkManager().GetNextLink(incomingLink))
                            {
                                GraphNode* pIncomingLinkTarget = pGraph->GetNodeManager().GetNode(pGraph->GetLinkManager().GetNextNode(incomingLink));
                                if (pIncomingLinkTarget == pCurrent)
                                {
                                    if (processedTriTriLinks.insert(incomingLink).second)
                                    {
                                        if (IsPathForbidden(pNextNode->GetPos(), pCurrent->GetPos()))
                                        {
                                            pGraph->GetLinkManager().SetRadius(incomingLink, -1.0f);
                                            pGraph->GetLinkManager().GetEdgeCenter(incomingLink).z = p3DEngine->GetTerrainElevation(pGraph->GetLinkManager().GetEdgeCenter(incomingLink).x, pGraph->GetLinkManager().GetEdgeCenter(incomingLink).y);
                                        }
                                        else
                                        {
                                            pGraph->GetLinkManager().SetRadius(incomingLink, pGraph->GetLinkManager().GetRadius(link));
                                            pGraph->GetLinkManager().GetEdgeCenter(incomingLink) = pGraph->GetLinkManager().GetEdgeCenter(link); // TODO: don't set shared member twice
                                        }
                                        assert((incomingLink >> 1) == (link >> 1));
                                        if ((incomingLink >> 1) != (link >> 1))
                                        {
                                            pGraph->GetLinkManager().SetExposure(incomingLink, pGraph->GetLinkManager().GetExposure(link));
                                        }
                                        pGraph->GetLinkManager().SetMaxWaterDepth(incomingLink, pGraph->GetLinkManager().GetMaxWaterDepth(link));
                                        pGraph->GetLinkManager().SetMinWaterDepth(incomingLink, pGraph->GetLinkManager().GetMinWaterDepth(link));
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // all building links have maximum pass radiuses
            for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
            {
                GraphNode* pNextNode = pGraph->GetNodeManager().GetNode(pGraph->GetLinkManager().GetNextNode(link));
                if (pNextNode->navType == IAISystem::NAV_WAYPOINT_HUMAN)
                {
                    pGraph->GetLinkManager().SetRadius(link, 100);
                }
            }
        }
    }
    CalculateNoncollidableLinks();
}

//====================================================================
// CalculateLinkExposure
//====================================================================
void CNavigation::CalculateLinkExposure(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link)
{
    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();
    static const float interval = 2.0f;
    static const float boxSize = 2.0f;
    static const float boxHeight = 2.0f;
    unsigned nextNodeIndex = linkManager.GetNextNode(link);
    GraphNode* pNextNode = nodeManager.GetNode(nextNodeIndex);
    float dist = (pNextNode->GetPos() - pNode->GetPos()).GetLength();
    int numPts = 1 + (int) (dist / interval);
    AABB box(Vec3(-boxSize, -boxSize, 0.0f), Vec3(boxSize, boxSize, boxHeight));
    float coverNum = 0.0f;
    for (int iPt = 0; iPt < numPts; ++iPt)
    {
        Vec3 pt;
        float distTotal = -1.0f;
        float distToMid = -1.0f;
        GetIntermediatePosition(pt, pNode->GetPos(), linkManager.GetEdgeCenter(link), pNextNode->GetPos(),
            distTotal, distToMid, iPt, numPts);

        IPhysicalEntity**   ppList = NULL;
        int numEntities = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(pt + box.min, pt + box.max, ppList, ent_static);
        float thisCoverNum = 0.0f;
        for (int i = 0; i < numEntities; ++i)
        {
            IPhysicalEntity* pCurrent = ppList[i];
            pe_status_pos status;
            pCurrent->GetStatus(&status);
            float objTop = status.pos.z + status.BBox[1].z;
            if (objTop > pt.z + 2.0f)
            {
                thisCoverNum += 0.5f;
            }
            else if (objTop > pt.z + 1.0f)
            {
                thisCoverNum += 0.25f;
            }

            if (thisCoverNum >= 1.0f)
            {
                thisCoverNum = 1.0f;
                break;
            }
        }
        coverNum += thisCoverNum;
    }
    linkManager.SetExposure(link, 1.0f - (coverNum / (1.0f + numPts)));
}

//====================================================================
// CalculateLinkDeepWaterFraction
//====================================================================
void CNavigation::CalculateLinkWater(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link)
{
    linkManager.SetMaxWaterDepth(link, 0.0f);
    linkManager.SetMinWaterDepth(link, 1000.0f);

    Vec3 nodePos = pNode->GetPos();
    unsigned nextNodeIndex = linkManager.GetNextNode(link);
    GraphNode* pNextNode = nodeManager.GetNode(nextNodeIndex);
    Vec3 otherPos = pNextNode->GetPos();

    if (!IsPointInWaterAreas(nodePos) && !IsPointInWaterAreas(otherPos))
    {
        return;
    }

    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();
    static const float interval = 2.0f;
    float dist = (otherPos - nodePos).GetLength();
    int numPts = 1 + (int) (dist / interval);
    for (int iPt = 0; iPt < numPts; ++iPt)
    {
        Vec3 pt;
        float distTotal = -1.0f;
        float distToMid = -1.0f;
        GetIntermediatePosition(pt, nodePos, linkManager.GetEdgeCenter(link), otherPos,
            distTotal, distToMid, iPt, numPts);
        float waterLevel = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(pt) : p3DEngine->GetWaterLevel(&pt);
        if (waterLevel != WATER_LEVEL_UNKNOWN)
        {
            float terrainZ = p3DEngine->GetTerrainElevation(pt.x, pt.y);
            float depth = waterLevel - terrainZ;
            if (depth > linkManager.GetMaxWaterDepth(link))
            {
                linkManager.SetMaxWaterDepth(link, depth);
            }
            if (depth < linkManager.GetMinWaterDepth(link))
            {
                linkManager.SetMinWaterDepth(link, depth);
            }
        }
    }
    if (linkManager.GetMinWaterDepth(link) < 0.0f)
    {
        linkManager.SetMinWaterDepth(link, 0.0f);
    }
    if (linkManager.GetMinWaterDepth(link) > linkManager.GetMaxWaterDepth(link))
    {
        linkManager.SetMinWaterDepth(link, linkManager.GetMaxWaterDepth(link));
    }
}

//====================================================================
// CalculateDistanceToCollidable
//====================================================================
float CNavigation::CalculateDistanceToCollidable(const Vec3& pos, float maxDist) const
{
    CGraph* pGraph = m_pGraph;

    std::vector< std::pair<float, unsigned> > nodes;
    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    allNodes.GetAllNodesWithinRange(nodes, pos, maxDist, IAISystem::NAV_TRIANGULAR);
    if (nodes.empty())
    {
        return maxDist;
    }
    // sort by increasing distance
    std::sort(nodes.begin(), nodes.end());

    unsigned nNodes = nodes.size();
    for (unsigned iNode = 0; iNode < nNodes; ++iNode)
    {
        const GraphNode* pNode = pGraph->GetNodeManager().GetNode(nodes[iNode].second);
        unsigned nVerts = pNode->GetTriangularNavData()->vertices.size();
        for (unsigned iVert = 0; iVert < nVerts; ++iVert)
        {
            unsigned index = pNode->GetTriangularNavData()->vertices[iVert];
            const ObstacleData& ob = m_VertexList.GetVertex(index);
            if (!ob.IsCollidable())
            {
                continue;
            }
            float dist = Distance::Point_Point2D(ob.vPos, pos);
            dist -= ob.fApproxRadius > 0 ? ob.fApproxRadius : 0.0f;
            if (dist > maxDist)
            {
                continue;
            }
            return dist;
        }
    }
    // nothing found
    return maxDist;
}


//====================================================================
// CalculateLinkRadius
//====================================================================
void CNavigation::CalculateLinkRadius(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode* pNode, unsigned link)
{
    if ((linkManager.GetStartIndex(link) >= pNode->GetTriangularNavData()->vertices.size()) || (linkManager.GetStartIndex(link) < 0) ||
        (linkManager.GetEndIndex(link) >= pNode->GetTriangularNavData()->vertices.size()) || (linkManager.GetEndIndex(link) < 0))
    {
        AIWarning("found bad triangle index!!");
        return;
    }

    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();
    unsigned nextNodeIndex = linkManager.GetNextNode(link);
    GraphNode* pNextNode = nodeManager.GetNode(nextNodeIndex);
    if (IsPathForbidden(pNode->GetPos(), pNextNode->GetPos()))
    {
        linkManager.SetRadius(link, -1.0f);
    }
    else
    {
        ObstacleData& obStart = GetVertexList().ModifyVertex(pNode->GetTriangularNavData()->vertices[linkManager.GetStartIndex(link)]);
        ObstacleData& obEnd   = GetVertexList().ModifyVertex(pNode->GetTriangularNavData()->vertices[linkManager.GetEndIndex(link)]);
        Vec3 vStart = obStart.vPos;
        Vec3 vEnd = obEnd.vPos;
        vStart.z = p3DEngine->GetTerrainElevation(vStart.x, vStart.y);
        vEnd.z = p3DEngine->GetTerrainElevation(vEnd.x, vEnd.y);

        Vec3 bboxsize(1.f, 1.f, 1.f);
        IPhysicalEntity* pEndPhys = 0;
        IPhysicalEntity* pStartPhys = 0;

        IPhysicalEntity** pEntityList;
        int nCount = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(vStart - bboxsize, vStart + bboxsize,
                pEntityList, ent_static);
        int i = 0;
        while (i < nCount)
        {
            pe_status_pos ppos;
            pEntityList[i]->GetStatus(&ppos);
            ppos.pos.z = vStart.z;
            if (IsEquivalent(ppos.pos, vStart, 0.001f))
            {
                pStartPhys = pEntityList[i];
                break;
            }
            ++i;
        }

        nCount = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(vEnd - bboxsize, vEnd + bboxsize, pEntityList, ent_static);
        i = 0;
        while (i < nCount)
        {
            pe_status_pos ppos;
            pEntityList[i]->GetStatus(&ppos);
            ppos.pos.z = vEnd.z;
            if (IsEquivalent(ppos.pos, vEnd, 0.001f))
            {
                pEndPhys = pEntityList[i];
                break;
            }
            ++i;
        }

        Vec3 vStartCut = vStart;
        Vec3 vEndCut = vEnd;
        Vec3 sideDir = vEnd - vStart;
        float sideHorLen = sideDir.GetLength2D();
        float sideLen = sideDir.NormalizeSafe();

        static float testLen = 10.0f;
        static float testRadius = 1.0f;
        ray_hit se_hit;

        if (pEndPhys)
        {
            float overrideRadius = -1.0f;
            IRenderNode* pRN = (IRenderNode*)pEndPhys->GetForeignData(PHYS_FOREIGN_ID_STATIC);
            if (pRN)
            {
                IStatObj* pStatObj = pRN->GetEntityStatObj();
                if (pStatObj)
                {
                    pe_status_pos status;
                    status.ipart = 0;
                    pEndPhys->GetStatus(&status);
                    float r = pStatObj->GetAIVegetationRadius();
                    if (r > 0.0f)
                    {
                        overrideRadius = r * status.scale;
                    }
                    else
                    {
                        overrideRadius = -1.0f;
                    }
                }
            }
            if (overrideRadius >= 0.0f)
            {
                vEndCut -= sideDir * overrideRadius;
            }
            else
            {
                Vec3 vModStart = vStart - testLen * sideDir;
                if (obEnd.IsCollidable() && m_pSystem->GetIPhysicalWorld()->CollideEntityWithBeam(pEndPhys, vModStart, (vEnd - vModStart), testRadius, &se_hit))
                {
                    float distToHit = testRadius + se_hit.dist;
                    if (distToHit < testLen)
                    {
                        // NOTE Jul 27, 2007: <pvl> collision occured even *before* the beam
                        // arrived to vStart.  That probably means that the whole edge
                        // (vStart,vEnd) is inside pEndPhys so mark it unpassable now.
                        linkManager.SetRadius(link, -1.0f);
                        linkManager.SetEdgeCenter(link, 0.5f * (vStart + vEnd));
                        return;
                    }
                    vEndCut = vModStart + distToHit * (vEnd - vModStart).GetNormalizedSafe();
                }
            }
        }

        if (pStartPhys)
        {
            float overrideRadius = -1.0f;
            IRenderNode* pRN = (IRenderNode*)pStartPhys->GetForeignData(PHYS_FOREIGN_ID_STATIC);
            if (pRN)
            {
                IStatObj* pStatObj = pRN->GetEntityStatObj();
                if (pStatObj)
                {
                    pe_status_pos status;
                    status.ipart = 0;
                    pStartPhys->GetStatus(&status);
                    float r = pStatObj->GetAIVegetationRadius();
                    if (r > 0.0f)
                    {
                        overrideRadius = r * status.scale;
                    }
                    else
                    {
                        overrideRadius = -1.0f;
                    }
                }
            }
            if (overrideRadius >= 0.0f)
            {
                vStartCut += sideDir * overrideRadius;
            }
            else
            {
                Vec3 vModEnd = vEnd + testLen * sideDir;
                if (obStart.IsCollidable() && m_pSystem->GetIPhysicalWorld()->CollideEntityWithBeam(pStartPhys, vModEnd, (vStart - vModEnd), testRadius, &se_hit))
                {
                    float distToHit = testRadius + se_hit.dist;
                    if (distToHit < testLen)
                    {
                        // NOTE Jul 27, 2007: <pvl> collision occured even *before* the beam
                        // arrived to vEnd.  That probably means that the whole edge
                        // (vStart,vEnd) is inside pStartPhys so mark it unpassable now.
                        linkManager.SetRadius(link, -1.0f);
                        linkManager.SetEdgeCenter(link, 0.5f * (vStart + vEnd));
                        return;
                    }
                    vStartCut = vModEnd + distToHit * (vStart - vModEnd).GetNormalizedSafe();
                }
            }
        }

        float rStart = Distance::Point_Point2D(vStartCut, vStart);
        float rEnd = Distance::Point_Point2D(vEndCut, vEnd);
        //    Limit(rStart, 0.0f, sideHorLen);
        //    Limit(rEnd, 0.0f, sideHorLen);
        if (obStart.fApproxRadius < rStart)
        {
            obStart.fApproxRadius = rStart;
        }
        if (obEnd.fApproxRadius < rEnd)
        {
            obEnd.fApproxRadius = rEnd;
        }

        Vec3 NormStart = vStartCut, NormEnd = vEndCut;
        NormStart.z = NormEnd.z = 0;
        vStartCut.z = vEndCut.z = 0.0f;
        Vec3 vStartZero = vStart;
        vStartZero.z = 0.0f;
        float dStart2 = (NormStart - vStartZero).GetLengthSquared();
        float dEnd2 = (NormEnd - vStartZero).GetLengthSquared();
        linkManager.SetRadius(link, 0.5f * (NormEnd - NormStart).GetLength());
        if (dStart2 < dEnd2)
        {
            // sometimes the start/end cut positions seem to be wrong - i.e.
            // the modpoint isn't actually on the line joining the points.
            //                          link.vEdgeCenter = 0.5f * (vStartCut+vEndCut);
            Vec3 delta = vEnd - vStart;
            delta.z = 0.0f;
            delta.NormalizeSafe();
            linkManager.SetEdgeCenter(link, vStart + 0.5f * (sqrtf(dStart2) + sqrtf(dEnd2)) * delta);
        }
        else
        {
            linkManager.SetEdgeCenter(link, 0.5f * (vStart + vEnd));
            linkManager.SetRadius(link, -linkManager.GetRadius(link));
        }
    }
    Vec3 vEdgeCenter = linkManager.GetEdgeCenter(link);
    vEdgeCenter.z = p3DEngine->GetTerrainElevation(linkManager.GetEdgeCenter(link).x, linkManager.GetEdgeCenter(link).y);
    linkManager.SetEdgeCenter(link, vEdgeCenter);
}

//====================================================================
// CalculateNoncollidableLinks
//====================================================================
void CNavigation::CalculateNoncollidableLinks()
{
    CGraph* pGraph = GetGraph();
    I3DEngine* p3DEngine = m_pSystem->GetI3DEngine();
    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
    while (unsigned currentNodeIndex = it.Increment())
    {
        GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

        for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
        {
            unsigned startIndex = pCurrent->GetTriangularNavData()->vertices[pGraph->GetLinkManager().GetStartIndex(link)];
            unsigned endIndex = pCurrent->GetTriangularNavData()->vertices[pGraph->GetLinkManager().GetEndIndex(link)];
            const ObstacleData& obStart = GetVertexList().GetVertex(startIndex);
            const ObstacleData& obEnd = GetVertexList().GetVertex(endIndex);
            static float maxDist = 10.0f;
            if (obStart.IsCollidable() && obEnd.IsCollidable())
            {
                continue;
            }
            else if (!obStart.IsCollidable() && !obEnd.IsCollidable())
            {
                pGraph->GetLinkManager().SetEdgeCenter(link, 0.5f * (obStart.vPos + obEnd.vPos)); // TODO: Don't set shared member twice
                float linkRadius = 0.5f * Distance::Point_Point2D(obStart.vPos, obEnd.vPos);
                float distanceToCollidable = CalculateDistanceToCollidable(pGraph->GetLinkManager().GetEdgeCenter(link), maxDist);
                float radius = min(maxDist, distanceToCollidable);
                if (radius < linkRadius)
                {
                    radius = linkRadius;
                }
                pGraph->GetLinkManager().SetRadius(link, radius);
            }
            else if (!obStart.IsCollidable())
            {
                float distanceToCollidable = CalculateDistanceToCollidable(pGraph->GetLinkManager().GetEdgeCenter(link), maxDist);
                Vec3 dir = (obEnd.vPos - obStart.vPos).GetNormalizedSafe();
                float linkRadius = pGraph->GetLinkManager().GetRadius(link);
                if (linkRadius <= 0.0f)
                {
                    continue;
                }
                Vec3 origEnd = pGraph->GetLinkManager().GetEdgeCenter(link) + dir * linkRadius;
                float newDist = Distance::Point_Point2D(origEnd, obStart.vPos);
                if (newDist < distanceToCollidable)
                {
                    newDist = distanceToCollidable;
                }
                Vec3 newStart = origEnd - dir * newDist;
                pGraph->GetLinkManager().GetEdgeCenter(link) = 0.5f * (origEnd + newStart); // TODO: Don't set shared member twice
                pGraph->GetLinkManager().GetEdgeCenter(link).z = p3DEngine->GetTerrainElevation(pGraph->GetLinkManager().GetEdgeCenter(link).x, pGraph->GetLinkManager().GetEdgeCenter(link).y);
                pGraph->GetLinkManager().SetRadius(link, newDist * 0.5f);
            }
            else if (!obEnd.IsCollidable())
            {
                float distanceToCollidable = CalculateDistanceToCollidable(pGraph->GetLinkManager().GetEdgeCenter(link), maxDist);
                Vec3 dir = (obEnd.vPos - obStart.vPos).GetNormalizedSafe();
                float linkRadius = pGraph->GetLinkManager().GetRadius(link);
                if (linkRadius <= 0.0f)
                {
                    continue;
                }
                Vec3 origStart = pGraph->GetLinkManager().GetEdgeCenter(link) - dir * linkRadius;
                float newDist = Distance::Point_Point2D(origStart, obEnd.vPos);
                if (newDist < distanceToCollidable)
                {
                    newDist = distanceToCollidable;
                }
                Vec3 newEnd = origStart + dir * newDist;
                pGraph->GetLinkManager().GetEdgeCenter(link) = 0.5f * (origStart + newEnd);
                pGraph->GetLinkManager().GetEdgeCenter(link).z = p3DEngine->GetTerrainElevation(pGraph->GetLinkManager().GetEdgeCenter(link).x, pGraph->GetLinkManager().GetEdgeCenter(link).y);
                pGraph->GetLinkManager().SetRadius(link, newDist * 0.5f);
            }
        }
    }
}

//===================================================================
// DisableThinNodesNearForbidden
//===================================================================
void CNavigation::DisableThinNodesNearForbidden()
{
    CGraph* pGraph = GetGraph();
    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
    while (unsigned currentNodeIndex = it.Increment())
    {
        GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

        bool badNode = false;

        const unsigned nVerts = pCurrent->GetTriangularNavData()->vertices.size();
        if (nVerts != 3)
        {
            continue;
        }

        for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
        {
            unsigned nextNodeIndex = pGraph->GetLinkManager().GetNextNode(link);
            const GraphNode* pNext = pGraph->GetNodeManager().GetNode(nextNodeIndex);
            if (pNext->navType != IAISystem::NAV_TRIANGULAR)
            {
                continue;
            }
            if (pGraph->GetLinkManager().GetRadius(link) > 0.0f)
            {
                continue;
            }

            unsigned iVertStart = pGraph->GetLinkManager().GetStartIndex(link);
            unsigned iVertEnd = pGraph->GetLinkManager().GetEndIndex(link);
            // to get the other note that the some of the vertex numbers = 0 + 1 + 2 = 3
            unsigned iVertOther = 3 - (iVertStart + iVertEnd);

            const ObstacleData& odStart = GetVertexList().GetVertex(pCurrent->GetTriangularNavData()->vertices[iVertStart]);
            const ObstacleData& odEnd = GetVertexList().GetVertex(pCurrent->GetTriangularNavData()->vertices[iVertEnd]);
            const ObstacleData& od = GetVertexList().GetVertex(pCurrent->GetTriangularNavData()->vertices[iVertOther]);
            if (od.fApproxRadius <= 0.0f)
            {
                continue;
            }

            Lineseg seg(odStart.vPos, odEnd.vPos);
            float junk;
            float distToEdgeSq = Distance::Point_Lineseg2DSq(od.vPos, seg, junk);

            if (distToEdgeSq < square(od.fApproxRadius))
            {
                badNode = true;
                break;
            }
        } // loop over links

        if (badNode)
        {
            for (unsigned link = pCurrent->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
            {
                unsigned nextNodeIndex = pGraph->GetLinkManager().GetNextNode(link);
                GraphNode* pNext = pGraph->GetNodeManager().GetNode(nextNodeIndex);
                if (pNext->navType != IAISystem::NAV_TRIANGULAR)
                {
                    continue;
                }
                unsigned incomingLink = pNext->GetLinkTo(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pCurrent);
                if (pGraph->GetLinkManager().GetRadius(incomingLink) < 0.0f)
                {
                    continue;
                }
                pGraph->GetLinkManager().SetRadius(incomingLink, -3.0f);
            }
        } // bad node
    } // loop over all nodes
}

//===================================================================
// MarkForbiddenTriangles
//===================================================================
void CNavigation::MarkForbiddenTriangles()
{
    CGraph* pGraph = GetGraph();
    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
    while (unsigned currentNodeIndex = it.Increment())
    {
        GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

        bool badNode = false;

        Vec3 pos = pCurrent->GetPos();
        bool forbidden = IsPointInForbiddenRegion(pos, 0, true);
        pCurrent->GetTriangularNavData()->isForbidden = forbidden;
        forbidden = IsPointInForbiddenRegion(pos, 0, false);
        pCurrent->GetTriangularNavData()->isForbiddenDesigner = forbidden;
    }
}

//-----------------------------------------------------------------------------------------------------------
void CNavigation::CreateNewTriangle(const ObstacleData& _od1, const ObstacleData& _od2, const ObstacleData& _od3, bool tag, ListNodeIds& lstNewNodes)
{
    CGraph* pGraph = GetGraph();

    // Since we add vertices don't use reference arguments since the reference may become invalid!

    ObstacleData od1 = _od1;
    ObstacleData od2 = _od2;
    ObstacleData od3 = _od3;

    int vIdx1 = GetVertexList().AddVertex(od1);
    int vIdx2 = GetVertexList().AddVertex(od2);
    int vIdx3 = GetVertexList().AddVertex(od3);

    if (vIdx1 == vIdx2 || vIdx1 == vIdx3 || vIdx2 == vIdx3)
    {
        // it's degenerate
        return;
    }

    unsigned nodeIndex = pGraph->CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3(ZERO));
    GraphNode* pNewNode = pGraph->GetNodeManager().GetNode(nodeIndex);

    pNewNode->GetTriangularNavData()->vertices.push_back(vIdx1);
    pNewNode->GetTriangularNavData()->vertices.push_back(vIdx2);
    pNewNode->GetTriangularNavData()->vertices.push_back(vIdx3);

    pGraph->FillGraphNodeData(nodeIndex);
    lstNewNodes.push_back(nodeIndex);
    if (tag)
    {
        pGraph->TagNode(nodeIndex);
    }

    return;
}

//====================================================================
// SetUseAutoNavigation
//====================================================================
void CNavigation::SetUseAutoNavigation(const char* szPathName, EWaypointConnections waypointConnections)
{
    SpecialAreaMap::iterator di;
    di = m_mapSpecialAreas.find(szPathName);

    if (di == m_mapSpecialAreas.end())
    {
        AIWarning("CAISystem::SetUseAutoNavigation Area %s not found", szPathName);
        return;
    }

    SpecialArea& sa = di->second;

    sa.waypointConnections = waypointConnections;

    GetWaypointHumanNavRegion()->RemoveAutoLinksInBuilding(sa.nBuildingID);
    GetWaypointHumanNavRegion()->ReconnectWaypointNodesInBuilding(sa.nBuildingID);
}

//====================================================================
// GetExtraLinkCost
//====================================================================
float CNavigation::GetExtraLinkCost(const Vec3& pos1, const Vec3& pos2, const AABB& linkAABB, const SExtraLinkCostShape& shape) const
{
    // Danny todo implement this properly. For now just return the full factor if there's any intersection - really
    // we should return just a fraction of the factor, depending on the extent of intersection.

    if (Overlap::Point_Polygon2D(pos1, shape.shape, &shape.aabb) ||
        Overlap::Point_Polygon2D(pos2, shape.shape, &shape.aabb) ||
        Overlap::Lineseg_Polygon2D(Lineseg(pos1, pos2), shape.shape, &shape.aabb))
    {
        return shape.costFactor;
    }
    else
    {
        return 0.0f;
    }
}

//====================================================================
// GetExtraLinkCost
//====================================================================
float CNavigation::GetExtraLinkCost(const Vec3& pos1, const Vec3& pos2) const
{
    if (m_mapExtraLinkCosts.empty())
    {
        return 0.0f;
    }

    AABB linkAABB(AABB::RESET);
    linkAABB.Add(pos1);
    linkAABB.Add(pos2);

    float factor = 0.0f;
    for (ExtraLinkCostShapeMap::const_iterator it = m_mapExtraLinkCosts.begin(); it != m_mapExtraLinkCosts.end(); ++it)
    {
        const SExtraLinkCostShape& shape = it->second;
        if (Overlap::AABB_AABB(shape.aabb, linkAABB))
        {
            float cost = GetExtraLinkCost(pos1, pos2, linkAABB, shape);
            if (cost < 0.0f)
            {
                return -1.0f;
            }
            factor += cost;
        }
    }
    return factor;
}

//====================================================================
// GetNavRegion
//====================================================================
CNavRegion* CNavigation::GetNavRegion(IAISystem::ENavigationType type, const class CGraph* pGraph)
{
    switch (type)
    {
    case IAISystem::NAV_TRIANGULAR:
        return GetTriangularNavRegion();
    case IAISystem::NAV_WAYPOINT_HUMAN:
        return m_pWaypointHumanNavRegion;
    case IAISystem::NAV_FLIGHT:
        return m_pFlightNavRegion;
    case IAISystem::NAV_VOLUME:
        return m_pVolumeNavRegion;
    case IAISystem::NAV_ROAD:
        return m_pRoadNavRegion;
    case IAISystem::NAV_SMARTOBJECT:
        return m_pSmartObjectNavRegion;
    case IAISystem::NAV_UNSET:
        return 0;
    }
    return 0;
}


//====================================================================
// Generate3DVolumes
//====================================================================
void CNavigation::Generate3DVolumes(const char* szLevel, const char* szMission)
{
    if (!szLevel || !szMission)
    {
        return;
    }

    // Danny todo get these from the entities/spawners in the level
    m_3DPassRadii.clear();
    m_3DPassRadii.push_back(3.0f);
    m_3DPassRadii.push_back(1.0f);

    char fileNameVolume[1024];
    sprintf_s(fileNameVolume, "%s/v3d%s.bai", szLevel, szMission);

    m_pVolumeNavRegion->Clear();

    GetGraph()->CheckForEmpty(IAISystem::NAV_VOLUME);

    // let's generate 3D nav volumes
    AILogProgress(" Volume Generation started.");
    m_pVolumeNavRegion->Generate(szLevel, szMission);
    AILogProgress(" Volume Generation complete.");
    // volumes generation over ---------------------
    m_pVolumeNavRegion->WriteToFile(fileNameVolume);
    m_nNumBuildings = 0;
    AILogProgress(" Volumes written to file finished.");
}

//====================================================================
// Generate3DDebugVoxels
//====================================================================
void CNavigation::Generate3DDebugVoxels()
{
    GetVolumeNavRegion()->Generate3DDebugVoxels();
}

//====================================================================
// GenerateFlightNavigation
//====================================================================
void CNavigation::GenerateFlightNavigation(const char* szLevel, const char* szMission)
{
    if (!szLevel || !szMission)
    {
        return;
    }

    char fileNameFlight[1024];
    sprintf_s(fileNameFlight, "%s/fnav%s.bai", szLevel, szMission);

    m_pFlightNavRegion->Clear();

    GetGraph()->CheckForEmpty(IAISystem::NAV_FLIGHT);

    AILogProgress(" FlightNavRegion started.");

    bool bypass = true;
    for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        SpecialArea& sa = si->second;
        if (sa.type == SpecialArea::TYPE_FLIGHT)
        {
            bypass = false;
        }
    }

    if (bypass)
    {
        AILogProgress("Bypassing flight nav region processing");
        IPhysicalEntity**   pObstacles = 0;
        int obstacleCount = 0;
        std::list<SpecialArea*> shortcuts;
        static size_t childSubDiv = 16;
        static size_t terrainDownSample = 256;
        m_pFlightNavRegion->Process(m_pSystem->GetI3DEngine(), pObstacles, obstacleCount, shortcuts, childSubDiv, terrainDownSample);
    }
    else
    {
        // get only static physical entities (trees, buildings etc...)
        Vec3 min, max;
        IPhysicalEntity**   pObstacles = 0;
        int obstacleCount = 0;
        float fTSize = (float) m_pSystem->GetI3DEngine()->GetTerrainSize();

        min.Set(0, 0, -100000);
        max.Set(fTSize, fTSize, 100000);

        obstacleCount = m_pSystem->GetIPhysicalWorld()->GetEntitiesInBox(min, max, pObstacles, ent_static | ent_ignore_noncolliding);

        // Collect shortcuts.
        std::list<SpecialArea*> shortcuts;
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (sa.type == SpecialArea::TYPE_FLIGHT)
            {
                shortcuts.push_back(&sa);
            }
        }

        m_pFlightNavRegion->Process(m_pSystem->GetI3DEngine(), pObstacles, obstacleCount, shortcuts);
    }

    m_pFlightNavRegion->WriteToFile(fileNameFlight);

    AILogProgress(" FlightNavRegion done.");
}

//====================================================================
// ReconnectWaypointNode
//====================================================================
void CNavigation::ReconnectWaypointNodesInBuilding(int nBuildingID)
{
    GetWaypointHumanNavRegion()->ReconnectWaypointNodesInBuilding(nBuildingID);
}

//====================================================================
// ReconnectAllWaypointNodes
//====================================================================
void CNavigation::ReconnectAllWaypointNodes()
{
    ClearBadFloorRecords();

    float startTime = gEnv->pTimer->GetAsyncCurTime();

    CGraph* pGraph = GetGraph();
    // go through all nodes and reevaluate their building ID, and if they're an entrance/exit then also
    // reset the pass radius
    CAllNodesContainer& allNodes = pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE);
    I3DEngine* p3dEngine = m_pSystem->GetI3DEngine();
    while (unsigned currentNodeIndex = it.Increment())
    {
        GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

        pCurrent->GetWaypointNavData()->nBuildingID = -1;
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (((sa.type == SpecialArea::TYPE_WAYPOINT_HUMAN && pCurrent->navType == IAISystem::NAV_WAYPOINT_HUMAN) ||
                 (sa.type == SpecialArea::TYPE_WAYPOINT_3DSURFACE && pCurrent->navType == IAISystem::NAV_WAYPOINT_3DSURFACE)) &&
                IsPointInSpecialArea(pCurrent->GetPos(), sa))
            {
                pCurrent->GetWaypointNavData()->nBuildingID = sa.nBuildingID;
                // adjust the entrance/exit radius
                if (pCurrent->navType == IAISystem::NAV_WAYPOINT_HUMAN &&
                    pCurrent->GetWaypointNavData()->type == WNT_ENTRYEXIT)
                {
                    float radius = 100.0f;
                    if (!sa.bVehiclesInHumanNav)
                    {
                        radius = 2.0f;
                    }

                    for (unsigned linkg = pCurrent->firstLinkIndex; linkg; linkg = pGraph->GetLinkManager().GetNextLink(linkg))
                    {
                        unsigned nextNodeIndex = pGraph->GetLinkManager().GetNextNode(linkg);
                        GraphNode* pNextNode = pGraph->GetNodeManager().GetNode(nextNodeIndex);
                        if (pNextNode->navType == IAISystem::NAV_TRIANGULAR)
                        {
                            GraphNode* pOutNode = pNextNode;
                            for (unsigned linki = pOutNode->firstLinkIndex; linki; linki = pGraph->GetLinkManager().GetNextLink(linki))
                            {
                                if (pGraph->GetNodeManager().GetNode(pGraph->GetLinkManager().GetNextNode(linki)) == pCurrent)
                                {
                                    pGraph->GetLinkManager().SetRadius(linki, radius);
                                }
                            }
                        }
                    } // loop over links
                } // if it's waypoint
                break;
            }
        }
        if (pCurrent->GetWaypointNavData()->nBuildingID == -1)
        {
            AIWarning("Cannot find nav modifier for AI node at position: (%5.2f, %5.2f, %5.2f)", pCurrent->GetPos().x, pCurrent->GetPos().y, pCurrent->GetPos().z);
        }
    }

    SpecialAreaMap::iterator si;
    for (si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        SpecialArea& sa = si->second;
        if (sa.waypointConnections > WPCON_DESIGNER_PARTIAL)
        {
            AILogProgress("Reconnecting waypoints in %s", si->first.c_str());
            ReconnectWaypointNodesInBuilding(sa.nBuildingID);
        }
        else
        {
            AILogProgress("Just disconnecting auto-waypoints in %s", si->first.c_str());
            GetWaypointHumanNavRegion()->RemoveAutoLinksInBuilding(sa.nBuildingID);
        }
    }

    AILogProgress("Inserting roads into navigation");
    m_pRoadNavRegion->ReinsertIntoGraph();

    float endTime = gEnv->pTimer->GetAsyncCurTime();

    const TBadFloorRecords& badFloorRecords = GetBadFloorRecords();
    for (TBadFloorRecords::const_iterator bit = badFloorRecords.begin(); bit != badFloorRecords.end(); ++bit)
    {
        AIWarning("Bad AI point position at position: (%5.2f, %5.2f, %5.2f)", bit->pos.x, bit->pos.y, bit->pos.z);
    }

    AILogProgress("Reconnected all waypoints in %5.2f seconds", endTime - startTime);
}

std::vector <const SpecialArea*> CNavigation::GetExclusionVolumes (const SpecialArea& sa, const string& agentType)
{
    std::vector <const SpecialArea*> exclusionVols;
    exclusionVols.reserve (m_mapSpecialAreas.size());

    AABB thisAABB ((sa.GetAABB ()));
    thisAABB.Add (thisAABB.max + Vec3 (0, 0, sa.fHeight));


    return exclusionVols;
}

//====================================================================
// IsFlightSpaceVoid
// check if the space is void inside of the box which is made by these 8 point
//====================================================================
Vec3 CNavigation::IsFlightSpaceVoid(const Vec3& vPos, const Vec3& vFwd, const Vec3& vWng, const Vec3& vUp)
{
    CFlightNavRegion* pNav = GetFlightNavRegion();
    if (pNav)
    {
        return pNav->IsSpaceVoid(vPos, vFwd, vWng, vUp);
    }

    return Vec3(ZERO);
}

//====================================================================
// CheckNavigationType
// When areas are nested there is an ordering - make this explicit by
// ordering the search over the navigation types
//====================================================================
IAISystem::ENavigationType CNavigation::CheckNavigationType(const Vec3& pos, int& nBuildingID, IVisArea*& pAreaID,
    IAISystem::tNavCapMask navCapMask)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    // make sure each area has a building id
    // Flight/water navigation modifiers are only used to limit the initial navigation
    // area during preprocessing - give them a building id anyway
    for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
    {
        SpecialArea& sa = si->second;
        if (sa.nBuildingID < 0)
        {
            (si->second).nBuildingID = m_BuildingIDManager.GetId();
        }
    }

    if (navCapMask & IAISystem::NAV_WAYPOINT_HUMAN)
    {
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (sa.type == SpecialArea::TYPE_WAYPOINT_HUMAN)
            {
                if (IsPointInSpecialArea(pos, sa))
                {
                    nBuildingID = sa.nBuildingID;
                    I3DEngine* p3dEngine = m_pSystem->GetI3DEngine();
                    pAreaID = p3dEngine->GetVisAreaFromPos(pos);
                    return IAISystem::NAV_WAYPOINT_HUMAN;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_VOLUME)
    {
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (sa.type == SpecialArea::TYPE_VOLUME)
            {
                if (IsPointInSpecialArea(pos, sa))
                {
                    nBuildingID = sa.nBuildingID;
                    I3DEngine* p3dEngine = m_pSystem->GetI3DEngine();
                    pAreaID = p3dEngine->GetVisAreaFromPos(pos);
                    return IAISystem::NAV_VOLUME;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_FLIGHT)
    {
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (sa.type == SpecialArea::TYPE_FLIGHT)
            {
                if (IsPointInSpecialArea(pos, sa))
                {
                    return IAISystem::NAV_FLIGHT;
                }
            }
        }
        // if (navCapMask != IAISystem::NAVMASK_ALL)
        //   AIWarning("No flight AI nav modifier around point (%5.2f, %5.2f, %5.2f)", pos.x, pos.y, pos.z);
    }

    if (navCapMask & IAISystem::NAV_FREE_2D)
    {
        for (SpecialAreaMap::iterator si = m_mapSpecialAreas.begin(); si != m_mapSpecialAreas.end(); ++si)
        {
            SpecialArea& sa = si->second;
            if (sa.type == SpecialArea::TYPE_FREE_2D)
            {
                if (IsPointInSpecialArea(pos, sa))
                {
                    nBuildingID = sa.nBuildingID;
                    I3DEngine* p3dEngine = m_pSystem->GetI3DEngine();
                    pAreaID = p3dEngine->GetVisAreaFromPos(pos);
                    return IAISystem::NAV_FREE_2D;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_TRIANGULAR)
    {
        return IAISystem::NAV_TRIANGULAR;
    }

    if (navCapMask & IAISystem::NAV_FREE_2D)
    {
        return IAISystem::NAV_FREE_2D;
    }

    return IAISystem::NAV_UNSET;
}

// TODO Mrz 4, 2008: <pvl> find a better place for this - perhaps Graph?
//===================================================================
// ExitNodeImpossible
//===================================================================
bool CNavigation::ExitNodeImpossible(CGraphLinkManager& linkManager, const GraphNode* pNode, float fRadius) const
{
    for (unsigned gl = pNode->firstLinkIndex; gl; gl = linkManager.GetNextLink(gl))
    {
        if (linkManager.GetRadius(gl) >= fRadius)
        {
            return false;
        }
    }
    return true;
}

void CNavigation::Update () const
{
}

void CNavigation::GetMemoryStatistics(ICrySizer* pSizer)
{
    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "NavRegions");
        if (m_pTriangularNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "Triangular");
            pSizer->AddObject(m_pTriangularNavRegion, m_pTriangularNavRegion->MemStats());
        }
        if (m_pWaypointHumanNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "WaypointHuman");
            pSizer->AddObject(m_pWaypointHumanNavRegion, m_pWaypointHumanNavRegion->MemStats());
        }
        if (m_pVolumeNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "Volume");
            pSizer->AddObject(m_pVolumeNavRegion, m_pVolumeNavRegion->MemStats());
        }
        if (m_pFlightNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "Flight");
            pSizer->AddObject(m_pFlightNavRegion, m_pFlightNavRegion->MemStats());
        }
        if (m_pRoadNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "Road");
            pSizer->AddObject(m_pRoadNavRegion, m_pRoadNavRegion->MemStats());
        }
        if (m_pSmartObjectNavRegion)
        {
            SIZER_SUBCOMPONENT_NAME(pSizer, "Smart");
            pSizer->AddObject(m_pSmartObjectNavRegion, m_pSmartObjectNavRegion->MemStats());
        }
    }

    size_t size = 0;
    for (ShapeMap::iterator itr = m_mapDesignerPaths.begin(); itr != m_mapDesignerPaths.end(); itr++)
    {
        size += (itr->first).capacity();
        size += itr->second.MemStats();
    }
    pSizer->AddObject(&m_mapDesignerPaths, size);

    pSizer->AddObject(&m_forbiddenAreas, m_forbiddenAreas.MemStats());
    pSizer->AddObject(&m_designerForbiddenAreas, m_designerForbiddenAreas.MemStats());
    pSizer->AddObject(&m_forbiddenBoundaries, m_forbiddenBoundaries.MemStats());

    size = 0;
    for (SpecialAreaMap::iterator sit = m_mapSpecialAreas.begin(); sit != m_mapSpecialAreas.end(); sit++)
    {
        size += (sit->first).capacity();
        size += sizeof(SpecialArea);
    }
    pSizer->AddObject(&m_mapSpecialAreas, size);

    {
        // FIXME Feb 1, 2008: <pvl> look into moving m_VertexList out of AISystem as well
        SIZER_SUBCOMPONENT_NAME(pSizer, "Triangle vertex data");
        size = GetVertexList().GetCapacity() * sizeof(ObstacleData);
        pSizer->AddObject(&GetVertexList(), size);
        size = m_vTriangles.capacity() * sizeof(Tri*) + m_vTriangles.size() * sizeof(Tri);
        pSizer->AddObject(&GetVertexList(), size);
        size = 0;
        for (std::vector<Vtx>::iterator vtx = m_vVertices.begin(); vtx != m_vVertices.end(); vtx++)
        {
            size += sizeof(Vtx) + sizeof(int) * vtx->m_lstTris.size();
        }
        pSizer->AddObject(&m_vVertices, size);
    }
}

