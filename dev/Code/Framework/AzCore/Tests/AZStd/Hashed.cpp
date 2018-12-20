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
#include "UserTypes.h"
#include <AzCore/std/hash_table.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/std/string/string.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif // HAVE_BENCHMARK

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    AZ_HAS_MEMBER(HashValidate, validate, void, ());

    /**
     * Hash functions test.
     */
    class HashedContainers
        : public AllocatorsFixture
    {
    public:
        template <class H, bool hasValidate>
        struct Validator
        {
            static void Validate(H& h)
            {
                EXPECT_TRUE(h.validate());
            }
        };

        template <class H>
        struct Validator<H, false>
        {
            static void Validate(H&) {}
        };

        template <class H>
        static void ValidateHash(H& h, size_t numElements = 0)
        {
            Validator<H, HasHashValidate<H>::value>::Validate(h);
            EXPECT_EQ(numElements, h.size());
            if (numElements > 0)
            {
                EXPECT_FALSE(h.empty());
                EXPECT_TRUE(h.begin() != h.end());
            }
            else
            {
                EXPECT_TRUE(h.empty());
                EXPECT_TRUE(h.begin() == h.end());
            }
        }
    };

    TEST_F(HashedContainers, Test)
    {
        EXPECT_EQ(1, hash<bool>()(true));
        EXPECT_EQ(0, hash<bool>()(false));

        char ch = 11;
        EXPECT_EQ(11, hash<char>()(ch));

        signed char sc = 127;
        EXPECT_EQ(127, hash<signed char>()(sc));

        unsigned char uc = 202;
        EXPECT_EQ(202, hash<unsigned char>()(uc));

        short sh = 16533;
        EXPECT_EQ(16533, hash<short>()(sh));

        signed short ss = 16333;
        EXPECT_EQ(16333, hash<signed short>()(ss));

        unsigned short us = 24533;
        EXPECT_EQ(24533, hash<unsigned short>()(us));

        int in = 5748538;
        EXPECT_EQ(5748538, hash<int>()(in));

        unsigned int ui = 0x0fffffff;
        EXPECT_EQ(0x0fffffff, hash<unsigned int>()(ui));

        AZ_TEST_ASSERT(hash<float>()(103.0f) != 0);
        EXPECT_EQ(hash<float>()(103.0f), hash<float>()(103.0f));
        AZ_TEST_ASSERT(hash<float>()(103.0f) != hash<float>()(103.1f));

        AZ_TEST_ASSERT(hash<double>()(103.0) != 0);
        EXPECT_EQ(hash<double>()(103.0), hash<double>()(103.0));
        AZ_TEST_ASSERT(hash<double>()(103.0) != hash<double>()(103.1));

        const char* string_literal = "Bla";
        const char* string_literal1 = "Cla";
        AZ_TEST_ASSERT(hash<const char*>()(string_literal) != 0);
        AZ_TEST_ASSERT(hash<const char*>()(string_literal) != hash<const char*>()(string_literal1));

        string str1(string_literal);
        string str2(string_literal1);
        AZ_TEST_ASSERT(hash<string>()(str1) != 0);
        AZ_TEST_ASSERT(hash<string>()(str1) != hash<string>()(str2));

        wstring wstr1(L"BLA");
        wstring wstr2(L"CLA");
        AZ_TEST_ASSERT(hash<wstring>()(wstr1) != 0);
        AZ_TEST_ASSERT(hash<wstring>()(wstr1) != hash<wstring>()(wstr2));
    }

    /**
     * HashTableSetTraits
     */
    // HashTableTest-Begin
    template<class T, bool isMultiSet, bool isDynamic>
    struct HashTableSetTestTraits
    {
        typedef T                               key_type;
        typedef typename AZStd::equal_to<T>     key_eq;
        typedef typename AZStd::hash<T>         hasher;
        typedef T                               value_type;
        typedef AZStd::allocator                allocator_type;
        enum
        {
            max_load_factor = 4,
            min_buckets = 7,
            has_multi_elements = isMultiSet,
            is_dynamic = isDynamic,
            fixed_num_buckets = 101, // a prime number between 1/3 - 1/4 of the number of elements.
            fixed_num_elements = 300,
        };
        static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
    };

    TEST_F(HashedContainers, HashTable_Dynamic)
    {
        array<int, 5> elements = {
            { 10, 100, 11, 2, 301 }
        };

        typedef HashTableSetTestTraits< int, false, true > hash_set_traits;
        typedef hash_table< hash_set_traits >   hash_key_table_type;
        hash_set_traits::hasher set_hasher;

        //
        hash_key_table_type hTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        ValidateHash(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        ValidateHash(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        ValidateHash(hTable1, hTable1.size());
        //
        hTable = hTable1;
        ValidateHash(hTable, hTable1.size());

        //AZ_TEST_ASSERT(hTable.bucket_count()==hash_set_traits::min_buckets);
        EXPECT_EQ((float)hash_set_traits::max_load_factor, hTable.max_load_factor());

        hTable.reserve(50);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() * hTable.max_load_factor() >= 50);
        AZ_TEST_ASSERT(hTable.bucket_count() < 100); // Make sure it doesn't interfere with the next test

        hTable.rehash(100);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        ValidateHash(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        ValidateHash(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        ValidateHash(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        ValidateHash(hTable, toInsert.size());

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        ValidateHash(hTable, toInsert.size() - 1);
        hTable.erase(18);
        ValidateHash(hTable, toInsert.size() - 1);

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        EXPECT_EQ(1, hTable.count(101));
        EXPECT_EQ(0, hTable.count(201));

        iter = hTable.lower_bound(17);
        AZ_TEST_ASSERT(iter != hTable.end());
        AZ_TEST_ASSERT(*prior(iter) != 17);
        citer = hTable.lower_bound(17);
        AZ_TEST_ASSERT(citer != hTable.end());
        AZ_TEST_ASSERT(*prior(citer) != 17);

        iter = hTable.lower_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        iter = hTable.upper_bound(101);
        AZ_TEST_ASSERT(iter == hTable.end() || *iter != 101);
        EXPECT_EQ(101, *prior(iter));
        citer = hTable.upper_bound(101);
        AZ_TEST_ASSERT(citer == hTable.end() || *citer != 101);
        EXPECT_EQ(101, *prior(citer));

        iter = hTable.upper_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        hash_key_table_type::pair_iter_iter rangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(rangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(rangeIt.first) == rangeIt.second);

        hash_key_table_type::pair_citer_citer crangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(crangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(crangeIt.first) == crangeIt.second);

        hTable1.clear();
        hTable.swap(hTable1);
        ValidateHash(hTable);
        ValidateHash(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, true > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        ValidateHash(hMultiTable, toInsert2.size());

        EXPECT_EQ(3, hMultiTable.count(101));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prior(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prior(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        EXPECT_EQ(3, distance(rangeIt2.first, rangeIt2.second));
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            EXPECT_EQ(101, *iter2);
        }
    }

    TEST_F(HashedContainers, HashTable_Fixed)
    {
        array<int, 5> elements = {
            { 10, 100, 11, 2, 301 }
        };

        typedef HashTableSetTestTraits< int, false, false > hash_set_traits;
        typedef hash_table< hash_set_traits >   hash_key_table_type;
        hash_set_traits::hasher set_hasher;

        //
        hash_key_table_type hTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        ValidateHash(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        ValidateHash(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        ValidateHash(hTable1, hTable1.size());
        //
        hTable = hTable1;
        ValidateHash(hTable, hTable1.size());

        hTable.rehash(100);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        ValidateHash(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        ValidateHash(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        ValidateHash(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        ValidateHash(hTable, toInsert.size());

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        ValidateHash(hTable, toInsert.size() - 1);
        hTable.erase(18);
        ValidateHash(hTable, toInsert.size() - 1);

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        EXPECT_EQ(1, hTable.count(101));
        EXPECT_EQ(0, hTable.count(201));

        iter = hTable.lower_bound(17);
        AZ_TEST_ASSERT(iter != hTable.end());
        citer = hTable.lower_bound(17);
        AZ_TEST_ASSERT(citer != hTable.end());

        iter = hTable.lower_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        iter = hTable.upper_bound(101);
        AZ_TEST_ASSERT(iter != hTable.end());
        citer = hTable.upper_bound(101);
        AZ_TEST_ASSERT(citer != hTable.end());

        iter = hTable.upper_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        hash_key_table_type::pair_iter_iter rangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(rangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(rangeIt.first) == rangeIt.second);

        hash_key_table_type::pair_citer_citer crangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(crangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(crangeIt.first) == crangeIt.second);

        hTable1.clear();
        hTable.swap(hTable1);
        ValidateHash(hTable);
        ValidateHash(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, false > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        ValidateHash(hMultiTable, toInsert2.size());

        EXPECT_EQ(3, hMultiTable.count(101));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prior(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prior(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        EXPECT_EQ(3, distance(rangeIt2.first, rangeIt2.second));
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            EXPECT_EQ(101, *iter2);
        }
    }

    TEST_F(HashedContainers, UnorderedSetTest)
    {
        typedef unordered_set<int> int_set_type;

        int_set_type::hasher myHasher;
        int_set_type::key_eq myKeyEqCmp;
        int_set_type::allocator_type myAllocator;

        int_set_type int_set;
        ValidateHash(int_set);

        int_set_type int_set1(100);
        ValidateHash(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_set_type int_set2(100, myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_set_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_set_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_set_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_set_type int_set8({ 1, 2, 3, 4, 4, 5 });
        ValidateHash(int_set8, 5);
        EXPECT_EQ(1, int_set8.count(1));
        EXPECT_EQ(1, int_set8.count(2));
        EXPECT_EQ(1, int_set8.count(3));
        EXPECT_EQ(1, int_set8.count(4));
        EXPECT_EQ(1, int_set8.count(5));

        int_set_type int_set9 = int_set8;
        EXPECT_EQ(int_set9, int_set8);
        EXPECT_TRUE(int_set1 != int_set9);

        int_set_type int_set10;
        int_set10.insert({ 1, 2, 3, 4, 4, 5 });
        EXPECT_EQ(int_set8, int_set10);
#endif
    }

    TEST_F(HashedContainers, FixedUnorderedSetTest)
    {
        typedef fixed_unordered_set<int, 101, 300> int_set_type;

        int_set_type::hasher myHasher;
        int_set_type::key_eq myKeyEqCmp;

        int_set_type int_set;
        ValidateHash(int_set);
        EXPECT_EQ(101, int_set.bucket_count());

        int_set_type int_set1(myHasher, myKeyEqCmp);
        ValidateHash(int_set1);
        EXPECT_EQ(101, int_set1.bucket_count());

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        EXPECT_EQ(101, int_set6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMultiSet)
    {
        typedef unordered_multiset<int> int_multiset_type;

        int_multiset_type::hasher myHasher;
        int_multiset_type::key_eq myKeyEqCmp;
        int_multiset_type::allocator_type myAllocator;

        int_multiset_type int_set;
        ValidateHash(int_set);

        int_multiset_type int_set1(100);
        ValidateHash(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_multiset_type int_set2(100, myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_multiset_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_multiset_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_multiset_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_multiset_type int_set8({ 1, 2, 3, 4, 4, 5 });
        ValidateHash(int_set8, 6);
        EXPECT_EQ(1, int_set8.count(1));
        EXPECT_EQ(1, int_set8.count(2));
        EXPECT_EQ(1, int_set8.count(3));
        EXPECT_EQ(2, int_set8.count(4));
        EXPECT_EQ(1, int_set8.count(5));

        int_multiset_type int_set9 = int_set8;
        AZ_TEST_ASSERT(int_set8 == int_set9);
        AZ_TEST_ASSERT(int_set1 != int_set9);

        int_multiset_type int_set10;
        int_set10.insert({ 1, 2, 3, 4, 4, 5 });
        EXPECT_EQ(int_set8, int_set10);
#endif
    }

    TEST_F(HashedContainers, FixedUnorderedMultiSet)
    {
        typedef fixed_unordered_multiset<int, 101, 300> int_multiset_type;

        int_multiset_type::hasher myHasher;
        int_multiset_type::key_eq myKeyEqCmp;

        int_multiset_type int_set;
        ValidateHash(int_set);
        EXPECT_EQ(101, int_set.bucket_count());

        int_multiset_type int_set2(myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        EXPECT_EQ(101, int_set2.bucket_count());

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());
        EXPECT_EQ(101, int_set4.bucket_count());

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        EXPECT_EQ(101, int_set6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMapBasic)
    {
        typedef unordered_map<int, int> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_eq myKeyEqCmp;
        int_int_map_type::allocator_type myAllocator;

        int_int_map_type intint_map;
        ValidateHash(intint_map);

        int_int_map_type intint_map1(100);
        ValidateHash(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_map_type intint_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_map_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        ValidateHash(intint_map, 1);
        EXPECT_EQ(16, intint_map.begin()->first);

        intint_map.insert(AZStd::make_pair(22, 11));
        ValidateHash(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        EXPECT_EQ(11, mappedValue);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        EXPECT_EQ(100, mappedValueNew);
        EXPECT_EQ(100, intint_map.at(33));

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intint_map4, uniqueArray.size());

        int_int_map_type intint_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(intint_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_map_type intint_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_int_map_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        ValidateHash(intint_map8, 5);
        EXPECT_EQ(10, intint_map8[1]);
        EXPECT_EQ(200, intint_map8[2]);
        EXPECT_EQ(3000, intint_map8[3]);
        EXPECT_EQ(40000, intint_map8[4]);
        EXPECT_EQ(500000, intint_map8[5]);
        EXPECT_EQ(1, intint_map8.find(1)->first);
        EXPECT_EQ(2, intint_map8.find(2)->first);
        EXPECT_EQ(3, intint_map8.find(3)->first);
        EXPECT_EQ(4, intint_map8.find(4)->first);
        EXPECT_EQ(5, intint_map8.find(5)->first);
        EXPECT_EQ(10, intint_map8.find(1)->second);
        EXPECT_EQ(200, intint_map8.find(2)->second);
        EXPECT_EQ(3000, intint_map8.find(3)->second);
        EXPECT_EQ(40000, intint_map8.find(4)->second);
        EXPECT_EQ(500000, intint_map8.find(5)->second);

        int_int_map_type intint_map9;
        intint_map9.insert({ { 1, 10 }, { 2, 200 }, { 3, 3000 }, { 4, 40000 }, { 4, 40001 }, { 5, 500000 } });
        EXPECT_EQ(intint_map8, intint_map9);
#endif
    }

    TEST_F(HashedContainers, UnorderedMapNoncopyableValue)
    {
        using nocopy_map = unordered_map<int, MyNoCopyClass>;

        nocopy_map ptr_map;
        nocopy_map::value_type test_pair;
        test_pair.second.m_bool = true;
        ptr_map.insert(AZStd::move(test_pair));

        for (const auto& pair : ptr_map)
        {
            AZ_TEST_ASSERT(pair.second.m_bool);
        }
    }

    TEST_F(HashedContainers, UnorderedMapNonTrivialValue)
    {
        typedef unordered_map<int, MyClass> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_eq myKeyEqCmp;
        int_myclass_map_type::allocator_type myAllocator;

        int_myclass_map_type intclass_map;
        ValidateHash(intclass_map);

        int_myclass_map_type intclass_map1(100);
        ValidateHash(intclass_map1);
        AZ_TEST_ASSERT(intclass_map1.bucket_count() >= 100);

        int_myclass_map_type intclass_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intclass_map2);
        AZ_TEST_ASSERT(intclass_map2.bucket_count() >= 100);

        int_myclass_map_type intclass_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intclass_map3);
        AZ_TEST_ASSERT(intclass_map3.bucket_count() >= 100);

        // insert with default value.
        intclass_map.insert_key(16);
        ValidateHash(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        ValidateHash(intclass_map, 2);

        // Emplace
        auto result = intclass_map.emplace(33, MyClass(37));
        AZ_TEST_ASSERT(result.second);
        EXPECT_EQ(37, result.first->second.m_data);
        ValidateHash(intclass_map, 3);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, intclass_map.at(33).m_data);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intclass_map4, uniqueArray.size());

        int_myclass_map_type intclass_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(intclass_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map5.bucket_count() >= 100);

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intclass_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map6.bucket_count() >= 100);

        int_myclass_map_type intclass_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intclass_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_myclass_map_type intclass_map8({
            { 1, MyClass(10) },{ 2, MyClass(200) },{ 3, MyClass(3000) },{ 4, MyClass(40000) },{ 4, MyClass(40001) },{ 5, MyClass(500000) }
        });
        ValidateHash(intclass_map8, 5);
        AZ_TEST_ASSERT(intclass_map8[1] == MyClass(10));
        AZ_TEST_ASSERT(intclass_map8[2] == MyClass(200));
        AZ_TEST_ASSERT(intclass_map8[3] == MyClass(3000));
        AZ_TEST_ASSERT(intclass_map8[4] == MyClass(40000));
        AZ_TEST_ASSERT(intclass_map8[5] == MyClass(500000));
        EXPECT_EQ(1, intclass_map8.find(1)->first);
        EXPECT_EQ(2, intclass_map8.find(2)->first);
        EXPECT_EQ(3, intclass_map8.find(3)->first);
        EXPECT_EQ(4, intclass_map8.find(4)->first);
        EXPECT_EQ(5, intclass_map8.find(5)->first);
        AZ_TEST_ASSERT(intclass_map8.find(1)->second == MyClass(10));
        AZ_TEST_ASSERT(intclass_map8.find(2)->second == MyClass(200));
        AZ_TEST_ASSERT(intclass_map8.find(3)->second == MyClass(3000));
        AZ_TEST_ASSERT(intclass_map8.find(4)->second == MyClass(40000));
        AZ_TEST_ASSERT(intclass_map8.find(5)->second == MyClass(500000));

        int_myclass_map_type intclass_map9;
        intclass_map9.insert({ { 1, MyClass(10) },{ 2, MyClass(200) },{ 3, MyClass(3000) },{ 4, MyClass(40000) },{ 4, MyClass(40001) },{ 5, MyClass(500000) } });
        EXPECT_EQ(intclass_map8, intclass_map9);
#endif
    }

    TEST_F(HashedContainers, UnorderedMapNonTrivialKey)
    {
        typedef unordered_map<AZStd::string, MyClass> string_myclass_map_type;

        string_myclass_map_type stringclass_map;
        ValidateHash(stringclass_map);

        // insert with default value.
        stringclass_map.insert_key("Hello");
        ValidateHash(stringclass_map, 1);

        stringclass_map.insert(AZStd::make_pair("World", MyClass(11)));
        ValidateHash(stringclass_map, 2);

        // Emplace
        auto result = stringclass_map.emplace("Goodbye", MyClass(37));
        AZ_TEST_ASSERT(result.second);
        EXPECT_EQ(37, result.first->second.m_data);
        ValidateHash(stringclass_map, 3);

        // map look up
        MyClass& mappedValue = stringclass_map["World"];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = stringclass_map["Goodbye"];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, stringclass_map.at("Goodbye").m_data);
    }

    TEST_F(HashedContainers, FixedUnorderedMapBasic)
    {
        typedef fixed_unordered_map<int, int, 101, 300> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_eq myKeyEqCmp;

        int_int_map_type intint_map;
        ValidateHash(intint_map);
        EXPECT_EQ(101, intint_map.bucket_count());

        int_int_map_type intint_map2(myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        EXPECT_EQ(101, intint_map2.bucket_count());

        // insert with default value.
        intint_map.insert_key(16);
        ValidateHash(intint_map, 1);
        EXPECT_EQ(16, intint_map.begin()->first);

        intint_map.insert(AZStd::make_pair(22, 11));
        ValidateHash(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        EXPECT_EQ(11, mappedValue);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        EXPECT_EQ(100, mappedValueNew);
        EXPECT_EQ(100, intint_map.at(33));

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intint_map4, uniqueArray.size());
        EXPECT_EQ(101, intint_map4.bucket_count());

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, uniqueArray.size());
        EXPECT_EQ(101, intint_map6.bucket_count());
    }

    TEST_F(HashedContainers, FixedUnorderedMapNonTrivialValue)
    {
        typedef fixed_unordered_map<int, MyClass, 101, 300> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_eq myKeyEqCmp;

        int_myclass_map_type intclass_map;
        ValidateHash(intclass_map);
        EXPECT_EQ(101, intclass_map.bucket_count());

        int_myclass_map_type intclass_map2(myHasher, myKeyEqCmp);
        ValidateHash(intclass_map2);
        EXPECT_EQ(101, intclass_map2.bucket_count());

        // insert with default value.
        intclass_map.insert_key(16);
        ValidateHash(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        ValidateHash(intclass_map, 2);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, intclass_map.at(33).m_data);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intclass_map4, uniqueArray.size());
        EXPECT_EQ(101, intclass_map4.bucket_count());

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        ValidateHash(intclass_map6, uniqueArray.size());
        EXPECT_EQ(101, intclass_map6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMultiMapBasic)
    {
        typedef unordered_multimap<int, int> int_int_multimap_type;

        int_int_multimap_type::hasher myHasher;
        int_int_multimap_type::key_eq myKeyEqCmp;
        int_int_multimap_type::allocator_type myAllocator;

        int_int_multimap_type intint_map;
        ValidateHash(intint_map);

        int_int_multimap_type intint_map1(100);
        ValidateHash(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_multimap_type intint_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_multimap_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        ValidateHash(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        ValidateHash(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        ValidateHash(intint_map4, multiArray.size());

        int_int_multimap_type intint_map5(multiArray.begin(), multiArray.end(), 100);
        ValidateHash(intint_map5, multiArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, multiArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_multimap_type intint_map7(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map7, multiArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_int_multimap_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        ValidateHash(intint_map8, 6);
        EXPECT_EQ(1, intint_map8.count(1));
        EXPECT_EQ(1, intint_map8.count(2));
        EXPECT_EQ(1, intint_map8.count(3));
        EXPECT_EQ(2, intint_map8.count(4));
        EXPECT_EQ(1, intint_map8.count(5));
        EXPECT_EQ(10, intint_map8.lower_bound(1)->second);
        EXPECT_EQ(200, intint_map8.lower_bound(2)->second);
        EXPECT_EQ(3000, intint_map8.lower_bound(3)->second);
        AZ_TEST_ASSERT(intint_map8.lower_bound(4)->second == 40000 || intint_map8.lower_bound(4)->second == 40001);
        AZ_TEST_ASSERT((++intint_map8.lower_bound(4))->second == 40000 || (++intint_map8.lower_bound(4))->second == 40001);
        AZ_TEST_ASSERT(intint_map8.lower_bound(5)->second == 500000);

        int_int_multimap_type intint_map9;
        intint_map9.insert({ { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 } });
        EXPECT_EQ(intint_map8, intint_map9);
#endif
    }

    TEST_F(HashedContainers, FixedUnorderedMultiMapBasic)
    {
        typedef fixed_unordered_multimap<int, int, 101, 300> int_int_multimap_type;

        int_int_multimap_type::hasher myHasher;
        int_int_multimap_type::key_eq myKeyEqCmp;

        int_int_multimap_type intint_map;
        ValidateHash(intint_map);
        EXPECT_EQ(101, intint_map.bucket_count());

        int_int_multimap_type intint_map2(myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        EXPECT_EQ(101, intint_map2.bucket_count());

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        ValidateHash(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        ValidateHash(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        ValidateHash(intint_map4, multiArray.size());
        EXPECT_EQ(101, intint_map4.bucket_count());

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, multiArray.size());
        EXPECT_EQ(101, intint_map6.bucket_count());
    }

    struct MoveOnlyType
    {
        MoveOnlyType() = default;
        MoveOnlyType(const AZStd::string name)
            : m_name(name)
        {}
        MoveOnlyType(const MoveOnlyType&) = delete;
        MoveOnlyType(MoveOnlyType&& other)
            : m_name(AZStd::move(other.m_name))
        {
        }

        MoveOnlyType& operator=(const MoveOnlyType&) = delete;
        MoveOnlyType& operator=(MoveOnlyType&& other)
        {
            m_name = AZStd::move(other.m_name);
            return *this;
        }

        bool operator==(const MoveOnlyType& other) const
        {
            return m_name == other.m_name;
        }

        AZStd::string m_name;
    };

    struct MoveOnlyTypeHasher
    {
        size_t operator()(const MoveOnlyType& moveOnlyType) const
        {
            return AZStd::hash<AZStd::string>()(moveOnlyType.m_name);
        }
    };

    TEST_F(HashedContainers, UnorderedSetMoveOnlyKeyTest)
    {
        AZStd::unordered_set<MoveOnlyType, MoveOnlyTypeHasher> ownedStringSet;

        AZStd::string nonOwnedString1("Test String");
        ownedStringSet.emplace(nonOwnedString1);

        auto keyEqual = [](const AZStd::string& testString, const MoveOnlyType& key)
        {
            return testString == key.m_name;
        };
        
        auto entityIt = ownedStringSet.find_as(nonOwnedString1, AZStd::hash<AZStd::string>(), keyEqual);
        EXPECT_NE(ownedStringSet.end(), entityIt);
        EXPECT_EQ(nonOwnedString1, entityIt->m_name);

        AZStd::string nonOwnedString2("Hashed Value");
        entityIt = ownedStringSet.find_as(nonOwnedString2, AZStd::hash<AZStd::string>(), keyEqual);
        EXPECT_EQ(ownedStringSet.end(), entityIt);
    }

    TEST_F(HashedContainers, UnorderedSetEquals)
    {
        AZStd::unordered_set<AZStd::string> aset1 = { "PlayerBase", "DamageableBase", "PlacementObstructionBase", "PredatorBase" };
        AZStd::unordered_set<AZStd::string> aset2;
        aset2.insert("PredatorBase");
        aset2.insert("PlayerBase");
        aset2.insert("PlacementObstructionBase");
        aset2.insert("DamageableBase");
        EXPECT_EQ(aset1, aset2); 
    }

    TEST_F(HashedContainers, UnorderedMapIterateEmpty)
    {
        AZStd::unordered_map<int, MoveOnlyType> map;
        for (auto& item : map)
        {
            AZ_UNUSED(item);
            EXPECT_TRUE(false) << "Iteration should never have occurred on an empty unordered_map";
        }
    }

    TEST_F(HashedContainers, UnorderedMapIterateItems)
    {
        AZStd::unordered_map<int, int> map{
            {1, 2},
            {3, 4},
            {5, 6},
            {7, 8}
        };

        unsigned idx = 0;
        for (auto& item : map)
        {
            EXPECT_EQ(item.second, item.first + 1);
            ++idx;
        }

        EXPECT_EQ(idx, map.size());
    }
            
#if defined(HAVE_BENCHMARK)
    template <template <typename...> class Hash>
    void Benchmark_Lookup(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        for (int i = 0; i < count; ++i)
        {
            map.emplace(i, i);
        }
        int val = 0;
        while (state.KeepRunning())
        {
            val += map[rand() % count];
        }
    }

    template <template <typename...> class Hash>
    void Benchmark_Insert(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        int i = 0;
        while (state.KeepRunning())
        {
            i = (i == count) ? 0 : i + 1;
            map[i] = i;
        }
    }

    template <template <typename ...> class Hash>
    void Benchmark_Erase(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        for (int i = 0; i < count; ++i)
        {
            map.emplace(i, i);
        }
        while (state.KeepRunning())
        {
            int val = rand() % count;
            map.erase(val);
        }
    }

    template <template <typename ...> class Hash>
    void Benchmark_Thrash(benchmark::State& state)
    {
        const int choices = 100;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        size_t ops = 0;
        while (state.KeepRunning())
        {
            int val = rand() % choices;
            switch (rand() % 3)
            {
                case 0:
                    ops += map.emplace(val, val).second;
                    break;
                case 1:
                    ops += map.erase(val);
                    break;
                case 2:
                    ops += map.find(val) == map.end();
                    break;
            }
        }
    }

    void Benchmark_UnorderedMapLookup(benchmark::State& state)
    {
        Benchmark_Lookup<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapLookup);

    void Benchmark_UnorderedMapInsert(benchmark::State& state)
    {
        Benchmark_Insert<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapInsert);

    void Benchmark_UnorderedMapErase(benchmark::State& state)
    {
        Benchmark_Erase<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapErase);

    void Benchmark_UnorderedMapThrash(benchmark::State& state)
    {
        Benchmark_Thrash<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapThrash);
#endif
} // namespace UnitTest

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    const int kNumInsertions = 10000;
    const int kModuloForDuplicates = 10;

    struct A
    {
        int m_int = 0;
    };

    using UnorderedMap = AZStd::unordered_map<int, A>;
    using AZStd::make_pair;

    // BM_UnorderedMap_InsertUniqueViaXXX: benchmark filling a map with unique values
    static void BM_UnorderedMap_InsertUniqueViaInsert(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.insert(make_pair(mapKey, A{})).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaInsert);

    static void BM_UnorderedMap_InsertUniqueViaEmplace(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.emplace(mapKey, A()).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaEmplace);

    static void BM_UnorderedMap_InsertUniqueViaBracket(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map[mapKey];
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaBracket);

    // BM_UnorderedMap_InsertDuplicatesViaXXX: benchmark filling a map with keys that repeat
    static void BM_UnorderedMap_InsertDuplicatesViaInsert(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.insert(make_pair(mapKey % kModuloForDuplicates, A())).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaInsert);

    static void BM_UnorderedMap_InsertDuplicatesViaEmplace(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.emplace(mapKey % kModuloForDuplicates, A{}).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaEmplace);

    static void BM_UnorderedMap_InsertDuplicatesViaBracket(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map[mapKey % kModuloForDuplicates];
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaBracket);

} // namespace Benchmark
#endif // HAVE_BENCHMARK