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
#include "AreaGrid.h"
#include "Area.h"
#include <IRenderAuxGeom.h>
#include "Components/IComponentAudio.h"

const uint32 CAreaGrid::GRID_CELL_SIZE = 4;
const float CAreaGrid::GRID_CELL_SIZE_R = 1.f / CAreaGrid::GRID_CELL_SIZE;

//////////////////////////////////////////////////////////////////////////
CAreaGrid::CAreaGrid()
{
    m_pEntitySystem = NULL;
    m_pbitFieldX = NULL;
    m_pbitFieldY = NULL;
    m_pAreaBounds = NULL;
    m_papAreas = NULL;
    m_bitFieldSizeU32 = 0;
    m_maxNumAreas = 0;
    m_nCells = 0;
}

CAreaGrid::~CAreaGrid()
{
    Reset();
}

TAreaPointers const& CAreaGrid::GetAreas(int x, int y)
{
    // Must be empty, don't clear here for performance reasons!
    assert(m_apAreasTmp.empty());
    assert((unsigned)x < m_nCells && (unsigned)y < m_nCells);

    const uint32* pBitsLHS = m_pbitFieldX + (m_bitFieldSizeU32 * x);
    const uint32* pBitsRHS = m_pbitFieldY + (m_bitFieldSizeU32 * y);

    for (uint32 i = 0, offset = 0; i < m_bitFieldSizeU32; ++i, offset += 32)
    {
        uint32 currentBitField = pBitsLHS[i] & pBitsRHS[i];

        for (uint32 j = 0; currentBitField != 0; ++j, currentBitField >>= 1)
        {
            if (currentBitField & 1)
            {
                m_apAreasTmp.push_back(m_papAreas->at(offset + j));
            }
        }
    }

    return m_apAreasTmp;
}


//////////////////////////////////////////////////////////////////////////
bool CAreaGrid::GetAreaIndex(CArea const* const pArea, size_t& nIndexOut)
{
    bool bSuccess = false;

    if (m_papAreas != NULL)
    {
        TAreaPointers::const_iterator Iter(m_papAreas->begin());
        TAreaPointers::const_iterator const IterEnd(m_papAreas->end());

        for (; Iter != IterEnd; ++Iter)
        {
            if ((*Iter) == pArea)
            {
                nIndexOut = std::distance(m_papAreas->begin(), Iter);
                assert(nIndexOut < m_papAreas->size());
                bSuccess = true;

                break;
            }
        }
    }

    return bSuccess;
}

void CAreaGrid::AddAreaBit(const Vec2i& start, const Vec2i& end, uint32 areaIndex)
{
    CRY_ASSERT(start.x >= 0 && start.y >= 0 && end.x < (signed)m_nCells && end.y < (signed)m_nCells);
    CRY_ASSERT(start.x <= end.x && start.y <= end.y);

    uint32* pBits;
    const uint32 bucketBit = 1 << (areaIndex & 31);
    const uint32 bucketIndex = areaIndex >> 5;

    // -- X ---
    pBits = m_pbitFieldX + (m_bitFieldSizeU32 * start.x) + bucketIndex;
    for (int i = start.x; i <= end.x; i++, pBits += m_bitFieldSizeU32)
    {
        *pBits |= bucketBit;
    }

    // -- Y ---
    pBits = m_pbitFieldY + (m_bitFieldSizeU32 * start.y) + bucketIndex;
    for (int i = start.y; i <= end.y; i++, pBits += m_bitFieldSizeU32)
    {
        *pBits |= bucketBit;
    }

    m_pAreaBounds[areaIndex][0] = start;
    m_pAreaBounds[areaIndex][1] = end;
}

