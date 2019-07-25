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

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>

#include <AzCore/std/containers/intrusive_set.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>

using namespace AZStd;
using namespace UnitTestInternal;

#define AZ_TEST_VALIDATE_EMPTY_TREE(_Tree_) \
    EXPECT_EQ(0, _Tree_.size());     \
    EXPECT_TRUE(_Tree_.empty());         \
    EXPECT_TRUE(_Tree_.begin() == _Tree_.end());

#define AZ_TEST_VALIDATE_TREE(_Tree_, _NumElements)                                                    \
    EXPECT_EQ(_NumElements, _Tree_.size());                                                     \
    EXPECT_TRUE((_Tree_.size() > 0) ? !_Tree_.empty() : _Tree_.empty());                              \
    EXPECT_TRUE((_NumElements > 0) ? _Tree_.begin() != _Tree_.end() : _Tree_.begin() == _Tree_.end()); \
    EXPECT_FALSE(_Tree_.empty());

namespace UnitTest
{
    /**
     * Setup the red-black tree. To achieve fixed_rbtree all you need is to use \ref AZStd::static_pool_allocator with
     * the node (Internal::rb_tree_node<T>).
     */
    template<class T, class KeyEq = AZStd::less<T>, class Allocator = AZStd::allocator >
    struct RedBlackTree_SetTestTraits
    {
        typedef T                   key_type;
        typedef KeyEq               key_eq;
        typedef T                   value_type;
        typedef Allocator           allocator_type;
        static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
    };

    class Tree_RedBlack
        : public AllocatorsFixture
    {
    };
    
    TEST_F(Tree_RedBlack, Test)
    {
        array<int, 5> elements = {
            {1, 5, 8, 8, 4}
        };
        AZStd::size_t num;

        typedef rbtree<RedBlackTree_SetTestTraits<int> > rbtree_set_type;

        rbtree_set_type tree;
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        rbtree_set_type copy_tree = tree;
        AZ_TEST_VALIDATE_EMPTY_TREE(copy_tree);

        // insert unique element
        bool isInsert = tree.insert_unique(10).second;
        AZ_TEST_VALIDATE_TREE(tree, 1);
        AZ_TEST_ASSERT(isInsert == true);

        // insert unique element, which we already have.
        isInsert = tree.insert_unique(10).second;
        AZ_TEST_VALIDATE_TREE(tree, 1);
        AZ_TEST_ASSERT(isInsert == false);

        // insert equal element.
        tree.insert_equal(11);
        AZ_TEST_VALIDATE_TREE(tree, 2);

        // insert equal element, which we already have.
        tree.insert_equal(11);
        AZ_TEST_VALIDATE_TREE(tree, 3);

        tree.insert_unique(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 7);  /// there are 4 unique elements in the list.

        tree.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 12);

        AZ_TEST_ASSERT(*tree.begin() == 1);
        rbtree_set_type::iterator iterNext = tree.erase(tree.begin());  // we have 2 elements with value (1)
        AZ_TEST_ASSERT(*iterNext == 1);
        tree.erase(tree.begin());
        AZ_TEST_VALIDATE_TREE(tree, 10);
        AZ_TEST_ASSERT(*tree.begin() == 4);

        AZ_TEST_ASSERT(*prior(tree.end()) == 11);
        iterNext = tree.erase(prior(tree.end()));
        AZ_TEST_ASSERT(iterNext == tree.end());
        AZ_TEST_VALIDATE_TREE(tree, 9);
        AZ_TEST_ASSERT(*prior(tree.end()) == 11);  // we have 2 elements with value(11) at the end

        num = tree.erase(8);
        AZ_TEST_VALIDATE_TREE(tree, 6);
        AZ_TEST_ASSERT(num == 3);

        tree.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        tree.insert_equal(elements.begin(), elements.end());

