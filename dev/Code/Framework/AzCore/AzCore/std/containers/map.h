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
#ifndef AZSTD_ORDERED_MAP_H
#define AZSTD_ORDERED_MAP_H 1

#include <AzCore/std/containers/rbtree.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/algorithm.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class KeyEq, class Allocator >
        struct OrderedMapRbTreeTraits
        {
            typedef Key                         key_type;
            typedef KeyEq                       key_eq;
            typedef AZStd::pair<Key, MappedType> value_type;
            typedef Allocator                   allocator_type;
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };
    }

    /**
    * Ordered map container is complaint with \ref C++0x (23.4.1)
    * This is an associative container, all keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class MappedType, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class map
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef map<Key, MappedType, Compare, Allocator> this_type;
        typedef rbtree< Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator> > tree_type;

    public:
        typedef typename tree_type::traits_type traits_type;

        typedef typename tree_type::key_type       key_type;
        typedef typename tree_type::value_type     value_type;
        typedef typename tree_type::key_eq         key_compare;
        typedef MappedType                         mapped_type;
        class value_compare
            : public AZStd::binary_function<value_type, value_type, bool>
        {
        protected:
            Compare m_comp;
            AZ_FORCE_INLINE value_compare(Compare c)
                : m_comp(c) {}
        public:
            AZ_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        typedef typename tree_type::allocator_type              allocator_type;
        typedef typename tree_type::size_type                   size_type;
        typedef typename tree_type::difference_type             difference_type;
        typedef typename tree_type::pointer                     pointer;
        typedef typename tree_type::const_pointer               const_pointer;
        typedef typename tree_type::reference                   reference;
        typedef typename tree_type::const_reference             const_reference;

        typedef typename tree_type::iterator                    iterator;
        typedef typename tree_type::const_iterator              const_iterator;

        typedef typename tree_type::reverse_iterator            reverse_iterator;
        typedef typename tree_type::const_reverse_iterator      const_reverse_iterator;

        AZ_FORCE_INLINE explicit map(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        template <class InputIterator>
        AZ_FORCE_INLINE map(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(first, last);
        }
        AZ_FORCE_INLINE map(const this_type& rhs)
            : m_tree(rhs.m_tree)  {}
        AZ_FORCE_INLINE map(const this_type& rhs, const Allocator& alloc)
            : m_tree(rhs.m_tree, alloc)   {}
#if defined(AZ_HAS_INITIALIZERS_LIST)
        AZ_FORCE_INLINE map(const std::initializer_list<value_type>& list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(list.begin(), list.end());
        }
#endif // #if defined(AZ_HAS_INITIALIZERS_LIST)

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const        { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const    { return value_comp(m_tree.key_comp()); }

        AZ_FORCE_INLINE iterator begin()                    { return m_tree.begin(); }
        AZ_FORCE_INLINE iterator end()                      { return m_tree.end(); }
        AZ_FORCE_INLINE const_iterator begin() const        { return m_tree.begin(); }
        AZ_FORCE_INLINE const_iterator end() const          { return m_tree.end(); }
        AZ_FORCE_INLINE reverse_iterator rbegin()           { return m_tree.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator rend()             { return m_tree.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        AZ_FORCE_INLINE bool empty() const                  { return m_tree.empty(); }
        AZ_FORCE_INLINE size_type size() const              { return m_tree.size(); }
        AZ_FORCE_INLINE size_type max_size() const          { return m_tree.max_size(); }
        AZ_FORCE_INLINE MappedType& operator[](const key_type& key)
        {
            iterator iter = m_tree.lower_bound(key);
            if (iter == end() || key_comp()(key, (*iter).first))
            {
                iter = insert(iter, AZStd::move(value_type(key, MappedType())));
            }
            return (*iter).second;
        }
        AZ_FORCE_INLINE void swap(this_type& rhs)           { m_tree.swap(rhs.m_tree); }

        // insert/erase
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value)             { return m_tree.insert_unique(value); }
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value)    { return m_tree.insert_unique(insertPos, value); }
        template <class InputIterator>
        AZ_FORCE_INLINE void insert(InputIterator first, InputIterator last)            { m_tree.insert_unique(first, last); }
        AZ_FORCE_INLINE iterator erase(const_iterator erasePos)                         { return m_tree.erase(erasePos); }
        AZ_FORCE_INLINE size_type erase(const key_type& key)                            { return m_tree.erase_unique(key); }
        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)       { return m_tree.erase(first, last); }
        AZ_FORCE_INLINE void clear() { m_tree.clear(); }

#ifdef AZ_HAS_RVALUE_REFS
        map(this_type&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}
        /* map(this_type&& rhs, const Allocator& alloc)
             : m_tree(AZStd::move(rhs.m_tree), alloc){}*/
        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        AZStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_unique(AZStd::forward<value_type>(value));
        }
        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_unique(insertPos, AZStd::forward<value_type>(value));
        }
        template<class ... InputArguments>
        AZStd::pair<iterator, bool> emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(AZStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(insertPos, AZStd::forward<InputArguments>(arguments) ...);
        }
