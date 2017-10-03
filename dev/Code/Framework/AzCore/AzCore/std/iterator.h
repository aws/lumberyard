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
#ifndef AZSTD_ITERATOR_H
#define AZSTD_ITERATOR_H 1

#include <AzCore/std/base.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/integral_constant.h>

#include <AzCore/std/typetraits/is_base_of.h> // use by ConstIteratorCast

#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
#   if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) // ACCEPTED_USE
// we use internal global lock
#       include <xutility>
#       define AZ_GLOBAL_SCOPED_LOCK(_MUTEX)    std::_Lockit    l(_LOCK_DEBUG)
#   else // !AZ_PLATFORM_WINDOWS
#       include <AzCore/std/parallel/mutex.h>
#       define AZ_GLOBAL_SCOPED_LOCK(_MUTEX)    AZStd::lock_guard<AZStd::mutex> l(_MUTEX)
#   endif
#endif

namespace AZStd
{
    // Everything unless specified is based on C++ standard 24 (lib.iterators).

    /// Identifying tag for input iterators.
    struct input_iterator_tag {};
    /// Identifying tag for output iterators.
    struct output_iterator_tag {};
    /// Identifying tag for forward iterators.
    struct forward_iterator_tag
        : public input_iterator_tag {};
    /// Identifying tag for bidirectional iterators.
    struct bidirectional_iterator_tag
        : public forward_iterator_tag {};
    /// Identifying tag for random-access iterators.
    struct random_access_iterator_tag
        : public bidirectional_iterator_tag {};

    /**
     *  AZStd continuous random access iterator. Stl extension.
     *  Random access iterator doesn't not guarantee that the random access iterator points to a continuous block of memory.
     *  This is why we add this iterator tag. We will be able to do some optimizations that random access iterator
     *  can not do.
     */
    struct continuous_random_access_iterator_tag
        : public random_access_iterator_tag {};

    /**
     * STL iterator template definition.
     */
    template <class Category, class T, class Distance = ptrdiff_t, class Pointer = T*, class Reference = T&>
    struct iterator
    {
        typedef T           value_type;
        typedef Distance    difference_type;
        typedef Pointer     pointer;
        typedef Reference   reference;
        typedef Category    iterator_category;
    };

    /**
     * Default iterator traits struct.
     */
    template <class Iterator>
    struct iterator_traits
    {
        typedef typename Iterator::iterator_category iterator_category;
        typedef typename Iterator::value_type        value_type;
        typedef typename Iterator::difference_type   difference_type;
        typedef typename Iterator::pointer           pointer;
        typedef typename Iterator::reference         reference;
    };

    /**
     * Default STL pointer specializations use random_access_iterator as a category.
     * We do refine this as being a continuous iterator.
     */
    template <class T>
    struct iterator_traits<T*>
    {
        typedef ptrdiff_t   difference_type;
        typedef T           value_type;
        typedef T*          pointer;
        typedef T&          reference;
        typedef continuous_random_access_iterator_tag iterator_category;
    };

    template <class T>
    struct iterator_traits<const T*>
    {
        typedef ptrdiff_t   difference_type;
        typedef T           value_type;
        typedef const       T* pointer;
        typedef const       T& reference;
        typedef continuous_random_access_iterator_tag iterator_category;
    };

    /**
     * Reverse iterator implementation.
     */
    template<class Iterator>
    class reverse_iterator
        : public
        iterator<   typename iterator_traits<Iterator>::iterator_category,
            typename iterator_traits<Iterator>::value_type,
            typename iterator_traits<Iterator>::difference_type,
            typename iterator_traits<Iterator>::pointer,
            typename iterator_traits<Iterator>::reference >
    {
    protected:
        typedef reverse_iterator<Iterator>  this_type;
        typedef Iterator                    iterator_type;
        Iterator m_current;
    public:
        typedef typename iterator_traits<Iterator>::reference       reference;
        typedef typename iterator_traits<Iterator>::pointer         pointer;
        typedef typename iterator_traits<Iterator>::difference_type difference_type;

        AZ_FORCE_INLINE reverse_iterator() {}
        AZ_FORCE_INLINE explicit reverse_iterator(iterator_type iter)
            : m_current(iter)  {}
        AZ_FORCE_INLINE reverse_iterator(const this_type& rhs)
            : m_current(rhs.m_current)       {}
        template<class U>
        AZ_FORCE_INLINE reverse_iterator(const reverse_iterator<U>& u)
            : m_current(u.base())    {}
        template <class U>
        AZ_FORCE_INLINE reverse_iterator& operator=(const reverse_iterator<U>& u)               { m_current = u.base(); }
        AZ_FORCE_INLINE iterator_type      base() const                                         { return m_current; }

        AZ_FORCE_INLINE reference operator*() const
        {
            iterator_type tmp = m_current;
            --tmp;
            return *tmp;
        }
        AZ_FORCE_INLINE pointer operator->() const
        {
            iterator_type tmp = m_current;
            --tmp;
            return &(*tmp);
        }

        AZ_FORCE_INLINE this_type& operator++()
        {
            --m_current;
            return *this;
        }
        AZ_FORCE_INLINE this_type  operator++(int)
        {
            this_type tmp = *this;
            --m_current;
            return tmp;
        }
        AZ_FORCE_INLINE this_type& operator--()
        {
            ++m_current;
            return *this;
        }
        AZ_FORCE_INLINE this_type  operator--(int)
        {
            this_type tmp = *this;
            ++m_current;
            return tmp;
        }

        AZ_FORCE_INLINE reverse_iterator operator+ (difference_type offset) const   { return this_type(m_current - offset); }
        AZ_FORCE_INLINE reverse_iterator& operator+=(difference_type offset)
        {
            m_current -= offset;
            return *this;
        }
        AZ_FORCE_INLINE reverse_iterator operator- (difference_type offset) const   { return this_type(m_current + offset); }
        AZ_FORCE_INLINE reverse_iterator& operator-=(difference_type offset)
        {
            m_current += offset;
            return *this;
        }
        AZ_FORCE_INLINE reference operator[](difference_type offset) const
        {
            return m_current[-offset - 1];
        }
    };

    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator==(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)  { return x.base() == y.base(); }
    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator<(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)  { return x.base() > y.base(); }
    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator!=(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y) { return x.base() != y.base(); }
    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator>(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)   { return x.base() < y.base(); }
    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator>=(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y) { return x.base() <= y.base(); }
    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE bool operator<=(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y) { return x.base() >= y.base(); }

