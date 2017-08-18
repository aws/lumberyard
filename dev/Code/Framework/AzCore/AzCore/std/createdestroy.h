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
#ifndef AZSTD_CREATE_DESTROY_H
#define AZSTD_CREATE_DESTROY_H 1

#include <AzCore/std/iterator.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/has_trivial_destructor.h>
#include <AzCore/std/typetraits/has_trivial_constructor.h>
#include <AzCore/std/typetraits/has_trivial_assign.h>
#include <AzCore/std/typetraits/has_trivial_copy.h>

namespace AZStd
{
    /**
     * Internal code should be used only by AZSTD. These are as flat as possible (less function calls) implementation, that
     * should be fast and easy in debug. You can call this functions, but it's better to use the specialized one in the
     * algorithms.
     */
    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        /**
         * Calls the destructor on a range of objects, defined by start and end forward iterators.
         */
        template<class InputIterator, class ValueType = typename iterator_traits<InputIterator>::value_type, class HasTrivialDestrustor = typename has_trivial_destructor<typename iterator_traits<InputIterator>::value_type>::type >
        struct destroy
        {
            static inline void range(InputIterator first, InputIterator& last)
            {
                for (; first != last; ++first)
                {
                    first->~ValueType();
                }
            }

            static inline void single(InputIterator& iter)
            {
                (void)iter;
                iter->~ValueType();
            }
        };

        /**
         * specialization for type with trivial destructor. We don't call it.
         */
        template<class InputIterator, class ValueType >
        struct destroy<InputIterator, ValueType, true_type>
        {
            static AZ_FORCE_INLINE void range(InputIterator first, InputIterator& last) { (void)first; (void)last; }
            static AZ_FORCE_INLINE void single(InputIterator& iter) { (void)iter; }
        };

        /**
         * Default object construction.
         */
        template<class InputIterator, class ValueType = typename iterator_traits<InputIterator>::value_type, class HasTrivialConstuctor = typename has_trivial_constructor<typename iterator_traits<InputIterator>::value_type>::type >
        struct construct
        {
            static AZ_FORCE_INLINE void range(InputIterator first, InputIterator& last)
            {
                for (; first != last; ++first)
                {
                    ::new (static_cast<void*>(&*first))ValueType();
                }
            }

            static AZ_FORCE_INLINE void range(InputIterator first, InputIterator& last, const ValueType& value)
            {
                for (; first != last; ++first)
                {
                    ::new (static_cast<void*>(&*first))ValueType(value);
                }
            }

            static AZ_FORCE_INLINE void single(InputIterator& iter)
            {
                ::new (static_cast<void*>(&*iter))ValueType();
            }

            template<class ... InputArguments>
            static AZ_FORCE_INLINE void single(InputIterator& iter, InputArguments&& ... inputArguments)
            {
                ::new (static_cast<void*>(&*iter))ValueType(AZStd::forward<InputArguments>(inputArguments) ...);
            }
        };

        template<class InputIterator, class ValueType>
        struct construct<InputIterator, ValueType, true_type>
        {
            static AZ_FORCE_INLINE void range(InputIterator first, InputIterator& last) { (void)first; (void)last; }
            static AZ_FORCE_INLINE void range(InputIterator first, InputIterator& last, const ValueType& value)
            {
                for (; first != last; ++first)
                {
                    *first = value;
                }
            }
            static AZ_FORCE_INLINE void single(InputIterator& iter) { (void)iter; }
            static AZ_FORCE_INLINE void single(InputIterator& iter, const ValueType& value)
            {
                *iter = value;
            }

