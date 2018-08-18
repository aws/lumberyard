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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MNM_TYPE_INFO_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MNM_TYPE_INFO_H
#pragma once

#include "TypeInfo_impl.h"

#include "Navigation/MNM/FixedVec3.h"
#include "Navigation/MNM/FixedAABB.h"
#include "Navigation/MNM/FixedVec3.h"

STRUCT_INFO_T2_BEGIN(fixed_t, typename, BaseType, size_t, IntegerBitCount)
VAR_INFO(v)
STRUCT_INFO_T2_END(fixed_t, typename, BaseType, size_t, size_t)

STRUCT_INFO_T2_BEGIN(MNM::FixedAABB, typename, BaseType, size_t, IntegerBitCount)
VAR_INFO(min)
VAR_INFO(max)
STRUCT_INFO_T2_END(MNM::FixedAABB, typename, BaseType, size_t, size_t)

STRUCT_INFO_T2_BEGIN(MNM::FixedVec3, typename, BaseType, size_t, IntegerBitCount)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
STRUCT_INFO_T2_END(MNM::FixedVec3, typename, BaseType, size_t, size_t)
#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MNM_TYPE_INFO_H