void CAreaGrid::RemoveAreaBit(uint32 areaIndex)
{
    Vec2i& start = m_pAreaBounds[areaIndex][0];
    Vec2i& end = m_pAreaBounds[areaIndex][1];

    if (start == Vec2i(-1, -1) && end == Vec2i(-1, -1))
    {
        return; // Hasn't been added yet
    }

    CRY_ASSERT(start.x >= 0 && start.y >= 0 && end.x < (signed)m_nCells && end.y < (signed)m_nCells);
    CRY_ASSERT(start.x <= end.x && start.y <= end.y);

    uint32* pBits;
    const uint32 bucketBit = ~(1 << (areaIndex & 31));
    const uint32 bucketIndex = areaIndex >> 5;

    // -- X ---
    pBits = m_pbitFieldX + (m_bitFieldSizeU32 * start.x) + bucketIndex;
    for (int i = start.x; i <= end.x; i++, pBits += m_bitFieldSizeU32)
    {
        *pBits &= bucketBit;
    }

    // -- Y ---
    pBits = m_pbitFieldY + (m_bitFieldSizeU32 * start.y) + bucketIndex;
    for (int i = start.y; i <= end.y; i++, pBits += m_bitFieldSizeU32)
    {
        *pBits &= bucketBit;
    }

    start = Vec2i(-1, -1);
    end = Vec2i(-1, -1);
}

void CAreaGrid::ClearAllBits()
{
    memset(m_pbitFieldX, 0, m_bitFieldSizeU32 * m_nCells * sizeof(m_pbitFieldX[0]));
    memset(m_pbitFieldY, 0, m_bitFieldSizeU32 * m_nCells * sizeof(m_pbitFieldY[0]));
    memset(m_pAreaBounds, -1, sizeof(m_pAreaBounds[0]) * m_maxNumAreas);
}


bool CAreaGrid::ResetArea(CArea* pArea)
{
    if (m_papAreas == NULL)
    {
        return false;
    }

    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    uint32 index = 0;
    TAreaPointers::const_iterator it = m_papAreas->begin();
    TAreaPointers::const_iterator const itEnd = m_papAreas->end();
    const uint32 numAreas = min((uint32)m_maxNumAreas, (uint32)m_papAreas->size());

    for (; it != itEnd && index < numAreas; ++it, ++index)
    {
        if (*it == pArea)
        {
            RemoveAreaBit(index);
            AddArea(pArea, index);
            return true;
        }
    }
    return false;
}

