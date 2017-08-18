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

#ifndef CRYINCLUDE_CRYPHYSICS_AABBTREE_INFO_H
#define CRYINCLUDE_CRYPHYSICS_AABBTREE_INFO_H
#pragma once

#include "aabbtree.h"

STRUCT_INFO_BEGIN(AABBnode)
STRUCT_VAR_INFO(ichild, TYPE_INFO(unsigned int))
STRUCT_VAR_INFO(minx, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(maxx, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(miny, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(maxy, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(minz, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(maxz, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(ntris, TYPE_INFO(unsigned char))
STRUCT_VAR_INFO(bSingleColl, TYPE_INFO(unsigned char))
STRUCT_INFO_END(AABBnode)

STRUCT_INFO_BEGIN(AABBnodeV0)
STRUCT_BITFIELD_INFO(minx, unsigned int, 7)
STRUCT_BITFIELD_INFO(maxx, unsigned int, 7)
STRUCT_BITFIELD_INFO(miny, unsigned int, 7)
STRUCT_BITFIELD_INFO(maxy, unsigned int, 7)
STRUCT_BITFIELD_INFO(minz, unsigned int, 7)
STRUCT_BITFIELD_INFO(maxz, unsigned int, 7)
STRUCT_BITFIELD_INFO(ichild, unsigned int, 15)
STRUCT_BITFIELD_INFO(ntris, unsigned int, 6)
STRUCT_BITFIELD_INFO(bSingleColl, unsigned int, 1)
STRUCT_INFO_END(AABBnodeV0)


#endif // CRYINCLUDE_CRYPHYSICS_AABBTREE_INFO_H
