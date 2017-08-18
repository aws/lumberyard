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
//
//  defines object traits classes:
//  is_object, is_scalar, is_class, is_compound, is_pod,
//  has_trivial_constructor, has_trivial_copy, has_trivial_assign,
//  has_trivial_destructor, is_empty.
//

#ifndef AZSTD_TYPE_TRAITS_OBJECT_TRAITS_INLCUDED
#define AZSTD_TYPE_TRAITS_OBJECT_TRAITS_INLCUDED

#include <AzCore/std/typetraits/has_trivial_assign.h>
#include <AzCore/std/typetraits/has_trivial_constructor.h>
#include <AzCore/std/typetraits/has_trivial_copy.h>
#include <AzCore/std/typetraits/has_trivial_destructor.h>
#include <AzCore/std/typetraits/has_nothrow_constructor.h>
#include <AzCore/std/typetraits/has_nothrow_copy.h>
#include <AzCore/std/typetraits/has_nothrow_assign.h>
#include <AzCore/std/typetraits/is_base_and_derived.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_compound.h>
#include <AzCore/std/typetraits/is_empty.h>
#include <AzCore/std/typetraits/is_object.h>
#include <AzCore/std/typetraits/is_pod.h>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/typetraits/is_stateless.h>

#endif // AZSTD_TYPE_TRAITS_OBJECT_TRAITS_INLCUDED
#pragma once