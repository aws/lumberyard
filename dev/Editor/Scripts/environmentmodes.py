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
	if mode == 'e_GlobalFog':
		toggleCvarsValue('mode_%s' % mode, 'e_Fog', 1, 0)	
	elif mode == 'e_Clouds':
		toggleCvarsValue('mode_%s' % mode, 'e_Clouds', 1, 0)
	elif mode == 'r_Rain':
		toggleCvarsValue('mode_%s' % mode, 'r_Rain', 1, 0)	
	elif mode == 'e_Sun':
		toggleCvarsValue('mode_%s' % mode, 'e_Sun', 1, 0)
	elif mode == 'e_Skybox':
		toggleCvarsValue('mode_%s' % mode, 'e_Skybox', 1, 0)
	elif mode == 'e_TimeOfDay':
		cycleCvarsFloatValue("e_TimeOfDay", [0.0, 1.5, 3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12.0, 13.5, 15.0, 16.5, 18.0, 19.5, 21.0, 22.5])
	elif mode == 'r_SSReflections':
		toggleCvarsValue('mode_%s' % mode, 'r_SSReflections', 1, 0)
	elif mode == 'e_Shadows':
		toggleCvarsValue('mode_%s' % mode, 'e_Shadows', 1, 0)
	elif mode == 'r_TransparentPasses':
		toggleCvarsValue('mode_%s' % mode, 'r_TransparentPasses', 1, 0)
	elif mode == 'e_GI':
		toggleCvarsValue('mode_%s' % mode, 'e_GI', 1, 0)
	elif mode == 'r_ssdo':
		toggleCvarsValue('mode_%s' % mode, 'r_ssdo', 1, 0)	
	elif mode == 'e_DynamicLights':
		toggleCvarsValue('mode_%s' % mode, 'e_DynamicLights', 1, 0)
	elif mode == 'e_Brushes':
		toggleCvarsValue('mode_%s' % mode, 'e_Brushes', 1, 0)
	elif mode == 'designerObjects':
		toggleHideMaskValues('solids')
	elif mode == 'e_Entities':
		toggleCvarsValue('mode_%s' % mode, 'e_Entities', 1, 0)
	elif mode == 'PrefabObjects':
		toggleHideMaskValues('prefabs')
	elif mode == 'e_Vegetation':
		toggleCvarsValue('mode_%s' % mode, 'e_Vegetation', 1, 0)
	elif mode == 'e_Decals':
		toggleCvarsValue('mode_%s' % mode, 'e_Decals', 1, 0)
	elif mode == 'e_Terrain':
		toggleCvarsValue('mode_%s' % mode, 'e_Terrain', 1, 0)
	elif mode == 'e_Particles':
		toggleCvarsValue('mode_%s' % mode, 'e_Particles', 1, 0)
	elif mode == 'r_Flares':
		toggleCvarsValue('mode_%s' % mode, 'r_Flares', 1, 0)
	elif mode == 'r_Beams':
		toggleCvarsValue('mode_%s' % mode, 'r_Beams', 1, 0)
	elif mode == 'e_FogVolumes':
		toggleCvarsValue('mode_%s' % mode, 'e_FogVolumes', 1, 0)
	elif mode == 'e_WaterOcean':
		toggleCvarsValue('mode_%s' % mode, 'e_WaterOcean', 1, 0)	
	elif mode == 'e_WaterVolumes':
		toggleCvarsValue('mode_%s' % mode, 'e_WaterVolumes', 1, 0)
	elif mode == 'e_Wind':
		toggleCvarsValue('mode_%s' % mode, 'e_Wind', 1, 0)
	elif mode == 'e_BBoxes':
		toggleCvarsValue('mode_%s' % mode, 'e_BBoxes', 0, 1)
	elif mode == 'default_view':
		for cVars in ['e_Fog', 'e_Clouds', 'r_Rain', 'e_Sun', 'e_Skybox',
					  'e_TimeOfDay', 'r_SSReflections', 'e_Shadows',
					  'r_TransparentPasses',  'e_GI', 'r_ssdo', 'e_DynamicLights',
					  'e_Brushes', 'e_Entities', 'e_Vegetation', 'e_Decals',
					  'e_Terrain', 'e_Particles', 'r_Flares', 'r_Beams', 'e_FogVolumes',
					  'e_WaterOcean', 'e_WaterVolumes', 'e_Wind', 'e_BBoxes',
					  'hiden_mask_for_solids', 'hiden_mask_for_prefabs']:
			restoreDefaultValue(cVars)
