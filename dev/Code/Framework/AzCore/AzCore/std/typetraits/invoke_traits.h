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

#pragma once

#include <AzCore/std/utils.h>

#include <AzCore/std/typetraits/is_member_function_pointer.h>
#include <AzCore/std/typetraits/is_member_object_pointer.h>
#include <AzCore/std/typetraits/void_t.h>

namespace AZStd
{
    namespace Internal
    {
        // Represents non-usable type which is not equal to any other type
        struct nat final
        {
            nat() = delete;
            ~nat() = delete;
            nat(const nat&) = delete;
            nat& operator=(const nat&) = delete;
        };

        // Represents a type that can accept any argument
        struct param_any final
        {
            param_any(...);
        };

        template <class... T> struct check_complete_type;

        template <>
        struct check_complete_type<>
        {
        };

        template <class T0, class T1, class... Ts>
        struct check_complete_type<T0, T1, Ts...>
            : private check_complete_type<T0>
            , private check_complete_type<T1, Ts...>
        {
        };

        template <class T>
        struct check_complete_type<T>
        {
            AZ_STATIC_ASSERT((sizeof(T) > 0), "Type must be complete.");
        };

        template <class T>
        struct check_complete_type<T&> : private check_complete_type<T>{};

        template <class T>
        struct check_complete_type<T&&> : private check_complete_type<T> {};

        template <class R, class... Args>
        struct check_complete_type<R(*)(Args...)> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(*)(Args...)> {};

        template <class R, class... Args>
        struct check_complete_type<R(Args...)> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(Args...)> {};

        template <class R, class ClassType>
        struct check_complete_type<R ClassType::*> : private check_complete_type<ClassType> {};

        // VS 2013 does not deduce a member function pointer type to the <R ClassType::*> as above
        // So explicit specializations are added for it.
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        template <class R, class ClassType, class... Args>
        struct check_complete_type<R(ClassType::*)(Args...)> : private check_complete_type<ClassType> {};

        template <class R, class ClassType, class... Args>
        struct check_complete_type<R(ClassType::*)(Args...) const> : private check_complete_type<ClassType> {};

        template <class R, class ClassType, class... Args>
        struct check_complete_type<R(ClassType::*)(Args...) volatile> : private check_complete_type<ClassType> {};

        template <class R, class ClassType, class... Args>
        struct check_complete_type<R(ClassType::*)(Args...) const volatile> : private check_complete_type<ClassType> {};
#endif


        template<class T>
        struct member_pointer_class_type {};

        template<class R, class ClassType>
        struct member_pointer_class_type<R ClassType::*>
        {
            using type = ClassType;
        };

        // VS 2013 does not deduce a member function pointer type to the <R ClassType::*> as above
        // So explicit specializations are added for it.
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        template<class R, class ClassType, class... Args>
        struct member_pointer_class_type<R (ClassType::*)(Args...)>
        {
            using type = ClassType;
        };
        template<class R, class ClassType, class... Args>
        struct member_pointer_class_type<R(ClassType::*)(Args...) const>
        {
            using type = ClassType;
        };
        template<class R, class ClassType, class... Args>
        struct member_pointer_class_type<R(ClassType::*)(Args...) volatile>
        {
            using type = ClassType;
        };
        template<class R, class ClassType, class... Args>
        struct member_pointer_class_type<R(ClassType::*)(Args...) const volatile>
        {
            using type = ClassType;
        };
#endif

        template<class T>
        using member_pointer_class_type_t = typename member_pointer_class_type<T>::type;

        // 23.14.3 [func.require]
        // fallback INVOKE declaration
        // Allows it to be used in unevaluated context such as is_invocable when none of the 7 bullet points apply
        template<class... Args>
        auto INVOKE(param_any, Args&&...) -> nat;