    template <class Iterator1, class Iterator2>
    AZ_FORCE_INLINE typename reverse_iterator<Iterator1>::difference_type operator-(const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    {
        return y.base() - x.base();
    }

    template <class Iterator>
    AZ_FORCE_INLINE reverse_iterator<Iterator> operator+(typename reverse_iterator<Iterator>::difference_type offset, const reverse_iterator<Iterator>& x)
    {
        return x + offset;
    }

#ifdef AZ_HAS_RVALUE_REFS
    /**
     * Move iterator. (24.4.3)
     */
    template<class Iterator>
    class move_iterator
        : public
        iterator<   typename iterator_traits<Iterator>::iterator_category,
            typename iterator_traits<Iterator>::value_type,
            typename iterator_traits<Iterator>::difference_type,
            typename iterator_traits<Iterator>::pointer,
            typename iterator_traits<Iterator>::value_type&& >
    {
    public:
        using iterator_base = iterator<typename iterator_traits<Iterator>::iterator_category,
            typename iterator_traits<Iterator>::value_type,
            typename iterator_traits<Iterator>::difference_type,
            typename iterator_traits<Iterator>::pointer,
            typename iterator_traits<Iterator>::value_type&&>;

        typedef typename iterator_base::reference       reference;
        typedef typename iterator_base::pointer         pointer;
        typedef typename iterator_base::difference_type difference_type;

        typedef move_iterator<Iterator>     this_type;
        typedef Iterator                    iterator_type;

        move_iterator() {}
        explicit move_iterator(iterator_type rhs)
            : m_current(rhs) {}

        template<class U>
        move_iterator(const move_iterator<U>& rhs)
            : m_current(rhs.base()) {}

        template<class U>
        this_type& operator=(const move_iterator<U>& rhs)
        {
            m_current = rhs.base();
            return (*this);
        }

        Iterator base() const           { return m_current; }
        reference operator*() const     { return (AZStd::move(*m_current)); }
        pointer operator->() const      { return (&**this); }
        this_type& operator++()
        {
            ++m_current;
            return (*this);
        }

        this_type operator++(int)
        {
            this_type tmp = *this;
            ++m_current;
            return tmp;
        }

        this_type& operator--()
        {
            --m_current;
            return (*this);
        }

        this_type operator--(int)
        {
            this_type tmp = *this;
            --m_current;
            return tmp;
        }

        this_type& operator+=(difference_type offset)
        {
            m_current += offset;
            return (*this);
        }

        this_type operator+(difference_type offset) const
        {
            return (this_type(m_current + offset));
        }

        this_type& operator-=(difference_type offset)
        {
            m_current -= offset;
            return (*this);
        }

        this_type operator-(difference_type offset) const
        {
            return (this_type(m_current - offset));
        }

        reference operator[](difference_type offset) const
        {
            return (AZStd::move(m_current[offset]));
        }
    protected:
        iterator_type m_current;
    };

    template<class Iterator, class Difference>
    inline
    move_iterator<Iterator> operator+(Difference offset, const move_iterator<Iterator>& right)
    {
        return (right + offset);
    }

    template<class Iterator1, class Iterator2>
    inline
    typename Iterator1::difference_type operator-(move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {
        return (left.base() - right.base());
    }

    template<class Iterator1,   class Iterator2>
    inline
    bool operator==(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {
        return (left.base() == right.base());
    }

    template<class Iterator1,   class Iterator2>
    inline
    bool operator!=(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {   // test for move_iterator inequality
        return (!(left == right));
    }

    template<class Iterator1,   class Iterator2>
    inline
    bool operator<(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {   // test for move_iterator < move_iterator
        return (left.base() < right.base());
    }

    template<class Iterator1,   class Iterator2>
    inline
    bool operator>(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {   // test for move_iterator > move_iterator
        return (right < left);
    }

    template<class Iterator1, class Iterator2>
    inline
    bool operator<=(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {
        return (!(right < left));
    }

    template<class Iterator1, class Iterator2>
    inline
    bool operator>=(const move_iterator<Iterator1>& left, const move_iterator<Iterator2>& right)
    {
        return (!(left < right));
    }

    template<class Iterator>
    inline
    move_iterator<Iterator> make_move_iterator(const Iterator iter)
    {
        return (move_iterator<Iterator>(iter));
    }
#endif // AZ_HAS_RVALUE_REFS

    /**
     *  Back insert iterator. (24.4.2.1)
     */
    template <class Container>
    class back_insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        typedef back_insert_iterator<Container> this_type;
    public:
        typedef Container          container_type;

        AZ_FORCE_INLINE explicit back_insert_iterator(Container& container)
            : m_container(&container) {}
        AZ_FORCE_INLINE this_type& operator=(const typename Container::value_type& value)
        {
            m_container->push_back(value);
            return *this;
        }
        AZ_FORCE_INLINE this_type& operator*()      { return *this; }
        AZ_FORCE_INLINE this_type& operator++()     { return *this; }
        AZ_FORCE_INLINE this_type  operator++(int)  { return *this; }
    protected:
        Container*  m_container;
    };

    template <class Container>
    AZ_FORCE_INLINE back_insert_iterator<Container> back_inserter(Container& container)     { return back_insert_iterator<Container>(container); }

    /**
     *  Front insert iterator. (24.4.2.2)
     */
    template <class Container>
    class front_insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        typedef front_insert_iterator<Container> this_type;

    public:
        typedef Container          container_type;

        AZ_FORCE_INLINE explicit front_insert_iterator(Container& container)
            : m_container(&container) {}

        AZ_FORCE_INLINE this_type& operator=(const typename Container::value_type& value)
        {
            m_container->push_front(value);
            return *this;
        }
        AZ_FORCE_INLINE this_type& operator*() { return *this; }
        AZ_FORCE_INLINE this_type& operator++() { return *this; }
        AZ_FORCE_INLINE this_type  operator++(int) { return *this; }
    protected:
        Container* m_container;
    };

    template <class Container>
    AZ_FORCE_INLINE front_insert_iterator<Container> front_inserter(Container& container)   { return front_insert_iterator<Container>(container); }

    /**
     * Insert iterator. (24.4.2.3)
     */
    template <class Container>
    class insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        typedef insert_iterator<Container> this_type;
    public:
        typedef Container                       container_type;
        typedef typename Container::iterator    container_iterator_type;

        AZ_FORCE_INLINE insert_iterator(Container& container, container_iterator_type iterator)
            : m_container(&container)
            , m_iterator(iterator) {}

        AZ_FORCE_INLINE this_type& operator=(const typename Container::value_type& value)
        {
            m_iterator = m_container->insert(m_iterator, value);
            ++m_iterator;
            return *this;
        }
        AZ_FORCE_INLINE this_type& operator*()      { return *this; }
        AZ_FORCE_INLINE this_type& operator++()     { return *this; }
        AZ_FORCE_INLINE this_type& operator++(int)  { return *this; }
    protected:
        Container*                  m_container;
        container_iterator_type     m_iterator;
    };

    template <class Container, class Iterator>
    AZ_FORCE_INLINE insert_iterator<Container>  inserter(Container& container, Iterator iterator)
    {
        typedef typename Container::iterator container_iterator_type;
        return insert_iterator<Container>(container, container_iterator_type(iterator));
    }



#ifdef AZSTD_DOXYGEN_INVOKED
    /**
     * Iterator status flags.
     */
    struct  iterator_status_flag
#else
    enum    iterator_status_flag
#endif
    {
        isf_none            = 0x00,     ///< Iterator is invalid.
        isf_valid           = 0x01,     ///< Iterator point to a valid element in container, we can't dereference of all them. For instance the end() is a valid iterator, but you can't access it.
        isf_can_dereference = 0x02,     ///< We can dereference this iterator (it points to a valid element).
        isf_current         = 0x04
    };


    // Stream iterators... todo.

    //////////////////////////////////////////////////////////////////////////
    // Advance
    template <class InputIterator, class Distance>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, Distance offset, const input_iterator_tag&)
    {
        //AZ_Assert(offset>=0,"AZStd::advance input iterator can only advance forward!");
        for (; 0 < offset; --offset)
        {
            ++iterator;
        }
    }
    template <class InputIterator, class Distance>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, Distance offset, const forward_iterator_tag&)
    {
        //AZ_Assert(offset>=0,"AZStd::advance forward iterator can only advance forward!");
        for (; 0 < offset; --offset)
        {
            ++iterator;
        }
    }
    template <class InputIterator, class Distance>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, Distance offset, const bidirectional_iterator_tag&)
    {
        for (; 0 < offset; --offset)
        {
            ++iterator;
        }
        for (; offset < 0; ++offset)
        {
            --iterator;
        }
    }
    template <class InputIterator>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, AZStd::size_t offset, const bidirectional_iterator_tag&)
    {
        for (; 0 < offset; --offset)
        {
            ++iterator;
        }
    }
    template <class InputIterator, class Distance>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, Distance offset, const random_access_iterator_tag&)
    {
        iterator += offset;
    }
    template <class InputIterator, class Distance>
    AZ_FORCE_INLINE void advance(InputIterator& iterator, Distance offset)
    {
        advance(iterator, offset, typename iterator_traits<InputIterator>::iterator_category());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reverse
    template<class InputIterator>
    AZ_INLINE void reverse(InputIterator first, InputIterator last, const bidirectional_iterator_tag&)
    {   // reverse elements in [first, last), bidirectional iterators
        for (; first != last && first != --last; ++first)
        {
            AZStd::iter_swap(first, last);
        }
    }

    template<class InputIterator>
    AZ_INLINE void reverse(InputIterator first, InputIterator last, const random_access_iterator_tag&)
    {   // reverse elements in [first, last), random-access iterators
        //      _DEBUG_RANGE(first, last);
        for (; first < last; ++first)
        {
            AZStd::iter_swap(first, --last);
        }
    }

    template<class InputIterator>
    AZ_INLINE void reverse(InputIterator first, InputIterator last)
    {   // reverse elements in [first, lLast)
        AZStd::reverse(first, last, typename iterator_traits<InputIterator>::iterator_category());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Rotate
    template<class InputIterator>
    AZ_INLINE void rotate(InputIterator first, InputIterator mid, InputIterator last, const forward_iterator_tag&)
    {
        // rotate [first, _Last), forward iterators
        for (InputIterator next = mid;; )
        {
            // swap [first, ...) into place
            AZStd::iter_swap(first, next);
            if (++first == mid)
            {
                if (++next == last)
                {
                    break;  // done, quit
                }
                else
                {
                    mid = next; // mark end of next interval
                }
            }
            else if (++next == last)
            {
                next = mid; // wrap to last end
            }
        }
    }

    template<class InputIterator>
    AZ_INLINE void rotate(InputIterator first, InputIterator mid, InputIterator last, const bidirectional_iterator_tag&)
    {
        // rotate [first, last), bidirectional iterators
        AZStd::reverse(first, mid);
        AZStd::reverse(mid, last);
        AZStd::reverse(first, last);
    }
    template<class InputIterator>
    AZ_INLINE void rotate(InputIterator first, InputIterator mid, InputIterator last, const random_access_iterator_tag&)
    {
        // rotate [first, last), random-access iterators
        typename iterator_traits<InputIterator>::difference_type shift = mid - first;
        typename iterator_traits<InputIterator>::difference_type factor, tmp, count = last - first;

        for (factor = shift; factor != 0; )
        {
            // find subcycle count as GCD of shift count and length
            tmp = count % factor;
            count = factor;
            factor = tmp;
        }

        if (count < last - first)
        {
            for (; 0 < count; --count)
            {   // rotate each subcycle
                InputIterator hole = first + count;
                InputIterator next = hole;
                InputIterator next1 = next + shift == last ? first : next + shift;
                for (;; )
                {
                    // percolate elements back around subcycle
                    AZStd::iter_swap(next, next1);
                    next = next1;
                    next1 = shift < last - next1 ? next1 + shift : first + (shift - (last - next1));
                    if (next1 == hole)
                    {
                        break;
                    }
                }
            }
        }
    }

    template<class InputIterator>
    AZ_INLINE void rotate(InputIterator first, InputIterator mid, InputIterator last)
    {
        // rotate [first, last)
        if (first != mid && mid != last)
        {
            rotate(first, mid, last, typename iterator_traits<InputIterator>::iterator_category());
        }
    }
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // Distance functions.
    template<class InputIterator, class DistanceType>
    AZ_FORCE_INLINE void distance(const InputIterator& first, const InputIterator& last, DistanceType& n, const input_iterator_tag&)
    {
        InputIterator iter(first);
        while (iter != last)
        {
            ++iter;
            ++n;
        }
    }

    template<class RandomAccessIterator, class DistanceType>
    AZ_FORCE_INLINE void distance(const RandomAccessIterator& first, const RandomAccessIterator& last, DistanceType& n, const random_access_iterator_tag&)
    {
        n += last - first;
    }

    template<typename InputIterator>
    AZ_FORCE_INLINE typename iterator_traits<InputIterator>::difference_type distance(const InputIterator& first, const InputIterator& last, const input_iterator_tag&)
    {
        InputIterator iter(first);
        typename iterator_traits<InputIterator>::difference_type n = 0;
        while (iter != last)
        {
            ++iter;
            ++n;
        }
        return n;
    }

    template<typename RandomAccessIterator>
    AZ_FORCE_INLINE typename iterator_traits<RandomAccessIterator>::difference_type distance(const RandomAccessIterator& first, const RandomAccessIterator& last,  const random_access_iterator_tag&)
    {
        typename iterator_traits<RandomAccessIterator>::difference_type n = last - first;
        return n;
    }

    template<class InputIterator>
    AZ_FORCE_INLINE typename iterator_traits<InputIterator>::difference_type    distance(const InputIterator& first, const InputIterator& last)
    {
        return distance(first, last, typename iterator_traits<InputIterator>::iterator_category());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // equal
    template<class InputIterator1, class InputIterator2>
    AZ_FORCE_INLINE bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
    {
        // \todo Check iterators.
        for (; first1 != last1; ++first1, ++first2)
        {
            if (!(*first1 == *first2))
            {
                return false;
            }
        }

        return true;
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    AZ_FORCE_INLINE bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate binaryPredicate)
    {
        // \todo Check iterators.
        for (; first1 != last1; ++first1, ++first2)
        {
            if (!binaryPredicate(*first1 == *first2))
            {
                return false;
            }
        }

        return true;
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // next prev utils
    template <class Iterator>
    Iterator next(Iterator iterator) { return ++iterator; }

    template <class Iterator, class Distance>
    Iterator next(Iterator iterator, Distance offset)
    {
        AZStd::advance(iterator, offset);
        return iterator;
    }

    template <class Iterator>
    Iterator prior(Iterator iterator) { return --iterator; }

    template <class Iterator, class Distance>
    Iterator prior(Iterator iterator, Distance offset)
    {
        AZStd::advance(iterator, -offset);
        return iterator;
    }
    //////////////////////////////////////////////////////////////////////////

    // Some iterator category check helpers
    //  AZSTD_TYPE_TRAIT_BOOL_DEF1(is_continuous_random_access_iterator_cat,Category,false);
    template< typename Category >
    struct is_continuous_random_access_iterator_cat
        : public ::AZStd::integral_constant<bool, false>
    {};
    //  AZSTD_TYPE_TRAIT_BOOL_SPEC1(is_continuous_random_access_iterator_cat,continuous_random_access_iterator_tag,true);
    template<>
    struct is_continuous_random_access_iterator_cat<continuous_random_access_iterator_tag>
        : public ::AZStd::integral_constant<bool, true>
    {};

    namespace Debug
    {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        /**
         * \page CheckedIterators Checked iterators
         *
         * Checked iterators are wrappers/adapters for standard iterators. They provide error control at the
         * cost of slower performance and bigger memory footprint. By default they are enabled in debug
         * build, but their can be controlled with \ref AZSTD_CHECKED_ITERATORS.\n
         * In general you should not worry about checked iterators at all, unless they
         * cause some serious slow down in debug builds. Otherwise you will gain great error control.
         * Checked iterators control the following events:
         * \li When you move the iterator, that it's still within the container boundaries.
         * \li When you try to dereference the iterator it points to a valid element.
         * For instance container.end() iterator is valid one, but it can not be dereferenced.
         * \li When you call container functions that will make invalidate some iterators (single iterator, iterator range or all iterators), that this
         * iterator(s) become invalid and don't belog to the container.
         * \li Keep information which container the iterator belongs to, if it's valid.
         * \li All containers have a list of all iterators the belong to them.
         *
         * There are checked iterator type for each fundamental iterator type.
         */
        struct checked_container_base;

        /**
        * Base class for checked iterators.
        */
        struct checked_iterator_base
        {
            checked_container_base*     m_container;
            checked_iterator_base*      m_nextIterator;
            checked_iterator_base*      m_prevIterator;
        };

        /**
         * Base class to checked containers.
         * \note Move this class to the container_base.h if we need real base class.
         */
        struct checked_container_base
        {
            AZ_FORCE_INLINE checked_container_base()
                : m_iteratorList(0)
            {}
            AZ_FORCE_INLINE ~checked_container_base()
            {
                orphan_all();
            }

            AZ_FORCE_INLINE void orphan_all()
            {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
                checked_iterator_base* iter = m_iteratorList;
                m_iteratorList = 0;
                while (iter != 0)
                {
                    AZ_Assert(iter->m_container == this, "checked_container_base::orphan_all - iterator was corrupted!");
                    iter->m_container = 0;
                    iter = iter->m_nextIterator;
                }
            }

            AZ_FORCE_INLINE void swap_all(checked_container_base& rhs)
            {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
                checked_iterator_base* temp = m_iteratorList;
                m_iteratorList = rhs.m_iteratorList;
                checked_iterator_base* iter = m_iteratorList;
                while (iter != 0)
                {
                    iter->m_container = this;
                    iter = iter->m_nextIterator;
                }
                rhs.m_iteratorList = temp;
                iter = rhs.m_iteratorList;
                while (iter != 0)
                {
                    iter->m_container = &rhs;
                    iter = iter->m_nextIterator;
                }
            }

#if defined(AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS) && !defined(AZ_PLATFORM_WINDOWS) && !defined(AZ_PLATFORM_X360) // ACCEPTED_USE
            static AZStd::mutex& get_global_section()
            {
                static mutex section;
                return section;
            }
#endif
            mutable checked_iterator_base*      m_iteratorList;
        };

        template<class Iterator, class Container>
        class checked_input_iterator
            : public checked_iterator_base
        {
            typedef checked_input_iterator<Iterator, Container> this_type;

        public:
            typedef typename iterator_traits<Iterator>::reference           reference;
            typedef typename iterator_traits<Iterator>::pointer             pointer;
            typedef typename iterator_traits<Iterator>::iterator_category   iterator_category;
            typedef typename iterator_traits<Iterator>::value_type          value_type;
            typedef typename iterator_traits<Iterator>::difference_type     difference_type;

            AZ_FORCE_INLINE checked_input_iterator()
            {
                m_container = 0;
            }
            AZ_FORCE_INLINE checked_input_iterator(const Iterator& iter, const checked_container_base* cont)
                : m_iter(iter)
            {
                m_container = const_cast<checked_container_base*>(cont);
                m_prevIterator = 0;

#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                AZ_GLOBAL_SCOPED_LOCK(checked_container_base::get_global_section());
#endif

                // add it at he top of the list
                m_nextIterator = m_container->m_iteratorList;
                if (m_nextIterator)
                {
                    m_nextIterator->m_prevIterator = this;
                }
                m_container->m_iteratorList = this;
            }
            AZ_FORCE_INLINE checked_input_iterator(const this_type& rhs)
            {
                m_container    = 0;
                copy(rhs);
            }
            template<class U>
            AZ_FORCE_INLINE checked_input_iterator(const checked_input_iterator<U, Container>& rhs)
            {
                m_container    = 0;
                copy(rhs);
            }
            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                copy(rhs);
                return *this;
            }

            AZ_FORCE_INLINE ~checked_input_iterator()
            {
                // remove from the list
                if (m_container != 0)
                {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                    AZ_GLOBAL_SCOPED_LOCK(checked_container_base::get_global_section());
#endif
                    if (m_container)
                    {
                        if (m_prevIterator)
                        {
                            m_prevIterator->m_nextIterator = m_nextIterator;
                        }
                        else
                        {
                            m_container->m_iteratorList = m_nextIterator;
                        }

                        if (m_nextIterator)
                        {
                            m_nextIterator->m_prevIterator = m_prevIterator;
                        }
                    }
                }
            }


            template<class U>
            AZ_FORCE_INLINE bool operator==(const checked_input_iterator<U, Container>& rhs) const
            {
                return m_iter == rhs.m_iter;
            }

            template<class U>
            AZ_FORCE_INLINE bool operator!=(const checked_input_iterator<U, Container>& rhs) const
            {
                return m_iter != rhs.m_iter;
            }

            AZ_FORCE_INLINE reference operator*() const
            {
                Container* container = static_cast<Container*>(m_container);
                (void)container;
                AZ_Assert((container->validate_iterator(*(typename Container::iterator*) this) & isf_can_dereference) != 0, "checked_input_iterator::operator* - iterator can not be dereferenced!");

                return *m_iter;
            }
            AZ_FORCE_INLINE pointer operator->() const
            {
                Container* container = static_cast<Container*>(m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_can_dereference, "checked_input_iterator::operator-> - iterator can not be dereferenced!");

                return &(*m_iter);
            }


            AZ_FORCE_INLINE this_type& operator++()
            {
                Container* container = static_cast<Container*>(m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++() - iterator is not valid!");

                ++m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                Container* container = static_cast<Container*>(m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");
                this_type temp = *this;
                ++m_iter;
                return temp;
            }


            AZ_FORCE_INLINE Iterator&   get_iterator() const
            {
                Container* container = static_cast<Container*>(m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");

                return const_cast<Iterator&>(m_iter);
            }

            Iterator m_iter;

        protected:
            template<class SourceType>
            AZ_INLINE void          copy(const SourceType& rhs)
            {
                if (m_container != rhs.m_container)
                {
                    // orphan me
                    if (m_container != 0)
                    {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                        AZ_GLOBAL_SCOPED_LOCK(checked_container_base::get_global_section());
#endif
                        if (m_container != 0)
                        {
                            if (m_prevIterator)
                            {
                                m_prevIterator->m_nextIterator = m_nextIterator;
                            }
                            else
                            {
                                m_container->m_iteratorList = m_nextIterator;
                            }

                            if (m_nextIterator)
                            {
                                m_nextIterator->m_prevIterator = m_prevIterator;
                            }
                        }
                    }

                    // adopt me
                    m_container = rhs.m_container;
                    if (m_container)
                    {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                        AZ_GLOBAL_SCOPED_LOCK(checked_container_base::get_global_section());
#endif
                        if (m_container)
                        {
                            m_prevIterator = 0;
                            // add it at he top of the list
                            m_nextIterator = m_container->m_iteratorList;
                            if (m_nextIterator)
                            {
                                m_nextIterator->m_prevIterator = this;
                            }
                            m_container->m_iteratorList = this;
                        }
                    }
                }

                m_iter = rhs.m_iter;
            }
        };

        /**
         *
         */
        template<class Iterator, class Container>
        class checked_output_iterator
            : public checked_input_iterator<Iterator, Container>
        {
            typedef checked_output_iterator<Iterator, Container> this_type;
            typedef checked_input_iterator<Iterator, Container> base_type;
        public:
            AZ_FORCE_INLINE checked_output_iterator() {}
            AZ_FORCE_INLINE checked_output_iterator(const Iterator& iter, const checked_container_base* cont)
                : base_type(iter, cont) {}
            AZ_FORCE_INLINE checked_output_iterator(const this_type& rhs)
                : base_type(rhs) {}
            template<class U>
            AZ_FORCE_INLINE checked_output_iterator(const checked_output_iterator<U, Container>& rhs)
                : base_type(rhs) {}

            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                base_type::copy(rhs);
                return *this;
            }
            AZ_FORCE_INLINE this_type& operator++()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++() - iterator is not valid!");

                ++base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");
                this_type temp = *this;
                ++base_type::m_iter;
                return temp;
            }
        };

        template<class Iterator, class Container>
        class checked_forward_iterator
            : public checked_input_iterator<Iterator, Container>
        {
            typedef checked_forward_iterator<Iterator, Container> this_type;
            typedef checked_input_iterator<Iterator, Container> base_type;
        public:
            AZ_FORCE_INLINE checked_forward_iterator() {}
            AZ_FORCE_INLINE checked_forward_iterator(const Iterator& iter, const checked_container_base* cont)
                : base_type(iter, cont)    {}
            AZ_FORCE_INLINE checked_forward_iterator(const this_type& rhs)
                : base_type(rhs) {}
            template<class U>
            AZ_FORCE_INLINE checked_forward_iterator(const checked_forward_iterator<U, Container>& rhs)
                : base_type(rhs) {}

            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                base_type::copy(rhs);
                return *this;
            }
            AZ_FORCE_INLINE this_type& operator++()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++() - iterator is not valid!");

                ++base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");
                this_type temp = *this;
                ++base_type::m_iter;
                return temp;
            }
        };

        /**
         *
         */
        template<class Iterator, class Container>
        class checked_bidirectional_iterator
            : public checked_input_iterator<Iterator, Container>
        {
            typedef checked_bidirectional_iterator<Iterator, Container>  this_type;
            typedef checked_input_iterator<Iterator, Container>          base_type;
        public:
            AZ_FORCE_INLINE checked_bidirectional_iterator() {}
            AZ_FORCE_INLINE checked_bidirectional_iterator(const Iterator& iter, const checked_container_base* cont)
                : base_type(iter, cont)  {}
            AZ_FORCE_INLINE checked_bidirectional_iterator(const this_type& rhs)
                : base_type(rhs) {}
            template<class U>
            AZ_FORCE_INLINE checked_bidirectional_iterator(const checked_bidirectional_iterator<U, Container>& rhs)
                : base_type(rhs) {}

            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                base_type::copy(rhs);
                return *this;
            }
            AZ_FORCE_INLINE this_type& operator++()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++() - iterator is not valid!");

                ++base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");
                this_type temp = *this;
                ++base_type::m_iter;
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_bidirectional_iterator::operator-- - iterator is not valid!");
                --base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_bidirectional_iterator::operator--(int) - iterator is not valid!");
                this_type temp = *this;
                --base_type::m_iter;
                return temp;
            }
        };


        /**
         *
         */
        template<class Iterator, class Container>
        class checked_randomaccess_iterator
            : public checked_bidirectional_iterator<Iterator, Container>
        {
            typedef checked_randomaccess_iterator<Iterator, Container>   this_type;
            typedef checked_bidirectional_iterator<Iterator, Container>  base_type;

        public:
            typedef typename iterator_traits<Iterator>::difference_type difference_type;
            typedef typename iterator_traits<Iterator>::reference       reference;

            AZ_FORCE_INLINE checked_randomaccess_iterator() {}
            AZ_FORCE_INLINE checked_randomaccess_iterator(const Iterator& iter, const checked_container_base* cont)
                : base_type(iter, cont)   {}
            AZ_FORCE_INLINE checked_randomaccess_iterator(const this_type& rhs)
                : base_type(rhs) {}
            template<class U>
            AZ_FORCE_INLINE checked_randomaccess_iterator(const checked_randomaccess_iterator<U, Container>& rhs)
                : base_type(rhs) {}
            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                base_type::copy(rhs);
                return *this;
            }
            AZ_FORCE_INLINE this_type& operator++()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++() - iterator is not valid!");

                ++base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_input_iterator::operator++(int) - iterator is not valid!");
                this_type temp = *this;
                ++base_type::m_iter;
                return temp;
            }


            AZ_FORCE_INLINE this_type& operator--()
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_bidirectional_iterator::operator-- - iterator is not valid!");
                --base_type::m_iter;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_bidirectional_iterator::operator--(int) - iterator is not valid!");
                this_type temp = *this;
                --base_type::m_iter;
                return temp;
            }
            this_type& operator+=(difference_type offset)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_randomaccess_iterator::operator+=(offset) - iterator is not valid!");
                base_type::m_iter += offset;
                return *this;
            }

            this_type operator+(difference_type offset) const
            {
                this_type temp = *this;
                return (temp += offset);
            }

            this_type& operator-=(difference_type offset)
            {
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::iterator*) this) & isf_valid, "checked_randomaccess_iterator::operator-=(offset) - iterator is not valid!");
                base_type::m_iter -= offset;
                return *this;
            }

            this_type operator-(difference_type offset) const
            {
                this_type temp = *this;
                return (temp -= offset);
            }

            difference_type operator-(const this_type& rhs) const
            {
                // return difference of iterators
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*) this) & isf_valid, "checked_randomaccess_iterator::operator-(rhs) - iterator is not valid!");
                AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*) & rhs) & isf_valid, "checked_randomaccess_iterator::operator-(rhs) - rhs.iterator is not valid!");

                return base_type::m_iter - rhs.m_iter;
            }

            reference operator[](difference_type offset) const
            {
                // subscript
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*) this) & isf_valid, "checked_randomaccess_iterator::operator[](offset) - iterator is not valid!");
                return base_type::m_iter[offset];  // the offset should be verified by the container
            }

            bool operator<(const this_type& rhs) const
            {
                // test if this < rhs
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)this)&isf_valid,"checked_randomaccess_iterator::operator<(rhs) - iterator is not valid!");
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)&rhs)&isf_valid,"checked_randomaccess_iterator::operator<(rhs) - rhs.iterator is not valid!");

                return base_type::m_iter < rhs.m_iter;
            }

            bool operator>(const this_type& rhs) const
            {
                // test if this > rhs
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)this)&isf_valid,"checked_randomaccess_iterator::operator>(rhs) - iterator is not valid!");
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)&rhs)&isf_valid,"checked_randomaccess_iterator::operator>(rhs) - rhs.iterator is not valid!");
                return (base_type::m_iter > rhs.m_iter);
            }

            bool operator<=(const this_type& rhs) const
            {   // test if this <= rhs
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)this)&isf_valid,"checked_randomaccess_iterator::operator<=(rhs) - iterator is not valid!");
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)&rhs)&isf_valid,"checked_randomaccess_iterator::operator<=(rhs) - rhs.iterator is not valid!");
                return (!(base_type::m_iter > rhs.m_iter));
            }

            bool operator>=(const this_type& rhs) const
            {   // test if this >= rhs
                Container* container = static_cast<Container*>(base_type::m_container);
                (void)container;
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)this)&isf_valid,"checked_randomaccess_iterator::operator>=(rhs) - iterator is not valid!");
                //              AZ_Assert(container->validate_iterator(*(typename Container::const_iterator*)&rhs)&isf_valid,"checked_randomaccess_iterator::operator>=(rhs) - rhs.iterator is not valid!");
                return (!(base_type::m_iter < rhs.m_iter));
            }
        };

        /**
         *
         */
        template<class Iterator, class Container>
        class checked_continous_randomaccess_iterator
            : public checked_randomaccess_iterator<Iterator, Container>
        {
        };

        // Since pointer iterators are special case, and we are avoiding function calls as much as possible
        #define AZSTD_POINTER_ITERATOR_PARAMS(_Pointer)         _Pointer, this
        #define AZSTD_CHECKED_ITERATOR(_IteratorImpl, _Param)    _IteratorImpl(_Param), this
        #define AZSTD_CHECKED_ITERATOR_2(_IteratorImpl, _Param1, _Param2) _IteratorImpl(_Param1, _Param2), this
        #define AZSTD_GET_ITER(_Iterator)                       _Iterator.m_iter
