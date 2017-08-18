/*#####################################################
Author: 		Harald Zlattinger
Date:			2010.01.13
Script:		ARMS_Max_Tool
Description:	Building assembly from Prefab file

Requirements:
	- needs a prefab xml file in your working directory e.g.: J:\Games\Crysis2\GameCrysis2\Prefabs\
	- needs a Max Scene with the name consisting of the first two tokens of the CGFs + "_source.max"
	  in the same directory as the CGFs
		J:\Games\Crysis2\GameCrysis2\Objects\arms\var_06_source.max
	- the MAX Scene must contain all the source parts used for the building organised in
	  layers named "source_LOD0", "source_LOD1"
	  
Version History:
	0.43	- added checkbox for create new scene - keep old scene
	0.42    - fixed a bug which made troubles when max scene had no LODs
	0.41	- fixed bug where leftover buildings were in list
			- converted all copies to EditablePoly
	0.4		- changed LOD0 names to not use "_LOD0"
			- deletes every empty Layer except "0 (Default)"
			- "building_LOD0" layer renamed to "building"
			- automatically scans MAX scene for max. lod number
	0.3		- Added better error checking
			- added color per floor odd/even
			- added dynamic MAx File tracing
	0.2		- Added LOD handling
			- added Layers
	0.1		Created rough version
#####################################################*/

gc()
global ARMS_Window
try(destroyDialog ARMS_Window)catch()
dotNet.loadAssembly "system.xml"
global ARMS_xmlDoc = dotNetObject "system.xml.xmlDocument"
global ARMS_sWorkingDirectory = ""
global ARMS_sXMLFile = ""
global ARMS_aBuildings = #()
global ARMS_docEle = ""
global ARMS_iMaxNumLODs = 2
global ARMS_errorArray = #()
global ARMS_sVersionNumber = "0.43"
global ARMS_theMaxSceneFile = ""
global ARMS_cOddColor = color 160 180 225
global ARMS_cEvenColor = color 0 35 125

