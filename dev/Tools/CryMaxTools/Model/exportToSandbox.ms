--export to cryLayer
--by Sascha Herfort
--
--exports a selection of nodes into a new layer of cryEngine sandbox level
--requires max file directory to be export directory, export file per node and CGF filetype

rollout rltCryExportToSandbox "Export to Sandbox Layer" width:190 height:492
(
	--######################################################################################
	--###  GUI  ###############################################################################
	--######################################################################################
	label lblInfo1 "1. Export some objects with:" offset:[-8,0] align:#left
	label lblInfo2 "    Filetype: CGF" offset:[-8,-5] align:#left
	label lblInfo3 "    Export File per Node: on" offset:[-8,-5] align:#left
	label lblInfo4 "    Merge All Nodes: off" offset:[-8,-5] align:#left
	
	label lblInfo5 "2. Select level and press button." offset:[-8,0] align:#left
	label lblInfo6 "    All objects in the export list will" offset:[-8,-5] align:#left
	label lblInfo7 "    be placed in the layer with the" offset:[-8,-5] align:#left
	label lblInfo8 "    same position/rotation as in max." offset:[-8,-5] align:#left
	
	button btnSetLevel "Set Level" offset:[0,3] width:172 height:16 align:#center
	button btnCreateLayer "CREATE SANDBOX LAYER" offset:[0,-5] width:172 height:24 align:#center enabled:false
	
	label lblInfo9 "3. Open the level in sandbox and" offset:[-8,0] align:#left
	label lblInfo10 "    add the \"cryExportLayer\"." offset:[-8,-5] align:#left
	
	label lblWarning1 "Warning!" offset:[-8,0] align:#left
	label lblWarning2 "The script will overwrite this layer." offset:[-8,-5] align:#left
	label lblWarning3 "Do not use it for anything else!" offset:[-8,-5] align:#left
	label lblWarning4 "Do not check it into the build!" offset:[-8,-5] align:#left
	
	
	--######################################################################################
	--###  GLOBAL VARIABLES  #####################################################################
	--######################################################################################
	
	--TWEAKABLES
	local sLayerName = "cryExportLayer"
	local fUnitScale = 0.01 --convert from 3dsmax units (centimeters) to editor units (meters)
	local bDebugPrint = false
	
	--GLOBALS
	local aObjects = #()
	local sLayerPath = undefined
	
	
	--######################################################################################
	--###  FUNCTIONS  ##########################################################################
	--######################################################################################
	fn fnGetGUID =
	(--return a windows GUID
		dotNet.loadAssembly "mscorlib"
		local sResult = "{" + (((dotNetClass "system.guid").newGuid()).toString()) + "}"
		
		if bDebugPrint then print ("Generating GUID: " + sResult)
		
		sResult
	)
	
	fn fnGetTransformsAsStrings nNode =
	(--returns an array containing pos and rot as strings for cryLayer
		local aResult = #()
		local sPos = (nNode.transform.pos*fUnitScale) as string
		append aResult (subString sPos 2 (sPos.count-2))
			
		/*
		local p3Euler = nNode.transform.rotation as eulerangles
		local p3EulerSwizzled = eulerangles p3Euler.z p3Euler.x p3Euler.y
		local qQuat = p3EulerSwizzled as quat
		local aRot = filterString (qQuat as string) "( )"
		*/
		
		local qQuat = nNode.transform.rotation
		local qQuatFlipped = quat -qQuat.x -qQuat.y -qQuat.z qQuat.w --don't ask my why I need to flip this for the engine!
		local aRot = filterString (qQuatFlipped as string) "( )"
		--append aResult (aRot[5] + "," + aRot[4] + "," + aRot[3] + "," + aRot[2]) --notation seems reverse in sandbox layer
		append aResult (aRot[5] + "," + aRot[2] + "," + aRot[3] + "," + aRot[4]) --notation seems reverse in sandbox layer
		
		
		aResult
	)
	
	fn fnSetLayerPath sLevelPath: =
	(--sets the path for the cryExportLayer - unsupplied sLevelPath will call user dialog - returns level path or undefined
		if sLevelPath == unsupplied then
		(
			local sGameDataDir = case of
			(
				(sLayerPath != undefined): (subString sLayerPath 1 (sLayerPath.count-7))
				(doesFileExist (cryMaxTools.basic.vars.buildPath + "\\GameCrysis2\\Levels")): (cryMaxTools.basic.vars.buildPath + "\\GameCrysis2\\Levels")
				(doesFileExist (cryMaxTools.basic.vars.buildPath + "\\Game\\Levels")): (cryMaxTools.basic.vars.buildPath + "\\Game\\Levels")
				default: cryMaxTools.basic.vars.buildPath
			)
			
			sLevelPath = getSavePath caption:"Please set level to create layer in." initialDir:sGameDataDir
			if sLevelPath == undefined or (findString sLevelPath cryMaxTools.basic.vars.buildPath) != 1 then
			(
				sLevelPath = undefined
			)
			else
			(--cut off build path
				sLevelPath = subString sLevelPath (cryMaxTools.basic.vars.buildPath.count + 2) -1
				
				if (filterString sLevelPath "\\")[2] != "Levels" or (not doesFileExist (cryMaxTools.basic.vars.buildPath + "\\" + sLevelPath + "\\Layers")) then
				(--verify path
					print "No \"Layers\" folder found in Level folder. Cannot create new layer. Please select a level with a \"Layers\" folder."
					
					sLevelPath = undefined
				)
			)
		)
		
		if sLevelPath != undefined then
		(
			sLayerPath = cryMaxTools.basic.vars.buildPath + "\\" + sLevelPath + "\\Layers\\"
			
			sLevelPath
		)
		else
		(
			sLayerPath = undefined
			
			undefined
		)
	)
	
	
	--######################################################################################
	--###  USER INPUT EVENT HANDLERS  ##############################################################
	--######################################################################################
	
	on btnSetLevel pressed do
	(--set level
		local sPath = fnSetLayerPath()
		
		if sPath != undefined then
		(
			btnSetLevel.caption = "Level: " + sPath
			btnCreateLayer.enabled = true
		)
		else
		(
			btnSetLevel.caption = "Set Level"
			btnCreateLayer.enabled = false
		)
	)
	
	on btnCreateLayer pressed do
	(
		--grab objects from cryExporter
		max utility mode
		UtilityPanel.OpenUtility CryEngine2_Exporter
		aObjects = csExport.export.get_node_list()
		
		dotNet.loadAssembly "system.xml"
		local xmlLayerFile = dotNetObject "system.xml.xmlDocument" --create XML document
		local xmlRootNode = xmlLayerFile.createElement "ObjectLayer"
		xmlLayerFile.appendChild xmlRootNode
		
		local xmlLayerNode = xmlLayerFile.createElement "Layer"
		xmlLayerNode.setAttribute "Name" sLayerName
		xmlLayerNode.setAttribute "GUID" (fnGetGUID()) --need to generate GUID with dotNet
		xmlLayerNode.setAttribute "Hidden" "0"
		xmlLayerNode.setAttribute "Frozen" "0"
		xmlLayerNode.setAttribute "External" "1"
		xmlLayerNode.setAttribute "Exportable" "0"
		xmlRootNode.appendChild xmlLayerNode
		
		local xmlLayerObjectsNode = xmlLayerFile.createElement "LayerObjects"
		xmlLayerNode.appendChild xmlLayerObjectsNode
		
		for each in aObjects do
		(--create xml node per object
			local sName = each.name
			local aTransforms = fnGetTransformsAsStrings each
			local sObjectPath = maxFilePath + each.name + ".cgf"
			
			if bDebugPrint then print ("Putting object to layer: " + sName + " - Transforms: " + aTransforms[1] + " " + aTransforms[2] + " - Filepath: " + sObjectPath)
			
			local xmlObjectNode = xmlLayerFile.createElement "Object"
			xmlObjectNode.setAttribute "Type" "GeomEntity"
			xmlObjectNode.setAttribute "Layer" sLayerName
			xmlObjectNode.setAttribute "Id" (fnGetGUID()) --need to generate GUID with dotNet
			xmlObjectNode.setAttribute "Name" sName
			xmlObjectNode.setAttribute "Pos" aTransforms[1]
			xmlObjectNode.setAttribute "Rotate" aTransforms[2]
			xmlObjectNode.setAttribute "ColorRGB" "65535"
			xmlObjectNode.setAttribute "MatLayersMask" "0"
			xmlObjectNode.setAttribute "Geometry" sObjectPath
			xmlObjectNode.setAttribute "OutdoorOnly" "0"
			xmlObjectNode.setAttribute "CastShadow" "1"
			xmlObjectNode.setAttribute "MotionBlurMultiplier" "1"
			xmlObjectNode.setAttribute "LodRatio" "100"
			xmlObjectNode.setAttribute "ViewDistRatio" "100"
			xmlObjectNode.setAttribute "HiddenInGame" "0"
			xmlObjectNode.setAttribute "RecvWind" "0"
			xmlObjectNode.setAttribute "RenderNearest" "0"
			xmlObjectNode.setAttribute "EntityClass" "GeomEntity"
			xmlLayerObjectsNode.appendChild xmlObjectNode
			
			local xmlPropertiesNode = xmlLayerFile.createElement "Properties"
			xmlPropertiesNode.setAttribute "eiSpecies" "-1"
			xmlObjectNode.appendChild xmlPropertiesNode
			
			local xmlHealthNode = xmlLayerFile.createElement "Health"
			xmlHealthNode.setAttribute "bInvulnerable" "1"
			xmlHealthNode.setAttribute "MaxHealth" "500"
			xmlHealthNode.setAttribute "bOnlyEnemyFire" "1"
			xmlPropertiesNode.appendChild xmlHealthNode
		)
		
		print ("Saving layer: " + sLayerPath + sLayerName + ".lyr")
		
		deleteFile (sLayerPath + sLayerName + ".lyr")
		xmlLayerFile.save (sLayerPath + sLayerName + ".lyr")
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryExportToSandbox" rltCryExportToSandbox #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExportToSandbox")