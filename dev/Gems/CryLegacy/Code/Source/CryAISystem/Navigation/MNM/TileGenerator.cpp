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
#include "TileGenerator.h"
#include "Voxelizer.h"
#include "HashComputer.h"


//#pragma optimize("", off)
//#pragma inline_depth(0)


namespace MNM
{
    //Code relies on the ordering (specifically we must list l/r/u/d cyclically before corners)
    static const int NeighbourOffset_TileGenerator[8][2] =
    {
        { 0, 1},
        {-1, 0},
        { 1, 0},
        { 0, -1},
        {-1, -1},
        { 1, -1},
        { 1, 1},
        {-1, 1},
    };

    static uchar CornerTable[3][3][3] =
    {
        {
            { 0, 0, 1   },
            {   0, 0, 0 },
            { 1, 1, 1 },
        },
        {
            { 0, 0, 1   },
            {   0, 0, 0 },
            { 1, 0, 0 },
        },
        {
            { 1, 1, 0   },
            {   0, 0, 1 },
            { 1, 0, 0 },
        },
    };

    /*

        Expanded, easier to read

        CornerTable[UW][UW][UW] = 0;
        CornerTable[UW][UW][NB] = 0;
        CornerTable[UW][UW][WB] = 1;
        CornerTable[UW][NB][UW] = 0;
        CornerTable[UW][NB][NB] = 0;
        CornerTable[UW][NB][WB] = 0;
        CornerTable[UW][WB][UW] = 1;
        CornerTable[UW][WB][NB] = 1;
        CornerTable[UW][WB][WB] = 1;

        CornerTable[NB][UW][UW] = 0;
        CornerTable[NB][UW][NB] = 0;
        CornerTable[NB][UW][WB] = 1;
        CornerTable[NB][NB][UW] = 0;
        CornerTable[NB][NB][NB] = 0;
        CornerTable[NB][NB][WB] = 0;
        CornerTable[NB][WB][UW] = 1;
        CornerTable[NB][WB][NB] = 0;
        CornerTable[NB][WB][WB] = 0;

        CornerTable[WB][UW][UW] = 1;
        CornerTable[WB][UW][NB] = 1;
        CornerTable[WB][UW][WB] = 0;
        CornerTable[WB][NB][UW] = 0;
        CornerTable[WB][NB][NB] = 0;
        CornerTable[WB][NB][WB] = 1;
        CornerTable[WB][WB][UW] = 1;
        CornerTable[WB][WB][NB] = 0;
        CornerTable[WB][WB][WB] = 0;
    */

    void TileGenerator::Clear()
    {
        m_profiler = ProfilerType();

        m_triangles.clear();
        m_vertices.clear();
        m_bvtree.clear();

        m_spanGrid.Clear();
        m_distances.clear();
        m_labels.clear();
        m_paint.clear();
        m_regions.clear();
        m_polygons.clear();
        m_tracerPaths.clear();
        m_spanGridFlagged.Clear();
    }

    bool TileGenerator::Generate(const Params& params, Tile& tile, uint32* tileHash)
    {
        if ((params.sizeX > MaxTileSizeX) || (params.sizeY > MaxTileSizeY) || (params.sizeZ > MaxTileSizeZ))
        {
            return false;
        }

        const float MinVoxelSize = 0.025f;
        if ((params.voxelSize.x < MinVoxelSize) || (params.voxelSize.y < MinVoxelSize) || (params.voxelSize.z < MinVoxelSize))
        {
            return false;
        }

        Clear();

        m_params = params;
        m_profiler = ProfilerType();
        m_top = (size_t)(m_params.sizeZ / m_params.voxelSize.z + 0.10f);

        AABB aabb(m_params.origin, m_params.origin + Vec3i(m_params.sizeX, m_params.sizeY, m_params.sizeZ));

        if (m_params.boundary && !m_params.boundary->Overlaps(aabb))
        {
            return false;
        }

        const size_t border = BorderSizeH();
        const size_t borderV = BorderSizeV();
        if (border | borderV)
        {
            aabb.Expand(Vec3(border * m_params.voxelSize.x, border * m_params.voxelSize.y, borderV * m_params.voxelSize.z));
        }

        aabb.max.z += m_params.agent.height * m_params.voxelSize.z;

        bool fullyContained = true;

        if (m_params.boundary || m_params.exclusionCount)
        {
            for (size_t e = 0; e < m_params.exclusionCount; ++e)
            {
                BoundingVolume::ExtendedOverlap eoverlap = m_params.exclusions[e].Contains(aabb);

                if (eoverlap == BoundingVolume::FullOverlap)
                {
                    return false;
                }
                else if (eoverlap == BoundingVolume::PartialOverlap)
                {
                    fullyContained = false;
                }
            }

            BoundingVolume::ExtendedOverlap ioverlap =
                m_params.boundary ? m_params.boundary->Contains(aabb) : BoundingVolume::FullOverlap;

            if (ioverlap == BoundingVolume::NoOverlap)
            {
                return false;
            }
            else if (ioverlap == BoundingVolume::PartialOverlap)
            {
                fullyContained = false;
            }
        }

        CompactSpanGrid().Swap(m_spanGrid);
        SpanExtraInfo().swap(m_distances);
        SpanExtraInfo().swap(m_labels);
        Regions().swap(m_regions);
        Polygons().swap(m_polygons);

        Vertices().swap(m_vertices);
        Triangles().swap(m_triangles);
        BVTree().swap(m_bvtree);

        CompactSpanGrid().Swap(m_spanGridFlagged);

        uint32 hashSeed = 0;
        if (fullyContained)
        {
            hashSeed = 0xf007b00b;
        }
        else
        {
            HashComputer hash;

            if (m_params.boundary || m_params.exclusionCount)
            {
                for (size_t e = 0; e < m_params.exclusionCount; ++e)
                {
                    const BoundingVolume& volume = m_params.exclusions[e];
                    const size_t vertexCount = volume.vertices.size();

                    for (size_t v = 0; v < vertexCount; ++v)
                    {
                        hash.Add(volume.vertices[v]);
                    }

                    hash.Add(volume.height);
                }

                if (m_params.boundary)
                {
                    const BoundingVolume& volume = *m_params.boundary;
                    const size_t vertexCount = volume.vertices.size();

                    for (size_t v = 0; v < vertexCount; ++v)
                    {
                        hash.Add(volume.vertices[v]);
                    }

                    hash.Add(volume.height);
                }
            }

            hash.Complete();
            hashSeed = hash.GetValue();
        }

        uint32 hashValue = 0;
        size_t triCount = VoxelizeVolume(aabb, hashSeed, &hashValue);

        if (tileHash)
        {
            *tileHash = hashValue;
        }

        if (!triCount)
        {
            return false;
        }

        tile.hashValue = hashValue;

        FilterWalkable(aabb, fullyContained);

        if (!m_spanGrid.GetSpanCount())
        {
            return false;
        }

        ComputeDistanceTransform();
        //BlurDistanceTransform();

        if (!ExtractContours())
        {
            return false;
        }

        FilterBadRegions(m_params.minWalkableArea);
        SimplifyContours();
        Triangulate();

        if (m_params.flags & Params::BuildBVTree)
        {
            BuildBVTree();
        }

        if (m_vertices.empty())
        {
            return false;
        }

        static const size_t MaxTriangleCount = 1024;

        if (m_triangles.size() > MaxTriangleCount)
        {
            AIWarning("[MNM] Too many triangles in one tile. Coords: [%.2f,%.2f,%.2f]", aabb.GetCenter().x, aabb.GetCenter().y, aabb.GetCenter().z);
        }

        tile.CopyTriangles(&m_triangles.front(), min(MaxTriangleCount, m_triangles.size()));
        tile.CopyVertices(&m_vertices.front(), m_vertices.size());

        if (m_params.flags & Params::BuildBVTree)
        {
            tile.CopyNodes(&m_bvtree.front(), m_bvtree.size());
        }

        return true;
    }

    const TileGenerator::ProfilerType& TileGenerator::GetProfiler() const
    {
        return m_profiler;
    }

    size_t TileGenerator::VoxelizeVolume(const AABB& volume, uint32 hashValueSeed, uint32* hashValue)
    {
        m_profiler.StartTimer(Voxelization);

        WorldVoxelizer voxelizer;

        voxelizer.Start(volume, m_params.voxelSize);
        size_t triCount = voxelizer.ProcessGeometry(hashValueSeed,
                m_params.flags & Params::NoHashTest ? 0 : m_params.hashValue, hashValue, m_params.agent.callback);
        voxelizer.CalculateWaterDepth();

        m_profiler.AddMemory(DynamicSpanGridMemory, voxelizer.GetSpanGrid().GetMemoryUsage());

        m_spanGrid.BuildFrom(voxelizer.GetSpanGrid());

        m_profiler.StopTimer(Voxelization);

        m_profiler.AddMemory(CompactSpanGridMemory, m_spanGrid.GetMemoryUsage());
        m_profiler.FreeMemory(DynamicSpanGridMemory, voxelizer.GetSpanGrid().GetMemoryUsage());

        m_profiler.AddStat(VoxelizationTriCount, triCount);

        return triCount;
    }

