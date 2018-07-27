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

#ifndef CRYINCLUDE_CRYACTION_IWORLDQUERY_H
#define CRYINCLUDE_CRYACTION_IWORLDQUERY_H
#pragma once

#include "IGameObject.h"

typedef std::vector<EntityId> Entities;

struct IWorldQuery
    : IGameObjectExtension
{
    IWorldQuery() {}

    virtual IEntity*                   GetEntityInFrontOf() = 0;
    virtual const EntityId*     ProximityQuery(int& numberOfEntities) = 0;
    virtual const Vec3&             GetPos() const = 0;
    virtual const Vec3&             GetDir() const = 0;
    virtual const EntityId      GetLookAtEntityId(bool ignoreGlass = false) = 0;
    virtual const ray_hit*      GetLookAtPoint(const float fMaxDist = 0, bool ignoreGlass = false) = 0;
    virtual const ray_hit*    GetBehindPoint(const float fMaxDist = 0) = 0;
    virtual const EntityId*   GetEntitiesAround(int& num) = 0;
    virtual IPhysicalEntity* const* GetPhysicalEntitiesAround(int& num) = 0;
    virtual IPhysicalEntity* GetPhysicalEntityInFrontOf() = 0;
};

#endif // CRYINCLUDE_CRYACTION_IWORLDQUERY_H
