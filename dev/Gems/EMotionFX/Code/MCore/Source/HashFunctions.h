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

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include "UnicodeString.h"


namespace MCore
{
    /**
     * The hash function.
     * The hash function must return an non-negative (so positive) integer, based on a key value.
     * Use partial template specialization to implement hashing functions for different data types.
     */
    template <typename Key>
    MCORE_INLINE uint32 Hash(const Key& key)
    {
        MCORE_ASSERT(false); // you should implement this function
        MCORE_UNUSED(key);
        //#pragma message (MCORE_ERROR "You should implement the Hash function for some specific Key type that you used")
        return 0;
    }


    template<>
    MCORE_INLINE uint32 Hash<String>(const String& key)
    {
        uint32 result = 0;
        const uint32 length = key.GetLength();
        for (uint32 i = 0; i < length; ++i)
        {
            result = (result << 4) + key[i];
            const uint32 g = result & 0xf0000000L;
            if (g != 0)
            {
                result ^= g >> 24;
            }
            result &= ~g;
        }

        return result;
    }


    template<>
    MCORE_INLINE uint32 Hash<StringRef>(const StringRef& key)
    {
        uint32 result = 0;
        const char* keyData = key.AsChar();
        const uint32 length = key.GetLength();
        for (uint32 i = 0; i < length; ++i)
        {
            result = (result << 4) + keyData[i];
            const uint32 g = result & 0xf0000000L;
            if (g != 0)
            {
                result ^= g >> 24;
            }
            result &= ~g;
        }

        return result;
    }


    template<>
    MCORE_INLINE uint32 Hash<int32>(const int32& key)
    {
        return (uint32)Math::Abs(static_cast<float>(key));
    }


    template<>
    MCORE_INLINE uint32 Hash<uint32>(const uint32& key)
    {
        return key;
    }


    template<>
    MCORE_INLINE uint32 Hash<float>(const float& key)
    {
        return (uint32)Math::Abs(key * 12345.0f);
    }


    template<>
    MCORE_INLINE uint32 Hash<Vector3>(const Vector3& key)
    {
        return (uint32)Math::Abs(key.x * 101.0f + key.y * 1002.0f + key.z * 10003.0f);
    }
}   // namespace MCore
