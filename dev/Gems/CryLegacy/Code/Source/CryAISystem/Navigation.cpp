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

#include "CryLegacy_precompiled.h"

#include "Navigation.h"
#include "PolygonSetOps/Polygon2d.h"
#include "FlightNavRegion2.h"
#include "Free2DNavRegion.h"

#include "Navigation/CustomNavRegion.h"
#include "DebugDrawContext.h"
#include "Graph.h"
#include "CalculationStopper.h"
#include "CryBufferedFileReader.h"

static const int maxForbiddenNameLen = 1024;

// flag used for debugging/profiling the improvement obtained from using a
// QuadTree for the forbidden shapes. Currently it's actually faster not
// using quadtree - need to experiment more
// kirill - enabling quadtree - is faster on low-spec
bool useForbiddenQuadTree = true;//false;

CNavigation::CNavigation(ISystem* pSystem)
    : m_navDataState(NDS_UNSET)
    , m_nNumBuildings(0)
    , m_pTriangulator(0)
    , m_dynamicLinkUpdateTimeBump(1.0f)
    , m_dynamicLinkUpdateTimeBumpDuration(0)
    , m_dynamicLinkUpdateTimeBumpElapsed(0)

{
    CGraph* pGraph = gAIEnv.pGraph;
}

CNavigation::~CNavigation ()
{
}

bool CNavigation::Init()
{
    m_nNumBuildings = 0;
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
}

// // loads the triangulation for this level and mission
//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::LoadNavigationData(const char* szLevel, const char* szMission)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    m_nNumBuildings = 0;

    CTimeValue startTime = gEnv->pTimer->GetAsyncCurTime();

    CAISystem* pAISystem = GetAISystem();
    CGraph* pGraph = gAIEnv.pGraph;

    pGraph->Clear(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE);

    pGraph->CheckForEmpty(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_VOLUME | IAISystem::NAV_FLIGHT | IAISystem::NAV_ROAD | IAISystem::NAV_FREE_2D);

    char fileNameAreas[1024];

    sprintf_s(fileNameAreas, "%s/areas%s.bai", szLevel, szMission);
    // NOTE Aug 22, 2008: <pvl> we consider nav data OK if 1) graph loaded OK and
    // 2) at least one of vertex list and LNM loaded OK
    m_navDataState = NDS_OK;
    pAISystem->ReadAreasFromFile(fileNameAreas);

    // When using MNM we don't care about the status of the old navigation
    // system, so it's always OK for us
    m_navDataState = NDS_OK;

    CTimeValue endTime = gEnv->pTimer->GetAsyncCurTime();
    AILogLoading("Navigation Data Loaded in %5.2f sec", (endTime - startTime).GetSeconds());
}

static const size_t DynamicLinkConnectionBumpDelayFrames = 10;

float CNavigation::GetDynamicLinkConnectionTimeModifier() const
{
    if ((m_dynamicLinkUpdateTimeBumpElapsed >= DynamicLinkConnectionBumpDelayFrames) &&
        (m_dynamicLinkUpdateTimeBumpElapsed < m_dynamicLinkUpdateTimeBumpDuration + DynamicLinkConnectionBumpDelayFrames))
    {
        return m_dynamicLinkUpdateTimeBump;
    }

    return 1.0f;
}

void CNavigation::BumpDynamicLinkConnectionUpdateTime(float modifier, size_t durationFrames)
{
    m_dynamicLinkUpdateTimeBump = modifier;
    m_dynamicLinkUpdateTimeBumpDuration = durationFrames;
    m_dynamicLinkUpdateTimeBumpElapsed = 0;
}

void CNavigation::Serialize(TSerialize ser)
{
}

