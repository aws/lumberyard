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

#ifndef CRYINCLUDE_CRYCOMMON_COMMON_TYPEINFO2_H
#define CRYINCLUDE_CRYCOMMON_COMMON_TYPEINFO2_H
#pragma once

#include "Cry_Vector3.h"

// IvoH:
// I'd like to have these structs in CommonTypeInfo.h
// But there seems to be no way without getting a linker error

STRUCT_INFO_T_BEGIN(Vec3_tpl, typename, F)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
STRUCT_INFO_T_END(Vec3_tpl, typename, F)

STRUCT_INFO_T_BEGIN(Vec4_tpl, typename, F)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
VAR_INFO(w)
STRUCT_INFO_T_END(Vec4_tpl, typename, F)

#endif // CRYINCLUDE_CRYCOMMON_COMMON_TYPEINFO2_H
