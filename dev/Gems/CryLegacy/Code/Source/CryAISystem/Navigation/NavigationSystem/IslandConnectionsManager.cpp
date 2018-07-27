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
#include "IslandConnectionsManager.h"

IslandConnectionsManager::IslandConnectionsManager()
{
}

void IslandConnectionsManager::Reset()
{
    m_globalIslandConnections.Reset();
}

MNM::IslandConnections& IslandConnectionsManager::GetIslandConnections()
{
    return m_globalIslandConnections;
}

void IslandConnectionsManager::SetOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const MNM::IslandConnections::Link link)
{
    m_globalIslandConnections.SetOneWayConnectionBetweenIsland(fromIsland, link);
}

bool IslandConnectionsManager::AreIslandsConnected(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID startIsland, const MNM::GlobalIslandID endIsland) const
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    MNM::IslandConnections::TIslandsWay way;
    return m_globalIslandConnections.CanNavigateBetweenIslands(pEntityToTestOffGridLinks, startIsland, endIsland, way);
}

#ifdef CRYAISYSTEM_DEBUG

void IslandConnectionsManager::DebugDraw() const
{
    m_globalIslandConnections.DebugDraw();
}

#endif