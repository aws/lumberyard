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
 * @file Hash64.cpp
 * @brief Contains the definition of the Hash64 functions supported by Lumberyard
 */
#include <AtomCore/Math/Hash/Hash64.h>
#include <AzCore/base.h>
#include <city.h>

namespace AZ
{

    /** 
     * @return uint64_t The resulting hash of input memory address and length in bytes
     * @param[in] buffer Pointer to the memory to be hashed
     * @param[in] length The length in bytes of memory to read for generating the hash
     */
    uint64_t Hash64(const char* buffer, uint64_t length)
    {
        AZ_Assert(buffer, "Hash64() - buffer cannot be null");

        return CityHash64(buffer, length);
    }

    /**
    * @return uint64_t The resulting hash of input memory address and length in bytes
    * @param[in] buffer Pointer to the memory to be hashed
    * @param[in] length The length in bytes of memory to read for generating the hash
    * @param[in] seed A seed that is also hashed into the result
    */
    uint64_t Hash64(const char* buffer, uint64_t length, uint64_t seed)
    {
        AZ_Assert(buffer, "Hash64() - buffer cannot be null");

        return CityHash64WithSeed(buffer, length, seed);
    }

    /**
     * @return uint64_t The resulting hash of input memory address and length in bytes
     * @param[in] buffer Pointer to the memory to be hashed
     * @param[in] length The length in bytes of memory to read for generating the hash
     * @param[in] seed1 First seed that is also hashed into the result
     * @param[in] seed2 Second seed that is also hashed into the result
     */
    uint64_t Hash64(const char* buffer, uint64_t length, uint64_t seed1, uint64_t seed2)
    {
        AZ_Assert(buffer, "Hash64() - buffer cannot be null");

        return CityHash64WithSeeds(buffer, length, seed1, seed2);
    }
} // namespace AZ