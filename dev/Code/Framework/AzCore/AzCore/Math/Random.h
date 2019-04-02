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
#ifndef AZCORE_MATH_RANDOM_H
#define AZCORE_MATH_RANDOM_H 1

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include <stdio.h>
#endif

namespace AZ
{
    /**
     * A very simple and fast LCG random number generator, useful if you need a fast pseudo-random sequence and
     * don't really care about the quality of the sequence.
     * Based on the java Random class, since it's decent quality for a LCG, e.g. avoids cycles in the lower bits.
     * See http://download.oracle.com/javase/6/docs/api/java/util/Random.html for details, also a good reference for
     * some subtle issues.
     */
    class SimpleLcgRandom
    {
    public:
        AZ_TYPE_INFO(SimpleLcgRandom, "{52BD3FA7-C15D-41FA-BC85-0FD4D310F688}")

        SimpleLcgRandom(u64 seed = 1234) { SetSeed(seed); }

        void SetSeed(u64 seed)
        {
            m_seed = (seed ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
        }

        unsigned int GetRandom()
        {
            m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            return static_cast<unsigned int>(m_seed >> 16);
        }

        //Gets a random float in the range [0,1)
        float GetRandomFloat()
        {
            unsigned int r = GetRandom();
            r &= 0x007fffff; //sets mantissa to random bits
            r |= 0x3f800000; //result is in [1,2), uniformly distributed
            union
            {
                float f;
                unsigned int i;
            } u;
            u.i = r;
            return u.f - 1.0f;
        }

    private:
        u64 m_seed;
    };

    /**
     * As an attempt to improve the pseudo-random (rand, time) we use a better random.
     * The cost of computing one if making it to great for high performance application, but
     * it's a great tool to get a good seed.
     */
    class BetterPseudoRandom
    {
    public:
        BetterPseudoRandom();
        ~BetterPseudoRandom();

        /// Fills a buffer with random data, if true is returned. Otherwise the random generator fails to generate random numbers.
        bool GetRandom(void* data, size_t dataSize);

        /// Template helper for value types \ref GetRandom
        template<class T>
        bool GetRandom(T& value)    { return GetRandom(&value, sizeof(T)); }

    private:
#if defined(AZ_PLATFORM_WINDOWS)
#   ifdef AZ_OS64
        unsigned __int64 m_generatorHandle;
#   else
        unsigned long m_generatorHandle;
#   endif // AZ_OS64
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif AZ_TRAIT_PSUEDO_RANDOM_USE_FILE
        FILE * m_generatorHandle;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Random_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Random_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#       error Platform not supported
#endif
    };

    /**
    * An enum representing the different random distributions available
    *
    * These map to random distributions in std::random. Those distributions
    * have no common base class so this is an enum to help determine which
    * random distribution is needed.
    */
    enum class RandomDistributionType : AZ::u32
    {
        Normal,
        UniformReal
    };
}

#endif
#pragma once
