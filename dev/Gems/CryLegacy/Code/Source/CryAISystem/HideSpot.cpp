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

// Description : Hidespot-related structures


#include "CryLegacy_precompiled.h"
#include "HideSpot.h"


SHideSpot::SHideSpot()
    : pNavNode(0)
    , pNavNodes(0)
    , pAnchorObject(0)
    , pObstacle(0)
    , entityId(0)
{
}

SHideSpot::SHideSpot(SHideSpotInfo::EHideSpotType type, const Vec3& pos, const Vec3& dir)
    : info(type, pos, dir)
    , pNavNode(0)
    , pNavNodes(0)
    , pAnchorObject(0)
    , pObstacle(0)
    , entityId(0)
{
}

bool SHideSpot::IsSecondary() const
{
    switch (info.type)
    {
    case SHideSpotInfo::eHST_TRIANGULAR:
        return pObstacle && !pObstacle->IsCollidable();
    case SHideSpotInfo::eHST_WAYPOINT:
        return pNavNode && (pNavNode->navType == IAISystem::NAV_WAYPOINT_HUMAN) &&
               (pNavNode->GetWaypointNavData()->type == WNT_HIDESECONDARY);
    case SHideSpotInfo::eHST_ANCHOR:
        return pAnchorObject && (pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY);
    }

    return false;
}
