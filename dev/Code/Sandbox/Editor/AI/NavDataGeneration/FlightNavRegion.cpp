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
#include "CommonDefs.h"             // NOTE Feb 22, 2008: <pvl> former AISystem StdAfx.h
#include "AILog.h"
#include <IPhysics.h>
#include <algorithm>
#include <limits>
#include <ISystem.h>
#include <IRenderer.h>          // this is needed for debug drawing
#include <IRenderAuxGeom.h> // this is needed for debug drawing
#include <ITimer.h>
#include <I3DEngine.h>
#include <Cry_GeoOverlap.h>
#include "FlightNavRegion.h"
#include "AICollision.h"
#include "ISerialize.h"
#include "Navigation.h"         // NOTE Feb 22, 2008: <pvl> for SpecialArea declaration
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#define BAI_FNAV_FILE_VERSION_READ 8
#define BAI_FNAV_FILE_VERSION_WRITE 9

#define MAX_CLIP_POINT  12

inline int
clamp(int val, int minVal, int maxVal)
{
    if (val < minVal)
    {
        return minVal;
    }
    if (val > maxVal)
    {
        return maxVal;
    }
    return val;
}

inline
int fsgn(float a)
{
    if (a < 0)
    {
        return -1;
    }
    return 1;
}

static
void
ClipPolygonToPlane(Plane& plane, const Vec3* in, int nin, Vec3* out, float* dist, int& nout)
{
    //  if( nin < 3 )
    //      return;

    for (int i = 0; i < nin; i++)
    {
        dist[i] = plane.DistFromPlane(in[i]);
    }

    nout = 0;

    for (int i = 0; i < nin; i++)
    {
        int next = i + 1;
        if (next >= nin)
        {
            next = 0;
        }
        // Need no clipping
        if (dist[i] >= 0)
        {
            out[nout] = in[i];
            nout++;
        }
        if (dist[i] * dist[next] < 0.0f)
        {
            // need clipping
            float range = dist[i] - dist[next];
            float scale = 0.0f;
            if (fabsf(range) > 0.0000001f)
            {
                scale = dist[i] / range;
            }
            const Vec3& a = in[i];
            const Vec3& b = in[next];
            out[nout].x = a.x + (b.x - a.x) * scale;
            out[nout].y = a.y + (b.y - a.y) * scale;
            out[nout].z = a.z + (b.z - a.z) * scale;
            nout++;
        }
    }
}

// The particle structure used for the path beautification.
struct SPart
{
    SPart(const Vec3& newPos)
        : pos(newPos)
        , vel(0, 0, 0)
        , force(0, 0, 0) { };
    Vec3  pos;
    Vec3  vel;
    Vec3  force;
};

struct SObstacle
{
    IGeometry*    geom;
    Vec3              pos;
    Matrix33      mat;
    AABB              bbox;
    int                   minx, miny, maxx, maxy;
    bool              isSmall;
    std::vector<Vec3> vertices;
};

CFlightNavRegion::CFlightNavRegion(IPhysicalWorld* pPhysWorld, CGraph* pGraph)
    : m_pPhysWorld(pPhysWorld)
    , m_pGraph(pGraph)
    , m_childSubDiv(0)
    , m_terrainDownSample(0)
    , m_heightFieldDimX(0)
    , m_heightFieldDimY(0)
{
    AIAssert(m_pPhysWorld);
    AIAssert(m_pGraph);
    Clear();
}

CFlightNavRegion::~CFlightNavRegion()
{
    Clear();
}

void CFlightNavRegion::Clear()
{
    // Delete the nodes.
    for (unsigned i = 0; i < m_spans.size(); i++)
    {
        if (m_spans[i].m_graphNodeIndex)
        {
            m_pGraph->Disconnect(m_spans[i].m_graphNodeIndex, true);
        }
    }

    m_spans.clear();

    m_heightFieldOriginX = 0;
    m_heightFieldOriginY = 0;
    m_heightFieldDimX = 0;
    m_heightFieldDimY = 0;
    m_childSubDiv = 2;
    m_terrainDownSample = 16;
}

