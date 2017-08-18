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
#ifndef AZSTD_BIND_STORAGE_HPP_INCLUDED
#define AZSTD_BIND_STORAGE_HPP_INCLUDED

// Based on boost 1.39.0
#include <AzCore/std/base.h>
#include <AzCore/std/bind/arg.h>

#ifdef AZ_COMPILER_MSVC
    # pragma warning(push)
    # pragma warning(disable: 4512) // assignment operator could not be generated
#endif

namespace AZStd
{
    namespace Internal
    {
        // 1
        template<class A1>
        struct storage1
        {
            explicit storage1(A1 a1)
                : a1_(a1) {}

            template<class V>
            void accept(V& v) const
            {
                AZSTD_BIND_VISIT_EACH(v, a1_, 0);
            }

            A1 a1_;
        };

        template<int I>
        struct storage1< AZStd::arg<I> >
        {
            explicit storage1(AZStd::arg<I> ) {}

            template<class V>
            void accept(V&) const { }

            static AZStd::arg<I> a1_() { return AZStd::arg<I>(); }
        };

        template<int I>
        struct storage1< AZStd::arg<I> (*)() >
        {
            explicit storage1(AZStd::arg<I> (*)()) {}

            template<class V>
            void accept(V&) const { }

            static AZStd::arg<I> a1_() { return AZStd::arg<I>(); }
        };

        // 2
        template<class A1, class A2>
        struct storage2
            : public storage1<A1>
        {
            typedef storage1<A1> inherited;

            storage2(A1 a1, A2 a2)
                : storage1<A1>(a1)
                , a2_(a2) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a2_, 0);
            }

