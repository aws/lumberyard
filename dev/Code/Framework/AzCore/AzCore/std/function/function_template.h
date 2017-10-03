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
// Based on boost 1.39.0

// Note: this header is a header template and must NOT have multiple-inclusion
// protection.

#include <AzCore/std/function/function_base.h>

#if defined(AZ_COMPILER_MSVC)
#   pragma warning( push )
#   pragma warning( disable : 4127 ) // "conditional expression is constant"
#endif

// can't wait for varadic templates.
#if     (AZSTD_FUNCTION_NUM_ARGS == 0)
    #define AZSTD_FUNCTION_TEMPLATE_PARMS
    #define AZSTD_FUNCTION_TEMPLATE_ARGS
    #define AZSTD_FUNCTION_PARMS
    #define AZSTD_FUNCTION_ARGS
    #define AZSTD_FUNCTION_ARG_TYPES
#elif   (AZSTD_FUNCTION_NUM_ARGS == 1)
    #define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0
#define AZSTD_FUNCTION_PARMS            T0 t0
#define AZSTD_FUNCTION_ARGS             t0
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 2)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1
#define AZSTD_FUNCTION_ARGS             t0, t1
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 3)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2
#define AZSTD_FUNCTION_ARGS             t0, t1, t2
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 4)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 5)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 6)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4, typename T5
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4, T5
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4, t5
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type; \
    typedef T5 t5_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 7)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4, T5, T6
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4, t5, t6
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type; \
    typedef T5 t5_type; typedef T6 t6_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 8)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4, T5, T6, T7
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4, t5, t6, t7
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type; \
    typedef T5 t5_type; typedef T6 t6_type; typedef T7 t7_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 9)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4, T5, T6, T7, T8
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4, t5, t6, t7, t8
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type; \
    typedef T5 t5_type; typedef T6 t6_type; typedef T7 t7_type; typedef T8 t8_type;
#elif   (AZSTD_FUNCTION_NUM_ARGS == 10)
#define AZSTD_FUNCTION_TEMPLATE_PARMS   typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9
#define AZSTD_FUNCTION_TEMPLATE_ARGS    T0, T1, T2, T3, T4, T5, T6, T7, T8, T9
#define AZSTD_FUNCTION_PARMS            T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9
#define AZSTD_FUNCTION_ARGS             t0, t1, t2, t3, t4, t5, t6, t7, t8, t9
#define AZSTD_FUNCTION_ARG_TYPES        typedef T0 t0_type; typedef T1 t1_type; typedef T2 t2_type; typedef T3 t3_type; typedef T4 t4_type; \
    typedef T5 t5_type; typedef T6 t6_type; typedef T7 t7_type; typedef T8 t8_type; typedef T9 t9_type;
#else
#error Too many arguments
#endif

// Comma if nonzero number of arguments
#if AZSTD_FUNCTION_NUM_ARGS == 0
#  define AZSTD_FUNCTION_COMMA
#else
#  define AZSTD_FUNCTION_COMMA ,
#endif // AZSTD_FUNCTION_NUM_ARGS > 0

// Class names used in this version of the code
#define AZSTD_FUNCTION_FUNCTION AZ_JOIN(function, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_FUNCTION_INVOKER AZ_JOIN(function_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_VOID_FUNCTION_INVOKER AZ_JOIN(void_function_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_FUNCTION_OBJ_INVOKER AZ_JOIN(function_obj_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_VOID_FUNCTION_OBJ_INVOKER AZ_JOIN(void_function_obj_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_FUNCTION_REF_INVOKER AZ_JOIN(function_ref_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_VOID_FUNCTION_REF_INVOKER AZ_JOIN(void_function_ref_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_MEMBER_INVOKER AZ_JOIN(function_mem_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_VOID_MEMBER_INVOKER AZ_JOIN(function_void_mem_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_GET_FUNCTION_INVOKER AZ_JOIN(get_function_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_GET_FUNCTION_OBJ_INVOKER AZ_JOIN(get_function_obj_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_GET_FUNCTION_REF_INVOKER AZ_JOIN(get_function_ref_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_GET_MEMBER_INVOKER AZ_JOIN(get_member_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_GET_INVOKER AZ_JOIN(get_invoker, AZSTD_FUNCTION_NUM_ARGS)
#define AZSTD_FUNCTION_VTABLE AZ_JOIN(basic_vtable, AZSTD_FUNCTION_NUM_ARGS)

