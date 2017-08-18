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

#ifndef CRYINCLUDE_CRYPHYSICS_GEOMAN_INFO_H
#define CRYINCLUDE_CRYPHYSICS_GEOMAN_INFO_H
#pragma once

#include "geoman.h"

STRUCT_INFO_BEGIN(phys_geometry_serialize)
STRUCT_VAR_INFO(dummy0, TYPE_INFO(int))
STRUCT_VAR_INFO(Ibody, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(q, TYPE_INFO(quaternionf))
STRUCT_VAR_INFO(origin, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(V, TYPE_INFO(float))
STRUCT_VAR_INFO(nRefCount, TYPE_INFO(int))
STRUCT_VAR_INFO(surface_idx, TYPE_INFO(int))
STRUCT_VAR_INFO(dummy1, TYPE_INFO(int))
STRUCT_VAR_INFO(nMats, TYPE_INFO(int))
STRUCT_INFO_END(phys_geometry_serialize)


#endif // CRYINCLUDE_CRYPHYSICS_GEOMAN_INFO_H
