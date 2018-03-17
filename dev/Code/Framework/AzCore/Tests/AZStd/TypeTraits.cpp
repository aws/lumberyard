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

#include "UserTypes.h"

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    /**
     * Tests the AZSTD type traits as much as possible for each target.
     * \cond
     * The following templates require compiler support for a fully conforming implementation:
     * template <class T> struct is_class;
     * template <class T> struct is_union;
     * template <class T> struct is_enum;
     * template <class T> struct is_polymorphic;
     * template <class T> struct is_empty;
     * template <class T> struct has_trivial_constructor;
     * template <class T> struct has_trivial_copy;
     * template <class T> struct has_trivial_assign;
     * template <class T> struct has_trivial_destructor;
     * template <class T> struct has_nothrow_constructor;
     * template <class T> struct has_nothrow_copy;
     * template <class T> struct has_nothrow_assign;
     * template <class T> struct is_pod;
     * template <class T> struct is_abstract;
     * \endcond
     */
    TEST(TypeTraits, All)
    {
        //////////////////////////////////////////////////////////////////////////
        // Primary type categories:

        // alignment_of and align_to
        AZ_TEST_STATIC_ASSERT(alignment_of<int>::value == 4);
        AZ_TEST_STATIC_ASSERT(alignment_of<char>::value == 1);
        AZ_TEST_STATIC_ASSERT(alignment_of<void>::value == 0);

        AZ_TEST_STATIC_ASSERT(alignment_of<MyClass>::value == 16);
        AZ_TEST_STATIC_ASSERT((alignment_of< align_to<int, 32>::type >::value) == 32);
        AZ_TEST_STATIC_ASSERT((sizeof(align_to<MyStruct, 32>)) == 32);
        AZ_TEST_STATIC_ASSERT((alignment_of< align_to<int, 64>::type >::value) == 64);
        aligned_storage<sizeof(int)*100, 16>::type alignedArray;
        AZ_TEST_ASSERT((((AZStd::size_t)&alignedArray) & 15) == 0);

        AZ_TEST_STATIC_ASSERT((alignment_of< aligned_storage<sizeof(int)*5, 8>::type >::value) == 8);
        AZ_TEST_STATIC_ASSERT(sizeof(aligned_storage<sizeof(int), 16>::type) == 16);

        // is_void
        AZ_TEST_STATIC_ASSERT(is_void<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_void<void>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void const volatile>::value == true);

        // is_integral
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned char>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned short>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned long>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed char>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed short>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed long>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<bool>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char>::value == true);
        //AZ_TEST_STATIC_ASSERT(is_integral<wchar_t>::value == true);

        AZ_TEST_STATIC_ASSERT(is_integral<char const >::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long const volatile>::value == true);

        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct const>::value == false);
        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct const volatile>::value == false);

        // is_floating_point
        AZ_TEST_STATIC_ASSERT(is_floating_point<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float const volatile>::value == true);

        // is_array
        AZ_TEST_STATIC_ASSERT(is_array<int*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_array<int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<volatile int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const volatile int[5]>::value == true);

        AZ_TEST_STATIC_ASSERT(is_array<float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<volatile float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const volatile float[]>::value == true);

        // is_pointer
        AZ_TEST_STATIC_ASSERT(is_pointer<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_pointer<int*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<const MyStruct*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<volatile int*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<const volatile MyStruct*>::value == true);

        // is_reference
        AZ_TEST_STATIC_ASSERT(is_reference<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_reference<int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<const MyStruct&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<volatile int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<const volatile MyStruct&>::value == true);

        // is_member_object_pointer
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<int (MyStruct::*)()>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<int MyStruct::*>::value == true);

        // is_member_function_pointer
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<int (MyStruct::*)()>::value == true);
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<int MyStruct::*>::value == false);
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() volatile>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile>::value));
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC >= 1900
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() &>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() &&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const&&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile&&>::value));
#endif

        // is_enum
        AZ_TEST_STATIC_ASSERT(is_enum<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_enum<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_enum<MyEnum>::value == true);

        // is_union
        AZ_TEST_STATIC_ASSERT(is_union<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_union<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_union<MyUnion>::value == true);

        // is_class
        AZ_TEST_STATIC_ASSERT(is_class<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_class<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_class<MyClass>::value == true);

        // is_function
        AZ_TEST_STATIC_ASSERT(is_function<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_function<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_function<int(float, char)>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // composite type categories:

        // is_arithmetic
        AZ_TEST_STATIC_ASSERT(is_arithmetic<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_arithmetic<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_arithmetic<float>::value == true);

        // is_fundamental
        AZ_TEST_STATIC_ASSERT(is_fundamental<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_fundamental<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_fundamental<const float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_fundamental<void>::value == true);

        // is_object
        AZ_TEST_STATIC_ASSERT(is_object<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_object<MyStruct&>::value == false);
        AZ_TEST_STATIC_ASSERT(is_object<int(short, float)>::value == false);
        AZ_TEST_STATIC_ASSERT(is_object<void>::value == false);

        // is_scalar
        AZ_TEST_STATIC_ASSERT(is_scalar<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_scalar<MyStruct*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_scalar<const float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_scalar<int>::value == true);

        // is_compound
        AZ_TEST_STATIC_ASSERT(is_compound<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_compound<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<int(short, float)>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<const void*>::value == true);

        // is_member_pointer
        AZ_TEST_STATIC_ASSERT(is_member_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_pointer<int MyStruct::*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_member_pointer<int (MyStruct::*)()>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // type properties:

        // is_const
        AZ_TEST_STATIC_ASSERT(is_const<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_const<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_const<const MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_const<const float>::value == true);

        // is_volatile
        AZ_TEST_STATIC_ASSERT(is_volatile<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_volatile<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_volatile<volatile MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_volatile<volatile float>::value == true);

        // is_pod
        AZ_TEST_STATIC_ASSERT(is_pod<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pod<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pod<const MyClass>::value == false);
        AZ_TEST_STATIC_ASSERT((is_pod< align_to<int, 32> >::value) == true);    // Check our aligned types.
        AZ_TEST_STATIC_ASSERT((is_pod< align_to<MyStruct, 32> >::value) == true);
        AZ_TEST_STATIC_ASSERT((is_pod< aligned_storage<30, 32>::type >::value) == true);

        // is_empty
        AZ_TEST_STATIC_ASSERT(is_empty<MyStruct>::value == false);
#if !(defined(AZ_COMPILER_GCC) && (AZ_COMPILER_GCC < 3))
        AZ_TEST_STATIC_ASSERT(is_empty<MyEmptyStruct>::value == true);
#endif
        AZ_TEST_STATIC_ASSERT(is_empty<int>::value == false);

        // is_polymorphic
        AZ_TEST_STATIC_ASSERT(is_polymorphic<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_polymorphic<MyClass>::value == true);

        // is_abstract
        AZ_TEST_STATIC_ASSERT(is_abstract<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_abstract<MyInterface>::value == true);

        // has_trivial_constructor
        AZ_TEST_STATIC_ASSERT(has_trivial_constructor<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_constructor<int>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_constructor<MyClass>::value == false);

        // has_trivial_copy
        AZ_TEST_STATIC_ASSERT(has_trivial_copy<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_copy<int>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_copy<MyClass>::value == false);

        // has_trivial_assign
        AZ_TEST_STATIC_ASSERT(has_trivial_assign<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_assign<int>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_assign<MyClass>::value == false);

        // has_trivial_destructor
        AZ_TEST_STATIC_ASSERT(has_trivial_destructor<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_destructor<int>::value == true);
        AZ_TEST_STATIC_ASSERT(has_trivial_destructor<MyClass>::value == false);

        // has_nothrow_constructr

        // has_nothrow_copy

        // has_nothrow_assign

        // is_signed
        AZ_TEST_STATIC_ASSERT(is_signed<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_signed<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_signed<unsigned int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_signed<float>::value == false);

        // is_unsigned
        AZ_TEST_STATIC_ASSERT(is_unsigned<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_unsigned<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_unsigned<unsigned int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_unsigned<float>::value == false);

        // true and false types
        AZ_TEST_STATIC_ASSERT(true_type::value == true);
        AZ_TEST_STATIC_ASSERT(false_type::value == false);

        //! function traits tests
        struct NotMyStruct
        {
            int foo() { return 0; }

        };

        struct FunctionTestStruct
        {
            bool operator()(FunctionTestStruct&) const { return true; };
        };
        
        using PrimitiveFunctionPtr = int(*)(bool, float, double, AZ::u8, AZ::s8, AZ::u16, AZ::s16, AZ::u32, AZ::s32, AZ::u64, AZ::s64);
        using NotMyStructMemberPtr = int(NotMyStruct::*)();
        using ComplexFunctionPtr = float(*)(MyEmptyStruct&, NotMyStructMemberPtr, MyUnion*);
        using ComplexFunction = remove_pointer_t<ComplexFunctionPtr>;
        using MemberFunctionPtr = void(MyInterface::*)(int);
        using ConstMemberFunctionPtr = bool(FunctionTestStruct::*)(FunctionTestStruct&) const;

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::result_type, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::get_arg_t<10>, AZ::s64>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_arg_t<PrimitiveFunctionPtr, 5>, AZ::u16>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<PrimitiveFunctionPtr>::arity == 11));
        
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_result_t<ComplexFunction>, float>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_arg_t<ComplexFunction, 1>, int(NotMyStruct::*)()>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<ComplexFunction>::arity == 3));

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_fp_type, void(MyInterface::*)(int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::raw_fp_type, void(*)(int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_type, MyInterface>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<MemberFunctionPtr>::arity == 1));

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const> ::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::raw_fp_type, bool(*)(FunctionTestStruct&)> ::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_type, FunctionTestStruct>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits_get_arg_t<ConstMemberFunctionPtr, 0>, FunctionTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<ConstMemberFunctionPtr>::arity == 1));
        
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<decltype(&FunctionTestStruct::operator())>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const>::value));

        auto lambdaFunction = [](FunctionTestStruct, int) -> bool
        {
            return false;
        };

        using LambdaType = decltype(lambdaFunction);
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<decltype(lambdaFunction)>::raw_fp_type, bool(*)(FunctionTestStruct, int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<decltype(lambdaFunction)>::class_fp_type, bool(LambdaType::*)(FunctionTestStruct, int) const>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<decltype(lambdaFunction)>::class_type, LambdaType>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<LambdaType>::arity == 2));

        AZStd::function<void(LambdaType*, ComplexFunction&)> stdFunction;
        using StdFunctionType = decay_t<decltype(stdFunction)>;
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<StdFunctionType>::raw_fp_type, void(*)(LambdaType*, ComplexFunction&)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<StdFunctionType>::arity == 2));
    } 

    struct ConstMethodTestStruct
    {
        void ConstMethod() const { }
        void NonConstMethod() { }
    };

    AZ_TEST_STATIC_ASSERT(is_method_t_const<decltype(&ConstMethodTestStruct::ConstMethod)>::value);
    AZ_TEST_STATIC_ASSERT(!is_method_t_const<decltype(&ConstMethodTestStruct::NonConstMethod)>::value);
}
