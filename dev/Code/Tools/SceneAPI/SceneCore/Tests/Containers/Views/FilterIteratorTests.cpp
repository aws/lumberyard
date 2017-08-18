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

#define _SCL_SECURE_NO_WARNINGS

#include <AzTest/AzTest.h>

#include <AzCore/std/createdestroy.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Tests/Containers/Views/IteratorTestsBase.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                template<typename CollectionType>
                class FilterIteratorBasicTests
                    : public IteratorTypedTestsBase<CollectionType>
                {
                public:
                    FilterIteratorBasicTests()
                    {
                        MakeComparePredicate(0);
                    }
                    ~FilterIteratorBasicTests() override = default;

                    template<typename T>
                    void MakeComparePredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value >= compareValue;
                            };
                    }

                    template<typename T>
                    void MakeNotEqualPredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value != compareValue;
                            };
                    }

                    AZStd::function<bool(const BaseIteratorReference)> m_testPredicate;
                };

                TYPED_TEST_CASE_P(FilterIteratorBasicTests);

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Constructor_InputIsEmptyValidBaseIterator_NoCrash)
                {
                    FilterIterator<BaseIterator>
                    testIterator(m_testCollection.begin(), m_testCollection.end(),
                        m_testPredicate);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Constructor_MovesForwardBasedOnPredicate_ExpectSkipFirstEntryAndReturnSecond)
                {
                    MakeComparePredicate(1);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    lhsIterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    EXPECT_EQ(1, *lhsIterator);
                }

                // Increment operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPreIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue)
                {
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    ++iterator;

                    EXPECT_EQ(1, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPreIncrement_MoveOneSkippingOne_ReturnsTheThirdValue)
                {
                    MakeComparePredicate(1);

                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    ++iterator;

                    EXPECT_EQ(2, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPostIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue)
                {
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    iterator++;

                    EXPECT_EQ(1, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPostIncrement_MoveOneSkippingOne_ReturnsTheThirdValue)
                {
                    MakeComparePredicate(1);

                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    iterator++;

                    EXPECT_EQ(2, *iterator);
                }

                // Equals equals operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesAll_ReturnsFalse)
                {
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);
                    FilterIterator<BaseIterator>
                    lhsIterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(GetBaseIterator(1), m_testCollection.end(),
                        m_testPredicate);
                    EXPECT_NE(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesPart_ReturnsTrue)
                {
                    MakeComparePredicate(1);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    lhsIterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(GetBaseIterator(1), m_testCollection.end(),
                        m_testPredicate);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_SkippingAllEntriesMatchesWithEndIterator_FullySkippedIteratorIsSameAsEnd)
                {
                    MakeComparePredicate(3);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(m_testCollection.begin(), m_testCollection.end(),
                        m_testPredicate);
                    FilterIterator<BaseIterator>
                    endIterator(m_testCollection.end(), m_testCollection.end(),
                        m_testPredicate);
                    EXPECT_EQ(iterator, endIterator);
                }

                // Not equals operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorNotEquals_DifferentObjects_ReturnsTrue)
                {
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    FilterIterator<BaseIterator>
                    lhsIterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(GetBaseIterator(1), m_testCollection.end(),
                        m_testPredicate);

                    EXPECT_TRUE(lhsIterator != rhsIterator);
                }

                // Star operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorStar_GetValueByDereferencingIterator_ExpectFirstValueInArray)
                {
                    AddElement(m_testCollection, 0);

                    FilterIterator<BaseIterator>
                    lhsIterator(GetBaseIterator(0), m_testCollection.end(),
                        m_testPredicate);
                    EXPECT_EQ(0, *lhsIterator);
                }

                // Decrement operator
                template<typename Container, typename = typename AZStd::iterator_traits<typename Container::iterator>::iterator_category>
                struct OperatorPreDecrement
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, int expectedResult)
                    {
                        return expectedResult;
                    }
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, int expectedResult)
                    {
                        FilterIterator<Container::iterator>
                        iterator(test.GetBaseIterator(iteratorOffset), test.m_testCollection.begin(), test.m_testCollection.end(),
                            test.m_testPredicate);

                        --iterator;
                        return *iterator;
                    };
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::random_access_iterator_tag>
                    : public OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::continuous_random_access_iterator_tag>
                    : public OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container, typename = typename AZStd::iterator_traits<typename Container::iterator>::iterator_category>
                struct OperatorPostDecrement
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, int expectedResult)
                    {
                        return expectedResult;
                    }
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, int expectedResult)
                    {
                        FilterIterator<Container::iterator>
                        iterator(test.GetBaseIterator(iteratorOffset), test.m_testCollection.begin(), test.m_testCollection.end(),
                            test.m_testPredicate);

                        iterator--;
                        return *iterator;
                    };
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::random_access_iterator_tag>
                    : public OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::continuous_random_access_iterator_tag>
                    : public OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneUnfilteredElementDown_ReturnsTheFirstValue)
                {
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);
                    EXPECT_EQ(0, OperatorPreDecrement<CollectionType>()(*this, 1, 0));
                    EXPECT_EQ(0, OperatorPostDecrement<CollectionType>()(*this, 1, 0));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneFilteredElementDown_ReturnsTheFirstValue)
                {
                    MakeNotEqualPredicate(1);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);
                    EXPECT_EQ(0, OperatorPreDecrement<CollectionType>()(*this, 2, 0));
                    EXPECT_EQ(0, OperatorPostDecrement<CollectionType>()(*this, 2, 0));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveDownToLastFilteredElement_ExpectToStayOnCurrentElement)
                {
                    MakeNotEqualPredicate(0);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);
                    EXPECT_EQ(1, OperatorPreDecrement<CollectionType>()(*this, 1, 1));
                    EXPECT_EQ(1, OperatorPostDecrement<CollectionType>()(*this, 1, 1));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneUnfilteredElementDownFromEnd_ReturnsTheSecondValue)
                {
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(m_testCollection);
                    EXPECT_EQ(1, OperatorPreDecrement<CollectionType>()(*this, 2, 1));
                    EXPECT_EQ(1, OperatorPostDecrement<CollectionType>()(*this, 2, 1));
                }

                // Filtered Elements
                TYPED_TEST_P(FilterIteratorBasicTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements)
                {
                    MakeComparePredicate(2);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);
                    AddElement(m_testCollection, 3);
                    AZStd::unordered_set<int> expectedElements = {2, 3};
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(m_testCollection.begin(), m_testCollection.end(),
                            m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it) != expectedElements.end())
                        {
                            expectedElements.erase(it);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    MakeFilterView_InputIteratorNotStartsAtBegin_CorrectFilteredElements)
                {
                    MakeComparePredicate(2);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);
                    AddElement(m_testCollection, 3);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    AZStd::unordered_set<int> expectedElements = { 2, 3 };
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(GetBaseIterator(1), m_testCollection.end(),
                            m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it) != expectedElements.end())
                        {
                            expectedElements.erase(it);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                // Algorithms
                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_CopyFilteredContainer_ContainerIsCopiedWithoutFilteredValue)
                {
                    MakeNotEqualPredicate(1);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    View<FilterIterator<BaseIterator> > view = MakeFilterView(GetBaseIterator(0), m_testCollection.end(),
                            m_testPredicate);

                    AZStd::vector<int> target;
                    AZStd::copy(view.begin(), view.end(), AZStd::back_inserter(target));

                    ASSERT_EQ(2, target.size());
                    EXPECT_EQ(0, target[0]);
                    EXPECT_EQ(2, target[1]);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_FillFilteredContainer_AllValuesAreSetTo42ExceptFilteredValue)
                {
                    MakeNotEqualPredicate(1);
                    AddElement(m_testCollection, 0);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(m_testCollection);

                    View<FilterIterator<BaseIterator> > view = MakeFilterView(GetBaseIterator(0), m_testCollection.end(),
                            m_testPredicate);

                    AZStd::generate_n(view.begin(), 2, [](){ return 42; });

                    EXPECT_EQ(42, *GetBaseIterator(0));
                    EXPECT_EQ(1, *GetBaseIterator(1));
                    EXPECT_EQ(42, *GetBaseIterator(2));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_PartialSortCopyFilteredContainer_AllValuesLargerOrEqualThan10AreCopiedAndSorted)
                {
                    MakeComparePredicate(10);
                    AddElement(m_testCollection, 18);
                    AddElement(m_testCollection, 42);
                    AddElement(m_testCollection, 36);
                    AddElement(m_testCollection, 9);
                    AddElement(m_testCollection, 88);
                    AddElement(m_testCollection, 3);

                    AZStd::vector<int> results;
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(m_testCollection.begin(), m_testCollection.end(), m_testPredicate);

                    AZStd::copy(view.begin(), view.end(), AZStd::back_inserter(results));
                    for (auto val : results)
                    {
                        EXPECT_GE(val, 10);
                    }
                }

                REGISTER_TYPED_TEST_CASE_P(FilterIteratorBasicTests,
                    Constructor_InputIsEmptyValidBaseIterator_NoCrash,
                    OperatorStar_GetValueByDereferencingIterator_ExpectFirstValueInArray,
                    Constructor_MovesForwardBasedOnPredicate_ExpectSkipFirstEntryAndReturnSecond,
                    OperatorPreIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue,
                    OperatorPreIncrement_MoveOneSkippingOne_ReturnsTheThirdValue,
                    OperatorPostIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue,
                    OperatorPostIncrement_MoveOneSkippingOne_ReturnsTheThirdValue,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesAll_ReturnsFalse,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesPart_ReturnsTrue,
                    OperatorEqualsEquals_SkippingAllEntriesMatchesWithEndIterator_FullySkippedIteratorIsSameAsEnd,
                    OperatorNotEquals_DifferentObjects_ReturnsTrue,
                    OperatorDecrement_MoveOneUnfilteredElementDown_ReturnsTheFirstValue,
                    OperatorDecrement_MoveOneFilteredElementDown_ReturnsTheFirstValue,
                    OperatorDecrement_MoveDownToLastFilteredElement_ExpectToStayOnCurrentElement,
                    OperatorDecrement_MoveOneUnfilteredElementDownFromEnd_ReturnsTheSecondValue,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements,
                    MakeFilterView_InputIteratorNotStartsAtBegin_CorrectFilteredElements,
                    Algorithms_CopyFilteredContainer_ContainerIsCopiedWithoutFilteredValue,
                    Algorithms_FillFilteredContainer_AllValuesAreSetTo42ExceptFilteredValue,
                    Algorithms_PartialSortCopyFilteredContainer_AllValuesLargerOrEqualThan10AreCopiedAndSorted
                    );

                INSTANTIATE_TYPED_TEST_CASE_P(CommonTests,
                    FilterIteratorBasicTests,
                    BasicCollectionTypes);

                // Map iterator tests
                template<typename CollectionType>
                class FilterIteratorMapTests
                    : public IteratorTypedTestsBase<CollectionType>
                {
                public:
                    FilterIteratorMapTests()
                    {
                        MakeComparePredicate(0);
                    }
                    ~FilterIteratorMapTests() override = default;

                protected:
                    template<typename T>
                    void MakeComparePredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value.first >= compareValue;
                            };
                    }
                    AZStd::function<bool(const BaseIteratorReference)> m_testPredicate;
                };

                TYPED_TEST_CASE_P(FilterIteratorMapTests);

                TYPED_TEST_P(FilterIteratorMapTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements)
                {
                    MakeComparePredicate(2);
                    AddElement(m_testCollection, 1);
                    AddElement(m_testCollection, 2);
                    AddElement(m_testCollection, 3);
                    AZStd::unordered_set<int> expectedElements = { 2, 3 };
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(m_testCollection.begin(), m_testCollection.end(),
                            m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it.first) != expectedElements.end())
                        {
                            expectedElements.erase(it.first);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                REGISTER_TYPED_TEST_CASE_P(FilterIteratorMapTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements
                    );

                INSTANTIATE_TYPED_TEST_CASE_P(CommonTests,
                    FilterIteratorMapTests,
                    MapCollectionTypes);

                // Added as separate test to avoid having to write another set of specialized templates.
                TEST(FilterIteratorTests, Algorithms_ReverseFilteredContainer_ValuesReversedExceptValuesLargerThan50)
                {
                    AZStd::vector<int> values = { 18, 7, 62, 63, 14 };

                    View<FilterIterator<AZStd::vector<int>::iterator> > view = MakeFilterView(values.begin(), values.end(),
                            [](int value) -> bool
                            {
                                return value < 50;
                            });

                    AZStd::reverse(view.begin(), view.end());

                    EXPECT_EQ(values[0], 14);
                    EXPECT_EQ(values[1], 7);
                    EXPECT_EQ(values[2], 62);
                    EXPECT_EQ(values[3], 63);
                    EXPECT_EQ(values[4], 18);
                }
            }
        }
    }
}

#undef _SCL_SECURE_NO_WARNINGS