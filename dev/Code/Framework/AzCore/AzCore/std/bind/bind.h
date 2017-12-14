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
#ifndef AZSTD_BIND_BIND_HPP_INCLUDED
#define AZSTD_BIND_BIND_HPP_INCLUDED

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/function_traits.h>

// As of version 0.92 the BIND implementation has missing features (missing operator() const version, issues with resolving overloads, etc.)
//#if /*defined(AZ_PLATFORM_PS4) ||*/ defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
//# define AZ_CORE_STD_SYSTEM_BIND
//#endif

#if defined(AZ_CORE_STD_SYSTEM_BIND)

#include <functional>

namespace AZStd
{
    // IMPORTANT: Bind does allocate memory, so if you enable the internal/compiler implementation you will
    // allocate using C++ heap instead of our default allocators!
    namespace placeholders
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        using std::placeholders::_7;
        using std::placeholders::_8;
        using std::placeholders::_9;
        using std::placeholders::_10;
        // >10 not support by MSVC so keep it to the lowest common denominator
        //using std::placeholders::_11;
        //using std::placeholders::_12;
        //using std::placeholders::_13;
        //using std::placeholders::_14;
        //using std::placeholders::_15;
        //using std::placeholders::_16;
        //using std::placeholders::_17;
        //using std::placeholders::_18;
        //using std::placeholders::_19;
        //using std::placeholders::_20;
    }

    using std::bind;
}

#else //!AZ_CORE_STD_SYSTEM_BIND

// Based on boost 1.39.0
    #include <AzCore/std/utils.h>

