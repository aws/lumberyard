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
#ifndef AZSTD_FIXED_VECTOR_H
#define AZSTD_FIXED_VECTOR_H 1

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    /**
     * This is the fixed version of the \ref AZStd::vector. All of their functionality is the same
     * except we do not have allocator and never allocate memory. We can not change the capacity, it
     * is set at compile time. leak_and_reset function just does the reset part, no destructor is called.
     *
     * \note Although big parts of the code is the same with vector, we do have separate implementation. This
     * is following AZStd rule to avoid deep function calls.
     *
     * Check the fixed_vector \ref AZStdExamples.
     */
    template< class T, AZStd::size_t Capacity>
    class fixed_vector
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef fixed_vector<T, Capacity>                this_type;
    public:
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef typename AZStd::ptrdiff_t               difference_type;
        typedef typename AZStd::size_t                  size_type;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_randomaccess_iterator<pointer, this_type>         iterator;
        typedef Debug::checked_randomaccess_iterator<const_pointer, this_type>   const_iterator;
#else
        typedef pointer                                 iterator;
        typedef const_pointer                           const_iterator;
#endif
        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T                                       value_type;
        //typedef Allocator                             allocator_type;

        // AZSTD extension.
        typedef value_type                              node_type;

        //////////////////////////////////////////////////////////////////////////
        // 23.2.4.1 construct/copy/destroy
        AZ_FORCE_INLINE explicit fixed_vector()
            : m_last(reinterpret_cast<pointer>(&m_data))
        {}


        inline explicit fixed_vector(size_type numElements, const_reference value = value_type())
        {
            AZSTD_CONTAINER_ASSERT(numElements <= Capacity, "AZStd::fixed_vector::fixed_vector too many elements!");

            pointer start = reinterpret_cast<pointer>(&m_data);
            AZStd::uninitialized_fill_n(start, numElements, value, Internal::is_fast_fill<pointer>());
            m_last  = start + numElements;
        }

        template <class InputIterator>
        AZ_FORCE_INLINE fixed_vector(const InputIterator& first, const InputIterator& last)
            : m_last(reinterpret_cast<pointer>(&m_data))
        {
            // We need some help here, since this function can be mistaken with the vector(size_type, const_reference value, const allocator_type&) one...
            // so we need to handle this case.
            construct_iter(first, last, is_integral<InputIterator>());
        }


        AZ_FORCE_INLINE fixed_vector(const this_type& rhs)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            m_last  = AZStd::uninitialized_copy((const_pointer) & rhs.m_data, (const_pointer)rhs.m_last, start, Internal::is_fast_copy<pointer, pointer>());
        }
#if defined(AZ_HAS_INITIALIZERS_LIST)
        AZ_FORCE_INLINE fixed_vector(std::initializer_list<T> list)
            : m_last(reinterpret_cast<pointer>(&m_data))
        {
            construct_iter(list.begin(), list.end(), is_integral<std::initializer_list<T> >());
        }
