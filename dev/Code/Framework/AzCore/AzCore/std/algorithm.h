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
#ifndef AZSTD_ALGORITHM_H
#define AZSTD_ALGORITHM_H 1

#include <AzCore/std/iterator.h>
#include <AzCore/std/functional_basic.h>

namespace AZStd
{
    /**
    * \page Algorithms
    * \subpage SortAlgorithms Sort Algorithms
    *
    * Search algorithms
    */

    //////////////////////////////////////////////////////////////////////////
    // Min, max, clamp
    template<class T>
    T   GetMin(const T& left, const T& right) { return (left < right) ? left : right; }

    template<class T>
    T   min AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return (left < right) ? left : right; }

    template<class T>
    T   GetMax(const T& left, const T& right) { return (left > right) ? left : right; }

    template<class T>
    T   max AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return (left > right) ? left : right; }

    template<class T>
    pair<T, T>   minmax AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return pair<T, T>((left < right) ? left : right, (left > right) ? left : right); }
    
    template<class T>
    T   clamp(const T& val, const T& lower, const T& upper) { return GetMin(upper, GetMax(val, lower)); }
    //////////////////////////////////////////////////////////////////////////

    // for_each.  Apply a function to every element of a range.
    template <class InputIter, class Function>
    AZ_FORCE_INLINE Function for_each(InputIter first, InputIter last, Function f)
    {
        for (; first != last; ++first)
        {
            f(*first);
        }
        return f;
    }

    // count_if
    template <class InputIter, class Predicate>
    typename iterator_traits<InputIter>::difference_type
    count_if(InputIter first, InputIter last, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        typename iterator_traits<InputIter>::difference_type n = 0;
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                ++n;
            }
        }
        return n;
    }

    //////////////////////////////////////////////////////////////////////////
    // Find
    template<class InputIterator, class ComparableToIteratorValue>
    AZ_FORCE_INLINE InputIterator           find(InputIterator first, InputIterator last, const ComparableToIteratorValue& value)
    {
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                break;
            }
        }
        return first;
    }

    AZ_FORCE_INLINE const char*            find(const char* first, const char* last, int value)
    {
        // find first char that matches _Val
        //_DEBUG_RANGE(first, last);
        first = (const char*)::memchr(first, value, last - first);
        return (first == 0 ? last : first);
    }

    AZ_FORCE_INLINE const signed char*     find(const signed char* first, const signed char* last, int value)
    {   // find first signed char that matches _Val
        //_DEBUG_RANGE(first, _Last);
        first = (const signed char*)::memchr(first, value, last - first);
        return (first == 0 ? last : first);
    }

    AZ_FORCE_INLINE const unsigned char*   find(const unsigned char* first, const unsigned char* last, int value)
    {   // find first unsigned char that matches _Val
        //_DEBUG_RANGE(first, _Last);
        first = (const unsigned char*)::memchr(first, value, last - first);
        return (first == 0 ? last : first);
    }

    template<class InputIterator, class Predicate>
    AZ_FORCE_INLINE InputIterator           find_if(InputIterator first, InputIterator last, Predicate pred)
    {
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                break;
            }
        }
        return first;
    }

    template<class InputIterator, class Predicate>
    AZ_FORCE_INLINE InputIterator           find_if_not(InputIterator first, InputIterator last, Predicate pred)
    {
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                break;
            }
        }
        return first;
    }

    // adjacent_find.
    template <class ForwardIter, class BinaryPredicate>
    ForwardIter adjacent_find(ForwardIter first, ForwardIter last, BinaryPredicate binary_pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        if (first == last)
        {
            return last;
        }
        ForwardIter next = first;
        while (++next != last)
        {
            if (binary_pred(*first, *next))
            {
                return first;
            }
            first = next;
        }
        return last;
    }
    template <class ForwardIter>
    AZ_FORCE_INLINE ForwardIter adjacent_find(ForwardIter first, ForwardIter last)
    {
        return AZStd::adjacent_find(first, last, AZStd::equal_to<typename iterator_traits<ForwardIter>::value_type>());
    }

    // find_first_of, with and without an explicitly supplied comparison function.
    template <class InputIter, class ForwardIter, class BinaryPredicate>
    InputIter find_first_of(InputIter first1, InputIter last1, ForwardIter first2, ForwardIter last2, BinaryPredicate comp)
    {
        //DEBUG_CHECK(check_range(first1, last1))
        //DEBUG_CHECK(check_range(first2, last2))
        for (; first1 != last1; ++first1)
        {
            for (ForwardIter iter = first2; iter != last2; ++iter)
            {
                if (comp(*first1, *iter))
                {
                    return first1;
                }
            }
        }
        return last1;
    }

    template <class InputIter, class ForwardIter>
    AZ_FORCE_INLINE InputIter find_first_of(InputIter first1, InputIter last1, ForwardIter first2, ForwardIter last2)
    {
        //DEBUG_CHECK(check_range(first1, last1))
        //DEBUG_CHECK(check_range(first2, last2))
        return AZStd::find_first_of(first1, last1, first2, last2, AZStd::equal_to<typename iterator_traits<InputIter>::value_type>());
    }

    // find_end for forward iterators.
    // find_end for bidirectional iterators.

    //! True if operation returns true for all elements in the range.
    //! True if range is empty.
    template <class InputIter, class UnaryOperation>
    AZ_FORCE_INLINE bool all_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if_not(first, last, operation) == last;
    }

    //! True if operation returns true for any element in the range.
    //! False if the range is empty.
    template <class InputIter, class UnaryOperation>
    AZ_FORCE_INLINE bool any_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if(first, last, operation) != last;
    }

    //! True if operation returns true for no elements in the range.
    //! True if range is empty.
    template <class InputIter, class UnaryOperation>
    AZ_FORCE_INLINE bool none_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if(first, last, operation) == last;
    }

    // transform
    template <class InputIterator, class OutputIterator, class UnaryOperation>
    AZ_FORCE_INLINE OutputIterator  transform(InputIterator first, InputIterator last, OutputIterator result, UnaryOperation operation)
    {
        for (; first != last; ++first, ++result)
        {
            *result = operation(*first);
        }
        return result;
    }
    template <class InputIterator1, class InputIterator2, class OutputIterator, class BinaryOperation>
    AZ_FORCE_INLINE OutputIterator transform(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, BinaryOperation operation)
    {
        for (; first1 != last1; ++first1, ++first2, ++result)
        {
            *result = operation(*first1, *first2);
        }
        return result;
    }

    template<class ForwardIter, class T >
    void replace(ForwardIter first, ForwardIter last, const T& old_value, const T& new_value)
    {
        for (; first != last; ++first)
        {
            if (*first == old_value)
            {
                *first = new_value;
            }
        }
    }

    // replace_if, replace_copy, replace_copy_if
    template <class ForwardIter, class Predicate, class T>
    AZ_FORCE_INLINE void replace_if(ForwardIter first, ForwardIter last, Predicate pred, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                *first = new_value;
            }
        }
    }

    template <class InputIter, class OutputIter, class T>
    AZ_FORCE_INLINE OutputIter replace_copy(InputIter first, InputIter last, OutputIter result, const T& old_value, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first, ++result)
        {
            *result = *first == old_value ? new_value : *first;
        }
        return result;
    }

    template <class Iterator, class OutputIter, class Predicate, class T>
    AZ_FORCE_INLINE OutputIter  replace_copy_if(Iterator first, Iterator last, OutputIter result, Predicate pred, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first, ++result)
        {
            *result = pred(*first) ? new_value : *first;
        }
        return result;
    }

    // generate and generate_n
    template <class ForwardIter, class Generator>
    AZ_FORCE_INLINE void generate(ForwardIter first, ForwardIter last, Generator gen)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            *first = gen();
        }
    }
    template <class OutputIter, class Size, class Generator>
    AZ_FORCE_INLINE void generate_n(OutputIter first, Size n, Generator gen)
    {
        for (; n > 0; --n, ++first)
        {
            *first = gen();
        }
    }
    // remove, remove_if, remove_copy, remove_copy_if
    template <class InputIter, class OutputIter, class T>
    AZ_FORCE_INLINE OutputIter remove_copy(InputIter first, InputIter last, OutputIter result, const T& val)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (!(*first == val))
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }
    template <class InputIter, class OutputIter, class Predicate>
    AZ_FORCE_INLINE OutputIter remove_copy_if(InputIter first, InputIter last, OutputIter result, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }

    template <class ForwardIter, class T>
    ForwardIter remove(ForwardIter first, ForwardIter last, const T& val)
    {
        //DEBUG_CHECK(check_range(first, last))
        first = AZStd::find(first, last, val);
        if (first == last)
        {
            return first;
        }
        else
        {
            ForwardIter next = first;
            return AZStd::remove_copy(++next, last, first, val);
        }
    }

    template <class ForwardIter, class Predicate>
    ForwardIter remove_if(ForwardIter first, ForwardIter last, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        first = AZStd::find_if(first, last, pred);
        if (first == last)
        {
            return first;
        }
        else
        {
            ForwardIter next = first;
            return AZStd::remove_copy_if(++next, last, first, pred);
        }
    }
    // unique and unique_copy
    //template <class InputIter, class OutputIter>
    //OutputIter unique_copy(InputIter first, InputIter last, OutputIter result);

    //template <class InputIter, class OutputIter, class BinaryPredicate>
    //OutputIter unique_copy(InputIter first, InputIter last, OutputIter result, BinaryPredicate binary_pred);

    //template <class ForwardIter>
    //inline ForwardIter unique(ForwardIter first, ForwardIter last) {
    //  first = adjacent_find(first, last);
    //  return unique_copy(first, last, first);
    //}

    //template <class ForwardIter, class BinaryPredicate>
    //inline ForwardIter unique(ForwardIter first, ForwardIter last, BinaryPredicate binary_pred) {
    //      first = adjacent_find(first, last, binary_pred);
    //      return unique_copy(first, last, first, binary_pred);
    //}
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Heap
    // \todo move to heap.h
    namespace Internal
    {
        template <class RandomAccessIterator, class Distance, class T>
        AZ_INLINE void push_heap(RandomAccessIterator& first, Distance holeIndex, Distance topIndex, const T& value)
        {
            Distance parent = (holeIndex - 1) / 2;
            while (holeIndex > topIndex && *(first + parent) < value)
            {
                *(first + holeIndex) = *(first + parent);
                holeIndex = parent;
                parent = (holeIndex - 1) / 2;
            }
            *(first + holeIndex) = value;
        }

        template <class RandomAccessIterator, class Distance, class T, class Compare>
        AZ_INLINE void push_heap(RandomAccessIterator& first, Distance holeIndex, Distance topIndex, const T& value, Compare comp)
        {
            Distance parent = (holeIndex - 1) / 2;
            while (holeIndex > topIndex && comp(*(first + parent), value))
            {
                *(first + holeIndex) = *(first + parent);
                holeIndex = parent;
                parent = (holeIndex - 1) / 2;
            }
            *(first + holeIndex) = value;
        }

        template <class RandomAccessIterator, class Distance, class T>
        AZ_INLINE void adjust_heap(RandomAccessIterator& first, Distance holeIndex, Distance length, const T& value)
        {
            Distance topIndex = holeIndex;
            Distance secondChild = 2 * holeIndex + 2;
            while (secondChild < length)
            {
                if (*(first + secondChild) < *(first + (secondChild - 1)))
                {
                    --secondChild;
                }
                *(first + holeIndex) = *(first + secondChild);
                holeIndex = secondChild;
                secondChild = 2 * (secondChild + 1);
            }
            if (secondChild == length)
            {
                *(first + holeIndex) = *(first + (secondChild - 1));
                holeIndex = secondChild - 1;
            }
            AZStd::Internal::push_heap(first, holeIndex, topIndex, value);
        }

        template <class RandomAccessIterator, class Distance, class T, class Compare>
        AZ_INLINE void adjust_heap(RandomAccessIterator& first, Distance holeIndex, Distance length, const T& value, Compare comp)
        {
            Distance topIndex = holeIndex;
            Distance secondChild = 2 * holeIndex + 2;
            while (secondChild < length)
            {
                if (comp(*(first + secondChild), *(first + (secondChild - 1))))
                {
                    --secondChild;
                }
                *(first + holeIndex) = *(first + secondChild);
                holeIndex = secondChild;
                secondChild = 2 * (secondChild + 1);
            }
            if (secondChild == length)
            {
                *(first + holeIndex) = *(first + (secondChild - 1));
                holeIndex = secondChild - 1;
            }
            AZStd::Internal::push_heap(first, holeIndex, topIndex, value, comp);
        }

        template<class RandomAccessIterator>
        AZ_INLINE void debug_ordered(RandomAccessIterator first, RandomAccessIterator last)
        {
            // test if range is a heap ordered by operator<
            if (first != last)
            {
                for (RandomAccessIterator iter = first; ++first != last; ++iter)
                {
                    AZ_Assert(!(*iter < *first), "AZStd::debug_ordered - invalid order");
                    if (++first == last)
                    {
                        break;
                    }
                    AZ_Assert(!(*iter < *first), "AZStd::debug_ordered - invalid order");
                }
            }
        }

        template<class RandomAccessIterator, class Compare>
        AZ_INLINE void debug_ordered(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
        {
            (void)comp;
            // test if range is a heap ordered by a predicate
            if (first != last)
            {
                for (RandomAccessIterator iter = first; ++first != last; ++iter)
                {
                    AZ_Assert(!comp(*iter, *first), "AZStd::debug_ordered - invalid order");
                    if (++first == last)
                    {
                        break;
                    }
                    AZ_Assert(!comp(*iter, *first), "AZStd::debug_ordered - invalid order");
                }
            }
        }
    }

    /**
    * \defgroup Heaps Heap functions
    * If you want to make sure you heaps are valid (if you use, make_heap, push_heap, pop_heap)
    * you can define \ref AZSTD_DEBUG_HEAP_IMPLEMENTATION in your configuration file. This will verify
    * that heaps are valid, before every operation.
    * @{
    */

    /// Pushes values to the heap using AZStd::less predicate. \ref CStd
    template<class RandomAccessIterator>
    AZ_FORCE_INLINE void push_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        RandomAccessIterator preLast = last - 1;
        Internal::debug_ordered(first, preLast);
#endif
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(last - 1);
        AZStd::Internal::push_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type((last - first) - 1), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), value);
    }
    /// Pushes values to the heap using provided binary predicate Compare. \ref CStd
    template<class RandomAccessIterator, class Compare>
    AZ_FORCE_INLINE void push_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        RandomAccessIterator preLast = last - 1;
        Internal::debug_ordered(first, preLast, comp);
