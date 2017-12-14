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
#ifndef AZSTD_BIND_MEM_FN_HPP_INCLUDED
#define AZSTD_BIND_MEM_FN_HPP_INCLUDED

// Based on boost 1.39.0

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/function_traits.h>

namespace AZStd
{
    #define AZSTD_MEM_FN_CLASS_F
    #define AZSTD_MEM_FN_TYPEDEF(X) typedef X;

    namespace Internal
    {
        #define AZSTD_MEM_FN_RETURN return

        #define AZSTD_MEM_FN_NAME(X) X
        #define AZSTD_MEM_FN_CC

        #include <AzCore/std/bind/mem_fn_template.h>

        #undef AZSTD_MEM_FN_CC
        #undef AZSTD_MEM_FN_NAME

        #ifdef AZSTD_MEM_FN_ENABLE_CDECL

            #define AZSTD_MEM_FN_NAME(X) X##_cdecl
            #define AZSTD_MEM_FN_CC __cdecl

            #include <AzCore/std/bind/mem_fn_template.h>

            #undef AZSTD_MEM_FN_CC
            #undef AZSTD_MEM_FN_NAME

        #endif

        #ifdef AZSTD_MEM_FN_ENABLE_STDCALL

            #define AZSTD_MEM_FN_NAME(X) X##_stdcall
            #define AZSTD_MEM_FN_CC __stdcall

            #include <AzCore/std/bind/mem_fn_template.h>

            #undef AZSTD_MEM_FN_CC
            #undef AZSTD_MEM_FN_NAME

        #endif

        #ifdef AZSTD_MEM_FN_ENABLE_FASTCALL

            #define AZSTD_MEM_FN_NAME(X) X##_fastcall
            #define AZSTD_MEM_FN_CC __fastcall

            #include <AzCore/std/bind/mem_fn_template.h>

            #undef AZSTD_MEM_FN_CC
            #undef AZSTD_MEM_FN_NAME

        #endif

    #undef AZSTD_MEM_FN_RETURN
    } // namespace Internal

    #undef AZSTD_MEM_FN_CLASS_F
    #undef AZSTD_MEM_FN_TYPEDEF

    #define AZSTD_MEM_FN_NAME(X) X
    #define AZSTD_MEM_FN_CC

    #include <AzCore/std/bind/mem_fn_cc.h>

    #undef AZSTD_MEM_FN_NAME
    #undef AZSTD_MEM_FN_CC

    #ifdef AZSTD_MEM_FN_ENABLE_CDECL

        #define AZSTD_MEM_FN_NAME(X) X##_cdecl
        #define AZSTD_MEM_FN_CC __cdecl

        #include <AzCore/std/bind/mem_fn_cc.h>

        #undef AZSTD_MEM_FN_NAME
        #undef AZSTD_MEM_FN_CC

    #endif

    #ifdef AZSTD_MEM_FN_ENABLE_STDCALL

        #define AZSTD_MEM_FN_NAME(X) X##_stdcall
        #define AZSTD_MEM_FN_CC __stdcall

        #include <AzCore/std/bind/mem_fn_cc.h>

        #undef AZSTD_MEM_FN_NAME
        #undef AZSTD_MEM_FN_CC

    #endif

    #ifdef AZSTD_MEM_FN_ENABLE_FASTCALL

        #define AZSTD_MEM_FN_NAME(X) X##_fastcall
        #define AZSTD_MEM_FN_CC __fastcall

        #include <AzCore/std/bind/mem_fn_cc.h>

        #undef AZSTD_MEM_FN_NAME
        #undef AZSTD_MEM_FN_CC

    #endif

    // data member support
    namespace Internal
    {
        template<class R, class T>
        class dm
        {
        public:

            typedef R const& result_type;
            typedef T const* argument_type;

        private:

            typedef R (T::* F);
            F f_;

            template<class U>
            R const& call(U& u, T const*) const
            {
                return (u.*f_);
            }

            template<class U>
            R const& call(U& u, void const*) const
            {
                return (get_pointer(u)->*f_);
            }

        public:

            explicit dm(F f)
                : f_(f) {}

            R& operator()(T* p) const
            {
                return (p->*f_);
            }

            R const& operator()(T const* p) const
            {
                return (p->*f_);
            }

            template<class U>
            R const& operator()(U const& u) const
            {
                return call(u, &u);
            }

            R& operator()(T& t) const
            {
                return (t.*f_);
            }

            R const& operator()(T const& t) const
            {
                return (t.*f_);
            }

            bool operator==(dm const& rhs) const
            {
                return f_ == rhs.f_;
            }

            bool operator!=(dm const& rhs) const
            {
                return f_ != rhs.f_;
            }
        };
    }

    template<class R, class T>
    Internal::dm<R, T> mem_fn(R T::* f)
    {
        return Internal::dm<R, T>(f);
    }
}

#endif // #ifndef AZSTD_BIND_MEM_FN_HPP_INCLUDED
#pragma once