#endif // #if defined(AZ_HAS_INITIALIZERS_LIST

        AZ_FORCE_INLINE ~fixed_vector()
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            // Call destructor if we need to.
            Internal::destroy<pointer>::range(start, m_last);
        }

        this_type& operator=(const this_type& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            const_pointer rhsStart = reinterpret_cast<const_pointer>(&rhs.m_data);
            size_type newSize = rhs.m_last - rhsStart;

            pointer start = reinterpret_cast<pointer>(&m_data);
            size_type size = m_last - start;
            if (size >= newSize)
            {
                if (newSize > 0)
                {
                    // We have capacity to hold the new data.
                    pointer newLast = Internal::copy(rhsStart, static_cast<const_pointer>(rhs.m_last), start, Internal::is_fast_copy<pointer, pointer>());

                    // Destroy the rest.
                    Internal::destroy<pointer>::range(newLast, m_last);

                    m_last = newLast;
                }
                else
                {
                    // Destroy the rest.
                    Internal::destroy<pointer>::range(start, m_last);
                    m_last = start;
                }
            }
            else
            {
                pointer newLast = Internal::copy(rhsStart, rhsStart + size, start, Internal::is_fast_copy<pointer, pointer>());

                m_last = AZStd::uninitialized_copy(rhsStart + size, static_cast<const_pointer>(rhs.m_last), newLast, Internal::is_fast_copy<pointer, pointer>());
            }

            return *this;
        }

        AZ_FORCE_INLINE size_type   size() const        { const_pointer start = reinterpret_cast<const_pointer>(&m_data); return m_last - start; }
        AZ_FORCE_INLINE size_type   max_size() const    { return Capacity; }
        AZ_FORCE_INLINE bool        empty() const               { const_pointer start = reinterpret_cast<const_pointer>(&m_data); return start == m_last; }
        AZ_FORCE_INLINE size_type   capacity() const    { return Capacity; }

        AZ_FORCE_INLINE iterator            begin()             { return iterator(AZSTD_POINTER_ITERATOR_PARAMS(reinterpret_cast<pointer>(&m_data))); }
        AZ_FORCE_INLINE const_iterator      begin() const       { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(reinterpret_cast<const_pointer>(&m_data))); }
        AZ_FORCE_INLINE iterator            end()               { return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)); }
        AZ_FORCE_INLINE const_iterator      end() const         { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)); }
        AZ_FORCE_INLINE reverse_iterator        rbegin()            { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const      { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator        rend()              { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const        { return const_reverse_iterator(begin()); }

        inline void resize(size_type newSize)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            size_type size = m_last - start;
            if (size < newSize)
            {
                insert(iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)), newSize - size, value_type());
            }
            else if (newSize < size)
            {
                erase(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)) + newSize, const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)));
            }
        }

        inline void resize(size_type newSize, const_reference value)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            size_type size = m_last - start;
            if (size < newSize)
            {
                insert(iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)), newSize - size, value);
            }
            else if (newSize < size)
            {
                erase(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)) + newSize, const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)));
            }
        }

        AZ_FORCE_INLINE reference at(size_type position)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - start), "AZStd::fixed_vector<>::at - position is out of range");
            return *(start + position);
        }
        AZ_FORCE_INLINE const_reference at(size_type position) const
        {
            const_pointer start = reinterpret_cast<const_pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - start), "AZStd::fixed_vector<>::at - position is out of range");
            return *(start + position);
        }

        AZ_FORCE_INLINE reference operator[](size_type position)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - start), "AZStd::fixed_vector<>::at - position is out of range");
            return *(start + position);
        }
        AZ_FORCE_INLINE const_reference operator[](size_type position) const
        {
            const_pointer start = reinterpret_cast<const_pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - start), "AZStd::fixed_vector<>::at - position is out of range");
            return *(start + position);
        }

        AZ_FORCE_INLINE reference       front()
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(m_last != start, "AZStd::fixed_vector<>::front - container is empty!");
            return *start;
        }
        AZ_FORCE_INLINE const_reference front() const
        {
            pointer start = reinterpret_cast<const_pointer>(&m_data);
            AZSTD_CONTAINER_ASSERT(m_last != start, "AZStd::fixed_vector<>::front - container is empty!");
            return *start;
        }
        AZ_FORCE_INLINE reference       back()
        {
            AZSTD_CONTAINER_ASSERT(m_last != reinterpret_cast<pointer>(&m_data), "AZStd::fixed_vector<>::back - container is empty! last: %p data: %p", m_last, &m_data);
            return *(m_last - 1);
        }
        AZ_FORCE_INLINE const_reference back() const
        {
            AZSTD_CONTAINER_ASSERT(m_last != reinterpret_cast<const_pointer>(&m_data), "AZStd::fixed_vector<>::back - container is empty!");
            return *(m_last - 1);
        }

        AZ_FORCE_INLINE void        push_back(const_reference value)
        {
            AZSTD_CONTAINER_ASSERT(size_type(m_last - reinterpret_cast<pointer>(&m_data)) < Capacity, "AZStd::fixed_vector is full (%zu elements)!", Capacity);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(m_last, m_last);
#endif
            AZStd::uninitialized_fill_n(m_last, 1, value, Internal::is_fast_fill<pointer>());
            ++m_last;
        }

        AZ_FORCE_INLINE void        pop_back()
        {
            AZSTD_CONTAINER_ASSERT(reinterpret_cast<pointer>(&m_data) != m_last, "AZStd::fixed_vector<>::pop_back - no element to pop! last: %p data: %p", m_last, &m_data);
            pointer toDestroy = m_last - 1;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(toDestroy, m_last);
#endif
            Internal::destroy<pointer>::single(toDestroy);
            m_last = toDestroy;
        }

        inline void         assign(size_type numElements, const_reference value)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            value_type valueCopy = value;   // in case value is in sequence
            clear();
            insert(iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)), numElements, valueCopy);
        }

        template<class InputIterator>
        AZ_FORCE_INLINE void        assign(const InputIterator& first, const InputIterator& last)
        {
            assign_iter(first, last, is_integral<InputIterator>());
        }

        iterator        insert(iterator insertPos, const_reference value)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer insertPosPtr = insertPos.get_iterator();
