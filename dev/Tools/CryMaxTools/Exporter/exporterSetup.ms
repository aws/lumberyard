---------------------------
-- CryMaxTools v2.0
-- Exporter Setup v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryExportSetup "Settings"
(
	button btnSettingsManager "CryENGINE3 Settings" width:172 height:24 align:#center
	
	button btnDisplayLog "Display Message Log" width:172 height:16 align:#center offset:[0,3]
	
	group "General Export Options"
	(
		--checkbox chkModernPopups "Less obtrusive Popups" align:#left offset:[0,0]
		
		label lblPopupVerbosity "Popup Message on:" align:#left offset:[0,3] --can move headline closer to radiobuttons a few pixels
		radiobuttons rdoPopupVerbosity labels:#("Error, Warning, Notification","Error, Warning", "Error") default:2 columns:1 align:#left offset:[0,-4]
		
		label lblPreExport "Before Export:" align:#left offset:[0,3]
		checkbox chkReparentTwistBones "Reparent Biped Twist Bones" align:#left offset:[0,-2]
	)
	
	group "Object Exporter Checks"
	(
		label lblPreObjectExportChecks "Before Export:" align:#left offset:[0,0]
		checkbox chkWarningMaterialSetup "Material Setup" align:#left offset:[0,-2]
		checkbox chkWarningNonZeroMorphs "Non Zero Morphs" align:#left offset:[0,-5]
		checkbox chkWarningBoneScale "Bones Scale" align:#left offset:[0,-5]
		
		--feature not yet supported:
		--label lblPostObjectExportChecks "After Export:" align:#left offset:[0,3]
		--checkbox chkHelperDegenerateFaces "Degenerate Faces/UVs" align:#left offset:[0,-2] enabled:false
		--checkbox chkHelperCollapsedVertices "Collapsed Triangles" align:#left offset:[0,-5] enabled:false
	)
	
	--group "Animation Exporter Checks"
	--(
		--label lblPreAnimationExportChecks "Before Export:" align:#left offset:[0,0]
	--)
	
--#########################################################
--GLOBAL VARIABLES
--#########################################################
	local bPrintFunctionNames = false
	
--#########################################################
--GLOBAL FUNCTIONS
--#########################################################
	fn fnUpdateUI =
	(--updates settings UI from global settings variables
		if bPrintFunctionNames then (print "fnUpdateUI")
		
		rdoPopupVerbosity.state = 4 - cryMaxTools.export.iPopupVerbosity
		chkReparentTwistBones.state = cryMaxTools.export.bReparentTwistBones
		
		chkWarningMaterialSetup.state = cryMaxTools.export.object.bCheckMaterials
		chkWarningNonZeroMorphs.state = cryMaxTools.export.object.bCheckMorphs
		--feature not yet supported:
		--chkHelperDegenerateFaces.state = cryMaxTools.export.object.bMarkDegenerateFaces
		--chkHelperCollapsedVertices.state = cryMaxTools.export.object.bMarkCollapsedVertices
		
		chkWarningBoneScale.state = cryMaxTools.export.object.bCheckBoneScale
	)
	
	
--#########################################################
--UI EVENT HANDLERS
--#########################################################
	on rltCryExportSetup open do
	(--on open get values from global vars
		fnUpdateUI()
	)
	
	on rltCryExportSetup close do
	(--on close save settings
		cryMaxTools.export.fnSaveSettings()
		callbacks.removeScripts id:#updateExportSettingsUI
	)
	
	on btnDisplayLog pressed do
	(--display log file
		cryMaxTools.export.log.fnDisplayLogFile()
	)
	
	on btnSettingsManager pressed do
	(--run cryengine2 settings manager
		shellLaunch (crymaxtools.basic.vars.buildpath + "\\Tools\\SettingsMgr.exe") ""
	)

	
	on rdoPopupVerbosity changed state do
	(--change global var
		cryMaxTools.export.iPopupVerbosity = (4 - state)
		cryMaxTools.export.fnSaveSettings()
	)
	
	on chkReparentTwistBones changed state do
	(--change global var
		cryMaxTools.export.bReparentTwistBones = state
		cryMaxTools.export.fnStorePerFileSettings()
	)

	
	on chkWarningMaterialSetup changed state do
	(--change global var
		cryMaxTools.export.object.bCheckMaterials = state
		cryMaxTools.export.fnSaveSettings()
	)
	
	on chkWarningNonZeroMorphs changed state do
	(--change global var
		cryMaxTools.export.object.bCheckMorphs = state
		cryMaxTools.export.fnSaveSettings()
	)
	
	--feature not yet supported:
	/*
	on chkHelperDegenerateFaces changed state do
	(--change global var
		cryMaxTools.export.object.bMarkDegenerateFaces = state
		cryMaxTools.export.fnSaveSettings()
	)
	
	on chkHelperCollapsedVertices changed state do
	(--change global var
		cryMaxTools.export.object.bMarkCollapsedVertices = state
		cryMaxTools.export.fnSaveSettings()
	)
	*/

	on chkWarningBoneScale changed state do
	(--change global var
		cryMaxTools.export.object.bCheckBoneScale = state
		cryMaxTools.export.fnSaveSettings()
	)
)

--add exporter rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryExportSetup" rltCryExportSetup #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExportSetup")
callbacks.addScript #filePostOpen "(cryMaxTools.basic.ROMan.get \"rltCryExportSetup\").fnUpdateUI()" id:#updateExportSettingsUI
callbacks.addScript #filePostMerge "(cryMaxTools.basic.ROMan.get \"rltCryExportSetup\").fnUpdateUI()" id:#updateExportSettingsUI
callbacks.addScript #postImport "(cryMaxTools.basic.ROMan.get \"rltCryExportSetup\").fnUpdateUI()" id:#updateExportSettingsUI
callbacks.addScript #systemPostNew "(cryMaxTools.basic.ROMan.get \"rltCryExportSetup\").fnUpdateUI()" id:#updateExportSettingsUI
callbacks.addScript #systemPostReset "(cryMaxTools.basic.ROMan.get \"rltCryExportSetup\").fnUpdateUI()" id:#updateExportSettingsUI