#endif

        // set operations:
        AZ_FORCE_INLINE const_iterator find(const key_type& key) const          { return m_tree.find(key); }
        AZ_FORCE_INLINE iterator find(const key_type& key)                      { return m_tree.find(key); }
        AZ_FORCE_INLINE size_type count(const key_type& key) const              { return m_tree.find(key) == m_tree.end() ? 0 : 1; }
        AZ_FORCE_INLINE iterator lower_bound(const key_type& key)               { return m_tree.lower_bound(key); }
        AZ_FORCE_INLINE const_iterator lower_bound(const key_type& key) const   { return m_tree.lower_bound(key); }
        AZ_FORCE_INLINE iterator upper_bound(const key_type& key)               { return m_tree.upper_bound(key); }
        AZ_FORCE_INLINE const_iterator upper_bound(const key_type& key) const   { return m_tree.upper_bound(key); }
        AZ_FORCE_INLINE pair<iterator, iterator> equal_range(const key_type& key)                   { return m_tree.equal_range_unique(key); }
        AZ_FORCE_INLINE pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range_unique(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        /**
         * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
         * this is here only to make sure it's available to the end user.
         */
        AZ_FORCE_INLINE pair<iterator, bool> insert(const key_type& key)
        {
            if (m_tree.find(key) != m_tree.end())
            {
                return AZStd::make_pair(m_tree.end(), false);
            }
            return m_tree.insert_unique(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_tree.get_allocator(); }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        AZ_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        AZ_INLINE bool      validate() const    { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        AZ_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        AZ_INLINE int       validate_iterator(const iterator& iter) const       { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        AZ_FORCE_INLINE void                        leak_and_reset()    { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(map<Key, MappedType, Compare, Allocator>& left, map<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    /**
    * Ordered multimap container is complaint with \ref C++0x (23.4.2)
    * This is an associative container, key can be equivalent (multiple copies of the same key value).
    * You can iterate over them, by checking if the key stays the same, after finding the first.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class MappedType, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class multimap
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef multimap<Key, MappedType, Compare, Allocator> this_type;
        typedef rbtree< Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator> > tree_type;
    public:
        typedef typename tree_type::traits_type traits_type;

        typedef typename tree_type::key_type       key_type;
        typedef typename tree_type::value_type     value_type;
        typedef typename tree_type::key_eq         key_compare;
        typedef MappedType                         mapped_type;
        class value_compare
            : public AZStd::binary_function<value_type, value_type, bool>
        {
        protected:
            Compare m_comp;
            AZ_FORCE_INLINE value_compare(Compare c)
                : m_comp(c) {}
        public:
            AZ_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        typedef typename tree_type::allocator_type              allocator_type;
        typedef typename tree_type::size_type                   size_type;
        typedef typename tree_type::difference_type             difference_type;
        typedef typename tree_type::pointer                     pointer;
        typedef typename tree_type::const_pointer               const_pointer;
        typedef typename tree_type::reference                   reference;
        typedef typename tree_type::const_reference             const_reference;

        typedef typename tree_type::iterator                    iterator;
        typedef typename tree_type::const_iterator              const_iterator;

        typedef typename tree_type::reverse_iterator            reverse_iterator;
        typedef typename tree_type::const_reverse_iterator      const_reverse_iterator;

        AZ_FORCE_INLINE explicit multimap(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        template <class InputIterator>
        AZ_FORCE_INLINE multimap(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(first, last);
        }
        AZ_FORCE_INLINE multimap(const this_type& rhs)
            : m_tree(rhs.m_tree) {}
        AZ_FORCE_INLINE multimap(const this_type& rhs, const Allocator& alloc)
            : m_tree(rhs.m_tree, alloc)   {}
#if defined(AZ_HAS_INITIALIZERS_LIST)
        AZ_FORCE_INLINE multimap(const std::initializer_list<value_type>& list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(list.begin(), list.end());
        }
#endif // #if defined(AZ_HAS_INITIALIZERS_LIST)

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const        { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const    { return value_compare(m_tree.key_comp()); }

        AZ_FORCE_INLINE iterator begin()                    { return m_tree.begin(); }
        AZ_FORCE_INLINE iterator end()                      { return m_tree.end(); }
        AZ_FORCE_INLINE const_iterator begin() const        { return m_tree.begin(); }
        AZ_FORCE_INLINE const_iterator end() const          { return m_tree.end(); }
        AZ_FORCE_INLINE reverse_iterator rbegin()           { return m_tree.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator rend()             { return m_tree.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        AZ_FORCE_INLINE bool empty() const                  { return m_tree.empty(); }
        AZ_FORCE_INLINE size_type size() const              { return m_tree.size(); }
        AZ_FORCE_INLINE size_type max_size() const          { return m_tree.max_size(); }
        AZ_FORCE_INLINE void swap(this_type& rhs)           { m_tree.swap(rhs.m_tree); }

        // insert/erase
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value)             { return m_tree.insert_equal(value); }
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value)    { return m_tree.insert_equal(insertPos, value); }
        template <class InputIterator>
        AZ_FORCE_INLINE void insert(InputIterator first, InputIterator last)            { m_tree.insert_equal(first, last); }
        AZ_FORCE_INLINE iterator erase(const_iterator erasePos)                         { return m_tree.erase(erasePos); }
        AZ_FORCE_INLINE size_type erase(const key_type& key)                            { return m_tree.erase(key); }
        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)       { return m_tree.erase(first, last); }
        AZ_FORCE_INLINE void clear() { m_tree.clear(); }

#ifdef AZ_HAS_RVALUE_REFS
        multimap(this_type&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}
        multimap(this_type&& rhs, const Allocator& alloc)
            : m_tree(AZStd::move(rhs.m_tree), alloc){}
        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        AZStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_equal(AZStd::forward<value_type>(value));
        }

        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_equal(insertPos, AZStd::forward<value_type>(value));
        }

        template<class ... InputArguments>
        iterator emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(AZStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(insertPos, AZStd::forward<InputArguments>(arguments) ...);
        }
#endif

        // set operations:
        AZ_FORCE_INLINE const_iterator find(const key_type& key) const          { return m_tree.find(key); }
        AZ_FORCE_INLINE iterator find(const key_type& key)                      { return m_tree.find(key); }
        AZ_FORCE_INLINE size_type count(const key_type& key) const              { return m_tree.count(key); }
        AZ_FORCE_INLINE iterator lower_bound(const key_type& key)               { return m_tree.lower_bound(key); }
        AZ_FORCE_INLINE const_iterator lower_bound(const key_type& key) const   { return m_tree.lower_bound(key); }
        AZ_FORCE_INLINE iterator upper_bound(const key_type& key)               { return m_tree.upper_bound(key); }
        AZ_FORCE_INLINE const_iterator upper_bound(const key_type& key) const   { return m_tree.upper_bound(key); }
        AZ_FORCE_INLINE pair<iterator, iterator> equal_range(const key_type& key)                   { return m_tree.equal_range(key); }
        AZ_FORCE_INLINE pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        /**
        * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
        * this is here only to make sure it's available to the end user.
        */
        AZ_FORCE_INLINE iterator  insert(const key_type& key)
        {
            return m_tree.insert_equal(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_tree.get_allocator(); }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        AZ_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        AZ_INLINE bool      validate() const    { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        AZ_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        AZ_INLINE int       validate_iterator(const iterator& iter) const       { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        AZ_FORCE_INLINE void                        leak_and_reset()    { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(multimap<Key, MappedType, Compare, Allocator>& left, multimap<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }
}

#endif // AZSTD_ORDERED_MAP_H
#pragma once