void CNavigation::Update(CTimeValue frameStartTime, float frameDeltaTime)
{
    if (m_dynamicLinkUpdateTimeBumpElapsed <= m_dynamicLinkUpdateTimeBumpDuration + DynamicLinkConnectionBumpDelayFrames)
    {
        ++m_dynamicLinkUpdateTimeBumpElapsed;
    }

    UpdateNavRegions();

    // Update path devalues.
    for (ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        it->second.devalueTime = max(0.0f, it->second.devalueTime - frameDeltaTime);
    }
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::UpdateNavRegions()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    //return;
    CAISystem* pAISystem = GetAISystem();
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::FlushSystemNavigation(bool bDeleteAll)
{
    FlushSpecialAreas();
    if (gAIEnv.pGraph)
    {
        if (bDeleteAll)
        {
            // Just reconstruct if unloading the level
            stl::reconstruct(*gAIEnv.pGraph);
        }
        else
        {
            gAIEnv.pGraph->Clear(IAISystem::NAVMASK_ALL);
            gAIEnv.pGraph->ResetIDs();
        }
    }
}

void CNavigation::Reset(IAISystem::EResetReason reason)
{
    if (reason == IAISystem::RESET_ENTER_GAME)
    {
        m_validationErrorMarkers.clear();
    }

    // enable all nav modifiers
    {
        SpecialAreas::iterator it = m_specialAreas.begin();
        SpecialAreas::iterator end = m_specialAreas.end();

        for (; it != end; ++it)
        {
            SpecialArea& specialArea = *it;
            specialArea.bAltered = false;
        }
    }

    // Reset path devalue stuff.
    for (ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        it->second.devalueTime = 0;
    }
}

// copies a designer path into provided list if a path of such name is found
//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::GetDesignerPath(const char* szName, SShape& path) const
{
    ShapeMap::const_iterator di = m_mapDesignerPaths.find(szName);
    if (di != m_mapDesignerPaths.end())
    {
        path = di->second;
        return true;
    }

    return false;
}

//====================================================================
// ReadArea
//====================================================================
void ReadArea(CCryBufferedFileReader& file, int version, string& name, SpecialArea& sa)
{
    unsigned nameLen;
    file.ReadType(&nameLen);
    char tmpName[1024];
    file.ReadType(&tmpName[0], nameLen);
    tmpName[nameLen] = '\0';
    name = tmpName;

    int64 type = 0;
    file.ReadType(&type);
    sa.type = (SpecialArea::EType) type;

    int64 waypointConnections = 0;
    file.ReadType(&waypointConnections);
    sa.waypointConnections = (EWaypointConnections) waypointConnections;

    char altered = 0;
    file.ReadType(&altered);
    sa.bAltered = altered != 0;
    file.ReadType(&sa.fHeight);
    if (version <= 16)
    {
        float junk;
        file.ReadType(&junk);
    }
    file.ReadType(&sa.fNodeAutoConnectDistance);
    file.ReadType(&sa.fMaxZ);
    file.ReadType(&sa.fMinZ);

    int nBuildingID;
    file.ReadType(&nBuildingID);
    sa.nBuildingID = (int16)nBuildingID;

    if (version >= 18)
    {
        unsigned char lightLevel = 0;
        file.ReadType(&lightLevel);
        sa.lightLevel = (EAILightLevel)lightLevel;
    }

    if (version >= 23)
    {
        char critterOnly = 0;
        file.ReadType(&critterOnly);
        sa.bCritterOnly = critterOnly != 0;
    }

    // now the area itself
    ListPositions pts;
    unsigned ptsSize;
    file.ReadType(&ptsSize);

    for (unsigned iPt = 0; iPt < ptsSize; ++iPt)
    {
        Vec3 pt;
        file.ReadType(&pt);
        pts.push_back(pt);
    }
    sa.SetPolygon(pts);
}

//====================================================================
// ReadPolygonArea
//====================================================================
bool ReadPolygonArea(CCryBufferedFileReader& file, int version, string& name, ListPositions& pts)
{
    unsigned nameLen = maxForbiddenNameLen;
    file.ReadType(&nameLen);
    if (nameLen >= maxForbiddenNameLen)
    {
        AIWarning("Excessive forbidden area name length - AI loading failed");
        return false;
    }
    char tmpName[maxForbiddenNameLen];
    file.ReadRaw(&tmpName[0], nameLen);
    tmpName[nameLen] = '\0';
    name = tmpName;

    unsigned ptsSize;
    file.ReadType(&ptsSize);

    for (unsigned iPt = 0; iPt < ptsSize; ++iPt)
    {
        Vec3 pt;
        file.ReadType(&pt);
        pts.push_back(pt);
    }
    return true;
}

//====================================================================
// ReadForbiddenArea
//====================================================================
bool ReadForbiddenArea(CCryBufferedFileReader& file, int version, CAIShape* shape)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "Forbidden areas");

    unsigned nameLen = maxForbiddenNameLen;
    file.ReadType(&nameLen);
    if (nameLen >= maxForbiddenNameLen)
    {
        AIWarning("Excessive forbidden area name length - AI loading failed");
        return false;
    }
    char tmpName[maxForbiddenNameLen];
    file.ReadRaw(&tmpName[0], nameLen);
    tmpName[nameLen] = '\0';
    shape->SetName(tmpName);

    unsigned ptsSize;
    file.ReadType(&ptsSize);

    ShapePointContainer& pts = shape->GetPoints();
    pts.resize(ptsSize);

    for (unsigned i = 0; i < ptsSize; ++i)
    {
        file.ReadType(&pts[i]);
    }

    // Call build AABB since the point container was filled directly.
    shape->BuildAABB();

    return true;
}

