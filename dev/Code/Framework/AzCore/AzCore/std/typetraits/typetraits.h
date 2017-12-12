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
#ifndef AZSTD_TYPE_TRAITS_H
#define AZSTD_TYPE_TRAITS_H 1

#include <AzCore/std/typetraits/config.h>

/**
 * \page TypeTraits Type Traits
 *
 * One of our goals is to be compatible with \ref CTR1 (which by the time you read it it may be in the standard).
 * At the moment it is not and it's supported by GCC > 4.3, MSVC > 9, MWCW > 9. We need older support at the
 * moment too.
 * \ref CTR1 is based on a boost type_traits, we do use modified version of them for the non \ref CTR1 compilers. Of course
 * some functionality will not be supported without \ref CTR1 compiler:
 *
 * The following templates require compiler support for a fully conforming implementation:
 * \li is_class
 * \li is_union
 * \li is_enum
 * \li is_polymorphic
 * \li is_empty
 * \li has_trivial_constructor
 * \li has_trivial_copy
 * \li has_trivial_assign
 * \li has_trivial_destructor
 * \li has_nothrow_constructor
 * \li has_nothrow_copy
 * \li has_nothrow_assign
 * \li is_pod
 * \li is_abstract
 *
 * For non \ref CTR1 compilers you can use \ref TypeTraitsDefines to provide this information about your classes and structures. Otherwise they will be handled in
 * the safest (which is the slowest) way.
 *
 * \todo add move semantics, this will be especially for smart pointers we use trough the engine. For instance a ref counted can be made relocatable
 * this when when for instance we have a vector of them, and we need to realloc the vector we can just memcpy them vs creating ref count storm. This need to be
 * separate from the current POD because most ref counted base classes are NOT POD, but yet they can be memcpy safely.
 */

/**
 * \defgroup TypeTraitsDefines Type Traits Defines
 * @{
 *
 * \def AZSTD_DECLARE_POD_TYPE
 * This macro and it's version for 1,2,3 template parameters are very useful when you use POD user structures in
 * AZStd containers. For \ref CTR1 compilers this macro does nothing, otherwise it indicated that the provided class/struct is POD.
 * This might lead to significant performance increase when you manipulate many of them at a time, because no constructors or destructors will be called.
 * They do have trivial assign, which for instance when we use AZStd::vector enables AZStd move and copy to use memcpy on the entire data block, instead of
 * copying it element by element.
 *
 * \def AZSTD_DECLARE_UNION
 *
 */
#include <AzCore/std/typetraits/add_const.h>
#include <AzCore/std/typetraits/add_cv.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/add_reference.h>
#include <AzCore/std/typetraits/add_volatile.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/has_nothrow_assign.h>
#include <AzCore/std/typetraits/has_nothrow_constructor.h>
#include <AzCore/std/typetraits/has_nothrow_copy.h>
#include <AzCore/std/typetraits/has_nothrow_destructor.h>
#include <AzCore/std/typetraits/has_trivial_assign.h>
#include <AzCore/std/typetraits/has_trivial_constructor.h>
#include <AzCore/std/typetraits/has_trivial_copy.h>
#include <AzCore/std/typetraits/has_trivial_destructor.h>
#include <AzCore/std/typetraits/has_virtual_destructor.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/is_arithmetic.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_base_and_derived.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_compound.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_empty.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_floating_point.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_fundamental.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_member_function_pointer.h>
#include <AzCore/std/typetraits/is_member_object_pointer.h>
#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/is_object.h>
#include <AzCore/std/typetraits/is_pod.h>
#include <AzCore/std/typetraits/is_polymorphic.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/typetraits/is_stateless.h>
#include <AzCore/std/typetraits/is_union.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/rank.h>
//#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/remove_bounds.h>
#include <AzCore/std/typetraits/remove_extent.h>
#include <AzCore/std/typetraits/remove_all_extents.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_volatile.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/is_complex.h>
#include <AzCore/std/typetraits/ice.h>
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/std/typetraits/void_t.h>

// To cover TR1 we need to implements the following type traits:
//::AZStd::integral_constant;
//::AZStd::true_type;
//::AZStd::false_type;
//::AZStd::is_integral;
//::AZStd::is_void;
//::AZStd::is_floating_point;
//::AZStd::is_array;
//::AZStd::is_pointer;
//::AZStd::is_reference;
//::AZStd::is_member_object_pointer;
//::AZStd::is_member_function_pointer;
//::AZStd::is_enum;
//::AZStd::is_union;
//::AZStd::is_class;
//::AZStd::is_function;
//::AZStd::is_arithmetic;
//::AZStd::is_fundamental;
//::AZStd::is_object;
//::AZStd::is_scalar;
//::AZStd::is_compound;
//::AZStd::is_member_pointer;
//::AZStd::is_const;
//::AZStd::is_volatile;
//::AZStd::is_pod;
//::AZStd::is_empty;
//::AZStd::is_polymorphic;
//::AZStd::is_abstract;
//::AZStd::has_trivial_constructor;
//::AZStd::has_trivial_copy;
//::AZStd::has_trivial_assign;
//::AZStd::has_trivial_destructor;
//::AZStd::has_nothrow_constructor;
//::AZStd::has_nothrow_copy;
//::AZStd::has_nothrow_assign;
//::AZStd::has_virtual_destructor;
//::AZStd::is_signed;
//::AZStd::is_unsigned;
//::AZStd::alignment_of;
//::AZStd::rank;
//::AZStd::extent;
//::AZStd::is_same;
//::AZStd::is_base_of;
//::AZStd::is_convertible;
//::AZStd::remove_const;
//::AZStd::remove_volatile;
//::AZStd::remove_cv;
//::AZStd::add_const;
//::AZStd::add_volatile;
//::AZStd::add_cv;
//::AZStd::remove_reference;
//::AZStd::remove_extent;
//::AZStd::remove_all_extents;
//::AZStd::remove_pointer;
//::AZStd::add_pointer;
//::AZStd::aligned_storage;
/// @}

#endif // AZSTD_TYPE_TRAITS_H
#pragma once