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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_ISLANDCONNECTIONSMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_ISLANDCONNECTIONSMANAGER_H
#pragma once

#include <INavigationSystem.h>
#include "../MNM/IslandConnections.h"

class IslandConnectionsManager
{
public:
    IslandConnectionsManager();
    void Reset();

    MNM::IslandConnections& GetIslandConnections();

    void SetOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const MNM::IslandConnections::Link link);

    bool AreIslandsConnected(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID startIsland, const MNM::GlobalIslandID endIsland) const;

#ifdef CRYAISYSTEM_DEBUG
    void DebugDraw() const;
#endif

private:
    MNM::IslandConnections m_globalIslandConnections;
};

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_ISLANDCONNECTIONSMANAGER_H