//====================================================================
// ReadExtraLinkCostArea
//====================================================================
void ReadExtraLinkCostArea(CCryBufferedFileReader& file, int version, string& name, SExtraLinkCostShape& shape)
{
    unsigned nameLen;
    file.ReadType(&nameLen);
    char tmpName[1024];
    file.ReadRaw(&tmpName[0], nameLen);
    tmpName[nameLen] = '\0';
    name = tmpName;

    file.ReadType(&shape.origCostFactor);
    shape.costFactor = shape.origCostFactor;
    file.ReadType(&shape.aabb.min);
    file.ReadType(&shape.aabb.max);

    unsigned ptsSize;
    file.ReadType(&ptsSize);

    for (unsigned iPt = 0; iPt < ptsSize; ++iPt)
    {
        Vec3 pt;
        file.ReadType(&pt);
        shape.shape.push_back(pt);
    }
}


//====================================================================
// ReadAreasFromFile_Old
//====================================================================
void CNavigation::ReadAreasFromFile_Old(CCryBufferedFileReader& file, int fileVersion)
{
    FlushAllAreas();

    unsigned numAreas;

    // Read forbidden areas
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            CAIShape* pShape = new CAIShape;
            if (!ReadForbiddenArea(file, fileVersion, pShape))
            {
                return;
            }
        }
    }

    // Read navigation modifiers (special areas)
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        m_specialAreas.clear();
        m_specialAreas.resize(numAreas);

        string name;
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            SpecialArea sa;
            ReadArea(file, fileVersion, name, sa);
        }
    }

    // Read designer forbidden areas
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            CAIShape* pShape = new CAIShape;
            ReadForbiddenArea(file, fileVersion, pShape);
        }
    }

    // Read forbidden boundaries
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            CAIShape* pShape = new CAIShape;
            ReadForbiddenArea(file, fileVersion, pShape);
        }
    }

    // Read extra link costs
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            SExtraLinkCostShape shape(ListPositions(), 0.0f);
            string name;
            ReadExtraLinkCostArea(file, fileVersion, name, shape);
        }
    }

    // Read designer paths
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            ListPositions lp;
            string name;
            ReadPolygonArea(file, fileVersion, name, lp);

            int navType(0), type(0);
            bool closed(false);
            file.ReadType(&navType);
            file.ReadType(&type);

            if (fileVersion >= 22)
            {
                file.ReadType(&closed);
            }

            if (m_mapDesignerPaths.find(name) != m_mapDesignerPaths.end())
            {
                AIError("CNavigation::ReadAreasFromFile_Old: Designer path '%s' already exists, please rename the path and reexport.", name.c_str());
            }
            else
            {
                m_mapDesignerPaths.insert(ShapeMap::iterator::value_type(name, SShape(lp, false, (IAISystem::ENavigationType)navType, type, closed)));
            }
        }
    }
}

//
// Reads (designer paths) areas from file. clears the existing areas
void CNavigation::ReadAreasFromFile(CCryBufferedFileReader& file, int fileVersion)
{
    FlushAllAreas();

    unsigned numAreas;

    // Read designer paths
    {
        file.ReadType(&numAreas);
        // vague sanity check
        AIAssert(numAreas < 1000000);
        for (unsigned iArea = 0; iArea < numAreas; ++iArea)
        {
            ListPositions lp;
            string name;
            ReadPolygonArea(file, fileVersion, name, lp);

            int navType(0), type(0);
            bool closed(false);
            file.ReadType(&navType);
            file.ReadType(&type);

            if (fileVersion >= 22)
            {
                file.ReadType(&closed);
            }

            if (m_mapDesignerPaths.find(name) != m_mapDesignerPaths.end())
            {
                AIError("CNavigation::ReadAreasFromFile: Designer path '%s' already exists, please rename the path and reexport.", name.c_str());
                //@TODO do something about AI Paths in multiple area.bai files when in Segmented World
            }
            else
            {
                m_mapDesignerPaths.insert(ShapeMap::iterator::value_type(name, SShape(lp, false, (IAISystem::ENavigationType)navType, type, closed)));
            }
        }
    }
}

// Offsets all areas when segmented world shifts
void CNavigation::OffsetAllAreas(const Vec3& additionalOffset)
{
    for (ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        it->second.OffsetShape(additionalOffset);
    }
}

