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
#include <AzCore/std/containers/bitset.h>
#include <type_traits>

#include "UserTypes.h"


namespace UnitTest
{
    using namespace AZStd;

    class BitsetTests
        : public AllocatorsFixture
    {
    };

    TEST_F(BitsetTests, DefaultConstructor_IsZero)
    {
        bitset<8> bitset;
        ASSERT_EQ(bitset.to_ullong(), 0);
    }

    TEST_F(BitsetTests, Constructor64Bits_MatchesInput)
    {
        constexpr AZ::u64 initValue = std::numeric_limits<AZ::u64>::max();
        bitset<64> bitset(initValue);

        ASSERT_EQ(bitset.to_ullong(), initValue);
    }

    TEST_F(BitsetTests, Constructor64BitsInto32Bits_MatchesLeastSignificant32Bits)
    {
        constexpr AZ::u64 initValue = std::numeric_limits<AZ::u64>::max();
        bitset<32> bitset(initValue);

        constexpr AZ::u64 expectedValue(initValue & static_cast<AZ::u32>(-1));

        ASSERT_EQ(bitset.to_ullong(), expectedValue);
    }

    TEST_F(BitsetTests, Constructor32BitsInto64Bits_ZeroPadRemaining)
    {
        constexpr AZ::u32 initValue = std::numeric_limits<AZ::u32>::max();
        bitset<64> bitset(initValue);

        constexpr AZ::u64 expectedValue = static_cast<AZ::u64>(initValue);;

        ASSERT_EQ(bitset.to_ullong(), expectedValue);
    }

    TEST_F(BitsetTests, GeneralTesting)
    {
        // BitsetTest-Begin
        typedef bitset<25> bitset25_type;

        bitset25_type bs;
        AZ_TEST_ASSERT(bs.count() == 0);

        bitset25_type bs1((unsigned long)5);
        AZ_TEST_ASSERT(bs1.count() == 2);
        AZ_TEST_ASSERT(bs1[0] && bs1[2]);

        string str("10110");
        bitset25_type bs2(str, 0, str.length());
        AZ_TEST_ASSERT(bs2.count() == 3);
        AZ_TEST_ASSERT(bs2[1] && bs2[2] && bs2[4]);

        bitset25_type::reference bit0 = bs2[0], bit1 = bs2[1];
        AZ_TEST_ASSERT(bit0 == false);
        AZ_TEST_ASSERT(bit1 == true);

        bs &= bs1;
        AZ_TEST_ASSERT(bs.count() == 0);

        bs |= bs1;
        AZ_TEST_ASSERT(bs.count() == 2);
        AZ_TEST_ASSERT(bs[0] && bs[2]);

        bs ^= bs2;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[0] && bs[1] && bs[4]);

        bs <<= 4;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[4] && bs[5] && bs[8]);

        bs >>= 3;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[1] && bs[2] && bs[5]);

        bs.set(3);
        AZ_TEST_ASSERT(bs.count() == 4);
        AZ_TEST_ASSERT(bs[1] && bs[2] && bs[3] && bs[5]);

        bs.set(1, false);
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(!bs[1] && bs[2] && bs[3] && bs[5]);

        bs.set();
        AZ_TEST_ASSERT(bs.count() == 25);

        bs.reset();
        AZ_TEST_ASSERT(bs.count() == 0);

        bs.set(0);
        bs.set(1);
        AZ_TEST_ASSERT(bs.count() == 2);

        bs.flip();
        AZ_TEST_ASSERT(bs.count() == 23);

        bs.flip(0);
        AZ_TEST_ASSERT(bs.count() == 24);

        str = bs.to_string<char>();
        AZ_TEST_ASSERT(str.length() == 25);

        AZ_TEST_ASSERT(bs != bs1);
        bs2 = bs;
        AZ_TEST_ASSERT(bs == bs2);

        bs1.reset();
        AZ_TEST_ASSERT(bs.any());
        AZ_TEST_ASSERT(!bs1.any());
        AZ_TEST_ASSERT(!bs.none());
        AZ_TEST_ASSERT(bs1.none());

        bs1 = bs >> 1;
        AZ_TEST_ASSERT(bs1.count() == 23);

        bs1 = bs << 2;
        AZ_TEST_ASSERT(bs1.count() == 22);

        // extensions
        bitset25_type bs3(string("10110"));
        AZ_TEST_ASSERT(bs3.num_words() == 1); // check number of words
        bitset25_type::word_t tempWord = *bs3.data(); // access the bits data
        AZ_TEST_ASSERT((tempWord & 0x16) == 0x16); // check values
        bitset25_type bs4;
        *bs4.data() = tempWord; // modify the data directly
        AZ_TEST_ASSERT(bs3 == bs4);
    }
} // end namespace UnitTest
