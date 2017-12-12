
--cryAMLoaded = false

/*try
(
	cryAM.showCryAM()
	cryAMLoaded = true
)
catch()
*/

try
(
	for i = cryAM.controlArray.count to 1 by -1 do
	(
		try cryAM.controlArray[i].Dispose() catch()
		try cryAM.controlArray[i] = undefined catch()
	)
)catch()


global cryNET = undefined
global cryAM = undefined

gc light:true



struct cryNET_Struct
(
	anchorStyles = dotNetClass "System.Windows.Forms.AnchorStyles",
	anchorNone = anchorStyles.None,
	anchorLeftRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Right,
	anchorLeftTop = dotNet.combineEnums anchorStyles.Left anchorStyles.Top,
	anchorLeftTopRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Top anchorStyles.Right,
	anchorLeftBottom = dotNet.combineEnums anchorStyles.Left anchorStyles.Bottom,
	anchorLeftBottomRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Bottom anchorStyles.Right,
	anchorTop = anchorStyles.Top,
	anchorTopBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Bottom,
	anchorTopLeftBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Left anchorStyles.Bottom,
	anchorTopRightBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Right anchorStyles.Bottom,
	anchorTopBottomLeftRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Bottom anchorStyles.Left anchorStyles.Right,
	anchorTopRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Right,
	anchorBottom = dotNet.combineEnums anchorStyles.Bottom,
	anchorDefault = dotNet.combineEnums anchorStyles.Top anchorStyles.Left,
	anchorTopLeftRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Left anchorStyles.Right,
	
	
	function getNetPos pos =
	(
		return (dotNetObject "System.Drawing.Point" pos[1] pos[2])
	),
	
	function getNetSize pos =
	(
		return (dotNetObject "System.Drawing.Size" pos[1] pos[2])
	),
	
	function getMaxPos netPoint =
	(
		return [netPoint.x, netPoint.y]
	),
	
	function getLoc netCon =
	(
		if netCon != undefined then
		(
			local parentPos = [0,0]
			local tempParent = netCon.parent
			
			
				--// itterate through parent containers and get the local offset
			while tempParent != undefined do
			(
				local groupAdd = [0,0]
				if (subString tempParent.name 1 2) == "gb" then
					groupAdd = [1,1]
				
				parentPos += (cryNET_Struct.getMaxPos tempParent.location) + groupAdd
				
				tempParent = tempParent.parent
			)
			
			return ((cryNET_Struct.getMaxPos netCon.location) + parentPos)
		)
		
		return [0,0]
	),
	
	function getSize netCon =
	(
		return [netCon.width, netCon.height]
	),
	
	function setLoc netCon pos =
	(
		local parentPos = [0,0]
		local tempParent = netCon.parent
		
		
			--// itterate through parent containers and get the local offset
		while tempParent != undefined do
		(
			local groupAdd = [0,0]
			if (subString tempParent.name 1 2) == "gb" then
				groupAdd = [1,1]
			
			parentPos += (cryNET_Struct.getMaxPos tempParent.location) + groupAdd
			
			tempParent = tempParent.parent
		)
		
		
		netCon.location = cryNET_Struct.getNetPos (pos - parentPos)
	),
	
	function setSize netCon size =
	(
		netCon.size = cryNET_Struct.getNetSize size
	),
	
	function setLocSize netCon pos size =
	(
		cryNET_Struct.setLoc netCon pos
		cryNET_Struct.setSize netCon size
	),
	
	function getNetColor maxColor =
	(
		if classOf maxColor == Point3 then
			return ((dotNetClass "System.Drawing.Color").FromARGB maxColor[1] maxColor[2] maxColor[3])
		
		if classOf maxColor == Array then
			return ((dotNetClass "System.Drawing.Color").FromARGB maxColor[1] maxColor[2] maxColor[3] maxColor[4])
	),
	
	function createCon netToken customToken:undefined cPos:undefined cLoc:undefined cSize:[0,0] cName:"" cText:"" cChecked:undefined cParent:undefined cToolTip:"" cBackColor:undefined cForeColor:undefined cAnchor:undefined cVisible:undefined borderFixed:false borderFlat:false =
	(
		if customToken != undefined then
			netToken = customToken + "." + netToken
		else
			netToken = "System.Windows.Forms." + netToken
		
		
		local netCon = dotNetObject netToken
		dotNet.setLifeTimeControl netCon #mxs
		
		try if cParent != undefined then netCon.parent = cParent catch()
		if cName != "" then netCon.name = cName
		if cText != "" then netCon.text = cText
		if cChecked != undefined then netCon.checked = cChecked
		if cLoc != undefined then netCon.location = cryNET.getNetPos cLoc
		if cPos != undefined then cryNET.setLoc netCon cPos
		if cSize != [0,0] then cryNET_Struct.setSize netCon cSize
		try if cBackColor != undefined then netCon.backColor = cryNET_Struct.getNetColor cBackColor catch()
		if cForeColor != undefined then netCon.foreColor = cryNET_Struct.getNetColor cForeColor
		if cAnchor != undefined then netCon.anchor = cAnchor else netCon.anchor = anchorLeftTop
		if cVisible != undefined then netCon.visible = cVisible
		--if cToolTip != "" then netCon.ToolTip = cToolTip
		
		if borderFixed == true then
			netCon.BorderStyle = netCon.BorderStyle.FixedSingle
		
		if borderFlat == true then
			netCon.FlatStyle = netCon.FlatStyle.Flat
		
		append cryAM.controlArray netCon
			
		return netCon
	)
)
global cryNET = cryNET_Struct()