            A2 a2_;
        };

        template<class A1, int I>
        struct storage2< A1, AZStd::arg<I> >
            : public storage1<A1>
        {
            typedef storage1<A1> inherited;

            storage2(A1 a1, AZStd::arg<I> )
                : storage1<A1>(a1) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a2_() { return AZStd::arg<I>(); }
        };

        template<class A1, int I>
        struct storage2< A1, AZStd::arg<I> (*)() >
            : public storage1<A1>
        {
            typedef storage1<A1> inherited;

            storage2(A1 a1, AZStd::arg<I> (*)())
                : storage1<A1>(a1) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a2_() { return AZStd::arg<I>(); }
        };

        // 3

        template<class A1, class A2, class A3>
        struct storage3
            : public storage2< A1, A2 >
        {
            typedef storage2<A1, A2> inherited;

            storage3(A1 a1, A2 a2, A3 a3)
                : storage2<A1, A2>(a1, a2)
                , a3_(a3) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a3_, 0);
            }

            A3 a3_;
        };

        template<class A1, class A2, int I>
        struct storage3< A1, A2, AZStd::arg<I> >
            : public storage2< A1, A2 >
        {
            typedef storage2<A1, A2> inherited;

            storage3(A1 a1, A2 a2, AZStd::arg<I> )
                : storage2<A1, A2>(a1, a2) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a3_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, int I>
        struct storage3< A1, A2, AZStd::arg<I> (*)() >
            : public storage2< A1, A2 >
        {
            typedef storage2<A1, A2> inherited;

            storage3(A1 a1, A2 a2, AZStd::arg<I> (*)())
                : storage2<A1, A2>(a1, a2) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a3_() { return AZStd::arg<I>(); }
        };

        // 4

        template<class A1, class A2, class A3, class A4>
        struct storage4
            : public storage3< A1, A2, A3 >
        {
            typedef storage3<A1, A2, A3> inherited;

            storage4(A1 a1, A2 a2, A3 a3, A4 a4)
                : storage3<A1, A2, A3>(a1, a2, a3)
                , a4_(a4) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a4_, 0);
            }

            A4 a4_;
        };

        template<class A1, class A2, class A3, int I>
        struct storage4< A1, A2, A3, AZStd::arg<I> >
            : public storage3< A1, A2, A3 >
        {
            typedef storage3<A1, A2, A3> inherited;

            storage4(A1 a1, A2 a2, A3 a3, AZStd::arg<I> )
                : storage3<A1, A2, A3>(a1, a2, a3) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a4_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, int I>
        struct storage4< A1, A2, A3, AZStd::arg<I> (*)() >
            : public storage3< A1, A2, A3 >
        {
            typedef storage3<A1, A2, A3> inherited;

            storage4(A1 a1, A2 a2, A3 a3, AZStd::arg<I> (*)())
                : storage3<A1, A2, A3>(a1, a2, a3) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a4_() { return AZStd::arg<I>(); }
        };


        // 5
        template<class A1, class A2, class A3, class A4, class A5>
        struct storage5
            : public storage4< A1, A2, A3, A4 >
        {
            typedef storage4<A1, A2, A3, A4> inherited;

            storage5(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
                : storage4<A1, A2, A3, A4>(a1, a2, a3, a4)
                , a5_(a5) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a5_, 0);
            }

            A5 a5_;
        };

        template<class A1, class A2, class A3, class A4, int I>
        struct storage5< A1, A2, A3, A4, AZStd::arg<I> >
            : public storage4< A1, A2, A3, A4 >
        {
            typedef storage4<A1, A2, A3, A4> inherited;

            storage5(A1 a1, A2 a2, A3 a3, A4 a4, AZStd::arg<I> )
                : storage4<A1, A2, A3, A4>(a1, a2, a3, a4) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a5_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, class A4, int I>
        struct storage5< A1, A2, A3, A4, AZStd::arg<I> (*)() >
            : public storage4< A1, A2, A3, A4 >
        {
            typedef storage4<A1, A2, A3, A4> inherited;

            storage5(A1 a1, A2 a2, A3 a3, A4 a4, AZStd::arg<I> (*)())
                : storage4<A1, A2, A3, A4>(a1, a2, a3, a4) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a5_() { return AZStd::arg<I>(); }
        };

        // 6

        template<class A1, class A2, class A3, class A4, class A5, class A6>
        struct storage6
            : public storage5< A1, A2, A3, A4, A5 >
        {
            typedef storage5<A1, A2, A3, A4, A5> inherited;

            storage6(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
                : storage5<A1, A2, A3, A4, A5>(a1, a2, a3, a4, a5)
                , a6_(a6) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a6_, 0);
            }

            A6 a6_;
        };

        template<class A1, class A2, class A3, class A4, class A5, int I>
        struct storage6< A1, A2, A3, A4, A5, AZStd::arg<I> >
            : public storage5< A1, A2, A3, A4, A5 >
        {
            typedef storage5<A1, A2, A3, A4, A5> inherited;

            storage6(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, AZStd::arg<I> )
                : storage5<A1, A2, A3, A4, A5>(a1, a2, a3, a4, a5) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a6_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, class A4, class A5, int I>
        struct storage6< A1, A2, A3, A4, A5, AZStd::arg<I> (*)() >
            : public storage5< A1, A2, A3, A4, A5 >
        {
            typedef storage5<A1, A2, A3, A4, A5> inherited;

            storage6(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, AZStd::arg<I> (*)())
                : storage5<A1, A2, A3, A4, A5>(a1, a2, a3, a4, a5) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a6_() { return AZStd::arg<I>(); }
        };

        // 7

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7>
        struct storage7
            : public storage6< A1, A2, A3, A4, A5, A6 >
        {
            typedef storage6<A1, A2, A3, A4, A5, A6> inherited;

            storage7(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
                : storage6<A1, A2, A3, A4, A5, A6>(a1, a2, a3, a4, a5, a6)
                , a7_(a7) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a7_, 0);
            }

            A7 a7_;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, int I>
        struct storage7< A1, A2, A3, A4, A5, A6, AZStd::arg<I> >
            : public storage6< A1, A2, A3, A4, A5, A6 >
        {
            typedef storage6<A1, A2, A3, A4, A5, A6> inherited;

            storage7(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, AZStd::arg<I> )
                : storage6<A1, A2, A3, A4, A5, A6>(a1, a2, a3, a4, a5, a6) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a7_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, int I>
        struct storage7< A1, A2, A3, A4, A5, A6, AZStd::arg<I> (*)() >
            : public storage6< A1, A2, A3, A4, A5, A6 >
        {
            typedef storage6<A1, A2, A3, A4, A5, A6> inherited;

            storage7(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, AZStd::arg<I> (*)())
                : storage6<A1, A2, A3, A4, A5, A6>(a1, a2, a3, a4, a5, a6) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a7_() { return AZStd::arg<I>(); }
        };


        // 8
        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
        struct storage8
            : public storage7< A1, A2, A3, A4, A5, A6, A7 >
        {
            typedef storage7<A1, A2, A3, A4, A5, A6, A7> inherited;

            storage8(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
                : storage7<A1, A2, A3, A4, A5, A6, A7>(a1, a2, a3, a4, a5, a6, a7)
                , a8_(a8) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a8_, 0);
            }

            A8 a8_;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, int I>
        struct storage8< A1, A2, A3, A4, A5, A6, A7, AZStd::arg<I> >
            : public storage7< A1, A2, A3, A4, A5, A6, A7 >
        {
            typedef storage7<A1, A2, A3, A4, A5, A6, A7> inherited;

            storage8(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, AZStd::arg<I> )
                : storage7<A1, A2, A3, A4, A5, A6, A7>(a1, a2, a3, a4, a5, a6, a7) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a8_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, int I>
        struct storage8< A1, A2, A3, A4, A5, A6, A7, AZStd::arg<I> (*)() >
            : public storage7< A1, A2, A3, A4, A5, A6, A7 >
        {
            typedef storage7<A1, A2, A3, A4, A5, A6, A7> inherited;

            storage8(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, AZStd::arg<I> (*)())
                : storage7<A1, A2, A3, A4, A5, A6, A7>(a1, a2, a3, a4, a5, a6, a7) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a8_() { return AZStd::arg<I>(); }
        };

        // 9
        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
        struct storage9
            : public storage8< A1, A2, A3, A4, A5, A6, A7, A8 >
        {
            typedef storage8<A1, A2, A3, A4, A5, A6, A7, A8> inherited;

            storage9(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
                : storage8<A1, A2, A3, A4, A5, A6, A7, A8>(a1, a2, a3, a4, a5, a6, a7, a8)
                , a9_(a9) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
                AZSTD_BIND_VISIT_EACH(v, a9_, 0);
            }

            A9 a9_;
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, int I>
        struct storage9< A1, A2, A3, A4, A5, A6, A7, A8, AZStd::arg<I> >
            : public storage8< A1, A2, A3, A4, A5, A6, A7, A8 >
        {
            typedef storage8<A1, A2, A3, A4, A5, A6, A7, A8> inherited;

            storage9(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, AZStd::arg<I> )
                : storage8<A1, A2, A3, A4, A5, A6, A7, A8>(a1, a2, a3, a4, a5, a6, a7, a8) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a9_() { return AZStd::arg<I>(); }
        };

        template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, int I>
        struct storage9< A1, A2, A3, A4, A5, A6, A7, A8, AZStd::arg<I> (*)() >
            : public storage8< A1, A2, A3, A4, A5, A6, A7, A8 >
        {
            typedef storage8<A1, A2, A3, A4, A5, A6, A7, A8> inherited;

            storage9(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, AZStd::arg<I> (*)())
                : storage8<A1, A2, A3, A4, A5, A6, A7, A8>(a1, a2, a3, a4, a5, a6, a7, a8) {}

            template<class V>
            void accept(V& v) const
            {
                inherited::accept(v);
            }

            static AZStd::arg<I> a9_() { return AZStd::arg<I>(); }
        };
    }
}

#ifdef AZ_COMPILER_MSVC
    # pragma warning(default: 4512) // assignment operator could not be generated
    # pragma warning(pop)
#endif

#endif
#pragma once