            template<class ... InputArguments>
            static AZ_FORCE_INLINE void single(InputIterator& iter, InputArguments&& ... inputArguments)
            {
                ::new (static_cast<void*>(&*iter))ValueType(AZStd::forward<InputArguments>(inputArguments) ...);
            }
        };
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Sequence copy. If we use optimized version we use memcpy.
        /**
        * Helper class to determine if we have apply fast copy. There are 2 conditions
        * - trivial copy ctor.
        * - all iterators are continuous iterators (pointers)
        */
        template<class InputIterator, class ResultIterator>
        struct is_fast_copy_helper
        {
            typedef typename iterator_traits<ResultIterator>::value_type value_type;
            AZSTD_STATIC_CONSTANT(bool, value = (::AZStd::type_traits::ice_and< AZStd::has_trivial_copy< value_type >::value
                                                     , is_continuous_random_access_iterator_cat< typename iterator_traits<InputIterator>::iterator_category >::value
                                                     , is_continuous_random_access_iterator_cat< typename iterator_traits<ResultIterator>::iterator_category >::value
                                                     >::value));
        };

        // Use this trait to to determine copy mode, based on the iterator category and object copy properties,
        // Use it when when you call  uninitialized_copy, Internal::copy, Internal::move, etc.
        template< typename InputIterator, typename ResultIterator >
        struct is_fast_copy
            : public ::AZStd::integral_constant<bool, ::AZStd::Internal::is_fast_copy_helper<InputIterator, ResultIterator>::value> {};

        template <class InputIterator, class ForwardIterator>
        inline ForwardIterator copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
        {
            InputIterator iter(first);
            for (; iter != last; ++result, ++iter)
            {
                *result = *iter;
            }

            return result;
        }