void CAreaGrid::AddArea(CArea* pArea, uint32 areaIndex)
{
    // Calculate a loose bounding box (ie one that covers all the region we will have to check this
    // shape for, including fade area).
    Vec2 vBBCentre(0, 0);
    Vec2 vBBExtent(0, 0);
    switch (pArea ? pArea->GetAreaType() : EEntityAreaType(-1))
    {
    case ENTITY_AREA_TYPE_SPHERE:
    {
        Vec3 vSphereCentre(ZERO);
        float fSphereRadius(0.0f);
        if (pArea)
        {
            pArea->GetSphere(vSphereCentre, fSphereRadius);
        }
        vBBCentre = Vec2(vSphereCentre.x, vSphereCentre.y);
        float const fGreatestFadeDistance = GetGreatestFadeDistance(pArea);
        vBBExtent = Vec2(fSphereRadius + fGreatestFadeDistance, fSphereRadius + fGreatestFadeDistance);
    }
    break;

    case ENTITY_AREA_TYPE_SHAPE:
    {
        Vec2 vShapeMin, vShapeMax;
        pArea->GetBBox(vShapeMin, vShapeMax);
        vBBCentre = (vShapeMax + vShapeMin) * 0.5f;
        float const fGreatestFadeDistance = GetGreatestFadeDistance(pArea);
        vBBExtent = (vShapeMax - vShapeMin) * 0.5f + Vec2(fGreatestFadeDistance, fGreatestFadeDistance);
    }
    break;

    case ENTITY_AREA_TYPE_BOX:
    {
        Vec3 vBoxMin, vBoxMax;
        pArea->GetBox(vBoxMin, vBoxMax);
        Matrix34 tm;
        pArea->GetMatrix(tm);
        static const float sqrt2 = 1.42f;
        float const fGreatestFadeDistance = GetGreatestFadeDistance(pArea);
        const Vec3 vBoxMinWorld = tm.TransformPoint(vBoxMin);
        const Vec3 vBoxMaxWorld = tm.TransformPoint(vBoxMax);
        vBBExtent = Vec2(abs(vBoxMaxWorld.x - vBoxMinWorld.x), abs(vBoxMaxWorld.y - vBoxMinWorld.y)) * sqrt2 * 0.5f;
        vBBExtent += Vec2(fGreatestFadeDistance, fGreatestFadeDistance);
        vBBCentre = Vec2(vBoxMinWorld.x + vBoxMaxWorld.x, vBoxMinWorld.y + vBoxMaxWorld.y) * 0.5f;
    }
    break;

    case ENTITY_AREA_TYPE_SOLID:
    {
        AABB boundbox;
        pArea->GetSolidBoundBox(boundbox);
        Matrix34 tm;
        pArea->GetMatrix(tm);
        static const float sqrt2 = 1.42f;
        float const fGreatestFadeDistance = GetGreatestFadeDistance(pArea);
        const Vec3 vBoxMinWorld = tm.TransformPoint(boundbox.min);
        const Vec3 vBoxMaxWorld = tm.TransformPoint(boundbox.max);
        vBBExtent = Vec2(abs(vBoxMaxWorld.x - vBoxMinWorld.x), abs(vBoxMaxWorld.y - vBoxMinWorld.y)) * sqrt2 * 0.5f;
        vBBExtent += Vec2(fGreatestFadeDistance, fGreatestFadeDistance);
        vBBCentre = Vec2(vBoxMinWorld.x + vBoxMaxWorld.x, vBoxMinWorld.y + vBoxMaxWorld.y) * 0.5f;
    }
    break;
    }

    //Covert BB pos into grid coords
    Vec2i start((int)((vBBCentre.x - vBBExtent.x) * GRID_CELL_SIZE_R), (int)((vBBCentre.y - vBBExtent.y) * GRID_CELL_SIZE_R));
    Vec2i end((int)((vBBCentre.x + vBBExtent.x) * GRID_CELL_SIZE_R), (int)((vBBCentre.y + vBBExtent.y) * GRID_CELL_SIZE_R));

    if ((end.x | end.y) < 0 || start.x >= (signed)m_nCells || start.y > (signed)m_nCells)
    {
        return;
    }

    start.x = max(start.x, 0);
    start.y = max(start.y, 0);
    end.x = min(end.x, (int)m_nCells - 1);
    end.y = min(end.y, (int)m_nCells - 1);

    AddAreaBit(start, end, areaIndex);

#ifndef _RELEASE
    //query bb extents to see if they are correctly added to the grid
    //Debug_CheckBB(vBBCentre, vBBExtent, pArea);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Compile(CEntitySystem* pEntitySystem, TAreaPointers const& rAreas)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    uint32 nOldCells = m_nCells;
    uint32 terrainSize = gEnv->p3DEngine->GetTerrainSize();
    uint32 nCells = terrainSize / GRID_CELL_SIZE;
    nCells = max(nCells, (uint32)2048);

    // No point creating an area grid if there are no areas.
    if (rAreas.empty() || nCells == 0)
    {
        Reset();
        return;
    }

    uint32 nAreaCount = rAreas.size();
    uint32 bitFieldSizeU32 = ((nAreaCount + 31) >> 5);

    if (nCells != nOldCells || nAreaCount > m_maxNumAreas || m_bitFieldSizeU32 != bitFieldSizeU32)
    {
        // Full reset and reallocate bit-fields
        Reset();
        m_nCells = nCells;

        m_bitFieldSizeU32 = bitFieldSizeU32;
        m_maxNumAreas = nAreaCount;
        m_papAreas = &rAreas;

        assert(m_pbitFieldX == NULL && m_apAreasTmp.empty());

        m_pbitFieldX = new uint32[m_bitFieldSizeU32 * m_nCells];
        m_pbitFieldY = new uint32[m_bitFieldSizeU32 * m_nCells];

        m_apAreasTmp.reserve(m_maxNumAreas);

        m_pAreaBounds = new Vec2i[m_maxNumAreas][2];
    }

    ClearAllBits();

    m_pEntitySystem = pEntitySystem;

    // Loop through ALL areas
    const int count = rAreas.size();
    for (int i = 0; i < count; i++)
    {
        AddArea(rAreas[i], i);
    }
}

