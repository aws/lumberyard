-------------------------------------------
-- CryMaxTools v2.0
-- Diagnostic Tools
-- by Chris Evans/Sascha Herfort/Marcus Krautwurst
---------------------------

rollout animDialogToolBoxRO "Dialogs"
(
	
	button btnAnimMan "Animation Manager" pos:[8,25-15] width:160 height:16
	button btnAlignLocator "Align / Pivot / Locator" pos:[8,48-15] width:160 height:16
	button btnAimPoseCreator "Direction Blend Creator" pos:[8,71-15] width:160 height:16
	
	groupbox gbMirror " Mirror " pos:[5,71-15+30] width:166 height:36
	button btnMirrorRotation "Rotation" pos:[8,71-15+46] width:78 height:16
	button btnMirrorAnmimation "Animation" pos:[8+82,71-15+46] width:78 height:16
	
	
	on btnAnimMan pressed do
		fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AnimManager.ms")
	
	on btnAlignLocator pressed do
		fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AlignLocator.ms")
	
	on btnAimPoseCreator pressed do
		fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AimPoseCreator.ms")
	
	
	on btnMirrorRotation pressed do
		fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\MirrorRotation.ms")
)
--add diagnostics tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "animToolBoxRO" animDialogToolBoxRO #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "animDialogToolBoxRO") rolledUp:false