#define AZSTD_FUNCTION_VOID_RETURN_TYPE void
#define AZSTD_FUNCTION_RETURN(X) X

namespace AZStd
{
    namespace Internal
    {
        namespace function_util
        {
            template<typename FunctionPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_FUNCTION_INVOKER
            {
                static R invoke(function_buffer& function_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionPtr f = reinterpret_cast<FunctionPtr>(function_ptr.func_ptr);
                    return f(AZSTD_FUNCTION_ARGS);
                }
            };

            template<typename FunctionPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_VOID_FUNCTION_INVOKER
            {
                static AZSTD_FUNCTION_VOID_RETURN_TYPE
                invoke(function_buffer& function_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionPtr f = reinterpret_cast<FunctionPtr>(function_ptr.func_ptr);
                    AZSTD_FUNCTION_RETURN(f(AZSTD_FUNCTION_ARGS));
                }
            };

            template<typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_FUNCTION_OBJ_INVOKER
            {
                static R invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionObj* f;
                    if (function_allows_small_object_optimization<FunctionObj>::value)
                    {
                        f = reinterpret_cast<FunctionObj*>(&function_obj_ptr.data);
                    }
                    else
                    {
                        f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    }
                    return (*f)(AZSTD_FUNCTION_ARGS);
                }
            };

            template< typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_VOID_FUNCTION_OBJ_INVOKER
            {
                static AZSTD_FUNCTION_VOID_RETURN_TYPE
                invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionObj* f;
                    if (function_allows_small_object_optimization<FunctionObj>::value)
                    {
                        f = reinterpret_cast<FunctionObj*>(&function_obj_ptr.data);
                    }
                    else
                    {
                        f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    }
                    AZSTD_FUNCTION_RETURN((*f)(AZSTD_FUNCTION_ARGS));
                }
            };

            template< typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_FUNCTION_REF_INVOKER
            {
                static R invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionObj* f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    return (*f)(AZSTD_FUNCTION_ARGS);
                }
            };

            template< typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_VOID_FUNCTION_REF_INVOKER
            {
                static AZSTD_FUNCTION_VOID_RETURN_TYPE
                invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    FunctionObj* f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    AZSTD_FUNCTION_RETURN((*f)(AZSTD_FUNCTION_ARGS));
                }
            };

#if AZSTD_FUNCTION_NUM_ARGS > 0
            /* Handle invocation of member pointers. */
            template< typename MemberPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_MEMBER_INVOKER
            {
                static R invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    MemberPtr* f = reinterpret_cast<MemberPtr*>(&function_obj_ptr.data);
                    return AZStd::mem_fn(* f)(AZSTD_FUNCTION_ARGS);
                }
            };

            template< typename MemberPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_VOID_MEMBER_INVOKER
            {
                static AZSTD_FUNCTION_VOID_RETURN_TYPE
                invoke(function_buffer& function_obj_ptr AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_PARMS)
                {
                    MemberPtr* f = reinterpret_cast<MemberPtr*>(&function_obj_ptr.data);
                    AZSTD_FUNCTION_RETURN(AZStd::mem_fn(* f)(AZSTD_FUNCTION_ARGS));
                }
            };
