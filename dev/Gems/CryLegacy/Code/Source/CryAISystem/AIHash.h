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

#ifndef CRYINCLUDE_CRYAISYSTEM_AIHASH_H
#define CRYINCLUDE_CRYAISYSTEM_AIHASH_H
#pragma once

//===================================================================
// HashFromFloat
//===================================================================
ILINE size_t HashFromFloat(float key, float tol, float invTol)
{
    float val = key;

    if (tol > 0.0f)
    {
        val *= invTol;
        val = floor_tpl(val);
        val *= tol;
    }

    union f32_u
    {
        float floatVal;
        size_t uintVal;
    };

    f32_u u;
    u.floatVal = val;

    size_t hash = u.uintVal;
    hash += ~(hash << 15);
    hash ^= (hash >> 10);
    hash += (hash << 3);
    hash ^= (hash >> 6);
    hash += ~(hash << 11);
    hash ^= (hash >> 16);

    return hash;
}

ILINE size_t HashFromUInt(size_t value)
{
    size_t hash = value;
    hash += ~(hash << 15);
    hash ^= (hash >> 10);
    hash += (hash << 3);
    hash ^= (hash >> 6);
    hash += ~(hash << 11);
    hash ^= (hash >> 16);

    return hash;
}




//===================================================================
// HashFromVec3
//===================================================================
inline size_t HashFromVec3(const Vec3& v, float tol, float invTol)
{
    return HashFromFloat(v.x, tol, invTol) + HashFromFloat(v.y, tol, invTol) + HashFromFloat(v.z, tol, invTol);
}

//===================================================================
// HashFromQuat
//===================================================================
inline size_t HashFromQuat(const Quat& q, float tol, float invTol)
{
    return HashFromFloat(q.v.x, tol, invTol) + HashFromFloat(q.v.y, tol, invTol) +
           HashFromFloat(q.v.z, tol, invTol) + HashFromFloat(q.w, tol, invTol);
}

//===================================================================
// GetHashFromEntities
//===================================================================
inline size_t GetHashFromEntities(const IPhysicalEntity* const* entities, size_t entityCount)
{
    size_t hash = 0;
    pe_status_pos status;

    for (size_t i = 0; i < entityCount; ++i)
    {
        const IPhysicalEntity* entity = entities[i];
        entity->GetStatus(&status);

        hash += HashFromUInt((size_t)(UINT_PTR)entity);
        hash += HashFromVec3(status.pos, 0.05f, 1.0f / 0.05f);
        hash += HashFromQuat(status.q, 0.01f, 1.0f / 0.01f);
    }

    if (hash == 0)
    {
        hash = 0x1337d00d;
    }

    return hash;
}

#endif // CRYINCLUDE_CRYAISYSTEM_AIHASH_H
