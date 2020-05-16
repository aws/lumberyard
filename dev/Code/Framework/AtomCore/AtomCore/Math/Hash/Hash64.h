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

/**
 * @file Hash64.h
 * @brief Contains the declaration of the 64 bit hashing function available in Lumberyard
 */
#pragma once
#include <cstdint>

namespace AZ
{

    uint64_t Hash64(const char* buffer, uint64_t length);
    uint64_t Hash64(const char* buffer, uint64_t length, uint64_t seed);
    uint64_t Hash64(const char* buffer, uint64_t length, uint64_t seed1, uint64_t seed2);

    template <typename T>
    uint64_t Hash64(const T& t)
    {
        return Hash64(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    template <typename T>
    uint64_t Hash64(const T& t, uint64_t seed)
    {
        return Hash64(reinterpret_cast<const char*>(&t), sizeof(T), seed);
    }

    template <typename T>
    uint64_t Hash64(const T& t, uint64_t seed1, uint64_t seed2)
    {
        return Hash64(reinterpret_cast<const char*>(&t), sizeof(T), seed1, seed2);
    }

} // namespace AZ