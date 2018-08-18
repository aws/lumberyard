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
#include "IslandConnections.h"
#include <IPathfinder.h>
#include "../NavigationSystem/NavigationSystem.h"
#include "OffGridLinks.h"
#ifdef CRYAISYSTEM_DEBUG
#include "DebugDrawContext.h"
#endif

void MNM::IslandConnections::SetOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const Link link)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    TLinksVector& links = m_islandConnections[fromIsland];
    stl::push_back_unique(links, link);
}

void MNM::IslandConnections::RemoveOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const Link link)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    TIslandConnectionsMap::iterator linksIt = m_islandConnections.find(fromIsland);
    if (linksIt != m_islandConnections.end())
    {
        TLinksVector& links = linksIt->second;
        if (!links.empty())
        {
            links.erase(std::remove(links.begin(), links.end(), link), links.end());
        }

        if (links.empty())
        {
            m_islandConnections.erase(linksIt);
        }
    }
}

struct IsLinkAssociatedWithObjectPredicate
{
    IsLinkAssociatedWithObjectPredicate(const uint32 objectId)
        : m_objectId(objectId)
    {}

    bool operator()(const MNM::IslandConnections::Link& linkToEvaluate)
    {
        return linkToEvaluate.objectIDThatCreatesTheConnection == m_objectId;
    }

    uint32 m_objectId;
};

void MNM::IslandConnections::RemoveAllIslandConnectionsForObject(const NavigationMeshID& meshID, const uint32 objectId)
{
    TIslandConnectionsMap::iterator linksIt = m_islandConnections.begin();
    TIslandConnectionsMap::iterator linksEnd = m_islandConnections.end();
    for (; linksIt != linksEnd; )
    {
        if (NavigationMeshID(linksIt->first.GetNavigationMeshIDAsUint32()) == meshID)
        {
            TLinksVector& links = linksIt->second;
            links.erase(std::remove_if(links.begin(), links.end(), IsLinkAssociatedWithObjectPredicate(objectId)), links.end());
            if (links.empty())
            {
                m_islandConnections.erase(linksIt++);
                continue;
            }
        }

        ++linksIt;
    }
}

bool MNM::IslandConnections::CanNavigateBetweenIslands(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, TIslandsWay& way) const
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    const static MNM::GlobalIslandID invalidID(MNM::Constants::eGlobalIsland_InvalidIslandID);
    IF_UNLIKELY (fromIsland == invalidID || toIsland == invalidID)
    {
        return false;
    }

    if (fromIsland == toIsland)
    {
        return true;
    }

    const NavigationMeshID startingMeshID = NavigationMeshID(fromIsland.GetNavigationMeshIDAsUint32());
    const MNM::OffMeshNavigation& offMeshLink = gAIEnv.pNavigationSystem->GetOffMeshNavigationManager()->GetOffMeshNavigationForMesh(startingMeshID);

    const size_t maxConnectedIsland = m_islandConnections.size();

    TIslandClosedSet closedSet;
    closedSet.reserve(maxConnectedIsland);
    IslandOpenList openList(maxConnectedIsland);
    openList.InsertElement(IslandNode(fromIsland, 0));
    TCameFromMap cameFrom;

    while (!openList.IsEmpty())
    {
        IslandNode currentItem(openList.PopBestElement());

        if (currentItem.id == toIsland)
        {
            ReconstructWay(cameFrom, fromIsland, toIsland, way);
            return true;
        }

        closedSet.push_back(currentItem.id);

        TIslandConnectionsMap::const_iterator islandConnectionsIt = m_islandConnections.find(currentItem.id);
        TIslandConnectionsMap::const_iterator islandConnectionsEnd = m_islandConnections.end();

        if (islandConnectionsIt != islandConnectionsEnd)
        {
            const TLinksVector& links = islandConnectionsIt->second;
            TLinksVector::const_iterator linksIt = links.begin();
            TLinksVector::const_iterator linksEnd = links.end();
            for (; linksIt != linksEnd; ++linksIt)
            {
                if (std::find(closedSet.begin(), closedSet.end(), linksIt->toIsland) != closedSet.end())
                {
                    continue;
                }

                const bool canUseLink = pEntityToTestOffGridLinks ? offMeshLink.CanUseLink(const_cast<IEntity*>(pEntityToTestOffGridLinks), linksIt->offMeshLinkID, NULL) : true;

                if (canUseLink)
                {
                    IslandNode nextIslandNode(linksIt->toIsland, currentItem.cost + 1.0f);

                    // At this point, if we have multiple connections to the same neighbour island,
                    // we cannot detect which one is the one that allows us to have the actual shortest path
                    // so to keep the code simple we will keep the first one

                    if (cameFrom.find(nextIslandNode) != cameFrom.end())
                    {
                        continue;
                    }

                    cameFrom[nextIslandNode] = currentItem;
                    openList.InsertElement(nextIslandNode);
                }
            }
        }
    }

    way.clear();

    return false;
}

void MNM::IslandConnections::Reset()
{
    m_islandConnections.clear();
}

void MNM::IslandConnections::ReconstructWay(const TCameFromMap& cameFromMap, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, TIslandsWay& way) const
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    IslandNode currentIsland(toIsland, .0f);
    way.push_front(currentIsland.id);

    TCameFromMap::const_iterator element = cameFromMap.find(currentIsland);
    while (element != cameFromMap.end())
    {
        currentIsland = element->second;
        way.push_front(currentIsland.id);
        element = cameFromMap.find(currentIsland);
    }
}

#ifdef CRYAISYSTEM_DEBUG

void MNM::IslandConnections::DebugDraw() const
{
    const size_t totalGlobalIslandsRegisterd = m_islandConnections.size();
    const size_t memoryUsedByALink = sizeof(Link);
    size_t totalUsedMemory, totalEffectiveMemory;
    totalUsedMemory = totalEffectiveMemory = 0;
    TIslandConnectionsMap::const_iterator linksIt = m_islandConnections.begin();
    TIslandConnectionsMap::const_iterator linksEnd = m_islandConnections.end();
    for (; linksIt != linksEnd; ++linksIt)
    {
        const TLinksVector& links = linksIt->second;
        totalUsedMemory += links.size() * memoryUsedByALink;
        totalEffectiveMemory += links.capacity() * memoryUsedByALink;
    }

    const float conversionValue = 1024.0f;
    CDebugDrawContext dc;
    dc->Draw2dLabel(10.0f, 5.0f,  1.6f, Col_White, false, "Amount of global islands registered into the system %d", (int)totalGlobalIslandsRegisterd);
    dc->Draw2dLabel(10.0f, 25.0f, 1.6f, Col_White, false, "MNM::IslandConnections total used memory %.2fKB", totalUsedMemory / conversionValue);
    dc->Draw2dLabel(10.0f, 45.0f, 1.6f, Col_White, false, "MNM::IslandConnections total effective memory %.2fKB", totalEffectiveMemory / conversionValue);
}

#endif