#endif
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(last - 1);
        AZStd::Internal::push_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type((last - first) - 1), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), value, comp);
    }
    /// Prepares heap for popping a value using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    AZ_FORCE_INLINE void pop_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last);
#endif
        RandomAccessIterator result = last - 1;
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(result - first), value);
    }
    /// Prepares heap for popping a value using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    AZ_FORCE_INLINE void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last, comp);
#endif
        RandomAccessIterator result = last - 1;
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(result - first), value, comp);
    }

    /// Same as AZStd::pop_heap using AZStd::less predicate, but allows you to provide iterator where to store the result.
    template <class RandomAccessIterator>
    AZ_FORCE_INLINE void pop_heap(RandomAccessIterator& first, RandomAccessIterator& last, RandomAccessIterator& result)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last);
#endif
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(last - first), value);
    }
    /// Same as AZStd::pop_heap using Compare predicate, but allows you to provide iterator where to store the result.
    template <class RandomAccessIterator, class Compare>
    AZ_FORCE_INLINE void pop_heap(RandomAccessIterator& first, RandomAccessIterator& last, RandomAccessIterator& result, Compare comp)
    {
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last, comp);
#endif
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(last - first), value, comp);
    }

    /// Make a heap from an array of values, using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    void make_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        if (last - first < 2)
        {
            return;
        }
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type length = last - first;
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type parent = (length - 2) / 2;

        for (;; )
        {
            typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(first + parent);
            AZStd::Internal::adjust_heap(first, parent, length, value);
            if (parent == 0)
            {
                return;
            }
            --parent;
        }
    }

    /// Make a heap from an array of values, using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    void make_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        if (last - first < 2)
        {
            return;
        }
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type length = last - first;
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type parent = (length - 2) / 2;

        for (;; )
        {
            typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(first + parent);
            AZStd::Internal::adjust_heap(first, parent, length, value, comp);
            if (parent == 0)
            {
                return;
            }
            --parent;
        }
    }

    /// Preforms a heap sort on a range of values, using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    AZ_FORCE_INLINE void sort_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        for (; last - first > 1; --last)
        {
            AZStd::pop_heap(first, last);
        }
    }
    /// Preforms a heap sort on a range of values, using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    AZ_FORCE_INLINE void    sort_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        for (; last - first > 1; --last)
        {
            AZStd::pop_heap(first, last, comp);
        }
    }
    /// @}
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Search
    // TEMPLATE FUNCTION lower_bound
    template<class ForwardIterator, class T>
    ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // find first element not before value, using operator<
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last);
#endif
        typename iterator_traits<ForwardIterator>::difference_type count, count2;
        count = AZStd::distance(first, last);

        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            count2 = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, count2);

            if (*mid < value)
            {
                first = ++mid;
                count -= count2 + 1;
            }
            else
            {
                count = count2;
            }
        }
        return first;
    }

    template<class ForwardIterator, class T, class Compare>
    ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
    {
        // find first element not before value, using compare predicate
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last, comp);
#endif
        typename iterator_traits<ForwardIterator>::difference_type count, count2;
        count = AZStd::distance(first, last);

        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            count2 = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, count2);

            if (comp(*mid, value))
            {
                first = ++mid;
                count -= count2 + 1;
            }
            else
            {
                count = count2;
            }
        }
        return first;
    }


    // TEMPLATE FUNCTION upper_bound
    template<class ForwardIterator, class T>
    ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // find first element that value is before, using operator<
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last);
#endif
        typename iterator_traits<ForwardIterator>::difference_type count, step;
        count = AZStd::distance(first, last);
        for (; 0 < count; )
        {   // divide and conquer, find half that contains answer
            step = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, step);

            if (!(value < *mid))
            {
                first = ++mid;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }
        return first;
    }
    template<class ForwardIterator, class T, class Compare>
    ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
    {
        // find first element not before value, using compare predicate
#ifdef AZSTD_DEBUG_HEAP_IMPLEMENTATION
        Internal::debug_ordered(first, last, comp);
#endif
        typename iterator_traits<ForwardIterator>::difference_type count, step;
        count = AZStd::distance(first, last);
        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            step = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, step);

            if (!comp(value, *mid))
            {
                first = ++mid;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }
        return first;
    }

    template <class ForwardIterator1, class ForwardIterator2>
    ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
    {
        if (first2 == last2)
        {
            return first1;                 // specified in C++11
        }
        while (first1 != last1)
        {
            ForwardIterator1 it1 = first1;
            ForwardIterator2 it2 = first2;
            while (*it1 == *it2)
            {
                ++it1;
                ++it2;
                if (it2 == last2)
                {
                    return first1;
                }
                if (it1 == last1)
                {
                    return last1;
                }
            }
            ++first1;
        }
        return last1;
    }

    template <class ForwardIterator1, class ForwardIterator2, class Compare>
    ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2, Compare comp)
    {
        if (first2 == last2)
        {
            return first1;                 // specified in C++11
        }
        while (first1 != last1)
        {
            ForwardIterator1 it1 = first1;
            ForwardIterator2 it2 = first2;
            while (comp(*it1, *it2))
            {
                ++it1;
                ++it2;
                if (it2 == last2)
                {
                    return first1;
                }
                if (it1 == last1)
                {
                    return last1;
                }
            }
            ++first1;
        }
        return last1;
    }

    // todo search_n
    //////////////////////////////////////////////////////////////////////////

    template<class InputIterator1, class InputIterator2>
    inline bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        {
            if (*first1 < *first2)
            {
                return true;
            }
            if (*first2 < *first1)
            {
                return false;
            }
        }
        return first1 == last1 && first2 != last2;
    }

    inline bool lexicographical_compare(const unsigned char* first1, const unsigned char* last1, const unsigned char* first2, const unsigned char* last2)
    {
        ptrdiff_t len1 = last1 - first1;
        ptrdiff_t len2 = last2 - first2;
        int res = ::memcmp(first1, first2, len1 < len2 ? len1 : len2);
        return (res < 0 || (res == 0 && len1 < len2));
    }

    template<class InputIterator1, class InputIterator2, class Compare>
    bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, Compare comp)
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        {
            if (comp(*first1, *first2))
            {
                return true;
            }
            if (comp(*first2, *first1))
            {
                return false;
            }
        }
        return first1 == last1 && first2 != last2;
    }

    //////////////////////////////////////////////////////////////////////////
    /**
    * Endian swapping templates
    * note you can specialize any type anywhere in the code if you fell like it.
    */
    template<typename T, AZStd::size_t size>
    struct endian_swap_impl
    {
        // this function is implemented for each specialization.
        static AZ_FORCE_INLINE void swap_data(T& data);
    };

    // specialization for 1 byte type (don't swap)
    template<typename T>
    struct endian_swap_impl<T, 1>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)  { (void)data; }
    };

    // specialization for 2 byte type
    template<typename T>
    struct endian_swap_impl<T, 2>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T               m_data;
                unsigned short  m_ushort;
            };
            SafeCast* sc = (SafeCast*)&data;
            unsigned short x = sc->m_ushort;
            sc->m_ushort = (x >> 8) | (x << 8);
        }
    };

    // specialization for 4 byte type
    template<typename T>
    struct endian_swap_impl<T, 4>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T               m_data;
                unsigned int    m_uint;
            };
            SafeCast* sc = (SafeCast*)&data;
            unsigned int x = sc->m_uint;
            sc->m_uint = (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
        }
    };