#else
            pointer insertPosPtr = insertPos;
#endif

            AZSTD_CONTAINER_ASSERT(insertPosPtr >= start && insertPosPtr <= m_last, "Invalid insert position!");
            size_type offset = insertPosPtr - start;
            insert(insertPos, (size_type)1, value);
            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)) + offset;
        }

        void            insert(iterator insertPos, size_type numElements, const_reference value)
        {
            if (numElements == 0)
            {
                return;
            }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer insertPosPtr = insertPos.get_iterator();
            orphan_range(insertPosPtr, m_last);
#else
            pointer insertPosPtr = insertPos;
#endif
            AZSTD_CONTAINER_ASSERT(Capacity >= ((m_last - reinterpret_cast<pointer>(&m_data)) + numElements), "AZStd::fixed_vector::insert - capacity is reached!");

            if (size_type(m_last - insertPosPtr) < numElements)
            {
                pointer newLast;
                // Number of elements we can just set not init needed.
                size_type numInitializedToFill = size_type(m_last - insertPosPtr);

                const_pointer valuePtr = AZStd::addressof(value);
                if (valuePtr >= insertPosPtr && valuePtr < m_last)
                {
                    // value is in the vector make a copy first
                    value_type valueCopy = value; // In case "value" is in the vector.

                    // Move the elements after insert position.
                    newLast = AZStd::uninitialized_move(insertPosPtr, m_last, insertPosPtr + numElements, Internal::is_fast_copy<pointer, pointer>());

                    // Add new elements to uninitialized elements.
                    AZStd::uninitialized_fill_n(m_last, numElements - numInitializedToFill, valueCopy, Internal::is_fast_fill<pointer>());

                    // Add new data
                    Internal::fill_n(insertPosPtr, numInitializedToFill, valueCopy, Internal::is_fast_fill<pointer>());
                }
                else
                {
                    // Move the elements after insert position.
                    newLast = AZStd::uninitialized_move(insertPosPtr, m_last, insertPosPtr + numElements, Internal::is_fast_copy<pointer, pointer>());

                    // Add new elements to uninitialized elements.
                    AZStd::uninitialized_fill_n(m_last, numElements - numInitializedToFill, value, Internal::is_fast_fill<pointer>());

                    // Add new data
                    Internal::fill_n(insertPosPtr, numInitializedToFill, value, Internal::is_fast_fill<pointer>());
                }

                m_last = newLast;
            }
            else
            {
                pointer newLast;
                pointer nonOverlap = m_last - numElements;
                const_pointer valuePtr = AZStd::addressof(value);
                if (valuePtr >= insertPosPtr && valuePtr < m_last)
                {
                    // value is in the vector make a copy first
                    value_type valueCopy = value;

                    // first copy the data that will not overlap.
                    newLast = AZStd::uninitialized_move(nonOverlap, m_last, m_last, Internal::is_fast_copy<pointer, pointer>());

                    // move the area with overlapping
                    Internal::move_backward(insertPosPtr, nonOverlap, m_last, Internal::is_fast_copy<pointer, pointer>());

                    // add new elements
                    Internal::fill_n(insertPosPtr, numElements, valueCopy, Internal::is_fast_fill<pointer>());
                }
                else
                {
                    // first copy the data that will not overlap.
                    newLast = AZStd::uninitialized_move(nonOverlap, m_last, m_last, Internal::is_fast_copy<pointer, pointer>());

                    // move the area with overlapping
                    Internal::move_backward(insertPosPtr, nonOverlap, m_last, Internal::is_fast_copy<pointer, pointer>());

                    // add new elements
                    Internal::fill_n(insertPosPtr, numElements, value, Internal::is_fast_fill<pointer>());
                }

                m_last = newLast;
            }
        }

        template<class InputIterator>
        AZ_FORCE_INLINE void        insert(iterator insertPos, const InputIterator& first, const InputIterator& last)
        {
            insert_impl(insertPos, first, last, is_integral<InputIterator>());
        }

        iterator        erase(const_iterator elementIter)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer elIter = elementIter.get_iterator();
            orphan_range(elIter, m_last);
#else
            pointer elIter = const_cast<pointer>(elementIter);
#endif
            size_type offset = elIter - start;
            // unless we have 1 elements we have memory overlapping, so we need to use move.
            Internal::move(elIter + 1, m_last, elIter, Internal::is_fast_copy<pointer, pointer>());
            pointer toDestroy = m_last - 1;
            Internal::destroy<pointer>::single(toDestroy);
            m_last = toDestroy;

            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)) + offset;
        }

        iterator        erase(const_iterator first, const_iterator last)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer firstPtr = first.get_iterator();
            pointer lastPtr = last.get_iterator();
            orphan_range(firstPtr, m_last);
