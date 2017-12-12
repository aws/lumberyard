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

#ifndef _CRE_BREAKABLE_GLASS_CONFIG_
#define _CRE_BREAKABLE_GLASS_CONFIG_
#pragma once

//==================================================================================================
// Name: CREBreakableGlassConfig
// Desc: Breakable glass defines and platform-specific configuration
// Author: Chris Bunner
//==================================================================================================

// Glass material
#define GLASSCFG_PLANE_TOUGHNESS                                1.0f                                                                        /* Direct scale for impulse needed to cause a break */
#define GLASSCFG_PLANE_SHATTER_IMPULSE                  (8000.0f * GLASSCFG_PLANE_TOUGHNESS)        /* Impulse threshold that will instantly shatter plane */
#define GLASSCFG_PLANE_SPLIT_IMPULSE                        (280.0f * GLASSCFG_PLANE_TOUGHNESS)         /* Min impulse to cause a physical hole in the mesh */
#define GLASSCFG_PLANE_SPLIT_IMPULSE_STRENGTH       (6.5f / GLASSCFG_PLANE_SHATTER_IMPULSE) /* Direct scale for size of physical mesh holes */
#define GLASSCFG_PLANE_AVG_SPEED_TO_BREAK               6.5f                                                                        /* Any object with velocity above this will likely cause a break */

// Hash grid
#define GLASSCFG_HASH_GRID_SIZE                     8               /* Optimizations rely on power of two, so check usage if changing */
#define GLASSCFG_HASH_GRID_BUCKET_SIZE      16          /* Should be max number of overlapping triangles in any given cell */
#define GLASSCFG_USE_HASH_GRID                      0               /* Determines if the glass system should use hash grids */

// Common
#define GLASSCFG_MIN_BULLET_SPEED                   400.0f  /* Anything above this speed will be considered a bullet (Generally 450 to 1100)*/
#define GLASSCFG_MIN_BULLET_HOLE_SCALE      1.1f        /* Minimum size for physical hole created by bullets */
#define GLASSCFG_MIN_STABLE_FRAG_AREA           0.0095f /* Any fragment sized below this threshold will be forced loose */
#define GLASSCFG_MIN_VALID_FRAG_AREA            0.0035f /* Any fragment sized below this threshold will not be created */

// Drawing
#define GLASSCFG_GLASS_PLANE_FLAG_LOD           255         /* Fixed number used to flag if render call is plane or fragment */

#define GLASSCFG_HIGH_QUALITY_MODE

#define GLASSCFG_NUM_RADIAL_CRACKS              7
#define GLASSCFG_MAX_NUM_CRACK_POINTS           128

#define GLASSCFG_SIMPLIFY_CRACKS                    4.5f
#define GLASSCFG_SIMPLIFY_AREA                      1.0f
#define GLASSCFG_SHATTER_ON_EXPLOSION           0
#define GLASSCFG_SPLIT_ON_EXPLOSION             0

#define GLASSCFG_MAX_NUM_IMPACTS                    12
#define GLASSCFG_MAX_NUM_IMPACT_DECALS      8
#define GLASSCFG_MAX_NUM_STABLE_FRAGMENTS   45
#define GLASSCFG_MAX_NUM_FRAGMENTS              80
#define GLASSCFG_FRAGMENT_ARRAY_SIZE            32

#define GLASSCFG_MAX_NUM_ACTIVE_GLASS           20
#define GLASSCFG_MAX_NUM_PHYS_FRAGMENTS     10
#define GLASSCFG_MAX_NUM_PLANE_VERTS            1024

// Geometry buffer sizes
#define GLASSCFG_MAX_NUM_CRACK_VERTS            (GLASSCFG_MAX_NUM_PLANE_VERTS * 2)
#define GLASSCFG_MAX_NUM_PLANE_INDS             (GLASSCFG_MAX_NUM_PLANE_VERTS * 3)
#define GLASSCFG_MAX_NUM_CRACK_INDS             (GLASSCFG_MAX_NUM_CRACK_VERTS * 6)

#endif // _CRE_BREAKABLE_GLASS_CONFIG_