//# define AZSTD_BIND_VISIT_EACH visit_each

    #include <AzCore/std/bind/storage.h>
    #include <AzCore/std/bind/mem_fn.h>

    #ifdef AZ_COMPILER_MSVC
    # pragma warning(push)
    # pragma warning(disable: 4512) // assignment operator could not be generated
    #endif

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // We can move this to
    template<typename Visitor, typename T>
    inline void visit_each(Visitor& visitor, const T& t, long)
    {
        visitor(t);
    }

    template<typename Visitor, typename T>
    inline void visit_each(Visitor& visitor, const T& t)
    {
        visit_each(visitor, t, 0);
    }
    //////////////////////////////////////////////////////////////////////////

    template <class T>
    struct type {};

    //template<class T> class weak_ptr;
    namespace Internal     // implementation details
    {
        // result_traits
        template<class R, class F>
        struct result_traits
        {
            typedef R type;
        };

        struct unspecified {};

        template<class F>
        struct result_traits<unspecified, F>
        {
            using type = typename function_traits<F>::result_type;
        };

        template<class F>
        struct result_traits< unspecified, AZStd::reference_wrapper<F> >
        {
            using type = typename function_traits<F>::result_type;
        };

        // ref_compare
        template<class T>
        bool ref_compare(T const& a, T const& b, long)
        {
            return a == b;
        }

        template<int I>
        bool ref_compare(arg<I> const&, arg<I> const&, int)
        {
            return true;
        }

        template<int I>
        bool ref_compare(arg<I> (*)(), arg<I> (*)(), int)
        {
            return true;
        }

        template<class T>
        bool ref_compare(AZStd::reference_wrapper<T> const& a, AZStd::reference_wrapper<T> const& b, int)
        {
            return a.get_pointer() == b.get_pointer();
        }

        // bind_t forward declaration for listN
        template<class R, class F, class L>
        class bind_t;

        // Specialize function_traits_helper for bind
        template <class R, class F, class L>
        struct function_traits_helper<bind_t<R, F, L>> : function_traits_helper<F> { };

        template<class R, class F, class L>
        bool ref_compare(bind_t<R, F, L> const& a, bind_t<R, F, L> const& b, int)
        {
            return a.compare(b);
        }

        // value
        template<class T>
        class value
        {
        public:

            value(T const& t)
                : t_(t) {}

            T& get() { return t_; }
            T const& get() const { return t_; }

            bool operator==(value const& rhs) const
            {
                return t_ == rhs.t_;
            }

        private:

            T t_;
        };

        // ref_compare for weak_ptr
        //template<class T> bool ref_compare( value< weak_ptr<T> > const & a, value< weak_ptr<T> > const & b, int )
        //{
        //  return !(a.get() < b.get()) && !(b.get() < a.get());
        //}

        // type
        template<class T>
        class type
        {
        };

        // unwrap
        template<class F>
        struct unwrapper
        {
            static inline F& unwrap(F& f, long)
            {
                return f;
            }

            template<class F2>
            static inline F2& unwrap(AZStd::reference_wrapper<F2> rf, int)
            {
                return rf.get();
            }

            template<class R, class T>
            static inline Internal::dm<R, T> unwrap(R T::* pm, int)
            {
                return Internal::dm<R, T>(pm);
            }
        };

        // listN
        class list0
        {
        public:

            list0() {}

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A&, long)
            {
                return unwrapper<F>::unwrap(f, 0)();
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A&, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)();
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A&, int)
            {
                unwrapper<F>::unwrap(f, 0)();
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A&, int) const
            {
                unwrapper<F const>::unwrap(f, 0)();
            }

            template<class V>
            void accept(V&) const
            {
            }

            bool operator==(list0 const&) const
            {
                return true;
            }
        };

        template< class A1 >
        class list1
            : private storage1< A1 >
        {
        private:

            typedef storage1< A1 > base_type;

        public:

            explicit list1(A1 a1)
                : base_type(a1) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list1 const& rhs) const
            {
                return ref_compare(base_type::a1_, rhs.a1_, 0);
            }
        };

        struct logical_and;
        struct logical_or;

        template< class A1, class A2 >
        class list2
            : private storage2< A1, A2 >
        {
        private:

            typedef storage2< A1, A2 > base_type;

        public:

            list2(A1 a1, A2 a2)
                : base_type(a1, a2) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
            }

            template<class A>
            bool operator()(type<bool>, logical_and& /*f*/, A& a, int)
            {
                return a[ base_type::a1_ ] && a[ base_type::a2_ ];
            }

            template<class A>
            bool operator()(type<bool>, logical_and const& /*f*/, A& a, int) const
            {
                return a[ base_type::a1_ ] && a[ base_type::a2_ ];
            }

            template<class A>
            bool operator()(type<bool>, logical_or& /*f*/, A& a, int)
            {
                return a[ base_type::a1_ ] || a[ base_type::a2_ ];
            }

            template<class A>
            bool operator()(type<bool>, logical_or const& /*f*/, A& a, int) const
            {
                return a[ base_type::a1_ ] || a[ base_type::a2_ ];
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list2 const& rhs) const
            {
                return ref_compare(base_type::a1_, rhs.a1_, 0) && ref_compare(base_type::a2_, rhs.a2_, 0);
            }
        };

        template< class A1, class A2, class A3 >
        class list3
            : private storage3< A1, A2, A3 >
        {
        private:

            typedef storage3< A1, A2, A3 > base_type;

        public:

            list3(A1 a1, A2 a2, A3 a3)
                : base_type(a1, a2, a3) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list3 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0);
            }
        };

        template< class A1, class A2, class A3, class A4 >
        class list4
            : private storage4< A1, A2, A3, A4 >
        {
        private:

            typedef storage4< A1, A2, A3, A4 > base_type;

        public:

            list4(A1 a1, A2 a2, A3 a3, A4 a4)
                : base_type(a1, a2, a3, a4) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list4 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0);
            }
        };

        template< class A1, class A2, class A3, class A4, class A5 >
        class list5
            : private storage5< A1, A2, A3, A4, A5 >
        {
        private:

            typedef storage5< A1, A2, A3, A4, A5 > base_type;

        public:

            list5(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
                : base_type(a1, a2, a3, a4, a5) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5>) const { return base_type::a5_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5> (*)()) const { return base_type::a5_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list5 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0) &&
                    ref_compare(base_type::a5_, rhs.a5_, 0);
            }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6>
        class list6
            : private storage6< A1, A2, A3, A4, A5, A6 >
        {
        private:

            typedef storage6< A1, A2, A3, A4, A5, A6 > base_type;

        public:

            list6(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
                : base_type(a1, a2, a3, a4, a5, a6) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5>) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6>) const { return base_type::a6_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5> (*)()) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6> (*)()) const { return base_type::a6_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (AZStd::reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list6 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0) &&
                    ref_compare(base_type::a5_, rhs.a5_, 0) &&
                    ref_compare(base_type::a6_, rhs.a6_, 0);
            }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7>
        class list7
            : private storage7< A1, A2, A3, A4, A5, A6, A7 >
        {
        private:

            typedef storage7< A1, A2, A3, A4, A5, A6, A7 > base_type;

        public:

            list7(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
                : base_type(a1, a2, a3, a4, a5, a6, a7) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5>) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6>) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7>) const { return base_type::a7_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5> (*)()) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6> (*)()) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7> (*)()) const { return base_type::a7_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list7 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0) &&
                    ref_compare(base_type::a5_, rhs.a5_, 0) &&
                    ref_compare(base_type::a6_, rhs.a6_, 0) &&
                    ref_compare(base_type::a7_, rhs.a7_, 0);
            }
        };

        template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
        class list8
            : private storage8< A1, A2, A3, A4, A5, A6, A7, A8 >
        {
        private:

            typedef storage8< A1, A2, A3, A4, A5, A6, A7, A8 > base_type;

        public:

            list8(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
                : base_type(a1, a2, a3, a4, a5, a6, a7, a8) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5>) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6>) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7>) const { return base_type::a7_; }
            A8 operator[] (AZStd::arg<8>) const { return base_type::a8_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5> (*)()) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6> (*)()) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7> (*)()) const { return base_type::a7_; }
            A8 operator[] (AZStd::arg<8> (*)()) const { return base_type::a8_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list8 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0) &&
                    ref_compare(base_type::a5_, rhs.a5_, 0) &&
                    ref_compare(base_type::a6_, rhs.a6_, 0) &&
                    ref_compare(base_type::a7_, rhs.a7_, 0) &&
                    ref_compare(base_type::a8_, rhs.a8_, 0);
            }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
        class list9
            : private storage9< A1, A2, A3, A4, A5, A6, A7, A8, A9 >
        {
        private:

            typedef storage9< A1, A2, A3, A4, A5, A6, A7, A8, A9 > base_type;

        public:

            list9(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
                : base_type(a1, a2, a3, a4, a5, a6, a7, a8, a9) {}

            A1 operator[] (AZStd::arg<1>) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2>) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3>) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4>) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5>) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6>) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7>) const { return base_type::a7_; }
            A8 operator[] (AZStd::arg<8>) const { return base_type::a8_; }
            A9 operator[] (AZStd::arg<9>) const { return base_type::a9_; }

            A1 operator[] (AZStd::arg<1> (*)()) const { return base_type::a1_; }
            A2 operator[] (AZStd::arg<2> (*)()) const { return base_type::a2_; }
            A3 operator[] (AZStd::arg<3> (*)()) const { return base_type::a3_; }
            A4 operator[] (AZStd::arg<4> (*)()) const { return base_type::a4_; }
            A5 operator[] (AZStd::arg<5> (*)()) const { return base_type::a5_; }
            A6 operator[] (AZStd::arg<6> (*)()) const { return base_type::a6_; }
            A7 operator[] (AZStd::arg<7> (*)()) const { return base_type::a7_; }
            A8 operator[] (AZStd::arg<8> (*)()) const { return base_type::a8_; }
            A9 operator[] (AZStd::arg<9> (*)()) const { return base_type::a9_; }

            template<class T>
            T& operator[] (Internal::value<T>& v) const { return v.get(); }

            template<class T>
            T const& operator[] (Internal::value<T> const& v) const { return v.get(); }

            template<class T>
            T& operator[] (reference_wrapper<T> const& v) const { return v.get(); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L>& b) const { return b.eval(*this); }

            template<class R, class F, class L>
            typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const& b) const { return b.eval(*this); }

            template<class R, class F, class A>
            R operator()(type<R>, F& f, A& a, long)
            {
                return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
            }

            template<class R, class F, class A>
            R operator()(type<R>, F const& f, A& a, long) const
            {
                return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
            }

            template<class F, class A>
            void operator()(type<void>, F& f, A& a, int)
            {
                unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
            }

            template<class F, class A>
            void operator()(type<void>, F const& f, A& a, int) const
            {
                unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
            }

            template<class V>
            void accept(V& v) const
            {
                base_type::accept(v);
            }

            bool operator==(list9 const& rhs) const
            {
                return

                    ref_compare(base_type::a1_, rhs.a1_, 0) &&
                    ref_compare(base_type::a2_, rhs.a2_, 0) &&
                    ref_compare(base_type::a3_, rhs.a3_, 0) &&
                    ref_compare(base_type::a4_, rhs.a4_, 0) &&
                    ref_compare(base_type::a5_, rhs.a5_, 0) &&
                    ref_compare(base_type::a6_, rhs.a6_, 0) &&
                    ref_compare(base_type::a7_, rhs.a7_, 0) &&
                    ref_compare(base_type::a8_, rhs.a8_, 0) &&
                    ref_compare(base_type::a9_, rhs.a9_, 0);
            }
        };

        // bind_t
        template<class R, class F, class L>
        class bind_t
        {
        public:

            typedef bind_t this_type;

            bind_t(F f, L const& l)
                : f_(f)
                , l_(l) {}

            #define AZSTD_BIND_RETURN return
            #include <AzCore/std/bind/bind_template.h>
            #undef AZSTD_BIND_RETURN
        };

        // function_equal

        // put overloads in Internal, rely on ADL

        template<class R, class F, class L>
        bool function_equal(bind_t<R, F, L> const& a, bind_t<R, F, L> const& b)
        {
            return a.compare(b);
        }

        // put overloads in AZStd
    } // namespace Internal

    template<class R, class F, class L>
    bool function_equal(Internal::bind_t<R, F, L> const& a, Internal::bind_t<R, F, L> const& b)
    {
        return a.compare(b);
    }

    namespace Internal
    {
        // add_value

        template< class T, int I >
        struct add_value_2
        {
            typedef AZStd::arg<I> type;
        };

        template< class T >
        struct add_value_2< T, 0 >
        {
            typedef Internal::value< T > type;
        };

        template<class T>
        struct add_value
        {
            typedef typename add_value_2< T, AZStd::is_placeholder< T >::value >::type type;
        };

        template<class T>
        struct add_value< value<T> >
        {
            typedef Internal::value<T> type;
        };

        template<class T>
        struct add_value< reference_wrapper<T> >
        {
            typedef reference_wrapper<T> type;
        };

        template<int I>
        struct add_value< arg<I> >
        {
            typedef AZStd::arg<I> type;
        };

        template<int I>
        struct add_value< arg<I> (*)() >
        {
            typedef AZStd::arg<I> (*type)();
        };

        template<class R, class F, class L>
        struct add_value< bind_t<R, F, L> >
        {
            typedef bind_t<R, F, L> type;
        };

        // list_av_N
        template<class A1>
        struct list_av_1
        {
            typedef typename add_value<A1>::type B1;
            typedef list1<B1> type;
        };

        template<class A1, class A2>
        struct list_av_2
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef list2<B1, B2> type;
        };

        template<class A1, class A2, class A3>
        struct list_av_3
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef list3<B1, B2, B3> type;
        };

        template<class A1, class A2, class A3, class A4>
        struct list_av_4
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef list4<B1, B2, B3, B4> type;
        };

        template<class A1, class A2, class A3, class A4, class A5>
        struct list_av_5
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef typename add_value<A5>::type B5;
            typedef list5<B1, B2, B3, B4, B5> type;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6>
        struct list_av_6
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef typename add_value<A5>::type B5;
            typedef typename add_value<A6>::type B6;
            typedef list6<B1, B2, B3, B4, B5, B6> type;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7>
        struct list_av_7
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef typename add_value<A5>::type B5;
            typedef typename add_value<A6>::type B6;
            typedef typename add_value<A7>::type B7;
            typedef list7<B1, B2, B3, B4, B5, B6, B7> type;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
        struct list_av_8
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef typename add_value<A5>::type B5;
            typedef typename add_value<A6>::type B6;
            typedef typename add_value<A7>::type B7;
            typedef typename add_value<A8>::type B8;
            typedef list8<B1, B2, B3, B4, B5, B6, B7, B8> type;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
        struct list_av_9
        {
            typedef typename add_value<A1>::type B1;
            typedef typename add_value<A2>::type B2;
            typedef typename add_value<A3>::type B3;
            typedef typename add_value<A4>::type B4;
            typedef typename add_value<A5>::type B5;
            typedef typename add_value<A6>::type B6;
            typedef typename add_value<A7>::type B7;
            typedef typename add_value<A8>::type B8;
            typedef typename add_value<A9>::type B9;
            typedef list9<B1, B2, B3, B4, B5, B6, B7, B8, B9> type;
        };

        // operator!

        struct logical_not
        {
            template<class V>
            bool operator()(V const& v) const { return !v; }
        };

        template<class R, class F, class L>
        bind_t< bool, logical_not, list1< bind_t<R, F, L> > >
        operator! (bind_t<R, F, L> const& f)
        {
            typedef list1< bind_t<R, F, L> > list_type;
            return bind_t<bool, logical_not, list_type> (logical_not(), list_type(f));
        }

        // relational operators

    #define AZSTD_BIND_OPERATOR(op, name)                                        \
    struct name                                                                  \
    {                                                                            \
        template<class V, class W>                                               \
        bool operator()(V const& v, W const& w) const { return v op w; }         \
    };                                                                           \
    template<class R, class F, class L, class A2>                                \
    bind_t< bool, name, list2< bind_t<R, F, L>, typename add_value<A2>::type > > \
    operator op (bind_t<R, F, L> const& f, A2 a2)                                \
    {                                                                            \
        typedef typename add_value<A2>::type B2;                                 \
        typedef list2< bind_t<R, F, L>, B2> list_type;                           \
        return bind_t<bool, name, list_type> (name(), list_type(f, a2));         \
    }

        AZSTD_BIND_OPERATOR(==, bind_equal)
        AZSTD_BIND_OPERATOR(!=, bind_not_equal)

        AZSTD_BIND_OPERATOR(<,  bind_less)
        AZSTD_BIND_OPERATOR(<=,  bind_less_equal)

        AZSTD_BIND_OPERATOR(>,  bind_greater)
        AZSTD_BIND_OPERATOR(>=,  bind_greater_equal)

        AZSTD_BIND_OPERATOR(&&, bind_logical_and)
        AZSTD_BIND_OPERATOR(||, bind_logical_or)

    #undef AZSTD_BIND_OPERATOR

        /*
        #if defined(AZ_COMPILER_GCC) && (__GNUC__ < 3)

        // resolve ambiguity with rel_ops

        #define AZSTD_BIND_OPERATOR( op, name ) \
        \
        template<class R, class F, class L> \
            bind_t< bool, name, list2< bind_t<R, F, L>, bind_t<R, F, L> > > \
            operator op (bind_t<R, F, L> const & f, bind_t<R, F, L> const & g) \
        { \
            typedef list2< bind_t<R, F, L>, bind_t<R, F, L> > list_type; \
            return bind_t<bool, name, list_type> ( name(), list_type(f, g) ); \
        }

        AZSTD_BIND_OPERATOR( !=, not_equal )
        AZSTD_BIND_OPERATOR( <=, less_equal )
        AZSTD_BIND_OPERATOR( >, greater )
        AZSTD_BIND_OPERATOR( >=, greater_equal )

        #endif
        */

        // visit_each, ADL
        template<class V, class T>
        void visit_each(V& v, value<T> const& t, int)
        {
            AZStd::visit_each(v, t.get(), 0);
            //using AZStd::visit_each;
            //AZSTD_BIND_VISIT_EACH( v, t.get(), 0 );
        }

        template<class V, class R, class F, class L>
        void visit_each(V& v, bind_t<R, F, L> const& t, int)
        {
            t.accept(v);
        }
    } // namespace Internal

    // isInternalnd_expression

    template< class T >
    struct isInternalnd_expression
    {
        enum _vt
        {
            value = 0
        };
    };

    template< class R, class F, class L >
    struct isInternalnd_expression< Internal::bind_t< R, F, L > >
    {
        enum _vt
        {
            value = 1
        };
    };

    // bind

    #ifndef AZSTD_BIND
    #define AZSTD_BIND bind
    #endif

    // generic function objects

    template<class R, class F>
    Internal::bind_t<R, F, Internal::list0>
    AZSTD_BIND(F f)
    {
        typedef Internal::list0 list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type());
    }

    template<class R, class F, class A1>
    Internal::bind_t<R, F, typename Internal::list_av_1<A1>::type>
    AZSTD_BIND(F f, A1 a1)
    {
        typedef typename Internal::list_av_1<A1>::type list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type(a1));
    }

    template<class R, class F, class A1, class A2>
    Internal::bind_t<R, F, typename Internal::list_av_2<A1, A2>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2)
    {
        typedef typename Internal::list_av_2<A1, A2>::type list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type(a1, a2));
    }

    template<class R, class F, class A1, class A2, class A3>
    Internal::bind_t<R, F, typename Internal::list_av_3<A1, A2, A3>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3)
    {
        typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3));
    }

    template<class R, class F, class A1, class A2, class A3, class A4>
    Internal::bind_t<R, F, typename Internal::list_av_4<A1, A2, A3, A4>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4)
    {
        typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5>
    Internal::bind_t<R, F, typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
    {
        typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6>
    Internal::bind_t<R, F, typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
    {
        typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    Internal::bind_t<R, F, typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
    {
        typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    Internal::bind_t<R, F, typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
    {
        typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    Internal::bind_t<R, F, typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
    {
        typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
    }

    // generic function objects, alternative syntax

    template<class R, class F>
    Internal::bind_t<R, F, Internal::list0>
    AZSTD_BIND(AZStd::type<R>, F f)
    {
        typedef Internal::list0 list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type());
    }

    template<class R, class F, class A1>
    Internal::bind_t<R, F, typename Internal::list_av_1<A1>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1)
    {
        typedef typename Internal::list_av_1<A1>::type list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type(a1));
    }

    template<class R, class F, class A1, class A2>
    Internal::bind_t<R, F, typename Internal::list_av_2<A1, A2>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2)
    {
        typedef typename Internal::list_av_2<A1, A2>::type list_type;
        return Internal::bind_t<R, F, list_type> (f, list_type(a1, a2));
    }

    template<class R, class F, class A1, class A2, class A3>
    Internal::bind_t<R, F, typename Internal::list_av_3<A1, A2, A3>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3)
    {
        typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3));
    }

    template<class R, class F, class A1, class A2, class A3, class A4>
    Internal::bind_t<R, F, typename Internal::list_av_4<A1, A2, A3, A4>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4)
    {
        typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5>
    Internal::bind_t<R, F, typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
    {
        typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6>
    Internal::bind_t<R, F, typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
    {
        typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    Internal::bind_t<R, F, typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
    {
        typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    Internal::bind_t<R, F, typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
    {
        typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
    }

    template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    Internal::bind_t<R, F, typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    AZSTD_BIND(AZStd::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
    {
        typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
        return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
    }

    // adaptable function objects

    template<class F>
    Internal::bind_t<Internal::unspecified, F, Internal::list0>
    AZSTD_BIND(F f)
    {
        typedef Internal::list0 list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type> (f, list_type());
    }

    template<class F, class A1>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_1<A1>::type>
    AZSTD_BIND(F f, A1 a1)
    {
        typedef typename Internal::list_av_1<A1>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type> (f, list_type(a1));
    }

    template<class F, class A1, class A2>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_2<A1, A2>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2)
    {
        typedef typename Internal::list_av_2<A1, A2>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type> (f, list_type(a1, a2));
    }

    template<class F, class A1, class A2, class A3>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_3<A1, A2, A3>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3)
    {
        typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3));
    }

    template<class F, class A1, class A2, class A3, class A4>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_4<A1, A2, A3, A4>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4)
    {
        typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4));
    }

    template<class F, class A1, class A2, class A3, class A4, class A5>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
    {
        typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
    {
        typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
    {
        typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
    {
        typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    Internal::bind_t<Internal::unspecified, F, typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    AZSTD_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
    {
        typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
        return Internal::bind_t<Internal::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
    }

    // function pointers

    #define AZSTD_BIND_CC
    #define AZSTD_BIND_ST

    #include <AzCore/std/bind/bind_cc.h>

    #undef AZSTD_BIND_CC
    #undef AZSTD_BIND_ST

    #ifdef AZSTD_BIND_ENABLE_STDCALL

    #define AZSTD_BIND_CC __stdcall
    #define AZSTD_BIND_ST

    #include <AzCore/std/bind/bind_cc.h>

    #undef AZSTD_BIND_CC
    #undef AZSTD_BIND_ST

    #endif

    #ifdef AZSTD_BIND_ENABLE_FASTCALL

    #define AZSTD_BIND_CC __fastcall
    #define AZSTD_BIND_ST

    #include <AzCore/std/bind/bind_cc.h>

    #undef AZSTD_BIND_CC
    #undef AZSTD_BIND_ST

    #endif

    #ifdef AZSTD_BIND_ENABLE_PASCAL

    #define AZSTD_BIND_ST pascal
    #define AZSTD_BIND_CC

    #include <AzCore/std/bind/bind_cc.h>

    #undef AZSTD_BIND_ST
    #undef AZSTD_BIND_CC

    #endif

    // member function pointers

    #define AZSTD_BIND_MF_NAME(X) X
    #define AZSTD_BIND_MF_CC

    #include <AzCore/std/bind/bind_mf_cc.h>
    #include <AzCore/std/bind/bind_mf2_cc.h>

    #undef AZSTD_BIND_MF_NAME
    #undef AZSTD_BIND_MF_CC

    #ifdef AZSTD_MEM_FN_ENABLE_CDECL

    #define AZSTD_BIND_MF_NAME(X) X##_cdecl
    #define AZSTD_BIND_MF_CC __cdecl

    #include <AzCore/std/bind/bind_mf_cc.h>
    #include <AzCore/std/bind/bind_mf2_cc.h>

    #undef AZSTD_BIND_MF_NAME
    #undef AZSTD_BIND_MF_CC

    #endif

    #ifdef AZSTD_MEM_FN_ENABLE_STDCALL

    #define AZSTD_BIND_MF_NAME(X) X##_stdcall
    #define AZSTD_BIND_MF_CC __stdcall

    #include <AzCore/std/bind/bind_mf_cc.h>
    #include <AzCore/std/bind/bind_mf2_cc.h>

    #undef AZSTD_BIND_MF_NAME
    #undef AZSTD_BIND_MF_CC

    #endif

    #ifdef AZSTD_MEM_FN_ENABLE_FASTCALL

    #define AZSTD_BIND_MF_NAME(X) X##_fastcall
    #define AZSTD_BIND_MF_CC __fastcall

    #include <AzCore/std/bind/bind_mf_cc.h>
    #include <AzCore/std/bind/bind_mf2_cc.h>

    #undef AZSTD_BIND_MF_NAME
    #undef AZSTD_BIND_MF_CC

    #endif

    // data member pointers

    namespace Internal
    {
        template< class Pm, int I >
        struct add_cref;

        template< class M, class T >
        struct add_cref< M T::*, 0 >
        {
            typedef M type;
        };

        template< class M, class T >
        struct add_cref< M T::*, 1 >
        {
            typedef M const& type;
        };

        template< class R, class T >
        struct add_cref< R (T::*)(), 1 >
        {
            typedef void type;
        };

        template< class R, class T >
        struct add_cref< R (T::*)() const, 1 >
        {
            typedef void type;
        };

        template<class R>
        struct isref
        {
            enum value_type
            {
                value = 0
            };
        };

        template<class R>
        struct isref< R& >
        {
            enum value_type
            {
                value = 1
            };
        };

        template<class R>
        struct isref< R* >
        {
            enum value_type
            {
                value = 1
            };
        };

        template<class Pm, class A1>
        struct dm_result
        {
            typedef typename add_cref< Pm, 1 >::type type;
        };

        template<class Pm, class R, class F, class L>
        struct dm_result< Pm, bind_t<R, F, L> >
        {
            typedef typename bind_t<R, F, L>::result_type result_type;
            typedef typename add_cref< Pm, isref< result_type >::value >::type type;
        };
    } // namespace Internal

    template< class A1, class M, class T >
    Internal::bind_t<typename Internal::dm_result< M T::*, A1 >::type, Internal::dm<M, T>, typename Internal::list_av_1<A1>::type>

    AZSTD_BIND(M T::* f, A1 a1)
    {
        typedef typename Internal::dm_result< M T::*, A1 >::type result_type;
        typedef Internal::dm<M, T> F;
        typedef typename Internal::list_av_1<A1>::type list_type;
        return Internal::bind_t< result_type, F, list_type >(F(f), list_type(a1));
    }
}     // namespace AZStd

    # include <AzCore/std/bind/placeholders.h>

    #ifdef AZ_COMPILER_MSVC
        # pragma warning(default: 4512) // assignment operator could not be generated
        # pragma warning(pop)
    #endif

#endif // !AZ_CORE_STD_SYSTEM_BIND

#endif // #ifndef AZSTD_BIND_BIND_HPP_INCLUDED
#pragma once