void CAreaGrid::Reset()
{
    SAFE_DELETE_ARRAY(m_pbitFieldX);
    SAFE_DELETE_ARRAY(m_pbitFieldY);
    SAFE_DELETE_ARRAY(m_pAreaBounds);
    stl::free_container(m_apAreasTmp);
    m_papAreas = NULL;
    m_bitFieldSizeU32 = 0;
    m_maxNumAreas = 0;
    m_nCells = 0;
}

TAreaPointers const& CAreaGrid::GetAreas(Vec3 const& position)
{
    // Clear this once before the call to GetAreas!
    ClearTmpAreas();

    if (m_pbitFieldX != NULL)
    {
        int gridX = (int)(position.x * GRID_CELL_SIZE_R);
        int gridY = (int)(position.y * GRID_CELL_SIZE_R);

        if ((unsigned)gridX < m_nCells && (unsigned)gridY < m_nCells)
        {
            return GetAreas(gridX, gridY);
        }
        else
        {
            //printf("Trying to index outside of grid\n");
        }
    }

    return m_apAreasTmp;
}

void CAreaGrid::Draw()
{
#ifndef _RELEASE
    if (GetNumAreas() == 0)
    {
        return;
    }

    // Clear this once before the call to GetAreas!
    ClearTmpAreas();

    I3DEngine* p3DEngine = gEnv->p3DEngine;
    IRenderAuxGeom* pRC = gEnv->pRenderer->GetIRenderAuxGeom();
    pRC->SetRenderFlags(e_Def3DPublicRenderflags);

    ColorF const colorsArray[] = {
        ColorF(1.0f, 0.0f, 0.0f, 1.0f),
        ColorF(0.0f, 1.0f, 0.0f, 1.0f),
        ColorF(0.0f, 0.0f, 1.0f, 1.0f),
        ColorF(1.0f, 1.0f, 0.0f, 1.0f),
        ColorF(1.0f, 0.0f, 1.0f, 1.0f),
        ColorF(0.0f, 1.0f, 1.0f, 1.0f),
        ColorF(1.0f, 1.0f, 1.0f, 1.0f),
    };

    for (uint32 gridX = 0; gridX < m_nCells; gridX++)
    {
        for (uint32 gridY = 0; gridY < m_nCells; gridY++)
        {
            TAreaPointers const& rAreas = GetAreas(gridX, gridY);

            if (!rAreas.empty())
            {
                Vec3 const vGridCentre = Vec3((float(gridX) + 0.5f) * GRID_CELL_SIZE, (float(gridY) + 0.5f) * GRID_CELL_SIZE, 0.0f);

                ColorF colour(0.0f, 0.0f, 0.0f, 0.0f);
                float divisor = 0.0f;
                TAreaPointers::const_iterator Iter(rAreas.begin());
                TAreaPointers::const_iterator const IterEnd(rAreas.end());

                for (; Iter != IterEnd; ++Iter)
                {
                    // Pick a random color from the array.
                    size_t nAreaIndex = 0;

                    if (GetAreaIndex(*Iter, nAreaIndex))
                    {
                        ColorF const areaColour = colorsArray[nAreaIndex % (sizeof(colorsArray) / sizeof(ColorF))];
                        colour += areaColour;
                        ++divisor;
                    }
                    else
                    {
                        // Areas must be known!
                        assert(0);

                        if (divisor == 0.0f)
                        {
                            divisor = 0.1f;
                        }
                    }
                }

                // Immediately clear the array to prevent it from re-entering this loop and messing up drawing of the grid.
                ClearTmpAreas();

                // "divisor" won't be 0!
                colour /= divisor;

                ColorB const colourB = colour;

                int const gridCellSize = GRID_CELL_SIZE;

                Vec3 points[] = {
                    vGridCentre + Vec3(-gridCellSize * 0.5f, -gridCellSize * 0.5f, 0.0f),
                    vGridCentre + Vec3(+gridCellSize * 0.5f, -gridCellSize * 0.5f, 0.0f),
                    vGridCentre + Vec3(+gridCellSize * 0.5f, +gridCellSize * 0.5f, 0.0f),
                    vGridCentre + Vec3(-gridCellSize * 0.5f, +gridCellSize * 0.5f, 0.0f)
                };

                enum
                {
                    NUM_POINTS = sizeof(points) / sizeof(points[0])
                };

                for (int i = 0; i < NUM_POINTS; ++i)
                {
                    points[i].z = p3DEngine->GetTerrainElevation(points[i].x, points[i].y);
                }

                pRC->DrawTriangle(points[0], colourB, points[1], colourB, points[2], colourB);
                pRC->DrawTriangle(points[2], colourB, points[3], colourB, points[0], colourB);
            }
        }
    }

    float const yPos = 300.0f;
    float const fColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    gEnv->pRenderer->Draw2dLabel(30, yPos, 1.35f, fColor, false, "Area Grid Mem Use: num cells: %d, memAlloced: %.2fk",
        m_nCells, (4 * m_bitFieldSizeU32 * m_nCells * 2) / 1024.f);
#endif // _RELEASE
}