        // Below are the 7 implementations of INVOKE bullet point
        // 1.1
        // Have to put the decltype as part of the template arguments instead of the return to workaround VS2013 issue
        // where it does not properly ignore SFINAE errors in a decltype expression
        template<class Fn, class Arg0, class... Args, typename = AZStd::enable_if_t<
            AZStd::is_member_function_pointer<AZStd::decay_t<Fn>>::value
            && AZStd::is_base_of<member_pointer_class_type_t<AZStd::decay_t<Fn>>, AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype((AZStd::declval<Arg0>().*AZStd::declval<Fn>())(AZStd::declval<Args>()...))>
            auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> RetType
        {
            return (AZStd::forward<Arg0>(arg0).*f)(AZStd::forward<Args>(args)...);
        }
        // 1.2
        // VS2013 workaround, need to add dummy typename template arguments to have a different number of typed template arguments as visual studio believes that each of these functions
        // are the same template
        template<class Fn, class Arg0, class... Args, typename = AZStd::enable_if_t<
                AZStd::is_member_function_pointer<AZStd::decay_t<Fn>>::value
                && AZStd::is_reference_wrapper<AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype((AZStd::declval<Arg0>().get().*AZStd::declval<Fn>())(AZStd::declval<Args>()...)),
            typename = void>
            auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> RetType
        {
            return (arg0.get().*f)(AZStd::forward<Args>(args)...);
        }
        // 1.3
        template<class Fn, class Arg0, class... Args, typename = AZStd::enable_if_t<
                AZStd::is_member_function_pointer<AZStd::decay_t<Fn>>::value
                && !AZStd::is_base_of<member_pointer_class_type_t<AZStd::decay_t<Fn>>, AZStd::decay_t<Arg0>>::value
                && !AZStd::is_reference_wrapper<AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype(((*AZStd::declval<Arg0>()).*AZStd::declval<Fn>())(AZStd::declval<Args>()...)),
            typename = void,
            typename = void>
            auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> RetType
        {
            return ((*AZStd::forward<Arg0>(arg0)).*f)(AZStd::forward<Args>(args)...);
        }
        // 1.4
        template<class Fn, class Arg0, typename = AZStd::enable_if_t<
                AZStd::is_member_object_pointer<AZStd::decay_t<Fn>>::value
                && AZStd::is_base_of<member_pointer_class_type_t<AZStd::decay_t<Fn>>, AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype(AZStd::declval<Arg0>().*AZStd::declval<Fn>())>
            auto INVOKE(Fn&& f, Arg0&& arg0) -> RetType
        {
            return AZStd::forward<Arg0>(arg0).*f;
        }
        // 1.5
        template<class Fn, class Arg0, typename = AZStd::enable_if_t<
                AZStd::is_member_object_pointer<AZStd::decay_t<Fn>>::value
                && AZStd::is_reference_wrapper<AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype(AZStd::declval<Arg0>().get().*AZStd::declval<Fn>()),
            typename = void>
            auto INVOKE(Fn&& f, Arg0&& arg0) -> RetType
        {
            return arg0.get().*f;
        }
        // 1.6
        template<class Fn, class Arg0, typename = AZStd::enable_if_t<
                AZStd::is_member_object_pointer<AZStd::decay_t<Fn>>::value
                && !AZStd::is_base_of<member_pointer_class_type_t<AZStd::decay_t<Fn>>, AZStd::decay_t<Arg0>>::value
                && !AZStd::is_reference_wrapper<AZStd::decay_t<Arg0>>::value>,
            typename RetType = decltype((*AZStd::declval<Arg0>()).*AZStd::declval<Fn>()),
            typename = void,
            typename = void>
            auto INVOKE(Fn&& f, Arg0&& arg0) -> RetType
        {
            return (*AZStd::forward<Arg0>(arg0)).*f;
        }
        // 1.7
        template<class Fn, class... Args, typename RetType = decltype(AZStd::declval<Fn>()(AZStd::declval<Args>()...))>
        auto INVOKE(Fn&& f, Args&&... args) -> RetType
        {
            return AZStd::forward<Fn>(f)(AZStd::forward<Args>(args)...);
        }

        template<class R, class Fn, class... ArgTypes>
        struct invocable_r : private check_complete_type<Fn>
        {  
            using result_type = decltype(INVOKE(AZStd::declval<Fn>(), AZStd::declval<ArgTypes>()...));
            using type = AZStd::conditional_t<!AZStd::is_same<result_type, nat>::value,
                AZStd::conditional_t<!AZStd::is_void<R>::value, AZStd::is_convertible<result_type, R>, AZStd::true_type>, 
                AZStd::false_type>;
            static const bool value = type::value;
        };

        template<class Fn, class... ArgTypes>
        using invocable = invocable_r<void, Fn, ArgTypes...>;
    }

    template <class Fn, class... ArgTypes>
    struct is_invocable : Internal::invocable<Fn, ArgTypes...>::type {};

    template <class R, class Fn, class... ArgTypes>
    struct is_invocable_r : Internal::invocable_r<R, Fn, ArgTypes...>::type {};

    // Uncomment when VS2013 support is dropped. VS2013 does not have support for Variable Templates
    // Variable Templates are specified C++ n4687 draft: Section 17 Bullet 3
    //template <class Fn, class... ArgTypes>
    //constexpr bool is_invocable_v = is_invocable<_n, ArgTypes...>::value;

    //template <class R, class Fn, class ...ArgTypes>
    //constexpr bool is_invocable_r_v = is_invocable_r<R, Fn, ArgTypes...>::value;

    template<class Fn, class... ArgTypes>
    struct invoke_result
    {
        using type = AZStd::enable_if_t<Internal::invocable<Fn, ArgTypes...>::value, typename Internal::invocable<Fn, ArgTypes...>::result_type>;
    };

    template<class Fn, class... ArgTypes>
    using invoke_result_t = typename invoke_result<Fn, ArgTypes...>::type;
}