#endif

            template< typename FunctionPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_GET_FUNCTION_INVOKER
            {
                typedef typename AZStd::Utils::if_c<(is_void<R>::value), AZSTD_FUNCTION_VOID_FUNCTION_INVOKER<FunctionPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >,
                    AZSTD_FUNCTION_FUNCTION_INVOKER<FunctionPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS > >::type type;
            };

            template< typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_GET_FUNCTION_OBJ_INVOKER
            {
                typedef typename AZStd::Utils::if_c<(is_void<R>::value), AZSTD_FUNCTION_VOID_FUNCTION_OBJ_INVOKER<FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >,
                    AZSTD_FUNCTION_FUNCTION_OBJ_INVOKER< FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS > >::type type;
            };

            template<typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS >
            struct AZSTD_FUNCTION_GET_FUNCTION_REF_INVOKER
            {
                typedef typename AZStd::Utils::if_c<(is_void<R>::value), AZSTD_FUNCTION_VOID_FUNCTION_REF_INVOKER< FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >,
                    AZSTD_FUNCTION_FUNCTION_REF_INVOKER<FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS > >::type type;
            };

#if AZSTD_FUNCTION_NUM_ARGS > 0
            /* Retrieve the appropriate invoker for a member pointer.  */
            template<typename MemberPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS
                >
            struct AZSTD_FUNCTION_GET_MEMBER_INVOKER
            {
                typedef typename AZStd::Utils::if_c<(is_void<R>::value), AZSTD_FUNCTION_VOID_MEMBER_INVOKER< MemberPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >,
                    AZSTD_FUNCTION_MEMBER_INVOKER<MemberPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS > >::type type;
            };
#endif

            /* Given the tag returned by get_function_tag, retrieve the
            actual invoker that will be used for the given function
            object.

            Each specialization contains an "apply" nested class template
            that accepts the function object, return type, function
            argument types, and allocator. The resulting "apply" class
            contains two typedefs, "invoker_type" and "manager_type",
            which correspond to the invoker and manager types. */
            template<typename Tag>
            struct AZSTD_FUNCTION_GET_INVOKER { };

            /* Retrieve the invoker for a function pointer. */
            template<>
            struct AZSTD_FUNCTION_GET_INVOKER<function_ptr_tag>
            {
                template<typename FunctionPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
                struct apply
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_INVOKER<FunctionPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type invoker_type;
                    typedef functor_manager<FunctionPtr> manager_type;
                };

                template<typename FunctionPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS, typename Allocator>
                struct apply_a
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_INVOKER<FunctionPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type invoker_type;
                    typedef functor_manager<FunctionPtr> manager_type;
                };
            };

#if AZSTD_FUNCTION_NUM_ARGS > 0
            /* Retrieve the invoker for a member pointer. */
            template<>
            struct AZSTD_FUNCTION_GET_INVOKER<member_ptr_tag>
            {
                template<typename MemberPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
                struct apply
                {
                    typedef typename AZSTD_FUNCTION_GET_MEMBER_INVOKER<MemberPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type     invoker_type;
                    typedef functor_manager<MemberPtr> manager_type;
                };

                template<typename MemberPtr, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS, typename Allocator>
                struct apply_a
                {
                    typedef typename AZSTD_FUNCTION_GET_MEMBER_INVOKER<MemberPtr, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type     invoker_type;
                    typedef functor_manager<MemberPtr> manager_type;
                };
            };
