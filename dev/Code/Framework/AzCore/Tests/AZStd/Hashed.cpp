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

using namespace AZStd;
using namespace UnitTestInternal;

#define AZ_TEST_VALIDATE_EMPTY_HASHMAP(_Hashmap) \
    AZ_TEST_ASSERT(_Hashmap.validate());         \
    AZ_TEST_ASSERT(_Hashmap.size() == 0);        \
    AZ_TEST_ASSERT(_Hashmap.empty());            \
    AZ_TEST_ASSERT(_Hashmap.begin() == _Hashmap.end());

#define AZ_TEST_VALIDATE_HASHMAP(_Hashmap, _NumElements)                       \
    AZ_TEST_ASSERT(_Hashmap.validate());                                       \
    AZ_TEST_ASSERT(_Hashmap.size() == _NumElements);                           \
    AZ_TEST_ASSERT((_NumElements > 0) ? !_Hashmap.empty() : _Hashmap.empty()); \
    AZ_TEST_ASSERT((_NumElements > 0) ? _Hashmap.begin() != _Hashmap.end() : _Hashmap.begin() == _Hashmap.end());

namespace UnitTest
{
    /**
     * Hash functions test.
     */
    class HashedContainers
        : public AllocatorsFixture
    {
    };

    TEST_F(HashedContainers, Test)
    {
        AZ_TEST_ASSERT(hash<bool>()(true) == 1);
        AZ_TEST_ASSERT(hash<bool>()(false) == 0);

        char ch = 11;
        AZ_TEST_ASSERT(hash<char>()(ch) == 11);

        signed char sc = 127;
        AZ_TEST_ASSERT(hash<signed char>()(sc) == 127);

        unsigned char uc = 202;
        AZ_TEST_ASSERT(hash<unsigned char>()(uc) == 202);

        short sh = 16533;
        AZ_TEST_ASSERT(hash<short>()(sh) == 16533);

        signed short ss = 16333;
        AZ_TEST_ASSERT(hash<signed short>()(ss) == 16333);

        unsigned short us = 24533;
        AZ_TEST_ASSERT(hash<unsigned short>()(us) == 24533);

        int in = 5748538;
        AZ_TEST_ASSERT(hash<int>()(in) == 5748538);

        unsigned int ui = 0x0fffffff;
        AZ_TEST_ASSERT(hash<unsigned int>()(ui) == 0x0fffffff);

        AZ_TEST_ASSERT(hash<float>()(103.0f) != 0);
        AZ_TEST_ASSERT(hash<float>()(103.0f) == hash<float>()(103.0f));
        AZ_TEST_ASSERT(hash<float>()(103.0f) != hash<float>()(103.1f));

        AZ_TEST_ASSERT(hash<double>()(103.0) != 0);
        AZ_TEST_ASSERT(hash<double>()(103.0) == hash<double>()(103.0));
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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        AZ_TEST_VALIDATE_HASHMAP(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        AZ_TEST_VALIDATE_HASHMAP(hTable1, hTable1.size());
        //
        hTable = hTable1;
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size());

        //AZ_TEST_ASSERT(hTable.bucket_count()==hash_set_traits::min_buckets);
        AZ_TEST_ASSERT(hTable.max_load_factor() == (float)hash_set_traits::max_load_factor);

        hTable.rehash(100);
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size());

