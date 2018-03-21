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
'''
@argumentname="debug_views",type="string"
'''
import sys

# GLOBALS - Need to avoid clobbering the global vars every execution
DEBUG = False
if 'CVARS' not in globals():
	CVARS = {
			'label' : '',
			'current' : {},
			'gbuffers' : [],
			'profiling' : [],
			'ai' : [],
			'art' : [],
			'materialfx' : [],
			'soundfx' : [],
			}

###############################################################################
class cvar():
	'''Crytek Variable Wrapper to allow for easy incrementing of useful cvar values'''
	def __init__(self, name, default, category, description=None):
		self.name = name
		self.default = default
		self.description = description
		self.category = category
		self.values = []
		
		CVARS['current'][self.name] = default
		
		try:
			self.curValue = eval(general.get_cvar(name))
		except:
			self.curValue = general.get_cvar(name)
		if self.category in CVARS:
			if self not in CVARS[self.category]:
				CVARS[self.category].append(self)
				
	def add_value(self, value, description=None):
		if value not in self.values:
			self.values.append((value, description))
			
	def get_value(self):
		try:
			return eval(general.get_cvar(self.name))
		except:
			return general.get_cvar(self.name)
			
	def set_value(self, value, desc):
		print('Displaying {0} = {1} - {2}'.format(self.name, value, desc))
		general.set_cvar(self.name, value)
			
	def next_value(self):
		if not self.values:
			return

		for value, desc in self.values:
			if value == self.curValue:
				curIdx = self.values.index((value, desc))
				if curIdx + 1 == len(self.values):
					reset_cvars()
					return
					
				try:
					valueNext, descNext = self.values[curIdx + 1]
					if DEBUG: print('NextValue: {0} Next Label: {1}'.format(valueNext, descNext))
					self.set_value(valueNext, descNext)
					general.log('Debug Views: Now Showing {0}, {1}'.format(valueNext, descNext))
					return
				except Exception as exc:
					if DEBUG: print('ERROR: Exception Met: {0}'.format(exc))
					reset_cvars()
					return
					
		try:
			valueNext, descNext = self.values[0]
			self.set_value(valueNext, descNext)
			general.log(' ')
			general.log('Debug Views: Starting Debug View Mode: {0}, This Mode Is Driven By The CVar: {1}'.format(self.category, self.name))
			general.log('Debug Views: Now Showing {0} {1}, {2}'.format(self.name, valueNext, descNext))
		except Exception as exc:
			if DEBUG: print('ERROR: Exception Met: {0}'.format(exc))
			reset_cvars()
			
###############################################################################
def set_auxiliary_cvar(cvar, default, value):
	'''Store the cvar and expected default value, and if set the value that it needs to be currently'''
	CVARS['current'][cvar] = default
	general.set_cvar(cvar, value)
		
def init_cvars(type):
	'''Set any cvars that'''
	# Display info is often in the way when displaying debug views, so just disable it always
	set_auxiliary_cvar('r_displayinfo', int(general.get_cvar('r_displayinfo')), 0)
	
	# If the user has switched from one debug view type to a different one before switching,
	# Return the previous debug view type to its default value along with all auxiliary cvars
	reset = False
	
	if int(general.get_cvar('r_debugGBuffer')) and type != 'gbuffers': reset = True
	if int(general.get_cvar('r_stats')) and type != 'profiling': reset = True
	if int(general.get_cvar('ai_DebugDraw')) != -1 and type != 'ai': reset = True
	if int(general.get_cvar('e_debugdraw')) and type != 'art': reset = True
	if int(general.get_cvar('mfx_debug')) and type != 'materialfx': reset = True
	# to be reinstated using ATL
	# if int(general.get_cvar('s_SoundInfo')) and type != 'soundfx': reset = True
	
	if int(general.get_cvar('r_wireframe')):
		try:
			general.set_cvar('r_wireframe', 0)
		except:
			pass
		
	if int(general.get_cvar('r_showlines')):
		try:
			general.set_cvar('r_showlines', 0)
		except:
			pass
		
	if reset:
		reset_cvars()
		