//===================================================================
// GetSpecialAreaNearestPos
//===================================================================
const SpecialArea* CNavigation::GetSpecialAreaNearestPos(const Vec3& pos, SpecialArea::EType areaType)
{
    if (const SpecialArea* specialArea = GetSpecialArea(pos, areaType))
    {
        return specialArea;
    }

    float bestSq = std::numeric_limits<float>::max();
    const SpecialArea* result = 0;

    SpecialAreas::const_iterator it = m_specialAreas.begin();
    SpecialAreas::const_iterator end = m_specialAreas.end();

    for (; it != end; ++it)
    {
        const SpecialArea& specialArea = *it;
        if (specialArea.type == areaType)
        {
            if (specialArea.fHeight > 0.00001f && (pos.z < specialArea.fMinZ || pos.z > specialArea.fMaxZ))
            {
                continue;
            }

            Vec3 polyPt;
            float distSq = Distance::Point_Polygon2DSq(pos, specialArea.GetPolygon(), polyPt);
            if (distSq < bestSq)
            {
                bestSq = distSq;
                result = &specialArea;
            }
        }
    }

    return result;
}

//====================================================================
// GetPath
//====================================================================
uint32 CNavigation::GetPath(const char* szPathName, Vec3* points, uint32 maxpoints) const
{
    ListPositions junk1;
    AABB junk2;
    junk2.Reset();
    SShape pathShape(junk1, junk2);
    uint32 count = 0;

    if (GetDesignerPath(szPathName, pathShape))
    {
        for (std::vector<Vec3>::iterator it = pathShape.shape.begin(), e = pathShape.shape.end(); it != e && count < maxpoints; ++it)
        {
            points[count] = *it;
            ++count;
        }
    }

    return count;
}

//====================================================================
// GetNearestPointOnPath
//====================================================================
float CNavigation::GetNearestPointOnPath(const char* szPathName, const Vec3& vPos, Vec3& vResult, bool& bLoopPath, float& totalLength) const
{
    // calculate a point on a path which has the nearest distance from the given point.
    // Also returns segno it means ( 0.0 start point 100.0 end point always)
    // strict even if t == 0 or t == 1.0

    // return values
    // return value : segno;
    // vResult : the point on path
    // bLoopPath : true if the path is looped

    vResult     = ZERO;
    bLoopPath   = false;

    float result = -1.0;

    SShape pathShape;
    if (!GetDesignerPath(szPathName, pathShape) || pathShape.shape.empty())
    {
        return result;
    }

    float   dist    = FLT_MAX;
    bool    bFound  = false;
    float   segNo   = 0.0f;
    float   howmanypath = 0.0f;

    Vec3    vPointOnLine;

    ListPositions::const_iterator cur = pathShape.shape.begin();
    ListPositions::const_reverse_iterator last = pathShape.shape.rbegin();
    ListPositions::const_iterator next(cur);
    ++next;

    float   lengthTmp = 0.0f;
    while (next != pathShape.shape.end())
    {
        Lineseg seg(*cur, *next);

        lengthTmp += (*cur - *next).GetLength();

        float   t;
        float   d = Distance::Point_Lineseg(vPos, seg, t);
        if (d < dist)
        {
            Vec3 vSeg = seg.GetPoint(1.0f) - seg.GetPoint(0.0f);
            Vec3 vTmp = vPos - seg.GetPoint(t);

            vSeg.NormalizeSafe();
            vTmp.NormalizeSafe();

            dist    = d;
            bFound  = true;
            result  = segNo + t;
            vPointOnLine = seg.GetPoint(t);
        }
        cur = next;
        ++next;
        segNo += 1.0f;
        howmanypath += 1.0f;
    }
    if (howmanypath == 0.0f)
    {
        return result;
    }

    if (bFound == false)
    {
        segNo = 0.0f;
        cur = pathShape.shape.begin();
        while (cur != pathShape.shape.end())
        {
            Vec3 vTmp = vPos - *cur;
            float d = vTmp.GetLength();
            if (d < dist)
            {
                dist    = d;
                bFound  = true;
                result  = segNo;
                vPointOnLine = *cur;
            }
            ++cur;
            segNo += 1.0f;
        }
    }

    vResult = vPointOnLine;

    cur     = pathShape.shape.begin();

    Vec3 vTmp   = *cur - *last;
    if (vTmp.GetLength() < 0.0001f)
    {
        bLoopPath = true;
    }

    totalLength = lengthTmp;

    return result * 100.0f / howmanypath;
}