#endif

            /* Retrieve the invoker for a function object. */
            template<>
            struct AZSTD_FUNCTION_GET_INVOKER<function_obj_tag>
            {
                template<typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
                struct apply
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_OBJ_INVOKER<FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type invoker_type;
                    typedef functor_manager<FunctionObj> manager_type;
                };

                template<typename FunctionObj, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS, typename Allocator>
                struct apply_a
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_OBJ_INVOKER<FunctionObj, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type  invoker_type;
                    typedef functor_manager_a<FunctionObj, Allocator> manager_type;
                };
            };

            /* Retrieve the invoker for a reference to a function object. */
            template<>
            struct AZSTD_FUNCTION_GET_INVOKER<function_obj_ref_tag>
            {
                template<typename RefWrapper, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
                struct apply
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_REF_INVOKER<typename RefWrapper::type, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type invoker_type;
                    typedef reference_manager<typename RefWrapper::type> manager_type;
                };

                template<typename RefWrapper, typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS, typename Allocator>
                struct apply_a
                {
                    typedef typename AZSTD_FUNCTION_GET_FUNCTION_REF_INVOKER< typename RefWrapper::type, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >::type invoker_type;
                    typedef reference_manager<typename RefWrapper::type> manager_type;
                };
            };


            /**
            * vtable for a specific boost::function instance. This
            * structure must be an aggregate so that we can use static
            * initialization in boost::function's assign_to and assign_to_a
            * members. It therefore cannot have any constructors,
            * destructors, base classes, etc.
            */
            template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
            struct AZSTD_FUNCTION_VTABLE
            {
                typedef R         result_type;
                typedef result_type (* invoker_type)(function_buffer& AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS);

                template<typename F>
                bool assign_to(F f, function_buffer& functor)
                {
                    typedef typename get_function_tag<F>::type tag;
                    return assign_to(f, functor, tag());
                }
                template<typename F, typename Allocator>
                bool assign_to_a(F f, function_buffer& functor, Allocator a)
                {
                    typedef typename get_function_tag<F>::type tag;
                    return assign_to_a(f, functor, a, tag());
                }

                void clear(function_buffer& functor)
                {
                    if (base.manager)
                    {
                        base.manager(functor, functor, destroy_functor_tag);
                    }
                }
            private:
                // Function pointers
                template<typename FunctionPtr>
                bool
                assign_to(FunctionPtr f, function_buffer& functor, function_ptr_tag)
                {
                    this->clear(functor);
                    if (f)
                    {
                        // should be a reinterpret cast, but some compilers insist
                        // on giving cv-qualifiers to free functions
                        functor.func_ptr = (void (*)())(f);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionPtr, typename Allocator>
                bool
                assign_to_a(FunctionPtr f, function_buffer& functor, Allocator, function_ptr_tag)
                {
                    return assign_to(f, functor, function_ptr_tag());
                }

                // Member pointers
#if AZSTD_FUNCTION_NUM_ARGS > 0
                template<typename MemberPtr>
                bool assign_to(MemberPtr f, function_buffer& functor, member_ptr_tag)
                {
                    // DPG TBD: Add explicit support for member function
                    // objects, so we invoke through mem_fn() but we retain the
                    // right target_type() values.
                    if (f)
                    {
                        this->assign_to(mem_fn(f), functor);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename MemberPtr, typename Allocator>
                bool assign_to_a(MemberPtr f, function_buffer& functor, Allocator a, member_ptr_tag)
                {
                    // DPG TBD: Add explicit support for member function
                    // objects, so we invoke through mem_fn() but we retain the
                    // right target_type() values.
                    if (f)
                    {
                        this->assign_to_a(mem_fn(f), functor, a);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
#endif // AZSTD_FUNCTION_NUM_ARGS > 0

                // Function objects
                // Assign to a function object using the small object optimization
                template<typename FunctionObj>
                void
                assign_functor(FunctionObj f, function_buffer& functor, AZStd::true_type)
                {
                    new ((void*)&functor.data)FunctionObj(f);
                }
                template<typename FunctionObj, typename Allocator>
                void
                assign_functor_a(FunctionObj f, function_buffer& functor, Allocator, AZStd::true_type)
                {
                    assign_functor(f, functor, AZStd::true_type());
                }

                // Assign to a function object allocated on the heap.
                template<typename FunctionObj>
                void assign_functor(FunctionObj f, function_buffer& functor, AZStd::false_type)
                {
                    AZStd::allocator a;
                    functor.obj_ptr = new (a.allocate(sizeof(FunctionObj), AZStd::alignment_of<FunctionObj>::value))FunctionObj(f);
                }
                template<typename FunctionObj, typename Allocator>
                void
                assign_functor_a(FunctionObj f, function_buffer& functor, const Allocator& a, AZStd::false_type)
                {
                    typedef functor_wrapper<FunctionObj, Allocator> functor_wrapper_type;
                    functor_wrapper_type* new_f = new (const_cast<Allocator&>(a).allocate(sizeof(functor_wrapper_type), AZStd::alignment_of<functor_wrapper_type>::value))functor_wrapper_type(f, a);
                    functor.obj_ptr = new_f;
                }

                template<typename FunctionObj>
                bool
                assign_to(FunctionObj f, function_buffer& functor, function_obj_tag)
                {
                    if (!AZStd::Internal::function_util::has_empty_target(AZStd::addressof(f)))
                    {
                        assign_functor(f, functor, AZStd::integral_constant<bool, function_allows_small_object_optimization<FunctionObj>::value>());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionObj, typename Allocator>
                bool
                assign_to_a(FunctionObj f, function_buffer& functor, Allocator a, function_obj_tag)
                {
                    if (!AZStd::Internal::function_util::has_empty_target(AZStd::addressof(f)))
                    {
                        assign_functor_a(f, functor, a, AZStd::integral_constant<bool, function_allows_small_object_optimization<FunctionObj>::value>());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // Reference to a function object
                template<typename FunctionObj>
                bool
                assign_to(const reference_wrapper<FunctionObj>& f,
                    function_buffer& functor, function_obj_ref_tag)
                {
                    if (!AZStd::Internal::function_util::has_empty_target(f.get_pointer()))
                    {
                        functor.obj_ref.obj_ptr = (void*)f.get_pointer();
                        functor.obj_ref.is_const_qualified = is_const<FunctionObj>::value;
                        functor.obj_ref.is_volatile_qualified = is_volatile<FunctionObj>::value;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionObj, typename Allocator>
                bool
                assign_to_a(const reference_wrapper<FunctionObj>& f, function_buffer& functor, Allocator, function_obj_ref_tag)
                {
                    return assign_to(f, functor, function_obj_ref_tag());
                }

            public:
                vtable_base base;
                invoker_type invoker;
            };
        } // end namespace function_util
    } // end namespace Internal

    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    class AZSTD_FUNCTION_FUNCTION
        : public function_base
#if AZSTD_FUNCTION_NUM_ARGS == 1
        , public AZStd::unary_function<T0, R>
#elif AZSTD_FUNCTION_NUM_ARGS == 2
        , public AZStd::binary_function<T0, T1, R>
#endif
    {
    public:
        typedef R         result_type;
    private:
        typedef AZStd::Internal::function_util::AZSTD_FUNCTION_VTABLE< R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>  vtable_type;
    public:
        AZSTD_STATIC_CONSTANT(int, args = AZSTD_FUNCTION_NUM_ARGS);
        //// add signature for boost::lambda
        //template<typename Args>
        //struct sig
        //{
        //  typedef result_type type;
        //};

#if AZSTD_FUNCTION_NUM_ARGS == 1
        typedef T0 argument_type;
#elif AZSTD_FUNCTION_NUM_ARGS == 2
        typedef T0 first_argument_type;
        typedef T1 second_argument_type;
#endif
        AZSTD_STATIC_CONSTANT(int, arity = AZSTD_FUNCTION_NUM_ARGS);
        AZSTD_FUNCTION_ARG_TYPES

        typedef AZSTD_FUNCTION_FUNCTION self_type;

        AZSTD_FUNCTION_FUNCTION()
            : function_base() { }

        // MSVC chokes if the following two constructors are collapsed into
        // one with a default parameter.
        template<typename Functor>
        AZSTD_FUNCTION_FUNCTION(Functor AZSTD_FUNCTION_TARGET_FIX(const &)f, typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), int>::type = 0)
            : function_base()
        {
            this->assign_to(f);
        }
        template<typename Functor, typename Allocator>
        AZSTD_FUNCTION_FUNCTION(Functor AZSTD_FUNCTION_TARGET_FIX(const &)f, Allocator a, typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), int>::type = 0)
            : function_base()
        {
            this->assign_to_a(f, a);
        }

        AZSTD_FUNCTION_FUNCTION(nullptr_t)
            : function_base() { }
        AZSTD_FUNCTION_FUNCTION(const AZSTD_FUNCTION_FUNCTION& f)
            : function_base()
        {
            this->assign_to_own(f);
        }

#if defined(AZ_HAS_RVALUE_REFS)
        AZSTD_FUNCTION_FUNCTION(AZSTD_FUNCTION_FUNCTION&& f)
            : function_base()
        {
            this->move_assign(f);
        }
#endif

        ~AZSTD_FUNCTION_FUNCTION() { clear(); }

        result_type operator()(AZSTD_FUNCTION_PARMS) const;

        // The distinction between when to use AZSTD_FUNCTION_FUNCTION and
        // when to use self_type is obnoxious. MSVC cannot handle self_type as
        // the return type of these assignment operators, but Borland C++ cannot
        // handle AZSTD_FUNCTION_FUNCTION as the type of the temporary to
        // construct.
        template<typename Functor>
        typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), AZSTD_FUNCTION_FUNCTION&>::type
        operator=(Functor AZSTD_FUNCTION_TARGET_FIX(const &)f)
        {
            this->clear();
            this->assign_to(f);
            return *this;
        }
        template<typename Functor, typename Allocator>
        void assign(Functor AZSTD_FUNCTION_TARGET_FIX(const &)f, Allocator a)
        {
            this->clear();
            this->assign_to_a(f, a);
        }

        AZSTD_FUNCTION_FUNCTION& operator=(nullptr_t)
        {
            this->clear();
            return *this;
        }

        // Assignment from another AZSTD_FUNCTION_FUNCTION
        AZSTD_FUNCTION_FUNCTION& operator=(const AZSTD_FUNCTION_FUNCTION& f)
        {
            if (&f == this)
            {
                return *this;
            }

            this->clear();
            this->assign_to_own(f);
            return *this;
        }

#if defined(AZ_HAS_RVALUE_REFS)
        // Move assignment from another AZSTD_FUNCTION_FUNCTION
        AZSTD_FUNCTION_FUNCTION& operator=(AZSTD_FUNCTION_FUNCTION&& f)
        {
            if (&f == this)
            {
                return *this;
            }

            this->clear();
            this->move_assign(f);
            return *this;
        }
#endif

        void swap(AZSTD_FUNCTION_FUNCTION& other)
        {
            if (&other == this)
            {
                return;
            }

            AZSTD_FUNCTION_FUNCTION tmp;
            tmp.move_assign(*this);
            this->move_assign(other);
            other.move_assign(tmp);
        }

        // Clear out a target, if there is one
        void clear()
        {
            if (vtable)
            {
                reinterpret_cast<vtable_type*>(vtable)->clear(this->functor);
                vtable = 0;
            }
        }

    private:
        struct dummy
        {
            void nonnull() {};
        };
        typedef void (dummy::* safe_bool)();

        using function_base::empty; // hide non-standard empty() function

    public:
        operator safe_bool () const
        {
            return (this->empty()) ? 0 : &dummy::nonnull;
        }

        bool operator!() const
        { return this->empty(); }

    private:
        void assign_to_own(const AZSTD_FUNCTION_FUNCTION& f)
        {
            if (!f.empty())
            {
                this->vtable = f.vtable;
                f.vtable->manager(f.functor, this->functor, AZStd::Internal::function_util::clone_functor_tag);
            }
        }

        template<typename Functor>
        void assign_to(Functor f)
        {
            using Internal::function_util::vtable_base;

            typedef typename Internal::function_util::get_function_tag<Functor>::type tag;
            typedef Internal::function_util::AZSTD_FUNCTION_GET_INVOKER<tag> get_invoker;
            typedef typename get_invoker::template apply<Functor, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS> handler_type;

            typedef typename handler_type::invoker_type invoker_type;
            typedef typename handler_type::manager_type manager_type;

            // Note: it is extremely important that this initialization use
            // static initialization. Otherwise, we will have a race
            // condition here in multi-threaded code. See
            // http://thread.gmane.org/gmane.comp.lib.boost.devel/164902/.
            static vtable_type stored_vtable = { { &manager_type::manage }, &invoker_type::invoke };

            if (stored_vtable.assign_to(f, functor))
            {
                vtable = &stored_vtable.base;
            }
            else
            {
                vtable = 0;
            }
        }

        template<typename Functor, typename Allocator>
        void assign_to_a(Functor f, const Allocator& a)
        {
            using Internal::function_util::vtable_base;
            typedef typename Internal::function_util::get_function_tag<Functor>::type tag;
            typedef Internal::function_util::AZSTD_FUNCTION_GET_INVOKER<tag> get_invoker;
            typedef typename get_invoker::template apply_a<Functor, R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS, Allocator > handler_type;

            typedef typename handler_type::invoker_type invoker_type;
            typedef typename handler_type::manager_type manager_type;

            // Note: it is extremely important that this initialization use
            // static initialization. Otherwise, we will have a race
            // condition here in multi-threaded code. See
            // http://thread.gmane.org/gmane.comp.lib.boost.devel/164902/.
            static vtable_type stored_vtable = { { &manager_type::manage }, &invoker_type::invoke };

            if (stored_vtable.assign_to_a(f, functor, a))
            {
                vtable = &stored_vtable.base;
            }
            else
            {
                vtable = 0;
            }
        }


        // Moves the value from the specified argument to *this. If the argument
        // has its function object allocated on the heap, move_assign will pass
        // its buffer to *this, and set the argument's buffer pointer to NULL.
        void move_assign(AZSTD_FUNCTION_FUNCTION& f)
        {
            if (&f == this)
            {
                return;
            }
            if (!f.empty())
            {
                this->vtable = f.vtable;
                f.vtable->manager(f.functor, this->functor, Internal::function_util::move_functor_tag);
                f.vtable = 0;
            }
        }
    };

    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    inline void swap(AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >& f1, AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS >& f2)
    {
        f1.swap(f2);
    }

    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    typename AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>::result_type
    AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>
        ::operator()(AZSTD_FUNCTION_PARMS) const
    {
        AZ_Assert(!this->empty(), "Bad function call!");
        return reinterpret_cast<const vtable_type*>(vtable)->invoker(this->functor AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_ARGS);
    }

    // Poison comparisons between boost::function objects of the same type.
    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    void operator==(const AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>&,
        const AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>&);
    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    void operator!=(const AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>&,
        const AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>&);

#if !defined(AZSTD_FUNCTION_NO_FUNCTION_TYPE_SYNTAX)

#if     AZSTD_FUNCTION_NUM_ARGS == 0
    #define AZSTD_FUNCTION_PARTIAL_SPEC R (void)
#else
    #define AZSTD_FUNCTION_PARTIAL_SPEC R (AZSTD_FUNCTION_TEMPLATE_ARGS)
#endif

    template<typename R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_PARMS>
    class function<AZSTD_FUNCTION_PARTIAL_SPEC>
        : public AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS>
    {
        typedef AZSTD_FUNCTION_FUNCTION<R AZSTD_FUNCTION_COMMA AZSTD_FUNCTION_TEMPLATE_ARGS> base_type;
        typedef function self_type;
    public:
        function()
            : base_type() {}
        template<typename Functor>
        function(Functor f, typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), int>::type = 0)
            : base_type(f)
        {}
        template<typename Functor, typename Allocator>
        function(Functor f, const Allocator& a, typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), int>::type = 0)
            : base_type(f, a)
        {}

        function(nullptr_t)
            : base_type() {}
        function(const self_type& f)
            : base_type(static_cast<const base_type&>(f)){}
        function(const base_type& f)
            : base_type(static_cast<const base_type&>(f)){}
#if defined(AZ_HAS_RVALUE_REFS)
        function(self_type&& f)
            : base_type(static_cast<base_type &&>(f)){}
        function(base_type&& f)
            : base_type(static_cast<base_type &&>(f)){}
#endif // AZ_HAS_RVALUE_REFS

        self_type& operator=(const self_type& f)
        {
            self_type(f).swap(*this);
            return *this;
        }
#if defined(AZ_HAS_RVALUE_REFS)
        self_type& operator=(self_type&& f)
        {
            self_type(static_cast<self_type &&>(f)).swap(*this);
            return *this;
        }
#endif // AZ_HAS_RVALUE_REFS

        template<typename Functor>
        typename Utils::enable_if_c<(AZStd::type_traits::ice_not<(is_integral<Functor>::value)>::value), self_type&>::type
        operator=(Functor f)
        {
            self_type(f).swap(*this);
            return *this;
        }

        self_type& operator=(nullptr_t)
        {
            this->clear();
            return *this;
        }
        self_type& operator=(const base_type& f)
        {
            self_type(f).swap(*this);
            return *this;
        }
#if defined(AZ_HAS_RVALUE_REFS)
        self_type& operator=(base_type&& f)
        {
            self_type(static_cast<base_type &&>(f)).swap(*this);
            return *this;
        }
#endif // AZ_HAS_RVALUE_REFS
    };

#undef AZSTD_FUNCTION_PARTIAL_SPEC
#endif // have partial specialization
} // end namespace AZStd

// Cleanup after ourselves...
#undef AZSTD_FUNCTION_VTABLE
#undef AZSTD_FUNCTION_COMMA
#undef AZSTD_FUNCTION_FUNCTION
#undef AZSTD_FUNCTION_FUNCTION_INVOKER
#undef AZSTD_FUNCTION_VOID_FUNCTION_INVOKER
#undef AZSTD_FUNCTION_FUNCTION_OBJ_INVOKER
#undef AZSTD_FUNCTION_VOID_FUNCTION_OBJ_INVOKER
#undef AZSTD_FUNCTION_FUNCTION_REF_INVOKER
#undef AZSTD_FUNCTION_VOID_FUNCTION_REF_INVOKER
#undef AZSTD_FUNCTION_MEMBER_INVOKER
#undef AZSTD_FUNCTION_VOID_MEMBER_INVOKER
#undef AZSTD_FUNCTION_GET_FUNCTION_INVOKER
#undef AZSTD_FUNCTION_GET_FUNCTION_OBJ_INVOKER
#undef AZSTD_FUNCTION_GET_FUNCTION_REF_INVOKER
#undef AZSTD_FUNCTION_GET_MEM_FUNCTION_INVOKER
#undef AZSTD_FUNCTION_GET_INVOKER
#undef AZSTD_FUNCTION_TEMPLATE_PARMS
#undef AZSTD_FUNCTION_TEMPLATE_ARGS
#undef AZSTD_FUNCTION_PARMS
#undef AZSTD_FUNCTION_PARM
#undef AZSTD_FUNCTION_ARGS
#undef AZSTD_FUNCTION_ARG_TYPE
#undef AZSTD_FUNCTION_ARG_TYPES
#undef AZSTD_FUNCTION_VOID_RETURN_TYPE
#undef AZSTD_FUNCTION_RETURN

#if defined(AZ_COMPILER_MSVC)
#   pragma warning( pop )
#endif