        // Specialized copy for continuous iterators (pointers) and trivial copy type.
        template <class InputIterator, class ForwardIterator>
        inline ForwardIterator copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
        {
            // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
            AZ_STATIC_ASSERT(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Size of value types must match for a trivial copy");
            AZStd::size_t numElements = last - first;
            if (numElements > 0)
            {
                AZ_Assert((&*result < &*first) || (&*result >= (&*first + numElements)), "AZStd::copy memory overlaps use AZStd::move_backward!");
                AZ_Assert(((&*result + numElements) <= &*first) || ((&*result + numElements) > (&*first + numElements)), "AZStd::copy memory overlaps use AZStd::move_backward!");
                /*AZSTD_STL::*/ memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
            }
            return result + numElements;
        }

        // Copy backward.
        template <class BidirectionalIterator1, class BidirectionalIterator2>
        inline BidirectionalIterator2 copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const false_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
        {
            BidirectionalIterator1 iter(last);
            while (first != iter)
            {
                *--result = *--iter;
            }

            return result;
        }

        // Specialized copy for continuous iterators (pointers) and trivial copy type.
        template <class BidirectionalIterator1, class BidirectionalIterator2>
        inline BidirectionalIterator2 copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const true_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
        {
            // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
            AZ_STATIC_ASSERT(sizeof(typename iterator_traits<BidirectionalIterator1>::value_type) == sizeof(typename iterator_traits<BidirectionalIterator2>::value_type), "Size of value types must match for a trivial copy");
            AZStd::size_t numElements = last - first;
            if (numElements > 0)
            {
                result -= numElements;
                AZ_Assert((&*result < &*first) || (&*result >= (&*first + numElements)), "AZStd::copy_backward memory overlaps use AZStd::move_backward!");
                AZ_Assert(((&*result + numElements) <= &*first) || ((&*result + numElements) > (&*first + numElements)), "AZStd::copy_backward memory overlaps use AZStd::move_backward!");
                /*AZSTD_STL::*/ memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<BidirectionalIterator1>::value_type));
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Sequence move. If we use optimized version we use memmove.
        template <class InputIterator, class ForwardIterator>
        inline ForwardIterator move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
        {
            InputIterator iter(first);
#ifdef AZ_HAS_RVALUE_REFS
            for (; iter != last; ++result, ++iter)
            {
                *result = AZStd::move(*iter);
            }
#else
            for (; iter != last; ++result, ++iter)
            {
                *result = *iter;
            }
#endif
            return result;
        }

        // Specialized copy for continuous iterators (pointers) and trivial copy type.
        template <class InputIterator, class ForwardIterator>
        inline ForwardIterator move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
        {
            AZ_STATIC_ASSERT(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Size of value types must match for a trivial copy");
            AZStd::size_t numElements = last - first;
            if (numElements > 0)
            {
                /*AZSTD_STL::*/
                memmove(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
            }
            return result + numElements;
        }

        // For generic iterators, move is the same as copy.
        template <class BidirectionalIterator1, class BidirectionalIterator2>
        inline BidirectionalIterator2 move_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const false_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
        {
            BidirectionalIterator1 iter(last);
#ifdef AZ_HAS_RVALUE_REFS
            while (first != iter)
            {
                *--result = AZStd::move(*--iter);
            }
#else
            while (first != iter)
            {
                *--result = *--iter;
            }
#endif
            return result;
        }

        // Specialized copy for continuous iterators (pointers) and trivial copy type.
        template <class BidirectionalIterator1, class BidirectionalIterator2>
        inline BidirectionalIterator2 move_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const true_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
        {
            // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
            AZ_STATIC_ASSERT(sizeof(typename iterator_traits<BidirectionalIterator1>::value_type) == sizeof(typename iterator_traits<BidirectionalIterator2>::value_type), "Size of value types must match for a trivial copy");
            AZStd::size_t numElements = last - first;
            result -= numElements;
            if (numElements > 0)
            {
                /*AZSTD_STL::*/
                memmove(&*result, &*first, numElements * sizeof(typename iterator_traits<BidirectionalIterator1>::value_type));
            }
            return result;
        }
        // end of sequence move.
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Fill
        /**
        * Helper class to determine if we have apply fast fill. There are 3 conditions
        * - trivial assign
        * - size of type == 1 (chars) to use memset
        * - continuous iterators (pointers)
        */
        template<class Iterator>
        struct is_fast_fill_helper
        {
            typedef typename iterator_traits<Iterator>::value_type value_type;
            AZSTD_STATIC_CONSTANT(bool, value = (::AZStd::type_traits::ice_and< AZStd::has_trivial_assign< value_type >::value
                                                     , (bool)(sizeof(value_type) == 1)
                                                     , is_continuous_random_access_iterator_cat< typename iterator_traits<Iterator>::iterator_category >::value
                                                     >::value));
        };

        // Use this trait to to determine fill mode, based on the iterator, value size, etc.
        // Use it when you call uninitialized_fill, uninitialized_fill_n, fill and fill_n.
        template< typename Iterator >
        struct is_fast_fill
            : public ::AZStd::integral_constant<bool, ::AZStd::Internal::is_fast_fill_helper<Iterator>::value>
        {};

        template <class ForwardIterator, class T>
        inline void fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
        {
            ForwardIterator iter(first);
            for (; iter != last; ++iter)
            {
                *iter = value;
            }
        }
        // Specialized version when we continuous iterators
        template <class ForwardIterator, class T>
        inline void fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
        {
            AZStd::size_t numElements = last - first;
            if (numElements > 0)
            {
                /*AZSTD_STL::*/
                memset((void*)&*first,  *reinterpret_cast<const unsigned char*>(&value), numElements);
            }
        }

        template <class ForwardIterator, class Size, class T>
        inline void fill_n(ForwardIterator first, Size numElements, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
        {
            for (; numElements--; ++first)
            {
                *first = value;
            }
        }

        // Specialized version when we continuous iterators
        template <class ForwardIterator, class Size, class T>
        inline void fill_n(ForwardIterator first, Size numElements, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
        {
            if (numElements > 0)
            {
                /*AZSTD_STL::*/
                memset(&*first,  *reinterpret_cast<const unsigned char*>(&value), numElements);
            }
        }
    }

    /**
 * Specialized algorithms 20.4.4. We extend that by adding faster specialized versions when we have trivial assign type.
 */
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);
        for (; iter != last; ++result, ++iter)
        {
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*iter);
        }

        return result;
    }

    // Specialized copy for continuous iterators and trivial copy type.
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        AZ_STATIC_ASSERT(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Value type sizes much match for a trivial copy");
        AZStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    // Uninitialized copy backward. This is an extension (not part of the \CStd).
    //template <class BidirectionalIterator1, class BidirectionalIterator2>
    //inline BidirectionalIterator2 uninitialized_copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const false_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    //{
    //  BidirectionalIterator1 iter(last);
    //  while(first != iter)
    //  {
    //      ::new (static_cast<void*>(&*--result)) typename iterator_traits<BidirectionalIterator2>::value_type(*--iter);
    //  }
    //  return result;
    //}

    //// Specialized copy for continuous iterators (pointers) and trivial copy type.
    //template <class BidirectionalIterator1, class BidirectionalIterator2>
    //inline BidirectionalIterator2 uninitialized_copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const true_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    //{
    //  // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
    //  AZ_STATIC_ASSERT(sizeof(typename iterator_traits<BidirectionalIterator1>::value_type)==sizeof(typename iterator_traits<BidirectionalIterator2>::value_type));
    //  AZStd::size_t numElements = last - first;
    //  result -= numElements;
    //  /*AZSTD_STL::*/memcpy(&*result,&*last,numElements*sizeof(typename iterator_traits<BidirectionalIterator1>::value_type));
    //  return result;
    //}

    template <class ForwardIterator, class T>
    inline void uninitialized_fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        ForwardIterator iter(first);
        for (; iter != last; ++iter)
        {
            ::new (static_cast<void*>(&*iter)) typename iterator_traits<ForwardIterator>::value_type(value);
        }
    }

    // Specialized version when we continuous iterators (pointers), trivial assign and the sizeof(T) == 1
    template <class ForwardIterator, class T>
    inline void uninitialized_fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        AZStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memset(&*first,  *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }

    template <class ForwardIterator, class Size, class T>
    inline void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        for (; numElements--; ++first)
        {
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(value);
        }
    }