        hTable.clear();
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size() - 1);
        hTable.erase(18);
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size() - 1);

        hTable.clear();
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        AZ_TEST_ASSERT(hTable.count(101) == 1);
        AZ_TEST_ASSERT(hTable.count(201) == 0);

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
        AZ_TEST_ASSERT(*prior(iter) == 101);
        citer = hTable.upper_bound(101);
        AZ_TEST_ASSERT(citer == hTable.end() || *citer != 101);
        AZ_TEST_ASSERT(*prior(citer) == 101);

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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        AZ_TEST_VALIDATE_HASHMAP(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, true > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        AZ_TEST_VALIDATE_HASHMAP(hMultiTable, toInsert2.size());

        AZ_TEST_ASSERT(hMultiTable.count(101) == 3);
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prior(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prior(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        AZ_TEST_ASSERT(distance(rangeIt2.first, rangeIt2.second) == 3);
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            AZ_TEST_ASSERT(*iter2 == 101);
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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());
        AZ_TEST_VALIDATE_HASHMAP(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        AZ_TEST_VALIDATE_HASHMAP(hTable1, hTable1.size());
        //
        hTable = hTable1;
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size());

        hTable.rehash(100);
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        AZ_TEST_VALIDATE_HASHMAP(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size());

        hTable.clear();
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size() - 1);
        hTable.erase(18);
        AZ_TEST_VALIDATE_HASHMAP(hTable, toInsert.size() - 1);

        hTable.clear();
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        AZ_TEST_ASSERT(hTable.count(101) == 1);
        AZ_TEST_ASSERT(hTable.count(201) == 0);

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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(hTable);
        AZ_TEST_VALIDATE_HASHMAP(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, false > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_eq(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        AZ_TEST_VALIDATE_HASHMAP(hMultiTable, toInsert2.size());

        AZ_TEST_ASSERT(hMultiTable.count(101) == 3);
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prior(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prior(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        AZ_TEST_ASSERT(distance(rangeIt2.first, rangeIt2.second) == 3);
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            AZ_TEST_ASSERT(*iter2 == 101);
        }
    }

    TEST_F(HashedContainers, UnorderedSetTest)
    {
        typedef unordered_set<int> int_set_type;

        int_set_type::hasher myHasher;
        int_set_type::key_eq myKeyEqCmp;
        int_set_type::allocator_type myAllocator;

        int_set_type int_set;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set);

        int_set_type int_set1(100);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_set_type int_set2(100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_set_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(int_set4, uniqueArray.size());

        int_set_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        AZ_TEST_VALIDATE_HASHMAP(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_set_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_HASHMAP(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_set_type int_set8({ 1, 2, 3, 4, 4, 5 });
        AZ_TEST_VALIDATE_HASHMAP(int_set8, 5);
        AZ_TEST_ASSERT(int_set8.count(1) == 1);
        AZ_TEST_ASSERT(int_set8.count(2) == 1);
        AZ_TEST_ASSERT(int_set8.count(3) == 1);
        AZ_TEST_ASSERT(int_set8.count(4) == 1);
        AZ_TEST_ASSERT(int_set8.count(5) == 1);

        int_set_type int_set9 = int_set8;
        AZ_TEST_ASSERT(int_set8 == int_set9);
        AZ_TEST_ASSERT(int_set1 != int_set9);

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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set);
        AZ_TEST_ASSERT(int_set.bucket_count() == 101);

        int_set_type int_set1(myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() == 101);

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(int_set4, uniqueArray.size());

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() == 101);
    }

    TEST_F(HashedContainers, UnorderedMultiSet)
    {
        typedef unordered_multiset<int> int_multiset_type;

        int_multiset_type::hasher myHasher;
        int_multiset_type::key_eq myKeyEqCmp;
        int_multiset_type::allocator_type myAllocator;

        int_multiset_type int_set;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set);

        int_multiset_type int_set1(100);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_multiset_type int_set2(100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_multiset_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(int_set4, uniqueArray.size());

        int_multiset_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        AZ_TEST_VALIDATE_HASHMAP(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_multiset_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_HASHMAP(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_multiset_type int_set8({ 1, 2, 3, 4, 4, 5 });
        AZ_TEST_VALIDATE_HASHMAP(int_set8, 6);
        AZ_TEST_ASSERT(int_set8.count(1) == 1);
        AZ_TEST_ASSERT(int_set8.count(2) == 1);
        AZ_TEST_ASSERT(int_set8.count(3) == 1);
        AZ_TEST_ASSERT(int_set8.count(4) == 2);
        AZ_TEST_ASSERT(int_set8.count(5) == 1);

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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set);
        AZ_TEST_ASSERT(int_set.bucket_count() == 101);

        int_multiset_type int_set2(myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() == 101);

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(int_set4, uniqueArray.size());
        AZ_TEST_ASSERT(int_set4.bucket_count() == 101);

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() == 101);
    }

    TEST_F(HashedContainers, UnorderedMapBasic)
    {
        typedef unordered_map<int, int> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_eq myKeyEqCmp;
        int_int_map_type::allocator_type myAllocator;

        int_int_map_type intint_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map);

        int_int_map_type intint_map1(100);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_map_type intint_map2(100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_map_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 1);
        AZ_TEST_ASSERT(intint_map.begin()->first == 16);

        intint_map.insert(AZStd::make_pair(22, 11));
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        AZ_TEST_ASSERT(mappedValue == 11);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        AZ_TEST_ASSERT(mappedValueNew == 100);
        AZ_TEST_ASSERT(intint_map.at(33) == 100);

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intint_map4, uniqueArray.size());

        int_int_map_type intint_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        AZ_TEST_VALIDATE_HASHMAP(intint_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intint_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_map_type intint_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_HASHMAP(intint_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_int_map_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        AZ_TEST_VALIDATE_HASHMAP(intint_map8, 5);
        AZ_TEST_ASSERT(intint_map8[1] == 10);
        AZ_TEST_ASSERT(intint_map8[2] == 200);
        AZ_TEST_ASSERT(intint_map8[3] == 3000);
        AZ_TEST_ASSERT(intint_map8[4] == 40000);
        AZ_TEST_ASSERT(intint_map8[5] == 500000);
        AZ_TEST_ASSERT(intint_map8.find(1)->first == 1);
        AZ_TEST_ASSERT(intint_map8.find(2)->first == 2);
        AZ_TEST_ASSERT(intint_map8.find(3)->first == 3);
        AZ_TEST_ASSERT(intint_map8.find(4)->first == 4);
        AZ_TEST_ASSERT(intint_map8.find(5)->first == 5);
        AZ_TEST_ASSERT(intint_map8.find(1)->second == 10);
        AZ_TEST_ASSERT(intint_map8.find(2)->second == 200);
        AZ_TEST_ASSERT(intint_map8.find(3)->second == 3000);
        AZ_TEST_ASSERT(intint_map8.find(4)->second == 40000);
        AZ_TEST_ASSERT(intint_map8.find(5)->second == 500000);

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

    TEST_F(HashedContainers, UnorderdMapNonTrivialValue)
    {
        typedef unordered_map<int, MyClass> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_eq myKeyEqCmp;
        int_myclass_map_type::allocator_type myAllocator;

        int_myclass_map_type intclass_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map);

        int_myclass_map_type intclass_map1(100);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map1);
        AZ_TEST_ASSERT(intclass_map1.bucket_count() >= 100);

        int_myclass_map_type intclass_map2(100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map2);
        AZ_TEST_ASSERT(intclass_map2.bucket_count() >= 100);

        int_myclass_map_type intclass_map3(100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map3);
        AZ_TEST_ASSERT(intclass_map3.bucket_count() >= 100);

        // insert with default value.
        intclass_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        AZ_TEST_VALIDATE_HASHMAP(intclass_map, 2);

        // Emplace
        auto result = intclass_map.emplace(33, MyClass(37));
        AZ_TEST_ASSERT(result.second);
        AZ_TEST_ASSERT(result.first->second.m_data == 37);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map, 3);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        AZ_TEST_ASSERT(mappedValue.m_data == 11);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        AZ_TEST_ASSERT(mappedValueNew.m_data == 100);
        AZ_TEST_ASSERT(intclass_map.at(33).m_data == 100);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intclass_map4, uniqueArray.size());

        int_myclass_map_type intclass_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map5.bucket_count() >= 100);

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map6.bucket_count() >= 100);

        int_myclass_map_type intclass_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_myclass_map_type intclass_map8({
            { 1, MyClass(10) },{ 2, MyClass(200) },{ 3, MyClass(3000) },{ 4, MyClass(40000) },{ 4, MyClass(40001) },{ 5, MyClass(500000) }
        });
        AZ_TEST_VALIDATE_HASHMAP(intclass_map8, 5);
        AZ_TEST_ASSERT(intclass_map8[1] == MyClass(10));
        AZ_TEST_ASSERT(intclass_map8[2] == MyClass(200));
        AZ_TEST_ASSERT(intclass_map8[3] == MyClass(3000));
        AZ_TEST_ASSERT(intclass_map8[4] == MyClass(40000));
        AZ_TEST_ASSERT(intclass_map8[5] == MyClass(500000));
        AZ_TEST_ASSERT(intclass_map8.find(1)->first == 1);
        AZ_TEST_ASSERT(intclass_map8.find(2)->first == 2);
        AZ_TEST_ASSERT(intclass_map8.find(3)->first == 3);
        AZ_TEST_ASSERT(intclass_map8.find(4)->first == 4);
        AZ_TEST_ASSERT(intclass_map8.find(5)->first == 5);
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

    TEST_F(HashedContainers, UnorderdMapNonTrivialKey)
    {
        typedef unordered_map<AZStd::string, MyClass> string_myclass_map_type;

        string_myclass_map_type stringclass_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(stringclass_map);

        // insert with default value.
        stringclass_map.insert_key("Hello");
        AZ_TEST_VALIDATE_HASHMAP(stringclass_map, 1);

        stringclass_map.insert(AZStd::make_pair("World", MyClass(11)));
        AZ_TEST_VALIDATE_HASHMAP(stringclass_map, 2);

        // Emplace
        auto result = stringclass_map.emplace("Goodbye", MyClass(37));
        AZ_TEST_ASSERT(result.second);
        AZ_TEST_ASSERT(result.first->second.m_data == 37);
        AZ_TEST_VALIDATE_HASHMAP(stringclass_map, 3);

        // map look up
        MyClass& mappedValue = stringclass_map["World"];
        AZ_TEST_ASSERT(mappedValue.m_data == 11);
        MyClass& mappedValueNew = stringclass_map["Goodbye"];     // insert a new element
        mappedValueNew.m_data = 100;
        AZ_TEST_ASSERT(mappedValueNew.m_data == 100);
        AZ_TEST_ASSERT(stringclass_map.at("Goodbye").m_data == 100);
    }

    TEST_F(HashedContainers, FixedUnorderedMapBasic)
    {
        typedef fixed_unordered_map<int, int, 101, 300> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_eq myKeyEqCmp;

        int_int_map_type intint_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map);
        AZ_TEST_ASSERT(intint_map.bucket_count() == 101);

        int_int_map_type intint_map2(myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() == 101);

        // insert with default value.
        intint_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 1);
        AZ_TEST_ASSERT(intint_map.begin()->first == 16);

        intint_map.insert(AZStd::make_pair(22, 11));
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        AZ_TEST_ASSERT(mappedValue == 11);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        AZ_TEST_ASSERT(mappedValueNew == 100);
        AZ_TEST_ASSERT(intint_map.at(33) == 100);

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intint_map4, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map4.bucket_count() == 101);

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intint_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() == 101);
    }

    TEST_F(HashedContainers, FixedUnorderedMapNonTrivialValue)
    {
        typedef fixed_unordered_map<int, MyClass, 101, 300> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_eq myKeyEqCmp;

        int_myclass_map_type intclass_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map);
        AZ_TEST_ASSERT(intclass_map.bucket_count() == 101);

        int_myclass_map_type intclass_map2(myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intclass_map2);
        AZ_TEST_ASSERT(intclass_map2.bucket_count() == 101);

        // insert with default value.
        intclass_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        AZ_TEST_VALIDATE_HASHMAP(intclass_map, 2);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        AZ_TEST_ASSERT(mappedValue.m_data == 11);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        AZ_TEST_ASSERT(mappedValueNew.m_data == 100);
        AZ_TEST_ASSERT(intclass_map.at(33).m_data == 100);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intclass_map4, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map4.bucket_count() == 101);

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intclass_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map6.bucket_count() == 101);
    }

    TEST_F(HashedContainers, UnorderedMultiMapBasic)
    {
        typedef unordered_multimap<int, int> int_int_multimap_type;

        int_int_multimap_type::hasher myHasher;
        int_int_multimap_type::key_eq myKeyEqCmp;
        int_int_multimap_type::allocator_type myAllocator;

        int_int_multimap_type intint_map;
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map);

        int_int_multimap_type intint_map1(100);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_multimap_type intint_map2(100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_multimap_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intint_map4, multiArray.size());

        int_int_multimap_type intint_map5(multiArray.begin(), multiArray.end(), 100);
        AZ_TEST_VALIDATE_HASHMAP(intint_map5, multiArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intint_map6, multiArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_multimap_type intint_map7(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        AZ_TEST_VALIDATE_HASHMAP(intint_map7, multiArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_int_multimap_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        AZ_TEST_VALIDATE_HASHMAP(intint_map8, 6);
        AZ_TEST_ASSERT(intint_map8.count(1) == 1);
        AZ_TEST_ASSERT(intint_map8.count(2) == 1);
        AZ_TEST_ASSERT(intint_map8.count(3) == 1);
        AZ_TEST_ASSERT(intint_map8.count(4) == 2);
        AZ_TEST_ASSERT(intint_map8.count(5) == 1);
        AZ_TEST_ASSERT(intint_map8.lower_bound(1)->second == 10);
        AZ_TEST_ASSERT(intint_map8.lower_bound(2)->second == 200);
        AZ_TEST_ASSERT(intint_map8.lower_bound(3)->second == 3000);
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
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map);
        AZ_TEST_ASSERT(intint_map.bucket_count() == 101);

        int_int_multimap_type intint_map2(myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_EMPTY_HASHMAP(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() == 101);

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        AZ_TEST_VALIDATE_HASHMAP(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        AZ_TEST_VALIDATE_HASHMAP(intint_map4, multiArray.size());
        AZ_TEST_ASSERT(intint_map4.bucket_count() == 101);

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), myHasher, myKeyEqCmp);
        AZ_TEST_VALIDATE_HASHMAP(intint_map6, multiArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() == 101);
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

}

