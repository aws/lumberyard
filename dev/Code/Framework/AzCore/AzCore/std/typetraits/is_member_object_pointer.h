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

#ifndef AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED

#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/is_member_function_pointer.h>

namespace AZStd
{
    template<class T>
    struct is_member_object_pointer : AZStd::integral_constant<bool, AZStd::is_member_pointer<T>::value && !AZStd::is_member_function_pointer<T>::value> {};
}

#endif // AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED
#pragma once