//////////////////////////////////////////////////////////////////////////
float CAreaGrid::GetGreatestFadeDistance(CArea const* const pArea)
{
    float fGreatestFadeDistance = 0.0f;
    TEntityIDs const& aEntityIDs = (*(pArea->GetEntities()));
    TEntityIDs::const_iterator Iter(aEntityIDs.begin());
    TEntityIDs::const_iterator const IterEnd(aEntityIDs.end());

    for (; Iter != IterEnd; ++Iter)
    {
        IEntity const* const pEntity = (m_pEntitySystem != NULL) ? m_pEntitySystem->GetEntity((*Iter)) : NULL;

        if (pEntity != NULL)
        {
            IComponentAudioConstPtr const pAudioComponent = pEntity->GetComponent<IComponentAudio>();

            if (pAudioComponent != NULL)
            {
                fGreatestFadeDistance = max(fGreatestFadeDistance, pAudioComponent->GetFadeDistance());
            }
        }
    }

    return fGreatestFadeDistance;
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Debug_CheckBB(Vec2 const& vBBCentre, Vec2 const& vBBExtent, CArea const* const pArea)
{
    uint32 nAreas = 0;

    Vec2 minPos(vBBCentre.x - vBBExtent.x, vBBCentre.y - vBBExtent.y);
    TAreaPointers const& rAreasAtMinPos = GetAreas(minPos);
    bool bFoundMin = false;

    TAreaPointers::const_iterator IterMin(rAreasAtMinPos.begin());
    TAreaPointers::const_iterator const IterMinEnd(rAreasAtMinPos.end());

    for (; IterMin != IterMinEnd; ++IterMin)
    {
        if (*IterMin == pArea)
        {
            bFoundMin = true;
            break;
        }
    }

    Vec2 maxPos(vBBCentre.x + vBBExtent.x, vBBCentre.y + vBBExtent.y);
    TAreaPointers const& rAreasAtMaxPos = GetAreas(maxPos);
    bool bFoundMax = false;

    TAreaPointers::const_iterator IterMax(rAreasAtMaxPos.begin());
    TAreaPointers::const_iterator const IterMaxEnd(rAreasAtMaxPos.end());

    for (; IterMax != IterMaxEnd; ++IterMax)
    {
        if (*IterMax == pArea)
        {
            bFoundMax = true;
            break;
        }
    }

    //caveat: this may fire if the area was clamped
    if ((!bFoundMin || !bFoundMax) /*&& !bClamped*/)
    {
        CryLogAlways("Error: AABB extent was not found in grid\n");
    }
}
#endif
