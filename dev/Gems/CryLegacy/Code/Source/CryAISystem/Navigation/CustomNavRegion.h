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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_CUSTOMNAVREGION_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_CUSTOMNAVREGION_H
#pragma once


#include "NavRegion.h"

class CGraph;

/// Handles all graph operations that relate to the smart objects aspect
class CCustomNavRegion
    : public CNavRegion
{
    CGraph*                     m_pGraph;

public:
    CCustomNavRegion(CGraph* pGraph)
        : m_pGraph(pGraph)
    {}

    virtual ~CCustomNavRegion() {}

    /// inherited
    virtual void BeautifyPath(
        const VectorConstNodeIndices& inPath, TPathPoints& outPath,
        const Vec3& startPos, const Vec3& startDir,
        const Vec3& endPos, const Vec3& endDir,
        float radius,
        const AgentMovementAbility& movementAbility,
        const NavigationBlockers& navigationBlockers)
    {
        UglifyPath(inPath, outPath, startPos, startDir, endPos, endDir);
    }

    /// inherited
    virtual void UglifyPath(
        const VectorConstNodeIndices& inPath, TPathPoints& outPath,
        const Vec3& startPos, const Vec3& startDir,
        const Vec3& endPos, const Vec3& endDir);
    /// inherited
    virtual unsigned    GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "", bool omitWalkabilityTest = false)
    {
        return 0;
    }

    /// Serialise the _modifications_ since load-time
    virtual void            Serialize(TSerialize ser)
    {}

    /// inherited
    virtual void            Clear();

    /// inherited
    virtual size_t      MemStats();

    float                           GetCustomLinkCostFactor (const GraphNode* nodes[2], const PathfindingHeuristicProperties& pathFindProperties) const;

    uint32                      CreateCustomNode                (const Vec3& pos, void* customData, uint16 customId, SCustomNavData::CostFunction pCostFunction, bool linkWithEnclosing = true);
    void                            RemoveNode                          (uint32 nodeIndex);
    void                            LinkCustomNodes                 (uint32 node1Index, uint32 node2Index, float radius1To2, float radius2To1);
};


#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_CUSTOMNAVREGION_H