#else
        #define AZSTD_POINTER_ITERATOR_PARAMS(_Pointer)         _Pointer
        #define AZSTD_CHECKED_ITERATOR(_IteratorImpl, _Param)    _Param
        #define AZSTD_CHECKED_ITERATOR_2(_IteratorImpl, _Param1, _Param2) _Param1, _Param2
        #define AZSTD_GET_ITER(_Iterator)                       _Iterator
#endif
        /**
        * Map iterator tags from AZStd to a STL implementation (compatible with the standard).
        */
        #define AZSTD_ITERATOR_MAP_CATEGORY(_Namespace)                                                                                               \
    template<class IteratorCategory>                                                                                                                  \
    struct adapt_##_Namespace;                                                                                                                        \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::input_iterator_tag>                    { typedef _Namespace::input_iterator_tag            iterator_category;    \
    };                                                                                                                                                \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::output_iterator_tag>                   { typedef _Namespace::output_iterator_tag           iterator_category; }; \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::forward_iterator_tag>                  { typedef _Namespace::forward_iterator_tag          iterator_category; }; \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::bidirectional_iterator_tag>            { typedef _Namespace::bidirectional_iterator_tag    iterator_category; }; \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::random_access_iterator_tag>            { typedef _Namespace::random_access_iterator_tag    iterator_category; }; \
    template<>                                                                                                                                        \
    struct adapt_##_Namespace<AZStd::continuous_random_access_iterator_tag> { typedef _Namespace::random_access_iterator_tag    iterator_category; };

        /**
        * Maps specific AZSTD iterator type to STD implementation. That way this AZSTD iterator
        * can be used in any STD implementation algorithms, containers, etc.
        */
        #define AZSTD_ITERATOR_ADAPT_TO_STD(_STDNamespace, _AZSTDIteratorType)                                                                    \
    namespace _STDNamespace {                                                                                                                     \
        template<>                                                                                                                                \
        struct iterator_traits<_AZSTDIteratorType>                                                                                                \
            : public AZStd::iterator_traits<_AZSTDIteratorType> {                                                                                 \
            typedef adapt_##_STDNamespace<AZStd::iterator_traits<_AZSTDIteratorType>::iterator_category>::iterator_category iterator_category; }; \
    }
    } // namespace Debug

    namespace Internal
    {
        // AZStd const and non-const iterators are written to be binary compatible, i.e. iterator inherits from const_iterator and only add methods. This we "const casting"
        // between them is a matter of reinterpretation. In general avoid this cast as much as possible, as it's a bad practice, however it's true for all iterators.
        template<class Iterator, class ConstIterator>
        inline Iterator ConstIteratorCast(ConstIterator& iter)
        {
            AZ_STATIC_ASSERT((AZStd::is_base_of<ConstIterator, Iterator>::value), "For this cast to work Iterator should derive from ConstIterator");
            AZ_STATIC_ASSERT(sizeof(ConstIterator) == sizeof(Iterator), "For this cast to work ConstIterator and Iterator should be binarily identical");
            return *reinterpret_cast<Iterator*>(&iter);
        }
    }
} // namespace AZStd


#endif // AZSTD_ITERATOR_H
#pragma once