        tree.erase(elements.begin(), elements.end());
        AZ_TEST_ASSERT(iterNext == tree.end());
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        tree.insert_equal(elements.begin(), elements.end());
        tree.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 10);

        rbtree_set_type::iterator iter, iter1;

        iter = tree.find(8);
        AZ_TEST_ASSERT(iter != tree.end());

        iter1 = tree.lower_bound(8);
        AZ_TEST_ASSERT(iter == iter1);

        iter1 = tree.upper_bound(8);
        AZ_TEST_ASSERT(iter1 == tree.end());
        AZ_TEST_ASSERT(iter1 != iter);

        num = tree.count(8);
        AZ_TEST_ASSERT(num == 4);

        AZStd::pair<rbtree_set_type::iterator, rbtree_set_type::iterator> range;
        range = tree.equal_range(8);
        AZ_TEST_ASSERT(range.first != tree.end());
        AZ_TEST_ASSERT(range.second == tree.end());
        AZ_TEST_ASSERT(AZStd::distance(range.first, range.second) == 4);

        // check the order
        int prevValue = *tree.begin();
        for (iter = next(tree.begin()); iter != tree.end(); ++iter)
        {
            AZ_TEST_ASSERT(prevValue <= *iter);
            prevValue = *iter;
        }

        // rbtree with different key equal function
        typedef rbtree<RedBlackTree_SetTestTraits<int, AZStd::greater<int> > > rbtree_set_type1;
        rbtree_set_type1 tree1;

        tree1.insert_equal(elements.begin(), elements.end());
        tree1.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree1, 10);

        // check the order
        prevValue = *tree1.begin();
        for (rbtree_set_type1::iterator it = next(tree1.begin()); it != tree1.end(); ++it)
        {
            AZ_TEST_ASSERT(prevValue >= *it);
            prevValue = *it;
        }
    }

    class Tree_IntrusiveMultiSet
        : public AllocatorsFixture
    {
    public:
        struct Node
            : public AZStd::intrusive_multiset_node<Node>
        {
            Node(int order)
                : m_order(order) {}

            operator int() {
                return m_order;
            }

            int m_order;
        };

        typedef AZStd::intrusive_multiset<Node, AZStd::intrusive_multiset_base_hook<Node>, AZStd::less<int> > IntrusiveSetIntKeyType;
    };

    TEST_F(Tree_IntrusiveMultiSet, Test)
    {
        IntrusiveSetIntKeyType  tree;

        // make sure the tree is empty
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        // insert node
        Node n1(1);
        tree.insert(&n1);
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 1);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        AZ_TEST_ASSERT(tree.begin()->m_order == 1);

        // insert node and sort
        Node n2(0);
        tree.insert(&n2);
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 2);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        AZ_TEST_ASSERT(tree.begin()->m_order == 0);
        AZ_TEST_ASSERT((++tree.begin())->m_order == 1);

        // erase the first node
        IntrusiveSetIntKeyType::iterator it = tree.erase(tree.begin());
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(tree.size() == 1);
        AZ_TEST_ASSERT(it->m_order == 1);

        // insert via reference (a pointer to it will be taken)
        it = tree.insert(n2);
        AZ_TEST_ASSERT(it->m_order == n2.m_order);
        AZ_TEST_ASSERT(tree.size() == 2);

        AZStd::fixed_vector<Node, 5> elements;
        elements.push_back(Node(2));
        elements.push_back(Node(5));
        elements.push_back(Node(8));
        elements.push_back(Node(8));
        elements.push_back(Node(4));

        // insert from input iterator
        tree.insert(elements.begin(), elements.end());
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 7);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        it = tree.begin();
        AZ_TEST_ASSERT((it++)->m_order == 0);
        AZ_TEST_ASSERT((it++)->m_order == 1);
        AZ_TEST_ASSERT((it++)->m_order == 2);
        AZ_TEST_ASSERT((it++)->m_order == 4);
        AZ_TEST_ASSERT((it++)->m_order == 5);
        AZ_TEST_ASSERT((it++)->m_order == 8);
        AZ_TEST_ASSERT((it++)->m_order == 8);

        // lower bound
        it = tree.lower_bound(8);
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 8);
        AZ_TEST_ASSERT((++it)->m_order == 8);

        // upper bound
        it = tree.upper_bound(8);
        AZ_TEST_ASSERT(it == tree.end());
        AZ_TEST_ASSERT((--it)->m_order == 8);
        AZ_TEST_ASSERT((--it)->m_order == 8);

        // minimum
        it = tree.minimum();
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 0);

        // maximum
        it = tree.maximum();
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 8);

        // erase elements with iterator
        tree.erase(elements.begin(), elements.end());
        it = tree.begin();
        AZ_TEST_ASSERT(tree.size() == 2);
        AZ_TEST_ASSERT((it++)->m_order == 0);
        AZ_TEST_ASSERT((it++)->m_order == 1);

        // clear the entire container
        tree.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);
    }

    // SetContainerTest-Begin
    class Tree_Set
        : public AllocatorsFixture
    {
    };

    TEST_F(Tree_Set, Test)
    {
        array<int, 5> elements = {
            {2, 6, 9, 3, 9}
        };
        array<int, 5> elements2 = {
            {11, 4, 13, 6, 1}
        };
        AZStd::size_t num;

        typedef set<int> int_set_type;

        int_set_type set;
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        int_set_type set1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(set1 > set);
        AZ_TEST_VALIDATE_TREE(set1, 4);
        AZ_TEST_ASSERT(*set1.begin() == 2);
        AZ_TEST_ASSERT(*prior(set1.end()) == 9);

        int_set_type set2(set1);
        AZ_TEST_ASSERT(set1 == set2);
        AZ_TEST_VALIDATE_TREE(set2, 4);
        AZ_TEST_ASSERT(*set2.begin() == 2);
        AZ_TEST_ASSERT(*prior(set2.end()) == 9);

        set = set2;
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prior(set.end()) == 9);

        set.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        set.swap(set2);
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prior(set.end()) == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(set2);

        bool isInsert = set.insert(6).second;
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(isInsert == false);

        isInsert = set.insert(10).second;
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(isInsert == true);
        AZ_TEST_ASSERT(*prior(set.end()) == 10);

        set.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(set, 9);
        AZ_TEST_ASSERT(*set.begin() == 1);
        AZ_TEST_ASSERT(*prior(set.end()) == 13);

        set.erase(set.begin());
        AZ_TEST_VALIDATE_TREE(set, 8);
        AZ_TEST_ASSERT(*set.begin() == 2);

        set.erase(prior(set.end()));
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(*prior(set.end()) == 11);

        num = set.erase(6);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(num == 1);

        num = set.erase(100);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(num == 0);

        int_set_type::iterator iter = set.find(9);
        AZ_TEST_ASSERT(iter != set.end());

        iter = set.find(99);
        AZ_TEST_ASSERT(iter == set.end());

        num = set.count(9);
        AZ_TEST_ASSERT(num == 1);

        num = set.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = set.lower_bound(11);
        AZ_TEST_ASSERT(iter != set.end());
        AZ_TEST_ASSERT(*iter == 11);

        iter = set.lower_bound(111);
        AZ_TEST_ASSERT(iter == set.end());

        iter = set.upper_bound(11);
        AZ_TEST_ASSERT(iter == set.end()); // this is the last element

        iter = set.upper_bound(4);
        AZ_TEST_ASSERT(iter != set.end()); // this is NOT the last element

        iter = set.upper_bound(44);
        AZ_TEST_ASSERT(iter == set.end());

        AZStd::pair<int_set_type::iterator, int_set_type::iterator> range = set.equal_range(11);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 1);
        AZ_TEST_ASSERT(range.first != set.end());
        AZ_TEST_ASSERT(*range.first == 11);
        AZ_TEST_ASSERT(range.second == set.end()); // this is the last element

        range = set.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == set.end());
        AZ_TEST_ASSERT(range.second == set.end());
    }

    TEST_F(Tree_Set, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::set<int, AZStd::less<int>, AZ::AZStdIAllocator> setWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        auto insertIter = setWithCustomAllocator.emplace(1);
        EXPECT_TRUE(insertIter.second);
        insertIter = setWithCustomAllocator.emplace(1);
        EXPECT_FALSE(insertIter.second);
        EXPECT_EQ(1, setWithCustomAllocator.size());
    }

    class Tree_MultiSet
        : public AllocatorsFixture
    {
    };

    TEST_F(Tree_MultiSet, Test)
    {
        array<int, 5> elements = {
            {2, 6, 9, 3, 9}
        };
        array<int, 5> elements2 = {
            {11, 4, 13, 6, 1}
        };
        AZStd::size_t num;

        typedef multiset<int> int_multiset_type;

        int_multiset_type set;
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        int_multiset_type set1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(set1 > set);
        AZ_TEST_VALIDATE_TREE(set1, 5);
        AZ_TEST_ASSERT(*set1.begin() == 2);
        AZ_TEST_ASSERT(*prior(set1.end()) == 9);

        int_multiset_type set2(set1);
        AZ_TEST_ASSERT(set1 == set2);
        AZ_TEST_VALIDATE_TREE(set2, 5);
        AZ_TEST_ASSERT(*set2.begin() == 2);
        AZ_TEST_ASSERT(*prior(set2.end()) == 9);

        set = set2;
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prior(set.end()) == 9);

        set.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        set.swap(set2);
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prior(set.end()) == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(set2);

        set.insert(10);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(*prior(set.end()) == 10);

        set.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(set, 11);
        AZ_TEST_ASSERT(*set.begin() == 1);
        AZ_TEST_ASSERT(*prior(set.end()) == 13);

        set.erase(set.begin());
        AZ_TEST_VALIDATE_TREE(set, 10);
        AZ_TEST_ASSERT(*set.begin() == 2);

        set.erase(prior(set.end()));
        AZ_TEST_VALIDATE_TREE(set, 9);
        AZ_TEST_ASSERT(*prior(set.end()) == 11);

        num = set.erase(6);
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(num == 2);

        num = set.erase(100);
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(num == 0);

        int_multiset_type::iterator iter = set.find(9);
        AZ_TEST_ASSERT(iter != set.end());

        iter = set.find(99);
        AZ_TEST_ASSERT(iter == set.end());

        num = set.count(9);
        AZ_TEST_ASSERT(num == 2);

        num = set.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = set.lower_bound(11);
        AZ_TEST_ASSERT(iter != set.end());
        AZ_TEST_ASSERT(*iter == 11);

        iter = set.lower_bound(111);
        AZ_TEST_ASSERT(iter == set.end());

        iter = set.upper_bound(11);
        AZ_TEST_ASSERT(iter == set.end()); // this is the last element

        iter = set.upper_bound(4);
        AZ_TEST_ASSERT(iter != set.end()); // this is NOT the last element

        iter = set.upper_bound(44);
        AZ_TEST_ASSERT(iter == set.end());

        AZStd::pair<int_multiset_type::iterator, int_multiset_type::iterator> range = set.equal_range(9);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 2);
        AZ_TEST_ASSERT(range.first != set.end());
        AZ_TEST_ASSERT(*range.first == 9);
        AZ_TEST_ASSERT(range.second != set.end());

        range = set.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == set.end());
        AZ_TEST_ASSERT(range.second == set.end());
    }

    TEST_F(Tree_MultiSet, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::multiset<int, AZStd::less<int>, AZ::AZStdIAllocator> setWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        setWithCustomAllocator.emplace(1);
        setWithCustomAllocator.emplace(1);
        EXPECT_EQ(2, setWithCustomAllocator.size());
    }

    class Tree_Map
        : public AllocatorsFixture
    {
    };

    TEST_F(Tree_Map, Test)
    {
        fixed_vector<AZStd::pair<int, int>, 5> elements;
        elements.push_back(AZStd::make_pair(2, 100));
        elements.push_back(AZStd::make_pair(6, 101));
        elements.push_back(AZStd::make_pair(9, 102));
        elements.push_back(AZStd::make_pair(3, 103));
        elements.push_back(AZStd::make_pair(9, 104));
        fixed_vector<AZStd::pair<int, int>, 5> elements2;
        elements2.push_back(AZStd::make_pair(11, 200));
        elements2.push_back(AZStd::make_pair(4, 201));
        elements2.push_back(AZStd::make_pair(13, 202));
        elements2.push_back(AZStd::make_pair(6, 203));
        elements2.push_back(AZStd::make_pair(1, 204));
        AZStd::size_t num;

        typedef map<int, int> int_map_type;

        int_map_type map;
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        int_map_type map1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(map1 > map);
        AZ_TEST_VALIDATE_TREE(map1, 4);
        AZ_TEST_ASSERT((*map1.begin()).first == 2);
        AZ_TEST_ASSERT((*prior(map1.end())).first == 9);

        int_map_type map2(map1);
        AZ_TEST_ASSERT(map1 == map2);
        AZ_TEST_VALIDATE_TREE(map2, 4);
        AZ_TEST_ASSERT((*map2.begin()).first == 2);
        AZ_TEST_ASSERT((*prior(map2.end())).first == 9);

        map = map2;
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT((*map.begin()).first == 2);
        AZ_TEST_ASSERT((*prior(map.end())).first == 9);

        map.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        map.swap(map2);
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT((*map.begin()).first == 2);
        AZ_TEST_ASSERT((*prior(map.end())).first == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(map2);

        bool isInsert = map.insert(6).second;
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT(isInsert == false);

        isInsert = map.insert(10).second;
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(isInsert == true);
        AZ_TEST_ASSERT((*prior(map.end())).first == 10);

        map.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(map, 9);
        AZ_TEST_ASSERT((*map.begin()).first == 1);
        AZ_TEST_ASSERT((*prior(map.end())).first == 13);

        map.erase(map.begin());
        AZ_TEST_VALIDATE_TREE(map, 8);
        AZ_TEST_ASSERT(map.begin()->first == 2);

        map.erase(prior(map.end()));
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(prior(map.end())->first == 11);

        num = map.erase(6);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(num == 1);

        num = map.erase(100);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(num == 0);

        int_map_type::iterator iter = map.find(9);
        AZ_TEST_ASSERT(iter != map.end());

        iter = map.find(99);
        AZ_TEST_ASSERT(iter == map.end());

        num = map.count(9);
        AZ_TEST_ASSERT(num == 1);

        num = map.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = map.lower_bound(11);
        AZ_TEST_ASSERT(iter != map.end());
        AZ_TEST_ASSERT(iter->first == 11);

        iter = map.lower_bound(111);
        AZ_TEST_ASSERT(iter == map.end());

        iter = map.upper_bound(11);
        AZ_TEST_ASSERT(iter == map.end()); // this is the last element

        iter = map.upper_bound(4);
        AZ_TEST_ASSERT(iter != map.end()); // this is NOT the last element

        iter = map.upper_bound(44);
        AZ_TEST_ASSERT(iter == map.end());

        AZStd::pair<int_map_type::iterator, int_map_type::iterator> range = map.equal_range(11);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 1);
        AZ_TEST_ASSERT(range.first != map.end());
        AZ_TEST_ASSERT(range.first->first == 11);
        AZ_TEST_ASSERT(range.second == map.end()); // this is the last element

        range = map.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == map.end());
        AZ_TEST_ASSERT(range.second == map.end());

        AZStd::map<int, MyNoCopyClass> nocopy_map;
        MyNoCopyClass& inserted = nocopy_map.insert(3).first->second;
        inserted.m_bool = true;
        AZ_TEST_ASSERT(nocopy_map.begin()->second.m_bool == true);

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_map_type map3({
                {1, 2}, {3, 4}, {5, 6}
            });
        AZ_TEST_VALIDATE_TREE(map3, 3);
        AZ_TEST_ASSERT((*map3.begin()).first == 1);
        AZ_TEST_ASSERT((*map3.begin()).second == 2);
        AZ_TEST_ASSERT((*prior(map3.end())).first == 5);
        AZ_TEST_ASSERT((*prior(map3.end())).second == 6);