//====================================================================
// GetPointOnPathBySegNo
//====================================================================
void CNavigation::GetPointOnPathBySegNo(const char* szPathName, Vec3& vResult, float segNo) const
{
    vResult = ZERO;

    SShape pathShape;
    if ((segNo < 0.f) || (segNo > 100.f) || !GetDesignerPath(szPathName, pathShape))
    {
        return;
    }

    ListPositions& shape = pathShape.shape;
    size_t size = shape.size();
    if (size == 0)
    {
        return;
    }

    if (size == 1)
    {
        vResult = *(shape.begin());
        return;
    }

    float totalLength = 0.0f;
    for (ListPositions::const_iterator cur = shape.begin(), next = cur; ++next != shape.end(); cur = next)
    {
        totalLength += (*next - *cur).GetLength();
    }

    float segLength = totalLength * segNo / 100.0f;
    float currentLength = 0.0f;
    float currentSegLength = 0.0f;

    ListPositions::const_iterator cur, next;
    for (cur = shape.begin(), next = cur; ++next != shape.end(); cur = next)
    {
        const Vec3& curPoint = *cur;
        Vec3 currentSeg = *next - curPoint;
        currentSegLength = currentSeg.GetLength();

        if (currentLength + currentSegLength > segLength)
        {
            vResult = curPoint;
            if (currentSegLength > 0.0003f)
            {
                vResult += ((segLength - currentLength) / currentSegLength) * currentSeg;
            }
            return;
        }

        currentLength += currentSegLength;
    }

    vResult = *cur;
}

//====================================================================
// IsSegmentValid
//====================================================================
bool CNavigation::IsSegmentValid(IAISystem::tNavCapMask navCap, float rad, const Vec3& posFrom, Vec3& posTo, IAISystem::ENavigationType& navTypeFrom) const
{
    int nBuildingID = -1;

    navTypeFrom = CheckNavigationType(posFrom, nBuildingID, navCap);

    if (navTypeFrom == IAISystem::NAV_TRIANGULAR)
    {
        // Make sure not to enter forbidden area.
        if (IsPathForbidden(posFrom, posTo))
        {
            return false;
        }

        if (IsPointForbidden(posTo, rad))
        {
            return false;
        }
    }

    const SpecialArea* pSpecialArea = (nBuildingID != -1 ? GetSpecialArea(nBuildingID) : 0);
    if (pSpecialArea)
    {
        const ListPositions& polygon = pSpecialArea->GetPolygon();

        if (Overlap::Lineseg_Polygon2D(Lineseg(posFrom, posTo), polygon, &pSpecialArea->GetAABB()))
        {
            return false;
        }
    }

    return CheckWalkability(posFrom, posTo, rad + 0.15f, &posTo);
}