#else
            pointer firstPtr = const_cast<pointer>(first);
            pointer lastPtr = const_cast<pointer>(last);
#endif
            size_type offset = firstPtr - start;
            // unless we have 1 elements we have memory overlapping, so we need to use move.
            pointer newLast = Internal::move(lastPtr, m_last, firstPtr, Internal::is_fast_copy<pointer, pointer>());
            Internal::destroy<pointer>::range(newLast, m_last);
            m_last = newLast;

            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)) + offset;
        }

        AZ_FORCE_INLINE void        clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            pointer start = reinterpret_cast<pointer>(&m_data);
            Internal::destroy<pointer>::range(start, m_last);
            m_last = start;
        }
        AZ_FORCE_INLINE void        swap(this_type& rhs)
        {
            this_type tempVec = *this;
            *this = rhs;
            rhs = tempVec;
        }

        // TR1 Extension. (\todo We should return 0 if there is no data.)
        AZ_FORCE_INLINE pointer         data()          { return reinterpret_cast<pointer>(&m_data); }
        AZ_FORCE_INLINE const_pointer   data() const    { return reinterpret_cast<const_pointer>(&m_data); }

        // Validate container status.
        inline bool                     validate() const
        {
            const_pointer start = reinterpret_cast<const_pointer>(&m_data);
            const_pointer end = start + Capacity;
            if (m_last > end || start > m_last)
            {
                return false;
            }
            return true;
        }
        // Validate iterator.
        inline int                      validate_iterator(const iterator& iter) const
        {
            const_pointer start = reinterpret_cast<const_pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            pointer iterPtr = iter.m_iter;
#else
            pointer iterPtr = iter;
#endif
            if (iterPtr < start || iterPtr > m_last)
            {
                return isf_none;
            }
            else if (iterPtr == m_last)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        inline int                      validate_iterator(const const_iterator& iter) const
        {
            const_pointer start = reinterpret_cast<const_pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            const_pointer iterPtr = iter.m_iter;
#else
            const_pointer iterPtr = iter;
#endif
            if (iterPtr < start || iterPtr > m_last)
            {
                return isf_none;
            }
            else if (iterPtr == m_last)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        // pushes back an empty without a provided instance.
        AZ_FORCE_INLINE void                    push_back()
        {
            AZSTD_CONTAINER_ASSERT(size_type(m_last - reinterpret_cast<pointer>(&m_data)) < Capacity, "AZStd::fixed_vector::push_back - capacity is reached!");
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(m_last, m_last);
#endif
            Internal::construct<pointer>::single(m_last);
            ++m_last;
        }


        inline void                 leak_and_reset()
        {
            m_last = reinterpret_cast<pointer>(&m_data);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

    private:

        //#pragma region Insert iterator specializations
        template<class Iterator>
        AZ_FORCE_INLINE void insert_impl(iterator insertPos, const Iterator& first, const Iterator& last, const true_type& /* is_integral<Iterator> */)
        {
            // we actually are calling this with integral types.
            insert(insertPos, (size_type)first, (const_reference)last);
        }

        template<class Iterator>
        AZ_FORCE_INLINE void insert_impl(iterator insertPos, const Iterator& first, const Iterator& last, const false_type& /* is_integral<Iterator> */)
        {
            // specialize for specific interators.
            insert_iter(insertPos, first, last, typename iterator_traits<Iterator>::iterator_category());
        }

        template<class Iterator>
        void insert_iter(iterator insertPos, const Iterator& first, const Iterator& last, const forward_iterator_tag&)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            // We can template this func if we want to check if first and last belong to the same container. It's possible
            // for them to be pointers or unchecked iterators.
            pointer insertPosPtr = insertPos.get_iterator();    // we validate iterators only if this container has elements. It is possible all to be 0.
            orphan_range(insertPosPtr, m_last);
#else
            pointer insertPosPtr = insertPos;
#endif
            size_type numElements = distance(first, last, typename iterator_traits<Iterator>::iterator_category());
            if (numElements == 0)
            {
                return;
            }
            //AZSTD_CONTAINER_ASSERT(numElements>0,("AZStd::vector<T>::insert<Iterator> - No point to insert 0 elements!"));

            AZSTD_CONTAINER_ASSERT(Capacity >= ((m_last - reinterpret_cast<pointer>(&m_data)) + numElements), "AZStd::fixed_vector::insert_iter - capacity is reached!");

            if (size_type(m_last - insertPosPtr) < numElements)
            {
                // Copy the elements after insert position.
                pointer newLast = AZStd::uninitialized_copy(insertPosPtr, m_last, insertPosPtr + numElements, Internal::is_fast_copy<pointer, pointer>());

                // Number of elements we can assign.
                size_type numInitializedToFill = size_type(m_last - insertPosPtr);

                // get last iterator to fill
                Iterator lastToAssign = first;
                AZStd::advance(lastToAssign, numInitializedToFill);

                // Add new elements to uninitialized elements.
                AZStd::uninitialized_copy(lastToAssign, last, m_last, Internal::is_fast_copy<Iterator, pointer>());
                m_last = newLast;

                // Add assign new data
                Internal::copy(first, lastToAssign, insertPosPtr, Internal::is_fast_copy<Iterator, pointer>());
            }
            else
            {
                // We need to copy data with care, it is overlapping.

                // first copy the data that will not overlap.
                pointer nonOverlap = m_last - numElements;
                pointer newLast = AZStd::uninitialized_copy(nonOverlap, m_last, m_last, Internal::is_fast_copy<pointer, pointer>());

                // move the area with overlapping
                Internal::move_backward(insertPosPtr, nonOverlap, m_last, Internal::is_fast_copy<pointer, pointer>());

                // add new elements
                Internal::copy(first, last, insertPosPtr, Internal::is_fast_copy<Iterator, pointer>());

                m_last = newLast;
            }
        }


        template<class Iterator>
        inline void insert_iter(iterator insertPos, const Iterator& first, const Iterator& last, const input_iterator_tag&)
        {
            iterator start = itearotor(AZSTD_POINTER_ITERATOR_PARAMS(reinterpret_cast<pointer>(&m_data)));
            size_type offset = insertPos - start;

            Iterator iter(first);
            for (; iter != last; ++iter, ++offset)
            {
                insert(start + offset, *iter);
            }
        }
        //#pragma endregion

        //#pragma region Construct interator specializations (construct_iter)
        template <class InputIterator>
        inline void construct_iter(const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            size_type numElements = first;
            value_type value = last;

            // ok so we did not really mean iterators when the called this function.
            AZStd::uninitialized_fill_n(start, numElements, value, Internal::is_fast_fill<pointer>());
            m_last  = start + numElements;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    construct_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            insert(iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)), first, last);
        }
        //#pragma endregion

        //#pragma region Assign iterator specializations (assign_iter)
        template <class InputIterator>
        AZ_FORCE_INLINE void    assign_iter(const InputIterator& numElements, const InputIterator& value, const true_type& /* is_integral<InputIterator> */)
        {
            assign((size_type)numElements, value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    assign_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            pointer start = reinterpret_cast<pointer>(&m_data);
            clear();
            insert(iterator(AZSTD_POINTER_ITERATOR_PARAMS(start)), first, last);
        }
        //#pragma endregion
    protected:
        typename aligned_storage<Capacity* sizeof(T), alignment_of<T>::value>::type   m_data;     ///< pointer with all the data
        pointer         m_last;             ///< Pointer after the last used element.

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        AZ_INLINE void orphan_range(pointer first, pointer last) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "vector::orphan_range - iterator was corrupted!");
                pointer iterPtr = static_cast<iterator*>(iter)->m_iter;

                if (iterPtr >= first && iterPtr <= last)
                {
                    // orphan the iterator
                    iter->m_container = 0;

                    if (iter->m_prevIterator)
                    {
                        iter->m_prevIterator->m_nextIterator = iter->m_nextIterator;
                    }
                    else
                    {
                        m_iteratorList = iter->m_nextIterator;
                    }

                    if (iter->m_nextIterator)
                    {
                        iter->m_nextIterator->m_prevIterator = iter->m_prevIterator;
                    }
                }

                iter = iter->m_nextIterator;
            }
        }
#endif
    };

    //#pragma region Vector equality/inequality
    template< class T, AZStd::size_t Capacity>
    AZ_FORCE_INLINE bool operator==(const fixed_vector<T, Capacity>& a, const fixed_vector<T, Capacity>& b)
    {
        return (a.size() == b.size() && equal(a.begin(), a.end(), b.begin()));
    }

    template< class T, AZStd::size_t Capacity>
    AZ_FORCE_INLINE bool operator!=(const fixed_vector<T, Capacity>& a, const fixed_vector<T, Capacity>& b)
    {
        return !(a == b);
    }
    //#pragma endregion
}

#endif // AZSTD_FIXED_VECTOR_H
#pragma once