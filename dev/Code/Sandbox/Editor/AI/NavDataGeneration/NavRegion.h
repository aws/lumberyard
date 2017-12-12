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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVREGION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVREGION_H
#pragma once


#include "Graph.h"
#include "SerializeFwd.h"

struct AgentMovementAbility;

/// Enforce a standard interface on the different types of navigation region
class CNavRegion
{
public:
    CNavRegion() {}
    virtual ~CNavRegion() {}

    /// Should return the graph node containing pos, using pStart as a hint as to
    /// where to start looking. If no node can be found then do an approximate search
    /// by extending the internal area/volume representation by range, and return the
    /// closest position that is valid for the returned graph node. If returnSuspect
    /// then a node may be returned even if it may not be reachable.
    /// If there are a number of nodes that could be returned, only nodes that have at least one
    /// link with radius > passRadius will be returned.
    virtual unsigned GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "") = 0;

    /// Should restore its original state
    virtual void Clear() = 0;

    /// Gets called on enter/exit etc from game mode - i.e. opportunity to clear/reset any dynamic updating
    /// data.
    virtual void Reset(IAISystem::EResetReason reason) {};

    /// Serialise the _modifications_ since load-time
    virtual void Serialize(TSerialize ser) = 0;

    /// Gets called when a node has been created (i.e. the region may choose to connect
    /// the node somewhere automatically)
    virtual void NodeCreated(unsigned nodeIndex) {}

    /// Called just after a node has been moved
    virtual void NodeMoved(unsigned nodeIndex) {}

    /// This gets called just _before_ the node is going to be deleted. Don't delete/destroy it
    /// during this call!!!
    virtual void NodeAboutToBeDeleted(GraphNode* pNode) {}

    /// Will get called when the mission has finished loaded (so all nav regions etc are created)
    virtual void OnMissionLoaded() {};

    /// Should return the memory usage in bytes
    virtual size_t MemStats() = 0;
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_NAVREGION_H