def reset_cvars():
	'''Walk over all modified cvar values and return them to their default expected values'''
	for var in CVARS['current']:		
		if DEBUG: 
			print('Var: {0}, Value: {1}'.format(var, CVARS['current'][var]))	
		
		try:
			general.set_cvar(var, CVARS['current'][var])
		except RuntimeError as exc:
			print('Debug Views: WARNING - Unable to reset the CVAR {0} {1} due to error {2}'.format(var, CVARS['current'][var], exc))
			general.log('Debug Views: WARNING - Unable to reset the CVAR {0} {1} due to error {2}'.format(var, CVARS['current'][var], exc))
	
	CVARS['current'].clear()
	CVARS['commands'] = None
	print('Debug Views: Reset To Default View Mode...')
	general.log('Debug Views: Resetting To Default View Mode...')

###############################################################################	
# Debug Function Formatting
# - Function name is called by toolbar by being passed in as a sys.argv
# - Any cvar that is going to cycle values is init'ed via cvar() with all values added via cvar.add_value(currentValue, screenLabel)
# - Set any necessary auxiliary cvars, often useful when your cycle cvar also needs some other cvar to be turned on to work
# - Once all values are set to the cycling cvar, tell it to go to the first index value via cvar.next_value()
# 
# def foo():
#     	cvarFoo = cvar('foo', 'category', 'label to display')
#		cvarFoo.add_value((int\float\string), label to display value description
#		
#		set_auxiliary_cvar('bar', defaultValue, newValue)
# 		
#		cvarFoo.next_value()
#
def gbuffers():
	'''GBuffer related debug views.'''
	# Init the current cvar and add all desired values.
	rGBuffers = cvar('r_debugGBuffer', 0, 'gbuffers', 'R_DebugGBuffers - Filter all gbuffers down via a given buffer.')
	rGBuffers.add_value(1, 'Normals')
	rGBuffers.add_value(2, 'Gloss')
	rGBuffers.add_value(3, 'Specular')
	rGBuffers.add_value(4, 'Diffuse')
	rGBuffers.add_value(5, 'Translucency')
	rGBuffers.add_value(6, 'Baked AO')
	rGBuffers.add_value(7, 'Subsurface Scattering')
	rGBuffers.add_value(8, 'Specular Validation Overlay')

	# Set default values for the cvars in-case we need to reset_cvars() back to original values
	# Set any other manditory cvars to display buffers. 
	set_auxiliary_cvar('r_AntialiasingMode', int(general.get_cvar('r_AntialiasingMode')), 0)

	# Display the first or next gbuffer, if exception met, or we have reached the final buffers, stop the update function and reset all changed cvars.
	rGBuffers.next_value()
		
def profiling():
	'''Profiling related debug views.'''
	rStats = cvar('r_stats', 0, 'profiling', 'R_STATS - Review CPU \ GPU \ Frame \ Asset Timings.')
	rStats.add_value(1, 'Frame Timings.')
	rStats.add_value(3, 'Object Timings.')
	rStats.add_value(6, 'Per Instance Draw Calls.')
	rStats.add_value(16, 'GPU Sync Timings.')
	
	# Disable GI when profiling as it causes unnecessary jittering in the threads output
	set_auxiliary_cvar('e_gi', int(general.get_cvar('e_gi')), 0)
	
	rStats.next_value()
	
def ai():
	'''Ai related debug views.'''	
	aiDebugDraw = cvar('ai_DebugDraw', -1, 'ai', 'ai_DebugDraw - AI and Nav Mesh Debug Views.')
	aiDebugDraw.add_value(0, 'Minimal AI Info.')
	aiDebugDraw.add_value(1, 'Extended AI Info, Rays and Targets.')
	
	set_auxiliary_cvar('ai_DebugDrawNavigation', 0, 3)
	
	# Supporting either the SDK or our own Pathing Types
	try:
		set_auxiliary_cvar('ai_DrawPath', 'none', 'all')
	except:
		set_auxiliary_cvar('ai_DrawPath', 0, 1)
	
	# AI is a special case where we have to execute a command before we can see the information we want.
	# But there is no guarentee a game will support that agent type, so catch the exception
	try:
		general.execute_command('ai_debugMNMAgentType MediumSizedCharacters') #MediumActor # Saving for reference for later
		general.execute_command('ai_debugMNMAgentType VehicleMedium') #LargeActor 
	except RuntimeError as exc:
		print('MediumSizedCharacters, and/or VehicleMedium are invalid agent types for your game.')
	
	# AI debug view will not display anything unless there are active AI and MNM built. Warn user about this. 
	print('NOTE: This view is only useful if there are active AI on screen, and Multi-layer Navigation Mesh generated.')
	
	aiDebugDraw.next_value()

