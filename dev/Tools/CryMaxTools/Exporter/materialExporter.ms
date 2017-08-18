---------------------------
-- CryMaxTools v2.0
-- Material Exporter v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryExportMaterial "CryEngine3 Material Export" width:190 --height:496
(
	group "Export Material"
	(
		button btnMaterialExportSingle "Export Material Editor Selection" height:16 width:160 align:#center offset:[0,-2] toolTip:"Generates an MTL file from the material selected in the 3dsmax material editor."
		button btnMaterialExportMulti "Export Object Exporter Selection" height:16 width:160 align:#center offset:[0,3] toolTip:"Generates one MTL file for each material used by the selected objects in the object exporter list."
	)
	
	/*
	group "Import Material"
	(
		button btnMaterialImportSingle "Import to Material Editor" height:16 width:160 align:#center offset:[0,-2] toolTip:"Loads an MTL file and puts the material into the selected slot in the 3dsmax Material editor." enabled:false
		button btnMaterialImportMulti "Selected in Object Exporter" height:16 width:160 align:#center offset:[0,3] toolTip:"Syncs each material used by the selected objects in the object exporter list with the according MTL file." enabled:false
	)
	*/
	
--########################################################################################
--MATERIAL EXPORTER GLOBAL VARIABLES
--########################################################################################
	
--########################################################################################
--MATERIAL EXPORTER GLOBAL FUNCTIONS
--########################################################################################

--########################################################################################
--MATERIAL EXPORTER UI EVENT HANDLERS
--########################################################################################
	on btnMaterialExportSingle pressed do
	(
		cryMaxTools.export.material.fnExportMaterial (medit.GetCurMtl())
	)
	
	on btnMaterialExportMulti pressed do
	(
		local aSelection = undefined
		try
		(
			aSelection = (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.selection
		)
		catch()
		
		if aSelection != undefined then
		(			
			cryMaxTools.export.material.fnExportMaterialsFromExportNodes aSelection
		)
	)
)

--add exporter rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryExportMaterial" rltCryExportMaterial #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExportMaterial")