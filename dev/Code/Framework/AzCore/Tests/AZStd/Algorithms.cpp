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

#include <AzCore/std/utils.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/sort.h>

#include <AzCore/Memory/SystemAllocator.h>

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    class Algorithms
        : public AllocatorsFixture
    {
    };

    /**
     *
     */
    class AlgorithmsFindTest
        : public AllocatorsFixture
    {
    public:
        static bool SearchCompare(int i1, int i2) { return (i1 == i2); }

        static bool IsFive(int i) { return i == 5; }
        static bool IsNinetyNine(int i) { return i == 99; }
        static bool IsOne(int i) { return i == 1; }
        static bool IsLessThanTen(int i) { return i < 10; }

        template<class T>
        static void Find_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(find(iterable.begin(), iterable.end(), 5) != iterable.end());
            AZ_TEST_ASSERT(*find(iterable.begin(), iterable.end(), 5) == 5);
            AZ_TEST_ASSERT(find(iterable.begin(), iterable.end(), 99) == iterable.end());
        }

        template<class T>
        static void FindIf_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(find_if(iterable.begin(), iterable.end(), IsFive) != iterable.end());
            AZ_TEST_ASSERT(*find_if(iterable.begin(), iterable.end(), IsFive) == 5);
            AZ_TEST_ASSERT(find_if(iterable.begin(), iterable.end(), IsNinetyNine) == iterable.end());
        }

        template<class T>
        static void FindIfNot_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(find_if_not(iterable.begin(), iterable.end(), IsOne) != iterable.end());
            AZ_TEST_ASSERT(*find_if_not(iterable.begin(), iterable.end(), IsOne) == 5);
            AZ_TEST_ASSERT(find_if_not(iterable.begin(), iterable.end(), IsLessThanTen) == iterable.end());
        }

        void run()
        {
            // search (default compare)
            array<int, 9> searchArray = {
                {10, 20, 30, 40, 50, 60, 70, 80, 90}
            };
            array<int, 4> searchDefault = {
                {40, 50, 60, 70}
            };
            array<int, 9>::iterator it = AZStd::search(searchArray.begin(), searchArray.end(), searchDefault.begin(), searchDefault.end());
            AZ_TEST_ASSERT(it != searchArray.end());
            AZ_TEST_ASSERT(AZStd::distance(searchArray.begin(), it) == 3);

            array<int, 4> searchCompare = {
                {20, 30, 50}
            };
            it = AZStd::search(searchArray.begin(), searchArray.end(), searchCompare.begin(), searchCompare.end(), SearchCompare);
            AZ_TEST_ASSERT(it == searchArray.end());

            // find, find_if, find_if_not
            vector<int> emptyVector;
            array<int, 1> singleFiveArray = {
                {5}
            };
            array<int, 3> fiveAtStartArray = {
                {5, 1, 1}
            };
            array<int, 3> fiveInMiddleArray = {
                {1, 5, 1}
            };
            array<int, 3> fiveAtEndArray = {
                {1, 1, 5}
            };

            AZ_TEST_ASSERT(find(emptyVector.begin(), emptyVector.end(), 5) == emptyVector.end());
            Find_With_OneFiveRestOnes(singleFiveArray);
            Find_With_OneFiveRestOnes(fiveAtStartArray);
            Find_With_OneFiveRestOnes(fiveInMiddleArray);
            Find_With_OneFiveRestOnes(fiveAtEndArray);

            AZ_TEST_ASSERT(find_if(emptyVector.begin(), emptyVector.end(), IsFive) == emptyVector.end());
            FindIf_With_OneFiveRestOnes(singleFiveArray);
            FindIf_With_OneFiveRestOnes(fiveAtStartArray);
            FindIf_With_OneFiveRestOnes(fiveInMiddleArray);
            FindIf_With_OneFiveRestOnes(fiveAtEndArray);

            AZ_TEST_ASSERT(find_if_not(emptyVector.begin(), emptyVector.end(), IsFive) == emptyVector.end());
            FindIfNot_With_OneFiveRestOnes(singleFiveArray);
            FindIfNot_With_OneFiveRestOnes(fiveAtStartArray);
            FindIfNot_With_OneFiveRestOnes(fiveInMiddleArray);
            FindIfNot_With_OneFiveRestOnes(fiveAtEndArray);
        }
    };

    TEST_F(AlgorithmsFindTest, Test)
    {
        run();
    }

    /**
     *
     */
    static bool IsFive(int i) { return i == 5; }
    TEST_F(Algorithms, Compare)
    {
        vector<int> emptyVector;
        array<int, 3> allFivesArray = {
            {5, 5, 5}
        };
        array<int, 3> noFivesArray  = {
            {0, 1, 2}
        };
        array<int, 3> oneFiveArray  = {
            {4, 5, 6}
        };

        // all_of
        AZ_TEST_ASSERT(all_of(emptyVector.begin(), emptyVector.end(), IsFive) == true);
        AZ_TEST_ASSERT(all_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(all_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(all_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == false);

        // any_of
        AZ_TEST_ASSERT(any_of(emptyVector.begin(), emptyVector.end(), IsFive) == false);
        AZ_TEST_ASSERT(any_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(any_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(any_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == true);

        // none_of
        AZ_TEST_ASSERT(none_of(emptyVector.begin(), emptyVector.end(), IsFive) == true);
        AZ_TEST_ASSERT(none_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(none_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(none_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == false);
    }

    /**
     *
     */
    TEST_F(Algorithms, Heap)
    {
        using AZStd::size_t;

        array<int, 10> elementsSrc = {
            {10, 2, 6, 3, 5, 8, 7, 9, 1, 4}
        };

        array<int, 10> elements(elementsSrc);
        make_heap(elements.begin(), elements.end());
        sort_heap(elements.begin(), elements.end());
        for (size_t i = 1; i < elements.size(); ++i)
        {
            AZ_TEST_ASSERT(elements[i - 1] < elements[i]);
        }
        make_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        sort_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        for (size_t i = 1; i < elements.size(); ++i)
        {
            AZ_TEST_ASSERT(elements[i - 1] > elements[i]);
        }

#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        array<int, 3> assertHeap = {
            {1, 2, 3}
        };
        // we should call make heap before we can sort, push or pop
        AZ_TEST_START_ASSERTTEST;
        sort_heap(assertHeap.begin(), assertHeap.end());
        AZ_TEST_STOP_ASSERTTEST(2);
        assertHeap[0] = 1;
        assertHeap[1] = 2;
        assertHeap[2] = 3;
        AZ_TEST_START_ASSERTTEST;
        push_heap(assertHeap.begin(), assertHeap.end());
        AZ_TEST_STOP_ASSERTTEST(1);
        assertHeap[0] = 1;
        assertHeap[1] = 2;
        assertHeap[2] = 3;
        AZ_TEST_START_ASSERTTEST;
        pop_heap(assertHeap.begin(), assertHeap.end());
        AZ_TEST_STOP_ASSERTTEST(2);
#endif
    }

    TEST_F(Algorithms, InsertionSort)
    {
        array<int, 10> elementsSrc = {
            { 10, 2, 6, 3, 5, 8, 7, 9, 1, 4 }
        };

        // Insertion sort
        array<int, 10> elements1(elementsSrc);
        insertion_sort(elements1.begin(), elements1.end());
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_LT(elements1[i - 1], elements1[i]);
        }
        insertion_sort(elements1.begin(), elements1.end(), AZStd::greater<int>());
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_GT(elements1[i - 1], elements1[i]);
        }
    }

    TEST_F(Algorithms, InsertionSort_SharedPtr)
    {
        array<shared_ptr<int>, 10> elementsSrc = {
        { 
            make_shared<int>(10), make_shared<int>(2), make_shared<int>(6), make_shared<int>(3), 
            make_shared<int>(5), make_shared<int>(8), make_shared<int>(7), make_shared<int>(9), 
            make_shared<int>(1), make_shared<int>(4) }
        };

        // Insertion sort
        auto compareLesser = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
        {
            return *lhs < *rhs;
        };
        array<shared_ptr<int>, 10> elements1(elementsSrc);
        insertion_sort(elements1.begin(), elements1.end(), compareLesser);
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_LT(*elements1[i - 1], *elements1[i]);
        }

        auto compareGreater = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
        {
            return *lhs > *rhs;
        };
        insertion_sort(elements1.begin(), elements1.end(), compareGreater);
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_GT(*elements1[i - 1], *elements1[i]);
        }
    }

    TEST_F(Algorithms, Sort)
    {
        vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // Normal sort test
            sort(sortTest.begin(), sortTest.end());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            sort(sortTest.begin(), sortTest.end(), AZStd::greater<int>());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, Sort_SharedPtr)
    {
        vector<shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize - 1; i >= 0; --i)
            {
                sortTest.push_back(make_shared<int>(i));
            }

            // Normal sort test
            auto compareLesser = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            sort(sortTest.begin(), sortTest.end(), compareLesser);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            sort(sortTest.begin(), sortTest.end(), compareGreater);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, StableSort)
    {
        vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // Stable sort test
            stable_sort(sortTest.begin(), sortTest.end(), sortTest.get_allocator());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            stable_sort(sortTest.begin(), sortTest.end(), AZStd::greater<int>(), sortTest.get_allocator());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, StableSort_SharedPtr)
    {
        vector<shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(make_shared<int>(i));
            }

            // Stable sort test
            auto compareLesser = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            stable_sort(sortTest.begin(), sortTest.end(), compareLesser, sortTest.get_allocator());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            stable_sort(sortTest.begin(), sortTest.end(), compareGreater, sortTest.get_allocator());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, DISABLED_StableSort_AlreadySorted)
    {
        AZStd::vector<uint64_t> testVec;
        for (int i = 0; i < 33; ++i)
        {
            testVec.push_back(i);
        }

        auto expectedResultVec = testVec;
        AZStd::stable_sort(testVec.begin(), testVec.end(), AZStd::less<uint64_t>(), testVec.get_allocator());
        EXPECT_EQ(expectedResultVec, testVec);
    }

    TEST_F(Algorithms, PartialSort)
    {
        vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // partial_sort test
            int sortSize = vectorSize / 2;
            partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end());
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), AZStd::greater<int>());
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, PartialSort_SharedPtr)
    {
        vector<shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize - 1; i >= 0; --i)
            {
                sortTest.push_back(make_shared<int>(i));
            }

            // partial_sort test
            auto compareLesser = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            int sortSize = vectorSize / 2;
            partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), compareLesser);
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const shared_ptr<int>& lhs, const shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), compareGreater);
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    /**
     * Endian swap test.
     */
    TEST_F(Algorithms, EndianSwap)
    {
        array<char, 10> charArr  = {
            {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        };
        array<short, 10> shortArr = {
            {0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05, 0x0f06, 0x0f07, 0x0f08, 0x0f09, 0x0f0a}
        };
        array<int, 10> intArr = {
            {0x0d0e0f01, 0x0d0e0f02, 0x0d0e0f03, 0x0d0e0f04, 0x0d0e0f05, 0x0d0e0f06, 0x0d0e0f07, 0x0d0e0f08, 0x0d0e0f09, 0x0d0e0f0a}
        };
        array<AZ::s64, 10> int64Arr = {
            {AZ_INT64_CONST(0x090a0b0c0d0e0f01), AZ_INT64_CONST(0x090a0b0c0d0e0f02), AZ_INT64_CONST(0x090a0b0c0d0e0f03), AZ_INT64_CONST(0x090a0b0c0d0e0f04),
                AZ_INT64_CONST(0x090a0b0c0d0e0f05), AZ_INT64_CONST(0x090a0b0c0d0e0f06), AZ_INT64_CONST(0x090a0b0c0d0e0f07),
                AZ_INT64_CONST(0x090a0b0c0d0e0f08), AZ_INT64_CONST(0x090a0b0c0d0e0f09), AZ_INT64_CONST(0x090a0b0c0d0e0f0a)}
        };

        endian_swap(charArr.begin(), charArr.end());
        for (char i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(charArr[i - 1] == i);
        }

        endian_swap(shortArr.begin(), shortArr.end());
        for (short i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(shortArr[i - 1] == (i << 8 | 0x000f));
        }

        endian_swap(intArr.begin(), intArr.end());
        for (int i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(intArr[i - 1] == (i << 24 | 0x000f0e0d));
        }

        endian_swap(int64Arr.begin(), int64Arr.end());
        for (AZ::s64 i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(int64Arr[static_cast<AZStd::size_t>(i - 1)] == (i << 56 | AZ_INT64_CONST(0x000f0e0d0c0b0a09)));
        }
    }

    TEST_F(Algorithms, CopyBackwardFastCopy)
    {
        array<int, 3> src = {{ 1, 2, 3 }};
        array<int, 3> dest;

        AZStd::copy_backward(src.begin(), src.end(), dest.end());
        
        EXPECT_EQ(1, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(3, dest[2]);
    }

    TEST_F(Algorithms, CopyBackwardStandardCopy)
    {
        // List is not contiguous, and is therefore unable to perform a fast copy
        list<int> src = { 1, 2, 3 };
        array<int, 3> dest;

        AZStd::copy_backward(src.begin(), src.end(), dest.end());
        
        EXPECT_EQ(1, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(3, dest[2]);
    }

    TEST_F(Algorithms, ReverseCopy)
    {
        array<int, 3> src = {{ 1, 2, 3 }};
        array<int, 3> dest;

        AZStd::reverse_copy(src.begin(), src.end(), dest.begin());
        EXPECT_EQ(3, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(1, dest[2]);
    }

    TEST_F(Algorithms, MinMaxElement)
    {
        AZStd::initializer_list<uint32_t> emptyData{};
        AZStd::array<uint32_t, 1> singleElementData{ { 313 } };
        AZStd::array<uint32_t, 3> unorderedData{ { 5, 3, 2} };
        AZStd::array<uint32_t, 5> multiMinMaxData{ { 5, 3, 2, 5, 2 } };

        // Empty container test
        {
            AZStd::pair<const uint32_t*, const uint32_t*> minMaxPair = AZStd::minmax_element(emptyData.begin(), emptyData.end());
            EXPECT_EQ(minMaxPair.first, minMaxPair.second);
            EXPECT_EQ(emptyData.end(), minMaxPair.first);
        }

        {
            // Single element container test
            AZStd::pair<uint32_t*, uint32_t*> minMaxPair = AZStd::minmax_element(singleElementData.begin(), singleElementData.end());
            EXPECT_EQ(minMaxPair.first, minMaxPair.second);
            EXPECT_NE(singleElementData.end(), minMaxPair.second);
            EXPECT_EQ(313, *minMaxPair.first);
            EXPECT_EQ(313, *minMaxPair.second);

            // Unordered container test
            minMaxPair = AZStd::minmax_element(unorderedData.begin(), unorderedData.end());
            EXPECT_NE(unorderedData.end(), minMaxPair.first);
            EXPECT_NE(unorderedData.end(), minMaxPair.second);
            EXPECT_EQ(2, *minMaxPair.first);
            EXPECT_EQ(5, *minMaxPair.second);

            // Multiple min and max elements in same container test
            minMaxPair = AZStd::minmax_element(multiMinMaxData.begin(), multiMinMaxData.end());
            EXPECT_NE(multiMinMaxData.end(), minMaxPair.first);
            EXPECT_NE(multiMinMaxData.end(), minMaxPair.second);
            // The smallest element should correspond to the first '2' within the multiMinMaxData container
            EXPECT_EQ(multiMinMaxData.begin() + 2, minMaxPair.first);
            // The greatest element should correspond to the second '5' within the multiMinMaxData container
            EXPECT_EQ(multiMinMaxData.begin() + 3, minMaxPair.second);
            EXPECT_EQ(2, *minMaxPair.first);
            EXPECT_EQ(5, *minMaxPair.second);

            // Custom comparator test
            minMaxPair = AZStd::minmax_element(unorderedData.begin(), unorderedData.end(), AZStd::greater<uint32_t>());
            EXPECT_NE(unorderedData.end(), minMaxPair.first);
            EXPECT_NE(unorderedData.end(), minMaxPair.second);
            EXPECT_EQ(5, *minMaxPair.first);
            EXPECT_EQ(2, *minMaxPair.second);
        }
    }

    TEST_F(Algorithms, MinMax)
    {
        AZStd::initializer_list<uint32_t> singleElementData{ 908 };
        AZStd::initializer_list<uint32_t> unorderedData{ 5, 3, 2 };
        AZStd::initializer_list<uint32_t> multiMinMaxData{ 7, 10, 552, 57234, 224, 57234, 7, 238 };

        // Initializer list test
        {
            // Single element container test
            AZStd::pair<uint32_t, uint32_t> minMaxPair = AZStd::minmax(singleElementData);
            EXPECT_EQ(908, minMaxPair.first);
            EXPECT_EQ(908, minMaxPair.second);

            // Unordered container test
            minMaxPair = AZStd::minmax(unorderedData);
            EXPECT_EQ(2, minMaxPair.first);
            EXPECT_EQ(5, minMaxPair.second);

            // Multiple min and max elements in same container test
            minMaxPair = AZStd::minmax(multiMinMaxData);
            EXPECT_EQ(7, minMaxPair.first);
            EXPECT_EQ(57234, minMaxPair.second);

            // Custom comparator test
            minMaxPair = AZStd::minmax(unorderedData, AZStd::greater<uint32_t>());
            EXPECT_EQ(5, minMaxPair.first);
            EXPECT_EQ(2, minMaxPair.second);
        }

        // Two parameter test
        {
            // Sanity test
            AZStd::pair<uint32_t, uint32_t> minMaxPair = AZStd::minmax(7000, 6999);
            EXPECT_EQ(6999, minMaxPair.first);
            EXPECT_EQ(7000, minMaxPair.second);

            // Customer comparator Test
            minMaxPair = AZStd::minmax(9001, 9000, AZStd::greater<uint32_t>());
            EXPECT_EQ(9001, minMaxPair.first);
            EXPECT_EQ(9000, minMaxPair.second);
        }
    }
}
