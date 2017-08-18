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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_WAYPOINTHUMANNAVREGION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_WAYPOINTHUMANNAVREGION_H
#pragma once


#include "NavRegion.h"
#include "AICollision.h"
#include <set>

class CNavigation;

#ifdef STATUS_PENDING
#undef STATUS_PENDING
#endif

/// Handles all graph operations that relate to the indoor/waypoint aspect
class CWaypointHumanNavRegion
    : public CNavRegion
{
public:
    CWaypointHumanNavRegion(CNavigation* pGraph);
    virtual ~CWaypointHumanNavRegion();

    /// inherited
    virtual unsigned GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "");

    /// Serialise the _modifications_ since load-time
    virtual void Serialize(TSerialize ser);

    /// inherited
    virtual void Clear();

    /// inherited
    virtual void Reset(IAISystem::EResetReason reason);

    /// inherited
    virtual size_t MemStats();

    // Functions specific to this navigation region
    unsigned GetClosestNode(const Vec3& pos, int nBuildingID);

    /// As IGraph. If flushQueue then the pending update queue is processed.
    /// If false then this reconnection request gets queued
    void ReconnectWaypointNodesInBuilding(int nBuildingID);

    /// Removes all automatically generated links (not affecting
    /// hand-generated links)
    void RemoveAutoLinksInBuilding(int nBuildingID);

    /// Finds the floor position then checks if a body will fit there, using the passability
    /// parameters. If pRenderer != 0 then the body gets drawn (green if it's OK, red if not)
    bool CheckAndDrawBody(const Vec3& pos, IRenderer* pRenderer);

    // should be called when anything happens that might upset the dynamic updates (e.g. node created/destroyed)
    void ResetUpdateStatus();

    //void CheckLinkLengths() const;

    // TODO Mai 21, 2007: <pvl> if there are links connecting two waypoints
    // in both directions, this iterator currently returns only one of them,
    // picked up at random essentially.  This means that it's only useful
    // for retrieving unidirectional data (that are the same in both directions).
    // It would be fairly easy make the iterator configurable so it can
    // optionally return both directions for any two given nodes.

    /// Iterates over all links in a graph that connect two human waypoints.
    class LinkIterator
    {
        const CGraph* m_pGraph;
        std::unique_ptr <CAllNodesContainer::Iterator> m_currNodeIt;
        unsigned int m_currLinkIndex;

        void FindNextWaypointLink();
    public:
        LinkIterator (const CWaypointHumanNavRegion*);
        LinkIterator (const LinkIterator&);

        operator bool () const {
            return m_currLinkIndex != 0;
        }
        LinkIterator& operator++ ();
        unsigned int operator* () const { return m_currLinkIndex; }
    };
    // TODO Mai 21, 2007: <pvl> doesn't really need to be a friend, just needs to access
    // region's m_pGraph member ...
    friend class LinkIterator;

    LinkIterator CreateLinkIterator () const { return LinkIterator (this); }

private:
    void FillGreedyMap(unsigned nodeIndex, const Vec3& pos,
        IVisArea* pTargetArea, bool bStayInArea, CandidateIdMap& candidates);

    /// Returns true if the link between two nodes would intersect the boundary (in 2D)
    bool WouldLinkIntersectBoundary(const Vec3& pos1, const Vec3& pos2, const ListPositions& boundary) const;

    /// just disconnects automatically created links
    void RemoveAutoLinks(unsigned nodeIndex);

    /// Connect all nodes - the two cache lists can overlap.
    /// Links are connected in order of increasing distance. Linking continues
    /// until (1) dist > maxDistNormal and either (2.1) a node has > nLinksForMaxDistMax
    /// or (2.2) dist > maxDistMax
    void ConnectNodes(const struct SpecialArea* pArea, EAICollisionEntities collisionEntities, float maxDistNormal, float maxDistMax, unsigned nLinksForMaxDistMax);

    /// Modify all connections within an area
    void ModifyConnections(const struct SpecialArea* pArea, EAICollisionEntities collisionEntities, bool adjustOriginalEditorLinks);

    /// checks and modifies a single connection coming from a node.
    void ModifyNodeConnections(unsigned nodeIndex, EAICollisionEntities collisionEntities, unsigned iLink, bool adjustOriginalEditorLinks);

#if 0
    /// Checks walkability using a faster algorithm than CheckWalkability() if possible.
    bool CheckPassability(unsigned linkIndex, Vec3 from, Vec3 to,
        float paddingRadius, bool checkStart,
        const ListPositions& boundary,
        EAICollisionEntities aiCollisionEntities,
        SCachedPassabilityResult* pCachedResult,
        SCheckWalkabilityState* pCheckWalkabilityState);
#endif

    /// Finds out if a link is eligible for simplified passability check.  The
    /// first return value says if the link is passable at all.  If not, the second
    /// one is not meaningful, otherwise it indicates if the link is also "simple".
    /// Note that all of the above is only valid if pCheckWalkabilityState->state
    /// is RESULT on return - otherwise the computation hasn't completed yet
    /// and passability information isn't available at all in the first place.
    std::pair<bool, bool> CheckIfLinkSimple(Vec3 from, Vec3 to,
        float paddingRadius, bool checkStart,
        const ListPositions& boundary,
        EAICollisionEntities aiCollisionEntities,
        SCheckWalkabilityState* pCheckWalkabilityState);

private:
    typedef std::vector< std::pair<float, unsigned> > TNodes;

private:
    static size_t s_instanceCount;
    static TNodes s_tmpNodes;

private:
    /// Iterator over all nodes for distributing the cxn modifications over multiple updates
    CAllNodesContainer::Iterator* m_currentNodeIt;

    /// link counter for distributing the cxn modifications over multiple updates
    unsigned m_currentLink;

    /// Used for distributing walkability checks over multiple updates
    SCheckWalkabilityState m_checkWalkabilityState;

    CNavigation* m_pNavigation;

    // Prevent copy as it would break the instance count
    CWaypointHumanNavRegion(const CWaypointHumanNavRegion&);
    CWaypointHumanNavRegion& operator=(const CWaypointHumanNavRegion&);
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_WAYPOINTHUMANNAVREGION_H
