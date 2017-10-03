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
#ifndef AZCORE_CRC_H
#define AZCORE_CRC_H 1

#include <AzCore/base.h>

//////////////////////////////////////////////////////////////////////////
// Macros for pre-processor Crc32 conversion
//
// When you user AZ_CRC("My string") by default it will map to AZ::Crc32("My string").
// We do have a pro-processor program which will precompute the crc for you and
// transform that macro to AZ_CRC("My string",0xabcdef00) this will expand to just 0xabcdef00.
// This will remove completely the "My string" from your executable, it will add it to a database and so on.
// WHen you want to update the string, just change the string.
// If you don't run the precomile step the code should still run fine, except it will be slower,
// the strings will be in the exe and converted on the fly and you can't use the result where you need
// a constant expression.
// For example
// switch(id) {
//  case AZ_CRC("My string",0xabcdef00): {} break; // this will compile fine
//  case AZ_CRC("My string"): {} break; // this will cause "error C2051: case expression not constant"
// }
// So it's you choice what you do, depending on your needs.
//
/// Implementation when we have only 1 param (by default it should be string.
#define AZ_CRC_1(_1)    AZ::Crc32(_1)
/// Implementation when we have 2 params (we use the 2 only)
#define AZ_CRC_2(_1, _2) AZ::Crc32(AZ::u32(_2))

#define AZ_CRC(...)     AZ_MACRO_SPECIALIZE(AZ_CRC_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))
//////////////////////////////////////////////////////////////////////////
namespace AZ
{
    class SerializeContext;

    /**
     * Class for all of our crc32 types, better than just using ints everywhere.
     */
    class Crc32
    {
    public:
        /**
         * Initializes to 0.
         */
        AZ_MATH_FORCE_INLINE Crc32()
            : m_value(0)    {  }

        /**
         * Initializes from an int.
         */
        AZ_MATH_FORCE_INLINE Crc32(AZ::u32 value)       { m_value = value; }

        /**
         * Calculates the value from a string.
         */
        explicit Crc32(const char* str);

        /**
         * Calculates the value from a block of raw data.
         */
        Crc32(const void* data, size_t size, bool forceLowerCase = false);

        void Add(const char* str);
        void Add(const void* data, size_t size, bool forceLowerCase = false);

        AZ_MATH_FORCE_INLINE operator u32() const               { return m_value; }

        AZ_MATH_FORCE_INLINE bool operator==(Crc32 rhs) const   { return (m_value == rhs.m_value); }
        AZ_MATH_FORCE_INLINE bool operator!=(Crc32 rhs) const   { return (m_value != rhs.m_value); }

        AZ_MATH_FORCE_INLINE bool operator!() const             { return (m_value == 0); }

        static void Reflect(AZ::SerializeContext& context);

    protected:
        void Set(const void* data, size_t size, bool forceLowerCase = false);
        void Combine(u32 crc, size_t len);

        u32 m_value;
    };
};

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Crc32);
#endif

#endif // AZCORE_CRC32_H
#pragma once