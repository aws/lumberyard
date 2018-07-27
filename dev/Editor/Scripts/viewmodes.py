#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import sys
import itertools
from tools_shelf_actions import *


if len(sys.argv) > 1:
	mode = sys.argv[1]
	if mode == 'Fullshading':
		updateCvars('r_DebugGBuffer', 0)
	elif mode == 'Normals':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 1, 0)
	elif mode == 'Smoothness':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 2, 0)
	elif mode == 'Reflectance':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 3, 0)
	elif mode == 'Albedo':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 4, 0)
	elif mode == 'Lighting_Model':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 5, 0)
	elif mode == 'Translucency':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 6, 0)
	elif mode == 'Sun_self_shadowing':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 7, 0)
	elif mode == 'Subsurface_scattering':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 8, 0)
	elif mode == 'Specular_validation_overlay':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 9, 0)
	elif mode == 'default_material':
		toggleCvarsValue('mode_%s' % mode, 'e_defaultmaterial', 1, 0)
	elif mode == 'default_material_normals':
		toggleCvarsValue('mode_%s' % mode, 'r_TexBindMode', 6, 0)
	elif mode == 'collisionshapes':
		toggleCvarsValue('mode_%s' % mode, 'p_draw_helpers', '1', '0')
	elif mode == 'shaded_wireframe':
		toggleCvarsValue('mode_%s' % mode, 'r_showlines', 2, 0)
	elif mode == 'wireframe':
		toggleCvarsValue('mode_%s' % mode, 'r_wireframe', 1, 0)
	elif mode == 'Vnormals':	
		toggleCvarsValue('mode_%s' % mode, 'r_shownormals', 1, 0)
	elif mode == 'Tangents':
		toggleCvarsValue('mode_%s' % mode, 'r_ShowTangents', 1, 0)	
	elif mode == 'texelspermeter360':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(256), float(0))
	elif mode == 'texelspermeterpc':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(512), float(0))
	elif mode == 'texelspermeterpc2':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(1024), float(0))
	elif mode == 'lods':
		cycleCvarsIntValue("e_DebugDraw", [0, 3, -3])
	elif mode == 'lods_level':
		cycleCvarsIntValue("e_LodMin", [0, 1, 2, 3, 4, 5])
	elif mode == 'default_view':
		for cVars in ['r_DebugGBuffer', 'e_defaultmaterial', 'r_TexBindMode',
					  'p_draw_helpers', 'r_showlines', 'r_wireframe', 'r_shownormals',
					  'r_ShowTangents', 'r_TexelsPerMeter', 'e_DebugDraw', 'e_LodMin']:
			restoreDefaultValue(cVars)