#   define AZ_UINT64_CONST(_Value) _Value
#   define AZ_INT64_CONST(_Value) _Value

    template<typename T>
    struct endian_swap_impl<T, 8>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T           m_data;
                AZ::u64     m_uint64;
            };
            SafeCast* sc = (SafeCast*)&data;
            AZ::u64 x = sc->m_uint64;
            sc->m_uint64 = (x >> 56) |
                ((x << 40) & AZ_UINT64_CONST(0x00FF000000000000)) |
                ((x << 24) & AZ_UINT64_CONST(0x0000FF0000000000)) |
                ((x << 8)  & AZ_UINT64_CONST(0x000000FF00000000)) |
                ((x >> 8)  & AZ_UINT64_CONST(0x00000000FF000000)) |
                ((x >> 24) & AZ_UINT64_CONST(0x0000000000FF0000)) |
                ((x >> 40) & AZ_UINT64_CONST(0x000000000000FF00)) |
                (x << 56);
        }
    };

    template<typename T>
    AZ_FORCE_INLINE void endian_swap(T& data)
    {
        endian_swap_impl<T, sizeof(T)>::swap_data(data);
    }

    template<typename Iterator>
    AZ_FORCE_INLINE void endian_swap(Iterator first, Iterator last)
    {
        for (; first != last; ++first)
        {
            AZStd::endian_swap(*first);
        }
    }
    //
    //////////////////////////////////////////////////////////////////////////
}

#endif // AZSTD_ALGORITHM_H
#pragma once