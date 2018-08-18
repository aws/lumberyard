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


#ifndef CRYINCLUDE_CRYAISYSTEM_HIDESPOT_H
#define CRYINCLUDE_CRYAISYSTEM_HIDESPOT_H
#pragma once

#include "SmartObjects.h"


// Description:
//   Structure that contains critical hidespot information.
struct SHideSpotInfo
{
    enum EHideSpotType
    {
        eHST_TRIANGULAR,
        eHST_WAYPOINT,
        eHST_ANCHOR,
        eHST_SMARTOBJECT,
        eHST_VOLUME,
        eHST_DYNAMIC,
        eHST_INVALID,
    };

    EHideSpotType type;
    Vec3 pos;
    Vec3 dir;

    SHideSpotInfo()
        : type(eHST_INVALID)
        , pos(ZERO)
        , dir(ZERO) {}
    SHideSpotInfo(EHideSpotType _type, const Vec3& _pos, const Vec3& _dir)
        : type(_type)
        , pos(_pos)
        , dir(_dir) {}
};


struct SHideSpot
{
    SHideSpot();
    SHideSpot(SHideSpotInfo::EHideSpotType type, const Vec3& pos, const Vec3& dir);

    bool IsSecondary() const;

    //////////////////////////////////////////////////////////////////////////

    SHideSpotInfo info;

    // optional parameters - can be used with multiple hide spot types
    const GraphNode*                                            pNavNode;
    const std::vector<const GraphNode*>*    pNavNodes;
    EntityId entityId;      // The entity id of the source object for dynamic hidepoints.

    // parameters used only with one specific hide spot type
    const ObstacleData* pObstacle;      // triangular
    CQueryEvent                 SOQueryEvent;   // smart objects
    const CAIObject*        pAnchorObject;  // anchors
};

typedef std::multimap<float, SHideSpot> MultimapRangeHideSpots;

#endif // CRYINCLUDE_CRYAISYSTEM_HIDESPOT_H