fn ARMS_Tool =
(
	fn ARMS_fnGetWorkingDirectory =
	(
		sRootPath = csexport.get_root_path()
		sConfigFile = sRootPath+"\\system.cfg"
		if doesFileExist(sConfigFile) then
		(
			fStream = openFile sConfigFile mode:"r"
			while not eof fStream do
			(
				sNextLine = readLine fStream
				if (matchPattern sNextLine pattern:"sys_game_folder*") then
				(
					aTokens = filterString sNextLine " ="
					ARMS_sWorkingDirectory = sRootPath + "\\" + aTokens[2] + "\\"
					ARMS_sXMLFile = getOpenFileName caption:"Open a Prefab file:" filename:(ARMS_sWorkingDirectory + "Prefabs\\") types:"Prefab XML (*.xml)|*.xml"
					close fStream
					return true
				)
			)
			close fStream
			return false
		)
		else
		(
			messageBox ("There is something wrong with your project files and rc.\nFile not found in:\n\""+sConfigFile+"\"") title:"FATAL ERROR"
			close fStream
			return false
		)
		return false
	)
	-- Create Window with controls
	fn ARMS_fnARMSWindow =
	(
		-- entry of listbox gets loaded
		rollout arms_rollout ("ARMS Tool v"+ARMS_sVersionNumber)
		(
			slider lodnum "Number of LODs to load: 2" orient:#horizontal type:#integer range:[0,5,ARMS_iMaxNumLODs] ticks:4 enabled:false
			colorpicker evencolPicker "Even floors:" color:ARMS_cEvenColor offset:[0,10]
			colorpicker oddcolPicker "Odd floors:" color:ARMS_cOddColor align:#right offset:[0,-25]
			on evencolPicker changed new_col do ARMS_cEvenColor = new_col
			on oddcolPicker changed new_col do ARMS_cOddColor = new_col
			listbox buildings "Doubleclick the desired building:" items:(ARMS_aBuildings) offset:[0,5]
			on lodnum changed val do
			(
				ARMS_iMaxNumLODs = val
				lodnum.caption = ("Number of LODs to load: " + val as string)
			)
			checkBox cbNewScene "Don't create new scene - merge to current"
			button restartButton "Load Prefab File" align:#left width:100
			button createBuilding "Create Building" align:#right offset:[0,-26] width:100

			-- get color by odd or even number
			fn fnARMS_getColorForFloor floorNum =
			(
				theColor = color 0 0 0
				if (mod floorNum 2) > 0 then
				(
					theColor = ARMS_cOddColor
				)
				else
				(
					theColor = ARMS_cEvenColor
				)
				return theColor
			)
			
			-- scans Array for highest LOD num - requires array with object names
			fn ARMS_fnGetMxLODsOfScene theMaxPiecesArray =
			(
				maxLods = 0
				collateArray = #()
				for obj in theMaxPiecesArray do
				(
					if (matchPattern obj pattern:"*_LOD?") then
					(
						lodNumber = (substring obj obj.count 1)
						appendIfUnique collateArray (lodNumber as number)
					)
				)
				if (aMax collateArray) != undefined then
                (
                    maxLods = (aMax collateArray)
                )
				return maxLods
			)

			-- read XML values for the building and process max scene
			fn fnGetXMLBuildingData iBuildingNum = 
			(
				if cbNewScene.checked != true then
				(
					resetMaxFile #noPrompt
				)
				global aARMS_Building_Array = #()
				for x = 0 to (ARMS_docEle.ChildNodes.itemOf[iBuildingNum - 1].ChildNodes.itemOf[0].ChildNodes.count-1) do
				(
					local aARMS_Building_Array_tmp = #()
					try
					(
						aARMS_Building_Array_tmp[1] = (ARMS_docEle.ChildNodes.itemOf[iBuildingNum - 1].ChildNodes.itemOf[0].ChildNodes.itemOf[x].GetAttributeNode "Prefab").value
					) catch()
					try
					(
						aARMS_Building_Array_tmp[2] = (ARMS_docEle.ChildNodes.itemOf[iBuildingNum - 1].ChildNodes.itemOf[0].ChildNodes.itemOf[x].GetAttributeNode "Pos").value
					)
					catch
					(
						aARMS_Building_Array_tmp[2] = "0,0,0"
					)
					try
					(
						aARMS_Building_Array_tmp[3] = (ARMS_docEle.ChildNodes.itemOf[iBuildingNum - 1].ChildNodes.itemOf[0].ChildNodes.itemOf[x].GetAttributeNode "Rotate").value
					)
					catch
					(
						aARMS_Building_Array_tmp[3] = "1,0,0,0"
					)
					try
					(
						aARMS_Building_Array_tmp[4] = (ARMS_docEle.ChildNodes.itemOf[iBuildingNum - 1].ChildNodes.itemOf[0].ChildNodes.itemOf[x].GetAttributeNode "FloorNumber").value
					)
					catch
					(
						aARMS_Building_Array_tmp[4] = "0"
					)
					aARMS_Building_Array[x+1] = aARMS_Building_Array_tmp
				)
				
				-- get the Name of the MAX File
				ARMS_theMaxSceneFile = ARMS_sWorkingDirectory
				aTokenizer = filterString aARMS_Building_Array[1][1] "\\/."
				for x = 1 to aTokenizer.count-2 do
				(
					ARMS_theMaxSceneFile += aTokenizer[x] + "\\"
				)
				aTokenizer = filterString aTokenizer[aTokenizer.count-1] "_"
				ARMS_theMaxSceneFile += aTokenizer[1] + "_" + aTokenizer[2] + "_source.max"
				
				-- check, if the MAX File exists
				if doesFileExist(ARMS_theMaxSceneFile) then
				(
					-- check LODs in Scene
					theMaxPiecesArray = getMAXFileObjectNames ARMS_theMaxSceneFile quiet:true
					ARMS_iMaxNumLODs = ARMS_fnGetMxLODsOfScene theMaxPiecesArray
					print ("Max Scene's highest LOD number: "+ARMS_iMaxNumLODs as string)
					
					-- go through arrays to see, what to duplicate
					aPiecesToLoad = #()
					for x = 0 to aARMS_Building_Array.count-1 do
					(
						aTokens = #()
						aTokens = filterString aARMS_Building_Array[x+1][1] "\\/."
						
						append aPiecesToLoad aTokens[aTokens.count-1]
						for y = 1 to ARMS_iMaxNumLODs do
						(
							sLODName = ("_LOD"+y as string)
							append aPiecesToLoad (aTokens[aTokens.count-1]+sLODName)
						)
						aARMS_Building_Array[x+1][1] = aTokens[aTokens.count-1]
					)
					aPiecesToLoad = makeUniqueArray aPiecesToLoad

					-- merge Pieces from the other MAX Scene into the current Scene
					mergeMAXFile ARMS_theMaxSceneFile aPiecesToLoad #deleteOldDups #useMergedMtlDups
					aLayers = #()
					for x = 0 to ARMS_iMaxNumLODs do
					(
						if x == 0 then
						(
							theLayer = LayerManager.newLayerFromName("building")
						)
						else
						(
							theLayer = LayerManager.newLayerFromName("building_LOD"+x as string)
						)
						
						if x > 0 then
						(
							theLayer.ishidden = true
						)
						aLayers[x+1] = theLayer
					)
					
					-- duplicate, position and rotate objects accordingly
					for z = 1 to aARMS_Building_Array.count do
					(
						theColor = (fnARMS_getColorForFloor (aARMS_Building_Array[z][4] as number))
						aMeshAndLODs = #()
						aMeshAndLODs[1] = aARMS_Building_Array[z][1]
						for x = 1 to ARMS_iMaxNumLODs do
						(
							append aMeshAndLODs (aARMS_Building_Array[z][1]+"_LOD"+x as string)
						)
						for x = 0 to aMeshAndLODs.count-1 do
						(
							nSourceObj = (getNodeByName aMeshAndLODs[x+1])
							if nSourceObj != undefined then
							(
								newObj = convertToPoly((snapshot nSourceObj))
								newObj.wireColor = theColor
								aPosition = (filterString aARMS_Building_Array[z][2] ",")
								aRotation = (filterString aARMS_Building_Array[z][3] ",")
								quatRotation = quat (aRotation[2] as float) (aRotation[3] as float) (aRotation[4] as float) (aRotation[1] as float)
								p3Position = [((aPosition[1] as float)*100),((aPosition[2] as float)*100),((aPosition[3] as float)*100)]
								newObj.rotation = quatRotation
								newObj.position = p3Position
								if x == 0 then
								(
									newObj.name = (aARMS_Building_Array[z][1] + "_" + z as string)
								)
								else
								(
									newObj.name = (aARMS_Building_Array[z][1] + "_" + z as string + "_LOD"+(x as string))
								)
								aLayers[x+1].addNode newObj
							)
						)
					)
					-- delete source objects
					ARMS_errorArray = #()
						
					for objs in aPiecesToLoad do
					(
						nSourceObj = (getNodeByName objs)
						if nSourceObj != undefined then
						(
							delete nSourceObj
						)
						else
						(
							append ARMS_errorArray (objs)
						)
					)
					-- delete unnecessary layers (empty layers)
					deleteLayers = #()
					for x = 0 to layerManager.count-1 do
					(
						theLayer = layerManager.getLayer x
						layerName = theLayer.name
						theLayer.nodes &theNodes
						if theNodes.count == 0 then
						(
							append deleteLayers theLayer.name
						)
					)
					for obj in deleteLayers do
					(
						try
						(
							layerManager.deleteLayerByName obj
						)
						catch ()
					)
					
					-- zoom and set edged faces
					viewport.activeViewport = 4
					max tool maximize
					viewport.SetShowEdgeFaces true
					max zoomext sel all
					
					-- print errors
					if ARMS_errorArray.count > 0 then
					(
						messageBox("There were some objects and/or LODs missing!\nCheck the MAXScript Listener window for details!")
						print "Missing objects:"
						print "##################################################################"
						print ARMS_errorArray
						print "ERRORS ############################################################"
					)
				)
				else
				(
					messageBox("The Source MAX File couldn't be found!\nPlease check:\n\""+ARMS_theMaxSceneFile+"\"")
				)
			)
			
			-- Controls actions
			on buildings doubleClicked val do
			(
				ARMS_xmlDoc.load ARMS_sXMLFile
				global ARMS_docEle=ARMS_xmlDoc.documentElement
				fnGetXMLBuildingData val
			)
			on restartButton pressed do
			(
				try(destroyDialog ARMS_Window)catch()
				gc()
                global ARMS_aBuildings = #()
				ARMS_Tool()
			)
			on createBuilding pressed do
			(
				val = buildings.selection
				ARMS_xmlDoc.load ARMS_sXMLFile
				global ARMS_docEle=ARMS_xmlDoc.documentElement
				fnGetXMLBuildingData val
			)
		)
		createDialog arms_rollout 250 305
		ARMS_Window = arms_rollout
	)

	-- check XML File, load nodes and display in window
	if ARMS_fnGetWorkingDirectory() == true then
	(
		if ARMS_sXMLFile != undefined then
		(
			if doesFileExist(ARMS_sXMLFile) then
			(
				-- get new path for ARMS_sWorkingDirectory assuming that XML File is in new working dir
				ARMS_xmlDoc.load ARMS_sXMLFile
				global ARMS_docEle=ARMS_xmlDoc.documentElement
				if ARMS_docEle!=undefined then
				(
					for i = 0 to ARMS_docEle.ChildNodes.count-1 do
					(
						try
						(
							objNameStr=(ARMS_docEle.ChildNodes.itemOf[i].GetAttributeNode "Name").value
							ARMS_aBuildings[i+1] = objNameStr
						)
						catch()
					)
				)

				if ARMS_aBuildings.count > 0 then
				(
					ARMS_fnARMSWindow()
				)
				else
				(
					messageBox "There was a problem reading the xml file.\nMaybe this is not a correct prefab xml." title:"CRITICAL ERROR"
				)
			)
			else
			(
				messageBox ("File was not found:\n\""+ARMS_sXMLFile+"\"\n\nPlease sync the file from perforce.") title:"Prefab File not found"
			)
		)
		else
		(
			messageBox ("You have to select a prefab xml-file") title:"CRITICAL ERROR"
		)
	)
	else
	(
		messageBox ("Problem with the working directory!") title:"CRITICAL ERROR"
	)
)
ARMS_Tool()