    void TileGenerator::FilterWalkable(const AABB& aabb, bool fullyContained)
    {
        m_profiler.StartTimer(Filter);

        const size_t gridWidth = m_spanGrid.GetWidth();
        const size_t gridHeight = m_spanGrid.GetHeight();
        const size_t gridSize = gridWidth * gridHeight;

        const size_t heightVoxelCount = m_params.agent.height;
        const size_t climbableVoxelCount = m_params.agent.climbableHeight;
        const size_t border = BorderSizeH();
        const float climbableInclineGradient = m_params.climbableInclineGradient;
        const float climbableStepRatio = m_params.climbableStepRatio;   // The bigger the step height, the more step width required to be traversable

        size_t const inclineTestCount(climbableVoxelCount + 1);   // Must be bigger that climbable height to correctly resolve valid steps
        size_t const climbableInclineGradientLowerBound((size_t)floor(climbableInclineGradient));
        float const climbableInclineGradientSquared(climbableInclineGradient * climbableInclineGradient);
        const size_t extraHeight = m_params.agent.height;
        const size_t spaceTop = (2 * BorderSizeV()) + extraHeight + m_top;

        size_t nonWalkableCount = 0;

        const size_t axisNeighbourCount = 4;

        for (size_t y = 0; y < gridHeight; ++y)
        {
            const size_t ymult = y * gridWidth;

            for (size_t x = 0; x < gridWidth; ++x)
            {
                if (const CompactSpanGrid::Cell cell = m_spanGrid[x + ymult])
                {
                    bool boundaryCell = IsBoundaryCell_Static(x, y, border, gridWidth, gridHeight);
                    uint32 const boundaryFlag(boundaryCell ? TileBoundary : 0);

                    const size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        CompactSpanGrid::Span& span = m_spanGrid.GetSpan(cell.index + s);

                        span.flags |= boundaryFlag;

                        if (span.backface || span.depth > m_params.agent.maxWaterDepth)
                        {
                            span.flags |= NotWalkable;
                            ++nonWalkableCount;
                        }
                        else
                        {
                            const size_t top = span.bottom + span.height;

                            size_t nextBottom = spaceTop;

                            if (s + 1 < count)
                            {
                                const CompactSpanGrid::Span& nextSpan = m_spanGrid.GetSpan(cell.index + s + 1);
                                nextBottom = nextSpan.bottom;
                            }

                            const size_t clearance = nextBottom - top;

                            if (clearance < heightVoxelCount)
                            {
                                span.flags |= NotWalkable;
                                ++nonWalkableCount;
                            }
                            else
                            {
                                // Assume all neighbouring region is good - try to prove otherwise
                                bool neighbourTest(true);

                                float pVars[axisNeighbourCount] = {0.0f};

                                for (size_t n = 0; n < axisNeighbourCount; ++n)
                                {
                                    const size_t nx = x + NeighbourOffset_TileGenerator[n][0];
                                    const size_t ny = y + NeighbourOffset_TileGenerator[n][1];

                                    // Neighbour off grid - pass
                                    if (nx >= gridWidth || ny >= gridHeight)
                                    {
                                        continue;
                                    }

                                    const size_t neighbourSpanGridIndex = (ny * gridWidth) + nx;
                                    const CompactSpanGrid::Cell& ncell = m_spanGrid[neighbourSpanGridIndex];
                                    if (!ncell)
                                    {
                                        //Empty neighbour on grid - fail
                                        neighbourTest = false;
                                        break;
                                    }

                                    const size_t ncount = ncell.count;
                                    const size_t nindex = ncell.index;

                                    size_t ptopLast;
                                    size_t pnextBottomLast;
                                    size_t dpTopFirst;
                                    size_t dpTopLast;

                                    int sdpTopFirst;
                                    int sdpTopLast;

                                    // Assume this neighbour cell is not valid. Succeed when we find just one valid adjacent span
                                    bool nCellValid(false);

                                    for (size_t ns = 0; ns < ncount; ++ns)
                                    {
                                        const size_t nsindex = nindex + ns;
                                        const CompactSpanGrid::Span& nspan = m_spanGrid.GetSpan(nsindex);

                                        size_t ntop = nspan.bottom + nspan.height;

                                        size_t nnextBottom = spaceTop;

                                        if (ns + 1 < ncount)
                                        {
                                            const CompactSpanGrid::Span& nnextSpan = m_spanGrid.GetSpan(ncell.index + ns + 1);
                                            nnextBottom = nnextSpan.bottom;
                                        }

                                        const size_t dTop = (size_t)abs((int)(ntop - top));

                                        //Test validity
                                        if (dTop <= climbableVoxelCount
                                            &&  min(nextBottom, nnextBottom) >= max(top, ntop) + heightVoxelCount)
                                        {
                                            ptopLast = ntop;
                                            pnextBottomLast = nnextBottom;
                                            dpTopFirst = dpTopLast = dTop;

                                            sdpTopFirst = sdpTopLast = (int)(ntop - top);


                                            nCellValid = true;

                                            break;
                                        }
                                    }

                                    if (!nCellValid)
                                    {
                                        neighbourTest = false;
                                        break;
                                    }

                                    if (dpTopFirst > 0)
                                    {
                                        size_t const stepTestCount((size_t)ceil(dpTopFirst * climbableStepRatio));
                                        size_t const stepTestTolerance(stepTestCount - 1);
                                        bool const isStep(dpTopFirst > (climbableInclineGradientLowerBound + 1));
                                        bool stepTest(true);

                                        AIAssert(stepTestCount <= inclineTestCount);

                                        size_t ptopMin(ptopLast);
                                        size_t ptopMax(ptopLast);

                                        size_t pTopOffsMax(dpTopFirst);

                                        size_t pc(2);

                                        for (; pc <= inclineTestCount; ++pc)
                                        {
                                            size_t px = x + NeighbourOffset_TileGenerator[n][0] * pc;
                                            size_t py = y + NeighbourOffset_TileGenerator[n][1] * pc;

                                            const CompactSpanGrid::Cell pcell = m_spanGrid.GetCell(px, py);

                                            if (!pcell)
                                            {
                                                //Empty neighbour - stop probe (but no fail)
                                                break;
                                            }

                                            const size_t pcount = pcell.count;
                                            const size_t pindex = pcell.index;

                                            bool pCellValid(false);

                                            for (size_t ps = 0; ps < pcount; ++ps)
                                            {
                                                const size_t psindex = pindex + ps;
                                                const CompactSpanGrid::Span& pspan = m_spanGrid.GetSpan(psindex);

                                                size_t ptop = pspan.bottom + pspan.height;

                                                size_t pnextBottom = spaceTop;

                                                if (ps + 1 < pcount)
                                                {
                                                    const CompactSpanGrid::Span& pnextSpan = m_spanGrid.GetSpan(pcell.index + ps + 1);
                                                    pnextBottom = pnextSpan.bottom;
                                                }

                                                const size_t dpTop = (size_t)abs((int)(ptop - ptopLast));

                                                //Test validity
                                                if (dpTop <= climbableVoxelCount
                                                    &&  min(pnextBottomLast, pnextBottom) >= max(ptopLast, ptop) + heightVoxelCount)
                                                {
                                                    // Track range of tops in probe
                                                    ptopMin = min(ptopMin, ptop);
                                                    ptopMax = max(ptopMax, ptop);

                                                    const size_t pTopOffs = (size_t)abs((int)(ptop - top));
                                                    pTopOffsMax = max(pTopOffsMax, pTopOffs);

                                                    sdpTopLast = (int)(ptop - ptopLast);

                                                    ptopLast = ptop;
                                                    pnextBottomLast = pnextBottom;
                                                    dpTopLast = dpTop;


                                                    nCellValid = true;

                                                    break;
                                                }
                                            }

                                            if (!nCellValid)
                                            {
                                                break;
                                            }

                                            if (isStep)
                                            {
                                                if (pc <= stepTestCount
                                                    &&  pTopOffsMax > stepTestTolerance + dpTopFirst)
                                                {
                                                    stepTest = false;
                                                }
                                            }
                                            else
                                            {
                                                if (sdpTopLast > sdpTopFirst + 1
                                                    ||  sdpTopLast < sdpTopFirst - 1)
                                                {
                                                    // stop probe (but no fail)
                                                    break;
                                                }

                                                pVars[n] = (float)(ptopMax - ptopMin);
                                            }
                                        }

                                        if (isStep
                                            &&  !stepTest
                                            &&  (pTopOffsMax > climbableVoxelCount))
                                        {
                                            neighbourTest = false;
                                        }
                                        else
                                        {
                                            pVars[n] /= pc - 1;
                                        }
                                    }
                                }

                                if (neighbourTest)
                                {
                                    float previousProbeGainSquared(pVars[axisNeighbourCount - 1] * pVars[axisNeighbourCount - 1]);

                                    // Test for probe pairs at 90degrees giving a net incline above tolerance
                                    for (size_t n = 0; n < axisNeighbourCount; ++n)
                                    {
                                        float thisProbeGainSquared(pVars[n] * pVars[n]);
                                        if ((float)(thisProbeGainSquared + previousProbeGainSquared) > climbableInclineGradientSquared)
                                        {
                                            neighbourTest = false;

                                            break;
                                        }
                                        previousProbeGainSquared = thisProbeGainSquared;
                                    }
                                }

                                if (!neighbourTest)
                                {
                                    span.flags |= NotWalkable;
                                    ++nonWalkableCount;
                                }
                            }
                        }
                    }
                }
            }
        }

        // apply boundaries
        if (!fullyContained)
        {
            const float convX = 1.0f / m_params.voxelSize.x;
            const float convY = 1.0f / m_params.voxelSize.y;
            const float convZ = 1.0f / m_params.voxelSize.z;

            const Vec3 voxelSize = m_params.voxelSize;
            const Vec3 halfVoxel = m_params.voxelSize * 0.5f;

            const Vec3 bmax = aabb.max - aabb.min;

            for (size_t e = 0; e < m_params.exclusionCount; ++e)
            {
                const BoundingVolume& exclusion = m_params.exclusions[e];

                if (exclusion.Overlaps(aabb))
                {
                    const Vec3 emin = (exclusion.aabb.min - aabb.min);
                    const Vec3 emax = (exclusion.aabb.max - aabb.min);

                    uint16 xmin = (uint16)(std::max(0.0f, emin.x) * convX);
                    uint16 xmax = (uint16)(std::min(bmax.x, emax.x) * convX);

                    uint16 ymin = (uint16)(std::max(0.0f, emin.y) * convY);
                    uint16 ymax = (uint16)(std::min(bmax.y, emax.y) * convY);

                    uint16 zmin = (uint16)(std::max(0.0f, emin.z) * convZ);
                    uint16 zmax = (uint16)(std::min(bmax.z, emax.z) * convZ);

                    for (uint16 y = ymin; y <= ymax; ++y)
                    {
                        const size_t ymult = y * gridWidth;

                        for (uint16 x = xmin; x <= xmax; ++x)
                        {
                            if (const CompactSpanGrid::Cell& cell = m_spanGrid[x + ymult])
                            {
                                for (size_t s = 0; s < cell.count; ++s)
                                {
                                    CompactSpanGrid::Span& span = m_spanGrid.GetSpan(cell.index + s);

                                    if (span.flags & NotWalkable)
                                    {
                                        continue;
                                    }

                                    size_t top = span.bottom + span.height;

                                    if ((top >= zmin) && (top <= zmax))
                                    {
                                        const Vec3 point = aabb.min + halfVoxel + Vec3(x * voxelSize.x, y * voxelSize.y, top * voxelSize.z);

                                        if (exclusion.Contains(point))
                                        {
                                            span.flags |= NotWalkable;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (m_params.boundary)
            {
                const BoundingVolume& boundary = *m_params.boundary;

                for (uint16 y = 0; y <= gridHeight; ++y)
                {
                    const size_t ymult = y * gridWidth;

                    for (uint16 x = 0; x <= gridWidth; ++x)
                    {
                        const CompactSpanGrid::Cell& cell = m_spanGrid[x + ymult];

                        if (cell)
                        {
                            for (size_t s = 0; s < cell.count; ++s)
                            {
                                CompactSpanGrid::Span& span = m_spanGrid.GetSpan(cell.index + s);

                                if (span.flags & NotWalkable)
                                {
                                    continue;
                                }

                                size_t top = span.bottom + span.height;

                                const Vec3 point = aabb.min + halfVoxel + Vec3(x * voxelSize.x, y * voxelSize.y, top * voxelSize.z);

                                if (!boundary.Contains(point))
                                {
                                    span.flags |= NotWalkable;
                                }
                            }
                        }
                    }
                }
            }
        }

        CompactSpanGrid compact;
        compact.CompactExcluding(m_spanGrid, NotWalkable, m_spanGrid.GetSpanCount() - nonWalkableCount);

        m_profiler.AddMemory(CompactSpanGridMemory, compact.GetMemoryUsage());

        compact.Swap(m_spanGrid);

        m_profiler.StopTimer(Filter);

        if (m_params.flags & Params::DebugInfo)
        {
            m_spanGridFlagged.Swap(compact);
        }
        else
        {
            m_profiler.FreeMemory(CompactSpanGridMemory, compact.GetMemoryUsage());
        }
    }

    inline bool IsBorderLabel(uint16 label)
    {
        return (label & (TileGenerator::BorderLabelH | TileGenerator::BorderLabelV)) != 0;
    }

    inline bool IsLabelValid(uint16 label)
    {
        return (label < TileGenerator::FirstInvalidLabel);
    }

    /*  inline uint16 ConsolidateLabel(uint16 label)
        {
            if (label < TileGenerator::FirstInvalidLabel)
                return label;
            return TileGenerator::FirstInvalidLabel;
        }*/

    void TileGenerator::ComputeDistanceTransform()
    {
        m_profiler.StartTimer(DistanceTransform);

        const size_t gridWidth = m_spanGrid.GetWidth();
        const size_t gridHeight = m_spanGrid.GetHeight();
        const size_t spanCount = m_spanGrid.GetSpanCount();

        const size_t climbableVoxelCount = m_params.agent.climbableHeight;

        m_distances.resize(spanCount);

        m_profiler.AddMemory(SegmentationMemory, m_distances.size() * sizeof(SpanExtraInfo::value_type));

        std::fill(m_distances.begin(), m_distances.end(), NoLabel);

        const size_t KStraight = 2;
        const size_t KDiagonal = 3;

        struct SweepElement
        {
            int x;
            int y;
            int k;
        };

        const size_t SweepPixelCount = 4;

        const SweepElement downSweep[SweepPixelCount] =
        {
            { -1, 0, KStraight },
            { -1, -1, KDiagonal },
            {  0, -1, KStraight },
            {  1, -1, KDiagonal },
        };

        for (size_t y = 0; y < gridHeight; ++y)
        {
            for (size_t x = 0; x < gridWidth; ++x)
            {
                if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x, y))
                {
                    size_t index = cell.index;
                    size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index + s);

                        const size_t top = span.bottom + span.height;
                        const uint16 current = m_distances[index + s];

                        uint16 sweepValues[SweepPixelCount];

                        for (size_t p = 0; p < SweepPixelCount; ++p)
                        {
                            sweepValues[p] = 0;

                            const size_t nx = x + downSweep[p].x;
                            const size_t ny = y + downSweep[p].y;

                            size_t nindex;
                            if (m_spanGrid.GetSpanAt(nx, ny, top, climbableVoxelCount, nindex))
                            {
                                sweepValues[p] = m_distances[nindex] + downSweep[p].k;
                            }
                        }

                        uint16 minimum = sweepValues[0];

                        for (size_t p = 1; p < SweepPixelCount; ++p)
                        {
                            minimum = std::min<uint16>(minimum, sweepValues[p]);
                        }

                        if (minimum < current)
                        {
                            m_distances[index + s] = minimum;
                        }
                    }
                }
            }
        }

        const SweepElement upSweep[SweepPixelCount] =
        {
            { 1, 0, KStraight },
            { 1, 1, KDiagonal },
            { 0, 1, KStraight },
            {-1, 1, KDiagonal },
        };

        for (size_t y = gridHeight - 1; y; --y)
        {
            for (size_t x = gridWidth; x; --x)
            {
                if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x, y))
                {
                    size_t index = cell.index;
                    size_t count = cell.count;

                    for (size_t s = 0; s < count; ++s)
                    {
                        const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index + s);

                        const size_t top = span.bottom + span.height;
                        const uint16 current = m_distances[index + s];

                        uint16 sweepValues[SweepPixelCount];

                        for (size_t p = 0; p < SweepPixelCount; ++p)
                        {
                            sweepValues[p] = 0;

                            const size_t nx = x + upSweep[p].x;
                            const size_t ny = y + upSweep[p].y;

                            size_t nindex;
                            if (m_spanGrid.GetSpanAt(nx, ny, top, climbableVoxelCount, nindex))
                            {
                                sweepValues[p] = m_distances[nindex] + upSweep[p].k;
                            }
                        }

                        uint16 minimum = sweepValues[0];

                        for (size_t p = 1; p < SweepPixelCount; ++p)
                        {
                            minimum = std::min<uint16>(minimum, sweepValues[p]);
                        }

                        if (minimum < current)
                        {
                            m_distances[index + s] = minimum;
                        }
                    }
                }
            }
        }

        m_profiler.StopTimer(DistanceTransform);
    }

    void TileGenerator::BlurDistanceTransform()
    {
        if (!m_params.blurAmount)
        {
            return;
        }

        m_profiler.StartTimer(Blur);

        const size_t threshold = 1;
        const size_t gridWidth = m_spanGrid.GetWidth();
        const size_t gridHeight = m_spanGrid.GetHeight();

        const size_t climbableVoxelCount = m_params.agent.climbableHeight;

        m_labels.resize(m_distances.size());

        for (size_t i = 0; i < m_params.blurAmount; ++i)
        {
            for (size_t y = 0; y < gridHeight; ++y)
            {
                for (size_t x = 0; x < gridWidth; ++x)
                {
                    if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x, y))
                    {
                        size_t index = cell.index;
                        size_t count = cell.count;

                        for (size_t s = 0; s < count; ++s)
                        {
                            const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index + s);
                            const size_t orig = m_distances[index + s];

                            if (orig > threshold)
                            {
                                const size_t top = span.bottom + span.height;
                                size_t accum = orig;

                                for (size_t n = 0; n < 8; ++n)
                                {
                                    const size_t nx = x + NeighbourOffset_TileGenerator[n][0];
                                    const size_t ny = y + NeighbourOffset_TileGenerator[n][1];

                                    size_t nindex;
                                    if (m_spanGrid.GetSpanAt(nx, ny, top, climbableVoxelCount, nindex))
                                    {
                                        accum += m_distances[nindex];
                                    }
                                    else
                                    {
                                        accum += orig;
                                    }
                                }

                                const size_t c = (accum + 5) / 9; // round
                                m_labels[index + s] = c;
                            }
                            else
                            {
                                m_labels[index + s] = orig;
                            }
                        }
                    }
                }
            }

            if ((i & 1) == 0)
            {
                m_labels.swap(m_distances);
            }
        }

        m_profiler.StopTimer(Blur);
    }

    struct StreamElement
    {
        StreamElement(size_t _x, size_t _y, size_t _span)
            : x(_x)
            , y(_y)
            , span(_span)
        {
        }

        uint16 x;
        uint16 y;
        size_t span;

        bool operator==(const StreamElement& other) const
        {
            return (x == other.x) && (y == other.y) && (span == other.span);
        }

        bool operator<(const StreamElement& other) const
        {
            if (x == other.x)
            {
                if (y == other.y)
                {
                    return (span < other.span);
                }
                return (y < other.y);
            }

            return x < other.x;
        }
    };


    typedef std::vector<StreamElement> LinearStream;

    size_t RunStream(const CompactSpanGrid& spanGrid, size_t climbableVoxelCount, LinearStream& unexplored,
        const StreamElement& first, LinearStream& stream, const uint16* data, size_t erosion, uint16* labels)
    {
        stream.clear();
        stream.push_back(first);

        unexplored.clear();
        unexplored.push_back(first);

        while (!unexplored.empty())
        {
            const StreamElement y = unexplored.back();
            unexplored.pop_back();

            const size_t yx = y.x;
            const size_t yy = y.y;

            const CompactSpanGrid::Span& yspan = spanGrid.GetSpan(first.span);
            const size_t ytop = yspan.bottom + yspan.height;

            size_t minimum = erosion;
            bool bfirst = true;

            while (bfirst)
            {
                for (size_t n = 0; n < 4; ++n)
                {
                    size_t nx = yx + NeighbourOffset_TileGenerator[n][0];
                    size_t ny = yy + NeighbourOffset_TileGenerator[n][1];

                    size_t nindex;
                    if (spanGrid.GetSpanAt(nx, ny, ytop, climbableVoxelCount, nindex))
                    {
                        uint16 label = labels[nindex];

                        if (!IsBorderLabel(label))
                        {
                            uint16 ndata = data[nindex];
                            if (ndata > minimum)
                            {
                                minimum = ndata;
                            }
                        }
                    }
                }

                size_t zx = ~0ul;
                size_t zy = ~0ul;
                size_t zspan = 0;

                for (size_t n = 0; n < 4; ++n)
                {
                    const size_t nx = yx + NeighbourOffset_TileGenerator[n][0];
                    const size_t ny = yy + NeighbourOffset_TileGenerator[n][1];

                    size_t nindex;
                    if (spanGrid.GetSpanAt(nx, ny, ytop, climbableVoxelCount, nindex))
                    {
                        uint16 label = labels[nindex];

                        if (!IsBorderLabel(label))
                        {
                            uint16 ndata = data[nindex];

                            if (data[nindex] == minimum)
                            {
                                const StreamElement probe(nx, ny, nindex);

                                if (std::find(stream.begin(), stream.end(), probe) == stream.end())
                                {
                                    zx = nx;
                                    zy = ny;
                                    zspan = nindex;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (zx == ~0ul)
                {
                    break;
                }

                if (IsLabelValid(labels[zspan]))
                {
                    return labels[zspan];
                }
                else
                {
                    if (data[zspan] > data[y.span])
                    {
                        unexplored.clear();
                        bfirst = false;
                    }

                    stream.push_back(StreamElement(zx, zy, zspan));
                    unexplored.push_back(StreamElement(zx, zy, zspan));
                }
            }
        }

        return TileGenerator::NoLabel;
    }

    void TileGenerator::PaintBorder(uint16* data, size_t borderH, size_t borderV)
    {
        const size_t width = m_spanGrid.GetWidth();
        const size_t height = m_spanGrid.GetHeight();

        if (borderH)
        {
            for (size_t y = 0; y < height; ++y)
            {
                for (size_t b = 0; b < 2; ++b)
                {
                    size_t xoffset = b * (width - borderH);

                    for (size_t x = 0; x < borderH; ++x)
                    {
                        if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x + xoffset, y))
                        {
                            const size_t index = cell.index;
                            const size_t count = cell.count;

                            for (size_t s = 0; s < count; ++s)
                            {
                                data[index + s] |= BorderLabelH;
                            }
                        }
                    }
                }
            }

            size_t hwidth = width - borderH;

            for (size_t b = 0; b < 2; ++b)
            {
                const size_t yoffset = b * (height - borderH);

                for (size_t y = 0; y < borderH; ++y)
                {
                    for (size_t x = borderH; x < hwidth; ++x)
                    {
                        if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x, y + yoffset))
                        {
                            const size_t index = cell.index;
                            const size_t count = cell.count;

                            for (size_t s = 0; s < count; ++s)
                            {
                                data[index + s] |= BorderLabelH;
                            }
                        }
                    }
                }
            }
        }

        if (borderV)
        {
            const size_t maxTop = borderV + m_top;

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    if (const CompactSpanGrid::Cell cell = m_spanGrid.GetCell(x, y))
                    {
                        const size_t index = cell.index;
                        const size_t count = cell.count;

                        for (size_t s = 0; s < count; ++s)
                        {
                            CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index + s);
                            const uint16 top = span.bottom + span.height;

                            if ((top < borderV) || (top >= maxTop))
                            {
                                data[index + s] |= BorderLabelV;
                            }
                        }
                    }
                }
            }
        }
    }

    real_t DistVertexToLineSq(int x, int y, int z, int ax, int ay, int az, int bx, int by, int bz)
    {
        const Vec3i diff(x - ax, y - ay, z - az);
        const Vec3i dir(bx - ax, by - ay, bz - az);
        const int t = diff.dot(dir);

        if (t > 0)
        {
            const int lenSq = dir.len2();

            if (t >= lenSq)
            {
                const real_t lenSq_t((diff - dir).len2());
                return lenSq_t >= 0 ? lenSq_t : real_t::max();
            }
            else
            {
                const vector3_t diff_t(diff);
                const vector3_t dir_t(dir);
                const real_t lenSq_t = (diff_t - (dir_t * real_t::fraction(t, lenSq))).lenSq();
                return lenSq_t >= 0 ? lenSq_t : real_t::max();
            }
        }

        const real_t lenSq_t(diff.len2());
        return lenSq_t >= 0 ? lenSq_t : real_t::max();
    }

    real_t DistVertexToLineSq(int x, int y, int ax, int ay, int bx, int by)
    {
        return DistVertexToLineSq(x, y, 0, ax, ay, 0, bx, by, 0);
    }

    const real_t AddContourVertexThreshold(real_t::fraction(15, 1000));

    void TileGenerator::AddContourVertex(const ContourVertex& contourVertex, Region& region, Contour& contour) const
    {
        if ((contour.size() < 2) || !ContourVertexRemovable(contour.back()))
        {
            contour.push_back(contourVertex);
        }
        else
        {
            ContourVertex& middle = contour.back();
            const ContourVertex& left = contour[contour.size() - 2];

            real_t distSq = DistVertexToLineSq(middle.x, middle.y, middle.z, left.x, left.y, left.z,
                    contourVertex.x, contourVertex.y, contourVertex.z);

            if (distSq <= AddContourVertexThreshold)
            {
                middle = contourVertex;
            }
            else
            {
                contour.push_back(contourVertex);
            }
        }

        if (contourVertex.flags & ContourVertex::TileBoundary)
        {
            region.flags |= Region::TileBoundary;
        }

        if (contourVertex.flags & ContourVertex::TileBoundaryV)
        {
            region.flags |= Region::TileBoundaryV;
        }
    }

    bool TileGenerator::GatherSurroundingInfo(const Vec2i& vertex, const Vec2i& direction, const uint16 top,
        const uint16 climbableVoxelCount, size_t& height, SurroundingSpanInfo& left, SurroundingSpanInfo& front,
        SurroundingSpanInfo& frontLeft) const
    {
        Vec2i external = vertex + direction;
        bool result = false;

        height = top;

        size_t index;
        if (m_spanGrid.GetSpanAt(external.x, external.y, top, climbableVoxelCount, index))
        {
            const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index);
            maximize<size_t>(height, span.bottom + span.height);

            front.label = m_labels[index];
            front.flags = span.flags;
            front.index = index;
            result = true;
        }

        external = vertex + direction + direction.rot90ccw();
        if (m_spanGrid.GetSpanAt(external.x, external.y, top, climbableVoxelCount, index))
        {
            const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index);
            maximize<size_t>(height, span.bottom + span.height);

            frontLeft.label = m_labels[index];
            frontLeft.flags = span.flags;
            frontLeft.index = index;

            result = true;
        }

        external = vertex + direction.rot90ccw();
        if (m_spanGrid.GetSpanAt(external.x, external.y, top, climbableVoxelCount, index))
        {
            const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index);
            maximize<size_t>(height, span.bottom + span.height);

            left.label = m_labels[index];
            left.flags = span.flags;
            left.index = index;
            result = true;
        }

        return result;
    }

    void TileGenerator::DetermineContourVertex(const Vec2i& vertex, const Vec2i& direction, const uint16 top,
        const uint16 climbableVoxelCount, ContourVertex& contourVertex) const
    {
        size_t xoffs = (direction.x == 1) | (direction.y == -1);
        size_t yoffs = (direction.y == 1) | (direction.x == 1);

        size_t cx = vertex.x + xoffs;
        size_t cy = vertex.y + yoffs;
        size_t cz = top;

        bool internalBorderV = false;
        {
            size_t index;
            if (m_spanGrid.GetSpanAt(vertex.x, vertex.y, top, climbableVoxelCount, index))
            {
                internalBorderV = (m_labels[index] & BorderLabelV) != 0;
            }
        }

        assert((top >= BorderSizeV()) || internalBorderV);

        SurroundingSpanInfo front(NoLabel, ~0ul, NotWalkable);
        SurroundingSpanInfo frontLeft(NoLabel, ~0ul, NotWalkable);
        SurroundingSpanInfo left(NoLabel, ~0ul, NotWalkable);

        const size_t borderV = BorderSizeV();
        GatherSurroundingInfo(vertex, direction, top, climbableVoxelCount, cz, left, front, frontLeft);
        minimize(cz, borderV + m_top);

        size_t flags = 0;

        const size_t erosion = m_params.flags & Params::NoErosion ? 0 : m_params.agent.radius * 2;

        size_t walkableBit = 0;
        walkableBit = (((frontLeft.flags & NotWalkable) == 0) && (m_distances[frontLeft.index] >= erosion)) ? 1 : 0;
        walkableBit |= (size_t)((((left.flags & NotWalkable) == 0) && (m_distances[left.index] >= erosion)) ? 1 : 0) << 1;
        walkableBit |= (size_t)((((front.flags & NotWalkable) == 0) && (m_distances[front.index] >= erosion)) ? 1 : 0) << 2;

        size_t borderBitH = 0;
        borderBitH = (frontLeft.label & BorderLabelH) ? 1 : 0;
        borderBitH |= (size_t)((left.label & BorderLabelH) ? 1 : 0) << 1;
        borderBitH |= (size_t)((front.label & BorderLabelH) ? 1 : 0) << 2;

        size_t borderBitV = 0;
        borderBitV = (frontLeft.label & BorderLabelV) ? 1 : 0;
        borderBitV |= (size_t)((left.label & BorderLabelV) ? 1 : 0) << 1;
        borderBitV |= (size_t)((front.label & BorderLabelV) ? 1 : 0) << 2;

        const size_t borderBit = borderBitH | borderBitV;

        NeighbourClassification lclass = ClassifyNeighbour(left, erosion, BorderLabelV | BorderLabelH);
        NeighbourClassification flclass = ClassifyNeighbour(frontLeft, erosion, BorderLabelV | BorderLabelH);
        NeighbourClassification fclass = ClassifyNeighbour(front, erosion, BorderLabelV | BorderLabelH);

        // horizontal border
        {
            if (IsCornerVertex(cx, cy))
            {
                flags |= ContourVertex::TileBoundary;
            }
            else
            {
                const bool boundary = IsBoundaryVertex(cx, cy);

                if (boundary)
                {
                    const bool frontBoundary = IsBoundaryCell(vertex.x + direction.x, vertex.y + direction.y)
                        || IsBorderCell(vertex.x + direction.x, vertex.y + direction.y);
                    const bool connection = (borderBitH == 7) && walkableBit;

                    if (frontBoundary && connection)
                    {
                        flags |= ContourVertex::TileBoundary;
                    }

                    if (CornerTable[lclass][flclass][fclass] || ((borderBit == 7) && (borderBitH && borderBitV)))
                    {
                        flags |= ContourVertex::Unremovable;
                    }
                }
            }
        }

        // vertical border
        {
            if (borderBitV || internalBorderV)
            {
                flags |= ContourVertex::TileBoundaryV;

                if (!internalBorderV)
                {
                    if (CornerTable[lclass][flclass][fclass] || ((borderBit == 7) && (borderBitH && borderBitV)))
                    {
                        flags |= ContourVertex::Unremovable;
                    }
                }
            }

            if (flags & ContourVertex::TileBoundaryV)
            {
                if (cz < borderV + m_top)
                {
                    cz = borderV;
                }
            }
        }

        contourVertex.x = cx;
        contourVertex.y = cy;
        contourVertex.z = cz;
        contourVertex.flags = flags;
    }

    void TileGenerator::AssessNeighbour(NeighbourInfo& info, size_t erosion, size_t climbableVoxelCount)
    {
        if (info.isValid = m_spanGrid.GetSpanAt(info.pos.x, info.pos.y, info.top, climbableVoxelCount, info.index))
        {
            const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(info.index);
            info.top = (span.bottom + span.height);
            info.label = m_labels[info.index];
            info.paint = m_paint[info.index];
        }
    }

    void TileGenerator::TraceContour(TileGenerator::TracerPath& path, const Tracer& start, size_t erosion, size_t climbableVoxelCount, const NeighbourInfoRequirements& contourReq)
    {
        Tracer tracer = start;
        path.steps.clear();
        path.steps.reserve(2048);
        path.turns = 0;

        do
        {
            tracer.bPinchPoint = false;

            // Check if the tracer path can turn left(next:front-left), carry on(next:front) else we rotate.
            NeighbourInfo left(tracer.GetLeft());
            NeighbourInfo frontLeft(tracer.GetFrontLeft());
            NeighbourInfo front(tracer.GetFront());

            AssessNeighbour(left, erosion, climbableVoxelCount);
            AssessNeighbour(frontLeft, erosion, climbableVoxelCount);
            AssessNeighbour(front, erosion, climbableVoxelCount);

            if (left.Check(contourReq))
            {
                // Shouldn't ever happen... Sidle over and close you eyes...
                CRY_ASSERT(false);
                tracer.SetPos(left);
                tracer.indexIn = left.index;
                tracer.indexOut = left.index;
            }
            else if (front.Check(contourReq))
            {
                if (frontLeft.Check(contourReq))
                {
                    // We can turn left!
                    tracer.SetPos(frontLeft);
                    tracer.TurnLeft();
                    tracer.indexIn = frontLeft.index;
                    tracer.indexOut = left.index;
                    path.turns--;
                }
                else
                {
                    // We can go forward!
                    tracer.SetPos(front);
                    tracer.indexIn = front.index;
                    tracer.indexOut = frontLeft.index;
                }
            }
            else
            {
                if (frontLeft.Check(contourReq))
                {
                    // We could have moved diagonally here...
                    // Mark the vert as pinch point.
                    tracer.bPinchPoint = true;
                }

                // Turn right.
                tracer.TurnRight();
                tracer.indexOut = front.index;
                path.turns++;
            }

            // Add tracer to the path.
            path.steps.push_back(tracer);
        } while (tracer != start);
    }

    int TileGenerator::LabelTracerPath(const TileGenerator::TracerPath& path, size_t climbableVoxelCount, Region& region, Contour& contour, const uint16 internalLabel, const uint16 internalLabelFlags, const uint16 externalLabel)
    {
        const int numSteps = path.steps.size();
        contour.reserve(numSteps);

        for (int i = numSteps - 1, j = 0; j < numSteps; i = j++)
        {
            const Tracer& curr = path.steps[i];
            const Tracer& next = path.steps[j];

            // Add a vertex at each point.
            ContourVertex vertex;
            DetermineContourVertex(Vec2i(curr.pos), curr.GetDir(), curr.pos.z, climbableVoxelCount, vertex);

            // Get the paint values for the current neighbour and the next neighbour. If they don't match then we must have a vert.
            const bool bImportantVert = (m_paint[curr.indexOut] != m_paint[next.indexOut]);
            if (bImportantVert)
            {
                vertex.flags |= ContourVertex::Unremovable;
            }
            if (next.bPinchPoint)
            {
                vertex.flags |= ContourVertex::Unremovable;
            }
            AddContourVertex(vertex, region, contour);

            // Apply the labels.
            if (internalLabel != NoLabel)
            {
                m_labels[curr.indexIn] = internalLabel;
            }
            m_labels[curr.indexIn] |= internalLabelFlags;

            if (externalLabel != NoLabel)
            {
                // Don't allow already established external contours to be stomped
                if ((m_labels[curr.indexOut] & ExternalContour) == 0)
                {
                    m_labels[curr.indexOut] = externalLabel;
                }
            }
        }

        TidyUpContourEnd(contour);
        return numSteps;
    }

    inline bool IsWalkable(uint16 label, uint16 distance,   size_t erosion)
    {
        if ((label & (TileGenerator::BorderLabelH | TileGenerator::BorderLabelV)) != 0)
        {
            return false;
        }

        if (distance < erosion)
        {
            return false;
        }

        return true;
    }

    inline bool IsWalkable(const Vec2i& voxel, size_t& index, uint16& label, size_t top, size_t climbableVoxelCount,
        size_t erosion, const CompactSpanGrid& spanGrid, const uint16* distances, const uint16* labels)
    {
        if (!spanGrid.GetSpanAt(voxel.x, voxel.y, top, climbableVoxelCount, index))
        {
            return false;
        }

        label = labels[index];
        if ((label & (TileGenerator::BorderLabelH | TileGenerator::BorderLabelV)) != 0)
        {
            return false;
        }

        if (distances[index] < erosion)
        {
            return false;
        }

        return true;
    }

    void TileGenerator::TidyUpContourEnd(Contour& contour)
    {
        if (contour.size() > 2)
        {
            const ContourVertex& left = contour[contour.size() - 2];
            const ContourVertex& middle = contour.back();
            const ContourVertex& right = contour.front();

            if ((DistVertexToLineSq(middle.x, middle.y, middle.z, left.x, left.y, left.z,
                     right.x, right.y, right.z) <= AddContourVertexThreshold) && ContourVertexRemovable(middle))
            {
                contour.pop_back();
            }
        }

        if (contour.size() > 2)
        {
            const ContourVertex& left = contour.back();
            ContourVertex& middle = contour.front();
            const ContourVertex& right = contour[1];

            if ((DistVertexToLineSq(middle.x, middle.y, middle.z, left.x, left.y, left.z,
                     right.x, right.y, right.z) <= AddContourVertexThreshold) && ContourVertexRemovable(middle))
            {
                middle = left;
                contour.pop_back();
            }
        }
    }

    uint16 TileGenerator::GetPaintVal(size_t x, size_t y, size_t z, size_t index, size_t borderH, size_t borderV, size_t erosion)
    {
        if (m_distances[index] < erosion)
        {
            return BadPaint;
        }

        if (m_labels[index] & (TileGenerator::BorderLabelH | TileGenerator::BorderLabelV))
        {
            return BadPaint;
        }

        return OkPaintStart;
    }

    void TileGenerator::CalcPaintValues()
    {
        // Adds the "paint" values to each voxel.
        // Different paint values represent different walkable regions that should NOT be merged.
        // We can add surface type filtering in here too.

        const size_t gridWidth = m_spanGrid.GetWidth();
        const size_t gridHeight = m_spanGrid.GetHeight();

        m_paint.resize(m_distances.size());
        std::fill(m_paint.begin(), m_paint.end(), NoPaint);

        const size_t borderH = BorderSizeH();
        const size_t borderV = BorderSizeV();
        const size_t erosion = m_params.flags & Params::NoErosion ? 0 : m_params.agent.radius << 1;
        const size_t climbableVoxelCount = m_params.agent.climbableHeight;

        for (size_t y = borderV; y < gridHeight - borderV; ++y)
        {
            for (size_t x = borderH; x < gridWidth - borderH; ++x)
            {
                const size_t spanGridIndex = (y * gridWidth) + x;
                const CompactSpanGrid::Cell& cell = m_spanGrid[spanGridIndex];
                for (size_t s = 0; s < cell.count; ++s)
                {
                    const size_t index = cell.index + s;
                    const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index);
                    m_paint[index] = GetPaintVal(x, y, span.bottom + span.height, index, borderH, borderV, erosion);
                }
            }
        }
    }

    size_t TileGenerator::ExtractContours()
    {
        m_profiler.StartTimer(ContourExtraction);

        const size_t gridWidth = m_spanGrid.GetWidth();
        const size_t gridHeight = m_spanGrid.GetHeight();

        m_labels.resize(m_distances.size());

        m_profiler.AddMemory(SegmentationMemory, m_labels.size() * sizeof(SpanExtraInfo::value_type));

        const size_t borderH = BorderSizeH();
        const size_t borderV = BorderSizeV();

        std::fill(m_labels.begin(), m_labels.end(), NoLabel);
        PaintBorder(&m_labels.front(), borderH, borderV);

        CalcPaintValues();

        size_t regionCount = 0;
        m_regions.reserve(128);

        TracerPath path;

        const size_t erosion = m_params.flags & Params::NoErosion ? 0 : m_params.agent.radius << 1;
        const size_t climbableVoxelCount = m_params.agent.climbableHeight;

        for (size_t y = borderV; y < gridHeight - borderV; ++y)
        {
            for (size_t x = borderH; x < gridWidth - borderH; ++x)
            {
                const size_t spanGridIndex = (y * gridWidth) + x;
                const CompactSpanGrid::Cell& cell = m_spanGrid[spanGridIndex];
                for (size_t s = 0; s < cell.count; ++s)
                {
                    const size_t index = cell.index + s;
                    const uint16 label = m_labels[index];
                    const uint16 labelsafe = label & NoLabel;

                    if (labelsafe != NoLabel)
                    {
                        continue;
                    }

                    const uint16 paint = m_paint[index];

                    const CompactSpanGrid::Span& span = m_spanGrid.GetSpan(index);
                    const size_t top = span.bottom + span.height;

                    NeighbourInfo prev(Vec2i(x - 1, y), top);
                    AssessNeighbour(prev, erosion, climbableVoxelCount);

                    const uint16 prevLabelSafe = (prev.label & NoLabel);

                    bool walkable = (paint >= OkPaintStart);
                    bool prevwalkable = (prev.paint >= OkPaintStart);
                    bool bothwalkable = (walkable && prevwalkable);
                    bool paintcontinuation = (paint == prev.paint);

                    Tracer startTracer;
                    startTracer.pos = Vec3i(x, y, top);
                    startTracer.dir = N;
                    startTracer.indexIn = index;
                    startTracer.indexOut = prev.index;

                    if (bothwalkable)
                    {
                        if (paintcontinuation)
                        {
                            if (prevLabelSafe < m_regions.size())
                            {
                                // Continuation of previous region. Walk label forward.
                                m_labels[index] = prevLabelSafe;
                                Region& region = m_regions[prevLabelSafe];
                                ++region.spanCount;
                            }
                        }
                        else
                        {
                            // Paint has changed colour. Trace the contour.
                            NeighbourInfoRequirements contourReq;
                            contourReq.paint = paint;

                            TraceContour(path, startTracer, erosion, climbableVoxelCount, contourReq);
                            if (path.turns > 0)
                            {
                                // Store for Debugging.
                                CacheTracerPath(path);

                                // Start of a new Region. Mark the inside with the new label.
                                const uint16 newLabel = regionCount;
                                m_regions.resize(++regionCount);

                                Region& region = m_regions.back();

                                region.spanCount += LabelTracerPath(path, climbableVoxelCount, region, region.contour, newLabel, ExternalContour, NoLabel);

                                // Also trace the hole contour for the previous painted colour.
                                if ((prev.label & ExternalContour) == 0 && (label & InternalContour) == 0)
                                {
                                    Region& holeRegion = m_regions[prevLabelSafe];
                                    holeRegion.holes.reserve(64);
                                    holeRegion.holes.resize(holeRegion.holes.size() + 1);

                                    NeighbourInfoRequirements holeReq;
                                    holeReq.notPaint = prev.paint;

                                    TraceContour(path, startTracer, erosion, climbableVoxelCount, holeReq);
                                    LabelTracerPath(path, climbableVoxelCount, holeRegion, holeRegion.holes.back(), NoLabel, InternalContour, prev.label);

                                    // Store for Debugging.
                                    CacheTracerPath(path);
                                }
                            }
                            else
                            {
                                // This is actually a hole we have discovered before we have entered the "containing" region.
                                // This is most likely when the hole inverts and exits the bounds of the region is within.
                                //CryLogAlways("Ignoring contour!");
                            }
                        }
                    }
                    else if (walkable)
                    {
                        // Trace the new region
                        NeighbourInfoRequirements contourReq;
                        contourReq.paint = paint;
                        TraceContour(path, startTracer, erosion, climbableVoxelCount, contourReq);

                        if (path.turns > 0)
                        {
                            // Store for Debugging.
                            CacheTracerPath(path);

                            // Just entered a new painted region. Marking the inside with the new label.
                            const uint16 newLabel = regionCount;
                            m_regions.resize(++regionCount);

                            Region& region = m_regions.back();

                            region.spanCount += LabelTracerPath(path, climbableVoxelCount, region, region.contour, newLabel, ExternalContour, NoLabel);
                        }
                        else
                        {
                            // This is actually a hole we have discovered before we have entered the "containing" region.
                            // This is most likely when the hole inverts and exits the bounds of the region is within.
                            //CryLogAlways("Ignoring contour!");
                        }
                    }
                    else if (prevwalkable)
                    {
                        if ((prev.label & ExternalContour) == 0 && (label & InternalContour) == 0)
                        {
                            if (prevLabelSafe < m_regions.size())
                            {
                                // Just left a painted region for a hole. Trace the outer of the hole with the previous region label.
                                Region& holeRegion = m_regions[prevLabelSafe];
                                holeRegion.holes.reserve(64);
                                holeRegion.holes.resize(holeRegion.holes.size() + 1);

                                NeighbourInfoRequirements holeReq;
                                holeReq.notPaint = prev.paint;

                                TraceContour(path, startTracer, erosion, climbableVoxelCount, holeReq);
                                LabelTracerPath(path, climbableVoxelCount, holeRegion, holeRegion.holes.back(), NoLabel, InternalContour, prev.label);

                                // Store for Debugging.
                                CacheTracerPath(path);
                            }
                            else
                            {
                                //CryLogAlways("Ending unknown section.");
                            }
                        }
                    }
                }
            }
        }

        m_profiler.StopTimer(ContourExtraction);

        m_profiler.AddMemory(RegionMemory, m_regions.capacity() * sizeof(Region));

        for (size_t i = 0; i < m_regions.size(); ++i)
        {
            const Region& region = m_regions[i];

            size_t memory = region.contour.capacity() * sizeof(ContourVertex);
            memory += region.holes.capacity() * sizeof(Contour);

            for (size_t h = 0; h < region.holes.size(); ++h)
            {
                memory += region.holes[h].capacity() * sizeof(ContourVertex);
            }

            m_profiler.AddMemory(RegionMemory, memory);
        }

        if ((m_params.flags & Params::DebugInfo) == 0)
        {
            SpanExtraInfo().swap(m_labels);

            m_profiler.FreeMemory(SegmentationMemory, m_labels.capacity() * sizeof(SpanExtraInfo::value_type));

            SpanExtraInfo().swap(m_distances);

            m_profiler.FreeMemory(SegmentationMemory, m_distances.capacity() * sizeof(SpanExtraInfo::value_type));
        }

        return regionCount;
    }

    size_t TileGenerator::InsertUniqueVertex(VertexIndexLookUp& lookUp, size_t x, size_t y, size_t z)
    {
        enum
        {
            zmask = (1 << 11) - 1,
        };
        enum
        {
            xmask = (1 << 10) - 1,
        };
        enum
        {
            ymask = (1 << 10) - 1,
        };

        const uint32 vertexID = ((uint32)(z & zmask) << 20) | ((uint32)(x & xmask) << 10) | (y & ymask);

        bool inserted;
        uint16& index = *lookUp.insert(vertexID, 0, &inserted);

        if (inserted)
        {
            index = m_vertices.size();

            m_vertices.push_back(Tile::Vertex(
                    Tile::Vertex::value_type(x * m_params.voxelSize.x),
                    Tile::Vertex::value_type(y * m_params.voxelSize.y),
                    Tile::Vertex::value_type(z * m_params.voxelSize.z)));
        }

        return index;
    }

    void TileGenerator::FilterBadRegions(size_t minSpanCount)
    {
        for (size_t i = 0; i < m_regions.size(); ++i)
        {
            Region& region = m_regions[i];

            if (((region.flags & Region::TileBoundary) == 0)
                && ((region.flags & Region::TileBoundaryV) == 0)
                && (region.spanCount && (region.spanCount <= minSpanCount)))
            {
                Region().swap(region);
            }
        }
    }

    bool TileGenerator::SimplifyContour(const Contour& contour, const real_t& tolerance2DSq, const real_t& tolerance3DSq, PolygonContour& poly)
    {
        const size_t MaxSimplifiedCount = 2048;
        uint16 simplified[MaxSimplifiedCount];
        size_t simplifiedCount = 0;
        const size_t contourSize = contour.size();
        bool boundary = (contour.back().flags & ContourVertex::TileBoundaryV) != 0;

        for (size_t i = 0; i < contourSize; ++i)
        {
            const ContourVertex& v = contour[i];

            if (v.flags & ContourVertex::TileBoundaryV)
            {
                if (!boundary || !ContourVertexRemovable(v))
                {
                    simplified[simplifiedCount++] = i;
                }

                boundary = true;
            }
            else
            {
                if (boundary && i && (simplified[simplifiedCount - 1] != (i - 1)))
                {
                    simplified[simplifiedCount++] = i - 1;
                }

                if (!ContourVertexRemovable(v))
                {
                    simplified[simplifiedCount++] = i;
                }

                boundary = false;
            }
        }

        if (simplifiedCount && (simplified[simplifiedCount - 1] != (contourSize - 1)))
        {
            if ((contour.back().flags & ContourVertex::TileBoundaryV)
                && ((contour.front().flags & ContourVertex::TileBoundaryV) == 0))
            {
                simplified[simplifiedCount++] = (contourSize - 1);
            }
        }

        if (!simplifiedCount)
        {
            Vec3i min(std::numeric_limits<int>::max());
            size_t minVertex = 0;

            Vec3i max(std::numeric_limits<int>::min());
            size_t maxVertex = 0;

            for (size_t i = 0; i < contourSize; ++i)
            {
                const ContourVertex& v = contour[i];

                if ((v.x < min.x) || ((v.x == min.x) && (v.y > min.y)))
                {
                    minVertex = i;
                    min = Vec3i(v.x, v.y, v.z);
                }

                if ((v.x > max.x) || ((v.x == max.x) && (v.y > min.y)))
                {
                    maxVertex = i;
                    max = Vec3i(v.x, v.y, v.z);
                }
            }

            simplified[simplifiedCount++] = minVertex;
            simplified[simplifiedCount++] = maxVertex;
        }

        for (size_t s0 = 0; s0 < simplifiedCount; )
        {
            const size_t s1 = (s0 + 1) % simplifiedCount;
            const size_t i0 = simplified[s0];
            const size_t i1 = simplified[s1];
            const size_t last = (i0 < i1) ? i1 : contourSize + i1;

            const ContourVertex& v0 = contour[i0];
            const ContourVertex& v1 = contour[i1];

            real_t dmax2DSq = real_t::min();
            real_t dmax3DSq = real_t::min();
            size_t index2D = 0;
            size_t index3D = 0;

            if (v0 < v1)
            {
                for (size_t v = i0 + 1; v < last; ++v)
                {
                    const size_t vi = v % contourSize;
                    const ContourVertex& vp = contour[vi];
                    const real_t d3DSq = DistVertexToLineSq(vp.x, vp.y, vp.z, v0.x, v0.y, v0.z, v1.x, v1.y, v1.z);
                    const real_t d2DSq = DistVertexToLineSq(vp.x, vp.y, v0.x, v0.y, v1.x, v1.y);

                    if (d2DSq > dmax2DSq)
                    {
                        index2D = vi;
                        dmax2DSq = d2DSq;
                    }

                    if (d3DSq > dmax3DSq)
                    {
                        index3D = vi;
                        dmax3DSq = d3DSq;
                    }
                }
            }
            else
            {
                for (size_t v = last - 1; v > i0; --v)
                {
                    const size_t vi = v % contourSize;
                    const ContourVertex& vp = contour[vi];
                    const real_t d3DSq = DistVertexToLineSq(vp.x, vp.y, vp.z, v1.x, v1.y, v1.z, v0.x, v0.y, v0.z);
                    const real_t d2DSq = DistVertexToLineSq(vp.x, vp.y, v1.x, v1.y, v0.x, v0.y);

                    if (d2DSq > dmax2DSq)
                    {
                        index2D = vi;
                        dmax2DSq = d2DSq;
                    }

                    if (d3DSq > dmax3DSq)
                    {
                        index3D = vi;
                        dmax3DSq = d3DSq;
                    }
                }
            }

            size_t index = (dmax3DSq >= tolerance3DSq) ? index3D : ((dmax2DSq >= tolerance2DSq) ? index2D : ~0ul);

            if (index != ~0ul)
            {
                assert(simplifiedCount + 1 < MaxSimplifiedCount);
                if (simplifiedCount + 1 == MaxSimplifiedCount)
                {
                    break;
                }

                for (size_t k = 0, i = s0 + 1; i < simplifiedCount; ++i, ++k)
                {
                    simplified[simplifiedCount - k] = simplified[simplifiedCount - k - 1];
                }

                simplified[s0 + 1] = index;
                ++simplifiedCount;

                continue;
            }

            ++s0;
        }

        // Remove degenerate verts for [a,b,c] where: a==c or a==b.
        for (size_t a = (simplifiedCount - 2), b = (simplifiedCount - 1), c = 0; simplifiedCount > 2 && c < simplifiedCount; )
        {
            const uint16 as = simplified[a];
            const uint16 bs = simplified[b];
            const uint16 cs = simplified[c];

            const ContourVertex& av = contour[as];
            const ContourVertex& bv = contour[bs];
            const ContourVertex& cv = contour[cs];

            if (as == bs || ((av.x == bv.x) && (av.y == bv.y) && (av.z == bv.z)))
            {
                // Remove b
                if ((b + 1) < simplifiedCount)
                {
                    const size_t numShift = simplifiedCount - (b + 1);
                    memmove(&simplified[b], &simplified[b + 1], sizeof(simplified[0]) * numShift);
                }
                simplifiedCount -= 1;
                b = ((a + 1) % simplifiedCount);
                c = ((a + 2) % simplifiedCount);
            }
            else if (as == cs || ((av.x == cv.x) && (av.y == cv.y) && (av.z == cv.z)))
            {
                // Remove b and c
                if ((b + 2) < simplifiedCount)
                {
                    const size_t numShift = simplifiedCount - (b + 2);
                    memmove(&simplified[b], &simplified[b + 2], sizeof(simplified[0]) * numShift);
                }
                simplifiedCount -= 2;
                b = ((a + 1) % simplifiedCount);
                c = ((a + 2) % simplifiedCount);
            }
            else
            {
                a = b;
                b = c++;
            }
        }

        if (simplifiedCount > 2)
        {
            PolygonContour spoly;
            spoly.reserve(simplifiedCount);

            for (size_t i = 0; i < simplifiedCount; ++i)
            {
                const ContourVertex& cv = contour[simplified[i]];
                spoly.push_back(PolygonVertex(cv.x, cv.y, cv.z));
            }

            poly.swap(spoly);

            return true;
        }

        return false;
    }

    void TileGenerator::SimplifyContours()
    {
        m_profiler.StartTimer(Simplification);

        const real_t tolerance2DSq(7);
        const real_t tolerance3DSq(11);

        size_t polygonCount = 0;

        for (size_t r = 0; r < m_regions.size(); ++r)
        {
            const Region& region = m_regions[r];
            polygonCount += (region.spanCount > 0);
        }

        m_polygons.reserve(polygonCount);

        for (size_t r = 0; r < m_regions.size(); ++r)
        {
            Region& region = m_regions[r];
            const Contour& contour = region.contour;

            if (contour.size() < 3)
            {
                continue;
            }

            m_polygons.resize(m_polygons.size() + 1);
            Polygon& poly = m_polygons.back();

            if (!SimplifyContour(region.contour, tolerance2DSq, tolerance3DSq, poly.contour))
            {
                m_polygons.pop_back();
                continue;
            }

            if (region.holes.empty())
            {
                continue;
            }

            poly.holes.reserve(region.holes.size());

            for (size_t h = 0; h < region.holes.size(); ++h)
            {
                poly.holes.resize(poly.holes.size() + 1);
                Hole& hole = poly.holes.back();

                if (!SimplifyContour(region.holes[h], tolerance2DSq, tolerance3DSq, hole.verts))
                {
                    poly.holes.pop_back();
                }
                else
                {
                    const size_t holeSize = hole.verts.size();
                    Vec2i avg(ZERO);
                    for (size_t hvi = 0; hvi < holeSize; ++hvi)
                    {
                        avg += Vec2i(hole.verts[hvi].x, hole.verts[hvi].y);
                    }
                    hole.center.x = avg.x / holeSize;
                    hole.center.y = avg.y / holeSize;
                    hole.rad = 0;
                    for (size_t hvi = 0; hvi < holeSize; ++hvi)
                    {
                        const Vec2i hv(hole.verts[hvi].x, hole.verts[hvi].y);
                        const int vertDist = int_ceil(sqrt_tpl((float)((hv - hole.center).GetLength2())));
                        hole.rad = max(hole.rad, vertDist);
                    }
                }
            }
        }

        m_profiler.StopTimer(Simplification);

        m_profiler.AddMemory(PolygonMemory, m_polygons.capacity() * sizeof(Polygon));

        for (size_t i = 0; i < m_polygons.size(); ++i)
        {
            m_profiler.AddMemory(PolygonMemory, m_polygons[i].contour.capacity() * sizeof(PolygonContour::value_type));
            m_profiler.AddMemory(PolygonMemory, m_polygons[i].holes.capacity() * sizeof(PolygonHoles::value_type));

            for (size_t k = 0; k < m_polygons[i].holes.size(); ++k)
            {
                m_profiler.AddMemory(PolygonMemory, m_polygons[i].holes[k].verts.capacity() * sizeof(PolygonContour::value_type));
            }
        }

        if ((m_params.flags & Params::DebugInfo) == 0)
        {
            Regions().swap(m_regions);

            m_profiler.FreeMemory(RegionMemory, m_profiler[RegionMemory].used);
        }

        m_profiler.AddStat(PolygonCount, m_polygons.size());
    }

    template<typename VertexTy>
    inline bool IsReflex(size_t vertex, const VertexTy* vertices, size_t vertexCount)
    {
        const uint16 i0 = vertex ? vertex - 1 : vertexCount - 1;
        const uint16 i1 = vertex;
        const uint16 i2 = (vertex + 1) % vertexCount;

        const VertexTy& v0 = vertices[i0];
        const VertexTy& v1 = vertices[i1];
        const VertexTy& v2 = vertices[i2];

        int area = ((int)v1.x - (int)v0.x) * ((int)v2.y - (int)v0.y) - ((int)v1.y - (int)v0.y) * ((int)v2.x - (int)v0.x);
        return area > 0;
    }

    template<typename VertexTy>
    inline bool IsReflex(size_t vertex, const VertexTy* vertices, uint16* indices, size_t indexCount)
    {
        const uint16 i0 = vertex ? vertex - 1 : indexCount - 1;
        const uint16 i1 = vertex;
        const uint16 i2 = (vertex + 1) % indexCount;

        const VertexTy& v0 = vertices[indices[i0]];
        const VertexTy& v1 = vertices[indices[i1]];
        const VertexTy& v2 = vertices[indices[i2]];

        int area = ((int)v1.x - (int)v0.x) * ((int)v2.y - (int)v0.y) - ((int)v1.y - (int)v0.y) * ((int)v2.x - (int)v0.x);
        return area > 0;
    }

    template<typename VertexTy>
    bool IsEar(size_t vertex, const VertexTy* vertices, size_t vertexCount, size_t agentHeight)
    {
        const uint16 i0 = vertex ? vertex - 1 : vertexCount - 1;
        const uint16 i1 = vertex;
        const uint16 i2 = (vertex + 1) % vertexCount;

        const VertexTy& v0 = vertices[i0];
        const VertexTy& v1 = vertices[i1];
        const VertexTy& v2 = vertices[i2];

        const size_t minZ = std::min<size_t>(std::min<size_t>(v0.z, v1.z), v2.z);
        const size_t maxZ = std::max<size_t>(std::max<size_t>(v0.z, v1.z), v2.z);

        // Calc edges
        const int x01 = ((int)v0.x - (int)v1.x);
        const int y01 = ((int)v0.y - (int)v1.y);
        const int x12 = ((int)v1.x - (int)v2.x);
        const int y12 = ((int)v1.y - (int)v2.y);
        const int x20 = ((int)v2.x - (int)v0.x);
        const int y20 = ((int)v2.y - (int)v0.y);

        // Check for any point existing within the test triangle
        for (size_t k = 0; k < vertexCount; ++k)
        {
            const VertexTy& cv = vertices[k];
            const Vec3i p(cv.x, cv.y, cv.z);

            if ((static_cast<size_t>(p.z) > maxZ + agentHeight) || (static_cast<size_t>(p.z) < minZ - 1))
            {
                continue;
            }

            bool e0 = ((((int)p.x - (int)v0.x) * y01 - ((int)p.y - (int)v0.y) * x01) < 0);
            bool e1 = ((((int)p.x - (int)v1.x) * y12 - ((int)p.y - (int)v1.y) * x12) < 0);
            bool e2 = ((((int)p.x - (int)v2.x) * y20 - ((int)p.y - (int)v2.y) * x20) < 0);

            // A point has been found inside the triangle so, therefore, not an ear.
            if (e0 && e1 && e2)
            {
                return false;
            }
        }

        return true;
    }

    template<typename VertexTy>
    bool IsEar(size_t vertex, const VertexTy* vertices, size_t vertexCount, uint16* indices, size_t indexCount,
        size_t agentHeight)
    {
        const uint16 i0 = vertex ? vertex - 1 : indexCount - 1;
        const uint16 i1 = vertex;
        const uint16 i2 = (vertex + 1) % indexCount;

        const VertexTy& v0 = vertices[indices[i0]];
        const VertexTy& v1 = vertices[indices[i1]];
        const VertexTy& v2 = vertices[indices[i2]];

        const size_t minZ = std::min<size_t>(std::min<size_t>(v0.z, v1.z), v2.z);
        const size_t maxZ = std::max<size_t>(std::max<size_t>(v0.z, v1.z), v2.z);

        // Calc edges
        const int x01 = ((int)v0.x - (int)v1.x);
        const int y01 = ((int)v0.y - (int)v1.y);
        const int x12 = ((int)v1.x - (int)v2.x);
        const int y12 = ((int)v1.y - (int)v2.y);
        const int x20 = ((int)v2.x - (int)v0.x);
        const int y20 = ((int)v2.y - (int)v0.y);

        // Check for any point existing within the test triangle
        for (size_t k = 0; k < vertexCount; ++k)
        {
            const VertexTy& cv = vertices[k];
            const Vec3i p(cv.x, cv.y, cv.z);

            if ((static_cast<size_t>(p.z) > maxZ + agentHeight) || (static_cast<size_t>(p.z) < minZ - 1))
            {
                continue;
            }

            bool e0 = ((((int)p.x - (int)v0.x) * y01 - ((int)p.y - (int)v0.y) * x01) < 0);
            bool e1 = ((((int)p.x - (int)v1.x) * y12 - ((int)p.y - (int)v1.y) * x12) < 0);
            bool e2 = ((((int)p.x - (int)v2.x) * y20 - ((int)p.y - (int)v2.y) * x20) < 0);

            // A point has been found inside the triangle so, therefore, not an ear.
            if (e0 && e1 && e2)
            {
                return false;
            }
        }

        return true;
    }

    size_t TileGenerator::Triangulate(PolygonContour& contour, const size_t agentHeight, const size_t borderH,
        const size_t borderV, VertexIndexLookUp& lookUp)
    {
        size_t triCount = 0;
        size_t vertexCount = contour.size();

        const size_t MaxIndices = 4096;
        uint16 indices[MaxIndices];

        for (size_t i = 0; i < vertexCount; ++i)
        {
            indices[i] = i;
        }

        while (true)
        {
            size_t bestInsideCount = ~0ul;
            size_t bestDiagonalIdx = ~0ul;
            size_t bestDiagonalSq = ~0ul;

            size_t bestDelaugnayIdx = ~0ul;
            size_t bestDelaugnaySq = ~0ul;

            size_t ii = vertexCount - 1;

            for (size_t i = 0; i < vertexCount; ++i)
            {
                uint16 ii0 = ii;
                uint16 i0 = indices[ii];
                uint16 i1 = indices[i];
                ii = i;

                const PolygonVertex& v1 = contour[i1];

                if ((v1.flags & PolygonVertex::Ear) == 0)
                {
                    continue;
                }

                uint16 i2 = indices[(i + 1) % vertexCount];

                const PolygonVertex& v0 = contour[i0];
                const PolygonVertex& v2 = contour[i2];

                const size_t minZ = std::min<size_t>(std::min<size_t>(v0.z, v1.z), v2.z);
                const size_t maxZ = std::max<size_t>(std::max<size_t>(v0.z, v1.z), v2.z);

                const int x13 = v0.x - v1.x;
                const int y13 = v0.y - v1.y;
                const int x23 = v2.x - v1.x;
                const int y23 = v2.y - v1.y;

                const int a = (x13 * x23 + y13 * y23);
                const int b = (y13 * x23 - x13 * y23);

                size_t insideCircumcircleCount = 0;

                size_t k = 0;
                for (; k < vertexCount - 3; ++k)
                {
                    const PolygonVertex& cv = contour[indices[(ii0 + 3 + k) % vertexCount]];
                    const Vec3i p(cv.x, cv.y, cv.z);

                    if ((static_cast<size_t>(p.z) > maxZ + agentHeight) || (static_cast<size_t>(p.z) + agentHeight < minZ))
                    {
                        continue;
                    }

                    const int x1p = v0.x - p.x;
                    const int y1p = v0.y - p.y;
                    const int x2p = v2.x - p.x;
                    const int y2p = v2.y - p.y;

                    if ((a * (x2p * y1p - x1p * y2p)) <= (b * (x2p * x1p + y1p * y2p)))
                    {
                        ++insideCircumcircleCount;
                    }
                }

                if (k == (vertexCount - 3))
                {
                    size_t diagonalSq = sqr(v2.x - v0.x) + sqr(v2.y - v0.y) + sqr(5 * (v2.z - v0.z));

                    if (insideCircumcircleCount < bestInsideCount)
                    {
                        bestInsideCount = insideCircumcircleCount;
                        bestDiagonalSq = diagonalSq;
                        bestDiagonalIdx = i;
                    }
                    else if (insideCircumcircleCount == bestInsideCount)
                    {
                        if (diagonalSq < bestDiagonalSq)
                        {
                            bestDiagonalSq = diagonalSq;
                            bestDiagonalIdx = i;
                        }
                    }

                    if (insideCircumcircleCount == 0)
                    {
                        if (diagonalSq < bestDelaugnaySq)
                        {
                            bestDelaugnaySq = diagonalSq;
                            bestDelaugnayIdx = i;
                        }
                    }
                }
            }

            size_t bestIdx = bestDelaugnayIdx;

            if (bestIdx == ~0ul)
            {
                bestIdx = bestDiagonalIdx;

                if (bestIdx == ~0ul)
                {
                    if (vertexCount != 3)
                    {
                        break;
                    }

                    bestIdx = 1;
                }
            }

            {
                const size_t a = bestIdx ? (bestIdx - 1) : (vertexCount - 1);
                const size_t b = bestIdx;
                const size_t c = (bestIdx + 1) % vertexCount;

                assert(a < MaxIndices);
                assert(b < MaxIndices);
                assert(c < MaxIndices);

                const uint16 i0 = indices[a];
                const uint16 i1 = indices[b];
                const uint16 i2 = indices[c];

                PolygonVertex& v0 = contour[i0];
                PolygonVertex& v1 = contour[i1];
                PolygonVertex& v2 = contour[i2];

                uint16 v0i = InsertUniqueVertex(lookUp, v0.x - borderH, v0.y - borderH, v0.z - borderV);
                uint16 v2i = InsertUniqueVertex(lookUp, v1.x - borderH, v1.y - borderH, v1.z - borderV);
                uint16 v1i = InsertUniqueVertex(lookUp, v2.x - borderH, v2.y - borderH, v2.z - borderV);

                --vertexCount;

                if ((v0i == v1i) | (v0i == v2i) | (v1i == v2i))
                {
                    if (vertexCount < 3)
                    {
                        break;
                    }

                    continue;
                }

                m_triangles.resize(m_triangles.size() + 1);
                Tile::Triangle& triangle = m_triangles.back();

                triangle.linkCount = 0;
                triangle.firstLink = 0;

                triangle.vertex[0] = v0i;
                triangle.vertex[1] = v1i;
                triangle.vertex[2] = v2i;

                if (vertexCount < 3)
                {
                    break;
                }

                for (size_t k = bestIdx; k < vertexCount; ++k)
                {
                    indices[k] = indices[k + 1];
                }

                const uint16 updi0 = bestIdx ? (bestIdx - 1) : (vertexCount - 1);
                const uint16 updi2 = bestIdx % vertexCount;

                contour[indices[updi0]].flags &= ~(PolygonVertex::Reflex | PolygonVertex::Ear);
                if (IsReflex(updi0, &contour[0], indices, vertexCount))
                {
                    contour[indices[updi0]].flags |= PolygonVertex::Reflex;
                }
                else if (IsEar(updi0, &contour[0], contour.size(), indices, vertexCount, agentHeight))
                {
                    contour[indices[updi0]].flags |= PolygonVertex::Ear;
                }

                contour[indices[updi2]].flags &= ~(PolygonVertex::Reflex | PolygonVertex::Ear);
                if (IsReflex(updi2, &contour[0], indices, vertexCount))
                {
                    contour[indices[updi2]].flags |= PolygonVertex::Reflex;
                }
                else if (IsEar(updi2, &contour[0], contour.size(), indices, vertexCount, agentHeight))
                {
                    contour[indices[updi2]].flags |= PolygonVertex::Ear;
                }

                ++triCount;
            }
        }

        return triCount;
    }

    bool Intersects(const Vec2i& a0, const Vec2i& a1, const Vec2i& b0, const Vec2i& b1)
    {
        const Vec2i e(b0 - a0);
        const Vec2i da(a1 - a0);
        const Vec2i db(b1 - b0);
        const int det = da.x * db.y - da.y * db.x;
        const int signAdjustment = det >= 0 ? 1 : -1;
        const int minAllowedValue = 0;
        const int maxAllowedValue = 1 * det * signAdjustment;
        if (det != 0)
        {
            int s = (e.x * db.y - e.y * db.x) * signAdjustment;

            if ((s < minAllowedValue) || (s > maxAllowedValue))
            {
                return false;
            }

            int t = (e.x * da.y - e.y * da.x) * signAdjustment;
            if ((t < minAllowedValue) || (t > maxAllowedValue))
            {
                return false;
            }

            return true;
        }

        return false;
    }

    void TileGenerator::MergeHole(PolygonContour& contour, const size_t contourVertex, const PolygonContour& hole, const size_t holeVertex, const int distSqr) const
    {
        const size_t holeSize = hole.size();
        const size_t contourSize = contour.size();

        if (distSqr == 0)
        {
            // No joint edges.
            contour.reserve(contourSize + holeSize);

            size_t ip = contourVertex + 1;
            contour.insert(contour.begin() + ip, hole.rbegin() + (holeSize - holeVertex), hole.rend());
            ip += holeVertex;

            contour.insert(contour.begin() + ip, hole.rbegin(), hole.rbegin() + (holeSize - holeVertex));
            ip += holeSize - holeVertex;
        }
        else
        {
            // Insert extra joint verts
            contour.reserve(contourSize + holeSize + 2);

            size_t ip = contourVertex + 1;
            contour.insert(contour.begin() + ip, hole.rbegin() + (holeSize - (holeVertex + 1)), hole.rend());
            ip += (holeVertex + 1);

            contour.insert(contour.begin() + ip, hole.rbegin(), hole.rbegin() + (holeSize - holeVertex));
            ip += holeSize - holeVertex;

            contour.insert(contour.begin() + ip, contour[contourVertex]);
        }
    }

    size_t TileGenerator::Triangulate()
    {
        m_profiler.StartTimer(Triangulation);

        size_t totalIndexCount = 0;
        size_t vertexCount = 0;

        for (size_t i = 0; i < m_polygons.size(); ++i)
        {
            const PolygonContour& contour = m_polygons[i].contour;
            const PolygonHoles& holes = m_polygons[i].holes;

            totalIndexCount += (contour.size() - 2) * 3;
            vertexCount += contour.size();

            for (size_t h = 0; h < holes.size(); ++h)
            {
                totalIndexCount += holes[h].verts.size() * 3;
                vertexCount += holes[h].verts.size() + 2;
            }
        }

        VertexIndexLookUp lookUp;
        lookUp.reset(vertexCount, vertexCount);

        m_triangles.clear();
        m_triangles.reserve(totalIndexCount);

        m_vertices.clear();
        m_vertices.reserve(vertexCount);

        size_t triCount = 0;

        const size_t agentHeight = m_params.agent.height;
        const size_t borderH = BorderSizeH();
        const size_t borderV = BorderSizeV();

        for (size_t p = 0; p < m_polygons.size(); ++p)
        {
            Polygon& polygon = m_polygons[p];
            PolygonHoles& holes = polygon.holes;
            PolygonContour& contour = polygon.contour;

            size_t contourSize = contour.size();

            size_t finalVertexCount = contourSize;
            for (size_t h = 0; h < holes.size(); ++h)
            {
                finalVertexCount += holes[h].verts.size() + 2;
            }
            contour.reserve(finalVertexCount);

            for (size_t v = 0; v < contourSize; v++)
            {
                contour[v].flags &= ~PolygonVertex::Reflex;
                contour[v].flags |= IsReflex(v, &contour[0], contourSize) ? PolygonVertex::Reflex : 0;
            }

            while (!holes.empty())
            {
                size_t bestContourIdx = 0;
                size_t bestHoleIdx = 0;
                size_t bestHole = 0;
                const int kMaxDist = 10000;
                int bestDist = kMaxDist;

                const size_t contourNum = contour.size();
                const size_t numHoles = holes.size();
                for (size_t ci0 = contourNum - 2, ci1 = contourNum - 1, ci2 = 0; ci2 < contourNum; ci0 = ci1, ci1 = ci2++)
                {
                    const Vec3i cv0(contour[ci0].x, contour[ci0].y, contour[ci0].z);
                    const Vec3i cv1(contour[ci1].x, contour[ci1].y, contour[ci1].z);
                    const Vec3i cv2(contour[ci2].x, contour[ci2].y, contour[ci2].z);
                    const Vec2i cIn0(cv1.y - cv0.y, cv0.x - cv1.x);
                    const Vec2i cIn1(cv2.y - cv1.y, cv1.x - cv2.x);
                    const bool bCVReflex = (cIn0.Dot(Vec2(cv2 - cv1)) < 0);
                    const Vec2i cv1_2D(cv1.x, cv1.y);
                    for (size_t hi = 0; hi < numHoles; hi++)
                    {
                        const Hole& hole = holes[hi];
                        const int holeDistSqr = (hole.center - cv1_2D).GetLength2();
                        if (bestDist == kMaxDist || holeDistSqr < sqr(hole.rad + bestDist))
                        {
                            const size_t vertNum = hole.verts.size();
                            for (size_t vi0 = vertNum - 2, vi1 = vertNum - 1, vi2 = 0; vi2 < vertNum; vi0 = vi1, vi1 = vi2++)
                            {
                                const Vec3i hv1(hole.verts[vi1].x, hole.verts[vi1].y, hole.verts[vi1].z);
                                const Vec3i diff(hv1 - cv1);
                                const Vec2i diff_2D(diff);

                                const int holeVertDistSqr = diff.GetLengthSquared();

                                // If the vert is too far, ignore.
                                if (bestDist != kMaxDist && holeVertDistSqr >= sqr(bestDist))
                                {
                                    continue;
                                }

                                if (holeVertDistSqr > 0)
                                {
                                    // Check it is internal to the contour.
                                    const bool inC0 = cIn0.Dot(diff_2D) > 0;
                                    const bool inC1 = cIn1.Dot(diff_2D) > 0;
                                    if (bCVReflex)
                                    {
                                        if (!inC0 && !inC1)
                                        {
                                            continue;
                                        }
                                    }
                                    else if (!inC0 || !inC1)
                                    {
                                        continue;
                                    }

                                    const Vec3i hv0(hole.verts[vi0].x, hole.verts[vi0].y, hole.verts[vi0].z);
                                    const Vec3i hv2(hole.verts[vi2].x, hole.verts[vi2].y, hole.verts[vi2].z);
                                    const Vec2i hIn0(hv1.y - hv0.y, hv0.x - hv1.x);
                                    const Vec2i hIn1(hv2.y - hv1.y, hv1.x - hv2.x);
                                    const bool bHVReflex = (hIn0.Dot(Vec2(hv2 - hv1)) > 0);

                                    // Check cv1 is external to the hole.
                                    const bool outH0 = hIn0.Dot(diff_2D) > 0;
                                    const bool outH1 = hIn1.Dot(diff_2D) > 0;
                                    if (bCVReflex)
                                    {
                                        if (!outH0 && !outH1)
                                        {
                                            continue;
                                        }
                                    }
                                    else if (!outH0 || !outH1)
                                    {
                                        continue;
                                    }

                                    // Only connect to within agent height.
                                    if (abs(diff.z) > (int)agentHeight)
                                    {
                                        continue;
                                    }
                                }

                                // Store Result.
                                bestDist = int_ceil(sqrt_tpl((float)holeVertDistSqr));
                                bestHole = hi;
                                bestHoleIdx = vi1;
                                bestContourIdx = ci1;
                            }
                        }
                    }
                }

                const PolygonContour& holeVerts = holes[bestHole].verts;

                bool bValid = (bestDist != kMaxDist);
                if (bValid)
                {
                    const Vec2i besthv(holeVerts[bestHoleIdx].x, holeVerts[bestHoleIdx].y);
                    const Vec2i bestcv(contour[bestContourIdx].x, contour[bestContourIdx].y);
                    const int heightMin = min(holeVerts[bestHoleIdx].z, contour[bestContourIdx].z) - agentHeight;
                    const int heightRange = (max(holeVerts[bestHoleIdx].z, contour[bestContourIdx].z) - heightMin) + agentHeight;

                    // Check for intersection with the other contour edges.
                    for (size_t i = contourNum - 1, j = 0; j < contourNum; i = j++)
                    {
                        if (i == bestContourIdx || j == bestContourIdx)
                        {
                            continue;
                        }

                        const int z0 = contour[i].z - heightMin;
                        const int z1 = contour[j].z - heightMin;
                        if ((z0 > heightRange && z1 > heightRange) || (z0 < 0 && z1 < 0))
                        {
                            continue;
                        }

                        const Vec2i v0(contour[i].x, contour[i].y);
                        const Vec2i v1(contour[j].x, contour[j].y);
                        if (Intersects(besthv, bestcv, v0, v1))
                        {
                            bValid = false;
                            break;
                        }
                    }

                    if (bValid)
                    {
                        // Check for intersection with the other hole edges.
                        const size_t holeNum = holeVerts.size();
                        for (size_t i = holeNum - 1, j = 0; j < holeNum; i = j++)
                        {
                            if (i == bestHoleIdx || j == bestHoleIdx)
                            {
                                continue;
                            }

                            const int z0 = holeVerts[i].z - heightMin;
                            const int z1 = holeVerts[j].z - heightMin;
                            if ((z0 > heightRange && z1 > heightRange) || (z0 < 0 && z1 < 0))
                            {
                                continue;
                            }

                            const Vec2i v0(holeVerts[i].x, holeVerts[i].y);
                            const Vec2i v1(holeVerts[j].x, holeVerts[j].y);
                            if (Intersects(besthv, bestcv, v0, v1))
                            {
                                bValid = false;
                                break;
                            }
                        }
                    }
                }

                if (!bValid)
                {
                    const Vec3 center(m_params.origin + Vec3i(m_params.sizeX, m_params.sizeY, m_params.sizeZ));
                    AIWarning("[MNM] Hole Merge selected possibly invalid verts to connect at [%.2f, %.2f, %.2f]", center.x, center.y, center.z);
                }

                MergeHole(contour, bestContourIdx, holeVerts, bestHoleIdx, bestDist);

                std::swap(holes[bestHole], holes.back());
                holes.pop_back();

                contourSize = contour.size();

                for (size_t v = 0; v < contourSize; v++)
                {
                    contour[v].flags &= ~PolygonVertex::Reflex;
                    contour[v].flags |= IsReflex(v, &contour[0], contourSize) ? PolygonVertex::Reflex : 0;
                }
            }

            for (size_t v = 0; v < contourSize; v++)
            {
                PolygonVertex& vv = contour[v];
                if ((vv.flags & PolygonVertex::Reflex) == 0)
                {
                    vv.flags |= IsEar(v, &contour[0], contourSize, agentHeight) ? PolygonVertex::Ear : 0;
                }
            }

            triCount += Triangulate(polygon.contour, agentHeight, borderH, borderV, lookUp);
        }

        m_profiler.StopTimer(Triangulation);

        m_profiler.AddMemory(TriangulationMemory, lookUp.size() * sizeof(VertexIndexLookUp::value_type));
        m_profiler.FreeMemory(TriangulationMemory, lookUp.size() * sizeof(VertexIndexLookUp::value_type));

        m_profiler.AddMemory(TriangleMemory, m_triangles.capacity() * sizeof(Triangles::value_type));
        m_profiler.AddMemory(VertexMemory, m_vertices.capacity() * sizeof(Vertices::value_type));

        m_profiler.AddStat(VertexCount, m_vertices.size());
        m_profiler.AddStat(TriangleCount, m_triangles.size());

        return triCount;
    }

    struct BVTriangle
    {
        BVTriangle(uint16 _triangleID = 0, uint16 _binID = 0)
            : triangleID(_triangleID)
            , binID(_binID)
        {
        }

        uint16 triangleID;
        uint16 binID;

        vector3_t centroid;
        aabb_t aabb;


        bool operator<(const BVTriangle& other) const
        {
            return binID < other.binID;
        }
    };

    struct BVBin
    {
        BVBin()
            : area(0)
            , triCount(0)
            , leftArea(0)
            , leftTriCount(0)
            , aabb(aabb_t::init_empty())
        {
        }

        aabb_t aabb;

        real_t area;
        size_t triCount;

        real_t leftArea;
        size_t leftTriCount;
    };


    struct sort_triangle_by_dimension
    {
        sort_triangle_by_dimension(size_t _dim)
            : dim(_dim)
        {
        }

        bool operator()(const BVTriangle& left, const BVTriangle& right) const
        {
            if (left.centroid[dim] == right.centroid[dim])
            {
                return left.aabb.min[dim] < right.aabb.min[dim];
            }

            return left.centroid[dim] < right.centroid[dim];
        }

        size_t dim;
    };

    void CalculateVolume(const BVTriangle* triangles, size_t firstTri, size_t triCount, aabb_t& aabb)
    {
        aabb = aabb_t(aabb_t::init_empty());

        const size_t triEnd = firstTri + triCount;
        for (size_t i = firstTri; i < triEnd; ++i)
        {
            aabb += triangles[i].aabb;
        }
    }

    void SplitSAH(const aabb_t& aabb, BVTriangle* triangles, size_t firstTri, size_t triCount,
        Tile::BVNode* nodes, size_t& nodeCount)
    {
        Tile::BVNode& node = nodes[nodeCount++];
        node.aabb = aabb;

        assert(!aabb.empty());
        assert(!node.aabb.empty());

        if (triCount == 1)
        {
            node.leaf = 1;
            node.offset = triangles[firstTri].triangleID;

            return;
        }

        node.leaf = 0;

        aabb_t caabb = aabb_t::init_empty();

        for (size_t i = firstTri; i < firstTri + triCount; ++i)
        {
            caabb += triangles[i].centroid;
        }

        const vector3_t size(aabb.size());
        size_t dimSplit = 0;

        if (size.y >= size.z)
        {
            if (size.y > size.x)
            {
                dimSplit = 1;
            }
        }
        else if (size.z > size.x)
        {
            dimSplit = 2;
        }

        const real_t distCentroid = caabb.max[dimSplit] - caabb.min[dimSplit];

        const size_t MaxBinCount = 8;
        BVBin bins[MaxBinCount];

        const real_t BinEpsilon = real_t::fraction(1, 1000);
        const size_t BinCount = distCentroid > BinEpsilon ? 8 : 2;
        const real_t BinConv = (distCentroid > 0) ? (real_t(static_cast<unsigned int>(BinCount)) - BinEpsilon) / distCentroid : 0;

        for (size_t i = firstTri; i < firstTri + triCount; ++i)
        {
            BVTriangle& tri = triangles[i];

            const real_t centroid = tri.centroid[dimSplit] - caabb.min[dimSplit];
            const size_t binID = (BinConv * centroid).as_uint();
            assert(binID < BinCount);

            tri.binID = binID;

            BVBin& bin = bins[binID];
            ++bin.triCount;

            bin.aabb += tri.aabb;
        }

        // find split plane
        real_t leftArea(0);
        size_t leftTriCount = 0;

        {
            aabb_t laabb = aabb_t::init_empty();

            for (size_t i = 0; i < BinCount; ++i)
            {
                BVBin& bin = bins[i];

                if (bin.triCount)
                {
                    const vector3_t dims(bin.aabb.size());
                    const real_t area = (dims.x * dims.y + dims.x * dims.z + dims.z * dims.y) * 2;
                    bin.area = area;

                    leftArea += area;
                    leftTriCount += bin.triCount;

                    bin.leftArea = leftArea;
                    bin.leftTriCount = leftTriCount;

                    laabb += bin.aabb;
                }

                bin.aabb = laabb;
            }
        }

        real_t costLowest = leftArea * static_cast<unsigned int>(leftTriCount);
        size_t split = BinCount - 1;

        aabb_t raabbLowest = aabb_t::init_empty();
        aabb_t raabb = aabb_t::init_empty();

        real_t rightArea(0);
        size_t rightTriCount = 0;

        for (size_t i = 0; i < BinCount - 2; ++i)
        {
            const BVBin& bin = bins[BinCount - i - 1];
            if (!bin.triCount)
            {
                continue;
            }

            raabb += bin.aabb;

            const BVBin& left = bins[BinCount - i - 2];

            rightArea += bin.area;
            rightTriCount = bin.triCount;

            const real_t cost = rightArea * static_cast<unsigned int>(rightTriCount) + bin.leftArea * static_cast<unsigned int>(bin.leftTriCount);

            if (cost < costLowest)
            {
                costLowest = cost;
                split = BinCount - i - 2; // split at the right side of the bin

                raabbLowest += raabb;
            }
        }

        size_t currentNode = nodeCount - 1;

        if (split == BinCount - 1)
        {
            std::sort(triangles + firstTri, triangles + firstTri + triCount, sort_triangle_by_dimension(dimSplit));

            size_t splitTri = triCount >> 1;

            aabb_t laabb;
            CalculateVolume(triangles, firstTri, splitTri, laabb);
            SplitSAH(laabb, triangles, firstTri, splitTri, nodes, nodeCount);

            CalculateVolume(triangles, firstTri + splitTri, triCount - splitTri, raabb);
            SplitSAH(raabb, triangles, firstTri + splitTri, triCount - splitTri, nodes, nodeCount);
            node.offset = nodeCount - currentNode;
        }
        else
        {
            std::sort(triangles + firstTri, triangles + firstTri + triCount);

            size_t splitTri = std::distance(triangles + firstTri,
                    std::lower_bound(triangles + firstTri, triangles + firstTri + triCount, BVTriangle(0, split + 1)));
            assert(splitTri < triCount);

            SplitSAH(bins[split].aabb, triangles, firstTri, splitTri, nodes, nodeCount);
            SplitSAH(raabbLowest, triangles, firstTri + splitTri, triCount - splitTri, nodes, nodeCount);
            node.offset = nodeCount - currentNode;
        }
    }

    void TileGenerator::BuildBVTree()
    {
        m_profiler.StartTimer(BVTreeConstruction);

        const size_t triangleCount = m_triangles.size();

        if (!triangleCount)
        {
            BVTree().swap(m_bvtree);

            return;
        }

        aabb_t aabb = aabb_t::init_empty();

        std::vector<BVTriangle> bvTriangles;
        bvTriangles.resize(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            const Tile::Triangle& triangle = m_triangles[i];
            BVTriangle& bvTri = bvTriangles[i];

            vector3_t vertices[3];

            for (size_t v = 0; v < 3; ++v)
            {
                vertices[v] = vector3_t(m_vertices[triangle.vertex[v]]);
            }

            bvTri.aabb = aabb_t(
                    vector3_t::minimize(vertices[0], vertices[1], vertices[2]),
                    vector3_t::maximize(vertices[0], vertices[1], vertices[2]));
            bvTri.centroid = bvTri.aabb.center();
            bvTri.triangleID = i;

            aabb += bvTri.aabb;
        }

        assert(aabb.min.x >= 0);
        assert(aabb.min.y >= 0);
        assert(aabb.min.z >= 0);

        assert(aabb.max.x <= m_params.sizeX);
        assert(aabb.max.y <= m_params.sizeY);
        assert(aabb.max.z <= m_params.sizeZ);

        std::vector<Tile::BVNode> nodes(2 * triangleCount);
        size_t nodeCount = 0;

        SplitSAH(aabb, &bvTriangles.front(), 0, bvTriangles.size(), &nodes.front(), nodeCount);
        nodes.resize(nodeCount);

        m_profiler.StopTimer(BVTreeConstruction);

        m_profiler.AddMemory(BVTreeMemory, nodes.size() * sizeof(Tile::BVNode));
        m_profiler.AddStat(BVTreeNodeCount, nodes.size());

        m_profiler.AddMemory(BVTreeConstructionMemory, nodes.capacity() * sizeof(Tile::BVNode));
        m_profiler.FreeMemory(BVTreeConstructionMemory, nodes.capacity() * sizeof(Tile::BVNode));

        m_bvtree = nodes;
    }
}