#endif
    }

    TEST_F(Tree_Map, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::map<int, int, AZStd::less<int>, AZ::AZStdIAllocator> mapWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        auto insertIter = mapWithCustomAllocator.emplace(1, 1);
        EXPECT_TRUE(insertIter.second);
        insertIter = mapWithCustomAllocator.emplace(1, 2);
        EXPECT_FALSE(insertIter.second);
        EXPECT_EQ(1, mapWithCustomAllocator.size());
    }

    TEST_F(Tree_Map, IndexOperatorCompilesWithMoveOnlyType)
    {
        AZStd::map<int, AZStd::unique_ptr<int>> uniquePtrIntMap;
        uniquePtrIntMap[4] = AZStd::make_unique<int>(74);
        auto findIter = uniquePtrIntMap.find(4);
        EXPECT_NE(uniquePtrIntMap.end(), findIter);
        EXPECT_EQ(74, *findIter->second);
    }

    class Tree_MultiMap
        : public AllocatorsFixture
    {
    };

    TEST_F(Tree_MultiMap, Test)
    {
        fixed_vector<AZStd::pair<int, int>, 5> elements;
        elements.push_back(AZStd::make_pair(2, 100));
        elements.push_back(AZStd::make_pair(6, 101));
        elements.push_back(AZStd::make_pair(9, 102));
        elements.push_back(AZStd::make_pair(3, 103));
        elements.push_back(AZStd::make_pair(9, 104));
        fixed_vector<AZStd::pair<int, int>, 5> elements2;
        elements2.push_back(AZStd::make_pair(11, 200));
        elements2.push_back(AZStd::make_pair(4, 201));
        elements2.push_back(AZStd::make_pair(13, 202));
        elements2.push_back(AZStd::make_pair(6, 203));
        elements2.push_back(AZStd::make_pair(1, 204));
        AZStd::size_t num;

        typedef multimap<int, int> int_multimap_type;

        int_multimap_type map;
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        int_multimap_type map1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(map1 > map);
        AZ_TEST_VALIDATE_TREE(map1, 5);
        AZ_TEST_ASSERT(map1.begin()->first == 2);
        AZ_TEST_ASSERT(prior(map1.end())->first == 9);

        int_multimap_type map2(map1);
        AZ_TEST_ASSERT(map1 == map2);
        AZ_TEST_VALIDATE_TREE(map2, 5);
        AZ_TEST_ASSERT(map2.begin()->first == 2);
        AZ_TEST_ASSERT(prior(map2.end())->first == 9);

        map = map2;
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(map.begin()->first == 2);
        AZ_TEST_ASSERT(prior(map.end())->first == 9);

        map.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        map.swap(map2);
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(map.begin()->first == 2);
        AZ_TEST_ASSERT(prior(map.end())->first == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(map2);

        map.insert(10);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(prior(map.end())->first == 10);

        map.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(map, 11);
        AZ_TEST_ASSERT(map.begin()->first == 1);
        AZ_TEST_ASSERT(prior(map.end())->first == 13);

        map.erase(map.begin());
        AZ_TEST_VALIDATE_TREE(map, 10);
        AZ_TEST_ASSERT(map.begin()->first == 2);

        map.erase(prior(map.end()));
        AZ_TEST_VALIDATE_TREE(map, 9);
        AZ_TEST_ASSERT(prior(map.end())->first == 11);

        num = map.erase(6);
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(num == 2);

        num = map.erase(100);
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(num == 0);

        int_multimap_type::iterator iter = map.find(9);
        AZ_TEST_ASSERT(iter != map.end());

        iter = map.find(99);
        AZ_TEST_ASSERT(iter == map.end());

        num = map.count(9);
        AZ_TEST_ASSERT(num == 2);

        num = map.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = map.lower_bound(11);
        AZ_TEST_ASSERT(iter != map.end());
        AZ_TEST_ASSERT(iter->first == 11);

        iter = map.lower_bound(111);
        AZ_TEST_ASSERT(iter == map.end());

        iter = map.upper_bound(11);
        AZ_TEST_ASSERT(iter == map.end()); // this is the last element

        iter = map.upper_bound(4);
        AZ_TEST_ASSERT(iter != map.end()); // this is NOT the last element

        iter = map.upper_bound(44);
        AZ_TEST_ASSERT(iter == map.end());

        AZStd::pair<int_multimap_type::iterator, int_multimap_type::iterator> range = map.equal_range(9);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 2);
        AZ_TEST_ASSERT(range.first != map.end());
        AZ_TEST_ASSERT(range.first->first == 9);
        AZ_TEST_ASSERT(range.second != map.end());

        range = map.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == map.end());
        AZ_TEST_ASSERT(range.second == map.end());