void CFlightNavRegion::Process(I3DEngine* p3DEngine, IPhysicalEntity** pObstacles, int obstacleCount,
    const std::list<SpecialArea*>& flightAreas,
    unsigned childSubDiv, unsigned terrainDownSample)
{
    m_childSubDiv = childSubDiv;
    m_terrainDownSample = terrainDownSample;

    float fStartTime = gEnv->pTimer->GetAsyncCurTime();

    // Process terrain
    AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
    const int terrainSize = static_cast<int>(terrainAabb.GetWidth());

    AZ::Vector2 gridResolution = AZ::Vector2::CreateOne();
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainGridResolution);
    const int terrainUnitSize = static_cast<int>(gridResolution.GetX());

    int   sampledSize;

    while (true)
    {
        sampledSize = terrainSize / m_terrainDownSample;
        m_heightFieldDimX = sampledSize / m_childSubDiv;
        m_heightFieldDimY = sampledSize / m_childSubDiv;

        if (m_heightFieldDimX < 1 || m_heightFieldDimY < 1)
        {
            if (m_terrainDownSample > 1)
            {
                m_terrainDownSample /= 2;
            }
            else if (m_childSubDiv > 1)
            {
                m_childSubDiv /= 2;
            }
            else
            {
                AIWarning("CFlightNavRegion::Process Unable to adjust sample sizes - aborting flight generation");
                Clear();
                return;
            }
        }
        else
        {
            break;
        }
    }
    if (m_childSubDiv != childSubDiv || m_terrainDownSample != terrainDownSample)
    {
        AILogProgress("CFlightNavRegion::Process Had to adjust sample values to m_childSubDiv = %d and m_terrainDownSample = %d", m_childSubDiv, m_terrainDownSample);
    }

    // Find the dimension of the flight enabled areas.
    // Process hi-definition areas.
    int   areaMinX = 0, areaMaxX = 0;
    int   areaMinY = 0, areaMaxY = 0;

    AABB  allAreas(Vec3(0, 0, 0), Vec3(0, 0, 0));
    bool  firstUpdateAllAreas = true;

    for (std::list<SpecialArea*>::const_iterator areaIt = flightAreas.begin(); areaIt != flightAreas.end(); ++areaIt)
    {
        SpecialArea*    sa = (*areaIt);

        int ptCount = sa->GetPolygon().size();

        if (ptCount < 3)
        {
            continue;
        }

        // Calculate the AA bounding box of the area.
        for (ListPositions::const_iterator ptIt = sa->GetPolygon().begin(); ptIt != sa->GetPolygon().end(); ++ptIt)
        {
            const Vec3& pos = (*ptIt);
            if (firstUpdateAllAreas)
            {
                firstUpdateAllAreas = false;
                allAreas.min = allAreas.max = pos;
            }
            else
            {
                allAreas.Add(pos);
            }
        }
    }

    float     cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    float     childCellSize = (float)(m_terrainDownSample);

    // Clamp the AABB to grid.
    areaMinX = (int)floor(allAreas.min.x / cellSize);
    areaMinY = (int)floor(allAreas.min.y / cellSize);
    areaMaxX = (int)ceil(allAreas.max.x / cellSize);
    areaMaxY = (int)ceil(allAreas.max.y / cellSize);

    if ((areaMaxX - areaMinX) == 0 || (areaMaxY - areaMinY) == 0)
    {
        areaMinX = 0;
        areaMinY = 0;
        areaMaxX = terrainSize / (m_terrainDownSample * m_childSubDiv);
        areaMaxY = terrainSize / (m_terrainDownSample * m_childSubDiv);
    }

    m_heightFieldOriginX = areaMinX;
    m_heightFieldOriginY = areaMinY;

    m_heightFieldDimX = (unsigned)(areaMaxX - areaMinX);
    m_heightFieldDimY = (unsigned)(areaMaxY - areaMinY);

    int           idx = 0;
    int           terrainOriginX = m_heightFieldOriginX * m_childSubDiv;
    int           terrainOriginY = m_heightFieldOriginY * m_childSubDiv;
    unsigned  terrainDimX = m_heightFieldDimX * m_childSubDiv;
    unsigned  terrainDimY = m_heightFieldDimY * m_childSubDiv;
    std::vector<SCell>    heightField;
    heightField.resize(terrainDimX * terrainDimY);

    // The process works in following steps:
    //  1) Create initial height field from the terrain data.
    //  2) Mark the hi-resolution cells.
    //  3) Sample all the static obstacles and add and merge new spans.
    //  4) Create links and nodes for path finding.
    //  5) Calculate the radius each link can pass.

    float fTerrainStartTime = gEnv->pTimer->GetAsyncCurTime();

    // Create a hi-res height field based o nthe terrain.
    AILogProgress("  - resampling terrain to %d x %d", terrainDimX, terrainDimY);
    AILogProgress("    - terrain size %d x %d.", terrainSize, terrainSize);
    AILogProgress("    - subdiv: %d  resample: %d.", m_childSubDiv, m_terrainDownSample);

    int nDownSamplePrecision = max(terrainUnitSize, 1);

    idx = 0;
    for (int i = 0; i < terrainDimY; i++)
    {
        for (int j = 0; j < terrainDimX; j++)
        {
            int   x = (j + terrainOriginX) * m_terrainDownSample;
            int   y = (i + terrainOriginY) * m_terrainDownSample;

            float minHeight = 10000000;
            float maxHeight = 0;

            // Find maximum value of a downsampled block.
            bool terrainExists = false;
            auto enumerationCallback = [&](AzFramework::Terrain::TerrainDataRequests* terrain) -> bool
            {
                terrainExists = true;
                for (int yy = 0; yy < m_terrainDownSample; yy += nDownSamplePrecision)
                {
                    for (int xx = 0; xx < m_terrainDownSample; xx += nDownSamplePrecision)
                    {
                        if ((x + xx) < terrainSize && (y + yy) < terrainSize)
                        {
                            float   terrainHeight = terrain->GetHeightFromFloats(x + xx, y + yy, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP);
                            minHeight = min(minHeight, terrainHeight);
                            maxHeight = max(maxHeight, terrainHeight);
                        }
                    }
                }
                //We only care about the first handler.
                return false;
            };
            AzFramework::Terrain::TerrainDataRequestBus::EnumerateHandlers(enumerationCallback);
            if (!terrainExists)
            {
                minHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
                maxHeight = minHeight;
            }

            // Make sure the access in in range.
            AIAssert(idx < terrainDimX * terrainDimY);

            int   c = 0;

            float maxDeviation = maxHeight - minHeight;
            if (maxDeviation < 1.0f)
            {
                c |= SSpan::LANDING_GOOD;
            }
            else if (maxDeviation < 5.0f)
            {
                c |= SSpan::LANDING_OK;
            }
            else
            {
                c |= SSpan::LANDING_BAD;
            }

            heightField[idx].m_spans.push_back(SSpan(((float)x / m_terrainDownSample + 0.5f) * childCellSize, ((float)y / m_terrainDownSample + 0.5f) * childCellSize, 0, maxHeight, 0, c));

            idx++;
        }

        AILogProgress("    - resampling terrain %d of %d", i, terrainDimY);
    }

    float fTerrainEndTime = gEnv->pTimer->GetAsyncCurTime();

    // Process hi-definition areas.
    // Mark all height field cells that fall into the high-def areas as hires for later calculations.
    for (std::list<SpecialArea*>::const_iterator areaIt = flightAreas.begin(); areaIt != flightAreas.end(); ++areaIt)
    {
        SpecialArea*    sa = (*areaIt);

        int ptCount = sa->GetPolygon().size();

        if (ptCount < 3)
        {
            continue;
        }

        // Calculate the AA bounding box of the area.
        AABB    bbox(sa->GetPolygon().front(), sa->GetPolygon().front());
        for (ListPositions::const_iterator ptIt = sa->GetPolygon().begin(); ptIt != sa->GetPolygon().end(); ++ptIt)
        {
            const Vec3& pos = (*ptIt);
            bbox.Add(pos);
        }

        // Clip the special are to each of the cells it touches, and reject clipped polygons, which have too small area.
        AABB    gridAABB(bbox);

        // Clamp the AABB to grid.
        gridAABB.min.x = floor(gridAABB.min.x / childCellSize);
        gridAABB.min.y = floor(gridAABB.min.y / childCellSize);
        gridAABB.max.x = ceil(gridAABB.max.x / childCellSize);
        gridAABB.max.y = ceil(gridAABB.max.y / childCellSize);

        int minx = (int)gridAABB.min.x - terrainOriginX;
        int miny = (int)gridAABB.min.y - terrainOriginY;
        int width = (int)(gridAABB.max.x - gridAABB.min.x);
        int height = (int)(gridAABB.max.y - gridAABB.min.y);

        gridAABB.min.x *= childCellSize;
        gridAABB.min.y *= childCellSize;
        gridAABB.max.x *= childCellSize;
        gridAABB.max.y *= childCellSize;

        std::vector<Vec3>       in;
        std::vector<Vec3>       out;
        std::vector<float>  distToPlane;

        // Add some extra padding to the arrays, we clip four times, each propably increasing the array, plus one extra for the rainy day.
        in.resize(ptCount + 4 * 2 + 1);
        out.resize(ptCount + 4 * 2 + 1);
        distToPlane.resize(ptCount + 4 * 2 + 1);

        for (int yy = 0; yy < height; yy++)
        {
            for (int xx = 0; xx < width; xx++)
            {
                int x = clamp(minx + xx, 0, terrainDimX - 1);
                int y = clamp(miny + yy, 0, terrainDimY - 1);

                AABB    cellAABB;
                cellAABB.min.x = (float)(x + terrainOriginX) * childCellSize;
                cellAABB.min.y = (float)(y + terrainOriginY) * childCellSize;
                cellAABB.min.z = gridAABB.min.z - 1;
                cellAABB.max.x = cellAABB.min.x + childCellSize;
                cellAABB.max.y = cellAABB.min.y + childCellSize;
                cellAABB.max.z = gridAABB.max.z + 1;

                // Clip geometry to the AABB
                Plane   planes[4];
                planes[0].SetPlane(Vec3(1, 0, 0), cellAABB.min);
                planes[1].SetPlane(Vec3(-1, 0, 0), cellAABB.max);
                planes[2].SetPlane(Vec3(0,  1, 0), cellAABB.min);
                planes[3].SetPlane(Vec3(0, -1, 0), cellAABB.max);

                // Fill in the list from the original polygon.
                ListPositions::const_iterator ptIt = sa->GetPolygon().begin();
                for (int i = 0; i < ptCount; i++, ++ptIt)
                {
                    in[i] = (*ptIt);
                }

                // Clip to planes.
                int count = ptCount;
                ClipPolygonToPlane(planes[0], &in[0], count, &out[0], &distToPlane[0], count);
                if (count < 3)
                {
                    continue;
                }
                ClipPolygonToPlane(planes[1], &out[0], count, &in[0], &distToPlane[0], count);
                if (count < 3)
                {
                    continue;
                }
                ClipPolygonToPlane(planes[2], &in[0], count, &out[0], &distToPlane[0], count);
                if (count < 3)
                {
                    continue;
                }
                ClipPolygonToPlane(planes[3], &out[0], count, &in[0], &distToPlane[0], count);
                if (count < 3)
                {
                    continue;
                }

                // Calc remaining polygon area
                // This calculation is likely to suffer from floating point precision.
                float totalCross = 0.0f;
                for (int i = 0; i < count; ++i)
                {
                    int j = (i + 1) % count;
                    totalCross += in[i].x * in[j].y - in[j].x * in[i].y;
                }
                // Skip degenerate polygons which may have been caused by clipping non-convex polygon.
                if (totalCross < 0.001f)
                {
                    continue;
                }

                // Something was left after the clipping.
                int hidx = x + y * terrainDimX;
                AIAssert(hidx < terrainDimX * terrainDimY);
                SCell&  cell = heightField[hidx];
                cell.m_hires = true;
            }
        }
    }

    // No high definition areas specified, use all cells.
    if (flightAreas.empty())
    {
        for (int i = 0; i < terrainDimX * terrainDimY; i++)
        {
            heightField[i].m_hires = false;
        }
    }

    // Process obstacles.
    // Walk throught each obstacles, and clip the obstacle to each grid cell it touches.
    // Calculate the minimum and maximum of each of remaining geometry and store that span to the cell.
    // This algorithm will use only the min/max info, so any hole inside the object is missed!

    float fObstacleStartTime = gEnv->pTimer->GetAsyncCurTime();
    float fObstacleRaycastTime = 0;
    float fObstacleClipTime = 0;
    float fObstacleTransformTime = 0;

    std::vector<SObstacle>    obstacles;

    obstacles.resize(obstacleCount);

    int   totalPolyCount = 0;
    int   smallCount = 0;

    for (int i = 0; i < obstacleCount; i++)
    {
        IPhysicalEntity* pEnt = pObstacles[i];

        SObstacle&  obst = obstacles[i];

        pe_status_pos status;
        status.ipart = 0;
        pEnt->GetStatus(&status);

        obst.pos = status.pos;
        obst.mat = Matrix33(status.q);
        obst.mat *= status.scale;

        obst.bbox.min = status.BBox[0] + obst.pos;
        obst.bbox.max = status.BBox[1] + obst.pos;

        // Clamp the AABB to grid.
        obst.minx = (int)floor(obst.bbox.min.x / childCellSize) - terrainOriginX;
        obst.miny = (int)floor(obst.bbox.min.y / childCellSize) - terrainOriginY;
        obst.maxx = (int)ceil(obst.bbox.max.x / childCellSize) - terrainOriginX;
        obst.maxy = (int)ceil(obst.bbox.max.y / childCellSize) - terrainOriginY;

        float   cellDX = obst.bbox.max.x - obst.bbox.min.x;
        float   cellDY = obst.bbox.max.y - obst.bbox.min.y;
        if (cellDX < childCellSize * 1.5f && cellDY < childCellSize * 1.5f)
        {
            obst.isSmall = true;
            smallCount++;
        }
        else
        {
            obst.isSmall = false;
        }

        IGeometry* geom = status.pGeom;

        obst.geom = geom;

        // Transform vertices.
        int type = geom->GetType();
        switch (type)
        {
        case GEOM_BOX:
        {
            totalPolyCount += 6;
            break;
        }
        case GEOM_TRIMESH:
        case GEOM_VOXELGRID:
        {
            const primitives::primitive* prim = geom->GetData();
            const mesh_data* mesh = static_cast<const mesh_data*>(prim);

            int numVerts = mesh->nVertices;
            int numTris = mesh->nTris;

            obst.vertices.resize(numVerts);

            for (int j = 0; j < numVerts; j++)
            {
                obst.vertices[j] = obst.pos + obst.mat * mesh->pVertices[j];
            }

            totalPolyCount += numTris;

            break;
        }
        default:
            break;
        }

        // Mark the cells that have objects.
        for (int y = obst.miny; y <= obst.maxy; y++)
        {
            for (int x = obst.minx; x <= obst.maxx; x++)
            {
                if (x >= 0 && x < terrainDimX && y >= 0 && y < terrainDimY)
                {
                    // Only sample the high resolution cells.
                    if (heightField[x + y * terrainDimX].m_hires)
                    {
                        heightField[x + y * terrainDimX].m_used++;
                        heightField[x + y * terrainDimX].m_obstacles.push_back(i);
                    }
                }
            }
        }
    }

    float smallPercentage = 0.0f;
    if (obstacleCount > 0)
    {
        smallPercentage = ((float)smallCount / (float)obstacleCount) * 100.0f;
    }

    AILogProgress("  - world: %dk polygons", totalPolyCount / 1000);
    AILogProgress("  - %d (%f%%%%) small obstacles", smallCount, smallPercentage);

    AILogProgress("  - processing %lu obstacles", obstacles.size());

    int   processedPolys = 0;
    int   minPolysPerCell = 100000000;
    int   maxPolysPerCell = 0;
    int   processedCells = 0;

    for (int y = 0; y < terrainDimY; y++)
    {
        for (int x = 0; x < terrainDimX; x++)
        {
            SCell&    cell = heightField[x + y * terrainDimX];
            if (cell.m_used == 0)
            {
                continue;
            }

            int   polys = 0;

            AABB  cellAABB;
            cellAABB.min.x = (float)(x + terrainOriginX) * childCellSize;
            cellAABB.min.y = (float)(y + terrainOriginY) * childCellSize;
            cellAABB.min.z = 0;
            cellAABB.max.x = cellAABB.min.x + childCellSize;
            cellAABB.max.y = cellAABB.min.y + childCellSize;
            cellAABB.max.z = 0;

            // Clip geometry to the AABB
            Plane planes[4];
            planes[0].SetPlane(Vec3(1, 0, 0), cellAABB.min);
            planes[1].SetPlane(Vec3(-1, 0, 0), cellAABB.max);
            planes[2].SetPlane(Vec3(0,  1, 0), cellAABB.min);
            planes[3].SetPlane(Vec3(0, -1, 0), cellAABB.max);

            Vec3  in[MAX_CLIP_POINT];
            Vec3  out[MAX_CLIP_POINT];
            float distToPlane[MAX_CLIP_POINT];

            for (std::list<int>::iterator obstIt = cell.m_obstacles.begin(); obstIt != cell.m_obstacles.end(); ++obstIt)
            {
                SObstacle&  obst = obstacles[(*obstIt)];//(*obstIt);

                if (x < obst.minx || x > obst.maxx || y < obst.miny || y > obst.maxy)
                {
                    continue;
                }

                bool    validSpan = false;

                float   spanMin = 100000000;
                float   spanMax = -1000000000;

                if (obst.isSmall)
                {
                    // The AABB of the object fits into the
                    spanMin = obst.bbox.min.z;
                    spanMax = obst.bbox.max.z;
                    validSpan = true;
                }
                else
                {
                    cellAABB.min.z = obst.bbox.min.z - 1;
                    cellAABB.max.z = obst.bbox.max.z + 1;

                    // Check if the AABB of the obstacles is completely inside the

                    int   totalPoints = 0;

                    IGeometry* geom = obst.geom;
                    int type = geom->GetType();
                    switch (type)
                    {
                    case GEOM_BOX:
                    {
                        const primitives::primitive* prim = geom->GetData();
                        const primitives::box* bbox = static_cast<const primitives::box*>(prim);
                        Matrix33 rot = bbox->Basis.T();

                        Vec3  boxVerts[8];
                        int       sides[6 * 4] = {
                            0, 1, 2, 3,
                            0, 1, 5, 4,
                            1, 2, 6, 5,
                            2, 3, 7, 6,
                            3, 0, 4, 7,
                            4, 5, 6, 7,
                        };

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
                            boxVerts[j] = obst.pos + obst.mat * boxVerts[j];
                        }

                        for (int j = 0; j < 6; j++)
                        {
                            in[0] = boxVerts[sides[j * 4 + 0]];
                            in[1] = boxVerts[sides[j * 4 + 1]];
                            in[2] = boxVerts[sides[j * 4 + 2]];
                            in[3] = boxVerts[sides[j * 4 + 3]];
                            int count = 4;

                            ClipPolygonToPlane(planes[0], in, count, out, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[1], out, count, in, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[2], in, count, out, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[3], out, count, in, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }

                            totalPoints += count;
                            polys++;

                            for (int k = 0; k < count; k++)
                            {
                                spanMin = min(spanMin, in[k].z);
                                spanMax = max(spanMax, in[k].z);
                            }
                        }
                    }
                    break;
                    case GEOM_TRIMESH:
                    case GEOM_VOXELGRID:
                    {
                        const primitives::primitive* prim = geom->GetData();
                        const mesh_data* mesh = static_cast<const mesh_data*>(prim);

                        int numVerts = mesh->nVertices;
                        int numTris = mesh->nTris;

                        for (int j = 0; j < numTris; ++j)
                        {
                            in[0] = obst.vertices[mesh->pIndices[j * 3 + 0]];
                            in[1] = obst.vertices[mesh->pIndices[j * 3 + 1]];
                            in[2] = obst.vertices[mesh->pIndices[j * 3 + 2]];
                            int count = 3;

                            ClipPolygonToPlane(planes[0], in, count, out, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[1], out, count, in, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[2], in, count, out, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }
                            ClipPolygonToPlane(planes[3], out, count, in, distToPlane, count);
                            if (count < 3)
                            {
                                continue;
                            }

                            totalPoints += count;
                            polys++;

                            for (int k = 0; k < count; k++)
                            {
                                spanMin = min(spanMin, in[k].z);
                                spanMax = max(spanMax, in[k].z);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                    }

                    if (totalPoints > 0)
                    {
                        validSpan = true;
                    }

                    processedPolys += polys;
                }

                if (validSpan)
                {
                    // Merge with other spans.

                    // There should always be at least one span, the first one is created from the height field.
                    AIAssert(!cell.m_spans.empty());

                    // Remove all the spans that touch the current span with certain fudge,
                    // and merge the deleted span to the current span, then add the new span and sort the list.
                    const float fudge = childCellSize;
                    for (std::list<SSpan>::iterator spanIt = cell.m_spans.begin(); spanIt != cell.m_spans.end(); )
                    {
                        const SSpan& other = (*spanIt);

                        if (other.overlaps(spanMin - fudge, spanMax + fudge))
                        {
                            if (other.m_minz < spanMin)
                            {
                                spanMin = other.m_minz;
                            }
                            if (other.m_maxz > spanMax)
                            {
                                spanMax = other.m_maxz;
                            }
                            cell.m_spans.erase(spanIt++);
                        }
                        else
                        {
                            ++spanIt;
                        }
                    }

                    cell.m_spans.push_back(SSpan((cellAABB.min.x + cellAABB.max.x) * 0.5f, (cellAABB.min.y + cellAABB.max.y) * 0.5f, spanMin, spanMax, 0));
                    cell.m_spans.sort();
                }
            }

            cellAABB.min.z = cell.m_spans.front().m_minz;
            cellAABB.max.z = cell.m_spans.back().m_maxz;

            // Classify spans
            for (std::list<SSpan>::iterator spanIt = cell.m_spans.begin(); spanIt != cell.m_spans.end(); ++spanIt)
            {
                SSpan& span = (*spanIt);
                span.m_classification = SSpan::LANDING_GOOD;
                if (cell.m_hires)
                {
                    span.m_classification |= SSpan::LINKABLE;
                }
            }

            if (polys > maxPolysPerCell)
            {
                maxPolysPerCell = polys;
            }
            if (polys > 0 && polys < maxPolysPerCell)
            {
                minPolysPerCell = polys;
            }

            processedCells++;
        }

        AILogProgress("  - row %d/%d", y, terrainDimY);
    }

    float fObstacleEndTime = gEnv->pTimer->GetAsyncCurTime();

    float fAvg = 0;

    if (processedCells)
    {
        fAvg = (float)processedPolys / (float)processedCells;
    }

    AILogProgress("  - processed %dk obstacle triangles.", processedPolys / 1000);
    AILogProgress("     - min: %d/cell", minPolysPerCell);
    AILogProgress("     - max: %d/cell", maxPolysPerCell);
    AILogProgress("     - avg: %.2f/cell", fAvg);


    // Now the hi-resolution height field is calculated. We will simplify this by trying to merge cells.
    // The number of cells to merge is specified in the m_childSubDiv variable.
    // The cell merging.

    float fMergeStartTime = gEnv->pTimer->GetAsyncCurTime();

    m_spans.resize(m_heightFieldDimX * m_heightFieldDimY);

    AILogProgress("  - processing cells %d x %d.", m_heightFieldDimX, m_heightFieldDimY);

    int   iHiresCellCount = 0;
    idx = 0;
    for (int y = 0; y < terrainDimY; y += m_childSubDiv)
    {
        for (int x = 0; x < terrainDimX; x += m_childSubDiv)
        {
            // Gather the data from the merged cells. At the moment the merging happens only if there
            // is just one span, and no hires flags are set.
            // In addition to that the algorithm checks The maximum delta change inside the block,
            // and if that exceeds the given threshold, the block is store in hires as well.
            // TODO: Add better calculation for the height threshold, and allow to merger blocks with multiple spans.

            float maxDelta = 0;
            float maxHeight = 0;
            int       spanCount = 0;
            int       hiresCount = 0;
            int       classification = 0;
            bool  linkable = false;

            for (int yy = 0; yy < m_childSubDiv; yy++)
            {
                for (int xx = 0; xx < m_childSubDiv; xx++)
                {
                    int x0 = clamp(x + xx, 0, terrainDimX - 1);
                    int y0 = clamp(y + yy, 0, terrainDimY - 1) * terrainDimX;
                    if (xx < (m_childSubDiv - 1) && yy < (m_childSubDiv - 1))
                    {
                        int x1 = clamp(x + xx + 1, 0, terrainDimX - 1);
                        int y1 = clamp(y + yy + 1, 0, terrainDimY - 1) * terrainDimX;

                        maxDelta = max(maxDelta, fabsf(heightField[x0 + y0].m_spans.front().m_maxz - heightField[x1 + y0].m_spans.front().m_maxz));
                        maxDelta = max(maxDelta, fabsf(heightField[x1 + y0].m_spans.front().m_maxz - heightField[x1 + y1].m_spans.front().m_maxz));
                        maxDelta = max(maxDelta, fabsf(heightField[x1 + y1].m_spans.front().m_maxz - heightField[x0 + y1].m_spans.front().m_maxz));
                        maxDelta = max(maxDelta, fabsf(heightField[x0 + y1].m_spans.front().m_maxz - heightField[x0 + y0].m_spans.front().m_maxz));
                    }

                    maxHeight = max(maxHeight, heightField[x0 + y0].m_spans.front().m_maxz);

                    AIAssert((x0 + y0) < terrainDimX * terrainDimY);
                    spanCount += (int)heightField[x0 + y0].m_spans.size();
                    classification |= heightField[x0 + y0].m_spans.front().m_classification;

                    if (heightField[x0 + y0].m_hires)
                    {
                        linkable = true;
                    }
                }
            }

            // If the maximum delta inside the block is greater than the treshold,
            // store the higher detail cells.

            int   lowestClass = 0;
            if (classification & SSpan::LANDING_BAD)
            {
                lowestClass = SSpan::LANDING_BAD;
            }
            else if (classification & SSpan::LANDING_OK)
            {
                lowestClass = SSpan::LANDING_OK;
            }
            else if (classification & SSpan::LANDING_GOOD)
            {
                lowestClass = SSpan::LANDING_GOOD;
            }
            else
            {
                lowestClass = SSpan::LANDING_BAD;
            }

            if (linkable)
            {
                lowestClass |= SSpan::LINKABLE;
            }

            float centerx = (float)(x + terrainOriginX + m_childSubDiv / 2) * childCellSize;
            float centery = (float)(y + terrainOriginY + m_childSubDiv / 2) * childCellSize;

            if (linkable && (spanCount > (m_childSubDiv * m_childSubDiv) || (classification& SSpan::HIRES) || maxDelta > childCellSize * 2.0f))
            {
                // Store low detail as a base.
                m_spans[idx].Set(centerx, centery, 0, maxHeight, 0, lowestClass);
                m_spans[idx].m_childIdx = (int)m_spans.size();

                // Calc the span count.
                unsigned    subSpanCount = 0;
                for (int yy = 0; yy < m_childSubDiv; yy++)
                {
                    for (int xx = 0; xx < m_childSubDiv; xx++)
                    {
                        AIAssert((x + xx + (y + yy) * terrainDimX) < terrainDimX * terrainDimY);
                        SCell&  tmpCell = heightField[x + xx + (y + yy) * terrainDimX];
                        if (!tmpCell.m_spans.empty())
                        {
                            subSpanCount += tmpCell.m_spans.size() - 1;
                        }
                    }
                }

                // Store high detail.
                m_spans.resize(m_spans.size() + (m_childSubDiv * m_childSubDiv) + subSpanCount);

                int childIdx = m_spans[idx].m_childIdx;
                int subSpanIdx = childIdx + (m_childSubDiv * m_childSubDiv);

                for (int yy = 0; yy < m_childSubDiv; yy++)
                {
                    for (int xx = 0; xx < m_childSubDiv; xx++)
                    {
                        AIAssert((x + xx + (y + yy) * terrainDimX) < terrainDimX * terrainDimY);
                        SCell&  tmpCell = heightField[x + xx + (y + yy) * terrainDimX];

                        std::list<SSpan>::iterator spanIt = tmpCell.m_spans.begin();

                        m_spans[childIdx].CopyFrom(*spanIt);
                        m_spans[childIdx].m_childIdx = INVALID_IDX;
                        m_spans[childIdx].m_nextIdx = INVALID_IDX;
                        if (tmpCell.m_hires)
                        {
                            m_spans[childIdx].m_classification |= SSpan::LINKABLE;
                        }

                        ++spanIt;

                        int prevIdx = childIdx;
                        // Copy spans
                        for (; spanIt != tmpCell.m_spans.end(); ++spanIt)
                        {
                            m_spans[prevIdx].m_nextIdx = subSpanIdx;
                            m_spans[subSpanIdx].CopyFrom(*spanIt);
                            m_spans[subSpanIdx].m_childIdx = INVALID_IDX;
                            m_spans[subSpanIdx].m_nextIdx = INVALID_IDX;
                            if (tmpCell.m_hires)
                            {
                                m_spans[subSpanIdx].m_classification |= SSpan::LINKABLE;
                            }

                            prevIdx = subSpanIdx;
                            subSpanIdx++;
                        }

                        childIdx++;
                    }
                }

                iHiresCellCount++;
            }
            else
            {
                // Store low detail.
                m_spans[idx].Set(centerx, centery, 0, maxHeight, 0, lowestClass);
            }
            idx++;
        }
    }

    float fMergeEndTime = gEnv->pTimer->GetAsyncCurTime();


    // Calculate the maximum sphere that can be fitted at each cell of the span.
    // As long as the there is space for the beautification algorithm to adjust the
    // sphere to new, safe location, the larger radius as aceptted too.

    float fRadiusStartTime = gEnv->pTimer->GetAsyncCurTime();

    AILogProgress("    - calculating cell radii");

    Vec3  cellMin, cellMax;
    float maxRadius = childCellSize * 3.0f;
    Vec3  spanCenter;
    idx = 0;

    for (int y = 0; y < m_heightFieldDimY; y++)
    {
        for (int x = 0; x < m_heightFieldDimX; x++)
        {
            float worldX = (float)x * cellSize;
            float worldY = (float)y * cellSize;

            if (m_spans[idx].m_childIdx != INVALID_IDX)
            {
                // Check for all child cells.
                int childIdx = m_spans[idx].m_childIdx;
                for (int childY = 0; childY < m_childSubDiv; childY++)
                {
                    for (int childX = 0; childX < m_childSubDiv; childX++)
                    {
                        for (int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                        {
                            float spanMin, spanMax;
                            int       spanNextIdx = m_spans[spanIdx].m_nextIdx;

                            float radSqr = maxRadius * maxRadius;
                            float minDistSqr = FLT_MAX;

                            // The sphere is located at the middle of the space between the spans.
                            spanCenter.x = m_spans[spanIdx].m_x;
                            spanCenter.y = m_spans[spanIdx].m_y;

                            spanMin = m_spans[spanIdx].m_maxz;

                            Vec3  pos;

                            if (spanNextIdx != INVALID_IDX)
                            {
                                spanMax = m_spans[spanNextIdx].m_minz;
                                spanCenter.z = (spanMin + spanMax) * 0.5f;
                                pos = spanCenter;
                            }
                            else
                            {
                                spanMax = spanMin + maxRadius * 2.0f;
                                spanCenter.z = (spanMin + spanMax) * 0.5f;

                                Vec3    adjPos(spanCenter);
                                if (CheckAndAdjustSphere(adjPos, maxRadius))
                                {
                                    radSqr = GetMinDistanceSqrCellsInCube(adjPos, maxRadius);
                                    pos = adjPos;
                                }
                                else
                                {
                                    pos = spanCenter;
                                }
                            }

                            minDistSqr = GetMinDistanceSqrCellsInCube(spanCenter, maxRadius);
                            if (minDistSqr > radSqr && minDistSqr < (maxRadius * maxRadius))
                            {
                                radSqr = minDistSqr;
                                pos = spanCenter;
                            }

                            m_spans[spanIdx].m_maxRadius = sqrt(radSqr);
#ifdef FLIGHTNAV_DEBUG_SPHERES
                            m_spans[spanIdx].m_spherePos = pos;
#endif
                        }

                        childIdx++;
                    }
                }
            }

            // Test for the larger cell.
            {
                for (int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                {
                    float spanMin, spanMax;
                    int       spanNextIdx = m_spans[spanIdx].m_nextIdx;

                    float radSqr = maxRadius * maxRadius;
                    float minDistSqr = FLT_MAX;

                    // The sphere is located at the middle of the space between the spans.
                    spanCenter.x = m_spans[spanIdx].m_x;
                    spanCenter.y = m_spans[spanIdx].m_y;

                    spanMin = m_spans[spanIdx].m_maxz;

                    Vec3  pos;

                    if (spanNextIdx != INVALID_IDX)
                    {
                        spanMax = m_spans[spanNextIdx].m_minz;
                        spanCenter.z = (spanMin + spanMax) * 0.5f;
                        pos = spanCenter;
                    }
                    else
                    {
                        spanMax = spanMin + maxRadius * 2.0f;
                        spanCenter.z = (spanMin + spanMax) * 0.5f;

                        Vec3    adjPos(spanCenter);
                        if (CheckAndAdjustSphere(adjPos, maxRadius))
                        {
                            radSqr = GetMinDistanceSqrCellsInCube(adjPos, maxRadius);
                            pos = adjPos;
                        }
                        else
                        {
                            pos = spanCenter;
                        }
                    }

                    minDistSqr = GetMinDistanceSqrCellsInCube(spanCenter, maxRadius);
                    if (minDistSqr > radSqr && minDistSqr < maxRadius * maxRadius)
                    {
                        radSqr = minDistSqr;
                        pos = spanCenter;
                    }

                    m_spans[spanIdx].m_maxRadius = sqrt(radSqr);
#ifdef FLIGHTNAV_DEBUG_SPHERES
                    m_spans[spanIdx].m_spherePos = pos;
#endif
                }
            }

            idx++;
        }
    }

    float fRadiusEndTime = gEnv->pTimer->GetAsyncCurTime();

    int   iTotalCellCount = m_heightFieldDimX * m_heightFieldDimY;

    AILogProgress("    - hi-res cells: %d (%.1f%%%%)", iHiresCellCount, ((float)iHiresCellCount / (float)iTotalCellCount) * 100.0f);
    AILogProgress("    - low-res cells: %d (%.1f%%%%)", iTotalCellCount - iHiresCellCount, ((float)(iTotalCellCount - iHiresCellCount) / (float)iTotalCellCount) * 100.0f);

    float fBuildGraphStartTime = gEnv->pTimer->GetAsyncCurTime();

    BuildGraph();

    float fBuildGraphEndTime = gEnv->pTimer->GetAsyncCurTime();

    AILogProgress(" Finished in %6.3fs", gEnv->pTimer->GetAsyncCurTime() - fStartTime);
    AILogProgress("   - Terrain: %6.3fs", fTerrainEndTime - fTerrainStartTime);
    AILogProgress("   - Obstacles: %6.3fs", fObstacleEndTime - fObstacleStartTime);
    AILogProgress("      - Clip: %6.3fs", fObstacleClipTime);
    AILogProgress("      - Transform: %6.3fs", fObstacleTransformTime);
    AILogProgress("      - Raycast: %6.3fs", fObstacleRaycastTime);
    AILogProgress("   - Merge: %6.3fs", fMergeEndTime - fMergeStartTime);
    AILogProgress("   - Radius: %6.3fs", fRadiusEndTime - fRadiusStartTime);
    AILogProgress("   - BuildGraph: %6.3fs", fBuildGraphEndTime - fBuildGraphStartTime);
    AILogProgress("   - mem: %luk", MemStats() / 1024);
}

float   CFlightNavRegion::GetMinDistanceSqrCellsInCube(const Vec3& center, float rad) const
{
    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);

    int   minx = clamp((int)floor((center.x - rad) / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   miny = clamp((int)floor((center.y - rad) / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);
    int   maxx = clamp((int)ceil((center.x + rad) / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   maxy = clamp((int)ceil((center.y + rad) / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);

    float minDistSqr = sqr(rad);

    for (int y = miny; y <= maxy; y++)
    {
        for (int x = minx; x <= maxx; x++)
        {
            float distSqr = GetMinDistanceSqrToCell(x, y, center);
            if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
            }
        }
    }

    return minDistSqr;
}

float   CFlightNavRegion::GetMinDistanceSqrToCell(int x, int y, const Vec3& center) const
{
    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    if (x < 0 || y < 0 || x >= m_heightFieldDimX || y >= m_heightFieldDimY)
    {
        return m_heightFieldDimX * m_heightFieldDimY * cellSize;
    }

    int   idx = x + y * m_heightFieldDimX;

    float worldX = (float)(x + m_heightFieldOriginX) * cellSize;
    float worldY = (float)(y + m_heightFieldOriginY) * cellSize;

    float minDistSqr = FLT_MAX;

    AABB  bbox;

    if (m_spans[idx].m_childIdx != INVALID_IDX)
    {
        int childIdx = m_spans[idx].m_childIdx;
        for (int childY = 0; childY < m_childSubDiv; childY++)
        {
            for (int childX = 0; childX < m_childSubDiv; childX++)
            {
                bbox.min.x = worldX + (float)childX * childCellSize;
                bbox.min.y = worldY + (float)childY * childCellSize;
                bbox.max.x = bbox.min.x + childCellSize;
                bbox.max.y = bbox.min.y + childCellSize;

                for (int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                {
                    bbox.min.z = m_spans[spanIdx].m_minz;
                    bbox.max.z = m_spans[spanIdx].m_maxz;

                    float distSqr = GetDistanceSqrToBox(bbox.min, bbox.max, center);
                    if (distSqr < minDistSqr)
                    {
                        minDistSqr = distSqr;
                    }
                }

                childIdx++;
            }
        }
    }
    else
    {
        bbox.min.x = worldX;
        bbox.min.y = worldY;
        bbox.max.x = bbox.min.x + cellSize;
        bbox.max.y = bbox.min.y + cellSize;

        for (int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
        {
            bbox.min.z = m_spans[spanIdx].m_minz;
            bbox.max.z = m_spans[spanIdx].m_maxz;

            float distSqr = GetDistanceSqrToBox(bbox.min, bbox.max, center);
            if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
            }
        }
    }

    return minDistSqr;
}

bool CFlightNavRegion::WriteToFile(const char* pName)
{
    // Don't do anything if the system is not initialised.
    if (m_spans.empty())
    {
        AIWarning("Trying to save flight navigation, but the navigation is not generated. [%s]", pName);
        return true;
    }

    CCryFile file;
    if (!file.Open(pName, "wb"))
    {
        AIWarning("could not save AI flight nav. [%s]", pName);
        return false;
    }
    int nFileVersion = BAI_FNAV_FILE_VERSION_WRITE;
    file.Write(&nFileVersion, sizeof(nFileVersion));

    file.Write(&m_heightFieldOriginX, sizeof(m_heightFieldOriginX));
    file.Write(&m_heightFieldOriginY, sizeof(m_heightFieldOriginY));
    file.Write(&m_heightFieldDimX, sizeof(m_heightFieldDimX));
    file.Write(&m_heightFieldDimY, sizeof(m_heightFieldDimY));
    file.Write(&m_childSubDiv, sizeof(m_childSubDiv));
    file.Write(&m_terrainDownSample, sizeof(m_terrainDownSample));

    uint32    spanCount = m_spans.size();
    std::vector<SFlightLinkDesc> linkDescs;
    std::vector<SFlightDesc> flightDescs(spanCount);
    for (uint32 i = 0; i < spanCount; ++i)
    {
        SSpan&  span = m_spans[i];
        // span itself
        SFlightDesc& fd = flightDescs[i];
        fd.x = span.m_x;
        fd.y = span.m_y;
        fd.minz = span.m_minz;
        fd.maxz = span.m_maxz;
        fd.maxRadius = span.m_maxRadius;
        fd.classification = span.m_classification;
        fd.childIdx = span.m_childIdx;
        fd.nextIdx = span.m_nextIdx;
        // links
        //    AIAssert( span.m_graphNodeIndex );
        GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
        if (pSpanGraphNode)
        {
            int startIdx = pSpanGraphNode->GetFlightNavData()->nSpanIdx;
            for (unsigned link = pSpanGraphNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
            {
                unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
                GraphNode* pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);
                if (pNextNode)
                {
                    int endIdx = pNextNode->GetFlightNavData()->nSpanIdx;
                    if (startIdx < endIdx)
                    {
                        linkDescs.push_back(SFlightLinkDesc(startIdx, endIdx));
                    }
                }
            }
        }
    }

    file.Write(&spanCount, sizeof(spanCount));
    if (spanCount > 0)
    {
        file.Write(&flightDescs[0], spanCount * sizeof(SFlightDesc));
    }

    uint32 size = linkDescs.size();
    file.Write(&size, sizeof(size));
    if (!linkDescs.empty())
    {
        file.Write(&linkDescs[0], linkDescs.size() * sizeof(SFlightLinkDesc));
    }

    AILogProgress("Flight nav saved successfully. [%s]", pName);

    return true;
}

bool CFlightNavRegion::ReadFromFile(const char* pName)
{
    float fStartTime = gEnv->pTimer->GetAsyncCurTime();

    CCryFile file;
    if (!file.Open(pName, "rb"))
    {
        AIWarning("could not read AI flight nav. [%s]", pName);
        return false;
    }
    int nFileVersion = BAI_FNAV_FILE_VERSION_READ;
    file.ReadType(&nFileVersion);

    if (nFileVersion < BAI_FNAV_FILE_VERSION_READ)
    {
        AIWarning("Wrong BAI file version (found %d expected at least %d)!! Regenerate Flight navigation in the editor.",
            nFileVersion, BAI_FNAV_FILE_VERSION_READ);
        return false;
    }

    file.ReadType(&m_heightFieldOriginX);
    file.ReadType(&m_heightFieldOriginY);
    file.ReadType(&m_heightFieldDimX);
    file.ReadType(&m_heightFieldDimY);
    file.ReadType(&m_childSubDiv);
    file.ReadType(&m_terrainDownSample);

    uint32    spanCount = 0;
    file.ReadType(&spanCount);
    m_spans.clear(); // free memory - resize won't
    m_spans.resize(spanCount);

    if (nFileVersion == 8)
    {
        for (uint32 i = 0; i < spanCount; i++)
        {
            SSpan&    span = m_spans[i];

            float x, y, minz, maxz, maxradius;
            int   classification, childIdx, nextIdx;

            file.ReadType(&x);
            file.ReadType(&y);
            file.ReadType(&minz);
            file.ReadType(&maxz);
            file.ReadType(&maxradius);
            file.ReadType(&classification);
            file.ReadType(&childIdx);
            file.ReadType(&nextIdx);

            span.m_x = x;
            span.m_y  = y;
            span.m_minz = minz;
            span.m_maxz = maxz;
            span.m_maxRadius = maxradius;
            span.m_classification = classification;
            span.m_childIdx = childIdx;
            span.m_nextIdx = nextIdx;
        }
    }
    else
    {
        std::vector<SFlightDesc> flightDescs(spanCount);
        if (spanCount > 0)
        {
            file.ReadType(&flightDescs[0], spanCount);
        }
        for (uint32 i = 0; i < spanCount; ++i)
        {
            SSpan&    span = m_spans[i];
            SFlightDesc& fd = flightDescs[i];
            span.m_x = fd.x;
            span.m_y = fd.y;
            span.m_minz = fd.minz;
            span.m_maxz = fd.maxz;
            span.m_maxRadius = fd.maxRadius;
            span.m_classification = fd.classification;
            span.m_childIdx = fd.childIdx;
            span.m_nextIdx = fd.nextIdx;
        }
    }

    float fLinkStartTime = gEnv->pTimer->GetAsyncCurTime();

    // Create nodes.
    std::size_t nSpans = m_spans.size();

    //if (!gEnv->IsEditor() && nSpans)
    //{
    //  if (!m_pGraph->NodesPool.AddPool(IAISystem::NAV_FLIGHT, nSpans))
    //      AILogLoading("[CGraph::ReadNodes] Nodes pool already initialized!");
    //}

    for (uint32 i = 0; i < nSpans; i++)
    {
        SSpan&  span = m_spans[i];
        span.m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_FLIGHT, Vec3(span.m_x, span.m_y, span.m_maxz));
        GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
        if (pSpanGraphNode)
        {
            pSpanGraphNode->GetFlightNavData()->nSpanIdx = (int)i;
        }
    }

    // read links
    uint32 linkCount = 0;
    file.ReadType(&linkCount);
    AILogLoading("Loading %d links descriptors.", linkCount);
    std::vector<SFlightLinkDesc> linkDescs(linkCount);
    if (linkCount > 0)
    {
        file.ReadType(&linkDescs[0], linkCount);
    }
    AILogLoading("Connecting %d links.", linkCount);
    for (uint32 i = 0; i < linkCount; i++)
    {
        int startIdx = linkDescs[i].index1;
        int endIdx = linkDescs[i].index2;
        AIAssert(startIdx != INVALID_IDX && endIdx != INVALID_IDX);
        float   rad = min(m_spans[startIdx].m_maxRadius, m_spans[endIdx].m_maxRadius);
        m_pGraph->Connect(m_spans[startIdx].m_graphNodeIndex, m_spans[endIdx].m_graphNodeIndex, rad, rad);
    }

    float fEndTime = gEnv->pTimer->GetAsyncCurTime();

    AILogLoading("AI flight nav loaded successfully in %6.3f (spans = %6.3f, links = %6.3f) sec. [%s]",
        fEndTime - fStartTime,
        fLinkStartTime - fStartTime,
        fEndTime - fLinkStartTime,
        pName);

    DumpLinkRadii("flightnav_readfromfile.txt");

    return true;
}

bool CFlightNavRegion::CheckAndAdjustSphere(Vec3& center, float rad)
{
    // Don't do anything if the system is not initialised.
    if (m_spans.empty())
    {
        return false;
    }

    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    int   minx = clamp((int)floor((center.x - rad) / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   miny = clamp((int)floor((center.y - rad) / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);
    int   maxx = clamp((int)ceil((center.x + rad) / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   maxy = clamp((int)ceil((center.y + rad) / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);

    bool  hit = false;

    for (int y = miny; y < maxy; y++)
    {
        for (int x = minx; x < maxx; x++)
        {
            int   idx = x + y * m_heightFieldDimX;

            float worldX = (float)(x + m_heightFieldOriginX) * cellSize;
            float worldY = (float)(y + m_heightFieldOriginY) * cellSize;

            if (m_spans[idx].m_childIdx != INVALID_IDX)
            {
                // test all childs.
                int childIdx = m_spans[idx].m_childIdx;
                for (int childY = 0; childY < m_childSubDiv; childY++)
                {
                    for (int childX = 0; childX < m_childSubDiv; childX++)
                    {
                        Vec3    boxMin(worldX + (float)childX * childCellSize, worldY + (float)childY * childCellSize, 0);
                        Vec3    boxMax(boxMin.x + childCellSize, boxMin.y + childCellSize, 0);

                        for (int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                        {
                            boxMin.z = m_spans[spanIdx].m_minz;
                            boxMax.z = m_spans[spanIdx].m_maxz;
                            if (BoxSphereAdjust(boxMin, boxMax, center, rad))
                            {
                                hit = true;
                            }
                        }

                        childIdx++;
                    }
                }
            }
            else
            {
                Vec3    boxMin(worldX, worldY, 0);
                Vec3    boxMax(boxMin.x + cellSize, boxMin.y + cellSize, 0);

                for (int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                {
                    boxMin.z = m_spans[spanIdx].m_minz;
                    boxMax.z = m_spans[spanIdx].m_maxz;
                    if (BoxSphereAdjust(boxMin, boxMax, center, rad))
                    {
                        hit = true;
                    }
                }
            }
        }
    }

    return hit;
}

float   CFlightNavRegion::GetDistanceSqrToBox(const Vec3& boxMin, const Vec3& boxMax, const Vec3& center) const
{
    Vec3  dmin(0, 0, 0);

    if (center.x < boxMin.x)
    {
        dmin.x += center.x - boxMin.x;
    }
    else if (center.x > boxMax.x)
    {
        dmin.x += center.x - boxMax.x;
    }

    if (center.y < boxMin.y)
    {
        dmin.y += center.y - boxMin.y;
    }
    else if (center.y > boxMax.y)
    {
        dmin.y += center.y - boxMax.y;
    }

    if (center.z < boxMin.z)
    {
        dmin.z += center.z - boxMin.z;
    }
    else if (center.z > boxMax.z)
    {
        dmin.z += center.z - boxMax.z;
    }

    return dmin.GetLengthSquared();
}

bool CFlightNavRegion::BoxSphereAdjust(const Vec3& boxMin, const Vec3& boxMax, Vec3& center, float rad)
{
    Vec3  dmin(0, 0, 0);
    float r2 = rad * rad;

    if (center.x < boxMin.x)
    {
        dmin.x += center.x - boxMin.x;
    }
    else if (center.x > boxMax.x)
    {
        dmin.x += center.x - boxMax.x;
    }

    if (center.y < boxMin.y)
    {
        dmin.y += center.y - boxMin.y;
    }
    else if (center.y > boxMax.y)
    {
        dmin.y += center.y - boxMax.y;
    }

    if (center.z < boxMin.z)
    {
        dmin.z += center.z - boxMin.z;
    }
    else if (center.z > boxMax.z)
    {
        dmin.z += center.z - boxMax.z;
    }

    float lenSqr = dmin.GetLengthSquared();
    if (lenSqr <= r2)
    {
        float len = (float)sqrt(lenSqr);
        if (len > 0)
        {
            len = (1.0f / len) * (-len + rad);
            center += dmin * len;
        }
        else
        {
            // If we get here, the center of the sphere is inside the
            // box, this is tricky case, and is handled in special way which
            // will behefit the way the boxes are used.
            // The sphere will be lifted up, and pushed down. Lifting is biased.
            float deltaz = 0;
            // Adjust height only.
            if ((center.z - boxMin.z) < (boxMax.z - center.z) * 0.8f)
            {
                deltaz = boxMin.z - rad - center.z;
            }
            else
            {
                deltaz = boxMax.z + rad - center.z;
            }

            center.z += deltaz;
        }
        return true;
    }

    return false;
}

void CFlightNavRegion::ConnectNodes(int srcSpanIdx, int dstSpanIdx)
{
    const float   childCellSize = (float)m_terrainDownSample;

    // Try to connect each source node to destination nodes.
    for (int srcIdx = srcSpanIdx; srcIdx != INVALID_IDX; srcIdx = m_spans[srcIdx].m_nextIdx)
    {
        if (!(m_spans[srcIdx].m_classification & SSpan::LINKABLE))
        {
            continue;
        }
        if (!m_spans[srcIdx].m_graphNodeIndex)
        {
            continue;
        }

        float   srcMin, srcMax;
        int     srcNextIdx = m_spans[srcIdx].m_nextIdx;
        srcMin = m_spans[srcIdx].m_maxz;
        if (srcNextIdx != INVALID_IDX)
        {
            srcMax = m_spans[srcNextIdx].m_minz;
        }
        else
        {
            srcMax = srcMin + 100000;
        }

        for (int dstIdx = dstSpanIdx; dstIdx != INVALID_IDX; dstIdx = m_spans[dstIdx].m_nextIdx)
        {
            if (!(m_spans[dstIdx].m_classification & SSpan::LINKABLE))
            {
                continue;
            }
            if (!m_spans[dstIdx].m_graphNodeIndex)
            {
                continue;
            }

            float dstMin, dstMax;
            int dstNextIdx = m_spans[dstIdx].m_nextIdx;
            dstMin = m_spans[dstIdx].m_maxz;
            if (dstNextIdx != INVALID_IDX)
            {
                dstMax = m_spans[dstNextIdx].m_minz;
            }
            else
            {
                dstMax = dstMin + 100000;
            }

            float linkMin = max(srcMin, dstMin);
            float linkMax = min(srcMax, dstMax);
            float linkGap = linkMax - linkMin;

            if (linkGap > childCellSize)
            {
                m_pGraph->Connect(m_spans[srcIdx].m_graphNodeIndex, m_spans[dstIdx].m_graphNodeIndex);
            }
        }
    }
}

void CFlightNavRegion::BuildGraph()
{
    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    int   nodeCount = 0;
    int   linkCount = 0;

    //
    // Create graph nodes.
    //

    int   idx = 0;
    for (int y = 0; y < m_heightFieldDimY; y++)
    {
        for (int x = 0; x < m_heightFieldDimX; x++)
        {
            float worldX = (float)(x + m_heightFieldOriginX) * cellSize;
            float worldY = (float)(y + m_heightFieldOriginY) * cellSize;

            if (m_spans[idx].m_childIdx != INVALID_IDX)
            {
                int childIdx = m_spans[idx].m_childIdx;
                for (int childY = 0; childY < m_childSubDiv; childY++)
                {
                    for (int childX = 0; childX < m_childSubDiv; childX++)
                    {
                        for (int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
                        {
                            SSpan&    span = m_spans[spanIdx];
                            span.m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_FLIGHT, Vec3(span.m_x, span.m_y, span.m_maxz));
                            GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
                            if (pSpanGraphNode)
                            {
                                pSpanGraphNode->GetFlightNavData()->nSpanIdx = spanIdx;
                            }
                        }

                        nodeCount++;

                        childIdx++;
                    }
                }
            }

            for (int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx)
            {
                SSpan&  span = m_spans[spanIdx];
                span.m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_FLIGHT, Vec3(span.m_x, span.m_y, span.m_maxz));
                GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
                if (pSpanGraphNode)
                {
                    pSpanGraphNode->GetFlightNavData()->nSpanIdx = spanIdx;
                }
            }

            nodeCount++;

            idx++;
        }
    }

    AILogProgress("    - created %d nodes", nodeCount);

    //
    // Link nodes.
    //
    idx = 0;
    for (int y = 0; y < m_heightFieldDimY; y++)
    {
        for (int x = 0; x < m_heightFieldDimX; x++)
        {
            if (m_spans[idx].m_childIdx != INVALID_IDX)
            {
                // Create grid connecting the neighbour child nodes.
                // This will not link diagonally.
                int childIdx = m_spans[idx].m_childIdx;
                for (int childY = 0; childY < m_childSubDiv; childY++)
                {
                    for (int childX = 0; childX < m_childSubDiv; childX++)
                    {
                        if (childX + 1 < m_childSubDiv)
                        {
                            ConnectNodes(childIdx, childIdx + 1);
                            linkCount++;
                        }

                        if (childY + 1 < m_childSubDiv)
                        {
                            ConnectNodes(childIdx, childIdx + m_childSubDiv);
                            linkCount++;
                        }

                        childIdx++;
                    }
                }
            }

            // Connect larger nodes.

            if (x + 1 < m_heightFieldDimX)
            {
                // Connect to the node right from the current node.
                //              SCell&  cellRight = m_cells[idx + 1];
                int rightIdx = idx + 1;

                if (m_spans[idx].m_childIdx != INVALID_IDX || m_spans[rightIdx].m_childIdx != INVALID_IDX)
                {
                    // Connect with child nodes.
                    int   srcInc = 0;
                    int   srcIdx = idx;
                    int   dstInc = 0;
                    int dstIdx = idx + 1;

                    if (m_spans[idx].m_childIdx != INVALID_IDX)
                    {
                        srcInc = m_childSubDiv;
                        srcIdx = m_spans[idx].m_childIdx + m_childSubDiv - 1;
                    }

                    if (m_spans[rightIdx].m_childIdx != INVALID_IDX)
                    {
                        dstInc = m_childSubDiv;
                        dstIdx = m_spans[rightIdx].m_childIdx;
                    }

                    int   nLink = (m_spans[idx].m_childIdx != INVALID_IDX || m_spans[rightIdx].m_childIdx != INVALID_IDX) ? m_childSubDiv : 1;

                    for (int i = 0; i < nLink; i++)
                    {
                        ConnectNodes(srcIdx, dstIdx);
                        linkCount++;

                        srcIdx += srcInc;
                        dstIdx += dstInc;
                    }
                }

                // connect bigger nodes always.
                ConnectNodes(idx, idx + 1);
                linkCount++;
            }

            if (y + 1 < m_heightFieldDimY)
            {
                // Connect to the node up from the current node.
                int upIdx = idx + m_heightFieldDimX;

                if (m_spans[idx].m_childIdx != INVALID_IDX || m_spans[upIdx].m_childIdx != INVALID_IDX)
                {
                    int   srcInc = 0;
                    int   srcIdx = idx;
                    int   dstInc = 0;
                    int dstIdx = idx + m_heightFieldDimX;

                    if (m_spans[idx].m_childIdx != INVALID_IDX)
                    {
                        srcInc = 1;
                        srcIdx = m_spans[idx].m_childIdx + (m_childSubDiv - 1) * m_childSubDiv;
                    }

                    if (m_spans[upIdx].m_childIdx != INVALID_IDX)
                    {
                        dstInc = 1;
                        dstIdx = m_spans[upIdx].m_childIdx;
                    }

                    int   nLink = (m_spans[idx].m_childIdx != INVALID_IDX || m_spans[upIdx].m_childIdx != INVALID_IDX) ? m_childSubDiv : 1;

                    for (int i = 0; i < nLink; i++)
                    {
                        ConnectNodes(srcIdx, dstIdx);
                        linkCount++;

                        srcIdx += srcInc;
                        dstIdx += dstInc;
                    }
                }

                // Connect big nodes always.
                ConnectNodes(idx, idx + m_heightFieldDimX);
                linkCount++;
            }

            // Cross links between bigger cells.
            if ((x + 1 < m_heightFieldDimX) && (y + 1 < m_heightFieldDimY))
            {
                int rightIdx = idx + 1;
                int upIdx = idx + m_heightFieldDimX;
                int rightUpIdx = idx + 1 + m_heightFieldDimX;

                // From current node to up-right node.
                {
                    int   srcIdx, dstIdx;

                    if (m_spans[idx].m_childIdx != INVALID_IDX)
                    {
                        srcIdx = m_spans[idx].m_childIdx + m_childSubDiv * m_childSubDiv - 1;
                    }
                    else
                    {
                        srcIdx = idx; // cell
                    }
                    if (m_spans[rightUpIdx].m_childIdx != INVALID_IDX)
                    {
                        dstIdx = m_spans[rightUpIdx].m_childIdx;
                    }
                    else
                    {
                        dstIdx = idx + 1 + m_heightFieldDimX; // cell up right
                    }
                    ConnectNodes(srcIdx, dstIdx);
                    linkCount++;
                }

                // From right node, to up node.
                {
                    int   srcIdx, dstIdx;

                    if (m_spans[rightIdx].m_childIdx != INVALID_IDX)
                    {
                        srcIdx = m_spans[rightIdx].m_childIdx + m_childSubDiv * (m_childSubDiv - 1);
                    }
                    else
                    {
                        srcIdx = idx + 1; // cellRight
                    }
                    if (m_spans[upIdx].m_childIdx != INVALID_IDX)
                    {
                        dstIdx = m_spans[upIdx].m_childIdx + m_childSubDiv - 1;
                    }
                    else
                    {
                        dstIdx = idx + m_heightFieldDimX; // cell up
                    }
                    ConnectNodes(srcIdx, dstIdx);
                    linkCount++;
                }
            }

            idx++;
        }
    }

    AILogProgress("    - created %d links", linkCount);

    for (unsigned i = 0; i < m_spans.size(); i++)
    {
        GraphNode*  node = m_pGraph->GetNodeManager().GetNode(m_spans[i].m_graphNodeIndex);
        if (!node)
        {
            continue;
        }
        int j = 0;
        for (unsigned link = node->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
        {
            unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
            GraphNode*    nextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);
            if (!nextNode)
            {
                continue;
            }
            int   nextIdx = nextNode->GetFlightNavData()->nSpanIdx;
            if (nextIdx != INVALID_IDX)
            {
                float   rad = min(m_spans[i].m_maxRadius, m_spans[nextIdx].m_maxRadius);
                m_pGraph->GetLinkManager().SetRadius(link, rad);
            }
            j++;
        }
    }

    AILogProgress("    - adjusted link radii");

    AILogProgress("Flight graph done!");

    DumpLinkRadii("flightnav_buildgraph.txt");
}

unsigned CFlightNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex, float /*range*/,
    Vec3* closestValid, bool returnSuspect, const char* requesterName)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    // Don't do anything if the system is not initialised.
    if (m_spans.empty())
    {
        AIWarning("CFlightNavRegion::GetEnclosing No flight navigation data available [Design bug]");
        return 0;
    }

    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    /*
    int sx = clamp( (int)floor( (pos.x - halfRange) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
    int sy = clamp( (int)floor( (pos.y - halfRange) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );
    int ex = clamp( (int)ceil( (pos.x + halfRange) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
    int ey = clamp( (int)ceil( (pos.y + halfRange) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );
    */

    /*
    int sx = (int)floor( pos.x / cellSize ) - m_heightFieldOriginX;
    int sy = (int)floor( pos.y / cellSize ) - m_heightFieldOriginY;

    Vec3                bestValid = pos;
    GraphNode*  bestNode = 0;
    float               bestDist = FLT_MAX;

    for( int y = sy - 1; y <= sy + 1; y++ )
    {
    for( int x = sx - 1; x <= sx + 1; x++ )
    {
    int xx = clamp( x, 0, m_heightFieldDimX - 1 );
    int yy = clamp( y, 0, m_heightFieldDimY - 1 );
    Vec3                valid;
    GraphNode*  node = GetEnclosing( xx, yy, pos, &valid );
    if( node && !node->links.empty() )
    {
    float   dist = (node->GetPos() - pos).GetLengthSquared();
    if( dist < bestDist )
    {
    bestNode = node;
    bestValid = valid;
    bestDist = dist;
    }
    }
    }
    }

    if( bestNode && closestValid )
    *closestValid = bestValid;

    return bestNode;*/

    int   x = clamp((int)floor(pos.x / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   y = clamp((int)floor(pos.y / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);

    Vec3 valid = pos;
    unsigned nodeIndex = GetEnclosing(x, y, pos, &valid);
    GraphNode*    node = m_pGraph->GetNodeManager().GetNode(nodeIndex);

    if (node && closestValid)
    {
        *closestValid = valid;
    }

    return nodeIndex;
}

unsigned CFlightNavRegion::GetEnclosing(int x, int y, const Vec3& pos, Vec3* closestValid, Vec3* spanPos)
{
    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    int startIdx = INVALID_IDX;
    int   idx = x + y * m_heightFieldDimX;

    if (m_spans[idx].m_childIdx != INVALID_IDX)
    {
        int childX = clamp((int)floor((pos.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize), 0, m_childSubDiv - 1);
        int childY = clamp((int)floor((pos.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize), 0, m_childSubDiv - 1);

        startIdx = m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
    }
    else
    {
        startIdx = idx;
    }

    for (int srcIdx = startIdx; srcIdx != INVALID_IDX; srcIdx = m_spans[srcIdx].m_nextIdx)
    {
        float   srcMin, srcMax;
        int srcNextIdx = m_spans[srcIdx].m_nextIdx;

        // Test against the current span, plus the space above it.
        srcMin = m_spans[srcIdx].m_minz;
        if (srcNextIdx != INVALID_IDX)
        {
            srcMax = m_spans[srcNextIdx].m_minz;
        }
        else
        {
            srcMax = srcMin + 100000;
        }

        if (pos.z >= srcMin && pos.z < srcMax)
        {
            if (closestValid)
            {
                if (pos.z < m_spans[srcIdx].m_maxz)
                {
                    *closestValid = pos;
                    (*closestValid).z = m_spans[srcIdx].m_maxz;
                }
            }

            if (spanPos)
            {
                (*spanPos).Set(m_spans[srcIdx].m_x, m_spans[srcIdx].m_y, m_spans[srcIdx].m_maxz);
            }

            return m_spans[srcIdx].m_graphNodeIndex;
        }
    }

    AIAssert(startIdx != INVALID_IDX);
    return m_spans[startIdx].m_graphNodeIndex;
}

Vec3 CFlightNavRegion::IsSpaceVoid(const Vec3& vPos, const Vec3& vForward, const Vec3& vWing, const Vec3& vUp) const
{
    // check if the space is void inside of the box which is made by these 8 point
    // (vPos)      (vPos+vForward)     (vPos+vWing)     (vPos+vForward+vWing)
    // (vPos+vUp)  (vPos+vForward+vUp) (vPos+vWing+vUp) (vPos+vForward+vWing+vUp)

    // returns the first point which is not void, as (x,y,max hight(or max hightAbove))
    // otherwize returns 0 vector;

    if (m_spans.empty())
    {
        AIWarning("CFlightNavRegion::IsSpaceVoid No flight navigation data available [Design bug]");
        return ZERO;
    }
    /*

        Vec3 vCapCenter = vPos + vForward * 0.75f;
        Vec3 vCapFwd = vForward * 0.5;

        primitives::capsule capsulePrim;
        capsulePrim.center = 0.5f * ( vPos + vForward );
        capsulePrim.axis = vForward;
        capsulePrim.hh = 0.5f * capsulePrim.axis.NormalizeSafe(Vec3_OneZ);
        capsulePrim.r = vWing.GetLength() * 0.5f;
        IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
        float d = pPhysics->PrimitiveWorldIntersection(capsulePrim.type, &capsulePrim, Vec3(ZERO),
        ent_static | ent_terrain | ent_ignore_noncolliding, 0, 0, geom_colltype0);
    */


    const float alomostzero     = 0.0001f;  //(normaliy 5 disits has almost no error for float)
    const float cellSize        = (float)(m_childSubDiv * m_terrainDownSample);
    const float childCellSize   = (float)m_terrainDownSample;

    float ls = vForward.GetLength();
    float lt = vWing.GetLength();
    float lu = vUp.GetLength();

    float ds = (ls < alomostzero) ? 1.0f : childCellSize / ls;
    float dt = (lt < alomostzero) ? 1.0f : childCellSize / lt;
    float du = (lu < alomostzero) ? 1.0f : childCellSize / lu;

    Vec3    vCheckPoint;
    Vec3    vReturnVec(ZERO);

    float   terrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainHeight
        , &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats
        , vPos.x, vPos.y, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP
        , nullptr);

    float   waterHeight = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(vPos) : GetISystem()->GetI3DEngine()->GetWaterLevel(&vPos);
    float   height =  max(terrainHeight, waterHeight) + 20.0f;
    float   heightAbove = height + 1000.0f;

    int     lastIndex = -1;
    bool    bCantGo = false;

    Vec3    ddd[8];
    {
        ddd[0] = vPos;
        ddd[1] = vPos + vForward;
        ddd[2] = vPos + vForward + vWing;
        ddd[3] = vPos + vWing;
        ddd[4] = vPos + vUp;
        ddd[5] = vPos + vForward + vUp;
        ddd[6] = vPos + vForward + vWing + vUp;
        ddd[7] = vPos + vWing + vUp;
    }


    for (float s = 0.0f; s < 1.0f; s += ds)
    {
        for (float t = 0.0f; t < 1.0f; t += dt)
        {
            for (float u = 0.0f; u < 1.0f; u += du)
            {
                // for using GetHeightAndSpaceAt, I avoided making a kind of the same function.
                // but when the cell is the same, it is better skip calling.

                vCheckPoint = vPos + vForward * s + vWing * t + vUp * u;

                int x = clamp((int)floor(vCheckPoint.x / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
                int y = clamp((int)floor(vCheckPoint.y / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);

                int startIdx = INVALID_IDX;
                int idx     = x + y * m_heightFieldDimX;

                if (m_spans[idx].m_childIdx != INVALID_IDX)
                {
                    int childX  = clamp((int)floor((vCheckPoint.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize), 0, m_childSubDiv - 1);
                    int childY  = clamp((int)floor((vCheckPoint.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize), 0, m_childSubDiv - 1);
                    startIdx    = m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
                }
                else
                {
                    startIdx = idx;
                }

                if (startIdx != lastIndex)
                {
                    lastIndex = startIdx;
                    GetHeightAndSpaceAt(vCheckPoint, height, heightAbove);
                    if (waterHeight > height)
                    {
                        height = waterHeight + 1.0f;
                    }
                }

                if (vCheckPoint.z > height && vCheckPoint.z < heightAbove)
                {
                    // this cell is void :)
                }
                else
                {
                    // when this point is not void :(
                    if (vCheckPoint.z > heightAbove)
                    {
                        // treat as closed space
                        vReturnVec.x = vCheckPoint.x;
                        vReturnVec.y = vCheckPoint.y;
                        vReturnVec.z = 10000.0f;
                        return vReturnVec;
                    }

                    if (vCheckPoint.z <= height)
                    {
                        if (bCantGo == false)
                        {
                            bCantGo = true;
                            vReturnVec.x = vCheckPoint.x;
                            vReturnVec.y = vCheckPoint.y;
                            vReturnVec.z = height;
                        }
                        else if (height > vReturnVec.z)
                        {
                            vReturnVec.x = vCheckPoint.x;
                            vReturnVec.y = vCheckPoint.y;
                            vReturnVec.z = height;
                        }
                    }
                }
            }
        }
    }

    unsigned char   r, g, b;
    if (vReturnVec.z > 0)
    {
        r = 255, g = 0, b = 0;
    }
    else if (vPos.z + vUp.z / 2.0f + 5.0f < vReturnVec.z)
    {
        r = 0, g = 255, b = 255;
    }
    else
    {
        r = 255, g = 255, b = 255;
    }

    gEnv->pAISystem->AddDebugLine(ddd[0], ddd[1], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[1], ddd[2], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[2], ddd[3], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[3], ddd[0], r, g, b, 1.0f);

    gEnv->pAISystem->AddDebugLine(ddd[4], ddd[5], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[5], ddd[6], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[6], ddd[7], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[7], ddd[4], r, g, b, 1.0f);

    gEnv->pAISystem->AddDebugLine(ddd[0], ddd[4], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[1], ddd[5], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[2], ddd[6], r, g, b, 1.0f);
    gEnv->pAISystem->AddDebugLine(ddd[3], ddd[7], r, g, b, 1.0f);

    return vReturnVec;
}

void CFlightNavRegion::GetHeightAndSpaceAt(const Vec3& pos, float& height, float& heightAbove) const
{
    const float   cellSize = (float)(m_childSubDiv * m_terrainDownSample);
    const float   childCellSize = (float)m_terrainDownSample;

    int   x = clamp((int)floor(pos.x / cellSize) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1);
    int   y = clamp((int)floor(pos.y / cellSize) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1);

    int startIdx = INVALID_IDX;
    int   idx = x + y * m_heightFieldDimX;

    if (m_spans[idx].m_childIdx != INVALID_IDX)
    {
        int childX = clamp((int)floor((pos.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize), 0, m_childSubDiv - 1);
        int childY = clamp((int)floor((pos.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize), 0, m_childSubDiv - 1);

        startIdx = m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
    }
    else
    {
        startIdx = idx;
    }

    for (int srcIdx = startIdx; srcIdx != INVALID_IDX; srcIdx = m_spans[srcIdx].m_nextIdx)
    {
        float   srcMin, srcMax;
        int srcNextIdx = m_spans[srcIdx].m_nextIdx;

        // Test against the current span, plus the space above it.
        srcMin = m_spans[srcIdx].m_minz;
        if (srcNextIdx != INVALID_IDX)
        {
            srcMax = m_spans[srcNextIdx].m_minz;
        }
        else
        {
            srcMax = srcMin + 100000;
        }

        // Test against the current span, plus the space above it.
        srcMin = (m_spans[srcIdx].m_minz + m_spans[srcIdx].m_maxz) * 0.5f;
        float   bot = 0, top = 0;
        if (srcNextIdx != INVALID_IDX)
        {
            srcMax = (m_spans[srcNextIdx].m_minz + m_spans[srcNextIdx].m_maxz) * 0.5f;
            bot = m_spans[srcIdx].m_maxz;
            top = m_spans[srcNextIdx].m_minz;
        }
        else
        {
            srcMax = srcMin + 100000;
            bot = m_spans[srcIdx].m_maxz;
            top = bot + 100000;
        }

        if (pos.z >= srcMin && pos.z < srcMax)
        {
            height = bot;
            heightAbove = top;
            return;
        }
    }

    if (startIdx != INVALID_IDX)
    {
        // we're not in a valid span, so return the terrain (lowest span) height
        height = m_spans[startIdx].m_maxz;
        ;
        heightAbove = 100000;
        return;
    }

    // We should never get here.
    AIError("CFlightNavRegion::GetHeightAndSpaceAt failed for pos (%5.2f, %5.2f, %5.2f)",
        pos.x, pos.y, pos.z);
    height = pos.z;
    heightAbove = 1000.0f;
}

//====================================================================
// Serialize
//====================================================================
void CFlightNavRegion::Serialize(TSerialize ser)
{
    ser.BeginGroup("FlightNavRegion");

    ser.EndGroup();
}

//===================================================================
// DumpLinkRadii
//===================================================================
void CFlightNavRegion::DumpLinkRadii(const char* filename)
{
    /*
    FILE*   pStream = fopen( filename, "w" );
    if( pStream )
    fprintf( pStream, "Flight navigation span links\n\n" );

    float   minRad = FLT_MAX;
    float   maxRad = FLT_MIN;

    for( unsigned i = 0; i < m_spans.size(); i++ )
    {
    GraphNode*  node = m_spans[i].m_pGraphNode;
    AIAssert( node );

    if( pStream )
    fprintf( pStream, "%d r:%.2f\n", i, m_spans[i].m_maxRadius );

    int j = 0;
    for( VectorOfLinks::iterator linkIt = node->links.begin(); linkIt != node->links.end(); ++linkIt )
    {
    GraphLink&  link = (*linkIt);
    GraphNode*  nextNode = link.pNextNode;
    int nextIdx = nextNode->GetFlightNavData()->nSpanIdx;
    if( nextIdx != INVALID_IDX )
    {
    float   rad = link.GetRadius();
    if( pStream )
    fprintf( pStream, " -> %d r:%.2f = min(%.2f, %.2f) \n", j, rad, m_spans[i].m_maxRadius, m_spans[nextIdx].m_maxRadius );
    minRad = min( rad, minRad );
    maxRad = max( rad, maxRad );
    }
    j++;
    }
    }

    if( pStream )
    fprintf( pStream, "\n** minRad=%f  maxRad=%f\n\n", minRad, maxRad );

    fclose( pStream );
    */
}