struct cryAM_conditionStruct (name = "", active = true, parentLayer = 0, variableID = 0, state = 1, condition = 0, group = 2, value = "")
struct cryAM_gameBoneStruct (name = "", conditions = #())
		
		
struct cryAM_setupModelStruct (path = "", autoSync = true, removeOld = true, preferLoad = false)
struct cryAM_setupBipedStruct (path = "", autoSync = true, removeOld = false)
struct cryAM_setupAlignStruct (sourceNode = "", targetNode = "", rot = (quat 1), pos = [0,0,0])
struct cryAM_setupMiscStruct (skipFirst = 0, skipLast = 0, customRig = false, mappingFile = "", poseFile = "")
struct cryAM_setupStruct (name = "", active = true, sourcePath = "", gamePath = "", sourceNodes = #(), gameBones = #(), load = "", model = cryAM_setupModelStruct(), biped = cryAM_setupBipedStruct(), align = cryAM_setupAlignStruct(), misc = cryAM_setupMiscStruct())
struct cryAM_characterStruct (name = "", active = true, setup = #((cryAM_setupStruct name:"Base Char")))


struct cryAM_perforceStruct (have, rev)
struct cryAM_maskStruct (	normal = #(),	add = #(), remove = #(), extension = #() )
struct cryAM_folderStruct (name, path, node)
struct cryAM_recentStruct (charID, setupID, path)
struct cryAM_lastCharStruct (charID = 0, usedSetup = 0, setup = #())
struct cryAM_lastSetupStruct (setupID = 0, folderID = 0, folderName = "", nodeSel = #{})
struct cryAM_animCBAParamStruct (name = "", value)
struct cryAM_animStruct
(
	name,
	type,
	path = "",
	start = 0,
	end = 0,
	export = "",
	perforce = cryAM_perforceStruct(),
	status = "",
	cbaParams = #(),
	node,
	selected = false
)



struct cryAM_scriptStruct (text = "", path = "")
struct cryAM_propertiesStruct (name = "", value = "", index = 0)
struct cryAM_functionStruct (name, properties = #(), index = 0)
struct cryAM_processStruct (name = "", active = true, script = cryAM_scriptStruct(), fnData = #())
struct cryAM_taskStruct (name = "", active = true, default = false, process = #())	
struct cryAM_resizeStruct (name = "", item, height = 0, width = 0, left = 0, top = 0)
struct cryAM_cbaStruct (modelPath = "", animationPath = "", compression = -1, autoCompression = -1, preset = "", dbaPath = "", locator = false)
struct cryAM_setupPresetStruct (charID = 0, charName = "", setupID = 0, setupName = "")
	
	
	
	
	
	
	
	
	


struct cryAM_Struct
(
	
		--// TASK STUFF
	taskArray = #(),
	taskUserArray = #(),
	templateArray = #(),
	taskFilepath = "",
	
	taskUIMode = #export,
	
	lastTask = 0,
	lastProcess = 0,
	
	resizeTaskArray = #(),
	
	
		--// MAIN STUFF
	maxHwnd = undefined,
	main = undefined,
	mainSave = undefined,
	_ = undefined,
	controlArray = #(),
	mainOpacity = 1,
	
	dialogPos = [300,300],
	dialogSize = [678,388],
	resetTaskSize = [678,388],
	scriptPath = "",
		
	characterFileRevision = 1.0,
	importFileRevision = 1.0,
	exportFileRevision = 1.0,
	userFileRevision = 1.1,
	importUserFileRevision = 1.0,
	exportUserFileRevision = 1.0,
	
	rootProjectPath = "",
	rootSourcePath = "",
	rootGamePath = "",
	rootCBAPath = "",
	
	characterArray = #(),
	folderArray = #(),
	animArray = #(),
	
	showSubFolders = false,
	filterFullPath = true,
	
	importArray = #(),
	importUserArray = #(),
	exportArray = #(),
	exportUserArray = #(),
	importTemplateArray = #(),
	exportTemplateArray = #(),
	
	recentHistoryArray = #(),
	setupPresetArray = #(),
	setupPresetRecordArray = #(),
	setupPresetRecord = false,
	
	taskMode = #none,
	characterMode = #none,
	
	copyCharacter = #(),
	copySetup = #(),
	copySourceNode = #(),
	copyGameBone = #(),
	copyCondition = #(),
	
	copyTask = #(),
	copyProcess = #(),
	
	
	conditionArray = #( \
			"string_animName", \
			"string_animPath", \
			"string_animDir", \
			"string_animExtension", \
			"string_selectionName", \
			"int_animEntrySelection", \
			"int_animEntryAmount" \
	),
	
	
	variableArray = #( \
			"cryMaxTools.basic.str.getFilename cryAM.currentQue", \
			"cryAM.currentQue", \
			"cryMaxTools.basic.str.getDirectory cryAM.currentQue", \
			"cryMaxTools.basic.str.getExtension cryAM.currentQue", \
			"cryAM.animArray[((cryAM._.lvNodes.selectedItems.item 0).index + 1)].name", \
			"(cryAM._.lvNodes.selectedItems.item 0).index + 1", \
			"cryAM._.labAmountMax.text" \
	),
	
		
	stateArray = #( \
			"has", \
			"HAS", \
			"==", \
			"!=", \
			"<", \
			">", \
			"<=", \
			">=" \
	),
	
	
	groupArray = #( \
			"and", \
			"or", \
			"not and", \
			"not or" \
	),
	
	
	characterSettingsPath = "",
	importSettingsPath = "",
	importUserSettingsPath = "",
	exportSettingsPath = "",
	exportUserSettingsPath = "",
	userSettingsPath = "",
	
	currentQue = "",
	exportAnims = #(),
	usedSkel = undefined,
	
	getLatestCharModel = true,
	cancelContinue = false,
	
	
	lastExportedAnim = "",
	lastImportedAnim = "",
	lastLoadedAnim = 0,
	lastHoveredNodeRow = -1,
	lastHoveredNodeColumn = -1,
	lastHoveredNodeBMP = undefined,
	lastSel = #(),
	lastDraggedEntry = -1,
	lastHoveredEntry = -1,
	lastCharacter = 0,
	lastSetup = 0,
	prevCharacter = #(),
	prevSetup = #(),
	lastFolderSel = undefined,
	
	showScript = false,
	showTaskSettings = false,
	showLog = false,
	showProjectInfo = false,
	
	oneClickLoad = false,
	animPreview = false,
	showSubAnims = false,
	showCBADetails = false,
	resizeSizeArray = #(),
	
	
	logFilePath = "",
	showFullLog = false,
	
	toolTipMode = #none,
	toolTip = undefined,
	labPreview = dotNetObject "System.Windows.Forms.Label" visible:false,
	currentPic = undefined,
	mousePos = [0,0],
	
	CBAArray = #(),
	
	logStr = "",
	advancedLogStr = "",
	
	UIUpdateRO = undefined,
	UIProcessRO = undefined,
	processRightMouseClickRC = undefined,
	rightMouseClickNodesRC = undefined,
	
	DLUText = "",
	RCText = "",
	CBAText = "",
	CHRText = "",
	
	onUpdate = false,
	
	infoBackBMP = bitmap 50 50 color:(color 1 1 1),
	animStyle = #(),
	animStyleExtra = #(),
	
	folderClickNode = undefined,



	function getTimeDate str =
	(
		local timeDateFilter = (filterString str " ")
		local dateVar = filterString timeDateFilter[1] "."
		local timeStr = ""
		local dateStr = ""
		
		if dateVar.count == 1 then
		(
			dateVar = filterString timeDateFilter[1] "/"
			local tempVar = dateVar[1]
			dateVar[1] = dateVar[2]
			dateVar[2] = tempVar
		)
		
		
		local timeVar = filterString timeDateFilter[2] ":"
		
		for i = dateVar.count to 1 by -1 do
		(
			try 
			(
				classOf (execute dateVar[i])
					
			)catch(deleteItem dateVar i)
		)
		
		
		
		for i = timeVar.count to 1 by -1 do
		(
			try 
			(
				classOf (execute timeVar[i])
					
			)catch(deleteItem timeVar i)
		)
		
		
		for i = 1 to dateVar.count do
			dateStr += dateVar[i] + (if i < dateVar.count then "." else "")
		
		for i = 1 to timeVar.count do
			timeStr += timeVar[i] + (if i < timeVar.count then ":" else "")
		
		--gc light:true
		
		return (#(dateStr, timeStr))
	),
	
	
	
	function getDayDifference date1 date2 =
	(
		local date1Filter = filterString date1 "."
		local date2Filter = filterString date2 "."
		local date1Count = (((execute date1Filter[3]) - 1) * 365) + (((execute date1Filter[2]) - 1) * 31) + (execute date1Filter[1])
		local date2Count = (((execute date2Filter[3]) - 1) * 365) + (((execute date2Filter[2]) - 1) * 31) + (execute date2Filter[1])
		
			--// date1 is older than date2
		return (date1Count - date2Count)
	),

	
	
	
	
	function updateCBAInfo =
	(
		CBAArray = #()
		
		if rootCBAPath != "" then
		(
			if doesFileExist rootCBAPath == true then
			(
				local xmlFile = dotNetObject "system.xml.xmlDocument"
				
				xmlFile.load rootCBAPath
				
				local fileRoot = xmlFile.documentElement
				
				for i = 0 to (fileRoot.childNodes.count - 1) do
				(
					case fileRoot.childNodes.itemOf[i].name of
					(
						"AnimationDefinition":
						(
							local animDefRoot = fileRoot.childNodes.itemOf[i]
							
							local modelPath = ""
							local animationPath = ""
							local compression = -1
							local autoCompression = -1
							local preset = ""
							local dbaPath = ""
							local locator = false
							
							local propertyArray = #()
							
							for f = 0 to (animDefRoot.childNodes.count - 1) do
							(
								local entryRoot = animDefRoot.childNodes.itemOf[f]
								
								case entryRoot.name of
								(
									"Animation":
									(
										try
										(
											animationPath = entryRoot.attributes.itemOf[0].value
											if animationPath[animationPath.count] != "/" or animationPath[animationPath.count] != "\\" then
												animationPath += "/"
										)
										catch()
									)
									"SpecialAnimsList":
									(
										local specialAnimPath = "SpecialAnimsList"
										local specialAnimProperties = #()
										
										for d = 0 to (entryRoot.childNodes.count - 1) do
										(
											local animRoot = entryRoot.childNodes.itemOf[d]
											
											case animRoot.name of
											(
												"Animation":
												(
													local propArray = #()
													
													for t = 0 to (animRoot.attributes.count - 1) do
														append propArray (cryAM_propertiesStruct name:animRoot.attributes.itemOf[t].name value:animRoot.attributes.itemOf[t].value)
													
													append specialAnimProperties (cryAM_propertiesStruct name:"SpecialAnimation" value:propArray)
												)
											)
										)
										
										if specialAnimProperties.count > 0 then
											append propertyArray (cryAM_propertiesStruct name:specialAnimPath value:specialAnimProperties)
									)
									
									default:	try append propertyArray (cryAM_propertiesStruct name:entryRoot.name value:entryRoot.attributes.itemOf[0].value) catch()
								)
							)
							
							insertItem (cryAM_propertiesStruct name:"Animation" value:animationPath) propertyArray 1
							
							
							for h = 1 to propertyArray.count do
							(
								if classOf propertyArray[h].value != Array then
								(
									if findString propertyArray[h].value "/" != undefined then
									(
										local tempFilter = filterString propertyArray[h].value "/"
										local newValue = ""
										for d = 1 to tempFilter.count do
											newValue += tempFilter[d] + (if d < tempFilter.count then "\\" else "")
									
										propertyArray[h].value = newValue
									)
								)
							)
							
							append CBAArray propertyArray
						)
					)
				)
			)
		)
	),
	
	
	
	
	
	function getCBASettings filepath =
	(
		local settingArray = #()
		
		for i = 1 to CBAArray.count do
		(
			if findString filepath CBAArray[i][1].value != undefined then
			(
				foundEntry = i
				
				for d = 1 to CBAArray[i].count do
				(
					if CBAArray[i][d].name != "SpecialAnimsList" then
						append
				)
				
				exit
			)
		)
			
			
		if foundEntry > 0 then
		(	
			local count = CBAArray[i].count
			
			if CBAArray[i][count].name == "SpecialAnimsList" then
			(
				for d = 1 to CBAArray[i][count].value.count do
				(
					if findString filepath CBAArray[i][count].value[d][1].value != undefined then
					(
						
					)
				)
			)
		)
		
		
		
	),
	
	
	
	
	
	
	function updateBuildInfo =
	(
		if rootProjectPath != "" then
		(
			local changesPath = rootProjectPath + "Code_Changes.txt"
			local RCPath = rootProjectPath + "Bin32\\RC\\rc.exe"
			
			local DLUDir = (getDir #maxroot) + "Plugins\\"
			local DLUFiles = getFiles (DLUDir + "CryExport*.dlu")
			local DLUName = ""
			local DLUMaxModDate = ""
			
			
			if DLUFiles.count > 0 then
			(
				DLUName = cryMaxTools.basic.str.getFilename DLUFiles[1]
				DLUMaxModDate = getTimeDate (getFileModDate DLUFiles[1])
			)
			
			local DLUPath = ""
			if DLUName != "" then
				DLUPath = rootProjectPath + "Tools\\" + DLUName
			
			
			local codeModDate = ""
			
				--// CODE CHANGES \\--
			
			if doesFileExist changesPath then
			(
				local tempStream = openFile changesPath mode:"r"
				while (eof tempStream) == false do
				(
					local tempLine = readLine tempStream
					if findString tempLine "Changes in Build" != undefined then
					(
						local tempStr = (filterString tempLine "Changes in Build ")[2]
						tempStr = (filterString tempStr "---")[1]
						
						cryAM._.labProjectInfoBuild.text = tempStr
						
						exit
					)
				)
				
				close tempStream
				
				codeModDate = getTimeDate (getFileModDate changesPath)
				
				cryAM._.labProjectInfoBuildDate.text = "Modified      " + codeModDate[1] + "    " + codeModDate[2]
			)
			
			
			
				--// RC \\--
			
			cryAM._.labProjectInfoRC.ForeColor = cryAM._.labProjectInfoRC.ForeColor.fromARGB 255 0 0
			RCText = "Found no RC.exe!"
			
			if doesFileExist RCPath and codeModDate != "" then
			(
				local rcModDate = getTimeDate (getFileModDate RCPath)
				
				local dayDifference = getDayDifference rcModDate[1] codeModDate[1]
				

				if dayDifference != 0 then
				(
					cryAM._.labProjectInfoRC.ForeColor = cryAM._.labProjectInfoRC.ForeColor.fromARGB 200 200 0
					
					if dayDifference < 0 then
						RCText = "RC.exe is older than the Build  (" + (dayDifference * -1) as String + " day" + (if dayDifference < -1 then "s" else "") + ")"
					else
						RCText = "RC.exe is newer than the Build  (" + (dayDifference) as String + " day" + (if dayDifference > 1 then "s" else "") + ")"
				)
				else
				(
					cryAM._.labProjectInfoRC.ForeColor = cryAM._.labProjectInfoRC.ForeColor.fromARGB 0 150 0
					RCText = "Found RC.exe and it is up-to-date"
				)
			)
			
			
				--// DLU \\--
			
			cryAM._.labProjectInfoDLU.ForeColor = cryAM._.labProjectInfoDLU.ForeColor.fromARGB 255 0 0
			DLUText = "Found no Export .dlu!"
			
			if doesFileExist DLUPath and DLUMaxModDate != "" then
			(
				local DLUToolsModDate = getTimeDate (getFileModDate DLUPath)
				
				local dayDifference = getDayDifference DLUMaxModDate[1] DLUToolsModDate[1]
				
				if dayDifference != 0 then
				(
					cryAM._.labProjectInfoDLU.ForeColor = cryAM._.labProjectInfoDLU.ForeColor.fromARGB 200 200 0
					
					if dayDifference < 0 then
						DLUText = DLUName + " is older than Build\\Tools\\" + DLUName +  "  (" + (dayDifference * -1) as String + " day" + (if dayDifference < -1 then "s" else "") + ")"
					else
						DLUText = DLUName + " is newer than Build\\Tools\\" + DLUName +  "  (" + (dayDifference) as String + " day" + (if dayDifference > 1 then "s" else "") + ")"
				)
				else
				(
					cryAM._.labProjectInfoDLU.ForeColor = cryAM._.labProjectInfoDLU.ForeColor.fromARGB 0 150 0
					DLUText = "Found " + DLUName + " and it is up-to-date"
				)
			)
			
			
				--// CBA \\--
			
			cryAM._.labProjectInfoCBA.ForeColor = cryAM._.labProjectInfoCBA.ForeColor.fromARGB 255 0 0
			CBAText = "Found no Animations.cba!"
			rootCBAPath = ""
			
			CBAArray = #()
			
			if rootProjectPath != "" and rootGamePath != "" then
			(
				local checkPath = rootProjectPath + rootGamePath
				local tempFilter = filterString checkPath "\\"
				local combineString = ""
				local foundPath = ""
				local foundEntry = 0
				
				for i = 1 to tempFilter.count do
				(
					combineString += tempFilter[i] + "\\"
					if doesFileExist (combineString + "Animations.cba") then
						foundPath = combineString
				)
				
				if foundPath != "" then
				(
					rootCBAPath = foundPath + "Animations.cba"
					CBAText = "Found Animations.cba in: " + foundPath
				
					
					updateCBAInfo()
					
					
				
					for i = 1 to CBAArray.count do
					(
						if findString rootGamePath CBAArray[i][1].value != undefined then
						(
							foundEntry = i
							exit
						)
					)
					
					if foundEntry > 0 then
					(
						CBAText += "\n\nFound Animation Definition:\n"--\nModel Path = " + CBAArray[foundEntry].modelPath + "\nAnimation Path = " + CBAArray[foundEntry].animationPath + "\nCompression = " + CBAArray[foundEntry].compression as String + "\nAuto-Compression = " + CBAArray[foundEntry].autoCompression as String + "\nPreset = " + CBAArray[foundEntry].preset + "\nDBA Path = " + CBAArray[foundEntry].dbaPath + "\nLocator = " + CBAArray[foundEntry].locator as String
						
						for t = 1 to CBAArray[foundEntry].count do
						(
								--// go through the special animation list
							if CBAArray[foundEntry][t].name == "SpecialAnimsList" then
							(
								CBAText += "\n\nSpecialAnimList:"
									
								for h = 1 to CBAArray[foundEntry][t].value.count do
								(
									CBAText += "\n    Animation:  "
									
									for k = 1 to CBAArray[foundEntry][t].value[h].value.count do
										CBAText += CBAArray[foundEntry][t].value[h].value[k].name + " =  " + CBAArray[foundEntry][t].value[h].value[k].value + "     "
								)
							)
							else
								CBAText += "\n" + CBAArray[foundEntry][t].name + "   =   " + CBAArray[foundEntry][t].value
						)
						
						cryAM._.labProjectInfoCBA.ForeColor = cryAM._.labProjectInfoCBA.ForeColor.fromARGB 0 150 0
					)
					else
					(
						CBAText += "\nCould not find Animation Definition!"
						cryAM._.labProjectInfoCBA.ForeColor = cryAM._.labProjectInfoCBA.ForeColor.fromARGB 200 200 0
					)
				)
			)
			
			
				--// CHR \\--
			
			cryAM._.labProjectInfoCHR.ForeColor = cryAM._.labProjectInfoCHR.ForeColor.fromARGB 255 0 0
			CHRText = "Found no character model!"
			
			if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex > -1 then
			(
				if characterArray[cryAM._.cbCharacter.selectedIndex] != undefined then
				(
					if characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1] != undefined then
					(
						local maxModelPath = characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].model.path
						if maxModelPath != "" then
						(
							maxModelPath = rootProjectPath + maxModelPath
							
							if doesFileExist maxModelPath then
							(
								local fileStats = cryMaxTools.basic.perforceMan.getFileStats maxModelPath
								if fileStats != undefined then
								(
									try
									(
										if fileStats.haveRev != undefined and fileStats.headRev != undefined then
										(
											if fileStats.haveRev < fileStats.headRev then
											(
												cryAM._.labProjectInfoCHR.ForeColor = cryAM._.labProjectInfoCHR.ForeColor.fromARGB 200 200 0
												CHRText = maxModelPath + "\n\nOlder than in Perforce"
											)
											else
											(
												cryAM._.labProjectInfoCHR.ForeColor = cryAM._.labProjectInfoCHR.ForeColor.fromARGB 0 150 0
												CHRText = maxModelPath + "\n\nUp-to-Date"
											)
										)
									)catch()
								)
							)
						)
					)
				)
			)
			
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	function getTimeEstimate indexArray curIndex timeForFirstAnim =
	(
		local maxFileSize = 0.0
		local firstFileSize = 0.0
		
		if doesFileExist animArray[indexArray[1]].path then
			firstFileSize = getFileSize animArray[indexArray[1]].path
		
		if firstFileSize > 0.0 then
		(
			for i = curIndex to indexArray.count do
			(
				local index = indexArray[i]
				
				if doesFileExist animArray[index].path then
					maxFileSize += getFileSize animArray[index].path
			)
		)
		
		return (timeForFirstAnim * maxFileSize / firstFileSize)
	),
	
	
	
	
	
	
	
	
	function callUIUpdateDialog =
	(
		if cryAM.main != undefined then
		(
		
			local isOpen = false
			
			try isOpen = UIUpdateRO.open catch()
			
			if isOpen == false then
			(
				rollout UIUpdateRO "update"
				(
					label labUpdate1 "A n i m M a n" pos:[10,3]
					label labUpdate2 "U P D A T E" pos:[13,18]
					progressbar pbUpdate "" pos:[-2,23+15] width:84 height:7 color:green
				)
				cryAM.UIUpdateRO = UIUpdateRO
				
				if startUp == undefined then
					createDialog cryAM.UIUpdateRO 80 45 (cryAM.main.left + (cryAM.main.width / 2) - (80 / 2)) (cryAM.main.top + (cryAM.main.height / 2) - (45 / 2)) style:#(#style_sunkenedge)
				else
					createDialog cryAM.UIUpdateRO 80 45 style:#(#style_sunkenedge)
			)
		)
	),
	
	
	
	
	
	
	
	function callUIProcessDialog =
	(
		try destroyDialog UIProcessRO catch()
		
		rollout UIProcessRO "process"
		(
			local tempTime = 0
			
			label labCurrent "Current :" pos:[5,5] width:60
			label labCurrentAnim "---" pos:[75,5] width:250 height:18
			progressbar pbCurrent "" pos:[3,23] width:355 height:10 color:green
			label labAmountSel "0" pos:[318+50,20]
			
			label labNext "Next :" pos:[5,35] width:60
			label labNextAnim "---" pos:[75,35] width:360 height:18
			
			progressbar pbOverall "" pos:[5,60] width:355 height:10 color:red
			label labAmountAll "0" pos:[318+50,58]
			
			label labEstimate "Estimate:  00 : 00 : 00" pos:[8,83] width:120
			label labElabsed "Elabsed:  00 : 00 : 00" pos:[5+140,83] 
			
			
			button btnCancel "C a n c e l" pos:[120+120+50,80] width:100 height:18
			
			
			timer tmTimer "" interval:1000 active:true
			
			on tmTimer tick do
			(
				tempTime += 1
				local seconds = tempTime
				
				local minute = (seconds / 60) as Integer
				seconds = (mod seconds 60) as Integer
					
				local hour = (minute / 60) as Integer
				minute = (mod minute 60) as Integer
				
				local timeString = (if hour < 10 then "0" else "") + hour as String + " : " + (if minute < 10 then "0" else "") + minute as String + " : " + (if seconds < 10 then "0" else "") + seconds as String
				
				labElabsed.text = "Elabsed:  " + timeString
			)
			
			
			
			
			
			on btnCancel pressed do
			(
				try destroyDialog UIProcessRO catch()
				
				cryAM.cancelContinue = true
			)
		)
		
		cryAM.UIProcessRO = UIProcessRO
		
		
		createDialog cryAM.UIProcessRO 400 105 (cryAM.main.left + (cryAM.main.width / 2) - (400 / 2)) (cryAM.main.top + (cryAM.main.height / 2) - (105 / 2)) style:#(#style_sunkenedge)
	),
	
	
	
	
	
	
	function updateLog =
	(
		local tempLog = logStr
		if showFullLog == true then
			tempLog = advancedLogStr
		
		local tempFilter = filterString tempLog "\n"
		
		cryAM._.cbLog.items.clear()
		
		for i = 1 to tempFilter.count do
			cryAM._.cbLog.items.Add tempFilter[i]
		
		if tempFilter.count > 0 then
			cryAM._.cbLog.text = tempFilter[tempFilter.count]
		else
			cryAM._.cbLog.text = ""
	),
	
	
	
	
	
	
	function addLog str addSep:false isAdvanced:false advancedLogPart:"" =
	(
		local preStr = " Log  "
		local tempStr = ""
		local sep = "-------------------------------------------------------------------------------------------------------------------------------------------------------------"
		local newLine = (if str != "" then "\n" else "")
		
		local logStream = undefined
		
		if doesFileExist logFilePath then
			logStream = openFile logFilePath mode:"a+"
		else
			logStream = createFile logFilePath
		
		
		local timeDate = getTimeDate localTime
		
		timeDateStr = "[ " + timeDate[1] + "  " + timeDate[2] + " ]"
		
		
		
		
		if addSep == true then
		(
			if isAdvanced == false or showFullLog == true then
				cryAM._.cbLog.items.add (sep as String)
			
			sep += "\n"
		)
		else
			sep = ""
		
		tempStr = "  [ " + timeDate[2] + " ] :        " + str
		
		if str != "" then
		(
			if isAdvanced == false or showFullLog == true then
				cryAM._.cbLog.items.add (tempStr)
			
			--tempStr += timeDateStr + " :    " + str + newLine
			
		)
		
		cryAM._.cbLog.text = tempStr
		
		
		
		tempStr = sep + tempStr
		
		if advancedLogPart != "" then
			advancedLogPart = "     (" + advancedLogPart + ")"
		
		local outputStr = sep + "[ " + timeDate[1] + "  " + timeDate[2] + " ] :        " + str + advancedLogPart + newLine
		
		
		
		if isAdvanced == false then
			logStr += tempStr + newLine
		
		advancedLogStr += tempStr + advancedLogPart + newLine
		
		format (outputStr) to:logStream
		
		close logStream
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	function extractPreview entry =
	(
		labPreview.image = undefined
		labPreview.visible = false
		
		if currentPic != undefined then
			currentPic.Dispose()
		
		
		
		
		
		try deleteFile @"C:\temp_preview.bmp" catch()
		
		if entry == undefined then
			return undefined
			
		if entry.type != "bip" and entry.type != "max" then
			return undefined
		
		
		local filepath = entry.path
		local ext = cryMaxTools.basic.str.getExtension filepath
		
		
		
		
		
		
		
		local picPath = ""
		local startIndicator = #()
		local picIndicator = #()
		
		local picStart = 0
		local picEnd = 0
		local foundStart = 0
		local found = 1
		
		
		if not (doesFileExist filepath) then
			return undefined
		
			--// open the file to read binary
		local binFile = fopen filepath "rb"
		
		case ext of
		(
			"bip":
			(
				picPath = @"C:\temp_preview.tga"
				startIndicator = #(10,0,0,0,0,0,0)  --// (Start of TGA)
				picIndicator = #(88, 70,73,76,69)  --// XFILE as integer (End of TGA)
			)
			"max":
			(
				picPath = @"C:\temp_preview.bmp"
				picIndicator = #(255, 255,255,255,3,0,0,0,1,0,128)  --// FF FF FF FF 03 00 00 00 01 00 80 as integer
			)
		)
		
		
			--// create saveTo file if not available and closes it afterwards
		if not (doesFileExist picPath) then
			close (createFile picPath)
			
			--// open saveTo file binary
		local binSaveFile = fopen picPath "wb"
		
		
			--// retrieve the end of file
		fseek binFile 0 #seek_end
		local endFile = ftell binFile
		
			
		
		
		case ext of
		(
			"bip":
			(
				fseek binFile 0 #seek_set
				
				while (ftell binFile < endFile - 1) and (ftell binFile < 1000) do
				(
							--// checks, if the last indicator is found
					if found <= startIndicator.count then
					(
							--// comapres the indexed indicator with the current readed char
						if startIndicator[found] == (readByte binFile #unsigned) then
							--// increase indicator to the next char
							found += 1
							--// otherwise reset, to start from the beginning
						else
							found = 1
					)
					else
					(
							--// set the TGA ending
						picStart = ftell binFile
						exit
					)
				)
				
				fseek binFile 14000 #seek_set
		
		
				if picStart == 0 then
				(
					fclose binFile
					fclose binSaveFile
					
					try deleteFile @"C:\temp_preview.tga" catch()
					return undefined
				)
			)
			
			"max":
			(
					--// rough gessing of the image start (in max it is located at the end of the file)
				fseek binFile -100000 #seek_end
			)
		)
			
		found = 1
		
		while ftell binFile < endFile - 1 do
		(
					--// checks, if the last indicator is found
			if found < picIndicator.count then
			(
					--// comapres the indexed indicator with the current readed char
				if picIndicator[found] == (readByte binFile #unsigned) then
					--// increase indicator to the next char
					found += 1
					--// otherwise reset, to start from the beginning
				else
					found = 1
			)
			else
			(
					--// set the TGA ending
				foundStart = ftell binFile
				exit
			)
		)
		
		case ext of
		(
			"bip":
			(
				picEnd = foundStart + 3
				picStart -= 9
			)
			
			"max":
			(
				if foundStart == 0 then
				(
					fclose binFile
					fclose binSaveFile
					
					try deleteFile @"C:\temp_preview.bmp" catch()
					return undefined
				)
				
				picEnd = endFile
				picStart = foundStart + 38
			)
		)
		
		
		fseek binFile picStart #seek_set
		
		
		while ftell binFile < picEnd - 1 do
			writeByte binSaveFile (readByte binFile #unsigned) #unsigned
		
		if ext == "max" then
		(
				--// moving to the start of the BMP file
			fseek binSaveFile 0 #seek_set
			
				--// new header as the max file had funny stuff in it
			local addedValues = #(66,77,0,0,0,0,0,0,0,0,54)
		
				--// writing new header
			for i = 1 to addedValues.count do
				writeByte binSaveFile addedValues[i] #unsigned
		)
		
		
		fclose binFile
		fclose binSaveFile
		
		
		if ext == "bip" then
		(
			try deleteFile @"C:\temp_preview.bmp" catch()
			
			local tempBMP = openBitMap picPath
			if tempBMP != undefined then
			(
				local newBMP = copy tempBMP
				close tempBMP
				
				newBMP.filename = @"C:\temp_preview.bmp"
				
				save newBMP
				close newBMP
				
			)
			
			try deleteFile picPath catch()
		)
		
		if currentPic != undefined then
			currentPic.Dispose()
		
		currentPic = (dotNetClass "System.Drawing.Image").fromFile @"C:\temp_preview.bmp"
		cryAM.labPreview.image = currentPic
		
		--gc light:true
		
		return true
	),
	
	
	
	
	function updateRecentList =
	(
		onUpdate = true
		
		cryAM._.cbAnimRecent.items.Clear()
		
		for i = 1 to recentHistoryArray.count do
		(
			if (cryAM._.cbCharacter.selectedIndex + 1) == recentHistoryArray[i].charID and (cryAM._.cbSetup.selectedIndex + 1) == recentHistoryArray[i].setupID then
				cryAM._.cbAnimRecent.items.Add (cryMaxTools.basic.str.getFilename recentHistoryArray[i].path)
		)
		
		if cryAM._.cbAnimRecent.items.count > 0 then
			cryAM._.cbAnimRecent.selectedIndex = 0
		
		onUpdate = false
	),
	
	
	
	
	
	
	function updateSetupList =
	(
		onUpdate = true
		
		cryAM._.cbSetup.items.Clear()
		
		if cryAM._.cbCharacter.selectedIndex > 0 then
			for i = 1 to characterArray[cryAM._.cbCharacter.selectedIndex].setup.count do
				cryAM._.cbSetup.items.Add ("  " + characterArray[cryAM._.cbCharacter.selectedIndex].setup[i].name)
		
		if cryAM._.cbSetup.items.count > 0 then
			cryAM._.cbSetup.selectedIndex = 0
			
		onUpdate = false
	),
	
	
	
	
	

	
	
	
	
	function updateCharacterList skipSetupList:false =
	(
		onUpdate = true
		
		cryAM._.cbCharacter.items.Clear()
		cryAM._.cbCharacter.items.Add "   ---   Scene   ---"
		
		for i = 1 to characterArray.count do
			cryAM._.cbCharacter.items.Add ("  " + characterArray[i].name)
		
		if cryAM._.cbCharacter.items.count == 1 then
			cryAM._.cbCharacter.selectedIndex = 0

		onUpdate = false
		
		if skipSetupList == false then
			updateSetupList()
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	function dragList orderList orderArray arg =
	(	
		local hitTestItem = (orderList.hitTest arg.x arg.y).item
		if lastDraggedEntry != hitTestItem and hitTestItem != lastHoveredEntry then
		(
			if hitTestItem != undefined then
			(
				lastHoveredEntry = hitTestItem
				
				local lastID = lastDraggedEntry.index
				local newID = hitTestItem.index
				
				local startOrder = newID
				local stopOrder = lastID
				
				if lastID < newID then
				(
					startOrder = lastID
					stopOrder = newID
				)
				
				if stopOrder > 0 then
					stopOrder -= 1
				
				
				for i = stopOrder to startOrder by -1 do
				(
					local tempArrayEntry = orderArray[i+2]
					orderArray[i+2] = orderArray[i+1]
					orderArray[i+1] = tempArrayEntry
					
					local tempListName = #()
					
					for f = 0 to ((orderList.items.item (i + 1)).subItems.count - 1) do
						append tempListName ((orderList.items.item (i + 1)).subItems.item f).text
					
					--local tempListName = (orderList.items.item (i + 1)).text
					local tempListChecked = (orderList.items.item (i + 1)).checked
						
					for f = 0 to ((orderList.items.item (i + 1)).subItems.count - 1) do
						((orderList.items.item (i + 1)).subItems.item f).text = ((orderList.items.item (i)).subItems.item f).text
							
					(orderList.items.item (i + 1)).checked = (orderList.items.item (i)).checked
						
					for f = 0 to ((orderList.items.item (i)).subItems.count - 1) do
						((orderList.items.item (i)).subItems.item f).text = tempListName[(f + 1)]
						
					(orderList.items.item i).checked = tempListChecked
				)
				
				for i = 0 to (orderList.items.count - 1) do
					(orderList.items.item i).selected = false
				
				lastDraggedEntry = orderList.items.item newID
				lastDraggedEntry.selected = true
			)
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	
	function callSetupRO =
	(
		fileIn "AnimManager_setup.ms"
	),
	
	
	function callMapNodes =
	(
		fileIn "AnimManager_mapper.ms"
	),
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function updateHistory cbTemp op useString:false skip:0 =
	(
		local tempString = cbTemp.text
		
		if useString != false then
			tempString = cryMaxTools.basic.str.getFilename useString
		
		case op of
		(
			#set:
			(
				onUpdate = true
				
				local foundEntry = -1
				local entryIndex = -1
				
				for i = 0 to (cbTemp.items.count - 1) do
				(
					if (cbTemp.items.item i) == tempString then
					(
						foundEntry = i
						exit
					)
				)
				
				if foundEntry > -1 then
					try cbTemp.SelectedIndex = foundEntry catch()
				else
				(
					if useString != false then
						insertItem (cryAM_recentStruct charID:(cryAM._.cbCharacter.selectedIndex + 1) setupID:(cryAM._.cbSetup.selectedIndex + 1) path:useString) recentHistoryArray (1+skip)
					
					try cbTemp.items.insert (skip) tempString catch()
				
					if cbTemp.items.count > 10 then
					(
						try cbTemp.items.removeAt (cbTemp.items.count - 1) catch()
						
						if useString != false then
						(
							local charArray = #()
							for i = 1 to recentHistoryArray.count do
							(
								if recentHistoryArray[i] != undefined then
								(
									if recentHistoryArray[i].charID == (cryAM._.cbCharacter.selectedIndex + 1) and recentHistoryArray[i].setupID == (cryAM._.cbSetup.selectedIndex + 1) then
										append charArray i
								)
							)
							
							deleteItem recentHistoryArray charArray[charArray.count]
						)
					)
					
					for i = 0 to (cbTemp.items.count - 1) do
						if (cbTemp.items.item i) == tempString then
							cbTemp.selectedIndex = i
				)
				
				onUpdate = false
			)
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	
	function checkFolderVisible basePath root:undefined =
	(
		local tempFilter = filterString basePath "\\"
		
		if cryAM._.cbFolderFilter.text != "" and root == undefined then
		(
			if findString cryAM._.cbFolderFilter.text "#_" != undefined then
			(
				if tempFilter[tempFilter.count][1] == "_" then
					return false
			)
			else	if findString tempFilter[tempFilter.count] cryAM._.cbFolderFilter.text == undefined then
				return false
		)
		
		return true
	),
	
	
	
	
	
	
	
	
	function addFolder basePath baseNode root:undefined =
	(
		if basePath != "" then
		(
			local appendArray = #()
			local childFolders = getDirectories (basePath + "*")
			local tempFilter = filterString basePath "\\"
			local newNode = baseNode
			local showNode = cryAM.checkFolderVisible basePath root:root
			
			if root == undefined then
			(
				if showNode == true then
				(
					newNode = baseNode.Nodes.Add tempFilter[tempFilter.count] tempFilter[tempFilter.count]
					append folderArray (cryAM_folderStruct name:tempFilter[tempFilter.count] path:basePath node:newNode)
				)
			)
			
			for i = 1 to childFolders.count do
				addFolder childFolders[i] newNode
		)
	),
	
	
	
	function getFolders folderPath rootArray:undefined =
	(
		if folderPath == "" then
			return (#())
		
		if rootArray == undefined then
			local rootArray = #()
		
		local tempFilter = filterString folderPath "\\"
		local newEntry = cryAM_folderStruct name:tempFilter[tempFilter.count] path:folderPath
		
		append rootArray newEntry
		
		if showSubFolders == true then
		(	
			local subFolders = #()
			local tempFolders = getDirectories (folderPath + "*")
			
			for i = 1 to tempFolders.count do
				if checkFolderVisible tempFolders[i] == true then
					append subFolders tempFolders[i]
			
			for i = 1 to subFolders.count do
				getFolders subFolders[i] rootArray:rootArray
		)
		
		--gc light:true
		
		return rootArray
	),
	
	
	
	
	function getSelectedFolderPath = 
	(
		if folderClickNode != undefined then
		(
			for i = 1 to folderArray.count do
				if folderArray[i].node == folderClickNode then
					return folderArray[i].path
		)
			
		return ""
	),
	
	
	
	
	
	function lowercase input =
	(
		local tempString = ""
		
		for i = 1 to input.count do
		(
			x = input[i]
			n = bit.charAsInt x
			if (n > 64) and (n < 90) then
				x = bit.intAsChar (n + 32)
			tempString += x
		)
		
		--gc light:true
		
		return tempString
	),
	
	
	
	
	
	function findMaskEntry tempMask tempString  =
	(
		local tempArray = #(false, false, false, false)
						
			--// if the normal criteria is not empty
		if tempMask.normal[1] != undefined then
		(
				--// file is part in normal
			tempArray[1] = true
			for i = 1 to tempMask.normal.count do
			(
					--// if the there is a missmatch in the file criteria
				if (findString tempString tempMask.normal[i]) == undefined then
						--// file is not part in normal
					tempArray[1] = false												
			)
		)
			--// normal criteria is empty
		else
				--// file is not part in normal criteria
			tempArray[1] = undefined
								
			--// goes through the addition criteria				
		for i = 1 to tempMask.add.count do
		(
				--// if the criteria is not empty
			if tempMask.add[i] != undefined then
					-- // if the criteria is found in the filename
				if (findString tempString tempMask.add[i]) != undefined then
						--// file is part in addition
					tempArray[2] = true
		)
		
			--// goes through the substraction criteria		
		for i = 1 to tempMask.remove.count do
		(
				--// if the criteria is not empty
			if tempMask.remove[i] != undefined then
					--// if the criteria is found in the filename
				if (findString tempString tempMask.remove[i]) != undefined then
						--// file is part in substraction
					tempArray[3] = true
		)
			--// if there is a extension criteria defined
		if tempMask.extension[1] != undefined then
		(
				--// goes through the extension criteria
			for i = 1 to tempMask.extension.count do
			(
					--// if criteria is not empty
				if tempMask.extension[i] != undefined then
				(
						--// seperates the filepath and the extension
					tempExtension = filterString tempString "."
						--// and gets the extension
					tempExtension = tempExtension[tempExtension.count]
						--// if the extension criteria is found in the file
					if (findString tempExtension tempMask.extension[i]) != undefined then
						--// file is part in extension criteria
						tempArray[4] = true
				)
			)
		)
			--// if there is no extension criteria defined
		else
			tempArray[4] = undefined
		
		--gc light:true
			
			--// return the list of founded criterias																											
		return tempArray
	),
	
	
	
	
	
	function getFileFound filename =
	(
			--// creates an empty mask struct
		local fileMask = cryAM_maskStruct()
			--// seperates all entries in the file mask and stores them as an array
		local fileMaskFilter = filterString cryAM._.cbAnimFilter.text " "
		
		local fileMaskExtension = filterString cryAM._.cbAnimExtension.text ","
			--// end result which will be returned
		local fileMaskFound = false
		
			--// goes through the sperated mask entries
		for f = 1 to fileMaskFilter.count do
		(
				--// if a substract letter is found, put the part of the string into the remove part
			if findString fileMaskFilter[f] "#" != undefined then
				append fileMask.remove (substring fileMaskFilter[f] 2 fileMaskFilter[f].count)
				
				--// if an addition letter is found, put the part of the string into the addition part
			if findString fileMaskFilter[f] "*" != undefined then
				append fileMask.add (substring fileMaskFilter[f] 2 fileMaskFilter[f].count)

				--// if an additon letter and a substract letter is not found, put the part of the string into the normal part
			if (findString fileMaskFilter[f] "*" == undefined) and (findString fileMaskFilter[f] "#" == undefined) and (findString fileMaskFilter[f] "$" == undefined) then
				append fileMask.normal fileMaskFilter[f]
				
				--// if a file type letter is found, put the part of the string into the extension part
			--if findString fileMaskFilter[f] "$" != undefined then												
				--append fileMask.extension (lowercase (substring fileMaskFilter[f] 2 fileMaskFilter[f].count))
		)
		
		for f = 1 to fileMaskExtension.count do
			fileMaskExtension[f] = lowercase (filterString fileMaskExtension[f] " ")[1]
		
		fileMask.extension = fileMaskExtension


			--// gets the result, if the file is part of the substract, add, normal or extension criteria
		local maskArray = findMaskEntry fileMask filename


			--// if the file extension is found
		if maskArray[4] != false then
		(
				--// if the file is part in normal or is not specified then file is found
			if (maskArray[1] == true or maskArray[1] == undefined) then
					--// file is found
				fileMaskFound = true
			else
					--// otherwise not
				fileMaskFound = false
			
				--// if the file is part in substraction
			if maskArray[3] == true then
					--// file is hidden
				fileMaskFound = false
				--// if the file is part in addition
			if maskArray[2] == true then
					--// file is found
				fileMaskFound = true
		)
			--// if the file extension is not found
		else
				--// file is hidden
			fileMaskFound = false
		
		--gc light:true
			
			--// returns the result if the file is visible or not
		return fileMaskFound
	),
	
	
	
	
	
	
	
	
	function updateRootPathsUI =
	(
			--// Project Text update \\--
		if cryAM._.edProjectPath.text != "" then
			cryAM._.edProjectPath.width = (getTextExtent cryAM._.edProjectPath.text)[1] + 12
		else
			cryAM._.edProjectPath.width = 200
		
		if cryAM._.edProjectPath.width < 120 then
			cryAM._.edProjectPath.width = 120
		
		cryNET.setLoc cryAM._.edSourcePath [(cryNET.getLoc cryAM._.edProjectPath)[1] + cryAM._.edProjectPath.width + 5, (cryNET.getLoc cryAM._.edSourcePath)[2]]
		cryAM._.edSourcePath.width = (cryNET.getLoc cryAM._.btnBrowseSourcePath)[1] - (cryNET.getLoc cryAM._.edSourcePath)[1] - 3
		
		cryNET.setLoc cryAM._.edGamePath [(cryNET.getLoc cryAM._.edProjectPath)[1] + cryAM._.edProjectPath.width + 5, (cryNET.getLoc cryAM._.edGamePath)[2]]
		cryAM._.edGamePath.width = (cryNET.getLoc cryAM._.btnBrowseGamePath)[1] - (cryNET.getLoc cryAM._.edGamePath)[1] - 3
		
		cryAM._.gbProject.width = (cryNET.getLoc cryAM._.edGamePath)[1] - (cryNET.getLoc cryAM._.gbProject)[1] - 3
		cryAM._.labProjectInfoBack.width = (cryNET.getLoc cryAM._.edGamePath)[1] - (cryNET.getLoc cryAM._.labProjectInfoBack)[1] - 5
		
		updateBuildInfo()
	),
	
	
	
	
	
	
	
	
	
	
	
	function updateAnimListMenu menu =
	(
		if menu != undefined then
		(
			try menu.miShowSubFolders.checked = cryAM.showSubFolders catch()
			
			
			local selCount = 0
			
			for i = 1 to animArray.count do
			(
				if animArray[i].selected == true then
					selCount += 1
			)
			
			menu.miLoadAnim.enabled = false
			menu.miSaveAnim.enabled = false
			menu.miSaveAnimAs.enabled = false
			
			if selCount > 0 then
			(
				menu.miLoadAnim.enabled = true
				menu.miSaveAnim.enabled = true
				menu.miSaveAnimAs.enabled = true
			)
			
			if selCount > 1 then
			(
				menu.miLoadAnim.enabled = false
				menu.miSaveAnimAs.enabled = false
			)
		
			
			menu.miOneClickLoad.checked = oneClickLoad
			menu.miUsePerforceLoad.checked = cryMaxTools.anim.vars.usePerforceLoad
			menu.miUsePerforceSave.checked = cryMaxTools.anim.vars.usePerforceSave
			menu.miAnimPreview.checked = animPreview
			menu.miFilterFullPath.checked = filterFullPath
			menu.miShowSubAnims.checked = showSubAnims
			menu.miShowCBADetails.checked = showCBADetails
		)
		
		return true
	),
	
	
	
	
	
	
	
	function updateAnimStatus =
	(
		local outOfRangeArray = #()
		local sceneNodeFound = false
		
		local selArray = #{}
		
		
		for i = 0 to (cryAM._.lvNodes.selectedCells.count - 1) do
		(
			local index = (cryAM._.lvNodes.selectedCells.item i).rowIndex + 1
			if animArray[index] != undefined then
			(
				selArray[index] = true
				
				if animArray[index].type == "scene" then
					sceneNodeFound = true
				
				if animArray[index].type == "folder" then
					append outOfRangeArray animArray[index].node
			)
			else
				append outOfRangeArray ((cryAM._.lvNodes.Rows.item (cryAM._.lvNodes.selectedCells.item i).RowIndex).cells.item 0)
		)
		
		for i = 1 to outOfRangeArray.count do
			outOfRangeArray[i].selected = false
		
		
		
		local maxCount = 0
		local selCount = 0
		
		
		for i = 1 to animArray.count do
		(
			local isSelected = true
			
			if animArray[i].type == "folder" then
				isSelected = false
			
			if sceneNodeFound == true and animArray[i].type != "scene" then
				isSelected = false
			
			if selArray[i] == false and cryAM._.lvNodes.selectedCells.count > 0 then
				isSelected = false
			
			if cryAM._.lvNodes.selectedCells.count == 0 and (animArray.count < 2 or animArray[i].type == "scene")then
				isSelected = false
			
			
			
			if isSelected == true then
				selCount += 1
			
			if animArray[i].type != "scene" and animArray[i].type != "folder" then
				maxCount += 1
			
			animArray[i].selected = isSelected
			
			if sceneNodeFound == true and animArray[i].type != "scene" then
				animArray[i].node.selected = false
		)
		
		if sceneNodeFound == true then
			maxCount += 1
		
		cryAM._.labAmountSel.text = "Sel:    " + selCount as String
		cryAM._.labAmountMax.text = "All:    " + maxCount as String
		
		updateAnimListMenu cryAMMenu
		--gc light:true
	),
	
	
	
	
	function compareNodes node1 node2 =
	(
		try
			local node1Filter = filterString node1.name "\\"
		catch
			local node1Filter = filterString node1 "\\"
		node1 = node1Filter[node1Filter.count]
		try
			local node2Filter = filterString node2.name "\\"
		catch
			local node2Filter = filterString node2 "\\"
		node2 = node2Filter[node2Filter.count]
		
		local nodeLength = node1.count
		if node2.count > node1.count then
			nodeLength = node2.count
		
		
		
		
		local result = 0
		
		for i = 1 to nodeLength do
		(
			if node1[i] != undefined and node2[i] != undefined then
			(
				if node1[i] != node2[i] then
				(
					local intNode1 = bit.charAsInt node1[i]
					local intNode2 = bit.charAsInt node2[i]
					
						
					case of
					(
							--// special signs between Z and a
						(intNode1 > 90 and intNode1 < 97):		intNode1 = intNode1 - 90
							--// special signs lower than chars
						(intNode1 < 65):									intNode1 = intNode1 + 5
							--// special signs higher than chars
						(intNode1 > 122):								intNode1 = intNode1 - 122 + 5 + 64
							--// a -> z
						(intNode1 >= 97 and intNode1 <= 122):	intNode1 = intNode1 - 96 + 134 + 5 + 64
							--// A -> Z
						(intNode1 >= 65 and intNode1 <= 90):	intNode1 = intNode1 - 64 + 134 + 5 + 64
					)
					
					
					
					
					case of
					(
							--// special signs between Z and a
						(intNode2 > 90 and intNode2 < 97):		intNode2 = intNode2 - 90
							--// special signs lower than chars
						(intNode2 < 65):									intNode2 = intNode2 + 5
							--// special signs higher than chars
						(intNode2 > 122):								intNode2 = intNode2 - 122 + 5 + 64
							--// a -> z
						(intNode2 >= 97 and intNode2 <= 122):	intNode2 = intNode2 - 96 +134 + 5 + 64
							--// A -> Z
						(intNode2 >= 65 and intNode2 <= 90):	intNode2 = intNode2 - 64 +134 + 5 + 64
					)
					
					
					case of
					(
						(intNode1 < intNode2): (result = -1; exit)
						(intNode1 > intNode2): (result = 1; exit)
					)
				)
			)
		)
		
		--gc light:true
		
		return result
	),
	
	
	
	
	
	
	
	function getMAXEntries filename =
	(
		local maxArray = #()
		
		if isMaxFile filename then
		(
			local objArray = getMAXFileObjectNames filename quiet:true
			
			local foundExportNode = false
			
			for i = 1 to objArray.count do
			(
				if objArray[i] == "cryAnimationExportNode" then
				(
					foundExportNode = true
					exit
				)
			)
			
			
			if foundExportNode == true then
			(
				local sceneExportNode = undefined
				
				for obj in objects do
				(
					if obj.name == "cryAnimationExportNode" then
					(
						sceneExportNode = obj
						exit
					)
				)
				
				if sceneExportNode != undefined then
					sceneExportNode.name = "___" + sceneExportNode.name
				
				mergeMaxFile filename (#("cryAnimationExportNode")) #mergeDups #useSceneMtlDups #neverReparent quiet:true
				
				local newExportNode = getNodeByName "cryAnimationExportNode"
					
				if newExportNode != undefined then
				(
					cryMaxTools.export.anim.fnGetRanges()
					
					local animRanges = cryMaxTools.export.anim.animRanges
					
					for i = 1 to animRanges.count do
						if animRanges[i].animName != "*current range" then
							append maxArray (cryAM_animStruct name:animRanges[i].animName path:filename type:"anim" start:animRanges[i].start end:animRanges[i].end export:animRanges[i].type)
						
					delete newExportNode
				)
				
				sceneExportNode.name = subString sceneExportNode.name 4 sceneExportNode.name.count
			)
		)
		
		return maxArray
	),
	
	
	
	
	function getXAFEntries filename =
	(
		
		
		
		
		
		
	),
	
	
	
	
	
	
	
	
	
	
	
	function updateDimensions newSize:undefined offset:0 onlyMinimum:false =
	(
		local minimumSize = [0,0]
		local curSize = cryNET.getSize cryAM.main
		
		local taskAdd = 0
		local scriptAdd = 0
		
		if showTaskSettings == true then
		(
			taskAdd = 203
			
			if showScript == true then
				scriptAdd = 408
		)
		
		
		
		local projectAdd = -58
		local nodeProjectAdd = -50
		
		if showProjectInfo == true then
		(
			projectAdd = 0
			nodeProjectAdd = 0
		)
		
		
		
		local logAdd = 0
		if showLog == true then
			logAdd = 25
		
		
		
		
		
		minimumSize[1] = (600 + taskAdd + scriptAdd)
		minimumSize[2] = (220 + logAdd)
		
		cryAM.main.minimumSize = cryNET.getNetSize minimumSize
		
		
		if onlyMinimum == false then
		(
			if showTaskSettings == false then
			(
				--resetTaskHeight = animManRO.height
				resetTaskSize = cryNET.getSize cryAM.main
			)
			
			
				
			
			local widthAdd = curSize[1] - 678 - scriptAdd - taskAdd + offset
			local widthMul2 = ((curSize[1] - 678 - scriptAdd - taskAdd) / 2.0) + offset
			local widthMul3 = ((curSize[1] - 678 - scriptAdd - taskAdd) / 3.0) + offset
			local widthAdd2 = curSize[1] - 678 - scriptAdd + offset
			local heightAdd = curSize[2] - 388 + offset
			local heightAdd2 = curSize[2] - 388 - logAdd + offset
			
			
			
			
			
			
			
			
			
			
			
			
			for i = 1 to resizeSizeArray.count do
			(
				case resizeSizeArray[i].name of
				(
					"pFolders":						resizeSizeArray[i].item.height = resizeSizeArray[i].height + heightAdd2 - logAdd
					"edProjectPath":				resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"edSourcePath":				resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"edGamePath":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					
					"lvFolders":						resizeSizeArray[i].item.height = resizeSizeArray[i].height + heightAdd2
					"lvNodes":
					(
						resizeSizeArray[i].item.height	=	resizeSizeArray[i].height + heightAdd2 - nodeProjectAdd
						resizeSizeArray[i].item.top = resizeSizeArray[i].top + nodeProjectAdd
						
							--// Animation column \\--
						(resizeSizeArray[i].item.columns.item 0).width = resizeSizeArray[i].item.width - 140
						
							--// CBA columns \\--
						for d = 3 to (resizeSizeArray[i].item.columns.count - 1) do
							(resizeSizeArray[i].item.columns.item d).width = (getTextExtent (resizeSizeArray[i].item.columns.item d).name)[1] + 20
					)
					
					
		
		
		
		
		
		
		
		
					
					"gbProject":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"btnLog":						resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + heightAdd2 - logAdd
					"cbLog":							resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + heightAdd2 - logAdd
					
					"pAmount":						resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					
					"btnBrowseProjectPath":	resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"btnBrowseSourcePath":	resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"btnBrowseGamePath":		resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"labRecent":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"labFilter":						resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"labExtension":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"cbAnimRecent":				(resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd ; resizeSizeArray[i].item.width = resizeSizeArray[i].width + widthMul2)
					"cbAnimFilter":					(resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd ; resizeSizeArray[i].item.width = resizeSizeArray[i].width + widthMul2)
					"cbAnimExtension":			resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					
					"btnLoadSet":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"btnSaveSet":					resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd
					"cbLoad":						(resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd ; resizeSizeArray[i].item.width =	resizeSizeArray[i].width + (widthAdd / 2.0))
					"cbSave":						(resizeSizeArray[i].item.top	=	resizeSizeArray[i].top + projectAdd ; resizeSizeArray[i].item.width =	resizeSizeArray[i].width + (widthAdd / 2.0))
				)
			)
		)
		
		
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	function updateNodeList onlyBackground:false keepSelection:false useList:(#()) skipDimensions:false =
	(
		local selFolder = false
		local usedArray = #()
		local foundCBAEntry = 0
		local foundCBASpecialEntry = 0
		local specialEntryStart = 0
		local collectColumnArray = #()
		local foundColumnArray = #{}
		local lastSelAnim = #()
		
		if onlyBackground == false then
		(
			if keepSelection == true then
			(
				if cryAM._.lvNodes.selectedCells.count > 0 then
					for i = 0 to (cryAM._.lvNodes.selectedCells.count - 1) do
						appendIfUnique lastSelAnim (animArray[((cryAM._.lvNodes.selectedCells.item i).RowIndex + 1)].path)
			)
			
			
				--// CBA UPDATE \\--
			if showCBADetails == true then
			(	
				for i = 1 to CBAArray.count do
				(
					if findString characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].gamePath CBAArray[i][1].value != undefined then
					(
						foundCBAEntry = i
						exit
					)
				)
				
				if foundCBAEntry > 0 then
				(
					for i = 3 to CBAArray[foundCBAEntry].count do
					(
						local propEntryName = cryMaxTools.basic.str.lower CBAArray[foundCBAEntry][i].name
						
						if propEntryName != "specialanimslist" then
						(
							if propEntryName != "database" and propEntryName != "preset" and propEntryName != "locomotion_locator" then
								append collectColumnArray (cryMaxTools.basic.str.lower propEntryName)
						)
						else
						(
							foundCBASpecialEntry = i
							specialEntryStart = collectColumnArray.count
							
							for d = 1 to CBAArray[foundCBAEntry][i].value.count do
							(
								for t = 1 to CBAArray[foundCBAEntry][i].value[d].value.count do
								(
									if CBAArray[foundCBAEntry][i].value[d].value[t].name != "APath" then
										if (findItem collectColumnArray (cryMaxTools.basic.str.lower CBAArray[foundCBAEntry][i].value[d].value[t].name)) == 0 then
											append collectColumnArray (cryMaxTools.basic.str.lower CBAArray[foundCBAEntry][i].value[d].value[t].name)
								)
							)
						)
					)
					
					if collectColumnArray.count < cryAM._.lvNodes.columns.count + 3 then
					(
						for i = (cryAM._.lvNodes.columns.count - 1) to (cryAM._.lvNodes.columns.count - collectColumnArray.count + 1) by -1 do
							try cryAM._.lvNodes.columns.removeAt i catch()
					)
					
					cryAM._.lvNodes.columnCount = collectColumnArray.count + 3
					
					for i = 1 to collectColumnArray.count do
					(
						local columnName = copy collectColumnArray[i]
						columnName[1] = cryMaxTools.basic.str.upper columnName[1]
						(cryAM._.lvNodes.columns.item (i + 2)).name = columnName
						
							
						
						if i <= specialEntryStart then
						(
							/*
							local defaultCellStyle = (lvNodes.columns.item (i + 2)).DefaultCellStyle
							defaultCellStyle.ForeColor = defaultCellStyle.ForeColor.fromARGB 150 0 0
							defaultCellStyle.BackColor = defaultCellStyle.BackColor.fromARGB 240 240 240
							defaultCellStyle.Alignment = defaultCellStyle.Alignment.MiddleCenter
							(lvNodes.columns.item (i + 2)).DefaultCellStyle = defaultCellStyle
							*/
							(cryAM._.lvNodes.columns.item (i+2)).DefaultCellStyle = animStyle[10]
						)
						else
							(cryAM._.lvNodes.columns.item (i + 2)).DefaultCellStyle = (cryAM._.lvNodes.columns.item 1).DefaultCellStyle
						
						(cryAM._.lvNodes.columns.item (i + 2)).sortMode = (cryAM._.lvNodes.columns.item 0).sortMode
					)
				)
			)
			else
				cryAM._.lvNodes.columnCount = 3
			
		
			lastLoadedAnim = 0
			
			if folderClickNode != undefined then
				selFolder = true
			
			if selFolder != undefined then
				join usedArray (getFolders (getSelectedFolderPath()))
			else
				join usedArray folderArray
			
			--animArray = #((cryAM_animStruct name:"     ----------------------------------------------     Load / Save As     -----------------------------------------" path:"" type:"scene"))
			
			animArray = #()
			
			for i = 1 to usedArray.count do
			(
				local fileArray = getFiles (usedArray[i].path + "*.*")
				
				qsort fileArray compareNodes
				local fileTempArray = #()
				
				for f = 1 to fileArray.count do
				(
					local filename = filterString fileArray[f] "\\"
					filename = filename[filename.count]
					local filetype = filterString filename "."
					
					filetype = filetype[filetype.count]
					
					
					local filterFile = filename
					if filterFullPath == true then
						filterFile = fileArray[f]
					
					if (getFileFound filterFile) == true then
					(
						append fileTempArray (cryAM_animStruct name:filename path:fileArray[f] type:filetype)
						
						if showSubAnims == true then
						(
							case filetype of
							(
								"max":	join fileTempArray (getMAXEntries fileArray[f])
							)
						)
					)
					
				)
				
				if fileTempArray.count > 0 then
					append animArray (cryAM_animStruct name:usedArray[i].name path:usedArray[i].path type:"folder")
				
				join animArray fileTempArray
			)
			
			
			
			
			cryAM._.lvNodes.Rows.Clear()
			cryAM._.lvNodes.RowCount = animArray.count + 1
		)
		
		
		
		for i = 1 to animArray.count do
		(
			if onlyBackground == false then
			(
				animArray[i].node = (cryAM._.lvNodes.Rows.item (i-1)).cells.item 0
				((cryAM._.lvNodes.Rows.item (i-1)).cells.item 1).value = animArray[i].type
					
				if keepSelection == true then
				(
					if findItem lastSelAnim animArray[i].path != 0 then
						animArray[i].node.selected = true
				)
				
				
				local animText = ""
					
				case animArray[i].type of
				(
					"folder":
					(
						local rootFilter = filterString rootSourcePath "\\"
						local tempFilter = filterString animArray[i].path "\\"
						local tempPath = "     ---    "
						
						for f = (rootFilter.count + 1) to tempFilter.count do
							tempPath += tempFilter[f] + (if f < tempFilter.count then " \\ " else "    ---")
						
						--((lvNodes.Rows.item (i-1)).cells.item 2).style = animStyle
						animText = tempPath
					)
					
					
					"scene":
						animText = animArray[i].name
					
					
					"anim":
					(
						animText = "      - " + animArray[i].name + "        (" + animArray[i].start as String + " - " + animArray[i].end as String + " )     -> " + animArray[i].export
						
					)
					
				)
				
				if animText == "" then
					animText = "   " + (filterString animArray[i].name ".")[1]
			
				((cryAM._.lvNodes.Rows.item (i-1)).cells.item 0).value = animText
			)
			
			
			((cryAM._.lvNodes.Rows.item (i-1)).cells.item 2).value = animArray[i].status
			
			

			
			
			if (onlyBackground == true and ((findItem useList i) != 0)) or onlyBackground == false then
			(	
				local style = animStyle[1]
				local styleExtra = animStyleExtra[1]
				
				
				case animArray[i].type of
				(
					
					"folder":
					(
						style = animStyle[2]
						styleExtra = animStyleExtra[2]
					)
					
					
					
					"bip":		style = animStyle[3]
					"max":	style = animStyle[4]
					"fbx":		style = animStyle[5]
					"htr":		style = animStyle[6]
					"xaf":		style = animStyle[7]
					"anim":	style = animStyle[8]
				)
				
				
			
				if animArray[i].status == "loaded" then
				(
					style = animStyle[9]
					styleExtra = animStyleExtra[3]
				)
				
				
				((cryAM._.lvNodes.Rows.item (i-1)).cells.item 0).style = style
				((cryAM._.lvNodes.Rows.item (i-1)).cells.item 1).style = style
				((cryAM._.lvNodes.Rows.item (i-1)).cells.item 2).style = styleExtra
			)
			
			
			if showCBADetails == true then
			(
				if animArray[i].type == "folder" then
				(
					for g = 3 to (cryAM._.lvNodes.columns.count - 1) do
					(
						((cryAM._.lvNodes.Rows.item (i-1)).cells.item g).value = ""
						((cryAM._.lvNodes.Rows.item (i-1)).cells.item g).style.backColor = ((cryAM._.lvNodes.Rows.item (i-1)).cells.item 0).style.backColor--.fromARGB 255 255 255
					)
				)
				else if foundCBAEntry > 0 then
				(
					if animArray[i].type != "folder" then
					(
						for h = 1 to CBAArray[foundCBAEntry].count do
						(
							local tempEntry = CBAArray[foundCBAEntry][h]
							local tempEntryName = cryMaxTools.basic.str.lower tempEntry.name
							
							for g = 1 to collectColumnArray.count do
							(
								if collectColumnArray[g] == tempEntryName then
								(
									animArray[i].cbaParams[g] = tempEntry.value
									((cryAM._.lvNodes.Rows.item (i-1)).cells.item (g + 2)).value = tempEntry.value
									foundColumnArray[g] = true
									exit
								)	
							)
						)
					
					
					
						if foundCBASpecialEntry > 0 then
						(
							for h = 1 to CBAArray[foundCBAEntry][foundCBASpecialEntry].value.count do
							(
								if findString animArray[i].path CBAArray[foundCBAEntry][foundCBASpecialEntry].value[h].value[1].value != undefined then
								(
									for t = 2 to CBAArray[foundCBAEntry][foundCBASpecialEntry].value[h].value.count do
									(
										local tempEntry = CBAArray[foundCBAEntry][foundCBASpecialEntry].value[h].value[t]
										
										local tempEntryName = cryMaxTools.basic.str.lower tempEntry.name
										
										if (local foundItem = findItem collectColumnArray tempEntryName) > 0 then
										(
											animArray[i].cbaParams[foundItem] = tempEntry.value
											((cryAM._.lvNodes.Rows.item (i-1)).cells.item (foundItem + 2)).value = tempEntry.value
											((cryAM._.lvNodes.Rows.item (i-1)).cells.item (foundItem + 2)).style = animStyle[11]
											
											foundColumnArray[foundItem] = true
										)
									)
									
									exit
								)
							)
						)
					)
				)
			)
			
		
			
			try UIUpdateRO.pbUpdate.value = 100.0 * i / animArray.count catch()
		)
		
		
		for i = (cryAM._.lvNodes.columns.count - 1) to 3 by -1 do
		(
			if foundColumnArray[i - 2] == false then
				cryAM._.lvNodes.columns.removeAt i
		)
		
		
		
		if onlyBackground == false then
		(
			updateAnimStatus()
			
			if skipDimensions == false then
				updateDimensions()
		)
		
		--gc light:true
		
		return true
	),
	
	
	
	
	
	
	function markFolder folderEntry =
	(
		cryAM._.lvFolders.BeginUpdate()
		
		
		local curFolder = undefined
		local selFolders = #()
		
		lastFolderSel = folderEntry
		
		
		if lastFolderSel != undefined then
		(
			local tempParent = lastFolderSel
			while tempParent != undefined do
			(
				escapeEnable = true
				append selFolders tempParent
				tempParent = tempParent.parent
			)
		)
			
		
		for i = 1 to folderArray.count do
		(
			curFolder = folderArray[i].node
			curFolder.imageIndex = 0
			curFolder.backColor = curFolder.backColor.FromARGB 225 224 210
		)
		
		
		
		for i = 1 to selFolders.count do
			selFolders[i].imageIndex = 1
		
		lastFolderSel.backColor = lastFolderSel.backColor.fromArgb 200 100 100
	
		
		cryAM._.lvFolders.EndUpdate()
	),
	
	
	
	
	function findEntry filename =
	(
		if doesFileExist filename == true then
		(
			local fileFolder = cryMaxTools.basic.str.getDirectory filename
			local file = cryMaxTools.basic.str.getFilename filename
			local sameFolder = false
			
			
			for i = 1 to folderArray.count do
				if folderArray[i].node == folderClickNode then
					if folderArray[i].path == fileFolder then
						sameFolder = true
			
			if sameFolder == false then
			(
				for i = 1 to folderArray.count do
				(
					if folderArray[i].path == fileFolder then
					(
						markFolder folderArray[i].node
						
						local tempParent = folderArray[i].node
						
						for i = 1 to 100 do
						(
							tempParent.expand()
							
							if tempParent.parent != undefined then
								tempParent = tempParent.parent
							else
								exit
						)
						
						folderClickNode = folderArray[i].node
						onUpdate = true
						cryAM._.lvFolders.selectedNode = folderClickNode
						onUpdate = false
						
						updateNodeList()
					
						
					)
				)
			)
			
			for i = 1 to animArray.count do
			(
				animArray[i].node.selected = false
				
				if animArray[i].name == file then
				(
					if i > 2 then
						cryAM._.lvNodes.FirstDisplayedCell = animArray[i - 2].node
					else
						cryAM._.lvNodes.FirstDisplayedCell = animArray[i].node
					
					animArray[i].node.selected = true
				)
			)
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	function updateFolderList keepSelection:false =
	(
		local selFolderPath = ""
		
		
		if keepSelection == true then
		(
			if folderClickNode != undefined then
				for i = 1 to folderArray.count do
					if folderArray[i].node == folderClickNode then
						selFolderPath = folderArray[i].path
		)
		
		cryAM._.lvFolders.BeginUpdate()
		cryAM._.lvFolders.Nodes.Clear()
		folderArray = #()
		
		local tempPath = rootProjectPath + rootSourcePath
		if rootSourcePath == "" then
			tempPath = ""
		
		
		if tempPath != "" then
		(
			
			local checkPath = tempPath
			if tempPath[tempPath.count] == "\\" then
				checkPath = subString tempPath 1 (tempPath.count - 1)
			
			if (getDirectories checkPath).count > 0 then
			(
				local tempFilter = filterString tempPath "\\"
				local folderRoot = tempFilter[tempFilter.count]
				
				local tempNode = cryAM._.lvFolders.Nodes.Add folderRoot folderRoot
				append folderArray (cryAM_folderStruct name:folderRoot path:tempPath node:tempNode)
				
				addFolder tempPath tempNode root:true
				
				tempNode.Expand()
			)
			
			if folderArray.count > 0 and keepSelection == false then
			(
				folderClickNode = (cryAM._.lvFolders.nodes.item 0)
				markFolder folderClickNode
				
				onUpdate = true
				cryAM._.lvFolders.selectedNode = folderClickNode
				onUpdate = false
			)
		)
		
		if keepSelection == true then
		(
			for i = 1 to folderArray.count do
			(
				if folderArray[i].path == selFolderPath then
				(
					folderClickNode = folderArray[i].node
					
					onUpdate = true
					cryAM._.lvFolders.selectedNode = folderClickNode
					onUpdate = false
					
					markFolder folderClickNode
				)
			)
		)
		
		cryAM._.lvFolders.EndUpdate()
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	function getSourceNodes =
	(
		local nodeArray = #()
		
		if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex > -1 then
		(
			for i = 1 to characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].sourceNodes.count do
				append nodeArray (getNodeByName characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].sourceNodes[i])
		)
		
		--gc light:true
		
		return nodeArray
	),
	
	
	
	
	function getGameBone =
	(
		if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
		(
			local charID = cryAM._.cbCharacter.selectedIndex
			local setupID = cryAM._.cbSetup.selectedIndex + 1
			local foundEmpty = 0
			local foundCondition = #()
			local gameBoneArray = #()
			
			
			
			
				--// Get condition string out of the several game bones \\--
			for i = 1 to characterArray[charID].setup[setupID].gameBones.count do
			(
				local conditionString = ""
				local conditionBracketsLayer = 0
				
				for d = 1 to characterArray[charID].setup[setupID].gameBones[i].conditions.count do
				(
					local conditionEntry = characterArray[charID].setup[setupID].gameBones[i].conditions[d]
					
					if d > 1 then
						local prevEntry = characterArray[charID].setup[setupID].gameBones[i].conditions[d-1]
					
					
					local openBrackets = "("
					local closeBrackets = ")"
					local preCloseBrackets = ""
					local notString = ""
					local groupString = ""
					local maxNewBrackets = conditionEntry.parentLayer - conditionBracketsLayer
					
					
					
						--// if the entry is the first or last one and open \\--
					if conditionEntry.parentLayer > 0 then
					(
						if d == characterArray[charID].setup[setupID].gameBones[i].conditions.count then
							maxNewBrackets = -conditionEntry.parentLayer
						
						if d == 1 then
							maxNewBrackets = conditionEntry.parentLayer
					)
					
					
					
					
					
						--// amount of brackets to add \\--
					local maxCount = maxNewBrackets
					if maxCount < 0 then
						maxCount *= -1
					
					
						--// current entry \\--
					local usedEntry = groupArray[conditionEntry.group]
					
						--// go through maxCount and add/remove brackets \\--
					for f = 1 to maxCount do
					(
							--// if a bracket should be opened \\--
						if maxNewBrackets > 0 then
						(
							openBrackets += "("
							
								--// if there was a condition before, use the previous instead \\--
							if d > 1 then
								usedEntry = groupArray[prevEntry.group]
						)
						
						
							--// if the bracket should be closed \\--
						if maxNewBrackets < 0 then
						(
								--// if the current layer is less than the previous one, close the pre-bracket, else close the post-bracket \\--
							if conditionEntry.parentLayer < conditionBracketsLayer then
								preCloseBrackets += ")"
							else
								closeBrackets += ")"
						)
						
					)
				
					
						--// set new bracket layer \\--
					conditionBracketsLayer = conditionEntry.parentLayer
					
						
					
					
						--// get group state \\--
					local tempFilter = filterString usedEntry " "
					groupString = tempFilter[tempFilter.count]
					
						--// add "not" at the beginning if NAND or NOR \\--
					if conditionEntry.group > 2 then
						notString = "not "
					
						--// no group before first entry \\--
					if d == 1 then
						groupString = ""
					
					
					
					
						--// condition \\--
					local state = stateArray[conditionEntry.state]
						--// variable string \\--
					local variableString = variableArray[conditionEntry.variableID]
						--// condition string together with "not" and variable \\--
					local conditionPart = ""
					
						--// special cases for condition
					if conditionEntry.active then
					(
						case state of
						(
							"has":	conditionPart = notString + "(findString (" + variableString + ") " + conditionEntry.value + " != undefined)"
							"HAS":	conditionPart = notString + "(cryMaxTools.basic.str.hasStr (" + variableString + ") " + conditionEntry.value + " isCase:true) == true"
							default:	conditionPart = notString + "(" + variableString + ") " + state + " " + conditionEntry.value
						)
					)	
					
						--// add pre-bracket, group part, open brackets and close brackets \\--
					conditionPart = preCloseBrackets + " " + groupString + " " + openBrackets + conditionPart + closeBrackets
						
						--// add the condition to the check string
					conditionString += conditionPart
				)
				
				
				try
					local condition = execute conditionString
				catch
					getCurrentException()
				
				if condition == true and characterArray[charID].setup[setupID].gameBones[i].conditions.count > 0 then
					append foundCondition i
				
				if characterArray[charID].setup[setupID].gameBones[i].conditions.count == 0 then
					foundEmpty = i
			)
			
			
			--gc light:true
			
			if foundCondition.count > 0 then
			(
				for i = 1 to foundCondition.count do
					append gameBoneArray (getNodeByName characterArray[charID].setup[setupID].gameBones[foundCondition[i]].name)
				
				return gameBoneArray
			)
			
			if foundEmpty > 0 then
				return (#(getNodeByName characterArray[charID].setup[setupID].gameBones[foundEmpty].name))
			
			return undefined
		)
	),
	
	
	
	
	
	
	
	
	
	
	function runTask mode:undefined getAmount:false updateWithMax:0 updateWithCur:0 =
	(
		case mode of
		(
			#import:
			(
				local taskArray = importArray
				local selIndex = cryAM._.cbLoad.selectedIndex
			)
			#export:
			(
				local taskArray = exportArray
				local selIndex = cryAM._.cbSave.selectedIndex + 1
			)
		)
		
			--// get active task
		
		local taskCount = 0
		local selTask = 1
		local errorOccured = false
		local curTask = 0
		
		if getAmount == true and selIndex == 0 then
			return 0
		
		
		if selIndex > 0 then
		(
			for i = 1 to taskArray.count do
			(
				if taskCount == selIndex then
					exit
				
				if taskArray[i].active == true then
				(
					taskCount += 1
					selTask = i
				)
			)
			
			
			if getAmount == true then
			(
				local selTaskCount = 0
				
				for i = 1 to taskArray[selTask].process.count do
					if taskArray[selTask].process[i].active == true then
						selTaskCount += 1
					
				return selTaskCount
			)
			
			
			
			
			
			for i = 1 to taskArray[selTask].process.count do
			(
				if taskArray[selTask].process[i].active == true then
				(
					local runScript = ""
					local scriptFound = ""
					local interrupted = false
					
					
					runScript = taskArray[selTask].process[i].script.text
					scriptFound = taskArray[selTask].process[i]
					
					
					if scriptFound != undefined then
						scriptFound = "in " + scriptFound.name + ":  "
					
					addLog ("        <--              Started:      " + (if mode == #import then "Import" else "Export") + " Script:       " + taskArray[selTask].name + "   ->   " + taskArray[selTask].process[i].name) isAdvanced:true
					
					
					
					
					try
					(
							--// tries executing the script
						execute( runScript )
						
						for f = 1 to taskArray[selTask].process[i].fnData.count do
						(
							local execStr = taskArray[selTask].process[i].fnData[f].name
							
							if taskArray[selTask].process[i].fnData[f].properties.count > 0 then
							(
								for d = 1 to taskArray[selTask].process[i].fnData[f].properties.count do
									execStr += " " + taskArray[selTask].process[i].fnData[f].properties[d].name + ":" + taskArray[selTask].process[i].fnData[f].properties[d].value as String
							)
							else
								execStr += "()"
							
							try
								execute ( execStr )
							catch
							(
								format "*** % ***\n" (scriptFound + "  Error in Function:  " + taskArray[selTask].process[i].fnData[f].name + "\n" + (getCurrentException()))
								errorOccured = true
								interrupted = true
							)
							
						)
					)
					catch
					(
							--// if an error occured, print the error message
						format "*** % ***\n" (scriptFound + " Error occured: " + (getCurrentException()))
						errorOccured = true
						interrupted = true
					)
					
					if interrupted == true then
						addLog ("        !!>              Error in:      " + (if mode == #import then "Import" else "Export") + " Script:       " + taskArray[selTask].name + "   ->   " + taskArray[selTask].process[i].name) isAdvanced:true
					else
						addLog ("        -->              Finished:      " + (if mode == #import then "Import" else "Export") + " Script:       " + taskArray[selTask].name + "   ->   " + taskArray[selTask].process[i].name) isAdvanced:true
					
					curTask += 1
					
					if updateWithMax > 0 then
						try UIProcessRO.pbCurrent.value = 100 * (curTask + (if updateWithCur > 0 then updateWithCur else 0)) / updateWithMax catch()
				)
			)
		)
		
		--gc light:true
		
		return (not errorOccured)
	),
	
	
	
	
	function getExportPath filepath extension =
	(
		local curPathFilter = filterString filepath "\\"
		local sourcePathFilter = filterString (rootProjectPath + rootSourcePath) "\\"
				
		local newPath = rootProjectPath + rootGamePath
				
		for i = (sourcePathFilter.count + 1) to curPathFilter.count do
			newPath += curPathFilter[i] + (if i < curPathFilter.count then "\\" else "")
				
		newPath = (filterString newPath ".")[1] + "." + extension
		
		--gc light:true
				
		return newPath
	),
	
	
	
	

	

	
	
	
	
	
	function multipleCharSelect =
	(
		rollout saveAnimBipedRO "Select Biped"
		(
			local bipRoots = #()
			
			listbox lbBiped "" pos:[5,5] width:110 height:6
			button btnUse "Use" pos:[5,95] width:50 height:20
			button btnCancel "Cancel" pos:[65,95] width:50 height:20
			
			on saveAnimBipedRO open do
			(
				bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
				local rootArray = #()
				for i = 1 to bipRoots.count do
					rootArray[i] = bipRoots[i].name
				
				lbBiped.items = rootArray
				
				--gc light:true
			)
			
			
			on btnUse pressed do
			(
				if lbBiped.selection > 0 then
				(
					destroyDialog saveAnimBipedRO
					
					cryAM.usedSkel = #(bipRoots[lbBiped.selection])
				)
			)
			
			
			on btnCancel pressed do
				destroyDialog saveAnimBipedRO
		)
		
		createDialog aveAnimBipedRO 120 120 modal:true
		
		--gc light:true
		
		return true
	),
	
	
	
	
	
	
	function updateImportList =
	(
		cryAM._.cbLoad.items.Clear()
		cryAM._.cbLoad.items.Add "   ---   None   ---"
		
		--local tempArray = #()
		
		
		
		for i = 1 to importArray.count do
			if importArray[i].active == true then
				cryAM._.cbLoad.items.Add ("  " + importArray[i].name)
			
		for i = 1 to importUserArray.count do
			if importUserArray[i].active == true then
				cryAM._.cbLoad.items.Add ("  " + importUserArray[i].name)
		
		
		onUpdate = true
			
		if cryAM._.cbLoad.items.count > 0 then
			cryAM._.cbLoad.selectedIndex = 0
		
		onUpdate = false
			--gc light:true
	),
	
	
	
	function updateExportList =
	(	
		onUpdate = true
		
		cryAM._.cbSave.items.Clear()
		
		for i = 1 to exportArray.count do
			if exportArray[i].active == true then
				cryAM._.cbSave.items.Add ("  " + exportArray[i].name)
			
		for i = 1 to exportUserArray.count do
			if exportUserArray[i].active == true then
				cryAM._.cbSave.items.Add ("  " + exportUserArray[i].name)
		
		if cryAM._.cbSave.items.count > 0 then
			cryAM._.cbSave.selectedIndex = 0
		
		onUpdate = false
		--gc light:true
	),
	
	
	
	
	function updateBySetup skipNodeList:false skipDimensions:false =
	(
		local charID = cryAM._.cbCharacter.selectedIndex
		
		try
		(
			if cryAM._.cbSetup.selectedIndex == -1 then
			(
				for i = characterArray[charID].setup.count to 1 by -1 do
				(
					if characterArray[charID].setup[i].active == true then
					(
						onUpdate = true
						cryAM._.cbSetup.selectedIndex = i - 1
						onUpdate = false
						exit
					)
				)
			)
		)catch()
		
		
		local setupID = cryAM._.cbSetup.selectedIndex + 1
		
		if charID > 0 and charID <= (characterArray.count + 1) and setupID > 0 then
		(
			cryAM._.edSourcePath.text = ""
			cryAM._.edGamePath.text = ""
			
			rootSourcePath = characterArray[charID].setup[setupID].sourcePath
			rootGamePath = characterArray[charID].setup[setupID].gamePath
			
			if rootSourcePath != "" then
				cryAM._.edSourcePath.text = rootSourcePath
			
			if rootGamePath != "" then
				cryAM._.edGamePath.text = rootGamePath
			
			updateFolderList()
			
			if skipNodeList == false then
				updateNodeList skipDimensions:skipDimensions
			
			updateRecentList()
		)
		
		--gc light:true
	),
	
	
	
	function updateByLastSel updateSetup:false skipDimensions:false =
	(
		local foundEntry = false
		
		if lastSel.count > 0 then
		(
			for i = 1 to lastSel.count do
			(
				if lastSel[i].charID == (cryAM._.cbCharacter.selectedIndex + 1) then
				(
					if updateSetup == true then
					(
						updateSetupList()
						
						cryAM.onUpdate = true
						cryAM._.cbSetup.selectedIndex = cryAM.lastSel[i].usedSetup - 1
						cryAM.onUpdate = false
						
						cryAM.updateBySetup skipNodeList:true skipDimensions:skipDimensions
					)
					
					if cryAM.lastSetup != cryAM._.cbSetup.selectedIndex + 1 then
					(
						cryAM.lastSetup = cryAM._.cbSetup.selectedIndex + 1
				
						if cryAM.prevSetup.count < 2 then
							cryAM.prevSetup = #(cryAM.lastSetup, cryAM.prevSetup[1])
						else
						(
							cryAM.prevSetup[2] = cryAM.prevSetup[1]
							cryAM.prevSetup[1] = cryAM.lastSetup
						)
					)
					
					
					
					for d = 1 to cryAM.lastSel[i].setup.count do
					(
						if cryAM.lastSel[i].setup[d].setupID == (cryAM._.cbSetup.selectedIndex + 1) then
						(
							local folderID = cryAM.lastSel[i].setup[d].folderID
							local folderName = cryAM.lastSel[i].setup[d].folderName
							local nodeSel = cryAM.lastSel[i].setup[d].nodeSel
							
							if folderID > 0 then
							(
								if cryAM.folderArray[folderID] != undefined then
								(
									foundEntry = true
									
									if cryAM.folderArray[folderID].name == folderName then
									(
										folderClickNode = cryAM.folderArray[folderID].node
										markFolder folderArray[folderID].node
										
										cryAM.onUpdate = true
										cryAM._.lvFolders.selectedNode = folderClickNode
										cryAM.onUpdate = false
										
										local tempFolder = cryAM.folderArray[folderID].node
										
										for i = 1 to 100 do
										(
											tempFolder.expand()
											
											if tempFolder.parent != undefined then
												tempFolder = tempFolder.parent
											else
												exit
										)
										
										folderClickNode = cryAM.folderArray[folderID].node
										
										cryAM.onUpdate = true
										cryAM._.lvFolders.selectedNode = folderClickNode
										cryAM.onUpdate = false
										
										cryAM.updateNodeList skipDimensions:skipDimensions
										
										if cryAM.animArray.count > 0 then
										(
											cryAM.animArray[1].selected = false
											cryAM.animArray[1].node.selected = false
										)
										
										local firstCell = undefined
										
										for d = 1 to cryAM.animArray.count do
										(
											if nodeSel[d] == true then
											(
												cryAM.animArray[d].selected = true
												cryAM.animArray[d].node.selected = true
												if firstCell == undefined then
												(
													if d > 2 then
														firstCell = cryAM.animArray[d-2].node
													else
														firstCell = cryAM.animArray[d].node
												)
											)
										)
										
										cryAM._.lvNodes.FirstDisplayedCell = firstCell
										
										updateAnimStatus()
										
										exit
									)
								)
							)
						)
					)
				)
			)
		)
		
		if foundEntry == false then
			updateBySetup skipDimensions:skipDimensions
		
		updateBuildInfo()
		
		--gc light:true
	),
	
	
	
	
	

	
	
	
	
	function updateByCharacter skipNodeList:false skipLastSel:false =
	(
		if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbCharacter.selectedIndex < (characterArray.count + 1) then
		(
			updateSetupList()
			
			if cryAM._.cbSetup.items.count > 0 then
			(
				onUpdate = true
				cryAM._.cbSetup.selectedIndex = 0
				onUpdate = false
			)
			
			if skipLastSel == false then
				updateByLastSel updateSetup:true
		)
	),
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function updateLastSel custom:undefined =
	(
		local charID = (cryAM._.cbCharacter.selectedIndex + 1)
		local setupID = cryAM._.cbSetup.selectedIndex + 1
		local folderID = 0
		local folderName = ""
		local nodeSel = #{}
		local foundEntry = [0,0]
		
		
		for i = 1 to folderArray.count do
		(
			if folderArray[i].node == lastFolderSel then
			(
				folderID = i
				folderName = folderArray[i].name
				exit
			)
		)
		
		if custom == undefined then
		(
			for i = 1 to animArray.count do
			(
				if animArray[i].selected == true then
					nodeSel[i] = true
			)
		)
		
		local tempSetup = cryAM_lastSetupStruct setupID:setupID folderID:folderID folderName:folderName nodeSel:nodeSel
		local tempChar = cryAM_lastCharStruct charID:charID usedSetup:setupID setup:#(tempSetup)
		
		
		if custom != undefined then
		(
			tempChar = custom[1]
			tempSetup = custom[2]
		)
		
		
		for i = 1 to lastSel.count do
		(
			if lastSel[i].charID == charID then
			(
				foundEntry[1] = i
				
				for d = 1 to lastSel[i].setup.count do
				(
					if lastSel[i].setup[d].setupID == setupID then
					(
						foundEntry[2] = d
						exit
					)
				)
				
				exit
			)
		)
		
		
		if foundEntry[1] > 0 then
		(
			if foundEntry[2] > 0 then
				lastSel[foundEntry[1]].setup[foundEntry[2]] = tempSetup
			else
			(
				append lastSel[foundEntry[1]].setup tempSetup
				--print foundEntry
			)
			
			lastSel[foundEntry[1]].usedSetup = setupID
		)
		else
			append lastSel tempChar
		
		--gc light:true
	),
	
	
	
	
	
	
	
	function getLastSel =
	(
		for i = 1 to lastSel.count do
		(
			if lastSel[i].charID == (cryAM._.cbCharacter.selectedIndex + 1) and lastSel[i].setupID == cryAM._.cbSetup.selectedIndex + 1 then
				return lastSel[i]
		)
		
		return undefined
	),
	
	
	
	
	
	
	
	
	function saveSettings mode:undefined saveAs:undefined custom:undefined noCheckout:false updateData:false =
	(
		case mode of
		(
			#character:	local settingsPath = characterSettingsPath
			#import:			local settingsPath = importSettingsPath
			#export:			local settingsPath = exportSettingsPath
			#user:			local settingsPath = userSettingsPath
			#dialogPos:		local settingsPath = userSettingsPath
			#importUser:	local settingsPath = importUserSettingsPath
			#exportUser:	local settingsPath = exportUserSettingsPath
		)
		
		if settingsPath == "" or saveAs == true then
			local newPath = getSaveFileName caption:("Save " + mode as String + " Settings") filename:settingsPath
		else
			local newPath = settingsPath
		
		
		if newPath != undefined then
		(
			if mode != #user and mode != #dialogPos and mode != #importUser and mode != #exportUser then
			(
				if sysInfo.username == "mathias" and noCheckout == false then
					if (queryBox "Check-Out file in Perforce?") == true then
						cryMaxTools.basic.perforceMan.open newPath
				
				cryMaxTools.basic.perforceMan.setWriteMessage newPath
			)
			
		
			local xmlFile = dotNetObject "system.xml.xmlDocument"
			local subMode = #none
			
			case mode of
			(
				#character:
				(
					subMode = #character
					local fileRoot = xmlFile.createElement "CharacterSettings"
					local templateArray = #()
					local charArray = characterArray
					if custom != undefined then charArray = custom
					local taskArray = #()
					local fileRev = characterFileRevision
				)
				
				#import:
				(
					subMode = #task
					local fileRoot = xmlFile.createElement "ImportSettings"
					local templateArray = importTemplateArray
					if custom != undefined then templateArray = custom[1]
					local charArray = #()
					local taskArray = importArray
					if custom != undefined then taskArray = custom[2]
					local fileRev = importFileRevision
				)
				#export:
				(
					subMode = #task
					local fileRoot = xmlFile.createElement "ExportSettings"
					local templateArray = exportTemplateArray
					if custom != undefined then templateArray = custom[1]
					local charArray = #()
					local taskArray = exportArray
					if custom != undefined then taskArray = custom[2]
					local fileRev = exportFileRevision
				)
				#user:
				(
					subMode = #user
					local fileRoot = xmlFile.createElement "UserSettings"
					local templateArray = #()
					local charArray = #()
					local taskArray = #()
					local fileRev = userFileRevision
				)
				#importUser:
				(
					subMode = #task
					local fileRoot = xmlFile.createElement "ImportUserSettings"
					local templateArray = #()
					if custom != undefined then templateArray = custom[1]
					local charArray = #()
					local taskArray = importUserArray
					if custom != undefined then taskArray = custom[2]
					local fileRev = importUserFileRevision
				)
				#exportUser:
				(
					subMode = #task
					local fileRoot = xmlFile.createElement "ExportUserSettings"
					local templateArray = #()
					if custom != undefined then templateArray = custom[1]
					local charArray = #()
					local taskArray = exportUserArray
					if custom != undefined then taskArray = custom[2]
					local fileRev = exportUserFileRevision
				)
			)
			
			
			if updateData != false and doesFileExist newPath then
			(
				xmlFile.load newPath
				local fileRoot = xmlFile.documentElement
			)
			else
			(
				xmlFile.appendChild fileRoot
				
				local fileInfo = xmlFile.createElement "FileInfo"
				fileInfo.setAttribute "Rev" (fileRev as String)
				fileRoot.appendChild fileInfo
			)
			
			case subMode of
			(
				#character:
				(
					if charArray.count > 0 then
					(	
						local charRoot =  xmlFile.createElement "Characters"
						
						for i = 1 to charArray.count do
						(
							local char = xmlFile.createElement "Character"
							char.setAttribute "Name" charArray[i].name
							char.setAttribute "Active" (charArray[i].active as String)
							
							for f = 1 to charArray[i].setup.count do
							(
								local setup = xmlFile.createElement "Setup"
								setup.setAttribute "Name" charArray[i].setup[f].name
								setup.setAttribute "Active" (charArray[i].setup[f].active as String)
								setup.setAttribute "SourcePath" charArray[i].setup[f].sourcePath
								setup.setAttribute "GamePath" charArray[i].setup[f].gamePath
								
								local setupModel = xmlFile.createElement "Model"
								setupModel.setAttribute "ModelPath" charArray[i].setup[f].model.path
								setupModel.setAttribute "AutoSync" (charArray[i].setup[f].model.autoSync as String)
								setupModel.setAttribute "RemoveOld" (charArray[i].setup[f].model.removeOld as String)
								setupModel.setAttribute "PreferLoad" (charArray[i].setup[f].model.preferLoad as String)
								
								local setupBiped = xmlFile.createElement "Biped"
								setupBiped.setAttribute "PosePath" charArray[i].setup[f].biped.path
								setupBiped.setAttribute "AutoSync" (charArray[i].setup[f].biped.autoSync as String)
								setupBiped.setAttribute "RemoveOld" (charArray[i].setup[f].biped.removeOld as String)
								
								local setupAlign = xmlFile.createElement "Align"
								setupAlign.setAttribute "TargetNode" charArray[i].setup[f].align.targetNode
								setupAlign.setAttribute "SourceNode" charArray[i].setup[f].align.sourceNode
								setupAlign.setAttribute "Rotation" (charArray[i].setup[f].align.rot as String)
								setupAlign.setAttribute "Position" (charArray[i].setup[f].align.pos as String)
								
								local setupMisc = xmlFile.createElement "Misc"
								setupMisc.setAttribute "SkipFirst" (charArray[i].setup[f].misc.skipFirst as String)
								setupMisc.setAttribute "SkipLast" (charArray[i].setup[f].misc.skipLast as String)
								setupMisc.setAttribute "CustomRig" (charArray[i].setup[f].misc.customRig as String)
								
								local sourceNodeRoot = xmlFile.createElement "SourceNodes"
								for d = 1 to charArray[i].setup[f].sourceNodes.count do
								(
									local sourceNode = xmlFile.createElement "Node"
									sourceNode.setAttribute "Name" charArray[i].setup[f].sourceNodes[d]
								
									sourceNodeRoot.appendChild sourceNode
								)
								
								local gameBoneRoot = xmlFile.createElement "GameBones"
								for d = 1 to charArray[i].setup[f].gameBones.count do
								(
									local gameBone = xmlFile.createElement "Node"
									gameBone.setAttribute "Name" charArray[i].setup[f].gameBones[d].name
									
									for t = 1 to charArray[i].setup[f].gameBones[d].conditions.count do
									(
										local tempCon = charArray[i].setup[f].gameBones[d].conditions[t]
										
										local condition = xmlFile.createElement "Condition"
										condition.setAttribute "Name" (tempCon.name as String)
										condition.setAttribute "Active" (tempCon.active as String)
										condition.setAttribute "Layer" (tempCon.parentLayer as string)
										condition.setAttribute "VarID" (tempCon.variableID as String)
										condition.setAttribute "StateID" (tempCon.state as String)
										condition.setAttribute "GroupID" (tempCon.group as String)
										condition.setAttribute "Value" (tempCon.value as String)
										
										gameBone.appendChild condition
									)
									--try gameBone.setAttribute "Condition" charArray[i].setup[f].gameBones[d].condition catch()
									
									gameBoneRoot.appendChild gameBone
								)
								
								
								setup.appendChild setupModel
								setup.appendChild setupBiped
								setup.appendChild setupAlign
								setup.appendChild setupMisc
								setup.appendChild sourceNodeRoot
								setup.appendChild gameBoneRoot
								
								char.appendChild setup
							)
							
							charRoot.appendChild char
						)
						
						fileRoot.appendChild charRoot
					)
				)
			
				#task:
				(
			
					if templateArray.count > 0 then
					(
						local templateRoot =  xmlFile.createElement "Templates"
						
						
						for i = 1 to templateArray.count do
						(
							local process = xmlFile.createElement "Process"
							process.setAttribute "Name" templateArray[i].name
							process.setAttribute "Active" (templateArray[i].active as String)
							
							local script = xmlFile.createElement "Script"
							script.setAttribute "Path" templateArray[i].script.path
							script.setAttribute "Text" templateArray[i].script.text
							
							process.appendChild script
							
							templateRoot.appendChild process
						)
						
						fileRoot.appendChild templateRoot
					)
						
					
					
					if taskArray.count > 0 then
					(
						local taskRoot =  xmlFile.createElement "Tasks"
						
						for i = 1 to taskArray.count do
						(
							local task = xmlFile.createElement ("Task")
							task.setAttribute "Name" taskArray[i].name
							task.setAttribute "Default" (taskArray[i].default as String)
							task.setAttribute "Active" (taskArray[i].active as String)
							
							for f = 1 to taskArray[i].process.count do
							(
								local process = xmlFile.createElement "Process"
								process.setAttribute "Name" taskArray[i].process[f].name
								process.setAttribute "Active" (taskArray[i].process[f].active as String)
								
								local script = xmlFile.createElement "Script"
								script.setAttribute "Path" taskArray[i].process[f].script.path
								script.setAttribute "Text" taskArray[i].process[f].script.text
								
								local paramRoot = xmlFile.createElement "Parameters"
								
								for d = 1 to taskArray[i].process[f].fnData.count do
								(
									local propertiesRoot = xmlFile.createElement "Function"
									propertiesRoot.setAttribute "Name" taskArray[i].process[f].fnData[d].name
									
									for s = 1 to taskArray[i].process[f].fnData[d].properties.count do
									(
										local property = xmlFile.createElement "Property"
										property.setAttribute "Name" taskArray[i].process[f].fnData[d].properties[s].name
										property.setAttribute "Value" taskArray[i].process[f].fnData[d].properties[s].value
										
										propertiesRoot.appendChild property
									)
									
									paramRoot.appendChild propertiesRoot
									
								)
								
								process.appendChild script
								process.appendChild paramRoot
								task.appendChild process
							)
							
							taskRoot.appendChild task
						)
						
						fileRoot.appendChild taskRoot
					)
				)
				
				#user:
				(
			
					if mode == #user then
					(
						if updateData == false then
						(
							local folderIndex = 0
							local folderName = ""
							for i = 1 to folderArray.count do
							(
								if folderArray[i].node.IsSelected == true then
								(
									folderIndex = i
									folderName = folderArray[i].name
									exit
								)
							)
							
							local nodeSel = ""
							for i = 1 to animArray.count do
							(
								if animArray[i].selected == true then
									nodeSel += i as String + ","
							)
						
						
							tempItem = xmlFile.createElement "DialogPos"; tempItem.setAttribute "Value" ((cryNET.getLoc main) as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "DialogSize"; tempItem.setAttribute "Value" ((cryNET.getSize main) as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "DialogDefaultSize"; tempItem.setAttribute "Value" (resetTaskSize as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "LoadSel"; tempItem.setAttribute "Value" ((cryAM._.cbLoad.selectedIndex + 1) as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "SaveSel"; tempItem.setAttribute "Value" ((cryAM._.cbSave.selectedIndex + 1) as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "ProjectPath"; tempItem.setAttribute "Value" (rootProjectPath) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "ShowSubFolders"; tempItem.setAttribute "Value" (showSubFolders as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "ShowProjectInfo"; tempItem.setAttribute "Value" (showProjectInfo as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "ShowTaskSetup"; tempItem.setAttribute "Value" (showTaskSettings as String) ; fileRoot.appendChild tempItem
							tempItem = xmlFile.createElement "ShowScript"; tempItem.setAttribute "Value" (showScript as String) ; fileRoot.appendChild tempItem
							
							tempItem = xmlFile.createElement "SetupPreset"
							for i = 1 to setupPresetArray.count do
							(
								local tempPreset = xmlFile.createElement "Preset"
								tempPreset.setAttribute "CharID" (setupPresetArray[i].charID as String)
								tempPreset.setAttribute "CharName" setupPresetArray[i].charName
								tempPreset.setAttribute "SetupID" (setupPresetArray[i].setupID as String)
								tempPreset.setAttribute "SetupName" setupPresetArray[i].setupName
								
								tempItem.appendChild tempPreset
							)
							fileRoot.appendChild tempItem
							
							
							tempItem = xmlFile.createElement "LastSel"
							tempItem.setAttribute "UsedChar" ((cryAM._.cbCharacter.selectedIndex + 1) as String)
							
							
							for i = 1 to lastSel.count do
							(
								local tempSel = xmlFile.createElement "Entry"
								tempSel.setAttribute "CharID" (lastSel[i].charID as String)
								tempSel.setAttribute "UsedSetup" (lastSel[i].usedSetup as String)
								
								
								
								
								for d = 1 to lastSel[i].setup.count do
								(
									tempSetup = xmlFile.createElement "Setup"
									tempSetup.setAttribute "SetupID" (lastSel[i].setup[d].setupID as String)
									tempSetup.setAttribute "FolderID" (lastSel[i].setup[d].folderID as String)
									tempSetup.setAttribute "FolderName" lastSel[i].setup[d].folderName
									tempSetup.setAttribute "NodeSel" (lastSel[i].setup[d].nodeSel as String)
									
									tempSel.appendChild tempSetup
								)
								
								tempItem.appendChild tempSel
							)
							
							fileRoot.appendChild tempItem
							
							
							tempItem = xmlFile.createElement "Filter"
							tempItem.setAttribute "Active" (cryAM._.cbAnimFilter.selectedIndex as String)
							for i = 2 to cryAM._.cbAnimFilter.items.count do
								tempItem.setAttribute ("Item" + i as String) (cryAM._.cbAnimFilter.items.item (i-1))
							fileRoot.appendChild tempItem
							
							tempItem = xmlFile.createElement "Recent"
							for i = 1 to recentHistoryArray.count do
							(
								local tempSubItem = xmlFile.createElement ("Item" + i as String)
								tempSubItem.setAttribute "CharID" (recentHistoryArray[i].charID as String)
								tempSubItem.setAttribute "SetupID" (recentHistoryArray[i].setupID as String)
								tempSubItem.setAttribute "Path" recentHistoryArray[i].path
								tempItem.appendChild tempSubItem
							)
							fileRoot.appendChild tempItem
							
							tempItem = xmlFile.createElement "Extension"
							tempItem.setAttribute "Active" (cryAM._.cbAnimExtension.selectedIndex as String)
							for i = 3 to cryAM._.cbAnimExtension.items.count do
								tempItem.setAttribute ("Item" + i as String) (cryAM._.cbAnimExtension.items.item (i-1))
							fileRoot.appendChild tempItem
						)
						else
						(
							case updateData of
							(
								#dialogPos:
								(
										--// dialog pos
									local tempItem = xmlFile.getElementsByTagName "DialogPos"
									if tempItem.count == 0 then
									(
										tempItem = xmlFile.createElement "DialogPos"
										fileRoot.appendChild tempItem
									)
									else
										tempItem = tempItem.itemOf[0]
									
									tempItem.setAttribute "Value" ((cryNET.getLoc main) as String)
									
										--// dialog size
									tempItem = xmlFile.getElementsByTagName "DialogSize"
									if tempItem.count == 0 then
									(
										tempItem = xmlFile.createElement "DialogSize"
										fileRoot.appendChild tempItem
									)
									else
										tempItem = tempItem.itemOf[0]
									
									tempItem.setAttribute "Value" ((cryNET.getSize main) as String)
								)
							)
						)
					)
				)
			)
			
			
			
			
			if updateData == false then
			(
				try (deleteFile newPath) catch()
				
				case mode of
				(
					#import: importSettingsPath = newPath
					#export: exportSettingsPath = newPath
				)
			)
			
			xmlFile.save newPath
		)
		
		gc light:true
	),
	
	
	
	
	
	
	
	
	function callRightMouseClickFolders =
	(
		RCMenu rightMouseClickFoldersRC
		(
			menuItem miRefresh "Refresh"
			--menuItem miRefreshList "Refresh Anim List"
			seperator sep1
			menuItem miShowSubFolders "Show All Sub-Folders"
			
			
			
			on rightMouseClickFoldersRC open do
			(
				miShowSubFolders.checked = cryAM.showSubFolders
			)
			
			
			on miShowSubFolders picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.showSubFolders = not miShowSubFolders.checked
				cryAM.saveSettings mode:#user
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			
			on miRefresh picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.updateFolderList keepSelection:true
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
		)
		
		
		--registerRightClickMenu rightMouseClickFoldersRC

		popUpMenu rightMouseClickFoldersRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
		
		
	),
	
	
	
	
	
	
	
	function callLoadAnim askLoad:false  =
	(
		local usedAnimIndex = 1
		local imported = false
		
		for i = 1 to animArray.count do
		(
			if animArray[i].selected == true and animArray[i].type != "scene" and animArray[i].type != "folder" and animArray[i].type != "" then
			(
				currentQue = animArray[i].path
				usedAnimIndex = i
				exit
			)
		)
		
		addLog (" <<-----        Start loading:      " + animArray[usedAnimIndex].name) addSep:true isAdvanced:true advancedLogPart:animArray[usedAnimIndex].path
		
		
		if askLoad == true then
		(
			local importPath = ""
			local ready = true
			
			local selEntryPath = lastImportedAnim
			
			if selEntryPath == "" and cryAM._.lvNodes.selectedCells.count > 0 then
				selEntryPath = animArray[((cryAM._.lvNodes.selectedCells.item 0).RowIndex + 1)].path
				
			local importFrom = getOpenFileName filename:selEntrypath caption:"Select file to open" types:"Biped File (*.bip)|*.bip|3ds Max (*.max)|*.max|XML Animation File (*.xaf)|*.xaf|Autodesk FBX (*.fbx)|*.fbx|Animation Manager Anim File (*.ama)|*.ama|Crytek Bone Animation File (*.caf)|*.caf"
			if importFrom != undefined then
				local importPath = importFrom
			else
				ready = false
			
			
			
			if ready == true then
			(
				case cryMaxTools.basic.str.getExtension importFrom of
				(
					"bip":
					(
						local bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
						if bipRoots.count > 1 and selection.count == 0 then
							multipleCharSelect()
						else
							usedSkel = (selection as Array)[1]
					)
				)
				
				currentQue = importPath
				cryMaxTools.anim.functions.UI.loadAnim currentQue
				runTask mode:#import
			
				lastImportedAnim = currentQue
				
				imported = true
			)
		)
		else
		(
			case animArray[usedAnimIndex].type of
			(
				"bip":
				(
					cryAM.usedSkel = #()
					
					local bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
					
					
					if selection.count > 0 then
					(
						if (selection as Array)[1].classID[1] == 37157 then
							cryAM.usedSkel[1] = (selection as Array)[1]
					)
					
					if cryAM.usedSkel.count == 0 then
					(
						if bipRoots.count > 1 then
							multipleCharSelect()
						else
							cryAM.usedSkel = bipRoots
					)
					
					
					if cryAM.usedSkel.count > 0 then
					(
						cryMaxTools.anim.functions.UI.loadAnim currentQue bipRoot:cryAM.usedSkel
						runTask mode:#import
							
						lastImportedAnim = currentQue
							
						imported = true
					)
				)
				
				"max":
				(
					cryMaxTools.anim.functions.UI.loadAnim currentQue
					runTask mode:#import
					
					lastImportedAnim = currentQue
					
					imported = true
				)
				
				"ama":
				(
					cryMaxTools.anim.functions.UI.loadAnim currentQue
					runTask mode:#import
					
					lastImportedAnim = currentQue
					
					imported = true
				)
				
				"xaf":
				(
					local nodes = #()
				
					if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
						nodes = characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].sourceNodes
					
					cryAM.usedSkel = nodes
					
					cryMaxTools.anim.functions.UI.loadAnim currentQue
					runTask mode:#import
					
					lastImportedAnim = currentQue
					
					imported = true
				)
				
				"htr":
				(
					cryMaxTools.anim.functions.UI.loadAnim currentQue
					runTask mode:#import
					
					lastImportedAnim = currentQue
					
					imported = true
				)
				
				
				"scene":
				(	
					
				)
				
				"anim":
				(
					if ((maxFilePath + maxFileName) != currentQue) or (lastImportedAnim != currentQue) then
					(
						cryMaxTools.anim.functions.UI.loadAnim currentQue
						lastImportedAnim = currentQue
					)
					
					currentQue = (cryMaxTools.basic.str.getDirectory currentQue) + animArray[usedAnimIndex].name
					
					animationRange = interval animArray[usedAnimIndex].start animArray[usedAnimIndex].end
					
					runTask mode:#import
					
					imported = true
				)
				
				default:
				(
					runTask mode:#import
				)
			)
		)
		
		if imported == true then
		(
			for i = 1 to animArray.count do
			(
				if i != usedAnimIndex then
					animArray[i].status = ""
				else
				(
					animArray[i].status = "loaded"
				)
			)
			
			
			updateHistory cryAM._.cbAnimRecent #set useString:currentQue
			saveSettings mode:#user
			
			updateNodeList onlyBackground:true useList:(#(lastLoadedAnim,usedAnimIndex))
			lastLoadedAnim = usedAnimIndex
				
			addLog (" ----->>        Finished loading:      " + animArray[usedAnimIndex].name) advancedLogPart:animArray[usedAnimIndex].path
		)
		else
			addLog (" !!!!!>>        Could not load:      " + animArray[usedAnimIndex].name) advancedLogPart:animArray[usedAnimIndex].path
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	function callSaveAnim askSave:false =
	(
		local indexArray = #()
		local ready = true
		local exportPath = ""
		local sceneExport = false
		
		if askSave == true then
		(
			local selEntryPath = lastExportedAnim
			
			if selEntryPath == "" and cryAM._.lvNodes.selectedCells.count > 0 then
				selEntryPath = animArray[((cryAM._.lvNodes.selectedCells.item 0).RowIndex + 1)].path
			
			local exportTo = getSaveFileName filename:selEntryPath caption:"Select file to export" types:"Biped File (*.bip)|*.bip|3ds Max (*.max)|*.max|XML Animation File (*.xaf)|*.xaf|Autodesk FBX (*.fbx)|*.fbx|Animation Manager Anim File (*.ama)|*.ama|Crytek Bone Animation File (*.caf)|*.caf"
			if exportTo != undefined then
			(
				exportPath = exportTo
				sceneExport = true
			)
			else
				ready = false
		)
		
		
		
		
		if ready == true then
		(
			for i = 1 to animArray.count do
			(
				animArray[i].status = ""
				
				if animArray[i].selected == true and animArray[i].type != "folder" and animArray[i].type != "" then
					append indexArray i
			)
			
			local bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
			if bipRoots.count > 1 and selection.count == 0 then
				multipleCharSelect()
			else
				usedSkel = (selection as Array)[1]	
			
			
			--labAmountMax.text = indexArray.count as String
			
			
			
			cancelContinue = false
			cryMaxRestart = undefined
			cryMaxTimeStamp = undefined
			cryMaxAnimContinue = undefined
			
			
			
			local amountImportTasks = 0
			local amountExportTasks = runTask mode:#export getAmount:true
			
			
			
			
			if indexArray.count > 1 then
				amountImportTasks = runTask mode:#import getAmount:true
			else
				if indexArray[1] != lastLoadedAnim then
					if not (queryBox "Continue saving over not loaded Animation?" title:"Animation Manager Saving File                       ") then
						return false
		
			
			callUIProcessDialog()
			
			
			local amountTasks = amountImportTasks + amountExportTasks
			
			local timeTest = 0
			local tempTime = 0
			
			
			with undo off
			(
				for i = 1 to indexArray.count do
				(
					if i == 1 then
						timeTest = timeStamp()
					
					escapeEnable = true
					
					if cancelContinue == true then
						exit
					
					local neededTime = 0
					
					if i > 1 then
						neededTime = getTimeEstimate indexArray i timeTest
					
					
					
					
					local id = indexArray[i]
					
					
					local entry = animArray[id]
					if entry != undefined then
					(	
						--if i == indexArray.count then
							--cancelContinue = true
						addLog (" <<-----        Start saving:      " + entry.name) addSep:true isAdvanced:true advancedLogPart:entry.path
						
						local seconds = neededTime / 1000
				
						local minute = (seconds / 60) as Integer
						seconds = (mod seconds 60) as Integer
							
						local hour = (minute / 60) as Integer
						minute = (mod minute 60) as Integer
						
						local timeString = (if hour < 10 then "0" else "") + hour as String + " : " + (if minute < 10 then "0" else "") + minute as String + " : " + (if seconds < 10 then "0" else "") + seconds as String
						
						
							
						
						try UIProcessRO.labEstimate.text = "Estimate:  " + timeString catch()
						try UIProcessRO.labCurrentAnim.text = entry.name catch()
						try UIProcessRO.labAmountSel.text = i as String catch()
						try UIProcessRO.labAmountAll.text = indexArray.count as String catch()
						try
						(
							local nextAnim = "---"
							if i < indexArray.count then
								nextAnim = animArray[indexArray[i+1]].name
							
							UIProcessRO.labNextAnim.text = nextAnim
						)
						catch()
						
						try UIProcessRO.pbCurrent.value = 0 catch()
						
						local loadNormalFile = false
						local loadAnimFile = false
						
						if askSave == false then
							currentQue = entry.path
						else
							currentQue = exportPath
						
						local savePath = currentQue
						
						if indexArray.count > 1 and entry.type != "anim" then
							loadNormalFile = true
						
						if entry.type == "anim" and (((maxFilePath + maxFileName) != entry.path) or (lastImportedAnim != entry.path)) then
							loadAnimFile = true
						
						
						if loadAnimFile == true or loadNormalFile == true then
						(
							cryAM.usedSkel = cryAM.getSourceNodes()
							
							cryMaxTools.anim.functions.UI.loadAnim currentQue
							runTask mode:#import
							--callLoadAnim()
							
							lastImportedAnim = currentQue
						)
						
						
						if entry.type == "anim" then
						(
							animationRange = interval entry.start entry.end
							savePath = (cryMaxTools.basic.str.getDirectory entry.path) + entry.name + "." + entry.export
						)
						
						
						/*if loadNormalFile == true or entry.type == "anim" then
							runTask mode:#import updateWithMax:amountTasks
							*/
						
						
						if cancelContinue == true then
							exit
						
						exportPath = savePath
						
						
						
						
						--animArray[id].selected = false
						--animArray[id].node.selected = false
					
						updateLastSel()
						saveSettings mode:#user
						
						if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
						(
							
						)
						
						currentQue = exportPath
						local error = not (runTask mode:#export updateWithMax:amountTasks updateWithCur:(amountImportTasks))
						
						lastExportedAnim = exportPath
						
						if error then
						(
							animArray[id].status = "Error @ Saving"
							addLog (" !!!!!>>        Error while saving:      " + entry.name) advancedLogPart:entry.path
						)
						else
						(
							animArray[id].status = "saved"
							addLog (" ----->>        Finished saving:      " + entry.name) advancedLogPart:entry.path
						)
						
						try UIProcessRO.pbOverall.value = 100.0 * (i as Float) / (indexArray.count as Float) catch()
						
						
						if i == 1 then
							timeTest = timeStamp() - timeTest
					)
				)
			)
		)
		
		try destroyDialog UIProcessRO catch()
		
		callUIUpdateDialog()
		
		if sceneExport == true then
			updateNodeList keepSelection:true
		else
			updateNodeList onlyBackground:true useList:indexArray keepSelection:true
		
		try destroyDialog UIUpdateRO catch()
		
		cancelContinue = false
		cryMaxRestart = undefined
		cryMaxTimeStamp = undefined
		cryMaxAnimContinue = undefined
		
		
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function loadSettings mode:undefined load:undefined writeTo:false skipDimensions:false =
	(
		local settingsPath = ""
		local newPath = ""
		
		case mode of
		(
			#character:	settingsPath = characterSettingsPath
			#import:			settingsPath = importSettingsPath
			#export:			settingsPath = exportSettingsPath
			#importUser:	settingsPath = importUserSettingsPath
			#exportUser:	settingsPath = exportUserSettingsPath
			#user:			settingsPath = userSettingsPath
			#dialogPos:		settingsPath = userSettingsPath
		)
		
		
		
		if load == undefined then
			newPath = getOpenFileName caption:("Open " + mode as String + " Template") filename:settingsPath
		else
		(
			newPath = settingsPath
			
			if load == true then
			(
				if doesFileExist settingsPath == false then
					newPath = getOpenFileName caption:("Open " + mode as String + " Template") filename:settingsPath
			)
		)
		
		
		
		
		if newPath != "" and newPath != undefined then
		(
			if doesFileExist newPath == true then
			(
				local xmlFile = dotNetObject "system.xml.xmlDocument"
				
				xmlFile.load newPath
				
				
				local fileRoot = xmlFile.documentElement
				local fileRev = execute (fileRoot.childNodes.itemOf[0].attributes.itemOf[0].value)
				
				
				local validFile = true
				
				case mode of
				(
					#character:	validFile = fileRev >= characterFileRevision
					#import:			validFile = fileRev >= importFileRevision
					#export:			validFile = fileRev >= exportFileRevision
					#importUser:	validFile = fileRev >= importUserFileRevision
					#exportUser:	validFile = fileRev >= exportUserFileRevision
					#user:			validFile = fileRev >= userFileRevision
					#dialogPos:		validFile = fileRev >= userFileRevision
				)
				
				if validFile == false then
				(
					print ("Could not load file:  " + newPath + "!\n\nCorrupt data with this version of the Script.\nRe-save your settings")
					return false
				)
				
				
				if mode == #import or mode == #export or mode == #importUser or mode == #exportUser then
				(
					local templateArray = #()
					local taskArray = #()
					
					for i = 0 to (fileRoot.childNodes.count - 1) do
					(
						case fileRoot.childNodes.itemOf[i].name of
						(
							"Templates":
							(
								local templates = fileRoot.childNodes.itemOf[i]
								
								if templates != undefined then
								(
									for d = 0 to (templates.childNodes.count - 1) do
									(
										case templates.childNodes.itemOf[d].name of
										(
											"Process":
											(
												local process = templates.childNodes.itemOf[d]
												local processName = ""
												local processActive = true
												local processScriptPath = ""
												local processScriptText = ""
												
												if process != undefined then
												(
													for f = 0 to (process.attributes.count - 1) do
													(
														case process.attributes.itemOf[f].name of
														(
															"Name":	processName = process.attributes.itemOf[f].value
															"Active":	processActive = (process.attributes.itemOf[f].value == "true")
														)
													)
													
													for f = 0 to (process.childNodes.count - 1) do
													(
														case process.childNodes.itemOf[f].name of
														(
															"Script":
															(
																local script = process.childNodes.itemOf[0]
																
																for t = 0 to (script.attributes.count - 1) do
																(
																	case script.attributes.itemOf[t].name of
																	(
																		"Path":	processScriptPath = script.attributes.itemOf[t].value
																		"Text":	processScriptText = script.attributes.itemOf[t].value
																	)
																)
															)
														)
													)
												)	
													
												append templateArray (cryAM_processStruct name:processName active:processActive script:(cryAM_scriptStruct text:processScriptText path:processScriptPath))
											)
										)
									)
								)
							)
					
					
							"Tasks":
							(
								local tasks = fileRoot.childNodes.itemOf[i]
								
								for n = 0 to (tasks.childNodes.count - 1) do
								(
									case tasks.childNodes.itemOf[n].name of
									(
										"Task":
										(
											local processArray = #()
											
											local task = tasks.childNodes.itemOf[n]
											local taskName = ""
											local taskActive = true
											local taskDefault = false
											
											for f = 0 to (task.attributes.count - 1) do
											(
												case task.attributes.itemOf[f].name of
												(
													"Name":		taskName = task.attributes.itemOf[f].value
													"Active":		taskActive = (task.attributes.itemOf[f].value == "true")
													"Default":	taskDefault = (task.attributes.itemOf[f].value == "true")
												)
											)
											
											for f = 0 to (task.childNodes.count - 1) do
											(
												case task.childNodes.itemOf[f].name of
												(
													"Process":
													(
														local process = task.childNodes.itemOf[f]
														local processName = ""
														local processActive = true
														
														local scriptPath = ""
														local scriptText = ""
														local fnData = #()
														
														for d = 0 to (process.attributes.count - 1) do
														(
															case process.attributes.itemOf[d].name of
															(
																"Name":	processName = process.attributes.itemOf[d].value
																"Active":	processActive = (process.attributes.itemOf[d].value == "true")
															)
														)
														
														
														for d = 0 to (process.childNodes.count - 1) do
														(
															case process.childNodes.itemOf[d].name of
															(
																"Script":
																(
																	local script = process.childNodes.itemOf[d]
																	
																	for h = 0 to (script.attributes.count - 1) do
																	(
																		case script.attributes.itemOf[h].name of
																		(
																			"Path":	scriptPath = script.attributes.itemOf[h].value
																			"Text":	scriptText = script.attributes.itemOf[h].value
																		)
																	)
																)
																
																"Parameters":
																(
																	
																	local parameterRoot = process.childNodes.itemOf[d]
																	local propIndex = 1
																	
																	for h = 0 to (parameterRoot.childNodes.count - 1) do
																	(	
																		case parameterRoot.childNodes.itemOf[h].name of
																		(
																			"Function":
																			(
																				local functionRoot = parameterRoot.childNodes.itemOf[h]
																				local functionName = ""
																				local functionIndex = propIndex
																				local propertyArray = #()
																				
																				for k = 0 to (functionRoot.attributes.count - 1) do
																				(
																					case functionRoot.attributes.itemOf[k].name of
																					(
																						"Name":	functionName = functionRoot.attributes.itemOf[k].value
																					)
																				)
																				
																				
																				for s = 0 to (functionRoot.childNodes.count - 1) do
																				(
																					case functionRoot.childNodes.itemOf[s].name of
																					(
																						"Property":
																						(
																							local property = functionRoot.childNodes.itemOf[s]
																							local propertyName = ""
																							local propertyValue = ""
																							
																							for t = 0 to (property.attributes.count - 1) do
																							(
																								case property.attributes.itemOf[t].name of
																								(
																									"Name":	propertyName = property.attributes.itemOf[t].value
																									"Value":	propertyValue = property.attributes.itemOf[t].value
																								)
																							)
																							
																							append propertyArray (cryAM_propertiesStruct name:propertyName value:propertyValue index:propIndex)
																							propIndex += 1
																						)
																					)
																				)
																				
																				append fnData (cryAM_functionStruct name:functionName properties:propertyArray index:functionIndex)
																				propIndex += 1
																			)
																		)
																	)
																)
															)
														)
														
														append processArray (cryAM_processStruct name:processName active:processActive script:(cryAM_scriptStruct text:scriptText path:scriptPath) fnData:fnData)
													)
												)
											)
											
											append taskArray (cryAM_taskStruct name:taskName active:taskActive default:taskDefault process:processArray)
										)
									)
								)
							)
						)
					)
				
					
					if writeTo == false then
					(
						case mode of
						(
							#import: 
							(
								importArray = taskArray
								importTemplateArray = templateArray
								importSettingsPath = newPath
								
								updateImportList()
							)
							#export:
							(
								exportArray = taskArray
								exportTemplateArray = templateArray
								exportSettingsPath = newPath
								
								updateExportList()
							)
							#importUser:
							(
								join importArray taskArray
								updateImportList()
							)
							#exportUser:
							(
								join exportArray taskArray
								updateExportList()
							)
						)
					)
					else
						return (#(templateArray,taskArray,newPath))
				)
				
				
				
					
				if mode == #character then
				(
					for t = 0 to (fileRoot.childNodes.count - 1) do
					(
						case fileRoot.childNodes.itemOf[t].name of
						(
							"Characters":
							(
								local characterArray = #()
								
								local characters = fileRoot.childNodes.itemOf[t]
								
								for i = 0 to (characters.childNodes.count - 1) do
								(
									case characters.childNodes.itemOf[i].name of
									(
										"Character":
										(
											local char = characters.childNodes.itemOf[i]
											
											local charName = ""
											local charActive = true
											local setups = #()
											
											
											for f = 0 to (char.attributes.count - 1) do
											(
												case char.attributes.itemOf[f].name of
												(
													"Name":	charName = char.attributes.itemOf[f].value
													"Active":	charActive = if (char.attributes.itemOf[f].value == "true") then true else false
												)
											)
									
									
									
											for f = 0 to (char.childNodes.count - 1) do
											(
												local setup = char.childNodes.itemOf[f]
												local sourceNodes = #()
												local gameBones = #()
												
												local setupName = ""
												local setupActive = true
												local setupSource = ""
												local setupGame = ""
												
												local setupModelPath = ""
												local setupModelAutoSync = true
												local setupModelRemoveOld = true
												local setupModelPreferLoad = false
												
												local setupBipedPosePath = ""
												local setupBipedAutoSync = true
												local setupBipedRemoveOld = false
												
												local setupAlignTargetNode = ""
												local setupAlignSourceNode = ""
												local setupAlignRotation = quat 1
												local setupAlignPosition = [0,0,0]
												
												local setupMiscSkipFirst = 0
												local setupMiscSkipLast = 0
												local setupMiscCustomRig = false
												
												
												for d = 0 to (setup.attributes.count - 1) do
												(
													case setup.attributes.itemOf[d].name of
													(
														"Name":			setupName = setup.attributes.itemOf[d].value
														"Active":			setupActive = (setup.attributes.itemOf[d].value == "true")
														"SourcePath":	setupSource = setup.attributes.itemOf[d].value
														"GamePath":	setupGame = setup.attributes.itemOf[d].value
													)
												)
												
												
												
												
												
												for h = 0 to (setup.childNodes.count - 1) do
												(
													case setup.childNodes.itemOf[h].name of
													(
														"Model":
														(
															local modelRoot = setup.childNodes.itemOf[h]
												
															for d = 0 to (modelRoot.attributes.count - 1) do
															(
																case modelRoot.attributes.itemOf[d].name of
																(
																	"ModelPath":	setupModelPath = modelRoot.attributes.itemOf[d].value
																	"AutoSync":	setupModelAutoSync = (modelRoot.attributes.itemOf[d].value == "true")
																	"RemoveOld":	setupModelRemoveOld = (modelRoot.attributes.itemOf[d].value == "true")
																	"PreferLoad":	setupModelPreferLoad = (modelRoot.attributes.itemOf[d].value == "true")
																)
															)
														)
														
														"Biped":
														(
															local bipedRoot = setup.childNodes.itemOf[h]
												
															for d = 0 to (bipedRoot.attributes.count - 1) do
															(
																case bipedRoot.attributes.itemOf[d].name of
																(
																	"PosePath":		setupBipedPosePath = bipedRoot.attributes.itemOf[d].value
																	"AutoSync":	setupBipedAutoSync = (bipedRoot.attributes.itemOf[d].value == "true")
																	"RemoveOld":	setupBipedRemoveOld = (bipedRoot.attributes.itemOf[d].value == "true")
																)
															)
														)
														
														"Align":
														(
															local alignRoot = setup.childNodes.itemOf[h]
												
															for d = 0 to (alignRoot.attributes.count - 1) do
															(
																case alignRoot.attributes.itemOf[d].name of
																(
																	"TargetNode":	setupAlignTargetNode = alignRoot.attributes.itemOf[d].value
																	"SourceNode":	setupAlignSourceNode = alignRoot.attributes.itemOf[d].value
																	"Rotation":		setupAlignRotation = execute alignRoot.attributes.itemOf[d].value
																	"Position":		setupAlignPosition = execute alignRoot.attributes.itemOf[d].value
																)
															)
														)
														
														"Misc":
														(
															local miscRoot = setup.childNodes.itemOf[h]
												
															for d = 0 to (miscRoot.attributes.count - 1) do
															(
																case miscRoot.attributes.itemOf[d].name of
																(
																	"SkipFirst":		setupMiscSkipFirst = execute miscRoot.attributes.itemOf[d].value
																	"SkipLast":		setupMiscSkipLast = execute miscRoot.attributes.itemOf[d].value
																	"CustomRig":	setupMiscCustomRig = execute miscRoot.attributes.itemOf[d].value
																)
															)
														)
														
														"SourceNodes":
														(
															for d = 0 to (setup.childNodes.itemOf[h].childNodes.count - 1) do
																append sourceNodes (setup.childNodes.itemOf[h].childNodes.itemOf[d].attributes.itemOf[0].value)
														)
														
														"GameBones":
														(
															for d = 0 to (setup.childNodes.itemOf[h].childNodes.count - 1) do
															(
																local gameBone = setup.childNodes.itemOf[h].childNodes.itemOf[d]
																local gameBoneName = gameBone.attributes.itemOf[0].value
																
																local conditionArray = #()
																
																for t = 0 to (gameBone.childNodes.count - 1) do
																(
																	case gameBone.childNodes.itemOf[t].name of
																	(
																		"Condition":
																		(
																			local conditionNode = gameBone.childNodes.itemOf[t]
																			
																			local conditionName = ""
																			local conditionActive = false
																			local conditionLayer = 0
																			local conditionVariableID = 0
																			local conditionStateID = 0
																			local conditionGroupID = 0
																			local conditionValue = ""
																			
																			for h = 0 to (conditionNode.attributes.count - 1) do
																			(
																				case conditionNode.attributes.itemOf[h].name of
																				(
																					"Name":		conditionName = conditionNode.attributes.itemOf[h].value as String
																					"Active":		conditionActive = execute (conditionNode.attributes.itemOf[h].value)
																					"Layer":		conditionLayer = execute (conditionNode.attributes.itemOf[h].value)
																					"VarID":		conditionVariableID = execute (conditionNode.attributes.itemOf[h].value)
																					"StateID":	conditionStateID = execute (conditionNode.attributes.itemOf[h].value)
																					"GroupID":	conditionGroupID = execute (conditionNode.attributes.itemOf[h].value)
																					"Value":		conditionValue = conditionNode.attributes.itemOf[h].value as String
																				)
																			)
																			
																			append conditionArray (cryAM_conditionStruct name:conditionName active:conditionActive parentLayer:conditionLayer variableID:conditionVariableID state:conditionStateID group:conditionGroupID value:conditionValue)
																		)
																	)
																)
																
																append gameBones (cryAM_gameBoneStruct name:gameBoneName conditions:conditionArray) --condition:gameBoneCondition)
															)
														)
													)
												)
												

												
												
												
												local tempSetup = cryAM_setupStruct name:setupName active:setupActive sourcePath:setupSource gamePath:setupGame sourceNodes:sourceNodes gameBones:gameBones
												
												tempSetup.model.path = setupModelPath
												tempSetup.model.autoSync = setupModelAutoSync
												tempSetup.model.removeOld = setupModelRemoveOld
												tempSetup.model.preferLoad = setupModelPreferLoad
												
												tempSetup.biped.path = setupBipedPosePath
												tempSetup.biped.autoSync = setupBipedAutoSync
												tempSetup.biped.removeOld = setupBipedRemoveOld
												
												tempSetup.align.targetNode = setupAlignTargetNode
												tempSetup.align.sourceNode = setupAlignSourceNode
												tempSetup.align.rot = setupAlignRotation
												tempSetup.align.pos = setupAlignPosition
												
												tempSetup.misc.skipFirst = setupMiscSkipFirst
												tempSetup.misc.skipLast = setupMiscSkipLast
												tempSetup.misc.customRig = setupMiscCustomRig
												
												
												
												append setups tempSetup
											)
											
											append characterArray (cryAM_characterStruct name:charName active:charActive setup:setups)
											
										)
									)
								)
								
								if writeTo == false then
									cryAM.characterArray = cryMaxTools.basic.misc.makeUnique characterArray
								else
									return (#(characterArray))
							)
						)
					)
				)
				
				if mode == #user then
				(
					local filterArray = cryAM._.cbAnimFilter.items
					local extensionArray = cryAM._.cbAnimExtension.items
					
					local recentFilenameArray = #()
					recentHistoryArray = #()
					rootProjectPath = ""
				
					onUpdate = true
					
					for i = 0 to (fileRoot.childNodes.count - 1) do
					(
						case fileRoot.childNodes.itemOf[i].name of
						(
							"LoadSel":				try cryAM._.cbLoad.selectedIndex = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) - 1 catch ()
							"SaveSel":				try cryAM._.cbSave.selectedIndex = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) - 1 catch ()
							"ProjectPath":			try rootProjectPath = (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) catch ()
							"DialogDefaultSize":	try resetTaskSize = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)	catch()
							"ShowSubFolders":	try showSubFolders = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) catch()
							"ShowScript":			try showScript = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)	catch()
							"ShowProjectInfo":	try showProjectInfo = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) catch ()
							"ShowTaskSetup":	try showTaskSettings = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value) catch ()
						)
					)
					
					onUpdate = false
					
					--updateByCharacter skipNodeList:true skipSetup:true skipLastSel:true
					
					
					for i = 0 to (fileRoot.childNodes.count - 1) do
					(
						case fileRoot.childNodes.itemOf[i].name of
						(
							"SetupPreset":
							(
								setupPresetArray = #()
								
								for d = 0 to (fileRoot.childNodes.itemOf[i].childNodes.count - 1) do
								(
									local tempPreset = fileRoot.childNodes.itemOf[i].childNodes.itemOf[d]
									local charID = 0
									local charName = ""
									local setupID = 0
									local setupName = ""
									
									for t = 0 to (tempPreset.attributes.count - 1) do
									(
										case tempPreset.attributes.itemOf[t].name of
										(
											"CharID":		try charID = execute (tempPreset.attributes.itemOf[t].value) catch()
											"CharName":	try charName = tempPreset.attributes.itemOf[t].value catch()
											"SetupID":		try setupID = execute (tempPreset.attributes.itemOf[t].value) catch()
											"SetupName":	try setupName = tempPreset.attributes.itemOf[t].value catch()
										)
									)
									
									append setupPresetArray (cryAM_setupPresetStruct charID:charID charName:charName setupID:setupID setupName:setupName)
								)
							)
							
							
							"LastSel":
							(
								lastSel = #()
								
								onUpdate = true
								
								for d = 0 to (fileRoot.childNodes.itemOf[i].attributes.count - 1) do
								(
									case fileRoot.childNodes.itemOf[i].attributes.itemOf[d].name of
									(
										"UsedChar": try cryAM._.cbCharacter.selectedIndex = (execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[d].value)) - 1 catch()
									)
								)
								
								onUpdate = false
								
								updateSetupList()
								
								
								for d = 0 to (fileRoot.childNodes.itemOf[i].childNodes.count - 1) do
								(
									local tempSel = fileRoot.childNodes.itemOf[i].childNodes.itemOf[d]
									local charID = 0
									local usedSetup = 0
									local setupArray = #()
									
									for t = 0 to (tempSel.attributes.count - 1) do
									(
										case tempSel.attributes.itemOf[t].name of
										(
											"CharID": try charID = execute (tempSel.attributes.itemOf[t].value) catch ()
											"UsedSetup": try usedSetup = execute (tempSel.attributes.itemOf[t].value) catch ()
										)
									)
									
									for t = 0 to (tempSel.childNodes.count - 1) do
									(
										local tempSetup = tempSel.childNodes.itemOf[t]
										
										local setupID = 0
										local folderID = 0
										local folderName = ""
										local nodeSel = #{}
										
										for f = 0 to (tempSetup.attributes.count - 1) do
										(
											case tempSetup.attributes.itemOf[f].name of
											(
												"SetupID": try setupID = execute (tempSetup.attributes.itemOf[f].value) catch ()
												"FolderID": try folderID = execute (tempSetup.attributes.itemOf[f].value) catch ()
												"FolderName": try folderName = tempSetup.attributes.itemOf[f].value catch ()
												"NodeSel": try nodeSel = execute (tempSetup.attributes.itemOf[f].value) catch ()
											)
										)
										
										append setupArray (cryAM_lastSetupStruct setupID:setupID folderID:folderID folderName:folderName nodeSel:nodeSel)
									)
									
									append lastSel (cryAM_lastCharStruct charID:charID usedSetup:usedSetup setup:setupArray)
								)
							)
							
							
							"Filter":
							(
								onUpdate = true
								
								for d = 1 to (fileRoot.childNodes.itemOf[i].attributes.count - 1) do
									filterArray.Add fileRoot.childNodes.itemOf[i].attributes.itemOf[d].value
								
								cryAM._.cbAnimFilter.selectedIndex = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)
								
								onUpdate = false
							)
							
							"Recent":
							(
								onUpdate = true
								
								local recentRoot = fileRoot.childNodes.itemOf[i]
								cryAM._.cbAnimRecent.items.clear()
								
								for d = 0 to (recentRoot.childNodes.count - 1) do
								(
									local charID = 0
									local setupID = 0
									local recentPath = ""
									local recent = recentRoot.childNodes.itemOf[d]
									
									for f = 0 to (recent.attributes.count - 1) do
									(
										case recent.attributes.itemOf[f].name of
										(
											"CharID":	charID = execute (recent.attributes.itemOf[f].value)
											"SetupID":	setupID = execute (recent.attributes.itemOf[f].value)
											"Path":		recentPath = recent.attributes.itemOf[f].value
										)
									)
									
									 
									append recentHistoryArray (cryAM_recentStruct charID:charID setupID:setupID path:recentPath)
									
									if charID == (cryAM._.cbCharacter.selectedIndex + 1) and setupID == cryAM._.cbSetup.selectedIndex + 1 then
										cryAM._.cbAnimRecent.items.Add (cryMaxTools.basic.str.getFilename recentHistoryArray[recentHistoryArray.count].path)
								)
								
								onUpdate = false
							)
							
							"Extension":
							(
								onUpdate = true
								
								for d = 1 to (fileRoot.childNodes.itemOf[i].attributes.count - 1) do
									cryAM._.cbAnimExtension.items.Add fileRoot.childNodes.itemOf[i].attributes.itemOf[d].value
								
								
								cryAM._.cbAnimExtension.selectedIndex = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)
								
								onUpdate = false
							)
						)
					)
					
									
					
					updateByLastSel updateSetup:true skipDimensions:skipDimensions
					
					cancelContinue = false
					
					try deleteFile ((getDir #scripts) + "\\restartAndContinue.ms") catch()
					
					
				)
				
				if mode == #dialogPos then
				(
					for i = 0 to (fileRoot.childNodes.count - 1) do
					(
						case fileRoot.childNodes.itemOf[i].name of
						(
							"DialogPos":		dialogPos = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)
							"DialogSize":	dialogSize = execute (fileRoot.childNodes.itemOf[i].attributes.itemOf[0].value)
						)
					)
				)
				
				dotnet.setLifeTimeControl xmlFile #dotNet
			)
		)
		
		if rootProjectPath == "" then
			rootProjectPath = cryMaxTools.basic.vars.buildPath + "\\"
		
		cryAM._.edProjectPath.text = rootProjectPath
		
		updateRootPathsUI()
		
		gc light:true
	),
	
	
	
	
	
	function updateRootPath value op get:"" =
	(
		local refPath = ""
		case op of
		(
			#source:	refPath = rootSourcePath
			#game:		refPath = rootGamePath
			#project:	refPath = rootProjectPath
		)
		
		if get != "" then
			refPath = get
		
		
		if value[value.count] != "\\" then
			value += "\\"
		
			--// strip out project path
		if op == #source or op == #game and rootProjectPath != "" then
		(
			if findString value rootProjectPath != undefined then
				value = subString value (rootProjectPath.count + 1) value.count
		)
		
		--gc light:true
		
		if get == "" then
		(
			case op of
			(
				#source:	rootSourcePath = value
				#game:		rootGamePath = value
				#project:	rootProjectPath = value
			)
		)
		else
			return refPath
		
		saveSettings mode:#character
	),
	
	
	
	
	
	
	function removeSetups =
	(
		for i = 1 to objects.count do
		(
			try
			(
				local tempObj = objects[i]
				local custAttr = custAttributes.get tempObj.modifiers[1] 1
				if custAttr.name == "removeWeapon" then
					custAttr.deleteMe()
			)
			catch()
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	function callLoadSetup useMerge:false usePrefer:false noPrompt:false =
	(
		local charID = cryAM._.cbCharacter.selectedIndex
		local setupID = cryAM._.cbSetup.selectedIndex + 1
		
		if charID > 0 and setupID > 0 then
		(
			local setup = characterArray[charID].setup[setupID]
			
			local useLoad = true
				
			if usePrefer == false then
			(
				if useMerge == true then
					useLoad = false
			)
			else
				useLoad = setup.model.preferLoad
			
			
			local ready = false
			
			if noPrompt == false then
			(
				if useLoad == true then
					ready = queryBox ("Load " + setup.name + " ?") == true
				else
					ready = queryBox ("Merge " + setup.name + " ?") == true
			)
			else
				ready = true
			
			
			if ready == true then
			(
				if setup.model.autoSync == true and setup.model.path != "" then
					cryMaxTools.basic.perforceMan.getLatestRev (rootProjectPath + setup.model.path)
				
				if setup.model.removeOld == true then
					removeSetups()
				
				if (doesFileExist (rootProjectPath + setup.model.path)) then
				(
					if useLoad == true then
						loadMaxFile (rootProjectPath + setup.model.path) quiet:true 
					else
						mergeMaxFile (rootProjectPath + setup.model.path) #useMergedMtlDups #alwaysReparent #mergeDups quiet:true
				)
				
				
				if setup.biped.autoSync == true and setup.biped.path != "" then
					cryMaxTools.basic.perforceMan.getLatestRev (rootProjectPath + setup.biped.path)
				
				
				local bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
				for i = 1 to bipRoots.count do
				(
					if setup.biped.removeOld == true then
						biped.deleteAllCopyCollections bipRoots[i].controller
					
					if (doesFileExist (rootProjectPath + setup.biped.path)) then
						biped.loadCopyPasteFile bipRoots[i].controller (rootProjectPath + setup.biped.path)
				)
				
				
				local sourceNode = getNodeByName setup.align.sourceNode
				local targetNode = getNodeByName setup.align.targetNode
				
				if sourceNode != undefined and targetNode != undefined then
				(
					local tempTM = matrix3 1
					tempTM.rotation = setup.align.rot
					tempTM.pos = setup.align.pos
					
					with animate off
						sourceNode.transform = tempTM * targetNode.transform
					
					sourceNode.parent = targetNode
					hide sourceNode
				)
				
				if setupPresetRecord == true then
					append setupPresetRecordArray (cryAM_setupPresetStruct charID:(cryAM._.cbCharacter.selectedIndex + 1) charName:cryAM._.cbCharacter.selectedItem setupID:(cryAM._.cbSetup.selectedIndex + 1) setupName:cryAM._.cbSetup.selectedItem)
			)
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	
	
	
	
	
	
	function refreshAnimList =
	(
		for i = 1 to animArray.count do
		(
			if animArray[i].selected == true and animArray[i].type != "scene" and animArray[i].type != "folder" and animArray[i].type != "" then
			(
				local perforceStatus = cryMaxTools.basic.perforceMan.getFileStats animArray[i].path
				
				local tempStyle = dotNetObject "System.Windows.Forms.DataGridViewCellStyle"
				
				if perforceStatus.haveRev != undefined then
				(
					case of
					(
						((perforceStatus.haveRev as Integer) < (perforceStatus.headRev as Integer)):	tempStyle.backColor = tempStyle.backColor.fromArgb 255 255 100
						((perforceStatus.haveRev as Integer) == (perforceStatus.headRev as Integer)):	tempStyle.backColor = tempStyle.backColor.fromArgb 200 255 200
					)
				)
				
				if perforceStatus.exist == false then
					tempStyle.backColor = tempStyle.backColor.fromArgb 255 200 200
				
				
				animArray[i].status = ""
				
				
				if perforceStatus.otherOpen != undefined then
					animArray[i].status = "@ " + perforceStatus.otherOpen as String
				
				if perforceStatus.userOpen != undefined then
					animArray[i].status = "@ you"
				
				
				
				((cryAM._.lvNodes.rows.item animArray[i].node.rowIndex).cells.item 2).value = animArray[i].status
				((cryAM._.lvNodes.rows.item animArray[i].node.rowIndex).cells.item 2).style = tempStyle
			)
		)
		
		--gc light:true
	),
	
	
	
	
	
	
	

	
	
	
	
	
	function exploreAnims mode = 
	(
		struct cryAM_tempFolderStruct (folder, nodes = #())
		
		local folderArray = #()
		
		
		
		for i = 1 to animArray.count do
		(
			if animArray[i].path != "" and animArray[i].selected == true then
			(
				case mode of
				(
					#source:	local usedAnim = animArray[i].path
					#export:		local usedAnim = getExportPath animArray[i].path "caf"
				)
				
				local tempFilter = filterString usedAnim "\\"
				local tempDir = ""
				for f = 1 to (tempFilter.count - 1) do
					tempDir += tempFilter[f] + "\\"-- + (if f < tempFilter.count - 1 then "\\" else "")
				
				
				
				local foundFolder = false
				
				for f = 1 to folderArray.count do
					if folderArray[f].folder == tempDir then
						foundFolder = true
					
				if foundFolder == false then
					append folderArray (cryAM_tempFolderStruct folder:tempDir)
				
				append folderArray[folderArray.count].nodes usedAnim --tempFilter[tempFilter.count]
			)
		)
		
		
		for i = 1 to folderArray.count do
		(
			local selectString = ""
			
			for d = 1 to folderArray[i].nodes.count do
				selectString += "\"" + folderArray[i].nodes[d] + "\"" + (if d < folderArray[i].nodes.count then "," else "")
			
			local paramString = "/select," + selectString
			
			
			shellLaunch "Explorer.exe" paramString
		)
		
		--gc light:true
	),
	
	
	function setClipboardPath op =
	(
		local copyString = ""
		
		for i = 1 to animArray.count do
		(
			if animArray[i].selected == true then
			(
				copyString += case op of
				(
					#animName:			animArray[i].name + "\n"
					#localSource:			animArray[i].path + "\n"
					#localGame:			(getExportPath animArray[i].path "caf") + "\n"
					#perforceSource:		(cryMaxTools.basic.perforceMan.localToDepot animArray[i].path) + "\n"
					#perforceGame:		(cryMaxTools.basic.perforceMan.localToDepot (getExportPath animArray[i].path "caf")) + "\n"
				)
			)
		)
		
		setclipboardText copyString
		
		--gc light:true
	),
	
	
	
	function syncAnim =
	(
		for i = 1 to animArray.count do
		(
			if animArray[i].selected == true and animArray[i].type != "scene" and animArray[i].type != "folder" and animArray[i].type != "" then
			(
				local perforceStatus = cryMaxTools.basic.perforceMan.getFileStats animArray[i].path
				
				local tempStyle = ((cryAM._.lvNodes.rows.item animArray[i].node.rowIndex).cells.item 2).style
				
				if perforceStatus.haveRev != undefined then
				(
					cryMaxTools.basic.perforceMan.getLatestRev animArray[i].path
					animArray[i].status = "synched"
					tempStyle.backColor = tempStyle.backColor.fromArgb 200 255 200
				)
				else
					tempStyle.backColor = tempStyle.backColor.fromArgb 255 200 200
				
				((cryAM._.lvNodes.rows.item animArray[i].node.rowIndex).cells.item 2).value = animArray[i].status
				((cryAM._.lvNodes.rows.item animArray[i].node.rowIndex).cells.item 2).style = tempStyle
			)
		)
	),
	
	
	
	
	function callRightMouseClickNodes =
	(
		RCMenu rightMouseClickNodesRC
		(
			fn perforceActive = cryMaxTools.anim.vars.usePerforce == true
			
			menuItem miLoadAnim "Load" enabled:true
			menuItem miSaveAnim "Save" enabled:true
			seperator sep1
			menuItem miLoadAnimFrom "Load From"
			menuItem miSaveAnimAs "Save As" enabled:true
			separator sep12
			menuItem miRefreshAnimList "Refresh"
			seperator sep11
			subMenu "Tools"
			(
				subMenu "Explore"
				(
					menuItem miExplore "Source" enabled:true
					menuItem miExploreGame "Game" enabled:true
				)
				seperator sep2
				subMenu "Perforce"
				(
					subMenu "Add / Open"
					(
						menuItem miAddOpenSource "Source" enabled:true
						menuItem miAddOpenGame "Game" enabled:true
					)
					menuItem miSync "Force Sync" enabled:true
					menuItem miRefreshPerforce "Update" enabled:true
					seperator sep3
					menuItem miUsePerforceLoad "Add / Open on Load" checked:false
					menuItem miUsePerforceSave "Add / Open on Save" checked:true
				)
				seperator sep4
				subMenu "Copy"
				(
					menuItem miCopyAnimName "Animation Name"
					seperator sep5
					menuItem miCopyLocalSourcePath "Local Source Path"
					menuItem miCopyLocalGamePath "Local Game Path"
					seperator sep6
					menuItem miCopyPerforceSourcePath "Perforce Source Path"
					menuItem miCopyPerforceGamePath "Perforce Game Path"
				)
			)
			seperator sep7
			subMenu "Options"
			(
				menuItem miOneClickLoad "One-Click Load" checked:false
				separator sep8
				menuItem miShowSubAnims "Show SubAnims" checked:true
				separator sep9
				menuItem miAnimPreview "Show Preview" checked:true
				menuItem miShowCBADetails "Show CBA Details" checked:false
				separator sep10
				menuItem miFilterFullPath "Search full Path" checked:true
			)
			
			
			
			
			
			
			
			
			
			
			on rightMouseClickNodesRC open do
			(
				cryAM.updateAnimListMenu rightMouseClickNodesRC
				--gc light:true
			)

			
			
			
			
			on miLoadAnim picked do
			(
				local ready = true
		
				if cryAM._.cbLoad.selectedIndex > 0 then
					ready = (queryBox ("Start Process:\n\nLoad:   " + cryAM._.cbLoad.selectedItem + " ?") title:"Animation Manager                         ")
				
				if ready == true then
					cryAM.callLoadAnim()
				
				--gc light:true
			)
			
			
			on miLoadAnimFrom picked do
			(
				local ready = true
				
				if acryAM._.cbLoad.selectedIndex > 0 then
					ready = (queryBox ("Start Process:\n\nLoad:   " + cryAM._.cbLoad.selectedItem + " ?") title:"Animation Manager                         ")
				
				if ready == true then
					cryAM.callLoadAnim askLoad:true
			)
			
			
			
			
			on miSaveAnim picked do
			(
				local animsSelected = 0
				local ready = true
				
				for i = 1 to cryAM.animArray.count do
					if cryAM.animArray[i].selected == true then
						animsSelected += 1
				
				if animsSelected > 1 then
					ready = (queryBox ("Save Process:\n\nLoad:  " + cryAM._.cbLoad.selectedItem + "\nSave:  " + cryAM._.cbSave.selectedItem) title:"Animation Manager                         ")
					
				if ready == true then
					cryAM.callSaveAnim()
				
				--gc light:true
			)
			
			
			on miSaveAnimAs picked do
			(
				cryAM.callSaveAnim askSave:true
			)
			
			
			
			on miRefreshAnimList picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.updateNodeList keepSelection:true
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			
			
			
			
			
			
			
			
			on miExplore picked do
				cryAM.exploreAnims #source
			
			
			on miExploreGame picked do
				cryAM.exploreAnims #export
			
			
			on miAddOpenSource picked do
			(
				for i = 1 to cryAM.animArray.count do
				(
					if cryAM.animArray[i].selected == true and cryAM.animArray[i].type != "scene" and cryAM.animArray[i].type != "folder" and cryAM.animArray[i].type != "" then
					(
						cryMaxTools.basic.perforceMan.cryAdd cryAM.animArray[i].path
						cryMaxTools.basic.perforceMan.open cryAM.animArray[i].path
					)
				)
				
				cryAM.refreshAnimList()
			)
			
			
			
			on miAddOpenGame picked do
			(
				for i = 1 to cryAM.animArray.count do
				(
					if cryAM.animArray[i].selected == true and cryAM.animArray[i].type != "scene" and cryAM.animArray[i].type != "folder" and cryAM.animArray[i].type != "" then
					(
						cryMaxTools.basic.perforceMan.cryAdd (cryAM.getExportPath cryAM.animArray[i].path "caf")
						cryMaxTools.basic.perforceMan.open (cryAM.getExportPath cryAM.animArray[i].path "caf")
					)
				)
			)
			
			
			
			
			
			on miSync picked do
			(
				cryAM.syncAnim()
				
				--gc light:true
			)
			
			
			
			on miRefreshPerforce picked do
			(
				cryAM.refreshAnimList()
			)
			
			on miCopyAnimName picked do
				cryAM.setClipboardPath #animName
			
			
			on miCopyLocalSourcePath picked do
				cryAM.setClipboardPath #localSource
			
			
			on miCopyLocalGamePath picked do
				cryAM.setClipboardPath #localGame
			
			
			on miCopyPerforceSourcePath picked do
				cryAM.setClipboardPath #perforceSource
			
			
			on miCopyPerforceGamePath picked do
				cryAM.setClipboardPath #perforceGame
			
			
			on miOneClickLoad picked do
				cryAM.oneClickLoad = not cryAM.oneClickLoad
			
			
			on miUsePerforceLoad picked do
				cryMaxTools.anim.vars.usePerforceLoad = not cryMaxTools.anim.vars.usePerforceLoad
			
			on miUsePerforceSave picked do
				cryMaxTools.anim.vars.usePerforceSave = not cryMaxTools.anim.vars.usePerforceSave
			
			on miAnimPreview picked do
				cryAM.animPreview = not cryAM.animPreview
			
			on miFilterFullPath picked do
			(
				cryAM.filterFullPath = not cryAM.filterFullPath
				cryAM.updateNodeList keepSelection:true
			)
			
			on miShowSubAnims picked do
			(
				cryAM.callUIUpdateDialog()
				cryAM.showSubAnims = not cryAM.showSubAnims
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			on miShowCBADetails picked do
			(
				cryAM.callUIUpdateDialog()
				cryAM.showCBADetails = not cryAM.showCBADetails
				cryAM.updateNodeList keepSelection:true --onlyBackground:true
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
		)
		
		--registerRightClickMenu rightMouseClickNodesRC

		popUpMenu rightMouseClickNodesRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function callSetupMenu =
	(
		RCMenu setupMenuRC
		(
			menuItem miLoad "Load Setup" enabled:true
			menuItem miMerge "Merge Setup" enabled:true
			seperator sep1
			menuItem miRemove "Remove Setups" enabled:true
			seperator sep2
			subMenu "Preset"
			(
				menuItem miLoadPreset "Load Preset" enabled:true
				seperator sep3
				menuItem miStartRecord "Record"
				menuItem miStopRecord "Stop" enabled:true
			)
			--seperator sep4
			--menuItem miManage "Manage"
			
			on setupMenuRC open do
			(
				miLoadPreset.enabled = cryAM.setupPresetArray.count != 0
				miStartRecord.enabled = not cryAM.setupPresetRecord
				miStopRecord.enabled = cryAM.setupPresetRecord
			)
			
			
			on miLoad picked do
				cryAM.callLoadSetup()
			
			
			on miMerge picked do
				cryAM.callLoadSetup useMerge:true
			
			
			on miRemove picked do
				cryAM.removeSetups()
			
			
			on miGetLatest picked do
				cryAM.getLatestCharModel = not cryAM.getLatestCharModel
			
			
			
			on miOpen picked do
				cryAM.loadSettings mode:taskMode
			
			on miSave picked do
			(
				if queryBox "Overwrite existing file?" title:("Save Character Settings") == true then
					cryAM.saveSettings mode:#character
			)
			
			on miSaveAs picked do
				cryAM.saveSettings mode:#character saveAs:true
			
			
			on miLoadPreset picked do
			(
				
				if cryAM.setupPresetArray.count > 0 then
				(
					for i = 1 to cryAM.setupPresetArray.count do
					(
						
						cryAM.onUpdate = true
						cryAM._.cbCharacter.selectedIndex = (cryAM.setupPresetArray[i].charID - 1)
						cryAM.onUpdate = false
						
						cryAM.updateByCharacter skipLastSel:true
						
						cryAM.onUpdate = true
						cryAM._.cbSetup.selectedIndex = cryAM.setupPresetArray[i].setupID - 1
						cryAM.onUpdate = false
						
						cryAM.updateBySetup skipNodeList:true
						cryAM.updateByLastSel()
						
						cryAM.callLoadSetup usePrefer:true noPrompt:true
					)
					
					cryAM.updateByLastSel()
				)
			)
			
			
			on miStartRecord picked do
			(
				if queryBox "Start Recording?" title:"Animation Manager Setup Preset" == true then
				(
					cryAM.setupPresetRecordArray = #()
					cryAM.setupPresetRecord = true
				)
			)
			
			
			
			on miStopRecord picked do
			(
				cryAM.setupPresetRecord = false
				cryAM.setupPresetArray = cryAM.setupPresetRecordArray
				cryAM.saveSettings mode:#user
				messageBox "Stopped Recording" title:"Animation Manager Setup Preset"
			)
			
			
			
			
			on miManage picked do
			(
				cryAM.callSetupRO()
			)
		)
		
		--registerRightClickMenu setupMenuRC

		popUpMenu setupMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
	
	
	
	
	
	
	
	
	
	function callTaskRO =
	(
		fileIn "AnimManager_task.ms"
	),
	
	
	
	
	
	
	
	
	
	/*
	function callTaskMenu =
	(
		RCMenu taskMenuRC
		(
			fn isLoad = (animManRO.taskMode == #import)
			fn isSave = (animManRO.taskMode == #export)
			fn isLoadOrSave = (animManRO.taskMode == #import or animManRO.taskMode == #export)
			
			menuItem miLoadAnim "Load" filter:isLoad
			menuItem miSaveAnim "Save" filter:isSave
			seperator sep1 filter:isLoadOrSave
			menuItem miManage "Manage"
			
			
			on miOpen picked do
			(
				loadSettings mode:taskMode
			)
			
			on miSave picked do
			(
				if queryBox "Overwrite existing file?" title:("Save " + taskMode as String " Settings") == true then
					saveSettings mode:taskMode
			)
			
			on miSaveAs picked do
			(
				saveSettings mode:taskMode saveAs:true
			)
			
			
			
			
			
			on miManage picked do
			(
				try destroyDialog taskRO catch()
				callTaskRO()
			)
		)
		
		--registerRightClickMenu taskMenuRC

		popUpMenu taskMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
	*/
	
	
	
	
	
	
	function getUIItemDimensions item =
	(
		if item != undefined then
		(
			local itemName = "" ;	try itemName = item.name catch()
			local itemHeight = 0;	try itemHeight = item.height catch()
			local itemWidth = 0;	try itemWidth = item.width catch()
			local itemLeft = 0;	try itemLeft = item.left catch()
			local itemTop = 0;	try itemTop = item.top catch()
		
			if itemName != "" then
				return (cryAM_resizeStruct name:itemName item:item height:itemHeight width:itemWidth left:itemLeft top:itemTop)
		)
		
		--gc light:true
		
		return undefined
	),
	
	
	
	
	
	function setToolTipTimer op =
	(
		case op of
		(
			#start:
			(
				toolTip = dotNetObject "System.Windows.Forms.ToolTip"
				
				toolTip.OwnerDraw = false
				toolTip.InitialDelay = 200
				toolTip.ReshowDelay = 1
				toolTip.Active = false
				
				--//tmTimer.active = true
			)
			#stop:
			(
				toolTipMode = #none
				if toolTip != undefined then
					toolTip.Active = false
					
				--//tmTimer.active = false
				cryAM.labPreview.visible = false
				toolTip = undefined
			)
		)
	),
	
	
	
	
	function updateToolTip =
	(
		case toolTipMode of
		(
			#nodes:
			(
				if animArray.count >= lastHoveredNodeRow + 1 and lastHoveredNodeRow >= 0 then
				(
					
					toolTip.ToolTipTitle = animArray[lastHoveredNodeRow + 1].name
					toolTip.SetToolTip cryAM._.lvNodes (cryMaxTools.basic.str.getDirectory animArray[lastHoveredNodeRow + 1].path)
					toolTip.Active = true
					
					if labPreview.image != undefined then
					(
						labPreview.visible = true
						
						labPreview.width = labPreview.image.size.width
						labPreview.height = labPreview.image.size.height
						
						labPreview.location.x = mousePos[1] - labPreview.width - 10
						labPreview.location.y = mousePos[2] + 20
					)
				)
			)
			
			#DLU:
			(
				toolTip.ToolTipTitle = "Exporter .dlu Check"
				toolTip.SetToolTip cryAM._.labProjectInfoDLU DLUText
				toolTip.Active = true
			)
			
			#RC:
			(
				toolTip.ToolTipTitle = "RC Check"
				toolTip.SetToolTip cryAM._.labProjectInfoRC RCText
				toolTip.Active = true
			)
			
			#CBA:
			(
				toolTip.ToolTipTitle = "Animations.cba Check"
				toolTip.SetToolTip cryAM._.labProjectInfoCBA CBAText
				toolTip.Active = true
			)
			
			#CHR:
			(
				toolTip.ToolTipTitle = "Character .chr Check"
				toolTip.SetToolTip cryAM._.labProjectInfoCHR CHRText
				toolTip.Active = true
			)
		)
	),

	
	
	
	
	
	
	
	
	
	
	
	
	
	
		--//////////////////////////////////////////////////////////////////////////////
		--//
		--//		TASK STUFF
		--//
		--//////////////////////////////////////////////////////////////////////////////
	
	
	fn sortByName entry1 entry2 =
	(
		if entry1.name > entry2.name then 1
		else 0
	),
	
	
	
	function updateProcessTemplates =
	(
		local tempArray = #()
		
		qsort templateArray sortByName
		
		for i = 1 to templateArray.count do
			append tempArray templateArray[i].name
		
		--gc light:true
	),
	
	
	
	
	
	function updateTaskDimensions item forceUpdate:false =
	(	
		if item != undefined then
		(
			onUpdate = true
			
			if item.parent.height > 22 or forceUpdate == true then
			(
			
				try item.BeginUpdate() catch()
				
					--// handle listView \\--
				
				local mul = 14
				local headerAdd = 4
				local amount = item.items.count + 1
				
				if item.checkBoxes == true then
					mul = 17
				
				
				
				local newHeight = (amount * mul) + headerAdd
				
				item.height = newHeight
				
				case item.name of
				(
					"lvTasks":
					(
						cryAM._.pTaskBack.height = newHeight + 2
						cryAM._.pTask.height = newHeight
						
						cryAM.setRoundRecRegion cryAM._.pTaskBack 10
						cryAM.setRoundRecRegion cryAM._.pTask 10
					)
					"lvProcess":
					(
						cryAM._.pProcessBack.height = newHeight + 2
						cryAM._.pProcess.height = newHeight
						
						cryAM.setRoundRecRegion cryAM._.pProcessBack 10
						cryAM.setRoundRecRegion cryAM._.pProcess 10
					)
					"lvProperties":
					(
						newHeight += 100
						cryAM._.pPropertiesBack.height = newHeight + 2
						cryAM._.pProperties.height = newHeight
						
						cryAM.setRoundRecRegion cryAM._.pPropertiesBack 10
						cryAM.setRoundRecRegion cryAM._.pProperties 10
					)
				)
				
				try item.EndUpdate() catch()
			)
			
			onUpdate = false
		)
	),
	
	
	
	
	
	
	
	
	
	function updatePropertyValue =
	(
		onUpdate = true
		
		cryAM._.edPropertyValue.text = ""
		cryAM._.spnPropertyValueInteger.value = 0
		cryAM._.spnPropertyValueFloat.value = 0.0
		cryAM._.cbPropertyValue.selectedIndex = 0
		
		cryAM._.edPropertyValue.visible = false
		cryAM._.cbPropertyValue.visible = false
		cryAM._.spnPropertyValueInteger.visible = false
		cryAM._.spnPropertyValueFloat.visible = false
		
		cryAM._.labPropertyType.text = "Type:      None"
		
		if cryAM._.lvProperties.selectedItems.count > 0 then
		(
			local tempEntry = ((cryAM._.lvProperties.selectedItems.item 0).subItems.item 0)
			if tempEntry.text != undefined and tempEntry.text != "function" then
			(
				cryAM._.edPropertyValue.enabled = true
				cryAM._.cbPropertyValue.enabled = true
				cryAM._.spnPropertyValueInteger.enabled = true
				cryAM._.spnPropertyValueFloat.enabled = true
				
				
				
				local usedClass = execute ("classOf " + tempEntry.text)
				local usedElement = undefined
				
				case usedClass of
				(
					Integer:			( usedElement = cryAM._.spnPropertyValueInteger ; usedElement.value = (execute tempEntry.text) )
					Float:			( usedElement = cryAM._.spnPropertyValueFloat ; usedElement.value = (execute tempEntry.text) )
					BooleanClass:	( usedElement = cryAM._.cbPropertyValue ; usedElement.selectedIndex = (if (execute tempEntry.text) == true then 0 else  1 ))
					String:			( usedElement = cryAM._.edPropertyValue ; usedElement.text = (execute tempEntry.text) )
				)
				
				
				usedElement.visible = true
				
				
				cryAM._.labPropertyType.text = try "Type:      " + usedClass as String catch("Type:      unknown")
			)
			else
			(
				cryAM._.edPropertyValue.enabled = false
				cryAM._.cbPropertyValue.enabled = false
				cryAM._.cbPropertyValue.visible = true
				cryAM._.spnPropertyValueInteger.enabled = false
				cryAM._.spnPropertyValueFloat.enabled = false
			)
		)
		
		onUpdate = false
		
		--gc light:true
	),
	
	
	
	
	
	
	function updateProperties forceUpdate:false keepSelection:false =
	(
		onUpdate = true
		
		local safeSel = -1
		try safeSel = (cryAM._.lvProperties.selectedItems.item 0).index catch()
		
		cryAM._.lvProperties.items.clear()
		
		if keepSelection == false then
			cryAM._.edPropertyValue.text = ""
		
		local abort = true
		
		if lastTask > 0 and lastProcess > 0 then
			if taskArray[lastTask] != undefined then
				if taskArray[lastTask].process[lastProcess] != undefined then
					abort = false
		
		local propIndex = 0
			
		if abort == false then
		(		
			if taskArray[lastTask].process[lastProcess].fnData.count == 0 or forceUpdate == true then
			(
				taskArray[lastTask].process[lastProcess].fnData = #()
				
				local fnDataArray = #()
				
				local tempStream = stringStream cryAM._.edScript.text
				
				
				while eof tempStream == false do
				(
					local functionLine = readLine tempStream
					
					if findString functionLine "function " != undefined or findString functionLine "fn " != undefined then
					(
						local paramArray = #()
						local functionString = functionLine
						
						if (local equalSign = findString functionLine "=") != undefined then
							functionString = subString functionLine 1 (equalSign - 1)
						
						
						local paramFilter = filterString functionString " "
						local functionName = paramFilter[2]
						local functionIndex = 0
						
						if functionName != undefined then
						(
							local tempEntry = cryAM._.lvProperties.items.add "function" "function"
							tempEntry.subItems.add functionName
							tempEntry.ForeColor = tempEntry.ForeColor.fromARGB 200 0 0
							
							functionIndex = propIndex
							
							propIndex += 1
						)
						
						for i = 3 to paramFilter.count do
						(
							local optionalParam = filterString paramFilter[i] ":"
							if optionalParam.count == 1 then
								append paramArray (cryAM_propertiesStruct name:optionalParam[1] value:"" index:propIndex)
							else
								append paramArray (cryAM_propertiesStruct name:optionalParam[1] value:optionalParam[2] index:propIndex)
								
							
							propIndex += 1
						)
						
						
						for i = 1 to paramArray.count do
						(
							local newParam = ""
							if paramArray[i].value != "undefined" then
								newParam = paramArray[i].value
							
							local tempEntry = cryAM._.lvProperties.items.add newParam newParam
							tempEntry.backColor = tempEntry.backColor.fromARGB 240 255 255
							tempEntry.subItems.add paramArray[i].name
							
						)
						
						
						append taskArray[lastTask].process[lastProcess].fnData (cryAM_functionStruct name:functionName properties:paramArray index:functionIndex)
					)
				)
				
				
				
				close tempStream
			)
			else
			(
				for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
				(
					local tempEntry = cryAM._.lvProperties.items.add "function" "function"
					tempEntry.subItems.add taskArray[lastTask].process[lastProcess].fnData[i].name
					tempEntry.ForeColor = tempEntry.ForeColor.fromARGB 200 0 0
					
					for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
					(
						local tempEntry = cryAM._.lvProperties.items.add taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value
						tempEntry.backColor = tempEntry.backColor.fromARGB 240 255 255
						tempEntry.subItems.add taskArray[lastTask].process[lastProcess].fnData[i].properties[d].name
						
					)
				)
			)
			
			if keepSelection == true then
				try (cryAM._.lvProperties.items.item safeSel).selected = true catch()
		)
		
		onUpdate = false
		
		updateTaskDimensions cryAM._.lvProperties
		
		--gc light:true
	),
	
	
	
	
	
	function updateScript =
	(
		onUpdate = true
		cryAM._.edScript.text = ""
		
		if lastTask > 0 and lastProcess > 0 then
			if taskArray[lastTask] != undefined then
				if taskArray[lastTask].process[lastProcess] != undefined then
					cryAM._.edScript.text = taskArray[lastTask].process[lastProcess].script.text
				
		onUpdate = false
		
		updateProperties()
	),
	
	
	
	
	
	
	function updateProcess =
	(
		onUpdate = true
		cryAM._.lvProcess.items.clear()
		
		if lastTask > 0 and lastTask <= taskArray.count then
		(
			if taskArray[lastTask] != undefined then
			(
				for i = 1 to taskArray[lastTask].process.count do
				(
					local newEntry = cryAM._.lvProcess.items.add taskArray[lastTask].process[i].name
					newEntry.backColor = newEntry.backColor.fromARGB 255 255 200
					newEntry.checked = taskArray[lastTask].process[i].active
				)
				
				if lastProcess > 0 and lastProcess <= cryAM._.lvProcess.items.count then
					(cryAM._.lvProcess.items.item (lastProcess - 1)).selected = true
			)
		)
		
		onUpdate = false
		
		updateScript()
		
		updateTaskDimensions cryAM._.lvProcess
		--gc light:true
	),
	
	
	
	
	function updateTasks =
	(
		onUpdate = true
		cryAM._.lvTasks.items.clear()
		
		for i = 1 to taskArray.count do
		(
			local newEntry = cryAM._.lvTasks.items.add taskArray[i].name
			newEntry.checked = taskArray[i].active
			
			if taskArray[i].default == true then
				newEntry.backColor = newEntry.backColor.fromARGB 230 255 230
			else
				newEntry.backColor = newEntry.backColor.fromARGB 255 240 240
			
			
		)
		
		if lastTask > 0 and lastTask <= cryAM._.lvTasks.items.count then
			(cryAM._.lvTasks.items.item (lastTask - 1)).selected = true
		
		onUpdate = false
		lastProcess = 1
		
		updateProcess()
				
		updateTaskDimensions cryAM._.lvTasks
				
		--gc light:true
	),
	
	
	
	
	
	
	
	function callTaskMenu =
	(
		RCMenu taskRightMouseClickRC
		(
			menuItem miNew "New" enabled:true
			seperator sep1
			menuItem miRun "Run" enabled:true
			seperator sep2
			menuItem miCopy "Copy" enabled:true
			menuItem miPaste "Paste" enabled:true
			separator sep3
			menuItem miSwitchDefault "Is User" enabled:true checked:false
			seperator sep4
			menuItem miDelete "Delete" enabled:true
			
			
			
			function newEntry =
			(
				local newName = "newTask01"
				local newNameMaxCount = 0
				
				
				for i = 1 to 50 do
				(
					local tempName = "newTask" + (if i < 10 then "0" else "") + i as String
					
					for f = 1 to cryAM.taskArray.count do
						if (cryAM.taskArray[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newTask" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				append cryAM.taskArray (cryAM_taskStruct name:newName)
				append cryAM.taskArray[cryAM.taskArray.count].process (cryAM_processStruct name:"newProcess01")
				
				cryAM.lastTask = cryAM.taskArray.count
				
				cryAM.onUpdate = true
				cryAM.updateTasks()
				
				--gc light:true
			)
			
			
			
			
			on taskRightMouseClickRC open do
			(
				if cryAM._.lvTasks.items.count == 0 then
				(
					miRun.enabled = false
					miDelete.enabled = false
					miCopy.enabled = false
					miPaste.enabled = false
					miSwitchDefault.enabled = false
				)
				else
				(
					miDelete.enabled = true
					
					if cryAM.lastTask > 0 then
						miSwitchDefault.checked = not cryAM.taskArray[cryAM.lastTask].default
					
					if cryAM.copyTask == undefined then
						miPaste.enabled = false
				)
			)
			
			
			on miNew picked do
				newEntry()
			
			
			
			on miRun picked do
			(
				if cryAM.lastTask > 0 then
				(
					if cryAM.taskArray[cryAM.lastTask] != undefined then
					(
						for i = 1 to cryAM.taskArray[cryAM.lastTask].process.count do
						(
							if cryAM.taskArray[cryAM.lastTask].process[i].active == true then
							(
							
								local runScript = ""
								local scriptFound = ""
								
								runScript = cryAM.taskArray[cryAM.lastTask].process[i].script.text
								scriptFound = cryAM.taskArray[cryAM.lastTask].process[i]
								
								if scriptFound != undefined then
									scriptFound = "in " + scriptFound.name + ":  "
								
								try
								(
										--// tries executing the script
									execute( runScript )
									
									for f = 1 to cryAM.taskArray[cryAM.lastTask].process[i].fnData.count do
									(
										local execStr = cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].name
										
										if cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].properties.count > 0 then
										(
											for d = 1 to cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].properties.count do
												execStr += " " + cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].properties[d].name + ":" + cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].properties[d].value as String
										)
										else
											execStr += "()"
										try
											execute ( execStr )
										catch
										(
											format "*** % ***\n" (scriptFound + "  Error in Function:  " + cryAM.taskArray[cryAM.lastTask].process[i].fnData[f].name)
										)
										
									)
								)
								catch
								(
										--// if an error occured, print the error message
									format "*** % ***\n" (scriptFound + (getCurrentException()))
								)
							)
						)
					)
				)
				
				--gc light:true
			)
			
			
			
			
			
			
			on miCopy picked do
			(
				cryAM.copyTask = #()
				
				if cryAM.lastTask > 0 then
				(
					for i = 0 to (cryAM._.lvTasks.selectedItems.count - 1) do
					(
						local index = (cryAM._.lvTasks.selectedItems.item i).index + 1
						if cryAM.taskArray[index] != undefined then
							append cryAM.copyTask cryAM.taskArray[index]
					)
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				for i = 1 to cryAM.copyTask.count do
				(
					newEntry()
					
					if cryAM.copyTask != undefined then
						if cryAM.lastTask > 0 then
							if cryAM.taskArray[cryAM.lastTask] != undefined then
									cryAM.taskArray[cryAM.lastTask] = cryMaxTools.basic.misc.makeUnique cryAM.copyTask[i]
					
					cryAM.onUpdate = true
					cryAM.updateTasks()
				)
			)
			
			
			
			on miSwitchDefault picked do
			(
				for i = 0 to (cryAM._.lvTasks.selectedItems.count - 1) do
				(
					local index = (cryAM._.lvTasks.selectedItems.item i).index + 1
					cryAM.taskArray[index].default = miSwitchDefault.checked
				)
				
				cryAM.onUpdate = true
				
				cryAM.updateTasks()
			)
			
			
			
			
			
			
			on miDelete picked do
			(
				if lvTasks.selectedItems.count > 0 then
				(
					local newTaskArray = #()
					local delArray = #{}
					
					for i = 0 to (cryAM._.lvTasks.selectedItems.count - 1) do
						delArray[(cryAM._.lvTasks.selectedItems.item i).index + 1] = true
					
					for i = 1 to cryAM.taskArray.count do
					(
						if delArray[i] == false then
							append newTaskArray cryAM.taskArray[i]
					)
						
					cryAM.taskArray = newTaskArray
				)
				
				if cryAM.taskArray.count == 0 then
					cryAM.lastTask = 0
				else
					cryAM.lastTask = 1
		
				cryAM.onUpdate = true
				cryAM.updateTasks()
				
				--gc light:true
			)
		)
		
		popUpMenu taskRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
	
	
	
	
	
	
	function callProcessMenu =
	(
		RCMenu processRightMouseClickRC
		(
			fn isVisible1 = (templateArray.count >= 1 )
			fn isVisible2 = (templateArray.count >= 2 )
			fn isVisible3 = (templateArray.count >= 3 )
			fn isVisible4 = (templateArray.count >= 4 )
			fn isVisible5 = (templateArray.count >= 5 )
			fn isVisible6 = (templateArray.count >= 6 )
			fn isVisible7 = (templateArray.count >= 7 )
			fn isVisible8 = (templateArray.count >= 8 )
			fn isVisible9 = (templateArray.count >= 9 )
			fn isVisible10 = (templateArray.count >= 10 )
			fn isVisible11 = (templateArray.count >= 11 )
			fn isVisible12 = (templateArray.count >= 12 )
			fn isVisible13 = (templateArray.count >= 13 )
			fn isVisible14 = (templateArray.count >= 14 )
			fn isVisible15 = (templateArray.count >= 15 )
			fn isVisible16 = (templateArray.count >= 16 )
			fn isVisible17 = (templateArray.count >= 17 )
			fn isVisible18 = (templateArray.count >= 18 )
			fn isVisible19 = (templateArray.count >= 19 )
			fn isVisible20 = (templateArray.count >= 20 )
			fn isVisible21 = (templateArray.count >= 21 )
			fn isVisible22 = (templateArray.count >= 22 )
			fn isVisible23 = (templateArray.count >= 23 )
			fn isVisible24 = (templateArray.count >= 24 )
			fn isVisible25 = (templateArray.count >= 25 )
			fn isVisible26 = (templateArray.count >= 26 )
			fn isVisible27 = (templateArray.count >= 27 )
			fn isVisible28 = (templateArray.count >= 28 )
			fn isVisible29 = (templateArray.count >= 29 )
			fn isVisible30 = (templateArray.count >= 30 )
			
			
			
			menuItem miNew "New" enabled:true
			seperator sep1
			menuItem miRun "Run" enabled:true
			seperator sep2
			subMenu "Template"
			(
				subMenu "Get"
				(
					menuItem miGetTemplate1 filter:isVisible1
					menuItem miGetTemplate2 filter:isVisible2
					menuItem miGetTemplate3 filter:isVisible3
					menuItem miGetTemplate4 filter:isVisible4
					menuItem miGetTemplate5 filter:isVisible5
					menuItem miGetTemplate6 filter:isVisible6
					menuItem miGetTemplate7 filter:isVisible7
					menuItem miGetTemplate8 filter:isVisible8
					menuItem miGetTemplate9 filter:isVisible9
					menuItem miGetTemplate10 filter:isVisible10
					menuItem miGetTemplate11 filter:isVisible11
					menuItem miGetTemplate12 filter:isVisible12
					menuItem miGetTemplate13 filter:isVisible13
					menuItem miGetTemplate14 filter:isVisible14
					menuItem miGetTemplate15 filter:isVisible15
					menuItem miGetTemplate16 filter:isVisible16
					menuItem miGetTemplate17 filter:isVisible17
					menuItem miGetTemplate18 filter:isVisible18
					menuItem miGetTemplate19 filter:isVisible19
					menuItem miGetTemplate20 filter:isVisible20
					menuItem miGetTemplate21 filter:isVisible21
					menuItem miGetTemplate22 filter:isVisible22
					menuItem miGetTemplate23 filter:isVisible23
					menuItem miGetTemplate24 filter:isVisible24
					menuItem miGetTemplate25 filter:isVisible25
					menuItem miGetTemplate26 filter:isVisible26
					menuItem miGetTemplate27 filter:isVisible27
					menuItem miGetTemplate28 filter:isVisible28
					menuItem miGetTemplate29 filter:isVisible29
					menuItem miGetTemplate30 filter:isVisible30
				)
				seperator sep3
				subMenu "Remove"
				(
					menuItem miRemoveTemplate1 filter:isVisible1
					menuItem miRemoveTemplate2 filter:isVisible2
					menuItem miRemoveTemplate3 filter:isVisible3
					menuItem miRemoveTemplate4 filter:isVisible4
					menuItem miRemoveTemplate5 filter:isVisible5
					menuItem miRemoveTemplate6 filter:isVisible6
					menuItem miRemoveTemplate7 filter:isVisible7
					menuItem miRemoveTemplate8 filter:isVisible8
					menuItem miRemoveTemplate9 filter:isVisible9
					menuItem miRemoveTemplate10 filter:isVisible10
					menuItem miRemoveTemplate11 filter:isVisible11
					menuItem miRemoveTemplate12 filter:isVisible12
					menuItem miRemoveTemplate13 filter:isVisible13
					menuItem miRemoveTemplate14 filter:isVisible14
					menuItem miRemoveTemplate15 filter:isVisible15
					menuItem miRemoveTemplate16 filter:isVisible16
					menuItem miRemoveTemplate17 filter:isVisible17
					menuItem miRemoveTemplate18 filter:isVisible18
					menuItem miRemoveTemplate19 filter:isVisible19
					menuItem miRemoveTemplate20 filter:isVisible20
					menuItem miRemoveTemplate21 filter:isVisible21
					menuItem miRemoveTemplate22 filter:isVisible22
					menuItem miRemoveTemplate23 filter:isVisible23
					menuItem miRemoveTemplate24 filter:isVisible24
					menuItem miRemoveTemplate25 filter:isVisible25
					menuItem miRemoveTemplate26 filter:isVisible26
					menuItem miRemoveTemplate27 filter:isVisible27
					menuItem miRemoveTemplate28 filter:isVisible28
					menuItem miRemoveTemplate29 filter:isVisible29
					menuItem miRemoveTemplate30 filter:isVisible30
				)
				
				seperator sep4
				menuItem miAddTemplate "Add / Update" enabled:true
				menuItem miRemoveAllTemplate "Remove All" enabled:true
			)
			seperator sep5
			menuItem miCopy "Copy" enabled:true
			menuItem miPaste "Paste" enabled:true
			seperator sep6
			menuItem miDelete "Delete" enabled:true
			
			
			
			function newEntry =
			(
				local newName = "newProcess01"
				local newNameMaxCount = 0
				
				
				for i = 1 to 50 do
				(
					local tempName = "newProcess" + (if i < 10 then "0" else "") + i as String
					
					for f = 1 to cryAM.taskArray[cryAM.lastTask].process.count do
						if (cryAM.taskArray[cryAM.lastTask].process[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newProcess" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				local newProcess = cryAM_processStruct name:newName
				newProcess.script.text = cryAM._.edScript.text
				
				append cryAM.taskArray[cryAM.lastTask].process newProcess
				
				
				cryAM.lastProcess = cryAM.taskArray[cryAM.lastTask].process.count
				
				cryAM.onUpdate = true
				cryAM.updateProcess()
				
				cryAM._.edScript.text = ""
				
				--gc light:true
			)
			
			
			function removeEntry index =
			(
				if queryBox ("Remove " + cryAM.templateArray[index].name + "?") == true then
				(
					deleteItem cryAM.templateArray index
					cryAM.updateProcessTemplates()
				)
				
				--gc light:true
			)
			
			
			
			function getEntry index =
			(
				if cryAM.lastTask > 0 then
				(
					if cryAM.taskArray[cryAM.lastTask] != undefined then
					(
						append cryAM.taskArray[cryAM.lastTask].process cryAM.templateArray[index] 
						
						cryAM.lastProcess = cryAM.taskArray[cryAM.lastTask].process.count
						
						cryAM.onUpdate = true
						cryAM.updateProcess()
					)
				)
				
				--gc light:true
			)
			
			
			
			
			on processRightMouseClickRC open do
			(
				if cryAM.lastTask == 0 then
					miNew.enabled = false
				
				if cryAM._.lvProcess.items.count == 0 then
				(
					miRun.enabled = false
					miCopy.enabled = false
					miDelete.enabled = false
				)
			
					
				if cryAM.copyProcess == undefined then
					miPaste.enabled = false
				
				
				
				if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
				(
					local foundEntry = 0
					
					for i = 1 to cryAM.templateArray.count do
					(
						if cryAM.templateArray[i].name == cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].name then
						(
							foundEntry = i
							exit
						)
					)
					
					if foundEntry > 0 then
						miAddTemplate.text = "Update"
					else
						miAddTemplate.text = "Add"
				)
				else
					miAddTemplate.enabled = false
				
				for i = 1 to 30 do
				(
					if cryAM.templateArray.count >= i then
					(
						local tempString = (".text = \"" + cryAM.templateArray[i].name + "\"")
						execute ("cryAM.processRightMouseClickRC." + "miGetTemplate" + i as String + tempString)
						execute ("cryAM.processRightMouseClickRC." + "miRemoveTemplate" + i as String + tempString)
					)
				)
				
				--gc light:true
			)
			
			
			
			
			on miNew picked do
				newEntry()
			
			
			
			on miRun picked do
			(
				local runScript = ""
				local scriptFound = ""
				
				if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
				(
					if cryAM.taskArray[cryAM.lastTask] != undefined then
					(
						if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
						(
							runScript = cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].script.text
							scriptFound = cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess]
						)
					)
				)
				
				if scriptFound != undefined then
					scriptFound = "in " + scriptFound.name + ":  "
						
				
				try
				(
						--// tries executing the script
					execute( runScript )
					
					for f = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData.count do
					(
						local execStr = cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].name
						
						if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].properties.count > 0 then
						(
							for d = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].properties.count do
								execStr += " " + cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].properties[d].name + ":" + cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].properties[d].value as String
						)
						else
							execStr += "()"
						try
							execute ( execStr )
						catch
						(
							format "*** % ***\n" (scriptFound + "  Error in Function:  " + cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[f].name)
						)
						
					)
				)
				catch
				(
						--// if an error occured, print the error message
					format "*** % ***\n" (scriptFound + (getCurrentException()))
				)
				
				--gc light:true
			)
			
			
			
			
			
			on miAddTemplate picked do
			(
				if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
				(
					if cryAM.taskArray[cryAM.lastTask] != undefined then
					(
						if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
						(
							local foundEntry = 0
							
							for i = 1 to cryAM.templateArray.count do
							(
								if cryAM.templateArray[i].name == cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].name then
								(
									foundEntry = i
									exit
								)
							)
							
							if queryBox ((if foundEntry == 0 then "Add" else "Update") + " selected Process to the Templates?") == true then
							(
								if foundEntry == 0 then
									append cryAM.templateArray cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess]
								else
									cryAM.templateArray[foundEntry] = cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess]
								
								cryAM.updateProcessTemplates()
							)
						)
					)
				)
				
				--gc light:true
			)
			
			
			
			
			
			on miGetTemplate1 picked do	getEntry 1
			on miGetTemplate2 picked do	getEntry 2
			on miGetTemplate3 picked do	getEntry 3
			on miGetTemplate4 picked do	getEntry 4
			on miGetTemplate5 picked do	getEntry 5
			on miGetTemplate6 picked do	getEntry 6	
			on miGetTemplate7 picked do	getEntry 7
			on miGetTemplate8 picked do	getEntry 8
			on miGetTemplate9 picked do	getEntry 9
			on miGetTemplate10 picked do	getEntry 10
			on miGetTemplate11 picked do	getEntry 11
			on miGetTemplate12 picked do	getEntry 12
			on miGetTemplate13 picked do	getEntry 13
			on miGetTemplate14 picked do	getEntry 14
			on miGetTemplate15 picked do	getEntry 15
			on miGetTemplate16 picked do	getEntry 16
			on miGetTemplate17 picked do	getEntry 17
			on miGetTemplate18 picked do	getEntry 18
			on miGetTemplate19 picked do	getEntry 19
			on miGetTemplate20 picked do	getEntry 20
			on miGetTemplate21 picked do	getEntry 21
			on miGetTemplate22 picked do	getEntry 22
			on miGetTemplate23 picked do	getEntry 23
			on miGetTemplate24 picked do	getEntry 24
			on miGetTemplate25 picked do	getEntry 25
			on miGetTemplate26 picked do	getEntry 26
			on miGetTemplate27 picked do	getEntry 27
			on miGetTemplate28 picked do	getEntry 28
			on miGetTemplate29 picked do	getEntry 29
			on miGetTemplate30 picked do	getEntry 30
				
			
			
			on miRemoveTemplate1 picked do	removeEntry 1
			on miRemoveTemplate2 picked do	removeEntry 2
			on miRemoveTemplate3 picked do	removeEntry 3
			on miRemoveTemplate4 picked do	removeEntry 4
			on miRemoveTemplate5 picked do	removeEntry 5
			on miRemoveTemplate6 picked do	removeEntry 6	
			on miRemoveTemplate7 picked do	removeEntry 7
			on miRemoveTemplate8 picked do	removeEntry 8
			on miRemoveTemplate9 picked do	removeEntry 9
			on miRemoveTemplate10 picked do	removeEntry 10
			on miRemoveTemplate11 picked do	removeEntry 11
			on miRemoveTemplate12 picked do	removeEntry 12
			on miRemoveTemplate13 picked do	removeEntry 13
			on miRemoveTemplate14 picked do	removeEntry 14
			on miRemoveTemplate15 picked do	removeEntry 15
			on miRemoveTemplate16 picked do	removeEntry 16
			on miRemoveTemplate17 picked do	removeEntry 17
			on miRemoveTemplate18 picked do	removeEntry 18
			on miRemoveTemplate19 picked do	removeEntry 19
			on miRemoveTemplate20 picked do	removeEntry 20
			on miRemoveTemplate21 picked do	removeEntry 21
			on miRemoveTemplate22 picked do	removeEntry 22
			on miRemoveTemplate23 picked do	removeEntry 23
			on miRemoveTemplate24 picked do	removeEntry 24
			on miRemoveTemplate25 picked do	removeEntry 25
			on miRemoveTemplate26 picked do	removeEntry 26
			on miRemoveTemplate27 picked do	removeEntry 27
			on miRemoveTemplate28 picked do	removeEntry 28
			on miRemoveTemplate29 picked do	removeEntry 29
			on miRemoveTemplate30 picked do	removeEntry 30
			
			
			
			
			
			
			on miDelete picked do
			(
				if cryAM._.lvProcess.selectedItems.count > 0 then
				(
					local newProcessArray = #()
					local delArray = #{}
					
					for i = 0 to (cryAM._.lvProcess.selectedItems.count - 1) do
						delArray[(cryAM._.lvProcess.selectedItems.item i).index + 1] = true
					
					for i = 1 to cryAM.taskArray[cryAM.lastTask].process.count do
					(
						if delArray[i] == false then
							append newProcessArray cryAM.taskArray[cryAM.lastTask].process[i]
					)
						
					cryAM.taskArray[cryAM.lastTask].process = newProcessArray
				)
				
				if cryAM.taskArray[cryAM.lastTask].process.count == 0 then
					cryAM.lastProcess = 0
				else
					cryAM.lastProcess = 1
		
				cryAM.onUpdate = true
				cryAM.updateProcess()
				
				--gc light:true
			)
			
			
			on miCopy picked do
			(
				cryAM.copyProcess = #()
				
				if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
				(
					for i = 0 to (cryAM._.lvProcess.selectedItems.count - 1) do
					(
						local index = (cryAM._.lvProcess.selectedItems.item i).index + 1
						if cryAM.taskArray[cryAM.lastTask] != undefined then
							if cryAM.taskArray[cryAM.lastTask].process[index] != undefined then
								append cryAM.copyProcess cryAM.taskArray[cryAM.lastTask].process[index]
					)
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				for i = 1 to cryAM.copyProcess.count do
				(
					newEntry()
					
					if cryAM.copyProcess != undefined then
						if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
							if cryAM.taskArray[cryAM.lastTask] != undefined then
								if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
									cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] = cryMaxTools.basic.misc.makeUnique cryAM.copyProcess[i]
								
					cryAM.onUpdate = true
					cryAM.updateProcess()
				)
			)
		)
		
		popUpMenu processRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
	
	
	
	
	
	
	
	
	
	
	

	
	
	function updateTaskDialog skipMode:false =
	(
		onUpdate = true
		
		if skipMode == false then
		(
			if cryAM._.cbLoad.selectedIndex > 0 then
			(
				taskUIMode = #import
				cryAM._.radLoadMode.Checked = true
			)
		)
		
		case taskUIMode of
		(
			#import:
			(
				taskArray = cryMaxTools.basic.misc.makeUnique importArray
				join taskArray (cryMaxTools.basic.misc.makeUnique importUserArray)
				templateArray = cryMaxTools.basic.misc.makeUnique importTemplateArray
				filepath = importSettingsPath
			)
			#export:
			(
				taskArray = cryMaxTools.basic.misc.makeUnique exportArray
				join taskArray (cryMaxTools.basic.misc.makeUnique exportUserArray)
				templateArray = cryMaxTools.basic.misc.makeUnique exportTemplateArray
				filepath = exportSettingsPath
			)
		)
		
		
		
		if taskArray.count > 0 then
		(
			local mainLastTask = 1
			case taskUIMode of
			(
				#import:
				(
					if cryAM._.cbLoad.selectedIndex > 0 then
						mainLastTask = cryAM._.cbLoad.selectedIndex
				)
				
				#export:
				(
					if cryAM._.cbSave.selectedIndex > -1 then
						mainLastTask = cryAM._.cbSave.selectedIndex + 1
				)
			)
			
			lastTask = mainLastTask
		)
		
		
		if lastTask > 0 then
			if taskArray[lastTask].process.count > 0 then
				lastProcess = 1
		
		
		onUpdate = false
		
		updateTasks()
		updateProcessTemplates()
	),
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

	
	function callCryAMMenu =
	(
		RCMenu cryAMMenu
		(
			fn perforceActive = cryMaxTools.anim.vars.usePerforce == true
			
			subMenu "File"
			(
				menuItem miLoadAnim "Load"
				menuItem miSaveAnim "Save"
				separator sep1
				menuItem miLoadAnimFrom "Load From"
				menuItem miSaveAnimAs "Save As"
			)
			
			subMenu "Settings"
			(
				subMenu "Folder List"
				(
					menuItem miRefreshFolderList "Refresh"
					seperator sep2
					menuItem miShowSubFolders "Show All Sub-Folders"
				)
				
				subMenu "Animation List"
				(	
					menuItem miRefreshAnimList "Refresh"
					seperator sep11
					subMenu "Tools"
					(
						subMenu "Explore"
						(
							menuItem miExplore "Source" enabled:true
							menuItem miExploreGame "Game" enabled:true
						)
						seperator sep21
						subMenu "Perforce"
						(
							subMenu "Add / Open"
							(
								menuItem miAddOpenSource "Source" enabled:true
								menuItem miAddOpenGame "Game" enabled:true
							)
							menuItem miSync "Force Sync" enabled:true
							menuItem miRefreshPerforce "Update" enabled:true
							seperator sep31
							menuItem miUsePerforceLoad "Add / Open on Load"
							menuItem miUsePerforceSave "Add / Open on Save"
						)
						seperator sep41
						subMenu "Copy"
						(
							menuItem miCopyAnimName "Animation Name"
							seperator sep51
							menuItem miCopyLocalSourcePath "Local Source Path"
							menuItem miCopyLocalGamePath "Local Game Path"
							seperator sep61
							menuItem miCopyPerforceSourcePath "Perforce Source Path"
							menuItem miCopyPerforceGamePath "Perforce Game Path"
						)
					)
					seperator sep71
					subMenu "Options"
					(
						menuItem miOneClickLoad "One-Click Load"
						separator sep81
						menuItem miShowSubAnims "Show SubAnims"
						separator sep91
						menuItem miAnimPreview "Show Preview"
						menuItem miShowCBADetails "Show CBA Details"
						separator sep101
						menuItem miFilterFullPath "Search full Path"
					)
				)
			)
			
			subMenu "View"
			(
				menuItem miShowTaskSettings "Task Setup"
				menuItem miShowCharSetup "Character Setup"
				menuItem miShowMapper "Mapper Setup"
				menuItem miShowProjectInfo "Project Info"
				menuItem miShowLog "Log"
				
			)
			
			
			--cryAM.updateAnimListMenu cryAMMenu
			
			
			
			on cryAMMenu open do
			(
				miShowProjectInfo.checked = cryAM.showProjectInfo
				try miShowCharSetup.checked = setupRO.open catch()
				miShowLog.checked = cryAM.showLog
				miShowTaskSettings.checked = cryAM.showTaskSettings
				try miShowMapper.checked = mapNodesRO.open catch()
				miShowSubFolders.checked = cryAM.showSubFolders
				miAnimPreview.checked = cryAM.animPreview
				miOneClickLoad.checked = cryAM.oneClickLoad
				miUsePerforceLoad.checked = cryMaxTools.anim.vars.usePerforceLoad
				miUsePerforceSave.checked = cryMaxTools.anim.vars.usePerforceSave
				miFilterFullPath.checked = cryAM.filterFullPath
				miShowSubAnims.checked = cryAM.showSubAnims
				miShowCBADetails.checked = cryAM.showCBADetails
			)
			
			
			
			
			on miShowProjectInfo picked do
			(
				cryAM.showProjectInfo = not cryAM.showProjectInfo
				cryAM.updateDimensions()
				
				cryAM.saveSettings mode:#user
			)
			
			
			
			
			on miShowCharSetup picked do
			(
				try
				(
					if setupRO.open == true then
					(
						destroyDialog setupRO
					)
					else
					(
						cryAM.callSetupRO()
					)
				)
				catch
				(
					cryAM.callSetupRO()
				)
			)
			
			
			
			on miShowLog picked do
			(
				cryAM.showLog = not cryAM.showLog
				
				
				cryAM.updateDimensions()
			)
			
			
			on miShowTaskSettings picked do
			(
				
				
				cryAM.showTaskSettings = not cryAM.showTaskSettings
				
				
				cryAM.onUpdate = true
				
				
				
				local anchorSettingArray = #()
				
				for i = 3 to cryAM.controlArray.count do
				(
					anchorSettingArray[i] = cryAM.controlArray[i].anchor
					cryAM.controlArray[i].anchor = cryAM.controlArray[i].anchor.Left
				)
				
				
				local scriptAdd = 0
				if cryAM.showScript == true then
					scriptAdd = 408
				
				
				
				if cryAM.showTaskSettings == true then
					cryAM.main.width += (203 + scriptAdd)
				
				cryAM.updateDimensions onlyMinimum:true
				
				if cryAM.showTaskSettings == false then
					cryAM.main.width -= (203 + scriptAdd)
				
				
				
				for i = 3 to cryAM.controlArray.count do
					cryAM.controlArray[i].anchor = anchorSettingArray[i]
				
				
				--try animManTaskRO.updateDialog() catch()
				--try animManTaskRO.updateDimensions animManTaskRO.lvTasks catch()
				
				
				if cryAM.showTaskSettings == false then
				(
					cryAM.main.width = cryAM.resetTaskSize[1]
					cryAM.main.height = cryAM.resetTaskSize[2]
				)
				
				
				
				cryAM.onUpdate = false
				
				cryAM.updateDimensions()
				
				cryAM.saveSettings mode:#user
				
				
			)
			
			
			on miShowMapper picked do
			(
				try
				(
					if mapNodesRO.open == false then
						cryAM.callMapNodes()
					else
						destroyDialog mapNodesRO
				)
				catch
				(
					cryAM.callMapNodes()
				)
			)
			
			
			
			
			
			
			
			
			
			
			
			on miShowSubFolders picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.showSubFolders = not cryAM.showSubFolders
				cryAM.saveSettings mode:#user
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			
			on miRefreshFolderList picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.updateFolderList keepSelection:true
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			
			
			
			
			
			
			
			
			
			
			
			
			on miLoadAnim picked do
			(
				local ready = true

				if cryAM.cbLoad.selectedIndex > 0 then
					ready = (queryBox ("Start Process:\n\nLoad:   " + cryAM.cbLoad.selectedItem + " ?") title:"Animation Manager                         ")
				
				if ready == true then
					cryAM.callLoadAnim()
				
				--cryAM.updateAnimListMenu cryAMMenu
				
				--gc light:true
			)
			
			
			on miLoadAnimFrom picked do
			(
				local ready = true
				
				if cryAM.cbLoad.selectedIndex > 0 then
					ready = (queryBox ("Start Process:\n\nLoad:   " + cbLoad.selectedItem + " ?") title:"Animation Manager                         ")
				
				if ready == true then
					cryAM.callLoadAnim askLoad:true
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			
			
			
			on miSaveAnim picked do
			(
				local animsSelected = 0
				local ready = true
				
				for i = 1 to cryAM.animArray.count do
					if cryAM.animArray[i].selected == true then
						animsSelected += 1
				
				if animsSelected > 1 then
					ready = (queryBox ("Save Process:\n\nLoad:  " + cryAM.cbLoad.selectedItem + "\nSave:  " + cryAM.cbSave.selectedItem) title:"Animation Manager                         ")
					
				if ready == true then
					cryAM.callSaveAnim()
				
				--cryAM.updateAnimListMenu cryAMMenu
				
				--gc light:true
			)
			
			
			on miSaveAnimAs picked do
			(
				cryAM.callSaveAnim askSave:true
			)
			
			
			
			on miRefreshAnimList picked do
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.updateNodeList keepSelection:true
				
				try destroyDialog cryAM.UIUpdateRO catch()
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			
			
			
			
			
			
			
			
			on miExplore picked do
				cryAM.exploreAnims #source
			
			
			on miExploreGame picked do
				cryAM.exploreAnims #export
			
			
			on miAddOpenSource picked do
			(
				for i = 1 to cryAM.animArray.count do
				(
					if cryAM.animArray[i].selected == true and cryAM.animArray[i].type != "folder" and cryAM.animArray[i].type != "" then
					(
						cryMaxTools.basic.perforceMan.cryAdd cryAM.animArray[i].path
						cryMaxTools.basic.perforceMan.open cryAM.animArray[i].path
					)
				)
				
				cryAM.refreshAnimList()
			)
			
			
			
			on miAddOpenGame picked do
			(
				for i = 1 to cryAM.animArray.count do
				(
					if cryAM.animArray[i].selected == true and cryAM.animArray[i].type != "folder" and cryAM.animArray[i].type != "" then
					(
						cryMaxTools.basic.perforceMan.cryAdd (getExportPath cryAM.animArray[i].path "caf")
						cryMaxTools.basic.perforceMan.open (getExportPath cryAM.animArray[i].path "caf")
					)
				)
			)
			
			
			
			
			
			on miSync picked do
			(
				cryAM.syncAnim()
				
				--gc light:true
			)
			
			
			
			on miRefreshPerforce picked do
			(
				cryAM.refreshAnimList()
			)
			
			on miCopyAnimName picked do
				cryAM.setClipboardPath #animName
			
			
			on miCopyLocalSourcePath picked do
				cryAM.setClipboardPath #localSource
			
			
			on miCopyLocalGamePath picked do
				cryAM.setClipboardPath #localGame
			
			
			on miCopyPerforceSourcePath picked do
				cryAM.setClipboardPath #perforceSource
			
			
			on miCopyPerforceGamePath picked do
				cryAM.setClipboardPath #perforceGame
			
			
			on miOneClickLoad picked do
			(
				cryAM.oneClickLoad = not cryAM.oneClickLoad
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			
			on miUsePerforceLoad picked do
			(
				cryMaxTools.anim.vars.usePerforceLoad = not cryMaxTools.anim.vars.usePerforceLoad
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			on miUsePerforceSave picked do
			(
				cryMaxTools.anim.vars.usePerforceSave = not cryMaxTools.anim.vars.usePerforceSave
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			on miAnimPreview picked do
			(
				cryAM.animPreview = not cryAM.animPreview
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			on miFilterFullPath picked do
			(
				cryAM.filterFullPath = not cryAM.filterFullPath
				cryAM.updateNodeList keepSelection:true
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			on miShowSubAnims picked do
			(
				cryAM.callUIUpdateDialog()
				cryAM.showSubAnims = not cryAM.showSubAnims
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
			
			on miShowCBADetails picked do
			(
				cryAM.callUIUpdateDialog()
				cryAM.showCBADetails = not cryAM.showCBADetails
				cryAM.updateNodeList keepSelection:true
				
				try destroyDialog cryAM.UIUpdateRO catch()
				
				--cryAM.updateAnimListMenu cryAMMenu
			)
		)
		
		popUpMenu cryAMMenu pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	),
		
	

	
	
	
	function hitTest arg con =
	(
		if arg != undefined and con != undefined then
			return (arg.x >= 0 and arg.x < con.width and arg.y >= 0 and arg.y < con.height)
		
		return false
	),
	
	

	
	
	
	
	
	
	function updateTaskRollup con conList conBack =
	(
		onUpdate = true
		
		if conBack.height > 22 then
		(
			conBack.height = 22
			
			con.height = conBack.height - 2
		
			cryAM.setRoundRecRegion conBack 10
			cryAM.setRoundRecRegion con 10
		)
		else
			updateTaskDimensions conList forceUpdate:true
		
		onUpdate = false
	),
	
	
	
	
	
		--////////////////////////////////////////////////////////////////////////////////////////////////////////////
		--//
		--//	event handlers
		--//
		--////////////////////////////////////////////////////////////////////////////////////////////////////////////
	

	function initEventHandlersCryAM =
	(
		function event_cbCharacter_SelectedIndexChanged =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.onUpdate = true
				
				if cryAM._.cbCharacter.selectedIndex > 0 then
				(
					if cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].active == false then
					(
						if cryAM._.cbCharacter.selectedIndex < (cryAM._.cbCharacter.items.count - 1) then
							cryAM._.cbCharacter.selectedIndex += 1
						else
							if cryAM._.cbCharacter.selectedIndex != 0 then
								cryAM._.cbCharacter.selectedIndex -= 1
					)
				)
				
				cryAM.onUpdate = false
				
				if (cryAM._.cbCharacter.selectedIndex + 1) != cryAM.lastCharacter then
				(
					cryAM.callUIUpdateDialog()
					
					cryAM.updateByCharacter()
					cryAM.saveSettings mode:#user
					
					cryAM.lastCharacter = (cryAM._.cbCharacter.selectedIndex + 1)
					
					if cryAM.prevCharacter.count < 2 then
						cryAM.prevCharacter = #(cryAM.lastCharacter, cryAM.prevCharacter[1])
					else
					(
						cryAM.prevCharacter[2] = cryAM.prevCharacter[1]
						cryAM.prevCharacter[1] = cryAM.lastCharacter
					)
					
					cryAM.prevSetup = #()
				)
			)
			
			try destroyDialog cryAM.UIUpdateRO catch()
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.cbCharacter "SelectedIndexChanged" event_cbCharacter_SelectedIndexChanged
		
		
		
		
		
		
		
		
		
		
		
		function event_btnLog_MouseUp arg =
		(
			if (cryAM.hitTest arg cryAM._.btnLog) == true then
			(
			
				if arg.button == arg.button.left then
				(
					cryAM.showFullLog = not cryAM.showFullLog
					cryAM.updateLog()
				)
			
				if arg.button == arg.button.right then
				(
					RCMenu LogMenuRC
					(
						menuItem miOpenLog "Open Log" enabled:false
						separator sep1
						menuItem miClearLog "Clear Log" enabled:false
						
						on LogMenuRC open do
						(
							if doesFileExist cryAM.logFilePath then
							(
								miOpenLog.enabled = true
								miClearLog.enabled = true
							)
						)
						
						on miOpenLog picked do
						(
							if doesFileExist cryAM.logFilePath then
								shellLaunch cryAM.logFilePath ""
						)
						
						on miClearLog picked do
						(
							if queryBox "Delete the Log?" title:"Animation Manager Log" == true then
							(
								try deleteFile cryAM.logFilePath catch()
								
								cryAM._.cbLog.items.clear()
								cryAM._.cbLog.text = ""
								
								cryAM.logStr = ""
								cryAM.advancedLogStr = ""
							)
						)
					)

					popUpMenu LogMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
				)
			)
		)
		dotNet.addEventHandler cryAM._.btnLog "MouseUp" event_btnLog_MouseUp
		
		
		
		
		
		--// DEACTIVATED!
		/*
		function event_tmTimer_tick tick =
		(
			cryAM.updateToolTip()
		)
		dotNet.addEventHandler cryAM.tmTimer "Tick" event_tmTimer_tick
		*/
		
		
		
		
		function event_lvNodes_CellMouseMove arg =
		(
			if cryAM.animPreview == true then
			(
				if arg != undefined then
				(
					if arg.RowIndex != cryAM.lastHoveredNodeRow or arg.ColumnIndex != cryAM.lastHoveredNodeColumn then
					(
						cryAM.lastHoveredNodeRow = arg.RowIndex
						cryAM.lastHoveredNodeColumn = arg.ColumnIndex
						
						if cryAM.lastHoveredNodeRow >= 0 and cryAM.lastHoveredNodeColumn == 0 then
						(
							cryAM._.labPreview.visible = false
							cryAM.extractPreview animArray[cryAM.lastHoveredNodeRow + 1]
						)
						
						cryAM.toolTipMode = #nodes
						if cryAM.toolTip == undefined then
							cryAM.setToolTipTimer #start
					)
					
					if arg.ColumnIndex != 0 then
						cryAM.setToolTipTimer #stop
				)
				else
					cryAM.setToolTipTimer #stop
			)
			else
				cryAM.setToolTipTimer #stop
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.lvNodes "CellMouseMove" event_lvNodes_CellMouseMove
		
		
		
		
		
		function event_lvNodes_MouseMove arg =
		(
			cryAM.mousePos = [arg.x,arg.y]
		)
		dotNet.addEventHandler cryAM._.lvNodes "MouseMove" event_lvNodes_MouseMove
		
		
		
		
		function event_lvNodes_Enter arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.lvNodes "Enter" event_lvNodes_Enter
		
		
			
		
		function event_lvNodes_GotFocus arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.lvNodes "GotFocus" event_lvNodes_GotFocus
		
		
		
		
		function event_cbSetup_SelectedIndexChanged =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.onUpdate = true
				
				if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
				(
					if cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].active == false then
					(
						if cryAM._.cbSetup.selectedIndex + 1 < cryAM._.cbSetup.items.count then
							cryAM._.cbSetup.selectedIndex += 1
						else
							if cryAM._.cbSetup.selectedIndex != 0 then
								cryAM._.cbSetup.selectedIndex -= 1
					)
				)
				
				cryAM.onUpdate = false
					
				if cryAM._.cbSetup.selectedIndex + 1 != cryAM.lastSetup then
				(
					cryAM.callUIUpdateDialog()
					
					cryAM.updateBySetup skipNodeList:true
					cryAM.updateByLastSel()
					cryAM.updateLastSel()
					
					cryAM.saveSettings mode:#user
					
					try destroyDialog cryAM.UIUpdateRO catch()
				)
			)
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.cbSetup "SelectedIndexChanged" event_cbSetup_SelectedIndexChanged
		
		
		
		
		
		
		
		/*
		function event_btnCharacterBack_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnCharacterBack) == true then
				(
					if cryAM.prevCharacter.count > 1 then
					(
						cryAM.callUIUpdateDialog()
						
						local safeSel = cryAM._.cbCharacter.selectedIndex + 1
						
						local tempVar = cryAM.prevCharacter[1]
						cryAM.prevCharacter[1] = cryAM.prevCharacter[2]
						cryAM.prevCharacter[2] = tempVar
						
						cryAM.onUpdate = true
						cryAM._.cbCharacter.selectedIndex = cryAM.prevCharacter[1] - 1
						cryAM.onUpdate = false
						
						cryAM.updateByCharacter skipNodeList:true
						cryAM.updateLastSel()
						cryAM.saveSettings mode:#user
						
						
						cryAM.lastCharacter = cryAM.prevCharacter[1]
						
						try destroyDialog cryAM.UIUpdateRO catch()
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnCharacterBack "MouseUp" event_btnCharacterBack_MouseUp
		*/
		
		
		
		
		/*
		function event_btnSetupBack_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnSetupBack) == true then
				(
				
					if cryAM.prevSetup.count > 1 then
					(
						cryAM.callUIUpdateDialog()
						
						local safeSel = cryAM._.cbSetup.selectedIndex + 1
						
						local tempVar = cryAM.prevSetup[1]
						cryAM.prevSetup[1] = cryAM.prevSetup[2]
						cryAM.prevSetup[2] = tempVar
						
						cryAM.onUpdate = true
						cryAM._.cbSetup.selectedIndex = cryAM.prevSetup[1] + 1
						cryAM.onUpdate = false
						
						cryAM.updateBySetup skipNodeList:true
						cryAM.updateByLastSel()
						
						cryAM.saveSettings mode:#user
						
						cryAM.lastSetup = cryAM.prevSetup[1]
						
						try destroyDialog cryAM.UIUpdateRO catch()
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnSetupBack "MouseUp" event_btnSetupBack_MouseUp
		*/
		
		
		
		
		function event_cbLoad_selectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.saveSettings mode:#user
				
				if cryAM.showTaskSettings == true then
				(
					try
					(
						cryAM.onUpdate = true
						cryAM._.radLoadMode.checked = true
						cryAM.onUpdate = false
						
						cryAM.taskUIMode = #import
						
						cryAM.updateTaskDialog skipMode:true
					)
					catch()
				)
			)
		)
		dotNet.addEventHandler cryAM._.cbLoad "selectedIndexChanged" event_cbLoad_selectedIndexChanged
		
		
		
		function event_cbSave_selectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.saveSettings mode:#user
				
				if cryAM.showTaskSettings == true then
				(
					try
					(
						cryAM.onUpdate = true
						cryAM._.radSaveMode.checked = true
						cryAM.onUpdate = false
						
						cryAM.taskUIMode = #export
						
						cryAM.updateTaskDialog skipMode:true
					)
					catch()
				)
			)
		)
		dotNet.addEventHandler cryAM._.cbSave "selectedIndexChanged" event_cbSave_selectedIndexChanged
		
		
		
		
		
		
		
		
		
		function event_edProjectPath_KeyUp arg =
		(
			if arg.keyCode == arg.keyCode.Enter then
			(
				local newStr = cryAM._.edProjectPath.text
				
				if newStr != "" then
				(
					if newStr[newStr.count] != "\\" then
						newStr += "\\"
				)
				
				cryAM.callUIUpdateDialog()
				
				cryAM.updateRootPath newStr #project
				
				cryAM.saveSettings mode:#user
				
				
				cryAM.updateFolderList()
				cryAM.updateNodeList()
				cryAM.updateRootPathsUI()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.edProjectPath "KeyUp" event_edProjectPath_KeyUp
		
		
		
		
		
		function event_btnBrowseProjectPath_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnBrowseProjectPath) == true then
				(
					local newPath = getSavePath "Select root folder" initialDir:(cryAM.rootProjectPath)
					if newPath != undefined then
					(
						cryAM.callUIUpdateDialog()
						
						newPath += "\\"
						
						cryAM.updateRootPath newPath #project
						
						cryAM._.edProjectPath.text = cryAM.rootProjectPath
						
						cryAM.saveSettings mode:#user
						cryAM.updateFolderList()
						cryAM.updateRootPathsUI()
						
						try destroyDialog cryAM.UIUpdateRO catch()
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnBrowseProjectPath "MouseUp" event_btnBrowseProjectPath_MouseUp
		
		
		
		
		
		
		
		
		function event_edSourcePath_KeyUp arg =
		(
			if arg.keyCode == arg.keyCode.Enter then
			(
				local newStr = cryAM._.edSourcePath.text
				
				if newStr != "" then
				(
					if newStr[newStr.count] != "\\" then
						newStr += "\\"
				)
				
				cryAM.callUIUpdateDialog()
				
				cryAM.updateRootPath newStr #source
				
				cryAM._.edSourcePath.text = cryAM.rootSourcePath
				
				cryAM.saveSettings mode:#user
				
				
				cryAM.updateFolderList()
				cryAM.updateNodeList()
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.edSourcePath "keyUp" event_edSourcePath_KeyUp

		
		
		

		
		function event_btnBrowseSourcePath_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnBrowseSourcePath) == true then
				(
				
					local newPath = getSavePath "Select root folder" initialDir:(cryAM.rootProjectPath + cryAM.rootSourcePath)
					if newPath != undefined then
					(
						cryAM.callUIUpdateDialog()
						
						newPath += "\\"
						
						cryAM.updateRootPath newPath #source
						
						cryAM._.edSourcePath.text = cryAM.rootSourcePath
						
						cryAM.saveSettings mode:#user
						cryAM.updateFolderList()
						
						try destroyDialog cryAM.UIUpdateRO catch()
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnBrowseSourcePath "MouseUp" event_btnBrowseSourcePath_MouseUp
		
		
		
		function event_edGamePath_KeyUp arg =
		(
			if arg.keyCode == arg.keyCode.Enter then
			(
				local newStr = cryAM._.edGamePath.text
				
				if newStr != "" then
				(
					if newStr[newStr.count] != "\\" then
						newStr += "\\"
				
				)
				
				cryAM.updateRootPath newStr #game
				
				cryAM._.edGamePath.text = cryAM.rootGamePath
				
				cryAM.saveSettings mode:#user
			)
		)
		dotNet.addEventHandler cryAM._.edGamePath "KeyUp" event_edGamePath_KeyUp
		
		
		
		function event_btnBrowseGamePath_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnBrowseGamePath) == true then
				(
				
					local newPath = getSavePath "Select root folder" initialDir:cryAM.rootGamePath
					if newPath != undefined then
					(
						newPath += "\\"
						
						cryAM.updateRootPath newPath #game
						
						cryAM._.edGamePath.text = cryAM.rootGamePath
						
						cryAM.saveSettings mode:#user
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnBrowseGamePath "MouseUp" event_btnBrowseGamePath_MouseUp
		
		
		
		
		function event_cbFolderFilter_LostFocus arg =
		(
			cryAM.updateHistory cryAM._.cbFolderFilter #set skip:1
		)
		dotNet.addEventHandler cryAM._.cbFolderFilter "LostFocus" event_cbFolderFilter_LostFocus
		
		
		
		
		function event_cbFolderFilter_TextChanged arg =
		(
			if cryAM.onUpdate == false then
				cryAM.updateFolderList keepSelection:true
		)
		dotNet.addEventHandler cryAM._.cbFolderFilter "TextChanged" event_cbFolderFilter_TextChanged
		
		
			
			
		function event_lvFolders_NodeMouseClick arg =
		(
			if cryAM.onUpdate == false then
			(
				if arg.button == arg.button.left then
				(
					if arg.Node != cryAM.folderClickNode then
					(
						cryAM.callUIUpdateDialog()
						
						cryAM.folderClickNode = arg.Node
						
						cryAM.markFolder arg.Node
						
						cryAM.updateNodeList()
						
						cryAM.updateLastSel()
						cryAM.saveSettings mode:#user
						
						
						
						try destroyDialog cryAM.UIUpdateRO catch()
						
						
					)
					
					
				)
				
				if arg.button == arg.button.right then
					cryAM.callRightMouseClickFolders()
			)
		)
		dotNet.addEventHandler cryAM._.lvFolders "NodeMouseClick" event_lvFolders_NodeMouseClick
		
		
		
		
		
		function event_cbAnimFilter_KeyUp arg =
		(
			if arg.keyCode == arg.keyCode.Enter then
			(	
				cryAM.callUIUpdateDialog()
				
				local backSlashFound = findString cryAM._.cbAnimFilter.text "\\" != undefined
				local slashFound = findString cryAM._.cbAnimFilter.text "/" != undefined
				
				if slashFound == true then
				(
					local newPath = cryAM._.cbAnimFilter.text
					
						--// perforce path
					if newPath[1] == "/" and newPath[2] == "/" then
						newPath = cryMaxTools.basic.perforceMan.depotToLocal newPath
					
					
					cryAM.findEntry newPath
					
					
					local foundIndex = 0
					
					if foundIndex > 0 then
					(
						cryAM.folderClickNode = cryAM.folderArray[foundIndex].node
						
						cryAM.onUpdate = true
						cryAM._.lvFolders.selectedNode = cryAM.folderClickNode
						cryAM.onUpdate = false
						
						for i = 1 to cryAM.folderArray.count do
						(
							if cryAM.folderArray[i].node != cryAM.folderClickNode then
								cryAM.folderArray[i].node.backColor = cryAM.folderArray[i].node.backColor.white
							else
								cryAM.folderArray[i].node.backColor = cryAM.folderArray[i].node.backColor.fromArgb 230 230 255
						)
						
						
						if findString cryAM.searchOriginalFilter[cryAM.searchOriginalFilter.count] "." != undefined then
							cryAM._.cbAnimFilter.text = cryAM.searchOriginalFilter[cryAM.searchOriginalFilter.count]
						else
							cryAM._.cbAnimFilter.text = ""
						
						cryAM.updateNodeList()
					)
					else
						cryAM._.cbAnimFilter.text = ""
					
				)
				else
					cryAM.updateNodeList()
				
				
				cryAM.updateHistory cryAM._.cbAnimFilter #set skip:1
				cryAM.saveSettings mode:#user
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			--gc light:true
			
		)
		dotNet.addEventHandler cryAM._.cbAnimFilter "KeyUp" event_cbAnimFilter_KeyUp
		
		
		
		
		
		function event_cbAnimFilter_SelectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				if startUp == undefined then
					cryAM.callUIUpdateDialog()
				
				cryAM.updateNodeList()
				cryAM.saveSettings mode:#user
				
				if startUp == undefined then
					try destroyDialog cryAM.UIUpdateRO catch()
			)
		)
		dotNet.addEventHandler cryAM._.cbAnimFilter "SelectedIndexChanged" event_cbAnimFilter_SelectedIndexChanged
		
		
		
		
		function event_cbAnimRecent_SelectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.callUIUpdateDialog()
				
				local counter = 0
				for i = 1 to cryAM.recentHistoryArray.count do
				(
					if cryAM.recentHistoryArray[i].charID == (cryAM._.cbCharacter.selectedIndex + 1) and cryAM.recentHistoryArray[i].setupID == cryAM._.cbSetup.selectedIndex + 1 then
					(
						counter += 1
					
						if counter == (cryAM._.cbAnimRecent.selectedIndex + 1) then
							cryAM.findEntry cryAM.recentHistoryArray[i].path
					)
				)
				
				cryAM.updateAnimStatus()
				cryAM.updateLastSel()
				cryAM.saveSettings mode:#user
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.cbAnimRecent "SelectedIndexChanged" event_cbAnimRecent_SelectedIndexChanged
		
		
		
		
		function event_cbAnimExtension_KeyUp arg =
		(
			if arg.keyCode == arg.keyCode.Enter then
			(
				cryAM.callUIUpdateDialog()
				
				cryAM.updateHistory cryAM._.cbAnimExtension #set skip:2
				cryAM.updateNodeList()
				cryAM.saveSettings mode:#user
				
				try destroyDialog cryAM.UIUpdateRO catch()
			)
		)
		dotNet.addEventHandler cryAM._.cbAnimExtension "KeyUp" event_cbAnimExtension_KeyUp
		
		
		
		
		function event_cbAnimExtension_SelectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				if startUp == undefined then
					cryAM.callUIUpdateDialog()
				
				cryAM.updateNodeList()
				cryAM.saveSettings mode:#user
				
				if startUp == undefined then
					try destroyDialog cryAM.UIUpdateRO catch()
			)
		)
		dotNet.addEventHandler cryAM._.cbAnimExtension "SelectedIndexChanged" event_cbAnimExtension_SelectedIndexChanged
		



		
		
		function event_lvNodes_cellMouseUp arg =
		(
			if arg.button == arg.button.right then
			(
				local selFound = false
				
				for i = 0 to (cryAM._.lvNodes.selectedCells.count - 1) do
				(
					if (cryAM._.lvNodes.selectedCells.item i).rowIndex == arg.RowIndex then
						selFound = true
				)
				
				if selFound == false then
				(
					if arg.RowIndex > -1 then
					(
						local selCell = (cryAM._.lvNodes.Rows.item arg.RowIndex).cells.item 0
						
						for i = 1 to cryAM.animArray.count do
						(
							if cryAM.animArray[i].node != selCell then
								cryAM.animArray[i].node.selected = false
							else
							(
								if cryAM.animArray[i].type != "folder" then
									cryAM.animArray[i].node.selected = true
								
							)
						)
					)
				)
				cryAM.updateAnimStatus()
				
				cryAM.callRightMouseClickNodes()
			)
			else
			(
				cryAM.updateAnimStatus()
				
				if cryAM._.lvNodes.selectedCells.count > 0 then
				(
					local index = (cryAM._.lvNodes.selectedCells.item 0).rowIndex + 1
					if cryAM.oneClickLoad == true then
						if cryAM.animArray[index] != undefined then
							if cryAM.animArray[index].type != "folder" and cryAM.animArray[index].type != "scene" then
								cryAM.callLoadAnim()
				)
			)
			
			cryAM.updateLastSel()
			cryAM.saveSettings mode:#user
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.lvNodes "cellMouseUp" event_lvNodes_cellMouseUp
		
		
		
		
		
		function event_lvNodes_cellDoubleClick arg =
		(
			if cryAM._.lvNodes.selectedCells.count > 0 then
			(
				local index = (cryAM._.lvNodes.selectedCells.item 0).rowIndex + 1
				if cryAM.animArray[index] != undefined then
				(
					if cryAM.animArray[index].type == "folder" then
					(
						cryAM.animArray[index].node.selected = false
						
						for i = (index+1) to cryAM.animArray.count do
						(
							if cryAM.animArray[i].type == "folder" then
								exit
							
							cryAM.animArray[i].node.selected = true
						)
					)
					else
						cryAM.callLoadAnim()
				)
				
				
				cryAM.updateLastSel()
			)
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.lvNodes "cellDoubleClick" event_lvNodes_cellDoubleClick
		
		
		
		
		function event_btnSetupSet_MouseUp arg =
		(
			if (cryAM.hitTest arg cryAM._.btnSetupSet) == true then
			(
				if arg.button == arg.button.left then
				(
					cryAM.callLoadSetup useMerge:true usePrefer:true
				)
				
				if arg.button == arg.button.right then
				(
					cryAM.taskMode = #character
					cryAM.callSetupMenu()
				)
			)
		)
		dotNet.addEventHandler cryAM._.btnSetupSet "MouseUp" event_btnSetupSet_MouseUp
		
	
		
		
		function event_btnLoadSet_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnLoadSet) == true then
				(
				
					local animsSelected = 0
					local ready = true
					
					for i = 1 to cryAM.animArray.count do
						if cryAM.animArray[i].selected == true then
							animsSelected += 1
					
					if animsSelected > 1 then
						ready = (queryBox ("Start Process: \n\nLoad:  " + cryAM._.cbLoad.selectedItem) title:"Animation Manager                         ")
					
					if ready == true then
						cryAM.callLoadAnim()
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnLoadSet "MouseUp" event_btnLoadSet_MouseUp
		

		
		
		function event_btnSaveSet_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnSaveSet) == true then
				(
					local animsSelected = 0
					local ready = true
					
					for i = 1 to cryAM.animArray.count do
						if cryAM.animArray[i].selected == true then
							animsSelected += 1
					
					if animsSelected > 1 then
						ready = (queryBox ("Start Process:\n\nLoad:  " + cryAM._.cbLoad.selectedItem + "\nSave:  " + cryAM._.cbSave.selectedItem) title:"Animation Manager                         ")
						
					if ready == true then
						cryAM.callSaveAnim()
				
				--gc light:true
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
		)
		
		dotNet.addEventHandler cryAM._.btnSaveSet "MouseUp" event_btnSaveSet_MouseUp

		
		
		function event_labProjectInfoDLU_MouseHover arg =
		(
			cryAM.toolTipMode = #DLU
			cryAM.setToolTipTimer #start
		)
		dotNet.addEventHandler cryAM._.labProjectInfoDLU "MouseHover" event_labProjectInfoDLU_MouseHover
	

		
		
		function event_labProjectInfoDLU_MouseClick arg =
		(
			if arg.button == arg.button.right then
			(
				RCMenu DLUMenuRC
				(
					menuItem miRefresh "Refresh"
					
					on miRefresh picked do
						cryAM.updateBuildInfo()
				)
				
				--registerRightClickMenu DLUMenuRC

				popUpMenu DLUMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
			)
		)
		dotNet.addEventHandler cryAM._.labProjectInfoDLU "MouseHover" event_labProjectInfoDLU_MouseHover
		
		
		
		
		function event_labProjectInfoDLU_MouseLeave arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.labProjectInfoDLU "MouseLeave" event_labProjectInfoDLU_MouseLeave
		
		
		
		
		function event_labProjectInfoRC_MouseHover arg =
		(
			cryAM.toolTipMode = #RC
			cryAM.setToolTipTimer #start
		)
		dotNet.addEventHandler cryAM._.labProjectInfoRC "MouseHover" event_labProjectInfoRC_MouseHover
		
		
		
		
		function event_labProjectInfoRC_MouseClick arg =
		(
			if arg.button == arg.button.right then
			(
				RCMenu RCMenuRC
				(
					menuItem miExplore "Explore" enabled:false
					separator sep1
					menuItem miOpenLog "Open Log" enabled:false
					separator sep2
					menuItem miRefresh "Refresh"
					
					
					on RCMenuRC open do
					(
						if doesFileExist (cryAM.rootProjectPath + "Bin32\\RC\\RC.exe") then
							miExplore.enabled = true
						
						if doesFileExist (cryAM.rootProjectPath + "Bin32\\RC\\RC_log.log") then
							miOpenLog.enabled = true
					)
					
					
					on miExplore picked do
					(
						if doesFileExist (cryAM.rootProjectPath + "Bin32\\RC\\RC.exe") then
						(
								local paramString = "/select," + (cryAM.rootProjectPath + "Bin32\\RC\\RC.exe")
								shellLaunch "Explorer.exe" paramString
						)
					)
					
					
					on miOpenLog picked do
					(
						if doesFileExist (cryAM.rootProjectPath + "Bin32\\RC\\RC_log.log") then
							shellLaunch (cryAM.rootProjectPath + "Bin32\\RC\\RC_log.log") ""
					)
					
					
					on miRefresh picked do
						cryAM.updateBuildInfo()
				)
				
				--registerRightClickMenu RCMenuRC

				popUpMenu RCMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
			)
		)
		dotNet.addEventHandler cryAM._.labProjectInfoRC "MouseClick" event_labProjectInfoRC_MouseClick
		
		
		
		
		
		function event_labProjectInfoRC_MouseLeave arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.labProjectInfoRC "MouseLeave" event_labProjectInfoRC_MouseLeave
		
		
		
		function event_labProjectInfoCBA_MouseHover arg =
		(
			cryAM.toolTipMode = #CBA
			cryAM.setToolTipTimer #start
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCBA "MouseHover" event_labProjectInfoCBA_MouseHover
		
		
		
		function event_labProjectInfoCBA_MouseDoubleClick arg =
		(
			if doesFileExist cryAM.rootCBAPath then
				shellLaunch cryAM.rootCBAPath ""
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCBA "MouseDoubleClick" event_labProjectInfoCBA_MouseDoubleClick
		
		
		
		
		function event_labProjectInfoCBA_MouseClick arg =
		(
			if arg.button == arg.button.right then
			(
				RCMenu CBAMenuRC
				(
					subMenu "Perforce"
					(
						menuItem miAddOpen "Add / Open" enabled:false
						menuItem miSync "Force Sync"
					)
					separator sep1
					menuItem miExplore "Explore" enabled:false
					separator sep2
					menuItem miOpen "Open" enabled:false
					separator sep3
					menuItem miRefresh "Refresh"
					
					
					
					
					on CBAMenuRC open do
					(
						if doesFileExist cryAM.rootCBAPath then
						(
							miAddOpen.enabled = true
							miExplore.enabled = true
							miOpen.enabled = true
						)
					)
				
					on miAddOpen picked do
					(
						cryMaxTools.basic.perforceMan.open cryAM.rootCBAPath
						cryMaxTools.basic.perforceMan.cryAdd cryAM.rootCBAPath
					)
					
					on miSync picked do
					(
						if cryAM.rootCBAPath != "" then
							cryMaxTools.basic.perforceMan.getLatestRev cryAM.rootCBAPath
						else
							cryMaxTools.basic.perforceMan.getLatestRev (cryAM.rootProjectPath + "GameCrysis2\\Animations\\Animations.cba")
						
						cryAM.updateBuildInfo()
					)
					
					on miExplore picked do
					(
						if doesFileExist cryAM.rootCBAPath then
						(
							local paramString = "/select," + cryAM.rootCBAPath
							shellLaunch "Explorer.exe" paramString
						)
					)
					
					on miOpen picked do
					(
						if doesFileExist cryAM.rootCBAPath then
							shellLaunch cryAM.rootCBAPath ""
					)
					
					on miRefresh picked do
						cryAM.updateBuildInfo()
				)
				
				--registerRightClickMenu CBAMenuRC

				popUpMenu CBAMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
			)
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCBA "MouseClick" event_labProjectInfoCBA_MouseClick
		
		
		
		
		function event_labProjectInfoCBA_MouseLeave arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCBA "MouseLeave" event_labProjectInfoCBA_MouseLeave
		
		
		
		
		function event_labProjectInfoCHR_MouseHover arg =
		(
			cryAM.toolTipMode = #CHR
			cryAM.setToolTipTimer #start
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCHR "MouseHover" event_labProjectInfoCHR_MouseHover
		
		
		
		
		function event_labProjectInfoCHR_MouseClick arg =
		(
			if arg.button == arg.button.right then
			(
				RCMenu CHRMenuRC
				(
					subMenu "Perforce"
					(
						menuItem miSync "Force Sync"
					)
					separator sep1
					menuItem miExplore "Explore" enabled:false
					separator sep2
					menuItem miRefresh "Refresh"
					
					
					on CHRMenuRC open do
					(
						if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
							if doesFileExist (cryAM.rootProjectPath + cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].model.path) then
								miExplore.enabled = true
					)
					
					
					on miSync picked do
					(
						if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
							cryMaxTools.basic.perforceMan.getLatestRev (cryAM.rootProjectPath + cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].model.path)
						
						cryAM.updateBuildInfo()
					)
					
					on miExplore picked do
					(
						if cryAM._.cbCharacter.selectedIndex > 0 and cryAM._.cbSetup.selectedIndex >= 0 then
						(
							if doesFileExist (cryAM.rootProjectPath + cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].model.path) then
							(
								local paramString = "/select," + (cryAM.rootProjectPath + cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].setup[cryAM._.cbSetup.selectedIndex + 1].model.path)
								shellLaunch "Explorer.exe" paramString
							)
						)
					)
					
					on miRefresh picked do
						cryAM.updateBuildInfo()
				)
				
				--registerRightClickMenu CHRMenuRC

				popUpMenu CHRMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
			)
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCHR "MouseClick" event_labProjectInfoCHR_MouseClick
		
		
		
		
		function event_labProjectInfoCHR_MouseLeave arg =
		(
			cryAM.setToolTipTimer #stop
		)
		dotNet.addEventHandler cryAM._.labProjectInfoCHR "MouseLeave" event_labProjectInfoCHR_MouseLeave
		
		
		
		
		
		

		
		
		
		
		
		function event_labTaskHeader_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.labTaskHeader) == true then
					cryAM.updateTaskRollup cryAM._.pTask cryAM._.lvTasks cryAM._.pTaskBack
			)
			
			if arg.button == arg.button.right then
				cryAM.callTaskMenu()
		)
		dotNet.addEventHandler cryAM._.labTaskHeader "MouseUp" event_labTaskHeader_MouseUp
		
		
		function event_labProcessHeader_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.labProcessHeader) == true then
					cryAM.updateTaskRollup cryAM._.pProcess cryAM._.lvProcess cryAM._.pProcessBack
			)
			
			if arg.button == arg.button.right then
				cryAM.callProcessMenu()
		)
		dotNet.addEventHandler cryAM._.labProcessHeader "MouseUp" event_labProcessHeader_MouseUp
		
		
		function event_labPropertiesHeader_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.labPropertiesHeader) == true then
					cryAM.updateTaskRollup cryAM._.pProperties cryAM._.lvProperties cryAM._.pPropertiesBack
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
		)
		dotNet.addEventHandler cryAM._.labPropertiesHeader "MouseUp" event_labPropertiesHeader_MouseUp
		
		
		
		
		
		
		function event_lvTasks_SizeChanged arg =
		(
			cryAM.updateTaskDimensions cryAM._.lvTasks
			
		)
		dotNet.addEventHandler cryAM._.lvTasks "SizeChanged" event_lvTasks_SizeChanged
		
		
		
		
		function event_edScript_TextChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				if cryAM.lastTask > 0 and cryAM.lastProcess > 0 then
				(
					if cryAM.taskArray[cryAM.lastTask] != undefined then
					(
						if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
						(
							cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].script.text = cryAM._.edScript.text
						)
					)
				)
				
				cryAM.updateProperties forceUpdate:true
			)
			
		)
		dotNet.addEventHandler cryAM._.edScript "TextChanged" event_edScript_TextChanged
		
		
		
		
		
		
		--/////////////////////////////////////////////////
		--//
		--//		TASK EVENTS
		--//
		--/////////////////////////////////////////////////
		
		
		
		function event_radLoadMode_CheckedChanged arg =
		(
			if cryAM._.radLoadMode.checked == true and cryAM.onUpdate == false then
			(
				cryAM.taskUIMode = #import
				cryAM.updateTaskDialog skipMode:true
			)
		)
		dotNet.addEventHandler cryAM._.radLoadMode "CheckedChanged" event_radLoadMode_CheckedChanged
		
		
		
		
		function event_radSaveMode_CheckedChanged arg =
		(
			if cryAM._.radSaveMode.checked == true and cryAM.onUpdate == false then
			(
				cryAM.taskUIMode = #export
				cryAM.updateTaskDialog skipMode:true
			)
		)
		dotNet.addEventHandler cryAM._.radSaveMode "CheckedChanged" event_radSaveMode_CheckedChanged
		
		
		
		
		function event_lvTasks_MouseDown arg =
		(
			local tempEntry = (cryAM._.lvTasks.hitTest arg.x arg.y).item
			
			if tempEntry == undefined then
				cryAM.lastDraggedEntry = -1
			else
				cryAM.lastDraggedEntry = tempEntry
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.lvTasks "MouseDown" event_lvTasks_MouseDown
		

		
		function event_lvTasks_MouseMove arg =
		(
			if arg.button == arg.button.left and cryAM.lastDraggedEntry != -1 then
				cryAM.dragList cryAM._.lvTasks cryAM.taskArray arg
		)
		dotNet.addEventHandler cryAM._.lvTasks "MouseMove" event_lvTasks_MouseMove
		
		
		
		
		function event_lvTasks_MouseUp arg =
		(
			if (cryAM.hitTest arg cryAM._.lvTasks) == true then
			(
				if arg.button == arg.button.left then
				(
					cryAM.lastDraggedEntry = -1
					cryAM.lastHoveredEntry = -1
					
					cryAM.onUpdate = true
					cryAM.lastProcess = 1
					cryAM.updateProcess()
					
					cryAM.onUpdate = true
					
					if cryAM._.radLoadMode.checked == true then
						try cryAM._.cbLoad.selectedIndex = (cryAM._.lvTasks.SelectedItems.item 0).index + 1 catch()
					
					if cryAM._.radSaveMode.checked == true then
						try cryAM._.cbSave.selectedIndex = (cryAM._.lvTasks.SelectedItems.item 0).index catch()
					
					cryAM.onUpdate = false
				)
			
				if arg.button == arg.button.right then
					cryAM.callTaskMenu()
			)
		)
		dotNet.addEventHandler cryAM._.lvTasks "MouseUp" event_lvTasks_MouseUp
		
		
		
		
		
		
		function event_lvProcess_MouseDown arg =
		(
			local tempEntry = (cryAM._.lvProcess.hitTest arg.x arg.y).item
			
			if tempEntry == undefined then
				cryAM.lastDraggedEntry = -1
			else
				cryAM.lastDraggedEntry = tempEntry
			
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.lvProcess "MouseDown" event_lvProcess_MouseDown
		

		
		function event_lvProcess_MouseMove arg =
		(
			if arg.button == arg.button.left and cryAM.lastDraggedEntry != -1 then
				cryAM.dragList cryAM._.lvProcess cryAM.taskArray[cryAM.lastTask].process arg
		)
		dotNet.addEventHandler cryAM._.lvProcess "MouseMove" event_lvProcess_MouseMove
		
			
		
		
		function event_lvProcess_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.lvProcess) == true then
				(
					cryAM.lastDraggedEntry = -1
					cryAM.lastHoveredEntry = -1
					
					cryAM.onUpdate = true
					cryAM.updateScript()
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callProcessMenu()
		)
		dotNet.addEventHandler cryAM._.lvProcess "MouseUp" event_lvProcess_MouseUp
		
		
		
		function event_lvTasks_GotFocus arg =
		(
			if cryAM._.lvTasks.selectedItems.count > 0 then
				cryAM.taskArray[(cryAM._.lvTasks.selectedItems.item 0).index + 1].name = (cryAM._.lvTasks.selectedItems.item 0).text
		)
		dotNet.addEventHandler cryAM._.lvTasks "GotFocus" event_lvTasks_GotFocus
		
		
		function event_lvProcess_GotFocus arg =
		(
			if cryAM._.lvProcess.selectedItems.count > 0 then
				if cryAM.taskArray[cryAM.lastTask] != undefined then
					cryAM.taskArray[cryAM.lastTask].process[(cryAM._.lvProcess.selectedItems.item 0).index + 1].name = (cryAM._.lvProcess.selectedItems.item 0).text
		)
		dotNet.addEventHandler cryAM._.lvProcess "GotFocus" event_lvProcess_GotFocus
		
		
		
		
		
		
		function event_lvTasks_SelectedIndexChanged arg =
		(
			if cryAM.onUpdate == false then
			(
				if cryAM._.lvTasks.selectedItems.count > 0 then
					cryAM.lastTask = (cryAM._.lvTasks.selectedItems.item 0).index + 1
				
				
				for i = 0 to (cryAM._.lvTasks.items.count - 1) do
				(
					local tempNode = cryAM._.lvTasks.items.item i
					
					if i != (cryAM.lastTask - 1) then
					(
						if cryAM.taskArray[i + 1].default == true then
							tempNode.backColor = tempNode.backColor.fromARGB 240 255 240
						else
							tempNode.backColor = tempNode.backColor.fromARGB 255 240 240
					)
					else
						tempNode.backColor = tempNode.backColor.fromArgb 200 100 100
					
					
				)
			)
		)
		dotNet.addEventHandler cryAM._.lvTasks "SelectedIndexChanged" event_lvTasks_SelectedIndexChanged
		
		
		
		function event_lvProcess_SelectedIndexChanged arg =
		(
			if cryAM._.lvProcess.selectedItems.count > 0 then
				cryAM.lastProcess = (cryAM._.lvProcess.selectedItems.item 0).index + 1
			
			
			for i = 0 to (cryAM._.lvProcess.items.count - 1) do
			(
				local tempNode = cryAM._.lvProcess.items.item i
				
				if i != (cryAM.lastProcess - 1) then
					tempNode.backColor = tempNode.backColor.fromArgb 255 255 200
				else
					tempNode.backColor = tempNode.backColor.fromArgb 200 100 100
			)
		)
		dotNet.addEventHandler cryAM._.lvProcess "SelectedIndexChanged" event_lvProcess_SelectedIndexChanged
		
		
		
		
		
		function event_lvTasks_ItemChecked arg =
		(
			if cryAM.onUpdate == false then
				cryAM.taskArray[arg.item.index + 1].active = arg.item.checked
		)
		dotNet.addEventHandler cryAM._.lvTasks "ItemChecked" event_lvTasks_ItemChecked
		
		
		function event_lvProcess_ItemChecked arg =
		(
			if cryAM.onUpdate == false then
				if cryAM.taskArray[cryAM.lastTask] != undefined then
					cryAM.taskArray[cryAM.lastTask].process[arg.item.index + 1].active = arg.item.checked
		)
		dotNet.addEventHandler cryAM._.lvProcess "ItemChecked" event_lvProcess_ItemChecked
		
		
		
		
		
		
		function event_lvProperties_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.lvProperties) == true then
					cryAM.updatePropertyValue()
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
		)
		dotNet.addEventHandler cryAM._.lvProperties "MouseUp" event_lvProperties_MouseUp
		
		
		
		
		
		
		
		
		
		
		
		
		
		function event_edPropertyValue_KeyUp arg =
		(
			if cryAM.lastTask > 0 and cryAM.lastProcess > 0 and cryAM.onUpdate == false then
			(
				if cryAM.taskArray[cryAM.lastTask] != undefined then
				(
					if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
					(
						if cryAM._.lvProperties.selectedItems.count > 0 then
						(
							if (cryAM._.lvProperties.selectedItems.item 0).index > 0 then
							(
								for i = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData.count do
									for d = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties.count do
										if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].index == ((cryAM._.lvProperties.selectedItems.item 0).index) then
											cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].value = "\"" + cryAM._.edPropertyValue.text + "\""
								
								cryAM.updateProperties keepSelection:true
							)
						)
					)
				)
			)
		)
		dotNet.addEventHandler cryAM._.edPropertyValue "KeyUp" event_edPropertyValue_KeyUp
		
		
		function event_cbPropertyValue_SelectedIndexChanged arg =
		(
			if cryAM.lastTask > 0 and cryAM.lastProcess > 0 and cryAM.onUpdate == false then
			(
				if cryAM.taskArray[cryAM.lastTask] != undefined then
				(
					if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
					(
						if cryAM._.lvProperties.selectedItems.count > 0 then
						(
							if (cryAM._.lvProperties.selectedItems.item 0).index > 0 then
							(
								for i = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData.count do
									for d = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties.count do
										if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].index == ((cryAM._.lvProperties.selectedItems.item 0).index) then
											cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].value = (cryAM._.cbPropertyValue.selectedIndex == 0) as String
										
								cryAM.updateProperties keepSelection:true
							)
						)
					)
				)
			)
		)
		dotNet.addEventHandler cryAM._.cbPropertyValue "SelectedIndexChanged" event_cbPropertyValue_SelectedIndexChanged
		
		
		
		
		function event_spnPropertyValueInteger_ValueChanged arg =
		(
			if cryAM.lastTask > 0 and cryAM.lastProcess > 0 and cryAM.onUpdate == false then
			(
				if cryAM.taskArray[cryAM.lastTask] != undefined then
				(
					if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
					(
						if cryAM._.lvProperties.selectedItems.count > 0 then
						(
							if (cryAM._.lvProperties.selectedItems.item 0).index > 0 then
							(
								for i = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData.count do
									for d = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties.count do
										if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].index == ((cryAM._.lvProperties.selectedItems.item 0).index) then
											cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].value = arg as String
										
								updateProperties keepSelection:true
							)
						)
					)
				)
			)
		)
		dotNet.addEventHandler cryAM._.spnPropertyValueInteger "ValueChanged" event_spnPropertyValueInteger_ValueChanged
		
		
		
		function event_spnPropertyValueFloat_ValueChanged arg =
		(
			if cryAM.lastTask > 0 and cryAM.lastProcess > 0 and cryAM.onUpdate == false then
			(
				if cryAM.taskArray[cryAM.lastTask] != undefined then
				(
					if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess] != undefined then
					(
						if cryAM._.lvProperties.selectedItems.count > 0 then
						(
							if (cryAM.lvProperties.selectedItems.item 0).index > 0 then
							(
								for i = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData.count do
									for d = 1 to cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties.count do
										if cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].index == ((cryAM._.lvProperties.selectedItems.item 0).index) then
											cryAM.taskArray[cryAM.lastTask].process[cryAM.lastProcess].fnData[i].properties[d].value = arg as String
										
								cryAM.updateProperties keepSelection:true
							)
						)
					)
				)
			)
		)
		dotNet.addEventHandler cryAM._.spnPropertyValueFloat "ValueChanged" event_spnPropertyValueFloat_ValueChanged
		
		
		
		
		
		
		
		
		function event_btnShowScript_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnShowScript) == true then
				(
					cryAM.onUpdate = true
					
					cryAM.showScript = not cryAM.showScript
					
					local anchorArray = #()
					
					for i = 3 to cryAM.controlArray.count do
					(
						anchorArray[i] = cryAM.controlArray[i].anchor
						cryAM.controlArray[i].anchor = cryAM.controlArray[i].anchor.left
					)
					
					if cryAM.showScript == true then
					(
						cryAM._.btnShowScript.text = "<<    S C R I P T"
						cryAM.main.width += 408
						cryAM.updateDimensions onlyMinimum:true
					)
					else
					(
						cryAM.updateDimensions onlyMinimum:true
						cryAM._.btnShowScript.text = "S C R I P T    >>"
						cryAM.main.width -= 408
					)
					
					for i = 3 to cryAM.controlArray.count do
						cryAM.controlArray[i].anchor = anchorArray[i]
					
					cryAM.saveSettings mode:#user
					
					cryAM.onUpdate = false
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
		)
		dotNet.addEventHandler cryAM._.btnShowScript "MouseUp" event_btnShowScript_MouseUp
		
		
		
		
		
		
		
		
		function event_btnSaveTasks_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnSaveTasks) == true then
				(
					case cryAM.taskUIMode of
					(
						#import:
						(
							cryAM.importArray = #()
							cryAM.importUserArray = #()
							
							for i = 1 to cryAM.taskArray.count do
							(
								if cryAM.taskArray[i].default == true then
									append cryAM.importArray cryAM.taskArray[i]
								else
									append cryAM.importUserArray cryAM.taskArray[i]
							)
							
							cryAM.importTemplateArray = cryAM.templateArray
							--cryAM.importSettingsPath = cryAM.taskFilePath
							cryAM.updateImportList()
							
							cryAM.onUpdate = true
							cryAM._.cbLoad.selectedIndex = cryAM.lastTask
							cryAM.onUpdate = false
						)
						#export:
						(
							cryAM.exportArray = #()
							cryAM.exportUserArray = #()
							
							for i = 1 to cryAM.taskArray.count do
							(
								if cryAM.taskArray[i].default == true then
									append cryAM.exportArray cryAM.taskArray[i]
								else
									append cryAM.exportUserArray cryAM.taskArray[i]
							)
							
							cryAM.exportTemplateArray = cryAM.templateArray
							--cryAM.exportSettingsPath = cryAM.taskFilePath
							cryAM.updateExportList()
							
							cryAM.onUpdate = true
							cryAM._.cbSave.selectedIndex = cryAM.lastTask - 1
							cryAM.onUpdate = false
						)
					)
					
					
					--try destroyDialog taskRO catch()
					
					if (queryBox "Save Settings to File?" title:("Save " + cryAM.taskUIMode as String + " Settings")) == true then
					(
						cryAM.saveSettings mode:cryAM.taskUIMode
						
						local foundUser = false
						
						for i = 1 to cryAM.taskArray.count do
							if cryAM.taskArray[i].default == false then
								foundUser = true
						
						if foundUser == true then
						(
							if cryAM.taskUIMode == #import then
								cryAM.saveSettings mode:#importUser noCheckout:true
							
							if cryAM.taskUIMode == #export then
								cryAM.saveSettings mode:#exportUser noCheckout:true
						)
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
				
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnSaveTasks "MouseUp" event_btnSaveTasks_MouseUp
		
		
		
		function event_btnOpenTasks_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnOpenTasks) == true then
				(
					local secTaskMode = "import"
					if cryAM.taskUIMode == #export then
						secTaskMode = "export"
					
					local loadUser = false
					
					if queryBox ("Load User " + secTaskMode + " Settings?") == true then
					(
						local newSettings = cryAM.loadSettings mode:((secTaskMode + "User") as Name) writeTo:true
						loadUser = true
					)
					
					local newSettings = cryAM.loadSettings mode:taskUIMode writeTo:true
					
					if newSettings != undefined then
					(
						if loadUser == false then
						(
							cryAM.templateArray = newSettings[1]
							cryAM.taskArray = newSettings[2]
							cryAM.taskFilePath = newSettings[3]
						)
						else
						(
							join cryAM.templateArray newSettings[1]
							join cryAM.taskArray newSettings[2]
						)
						
						
						if cryAM.taskArray.count > 0 then
							cryAM.lastTask = 1
						
						if cryAM.lastTask > 0 then
							if cryAM.taskArray[cryAM.lastTask].process.count > 0 then
								cryAM.lastProcess = 1
						
						cryAM.onUpdate = true
						
						
						cryAM.updateProcessTemplates()
						cryAM.updateTasks()
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
				
			--gc light:true
		)
		dotNet.addEventHandler cryAM._.btnOpenTasks "MouseUp" event_btnOpenTasks_MouseUp
		
		
		
		function event_btnSaveAsTasks_MouseUp arg =
		(
			if arg.button == arg.button.left then
			(
				if (cryAM.hitTest arg cryAM._.btnSaveAsTasks) == true then
				(
					local secTaskMode = "import"
					if cryAM.taskUIMode == #export then
						secTaskMode = "export"
					
					local foundUser = false
					local foundDefault = false
					
					for i = 1 to cryAM.taskArray.count do
					(
						if cryAM.taskArray[i].default == false then
							foundUser = true
						else
							foundDefault = true
					)
					
					if foundDefault == true then
					(
						if queryBox ("Save Default " + secTaskMode + " Settings?") == true then
						(
							local defaultArray = #()
							
							for i = 1 to cryAM.taskArray.count do
								if cryAM.taskArray[i].default == true then
									append cryAM.defaultArray cryAM.taskArray[i]
								
							cryAM.saveSettings mode:taskUIMode saveAs:true custom:#(cryAM.templateArray,cryAM.defaultArray)
						)
					)
						
					
					
					
					if foundUser == true then
					(
						if queryBox ("Save User " + secTaskMode + " Settings?") == true then
						(
							local userArray = #()
							
							for i = 1 to cryAM.taskArray.count do
								if cryAM.taskArray[i].default == false then
									append userArray cryAM.taskArray[i]
							
							cryAM.saveSettings mode:((secTaskMode + "User") as Name) saveAs:true custom:#(#(),userArray)
						)
					)
				)
			)
			
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
			
		)
		dotNet.addEventHandler cryAM._.btnSaveAsTasks "MouseUp" event_btnSaveAsTasks_MouseUp
		
		
		
		
		
		
		
		
		
		
		
		
		
		function event_main_MouseClick arg =
		(
			if arg.button == arg.button.right then
				cryAM.callCryAMMenu()
		)
		
		
		dotNet.addEventHandler cryAM._.pMain "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.pCharacter "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.pFolders "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.gbProject "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbCharacter "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbSetup "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbLoad "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbSave "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labAmountSel "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labAmountMax "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labProjectInfoBack "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labProjectInfoBuildStatic "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labProjectInfoBuild "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labProjectInfoDLU "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labProjectInfoBuildDate "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labFolderFilter "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labRecent "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labFilter "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labExtension "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbAnimRecent "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.radLoadMode "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.radSaveMode "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.fpTaskBack "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.pTask "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.pProcess "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.pProperties "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labPropertyType "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.labPropertyValue "MouseUp" event_main_MouseClick
		dotNet.addEventHandler cryAM._.cbPropertyValue "MouseUp" event_main_MouseClick
		
		
		
		
		
		
		
		
		
		function event_main_Resize arg =
		(
			if cryAM.onUpdate == false then
			(
				cryAM.updateDimensions()
			)
		)
		dotNet.addEventHandler cryAM.main "Resize" event_main_Resize
		
		
		
		
		
		function event_main_ResizeEnd arg =
		(
			if startUp == undefined and cryAM.onUpdate == false then
			(
				cryAM.saveSettings mode:#user updateData:#dialogPos
			)
		)
		dotNet.addEventHandler cryAM.main "ResizeEnd" event_main_ResizeEnd
		
		
		
		
		/*
		function event_main_Move arg =
		(
			if startUp == undefined and cryAM.onUpdate == false then
			(
				cryAM.saveSettings mode:#user updateData:#dialogPos
			)
		)
		dotNet.addEventHandler cryAM.main "Move" event_main_Move
		*/
		
		
		
		function event_main_LostFocus =
		(
			local useOpacity = true
			
			if cryAM.UIUpdateRO != undefined and cryAM.UIUpdateRO.open == true then
				useOpacity = false
			
			if useOpacity == true then
				cryAM.main.opacity = cryAM.mainOpacity
		)
		
		function event_main_GotFocus =
			cryAM.main.opacity = 1.0
		
		
		
		if cryAM.mainOpacity < 1.0 then
		(
			for i = 1 to cryAM.controlArray.count do
			(
				dotNet.addEventHandler cryAM.controlArray[i] "GotFocus" event_main_GotFocus
				dotNet.addEventHandler cryAM.controlArray[i] "LostFocus" event_main_LostFocus
			)
		)
		
		
		function event_main_Closing arg =
		(
			if startUp == undefined and cryAM.onUpdate == false then
			(
				cryAM.saveSettings mode:#user updateData:#dialogPos
			)
			
			/*
			arg.cancel = true
			cryAM.main.visible = false
			*/
			gc light:true
		)
		dotNet.addEventHandler cryAM.main "Closing" event_main_Closing
		
	),


	
	
	
	
	
	function setRoundRecRegion con border =
	(
		local gp = dotNetObject "System.Drawing.Drawing2D.GraphicsPath"
		local rec = dotNetClass "System.Drawing.Rectangle"
		local cRec = [con.width, con.height]
		
		try if con.BorderStyle == con.BorderStyle.FixedSingle then border += 1 catch()

		if border > 0 then
		(
			gp.StartFigure()
				--// topLeft \\--
			gp.AddArc (dotNetObject rec 0 0 border border) 180 90
				--// top \\
			gp.AddLine border 0 (cRec[1] - border - 1) 0
				--// topRight \\--
			gp.AddArc (dotNetObject rec (cRec[1] - border - 1) 0 border border) -90 90
				--// right \\--
			gp.AddLine cRec[1] border cRec[1] (cRec[2] - border - 1)
				--// bottomRight \\--
			gp.AddArc (dotNetObject rec (cRec[1] - border - 1) (cRec[2] - border - 1) border border) 0 90
				--// bottom \\--
			gp.AddLine (cRec[1] - border - 1) cRec[2] border cRec[2]
				--// bottomLeft \\--
			gp.AddArc (dotNetObject rec 0 (cRec[2] - border - 1) border border) 90 90
			gp.CloseFigure()
		)
		else
			gp.AddRectangle (dotNetObject rec 0 0 cRec[1] cRec[2])
		

		con.Region = dotNetObject "System.Drawing.Region" gp
		
		return true
	),
	
	
	
	
	
	
	
	function initCryAMControls =
	(
		controlArray = #()
		
			--//////////////////////////////////////////////////////////////////////////////
			--//		FORM DEFINITIONS
			--//////////////////////////////////////////////////////////////////////////////
		
		cryAM.main = cryNET.createCon "maxForm" customToken:"maxCustomControls" cName:"cryAnimMan" cText:"Animation Manager" cSize:[678,415]
		(
			cryAM.main.width += cryAM.main.width - cryAM.main.clientRectangle.width
			cryAM.main.height += cryAM.main.height - cryAM.main.clientRectangle.height
		)	
		
		local pMain = cryNET.createCon "Panel" cName:"pMain" cParent:cryAM.main cAnchor:cryNET.anchorTopBottomLeftRight cPos:[0,0] cSize:[678,415] cBackColor:[87,87,87]
		
		--local pMain = cryNET.createCon "Panel" cName:"pMain" cParent:cryAM.main cAnchor:cryNET.anchorTopBottomLeftRight cLoc:[0,0] cSize:[686,415] 
		local pCharacter = cryNET.createCon "Panel" cName:"pCharacter" cParent:pMain cAnchor:cryNET.anchorLeftTopRight cPos:[0,57] cSize:[686,1] borderFixed:true
		--dotNet.setLifeTimeControl pCharacter #dotNet
		local pFolders = cryNET.createCon "Panel" cName:"pFolders" cParent:pMain cAnchor:cryNET.anchorTopLeftBottom cPos:[182,0] cSize:[1,389] borderFixed:true
		--dotNet.setLifeTimeControl pFolders #dotNet
		--local btnCharacterBack = cryNET.createCon "Button" cName:"btnCharacterBack" cParent:cryAM.main cText:"<" cPos:[3,8] cSize:[13,18] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnCharacterBack #dotNet
		local cbCharacter = cryNET.createCon "ComboBox" cName:"cbCharacter" cParent:pMain cPos:[5,6] cSize:[110,20] borderFlat:false
		(
			cbCharacter.MaxDropDownItems = 10
			cbCharacter.DropDownStyle = cbCharacter.DropDownStyle.DropDownList
			cbCharacter.backColor = cbCharacter.backColor.fromARGB 255 130 36
			cbCharacter.items.Add " Humans"
		)
		--dotNet.setLifeTimeControl cbCharacter #dotNet
		
		--local btnSetupBack = cryNET.createCon "Button" cName:"btnSetupBack" cParent:cryAM.main cText:"<" cPos:[3,33] cSize:[13,18] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnSetupBack #dotNet
		local cbSetup = cryNET.createCon "ComboBox" cName:"cbSetup" cParent:pMain cPos:[5,31] cSize:[110,20] borderFlat:false
		(
			cbSetup.MaxDropDownItems = 10
			cbSetup.DropDownStyle = cbSetup.DropDownStyle.DropDownList
			cbSetup.backColor = cbSetup.backColor.fromARGB 255 130 36
		)
		--dotNet.setLifeTimeControl cbSetup #dotNet
		
		local btnSetupSet = cryNET.createCon "Button" cName:"btnSetupSet" cParent:pMain cText:"S e t u p" cPos:[120,6] cSize:[57,46] cToolTip:"Left Click:    Load / Merge the Setup\nRight Click:  Open Settings Menu" borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnSetupSet #dotNet

		
		local btnBrowseProjectPath = cryNET.createCon "Button" cName:"btnBrowseProjectPath" cParent:pMain cText:"Project >" cPos:[188,6] cSize:[55,19] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnBrowseProjectPath #dotNet
		local edProjectPath = cryNET.createCon "TextBox" cName:"edProjectPath" cParent:pMain cPos:[248,6] cSize:[165,18] cBackColor:[255,152,37] borderFixed:true
		--dotNet.setLifeTimeControl edProjectPath #dotNet
		
		local gbProject = cryNET.createCon "Panel" cName:"gbProject" cParent:pMain cPos:[182,29] cSize:[200,29] borderFixed:true
		--dotNet.setLifeTimeControl gbProject #dotNet
		
		local labProjectInfoBack = cryNET.createCon "Label" cName:"labProjectInfoBack" cParent:gbProject cPos:[184,31] cSize:[196,25] cBackColor:[50,50,50]
		--dotNet.setLifeTimeControl labProjectInfoBack #dotNet
		local labProjectInfoBuildStatic = cryNET.createCon "Label" cName:"labProjectInfoBuildStatic" cParent:labProjectInfoBack cText:"Build-Nr." cPos:[184,31] cSize:[47,12] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl labProjectInfoBuildStatic #dotNet
		local labProjectInfoBuild = cryNET.createCon "Label" cName:"labProjectInfoBuild" cParent:labProjectInfoBack cText:"1689" cPos:[230,31] cSize:[30,12] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl labProjectInfoBuild #dotNet

		local labProjectInfoDLU = cryNET.createCon "Label" cName:"labProjectInfoDLU" cParent:labProjectInfoBack cText:"DLU" cPos:[265,31] cSize:[25,12] cForeColor:[0,150,0]
		--dotNet.setLifeTimeControl labProjectInfoDLU #dotNet
		local labProjectInfoRC = cryNET.createCon "Label" cName:"labProjectInfoRC" cParent:labProjectInfoBack cText:"RC" cPos:[290,31] cSize:[22,12] cForeColor:[0,150,0]
		--dotNet.setLifeTimeControl labProjectInfoRC #dotNet
		local labProjectInfoCBA = cryNET.createCon "Label" cName:"labProjectInfoCBA" cParent:labProjectInfoBack cText:"CBA" cPos:[310,31] cSize:[25,12] cForeColor:[0,150,0]
		--dotNet.setLifeTimeControl labProjectInfoCBA #dotNet
		local labProjectInfoCHR = cryNET.createCon "Label" cName:"labProjectInfoCHR" cParent:labProjectInfoBack cText:"CHR" cPos:[335,31] cSize:[26,12] cForeColor:[0,150,0]
		--dotNet.setLifeTimeControl labProjectInfoCHR #dotNet
		local labProjectInfoBuildDate = cryNET.createCon "Label" cName:"labProjectInfoBuildDate" cParent:labProjectInfoBack cText:"Created       26.11.2009    12:00:48" cPos:[184,42] cSize:[183,12] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl labProjectInfoBuildDate #dotNet
		
		
		
		local edSourcePath = cryNET.createCon "TextBox" cName:"edSourcePath" cParent:pMain cAnchor:cryNET.anchorTopLeftRight cPos:[408,6] cSize:[201,18] cBackColor:[255,152,37] borderFixed:true
		--dotNet.setLifeTimeControl edSourcePath #dotNet
		local btnBrowseSourcePath = cryNET.createCon "Button" cName:"btnBrowseSourcePath" cParent:pMain cText:"< Source" cAnchor:cryNET.anchorTopRight cPos:[612,6] cSize:[61,19] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnBrowseSourcePath #dotNet
		
		local edGamePath = cryNET.createCon "TextBox" cName:"edGamePath" cParent:pMain cAnchor:cryNET.anchorTopLeftRight cPos:[408,29] cSize:[201,18] cBackColor:[255,152,37] borderFixed:true
		--dotNet.setLifeTimeControl edGamepath #dotNet
		local btnBrowseGamePath = cryNET.createCon "Button" cName:"btnBrowseGamePath" cParent:pMain cText:"< Game" cAnchor:cryNET.anchorTopRight cPos:[612,29] cSize:[61,19] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnBrowseGamePath #dotNet
		
		
		local labRecent = cryNET.createCon "Label" cName:"labRecent" cParent:pMain cText:"Recent" cAnchor:cryNET.anchorTop cPos:[450,67] cSize:[45,18] cForeColor:[177,177,177]
		--dotNet.setLifeTimeControl labRecent #dotNet
		local cbAnimRecent = cryNET.createCon "ComboBox" cName:"cbAnimRecent" cParent:pMain cAnchor:cryNET.anchorTop cPos:[495,63] cSize:[174,20] cBackColor:[255,152,37] borderFlat:false
		(
			cbAnimRecent.MaxDropDownItems = 10
			cbAnimRecent.DropDownStyle = cbAnimRecent.DropDownStyle.DropDownList
		)
		--dotNet.setLifeTimeControl cbAnimRecent #dotNet
		
		local labFilter = cryNET.createCon "Label" cName:"labFilter" cParent:pMain cText:"Search" cPos:[450,92] cSize:[45,18] cAnchor:cryNET.anchorTop cForeColor:[177,177,177]
		--dotNet.setLifeTimeControl labFilter #dotNet
		local cbAnimFilter = cryNET.createCon "ComboBox" cName:"cbAnimFilter" cParent:pMain cAnchor:cryNET.anchorTop cPos:[495,88] cSize:[94,20] cBackColor:[255,152,37] borderFlat:false
		(
			cbAnimFilter.MaxDropDownItems = 10
			cbAnimFilter.items.Add ""
		)
		--dotNet.setLifeTimeControl cbAnimFilter #dotNet
		
		local labExtension = cryNET.createCon "Label" cName:"labExtension" cParent:pMain cText:"." cPos:[591,94] cSize:[5,18] cAnchor:cryNET.anchorTopRight cForeColor:[177,177,177]
		--dotNet.setLifeTimeControl labExtension #dotNet
		local cbAnimExtension = cryNET.createCon "ComboBox" cName:"cbAnimExtension" cParent:pMain cAnchor:cryNET.anchorTopRight cPos:[596,88] cSize:[77,20] cBackColor:[255,152,37] borderFlat:false
		(
			cbAnimExtension.MaxDropDownItems = 10
			cbAnimExtension.items.Add ""
			cbAnimExtension.items.Add "bip, max, xaf, fbx, htr"
		)
		--dotNet.setLifeTimeControl cbAnimExtension #dotNet
		
		
		
		
		
		local labFolderFilter = cryNET.createCon "Label" cName:"labFolderFilter" cParent:pMain cText:"Search" cPos:[8,66] cSize:[45,18] cForeColor:[177,177,177]
		--dotNet.setLifeTimeControl labFolderFilter #dotNet
		local cbFolderFilter = cryNET.createCon "ComboBox" cName:"cbFolderFilter" cParent:pMain cPos:[55,64] cSize:[122,20] cForeColor:[0,0,0] cBackColor:[255,152,37]
		(
			cbFolderFilter.MaxDropDownItems = 10
			cbFolderFilter.items.add ""
		)
		--dotNet.setLifeTimeControl cbFolderFilter #dotNet
		local lvFolders = cryNET.createCon "TreeView" cName:"lvFolders" cParent:pMain cAnchor:cryNET.anchorTopLeftBottom cPos:[5,88] cSize:[172,268] borderFixed:true cForeColor:[0,0,0] cBackColor:[225,224,210]
		(
			lvFolders.LineColor = lvFolders.LineColor.fromARGB 100 100 100
			lvFolders.ShowRootLines = false
			lvFolders.ShowLines = false
			lvFolders.FullRowSelect = true
			lvFolders.BorderStyle = lvFolders.BorderStyle.FixedSingle
			
			local folderImageList = dotNetObject "System.Windows.Forms.ImageList"
			local openImage = (dotNetClass "System.Drawing.Image").FromFile (cryMaxTools.basic.vars.toolsPath + "Icons\\animMan_folder_open.png")
			local closeImage = (dotNetClass "System.Drawing.Image").FromFile (cryMaxTools.basic.vars.toolsPath + "Icons\\animMan_folder_close.png")
			folderImageList.images.Add closeImage
			folderImageList.images.Add openImage
			lvFolders.ImageList = folderImageList
			lvFolders.selectedImageIndex = 1
		)
		--dotNet.setLifeTimeControl lvFolders #dotNet

		

		local lvNodes = cryNET.createCon "DataGridView" cName:"lvNodes" cParent:pMain cAnchor:cryNET.anchorTopBottomLeftRight cPos:[188,114] cSize:[485,242] cForeColor:[0,0,0] cBackColor:[225,224,210]
		(	
			lvNodes.RowHeadersVisible = false
			lvNodes.ReadOnly = true
			lvNodes.AllowUserToResizeColumns = false
			lvNodes.AllowUserToOrderColumns = false
			lvNodes.ShowCellToolTips = false
			
			
			lvNodes.ColumnHeadersDefaultCellStyle.alignment = lvNodes.ColumnHeadersDefaultCellStyle.alignment.MiddleCenter
			
			
			local RowTemplate = lvNodes.RowTemplate
			
			RowTemplate.resizable = RowTemplate.resizable.false
			RowTemplate.height = 19
			lvNodes.RowTemplate = RowTemplate
			
			lvNodes.ColumnHeadersHeightSizeMode = lvNodes.ColumnHeadersHeightSizeMode.DisableResizing
			lvNodes.ColumnHeadersHeight = 20
			
			
			
			lvNodes.ColumnCount = 3
			(lvNodes.Columns.item 0).name = "Animation"
			(lvNodes.Columns.item 1).name = "Type"
			(lvNodes.Columns.item 2).name = "Status"
			
			
			local typeStyle = (lvNodes.Columns.item 1).DefaultCellStyle
			typeStyle.Alignment = typeStyle.Alignment.MiddleCenter
			(lvNodes.Columns.item 1).DefaultCellStyle = typeStyle
			(lvNodes.Columns.item 2).DefaultCellStyle = typeStyle
			
			(lvNodes.Columns.item 0).width = (405+50)--350
			(lvNodes.Columns.item 1).width = 50
			(lvNodes.Columns.item 2).width = 70
			
			local sortMode = (lvNodes.Columns.item 0).SortMode.NotSortable
			(lvNodes.Columns.item 0).SortMode = sortMode
			(lvNodes.Columns.item 1).SortMode = sortMode
			(lvNodes.Columns.item 2).SortMode = sortMode
		)
		--dotNet.setLifeTimeControl lvNodes #dotNet
		
		local pAmount = cryNET.createCon "Panel" cName:"pAmount" cParent:pMain cAnchor:cryNET.anchorTop cPos:[375,57] cSize:[67,59] cBackColor:[50,50,50] cForeColor:[0,255,0] borderFixed:true
		--dotNet.setLifeTimeControl pAmount #dotNet
		
		local labAmountSel = cryNET.createCon "Label" cName:"labAmountSel" cParent:pAmount cText:"0" cPos:[375,57] cSize:[67,34] cForeColor:[0,255,0]
		(
			labAmountSel.TextAlign = labAmountSel.TextAlign.MiddleCenter
		)
		--dotNet.setLifeTimeControl labAmountSel #dotNet
		
		local labAmountMax = cryNET.createCon "Label" cName:"labAmountMax" cParent:pAmount cText:"0" cPos:[375,82] cSize:[67,34] cForeColor:[255,0,0]
		(
			labAmountMax.TextAlign = labAmountMax.TextAlign.MiddleCenter
		)
		--dotNet.setLifeTimeControl labAmountMax #dotNet

		local btnLoadSet = cryNET.createCon "Button" cName:"btnLoadSet" cParent:pMain cText:"L o a d" cPos:[188,64] cSize:[55,20] cToolTip:"Left Click:    Load the Animation" borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnLoadSet #dotNet
		local cbLoad = cryNET.createCon "ComboBox" cName:"cbLoad" cParent:pMain cPos:[248,63] cSize:[117,20] cBackColor:[255,152,37] borderFlat:false
		(
			cbLoad.MaxDropDownItems = 10
			cbLoad.DropDownStyle = cbLoad.DropDownStyle.DropDownList
		)
		--dotNet.setLifeTimeControl cbLoad #dotNet

		local btnSaveSet = cryNET.createCon "Button" cName:"btnSaveSet" cParent:pMain cText:"S a v e" cPos:[188,89] cSize:[55,20] cToolTip:"Left Click:    Save the Animation" borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnSaveSet #dotNet
		local cbSave = cryNET.createCon "ComboBox" cName:"cbSave" cParent:pMain cPos:[248,88] cSize:[117,20] cBackColor:[255,152,37] borderFlat:false
		(
			cbSave.MaxDropDownItems = 10
			cbSave.DropDownStyle = cbSave.DropDownStyle.DropDownList
		)
		--dotNet.setLifeTimeControl cbSave #dotNet
		
		
		local btnLog = cryNET.createCon "Button" cName:"btnLog" cParent:pMain cAnchor:cryNET.anchorLeftBottom cText:"Log+" cPos:[5,388] cSize:[35,20] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnLog #dotNet
		local cbLog = cryNET.createCon "ComboBox" cName:"cbLog" cParent:pMain cText:" Log :  " cAnchor:cryNET.anchorLeftBottomRight cPos:[42,388] cSize:[633,18] cBackColor:[87,87,87] cForeColor:[200,200,200]
		(
			cbLog.DropDownHeight = 200
		)
		--dotNet.setLifeTimeControl cbLog #dotNet
		
		
		
		
			--// TASK STUFF
		
		local gbMode = cryNET.createCon "Panel" cName:"gbMode" cParent:pMain cAnchor:cryNET.anchorTopRight cPos:[678,5] cSize:[65,53] borderFixed:true
		--dotNet.setLifeTimeControl gbMode #dotNet
		local radLoadMode = cryNET.createCon "RadioButton" cName:"radLoadMode" cParent:gbMode cText:"Load" cLoc:[8,5] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl radLoadMode #dotNet
		local radSaveMode = cryNET.createCon "RadioButton" cName:"radSaveMode" cParent:gbMode cText:"Save" cLoc:[8,25] cForeColor:[200,200,200] cChecked:true
		--dotNet.setLifeTimeControl radSaveMode #dotNet
		local gbFile = cryNET.createCon "Panel" cName:"gbMode" cParent:pMain cAnchor:cryNET.anchorTopRight cPos:[742,5] cSize:[135,53] borderFixed:true
		--dotNet.setLifeTimeControl gbFile #dotNet
		local btnOpenTasks = cryNET.createCon "Button" cName:"btnOpenTasks" cParent:gbFile cText:"Open" cLoc:[7,6] cSize:[61,20] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnOpenTasks #dotNet
		local btnSaveAsTasks = cryNET.createCon "Button" cName:"btnSaveAsTasks" cParent:gbFile cText:"Save As" cLoc:[67,6] cSize:[61,20] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnSaveAsTasks #dotNet
		local btnSaveTasks = cryNET.createCon "Button" cName:"btnSaveTasks" cParent:gbFile cText:"Save" cLoc:[7,25] cSize:[121,20] borderFlat:true cBackColor:[87,87,87] cForeColor:[200,200,200]
		--dotNet.setLifeTimeControl btnSaveTasks #dotNet
		
		
		local fpTaskBack = cryNET.createCon "FlowLayoutPanel" cName:"fpTaskBack" cParent:pMain cAnchor:cryNET.anchorTopRightBottom cPos:[678,63] cSize:[200,352] cBackColor:[87,87,87]
		(
			fpTaskBack.flowDirection = fpTaskBack.flowDirection.TopDown
			fpTaskBack.wrapContents = false
		)
		--dotNet.setLifeTimeControl fpTaskBack #dotNet
		
		local pTaskBack = cryNET.createCon "Panel" cName:"pTaskBack" cParent:fpTaskBack cSize:[195,100] cBackColor:[0,0,0]
		(
			cryAM.setRoundRecRegion pTaskBack 10
		)
		--dotNet.setLifeTimeControl pTaskBack #dotNet
		local pTask = cryNET.createCon "Panel" cName:"pTask" cParent:pTaskBack cAnchor:cryNET.anchorTopBottomLeftRight cSize:[193,98] cBackColor:[100,100,100]
		(
			cryAM.setRoundRecRegion pTask 10
		)
		--dotNet.setLifeTimeControl pTask #dotNet
		local labTaskHeader = cryNET.createCon "Label" cName:"labTaskHeader" cParent:pTask cText:"Task" cLoc:[0,0] cSize:[193,20] cBackColor:[255,152,37] cForeColor:[0,0,0]
		(
			labTaskHeader.TextAlign = labTaskHeader.TextAlign.MiddleCenter
		)
		--dotNet.setLifeTimeControl labTaskHeader #dotNet
		local lvTasks = cryNET.createCon "ListView" cName:"lvTasks" cParent:pTask cLoc:[0,20] cSize:[193,78] cBackColor:[200,200,200]
		(
			lvTasks.AllowDrop = true
			lvTasks.GridLines = true
			lvTasks.Checkboxes = true
			lvTasks.AllowColumnReorder = false
			lvTasks.View = lvTasks.View.Details
			lvTasks.LabelEdit = true
			lvTasks.LabelWrap = true
			lvTasks.MultiSelect = true
			lvTasks.FullRowSelect = true
			lvTasks.HideSelection = true
			lvTasks.HeaderStyle = lvTasks.HeaderStyle.None
			lvTasks.borderStyle = lvTasks.borderStyle.None
			lvTasks.Scrollable = false
			
			lvTasks.Columns.Add ""
			(lvTasks.Columns.item 0).width = lvTasks.width
		)
		--dotNet.setLifeTimeControl lvTasks #dotNet
		
		
		local pProcessBack = cryNET.createCon "Panel" cName:"pProcessBack" cParent:fpTaskBack cSize:[195,100] cBackColor:[0,0,0]
		(
			cryAM.setRoundRecRegion pProcessBack 10
		)
		--dotNet.setLifeTimeControl pProcessBack #dotNet
		local pProcess = cryNET.createCon "Panel" cName:"pProcess" cParent:pProcessBack cAnchor:cryNET.anchorTopBottomLeftRight cSize:[193,98] cBackColor:[200,200,200]
		(
			cryAM.setRoundRecRegion pProcess 10
		)
		--dotNet.setLifeTimeControl pProcess #dotNet
		local labProcessHeader = cryNET.createCon "Label" cName:"labProcessHeader" cParent:pProcess cText:"Process" cLoc:[0,0] cSize:[193,20] cBackColor:[255,152,37]  cForeColor:[0,0,0]
		(
			labProcessHeader.TextAlign = labProcessHeader.TextAlign.MiddleCenter
		)
		--dotNet.setLifeTimeControl labProcessHeader #dotNet
		local lvProcess = cryNET.createCon "ListView" cName:"lvProcess" cParent:pProcess cLoc:[0,20] cSize:[193,78] cBackColor:[200,200,200]
		(
			lvProcess.GridLines = true
			lvProcess.Checkboxes = true
			lvProcess.AllowColumnReorder = false
			lvProcess.View = lvProcess.View.Details
			lvProcess.LabelEdit = true
			lvProcess.LabelWrap = true
			lvProcess.MultiSelect = true
			lvProcess.FullRowSelect = true
			lvProcess.HideSelection = true
			lvProcess.HeaderStyle = lvProcess.HeaderStyle.None
			lvProcess.BorderStyle = lvProcess.BorderStyle.None
			lvProcess.Scrollable = false
			
			lvProcess.Columns.Add ""
			(lvProcess.Columns.item 0).width = lvProcess.width
		)
		--dotNet.setLifeTimeControl lvProcess #dotNet
		
		
		local pPropertiesBack = cryNET.createCon "Panel" cName:"pPropertiesBack" cParent:fpTaskBack cSize:[195,185] cBackColor:[0,0,0]
		(
			cryAM.setRoundRecRegion pPropertiesBack 10
		)
		--dotNet.setLifeTimeControl pPropertiesBack #dotNet
		local pProperties = cryNET.createCon "Panel" cName:"pProperties" cParent:pPropertiesBack cAnchor:cryNET.anchorTopBottomLeftRight cSize:[193,183] cBackColor:[220,220,220]
		(
			cryAM.setRoundRecRegion pProperties 10
		)
		--dotNet.setLifeTimeControl pProperties #dotNet
		local labPropertiesHeader = cryNET.createCon "Label" cName:"labPropertiesHeader" cParent:pProperties cText:"Properties" cLoc:[0,0] cSize:[193,20] cBackColor:[255,152,37]  cForeColor:[0,0,0]
		(
			labPropertiesHeader.TextAlign = labPropertiesHeader.TextAlign.MiddleCenter
		)
		--dotNet.setLifeTimeControl labPropertiesHeader #dotNet
		
		local lvProperties = cryNET.createCon "ListView" cName:"lvProperties" cParent:pProperties cLoc:[0,20] cSize:[193,78] cBackColor:[200,200,200]
		(
			lvProperties.GridLines = true
			lvProperties.Checkboxes = false
			lvProperties.AllowColumnReorder = false
			lvProperties.View = lvProcess.View.Details
			lvProperties.LabelEdit = false
			lvProperties.LabelWrap = false
			lvProperties.MultiSelect = false
			lvProperties.FullRowSelect = true
			lvProperties.HideSelection = true
			lvProperties.HeaderStyle = lvProperties.HeaderStyle.Nonclickable
			lvProperties.BorderStyle = lvProperties.BorderStyle.None
			lvProperties.Scrollable = false
			
			lvProperties.Columns.Add "        Value"
			lvProperties.Columns.Add "     Property"
			
			(lvProperties.Columns.item 0).DisplayIndex = 1
			
			(lvProperties.Columns.item 0).width = (lvProperties.width + 1) / 2
			(lvProperties.Columns.item 1).width = (lvProperties.width + 1) / 2
		)
		--dotNet.setLifeTimeControl lvProperties #dotNet
		
		local labPropertyType = cryNET.createCon "Label" cName:"labPropertyType" cParent:pProperties cText:"Type :      None" cAnchor:cryNET.anchorBottom cLoc:[5,110] cSize:[180,18]
		--dotNet.setLifeTimeControl labPropertyType #dotNet
		local labPropertyValue = cryNET.createCon "Label" cName:"labPropertyValue" cParent:pProperties cText:"Value :" cAnchor:cryNET.anchorBottom cLoc:[5,131] cSize:[45,18]
		--dotNet.setLifeTimeControl labPropertyValue #dotNet
		
		local edPropertyValue = cryNET.createCon "TextBox" cName:"edPropertyValue" cParent:pProperties cAnchor:cryNET.anchorBottom cLoc:[50,128] cSize:[138,18] borderFixed:true
		--dotNet.setLifeTimeControl edPropertyValue #dotNet
		local cbPropertyValue = cryNET.createCon "ComboBox" cName:"cbPropertyValue" cParent:pProperties cAnchor:cryNET.anchorBottom cLoc:[50,128] cSize:[138,18] cVisible:false
		(
			cbPropertyValue.items.Add "true"
			cbPropertyValue.items.Add "false"
			cbPropertyValue.DropDownStyle = cbPropertyValue.DropDownStyle.DropDownList
		)
		--dotNet.setLifeTimeControl cbPropertyValue #dotNet
		local spnPropertyValueInteger = cryNET.createCon "NumericUpDown" cName:"spnPropertyValueInteger" cParent:pProperties cAnchor:cryNET.anchorBottom cLoc:[50,130] cSize:[138,18] cVisible:false
		--dotNet.setLifeTimeControl spnPropertyValueInteger #dotNet
		local spnPropertyValueFloat = cryNET.createCon "NumericUpDown" cName:"spnPropertyValueFloat" cParent:pProperties cAnchor:cryNET.anchorBottom cLoc:[50,130] cSize:[138,18] cVisible:false
		--dotNet.setLifeTimeControl spnPropertyValueFloat #dotNet
		
		local btnShowScript = cryNET.createCon "Button" cName:"btnShowScript" cParent:pProperties cText:"S C R I P T    >>" cAnchor:cryNET.anchorBottom cLoc:[22,158] cSize:[150,20] borderFlat:true
		--dotNet.setLifeTimeControl btnShowScript #dotNet
		
		
		
		local edScript = cryNET.createCon "RichTextBox" cName:"edScript" cParent:pMain cAnchor:cryNET.anchorTopRightBottom cPos:[885,5] cSize:[400,405]
		(
			edScript.Multiline = true
			edScript.ScrollBars = edScript.Scrollbars.Both
			edScript.WordWrap = false
			--edScript.AcceptsReturn = true
			edScript.AcceptsTab = true
			edScript.BorderStyle = edScript.BorderStyle.FixedSingle
		)
		--dotNet.setLifeTimeControl edScript #dotNet
		
		
		local structString = "struct cryAnimManConStruct ("
		
		for i = 2 to cryAM.controlArray.count do
			structString += cryAM.controlArray[i].name + "=cryAM.controlArray[" + i as String + "]" + (if i < cryAM.controlArray.count then ", " else ") ; cryAM._ = cryAnimManConStruct()")
		
		
		
		execute structString
	),
	
	
	
	
	function startCryAM =
	(
		try
		(
			for i = cryAM.controlArray.count to 1 by -1 do
			(
				try cryAM.controlArray[i].Dispose() catch()
				try cryAM.controlArray[i] = undefined catch()
			)
		)catch()
		
		gc light:true
		
		
		startUp = true
		
		callUIUpdateDialog()
		
		initCryAMControls()
		
		loadSettings mode:#dialogPos load:false
		loadSettings mode:#character load:true
		loadSettings mode:#import load:true
		loadSettings mode:#importUser load:false
		loadSettings mode:#export load:true
		loadSettings mode:#exportUser load:false
		
		updateCharacterList()
		updateImportList()
		updateExportList()
		
		resizeSizeArray = #()
		
		
		for i = 3 to cryAM.controlArray.count do
			if (local tempItem = getUIItemDimensions cryAM.controlArray[i]) != undefined then
				append resizeSizeArray tempItem
		
		
		
		
		local anchorSettingArray = #()
		
		for i = 3 to cryAM.controlArray.count do
		(
			anchorSettingArray[i] = cryAM.controlArray[i].anchor
			cryAM.controlArray[i].anchor = cryAM.controlArray[i].anchor.Left
		)
		
		
		loadSettings mode:#user load:false skipDimensions:true
		
		
		onUpdate = true
		
		
		local taskAdd = 0
		if cryAM.showTaskSettings == true then
		(
			taskAdd = 203
			
			if cryAM.showScript == true then
				taskAdd += 408
		)
		
		cryAM.main.width += taskAdd
		
		
		
		for i = 3 to cryAM.controlArray.count do
			cryAM.controlArray[i].anchor = anchorSettingArray[i]
		
		
		
		
		
		if cryAM._.cbSave.selectedIndex == -1 and cryAM._.cbSave.items.count > 0 then
			cryAM._.cbSave.selectedIndex = 0
		
		
		if cryAM._.cbLoad.selectedIndex == -1 and cryAM._.cbLoad.items.count > 0 then
			cryAM._.cbLoad.selectedIndex = 0
		
		
		
			--// TOOL TIP SETTINGS
		labPreview.parent = lvNodes
		labPreview.visible = false
		
		lastCharacter = cryAM._.cbCharacter.selectedIndex + 1
		lastSetup = cryAM._.cbSetup.selectedIndex + 1
		
		
		prevCharacter = #(cryAM._.cbCharacter.selectedIndex + 1)
		prevSetup = #(cryAM._.cbSetup.selectedIndex + 1)
		
		
		addLog "Started Session" addSep:true
		
		
		
		
			
		
		--cryAM.main.width = dialogSize[1]
		--cryAM.main.height = dialogSize[2]
		
		
		updateDimensions()
		updateTaskDialog()
		
		updateTaskRollup cryAM._.pTask cryAM._.lvTasks cryAM._.pTaskBack
		
		
		initEventHandlersCryAM()
		
		try destroyDialog UIUpdateRO catch()
		
		
		onUpdate = false
		global startUp = undefined
		
		cryAM.maxHwnd = DotNetObject "MaxCustomControls.Win32HandleWrapper" (DotNetObject "System.IntPtr" (Windows.GetMAXHWND()))
		
		cryAM.main.visible = false
			
		cryAM.main.Show(cryAM.maxHwnd)
			
		cryNET.setLoc cryAM.main dialogPos
			
		cryAM.main.visible = true
	),
	
	
	function initCryAM =
	(
		--callUIUpdateDialog()
		
		
		--// RUN ANIM.MS AGAIN!! :(
		if restartCryMaxTools == undefined then
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Anim.ms")
			global restartCryMaxTools = true
		)
		
		
		for i = 1 to 11 do
		(
			append animStyle (dotNetObject "System.Windows.Forms.DataGridViewCellStyle")
			animStyle[animStyle.count].backColor = animStyle[animStyle.count].backColor.white
		)
		
		for i = 1 to 3 do
		(
			append animStyleExtra (dotNetObject "System.Windows.Forms.DataGridViewCellStyle")
			animStyleExtra[animStyleExtra.count].backColor = animStyleExtra[animStyleExtra.count].backColor.white
		)
		
		
		
		
			--// folder \\--
		animStyle[2].backColor = animStyle[2].backColor.FromArgb 230 245 245
		animStyle[2].foreColor = animStyle[2].foreColor.FromArgb 180 0 0
		animStyleExtra[2].backColor = animStyleExtra[2].backColor.FromArgb 230 245 245
		
			--// bip \\--
		animStyle[3].backColor = animStyle[3].backColor.FromArgb 255 255 230
		
			--// max \\--
		animStyle[4].backColor = animStyle[4].backColor.FromArgb 255 230 230
		
			--// fbx \\--
		animStyle[5].backColor = animStyle[5].backColor.FromArgb 230 255 230
		
			--// htr \\--
		animStyle[6].backColor = animStyle[6].backColor.FromArgb 255 234 213
		
			--// xaf \\--
		animStyle[7].backColor = animStyle[7].backColor.FromArgb 240 240 255
		
			--// anim \\--
		animStyle[8].backColor = animStyle[8].backColor.FromArgb 245 245 245
		
			--// loaded \\--
		animStyle[9].backColor = animStyle[9].backColor.FromArgb 200 100 100
		animStyleExtra[3].backColor = animStyleExtra[3].backColor.FromArgb 200 100 100
		
			--// CBA colored \\--
		animStyle[10].backColor = animStyle[10].backColor.FromArgb 240 240 240
		animStyle[10].foreColor = animStyle[10].foreColor.FromArgb 150 0 0
		animStyle[10].Alignment = animStyle[10].Alignment.MiddleCenter
		
			--// CBA white \\--
		animStyle[11].backColor = animStyle[11].backColor.FromArgb 255 255 255
		animStyle[11].foreColor = animStyle[11].foreColor.FromArgb 0 0 0
		animStyle[11].Alignment = animStyle[11].Alignment.MiddleCenter
		
		
		
		
		local scriptFilter = filterString (getSourceFileName()) "\\"
		for i = 1 to (scriptFilter.count - 1) do
			scriptPath += scriptFilter[i] + "\\"
		
		characterSettingsPath = scriptPath + "AnimManager_characters.xml"
		importSettingsPath = scriptPath + "AnimManager_import.xml"
		exportSettingsPath = scriptPath + "AnimManager_export.xml"
		userSettingsPath = scriptPath + "AnimManager_user.xml"
		importUserSettingsPath = scriptPath + "AnimManager_import_user.xml"
		exportUserSettingsPath = scriptPath + "AnimManager_export_user.xml"
		
		logFilePath = scriptPath + "AnimManager_log.txt"
	)
)










cryAM = undefined

gc light:true

cryAM = cryAM_Struct()

cryAM.initCryAM()
cryAM.startCryAM()


	--// workaround for MaxDialog \\--
--cryAM.main.backColor = cryAM.main.backColor.FromArgb 87 87 87


-- TEST --