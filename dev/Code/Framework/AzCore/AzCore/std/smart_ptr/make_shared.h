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
#ifndef AZSTD_SMART_PTR_MAKE_SHARED_H
#define AZSTD_SMART_PTR_MAKE_SHARED_H

//  make_shared.hpp
//
//  Copyright (c) 2007, 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/make_shared.html
//  for documentation.

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/typetraits/aligned_storage.h>

namespace AZStd
{
    namespace Internal
    {
        template< class T >
        class sp_ms_deleter
        {
        private:

            typedef typename AZStd::aligned_storage< sizeof(T), ::AZStd::alignment_of< T >::value >::type storage_type;

            bool initialized_;
            storage_type storage_;

        private:

            void destroy()
            {
                if (initialized_)
                {
                    reinterpret_cast< T* >(&storage_)->~T();
                    initialized_ = false;
                }
            }

        public:
            sp_ms_deleter()
                : initialized_(false)
            {
            }

            // optimization: do not copy storage_
            sp_ms_deleter(sp_ms_deleter const&)
                : initialized_(false)
            {
            }

            ~sp_ms_deleter()
            {
                destroy();
            }

            void operator()(T*)
            {
                destroy();
            }

            void* address()
            {
                return &storage_;
            }

            void set_initialized()
            {
                initialized_ = true;
            }
        };

    #if defined(AZ_HAS_RVALUE_REFS)
        template< class T >
        T && sp_forward(T & t)
        {
            return static_cast< T && >(t);
        }
    #endif
    } // namespace Internal

    // Zero-argument versions
    //
    // Used even when variadic templates are available because of the new T() vs new T issue

    template< class T >
    AZStd::shared_ptr< T > make_shared()
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag< AZStd::Internal::sp_ms_deleter< T > >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A >
    AZStd::shared_ptr< T > allocate_shared(A const& a)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag< AZStd::Internal::sp_ms_deleter< T > >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_HAS_RVALUE_REFS)
    // Variadic templates, rvalue reference
    template< class T, class ... Args >
    AZStd::shared_ptr< T > make_shared(Args&& ... args)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_inplace_tag<AZStd::Internal::sp_ms_deleter< T > >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(AZStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class Arg1, class ... Args >
    AZStd::shared_ptr< T > allocate_shared(A const& a, Arg1&& arg1, Args&& ... args)
    {
        AZStd::shared_ptr< T > pt(static_cast<T*>(0), AZStd::Internal::sp_inplace_tag<AZStd::Internal::sp_ms_deleter< T > >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new(pv)T(AZStd::Internal::sp_forward<Arg1>(arg1), AZStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast<T*>(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

#else

    // C++03 version

    template< class T, class A1 >
    AZStd::shared_ptr< T > make_shared(A1 const& a1)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class A1 >
    AZStd::shared_ptr< T > allocate_shared(A const& a, A1 const& a1)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A1, class A2 >
    AZStd::shared_ptr< T > make_shared(A1 const& a1, A2 const& a2)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class A1, class A2 >
    AZStd::shared_ptr< T > allocate_shared(A const& a, A1 const& a1, A2 const& a2)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A1, class A2, class A3 >
    AZStd::shared_ptr< T > make_shared(A1 const& a1, A2 const& a2, A3 const& a3)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class A1, class A2, class A3 >
    AZStd::shared_ptr< T > allocate_shared(A const& a, A1 const& a1, A2 const& a2, A3 const& a3)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A1, class A2, class A3, class A4 >
    AZStd::shared_ptr< T > make_shared(A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3, a4);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class A1, class A2, class A3, class A4 >
    AZStd::shared_ptr< T > allocate_shared(A const& a, A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3, a4);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A1, class A2, class A3, class A4, class A5 >
    AZStd::shared_ptr< T > make_shared(A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4, A5 const& a5)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >());

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3, a4, a5);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class A1, class A2, class A3, class A4, class A5 >
    AZStd::shared_ptr< T > allocate_shared(A const& a, A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4, A5 const& a5)
    {
        AZStd::shared_ptr< T > pt(static_cast< T* >(0), AZStd::Internal::sp_ms_deleter< T >(), a);

        AZStd::Internal::sp_ms_deleter< T >* pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(a1, a2, a3, a4, a5);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        AZStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return AZStd::shared_ptr< T >(pt, pt2);
    }

    // uncomment if we need more
    //template< class T, class A1, class A2, class A3, class A4, class A5, class A6 >
    //AZStd::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >() );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6 >
    //AZStd::shared_ptr< T > allocate_shared( A const & a, A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >(), a );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
    //AZStd::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >() );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
    //AZStd::shared_ptr< T > allocate_shared( A const & a, A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >(), a );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
    //AZStd::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >() );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
    //AZStd::shared_ptr< T > allocate_shared( A const & a, A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >(), a );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
    //AZStd::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >() );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}

    //template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
    //AZStd::shared_ptr< T > allocate_shared( A const & a, A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9 )
    //{
    //  AZStd::shared_ptr< T > pt( static_cast< T* >( 0 ), AZStd::Internal::sp_ms_deleter< T >(), a );

    //  AZStd::Internal::sp_ms_deleter< T > * pd = AZStd::get_deleter< AZStd::Internal::sp_ms_deleter< T > >( pt );

    //  void * pv = pd->address();

    //  ::new( pv ) T( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
    //  pd->set_initialized();

    //  T * pt2 = static_cast< T* >( pv );

    //  AZStd::Internal::sp_enable_shared_from_this( &pt, pt2, pt2 );
    //  return AZStd::shared_ptr< T >( pt, pt2 );
    //}
#endif
} // namespace AZStd

#endif // #ifndef AZSTD_SMART_PTR_MAKE_SHARED_H
#pragma once