#if defined(AZ_HAS_INITIALIZERS_LIST)
        int_multimap_type intint_map3({
                {1, 10}, {2, 200}, {3, 3000}, {4, 40000}, {4, 40001}, {5, 500000}
            });
        AZ_TEST_ASSERT(intint_map3.size() == 6);
        AZ_TEST_ASSERT(intint_map3.count(1) == 1);
        AZ_TEST_ASSERT(intint_map3.count(2) == 1);
        AZ_TEST_ASSERT(intint_map3.count(3) == 1);
        AZ_TEST_ASSERT(intint_map3.count(4) == 2);
        AZ_TEST_ASSERT(intint_map3.count(5) == 1);
        AZ_TEST_ASSERT(intint_map3.lower_bound(1)->second == 10);
        AZ_TEST_ASSERT(intint_map3.lower_bound(2)->second == 200);
        AZ_TEST_ASSERT(intint_map3.lower_bound(3)->second == 3000);
        AZ_TEST_ASSERT(intint_map3.lower_bound(4)->second == 40000 || intint_map3.lower_bound(4)->second == 40001);
        AZ_TEST_ASSERT((++intint_map3.lower_bound(4))->second == 40000 || (++intint_map3.lower_bound(4))->second == 40001);
        AZ_TEST_ASSERT(intint_map3.lower_bound(5)->second == 500000);
#endif
    }

    TEST_F(Tree_MultiMap, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::multimap<int, int, AZStd::less<int>, AZ::AZStdIAllocator> mapWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        mapWithCustomAllocator.emplace(1, 1);
        mapWithCustomAllocator.emplace(1, 2);
        EXPECT_EQ(2, mapWithCustomAllocator.size());
    }
}