def art():
	'''Art related debug views.'''
	
	if 'e_debugdraw' not in CVARS['current']:
		# Since it was desired to mix and match cvars, supporting additional cvar calls before we jump into cycling eDebugDraw
		if 'r_wireframe' not in CVARS['current']:
			try:
				general.set_cvar('r_wireframe', 1)
				CVARS['current']['r_wireframe'] = 0
			except RuntimeError as exc:
				print('Debug Views: WARNING - Unable to set r_wireframe 1')
				general.log('Debug Views: WARNING - Unable to set r_wireframe 1')
		else:
			try:
				general.set_cvar('r_wireframe', 0)
				general.set_cvar('r_showlines', 2)
				CVARS['current']['e_debugdraw'] = 0
			except RuntimeError as exc:
				print('Debug Views: WARNING - Unable to set r_wireframe 1')
				general.log('Debug Views: WARNING - Unable to set r_wireframe 1')
	else:
		# disabling any of the mixed cvars that could have been set above
		try:
			general.set_cvar('r_wireframe', 0)
			general.set_cvar('r_showlines', 0)
		except:
			pass
			
		eDebugDraw = cvar('e_debugdraw', 0, 'art', 'E_DEBUGDRAW - Review Art Assets LODs \ Texture Memory \ Render Mat Count \ Phys Tri Count.')
		# NOTE: I removed e_debugdraw 1 because it was crashing after the 3.6 integration
		eDebugDraw.add_value(4, 'Texture memory total for each object.')
		eDebugDraw.add_value(5, 'Renderable material counts for each object.')
		eDebugDraw.add_value(19, 'Physics triangle count for each object.')
		eDebugDraw.add_value(22, 'Current LOD index and its corrisponding vertex count for each object.')
		
		eDebugDraw.next_value()
	
def materialfx():
	'''Material fx related debug views.'''
	mfxDebug = cvar('mfx_Debug', 0, 'materialfx', 'Show material collisions to verify impact type.')
	mfxDebug.add_value(1, 'Material Impact Debug - 1.')
	mfxDebug.add_value(2, 'Material Impact Debug - 2.')
	mfxDebug.add_value(3, 'Material Impact Debug - Draw collisions and their surface type.')
	
	set_auxiliary_cvar('mfx_DebugVisual', 0, 2)
	
	mfxDebug.next_value()

# to be reinstated using ATL
# def soundfx():
	# '''Sound fx related debug views.'''
	# sfxDebug = cvar('s_SoundInfo', 0, 'soundfx', 'Show various sound fx debug views.')
	# sfxDebug.add_value(1, 'SoundFX - Sounds Playing')
	# sfxDebug.add_value(8, 'SoundFX - Music Information')
	# sfxDebug.add_value(12, 'SoundFX - Memory Usage By Project and Group')
	# 
	# set_auxiliary_cvar('s_DrawSounds', 0, 5)
	# 	
	# sfxDebug.next_value()

###############################################################################		
def main():
	'''sys.__name__ wrapper function'''
	# Need to verify no other toolbars are active, if they are clear them out if they do not match the current argument passed in. 
	if sys.argv:
		if len(sys.argv) > 1:
			# Convert the system argument string over to the function pointer in this file. ie 'profile' -> __main__.profile()
			function = sys.argv[1]
			if function in CVARS:
				# Disable any cvars that would negatively effect the display of the given function's debug views. 
				init_cvars(function)

				this = sys.modules[__name__]
				if function in dir(this):
					getattr(this, function)()
			elif function == 'default_view':
				reset_cvars()

if __name__ == '__main__':
	# GLOBAL NOTE: 
	# - All python scripts should execute through a main() function. 
	# - Otherwise python will not properly garbage collect the top level variables.
	main()