    // Specialized version when we continuous iterators, trivial assign and the sizeof(T) == 1
    template <class ForwardIterator, class Size, class T>
    inline void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        if (numElements)
        {
            /*AZSTD_STL::*/
            memset(&*first,  *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }

    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);

#ifdef AZ_HAS_RVALUE_REFS
        for (; iter != last; ++result, ++iter)
        {
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(AZStd::move(*iter));
        }
#else
        for (; iter != last; ++result, ++iter)
        {
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*iter);
        }
#endif
        return result;
    }
    // Specialized copy for continuous iterators and trivial move type. (since the object is POD we will just perform a copy)
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        AZ_STATIC_ASSERT(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Value type sizes much match for a trivial copy");
        AZStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    // 25.3.1 Copy
    template<class InputIterator, class OutputIterator>
    OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
    {
        return AZStd::Internal::copy(first, last, result, AZStd::Internal::is_fast_copy<InputIterator, OutputIterator>());
    }

    //template<class InputIterator, class Size, class OutputIterator>
    //OutputIterator copy_n(InputIterator first, Size n,OutputIterator result)
    //{
    //  for(Size i = 0; i < n; ++i,++first,++result)
    //      *result = *first
    //  return result;
    //}

    /*template<class InputIterator, class OutputIterator, class Predicate>
    OutputIterator copy_if(InputIterator first, InputIterator last,OutputIterator result, Predicate pred)
    {
        for(;first != last; ++first)
        {
            if(pred(*first))
                *result++=*first;
        }
        return result;
    }*/

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2  copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return AZStd::Internal::copy_backward(first, last, result, AZStd::Internal::is_fast_copy<BidirectionalIterator1, BidirectionalIterator2>());
    }

    // 25.3.2 Move
    template<class InputIterator, class OutputIterator>
    OutputIterator move(InputIterator first, InputIterator last, OutputIterator result)
    {
        return AZStd::Internal::move(first, last, result, AZStd::Internal::is_fast_copy<InputIterator, OutputIterator>());
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2  move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return AZStd::Internal::move_backward(first, last, result, AZStd::Internal::is_fast_copy<BidirectionalIterator1, BidirectionalIterator2>());
    }
}

#endif // AZSTD_CREATE_DESTROY_H
#pragma once


