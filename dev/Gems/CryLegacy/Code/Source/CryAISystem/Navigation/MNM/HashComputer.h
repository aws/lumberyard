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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_HASHCOMPUTER_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_HASHCOMPUTER_H
#pragma once


#include "MNM.h"

#if !defined(_MSC_VER) && !defined(_rotl)
    #define _rotl(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
#endif

namespace MNM
{
    struct HashComputer
    {
        HashComputer(uint32 seed = 0)
            : hash(seed)
            , len(0)
        {
        }

        inline void Add(float key)
        {
            union f32_u
            {
                float fv;
                uint32 uv;
            };

            f32_u u;
            u.fv = key;

            Add(u.uv);
        }

        inline void Add(uint32 key)
        {
            key *= 0xcc9e2d51;
            key = _rotl(key, 15);
            key *= 0x1b873593;

            hash ^= key;
            hash = _rotl(hash, 13);
            hash = hash * 5 + 0xe6546b64;

            len += 4;
        }

        inline void Complete()
        {
            hash ^= len;

            hash ^= hash >> 16;
            hash *= 0x85ebca6b;
            hash ^= hash >> 13;
            hash *= 0xc2b2ae35;
            hash ^= hash >> 16;
        }

        inline void Add(const Vec3& key)
        {
            Add(key.x);
            Add(key.y);
            Add(key.z);
        }

        inline void Add(const Quat& key)
        {
            Add(key.v);
            Add(key.w);
        }

        inline void Add(const Matrix34& key)
        {
            Add(key.GetTranslation());
            Add(key.GetColumn0());
            Add(key.GetColumn1());
            Add(key.GetColumn2());
        }

        inline uint32 GetValue() const
        {
            return hash;
        }
    private:
        uint32 len;
        uint32 hash;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_HASHCOMPUTER_H