//====================================================================
// GetSpecialArea
//====================================================================
const SpecialArea* CNavigation::GetSpecialArea(const Vec3& pos, SpecialArea::EType areaType)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    SpecialAreas::iterator it = m_specialAreas.begin();
    SpecialAreas::iterator end = m_specialAreas.end();

    for (; it != end; ++it)
    {
        const SpecialArea& specialArea = *it;
        if ((specialArea.type == areaType) && IsPointInSpecialArea(pos, specialArea))
        {
            return &specialArea;
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

    assert(buildingID < 0 || buildingID < (int)m_specialAreas.size());
    if (buildingID >= 0 && buildingID < (int)m_specialAreas.size())
    {
        return &m_specialAreas[buildingID];
    }

    return 0;
}

const SpecialArea* CNavigation::GetSpecialArea(const char* name) const
{
    SpecialAreaNames::const_iterator it = m_specialAreaNames.find(CONST_TEMP_STRING(name));
    if (it != m_specialAreaNames.end())
    {
        return GetSpecialArea(it->second);
    }

    return 0;
}

const char* CNavigation::GetSpecialAreaName(int buildingID) const
{
    SpecialAreaNames::const_iterator it = m_specialAreaNames.begin();
    SpecialAreaNames::const_iterator end = m_specialAreaNames.end();

    for (; it != end; ++it)
    {
        if (it->second == buildingID)
        {
            return it->first.c_str();
        }
    }

    return "<Unknown>";
}

void CNavigation::InsertSpecialArea(const char* name, const SpecialArea& sa)
{
    assert(sa.nBuildingID >= 0);
    std::pair<SpecialAreaNames::iterator, bool> result = m_specialAreaNames.insert(
            SpecialAreaNames::iterator::value_type(name, sa.nBuildingID));

    if ((size_t)sa.nBuildingID >= m_specialAreas.size())
    {
        m_specialAreas.resize(sa.nBuildingID + 1);
    }
    m_specialAreas[sa.nBuildingID] = sa;
}

void CNavigation::EraseSpecialArea(const char* name)
{
    SpecialAreaNames::iterator it = m_specialAreaNames.find(CONST_TEMP_STRING(name));
    if (it == m_specialAreaNames.end())
    {
        return;
    }

    SpecialArea& specialArea = m_specialAreas[it->second];
    assert(specialArea.nBuildingID >= 0);

    m_freeSpecialAreaIDs.push_back(specialArea.nBuildingID);
    m_specialAreaNames.erase(it);
    specialArea.nBuildingID = -1;
}

void CNavigation::FlushSpecialAreas()
{
    m_specialAreaNames.clear();
    m_specialAreas.clear();
    m_freeSpecialAreaIDs.clear();
}


//====================================================================
// GetVolumeRegions
//====================================================================
void CNavigation::GetVolumeRegions(VolumeRegions& volumeRegions) const
{
    volumeRegions.resize(0);

    SpecialAreas::const_iterator it = m_specialAreas.begin();
    SpecialAreas::const_iterator end = m_specialAreas.end();

    for (; it != end; ++it)
    {
        const SpecialArea& specialArea = *it;

        if (specialArea.type == SpecialArea::TYPE_VOLUME)
        {
            volumeRegions.push_back(std::make_pair(GetSpecialAreaName(specialArea.nBuildingID), &specialArea));
        }
    }
}

//====================================================================
// GetBuildingInfo
//====================================================================
bool CNavigation::GetBuildingInfo(int nBuildingID, IAISystem::SBuildingInfo& info) const
{
    if (const SpecialArea* specialArea = GetSpecialArea(nBuildingID))
    {
        info.fNodeAutoConnectDistance = specialArea->fNodeAutoConnectDistance;
        info.waypointConnections = specialArea->waypointConnections;

        return true;
    }

    return false;
}

bool CNavigation::IsPointInBuilding(const Vec3& pos, int nBuildingID) const
{
    if (const SpecialArea* specialArea = GetSpecialArea(nBuildingID))
    {
        return IsPointInSpecialArea(pos, *specialArea);
    }

    return false;
}

void CNavigation::FlushAllAreas()
{
    FlushSpecialAreas();
    m_mapDesignerPaths.clear();
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::DoesNavigationShapeExists(const char* szName, EnumAreaType areaType, bool road)
{
    if (areaType == AREATYPE_PATH && !road)
    {
        return m_mapDesignerPaths.find(szName) != m_mapDesignerPaths.end();
    }
    return false;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::CreateNavigationShape(const SNavigationShapeParams& params)
{
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
        if (m_mapDesignerPaths.find(params.szPathName) != m_mapDesignerPaths.end())
        {
            AIError("CAISystem::CreateNavigationShape: Designer path '%s' already exists, please rename the path.", params.szPathName);
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
    return true;
}

// deletes designer created path
//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DeleteNavigationShape(const char* szName)
{
    ShapeMap::iterator di;
    di = m_mapDesignerPaths.find(szName);
    if (di != m_mapDesignerPaths.end())
    {
        m_mapDesignerPaths.erase(di);
    }
}

void CNavigation::DisableModifier(const char* name)
{
    SpecialAreaNames::iterator it = m_specialAreaNames.find(name);

    if (it != m_specialAreaNames.end())
    {
        SpecialArea& specialArea = m_specialAreas[it->second];
        if (specialArea.bAltered)    // already disabled
        {
            return;
        }

        GetAISystem()->InvalidatePathsThroughArea(specialArea.GetPolygon());
        specialArea.bAltered = true;
    }
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
    SpecialAreas::const_iterator it = m_specialAreas.begin();
    SpecialAreas::const_iterator end = m_specialAreas.end();

    for (; it != end; ++it)
    {
        const SpecialArea& specialArea = *it;

        if (specialArea.type == SpecialArea::TYPE_WATER)
        {
            if (Overlap::Point_AABB2D(pt, specialArea.GetAABB()))
            {
                if (Overlap::Point_Polygon2D(pt, specialArea.GetPolygon()))
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

    return Overlap::Point_Polygon2D(pos, sa.GetPolygon(), &sa.GetAABB());
}

//===================================================================
// IsPointInTriangulationAreas
//===================================================================
bool CNavigation::IsPointInTriangulationAreas(const Vec3& pos) const
{
    AIAssert(gEnv->IsEditor());

    bool foundOne = false;
    SpecialAreas::const_iterator it = m_specialAreas.begin();
    SpecialAreas::const_iterator end = m_specialAreas.end();

    for (; it != end; ++it)
    {
        const SpecialArea& specialArea = *it;

        if (specialArea.type == SpecialArea::TYPE_TRIANGULATION)
        {
            if (Overlap::Point_Polygon2D(pos, specialArea.GetPolygon(), &specialArea.GetAABB()))
            {
                return true;
            }

            foundOne = true;
        }
    }

    return !foundOne;
}

//====================================================================
// GetPointOutsideForbidden
//====================================================================
Vec3 CNavigation::GetPointOutsideForbidden(Vec3& pos, float distance, const Vec3* startPos) const
{
    return pos;
}

const char* CNavigation::GetNearestPathOfTypeInRange(IAIObject* requester, const Vec3& reqPos, float range, int type, float devalue, bool useStartNode)
{
    AIAssert(requester);
    float   rangeSq(sqr(range));

    ShapeMap::iterator  closestShapeIt(m_mapDesignerPaths.end());
    float closestShapeDist(rangeSq);
    CAIActor* pReqActor = requester->CastToCAIActor();

    for (ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        const SShape&   path = it->second;

        // Skip paths which we cannot travel
        if ((path.navType & pReqActor->GetMovementAbility().pathfindingProperties.navCapMask) == 0)
        {
            continue;
        }
        // Skip wrong type
        if (path.type != type)
        {
            continue;
        }
        // Skip locked paths
        if (path.devalueTime > 0.01f)
        {
            continue;
        }

        // Skip paths too far away
        Vec3    tmp;
        if (Distance::Point_AABBSq(reqPos, path.aabb, tmp) > rangeSq)
        {
            continue;
        }

        float   d;
        if (useStartNode)
        {
            // Disntance to start node.
            d = Distance::Point_PointSq(reqPos, path.shape.front());
        }
        else
        {
            // Distance to nearest point on path.
            ListPositions::const_iterator   nearest = path.NearestPointOnPath(reqPos, false, d, tmp);
        }

        if (d < closestShapeDist)
        {
            closestShapeIt = it;
            closestShapeDist = d;
        }
    }

    if (closestShapeIt != m_mapDesignerPaths.end())
    {
        closestShapeIt->second.devalueTime = devalue;
        return closestShapeIt->first.c_str();
    }

    return 0;
}

//====================================================================
// DisableNavigationInBrokenRegion
//====================================================================
void CNavigation::DisableNavigationInBrokenRegion(std::vector<Vec3>& outline)
{
    if (!gAIEnv.pGraph)
    {
        AIWarning("Being asked to disable navigation in broken region, yet no graph");
        return;
    }
    AABB outlineAABB(AABB::RESET);
    for (std::vector<Vec3>::const_iterator it = outline.begin(); it != outline.end(); ++it)
    {
        outlineAABB.Add(*it);
    }
    outlineAABB.min.z = -std::numeric_limits<float>::max();
    outlineAABB.max.z = std::numeric_limits<float>::max();

    AILogEvent("Disabling navigation in broken region (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
        outlineAABB.min.x, outlineAABB.min.y, outlineAABB.min.z,
        outlineAABB.max.x, outlineAABB.max.y, outlineAABB.max.z);
}

//====================================================================
// GetVolumeRegionFiles
//====================================================================
void CNavigation::GetVolumeRegionFiles(const char* szLevel, const char* szMission, DynArray<CryStringT<char> >& filenames) const
{
    filenames.clear();

    VolumeRegions volumeRegions;
    GetVolumeRegions(volumeRegions);

    string name;
    unsigned nRegions = volumeRegions.size();
    for (unsigned i = 0; i < nRegions; ++i)
    {
        const string& regionName = volumeRegions[i].first;
        /// match what's done in CVolumeNavRegion
        if (regionName.length() > 17)
        {
            name = string(regionName.c_str() + 17);
        }
        else
        {
            AIWarning("CAISystem::GetVolumeRegionFiles region name is too short %s", regionName.c_str());
            continue;
        }

        CryStringT<char> fileName;
        fileName.Format("%s/v3d%s-region-%s.bai", szLevel, szMission, name.c_str());

        filenames.push_back(fileName);
    }
}

//====================================================================
// GetNavRegion
//====================================================================
CNavRegion* CNavigation::GetNavRegion(IAISystem::ENavigationType type, const class CGraph* pGraph) const
{
    switch (type)
    {
    case IAISystem::NAV_UNSET:
        return 0;
    }
    return 0;
}

//====================================================================
// CheckNavigationType
// When areas are nested there is an ordering - make this explicit by
// ordering the search over the navigation types
//====================================================================
IAISystem::ENavigationType CNavigation::CheckNavigationType(const Vec3& pos, int& nBuildingID, IAISystem::tNavCapMask navCapMask) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    if (navCapMask & IAISystem::NAV_WAYPOINT_HUMAN)
    {
        SpecialAreas::const_iterator it = m_specialAreas.begin();
        SpecialAreas::const_iterator end = m_specialAreas.end();

        for (; it != end; ++it)
        {
            const SpecialArea& specialArea = *it;

            if (specialArea.type == SpecialArea::TYPE_WAYPOINT_HUMAN)
            {
                if (IsPointInSpecialArea(pos, specialArea))
                {
                    nBuildingID = specialArea.nBuildingID;

                    return IAISystem::NAV_WAYPOINT_HUMAN;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_VOLUME)
    {
        SpecialAreas::const_iterator it = m_specialAreas.begin();
        SpecialAreas::const_iterator end = m_specialAreas.end();

        for (; it != end; ++it)
        {
            const SpecialArea& specialArea = *it;

            if (specialArea.type == SpecialArea::TYPE_VOLUME)
            {
                if (IsPointInSpecialArea(pos, specialArea))
                {
                    nBuildingID = specialArea.nBuildingID;

                    return IAISystem::NAV_VOLUME;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_FLIGHT)
    {
        SpecialAreas::const_iterator it = m_specialAreas.begin();
        SpecialAreas::const_iterator end = m_specialAreas.end();

        for (; it != end; ++it)
        {
            const SpecialArea& specialArea = *it;

            if (specialArea.type == SpecialArea::TYPE_FLIGHT)
            {
                if (IsPointInSpecialArea(pos, specialArea))
                {
                    nBuildingID = specialArea.nBuildingID;

                    return IAISystem::NAV_FLIGHT;
                }
            }
        }
    }

    if (navCapMask & IAISystem::NAV_FREE_2D)
    {
        SpecialAreas::const_iterator it = m_specialAreas.begin();
        SpecialAreas::const_iterator end = m_specialAreas.end();

        for (; it != end; ++it)
        {
            const SpecialArea& specialArea = *it;

            if (specialArea.type == SpecialArea::TYPE_FREE_2D)
            {
                if (IsPointInSpecialArea(pos, specialArea))
                {
                    nBuildingID = specialArea.nBuildingID;

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

void CNavigation::GetMemoryStatistics(ICrySizer* pSizer)
{
    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "NavRegions");
    }

    size_t size = 0;
    for (ShapeMap::iterator itr = m_mapDesignerPaths.begin(); itr != m_mapDesignerPaths.end(); ++itr)
    {
        size += (itr->first).capacity();
        size += itr->second.MemStats();
    }
    pSizer->AddObject(&m_mapDesignerPaths, size);
}

size_t CNavigation::GetMemoryUsage() const
{
    size_t mem = 0;
    return mem;
}


#ifdef CRYAISYSTEM_DEBUG
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DebugDraw() const
{
    CDebugDrawContext dc;

    if (!m_validationErrorMarkers.empty())
    {
        for (unsigned i = 0; i < m_validationErrorMarkers.size(); ++i)
        {
            const SValidationErrorMarker& marker = m_validationErrorMarkers[i];
            dc->DrawOBB(marker.obb, marker.pos, false, marker.col, eBBD_Faceted);

            const float s = 0.01f;
            dc->DrawLine(marker.pos + Vec3(-s,  0,  0), marker.col, marker.pos + Vec3(s, 0, 0), marker.col);
            dc->DrawLine(marker.pos + Vec3(0, -s,  0), marker.col, marker.pos + Vec3(0, s, 0), marker.col);
            dc->DrawLine(marker.pos + Vec3(0,  0, -s), marker.col, marker.pos + Vec3(0, 0, s), marker.col);

            dc->Draw3dLabelEx(marker.pos, 1.1f, marker.col, true, true, false, false, "%s", marker.msg.c_str());
        }
    }

    CAISystem* pAISystem = GetAISystem();
    int debugDrawVal = gAIEnv.CVars.DebugDraw;

    // Draw occupied designer paths
    for (ShapeMap::const_iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
    {
        const SShape&   path = it->second;
        if (path.devalueTime < 0.01f)
        {
            continue;
        }
        dc->Draw3dLabel(path.shape.front(), 1, "%.1f", path.devalueTime);
    }
}

#endif //CRYAISYSTEM_DEBUG
