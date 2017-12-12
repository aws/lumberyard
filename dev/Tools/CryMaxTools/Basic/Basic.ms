gc light:true

-- Basic functionality needed for all tools
-- shared functions

	--// set heap size to 30mb if not higher
(	
	local freeHeap = heapFree / 1048576.0
	if (freeHeap - 10) <= 0 then
	(
		heapSize += (10 - freeHeap) * 1048576.0
		print ("New HeapSize :   " + (heapSize / 1048576.0) as String)
	)
		
	
)


	--// BASIC STRUCTURES \\--

struct cryMaxToolsSettingsManStruct
(
	load,
	save
)

	--// PERFORCE \\--
struct cryMaxToolsPerforceManStruct
(
	perforceDir = "",
	tempFilePath = (sysInfo.tempDir + "cryMaxTools_perforce.at"),
	readTempFile,
	localToDepot,
	depotToLocal,
	getFileStats,
	getLatestRev,
	open,
	cryAdd,
	getPath,
	getPerforcePath,
	checkForLoad,
	checkForSave,
	setWrite,
	setWriteMessage,
	checkForAdd
)


struct cryMaxToolsROManDialogStruct
(
	main,
	name,
	rollouts = #()
)


struct cryMaxToolsROManStruct
(
	get,
	cryAdd,
	remove,
	dialogs = #()
)


struct cryMaxToolsBasicVarsStruct
(
	various = #(),
	buildPath = (csexport.get_root_path()),
	toolsPath = "",
	tempFilePath = (sysInfo.tempDir + "cryMaxTools_tempFile.dmp"),
	silentWarnings = true
)


struct cryMaxToolsStrStruct
(
	lower,
	upper,
	getFilename,
	getDirectory,
	getExtension,
	getFilenameNoExt,
	cutStringToWidth,
	hasStr
)




struct cryMaxToolsMiscStruct
(
	makeUnique,
	compareStructures,
	groupBoxConverter
)







struct cryMaxToolsBasicStruct
(
	settingsMan = cryMaxToolsSettingsManStruct(),
	perforceMan = cryMaxToolsPerforceManStruct(),
	ROMan = cryMaxToolsROManStruct(),
	menu,
	scmd,
	str = cryMaxToolsStrStruct(),
	misc = cryMaxToolsMiscStruct(),
	
	vars = cryMaxToolsBasicVarsStruct()
)











struct cryMaxToolsStruct
(
		--// tools \\--
	anim,	--// animation tools
	char, --// character/rigging tools
	model, --// modeling tools
	export, --// exporter tools
	
		--// basic functionality \\--
	basic = cryMaxToolsBasicStruct(),
	toolBox,
	
		--// shared functions \\--
	getchildren, --returns all nodes in hierarchy below this node
	findRoot, --returns root of hierarchy above this node
	getBips --returns all bip roots in a scene
)




global cryMaxTools = cryMaxToolsStruct()







fn getSkinOps sel:undefined =
(
	local inter = skinOps
	
	if classOf (modPanel.GetCurrentObject()) == Skin and sel == undefined then
		sel = modPanel.GetCurrentObject()
	
	if superClassOf sel == modifier then
	(
		try
		(
			if(crySkinOps.isCrySkin(sel)) then
				inter = crySkinOps
		)
		catch()
	)
	
	inter
)









cryMaxTools.basic.ROMan.cryAdd = function cryAdd dialog ro op =
(
	local dialogName = ""
	
	if dialog == "" or op == #main then
	(
		
		try ( dialogName = ro.name )
		catch ( dialogName = ro.title )
	)
	else
		dialogName = dialog
	
	
	local ROFound = false
	local dialogFound = false
	
	for i = 1 to cryMaxTools.basic.ROMan.dialogs.count do
	(
		if cryMaxTools.basic.ROMan.dialogs[i].name == dialogName then
		(
			dialogFound = i
			exit
		)
	)
	
	
	case op of
	(
		#main:
		(
			if dialogFound == false then
			(
				append cryMaxTools.basic.ROMan.dialogs (cryMaxToolsROManDialogStruct main:ro name:dialogName)
				return true
			)
			else
			(
				cryMaxTools.basic.ROMan.dialogs[dialogFound].main = ro
				return true
			)
		)
		
		#sub:
		(
			if dialogFound == false then
				return false
			else
			(
				for i = 1 to cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts.count do
				(
					if cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[i].name == ro.name then
					(
						ROFound = i
						exit
					)
				)
				
				if ROFound == false then
				(
					append cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts ro
					addRollout cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts.count] cryMaxTools.basic.ROMan.dialogs[dialogFound].main

					return true
				)
				else
				(
					removeRollout cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[ROFound] cryMaxTools.basic.ROMan.dialogs[dialogFound].main
					cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[ROFound] = ro
					addRollout cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[ROFound] cryMaxTools.basic.ROMan.dialogs[dialogFound].main
					
					return true
				)
			)
		)
	)
	
	return false
)
cryAdd = undefined



cryMaxTools.basic.ROMan.remove = function remove dialog roName op =
(
	local ROFound = false
	local dialogFound = false
	
	for i = 1 to cryMaxTools.basic.ROMan.dialogs.count do
	(
		if cryMaxTools.basic.ROMan.dialogs[i].name == dialog then
		(
			dialogFound = i
			exit
		)
	)
	
	
	
	
	case op of
	(
		#main:
		(
			if dialogFound == false then
				return false
			else
			(
				try closeRolloutFloater cryMaxTools.basic.ROMan.dialogs[dialogFound].main catch()
				try destroyDialog cryMaxTools.basic.ROMan.dialogs[dialogFound].main catch()
				deleteItem cryMaxTools.basic.ROMan.dialogs dialogFound
				return true
			)
		)
		
		#sub:
		(
			if dialogFound == false then
				return false
			else
			(
				for i = 1 to cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts.count do
				(
					if cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[i].name == roName then
					(
						ROFound = i
						exit
					)
				)
				
				if ROFound == false then
					return false
				else
				(
					removeRollout cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts[ROFound] cryMaxTools.basic.ROMan.dialogs[dialogFound].main
					deleteItem cryMaxTools.basic.ROMan.dialogs[dialogFound].rollouts ROFound
					
					return true
				)
			)
		)
	)
	
	return false
)
remove = undefined




cryMaxTools.basic.ROMan.get = function get rolloutTitle element:undefined =
(
	local ROFound = false
	local elementFound = false
	
	
	for i = 1 to cryMaxTools.basic.ROMan.dialogs.count do
	(
		if cryMaxTools.basic.ROMan.dialogs[i].name == rolloutTitle then
		(
			ROFound = cryMaxTools.basic.ROMan.dialogs[i].main
			exit
		)
		else
		(
			for d = 1 to cryMaxTools.basic.ROMan.dialogs[i].rollouts.count do
			(
				if (cryMaxTools.basic.ROMan.dialogs[i].rollouts[d].name == rolloutTitle) or (cryMaxTools.basic.ROMan.dialogs[i].rollouts[d].title == rolloutTitle) then
				(
					ROFound = cryMaxTools.basic.ROMan.dialogs[i].rollouts[d]
					exit
				)
			)
		)
	)
	
	
	
	if element != undefined then
	(
		if ((classOf ROFound) as String) != "RolloutFloater" then
		(	
			if ROFound != false then
			(
				for i = 1 to ROFound.controls.count do
				(
					if ROFound.controls[i].name == element then
						elementFound = ROFound.controls[i]
				)
			)
		)
	)
	
	
	if element != undefined then
	(
		if elementFound != false then
			return elementFound
		else
		(
			if element == "" and ROFound != false then
				return ROFound
			else
				return undefined
		)
	)
		
	
	if ROFound != false then
		return ROFound
	
	return undefined
)
get = undefined








cryMaxTools.basic.settingsMan.save = function settingsManSave elementArray =
(
	local saveString = ""
	
	for i = 2 to elementArray.count do
	(
		local tempValue = elementArray[i] as String
		
		case ((classOf elementArray[i]) as String) of
		(
			"RolloutClass":( tempValue = elementArray[i].name + "*" + (getDialogPos elementArray[i]) as String + "|" + (if elementArray[i].open == true then "1" else "0" ) )
			"RolloutFloater": ( tempValue = elementArray[i].title + "*" + elementArray[i].pos as String )
			"RadioControl": ( tempValue = elementArray[i].name + "*" + elementArray[i].state as String )
			"CheckBoxControl": ( tempValue = elementArray[i].name + "*" + (if elementArray[i].checked == true then "1" else "0") )
			"EditTextControl": ( tempValue = elementArray[i].name + "*@" + elementArray[i].text )
			"LabelControl": ( tempValue = elementArray[i].name + "*@" + elementArray[i].text )
			"ComboBoxControl": (tempValue = elementArray[i].name + "*" + elementArray[i].selection as String )
			"CheckButtonControl": (tempValue = elementArray[i].name + "*" + (if elementArray[i].checked == true then "1" else "0") )
			"SpinnerControl": (tempValue = elementArray[i].name + "*" + elementArray[i].value as String )
		)
		
		
		
		saveString += tempValue + (if i < elementArray.count then "?" else "")
	)
	
	try csexport.set_value (elementArray[1]) saveString catch( return false )
	if elementArray.count > 1 then
		return true
	else
		return false
)
settingsManSave = undefined




cryMaxTools.basic.settingsMan.load = function load entry intoRollout:undefined returnPos:undefined =
(
	local tempString = csexport.get_value entry
	if tempString != "" then
	(
		local outputArray = #()
		
		struct cryMaxToolsSettingsManOutputStruct
		(
			name,
			value = #()
		)
		
		
		local filtered = filterString tempString "?"
		
		
			--// go througth the rest of the list and extract the data
		for i = 1 to filtered.count do
		(
			local dataFiltered = filterString filtered[i] "*"
			local dataValue = #()
			
			if dataFiltered.count > 1 then
			(
				local dataVariable = dataFiltered[1]
				local dataValueFiltered = filterString dataFiltered[2] "|"
				
				
				for d = 1 to dataValueFiltered.count do
				(
					if dataValueFiltered[d][1] == "@" then
					(
						dataValueFiltered[d] = subString dataValueFiltered[d] 2 dataValueFiltered[d].count
						append dataValue dataValueFiltered[d]
					)
					else
					(
						if dataValueFiltered[d] != "" and dataValueFiltered[d] != undefined then
							append dataValue (execute dataValueFiltered[d])
					)		
				)
			)
			
			
			append outputArray (cryMaxToolsSettingsManOutputStruct name:dataFiltered[1] value:dataValue)
		)
		
		
		if intoRollout != undefined then
		(
			local returnArray = #()
			
			for i = 1 to outputArray.count do
			(
				local tempCon = cryMaxTools.basic.ROMan.get entry element:(if outputArray[i].name != entry then outputArray[i].name else "")
				
				if tempCon != undefined then
				(
					case ((classOf tempCon) as String) of
					(
						"RolloutClass":( if returnPos == undefined then setDialogPos tempCon outputArray[i].value[1] else append returnArray outputArray[i] ; tempCon.open = (if outputArray[i].value[2] == 1 then true else false) )
						"RolloutFloater": ( tempCon.pos = outputArray[i].value[1] )
						"RadioControl": ( tempCon.state = outputArray[i].value[1] )
						"CheckBoxControl": ( tempCon.checked = (if outputArray[i].value[1] == 1 then true else false) )
						"EditTextControl": ( tempCon.text = outputArray[i].value[1] )
						"LabelControl": ( tempCon.text = outputArray[i].value[1] )
						"ComboBoxControl": ( tempCon.selection = outputArray[i].value[1] )
						"CheckButtonControl": ( tempCon.state = (if outputArray[i].value[1] == 1 then true else false) )
						"SpinnerControl": ( tempCon.value = outputArray[i].value[1] )
					)
				)
				else
					append returnArray outputArray[i]
			)
			
			if returnArray.count == 0 then
				return true
			else
				return returnArray
			
		)
		else
			return outputArray
	)
	
	
	return false
)
load = undefined





cryMaxTools.basic.scmd = function scmd inputString wait =
(
	csexport.export.execute_command_line ("cmd /c \"" + inputString + "\"") wait
)
scmd = undefined














cryMaxTools.basic.perforceMan.readTempFile = function readTempFile =
(
	local tempSize = 0
	local tempString = ""
	local tempStream = undefined
	
		--// if file temp file exists
	if doesFileExist cryMaxTools.basic.perforceMan.tempFilePath == true then
	(
	
			--// wait, until maxscript can access the file
		while tempStream == undefined do --tempSize < 1 do
		(
			escapeEnable = true
			tempStream = openFile cryMaxTools.basic.perforceMan.tempFilePath mode:"r"
		)
			
		
		--// open the temp file
		if getFileSize cryMaxTools.basic.perforceMan.tempFilePath == 0 then
		(
			--print "Perforce not found..."
			return undefined
		)
			--// read all lines
		while eof tempStream != true do
			tempString += (readLine tempStream) + "\n"

			--// close the file
		close tempStream
			--// delete file
		deleteFile cryMaxTools.basic.perforceMan.tempFilePath
		
			--// returns the lines of the file
		return tempString
	)
	else
		return undefined
)
readTempFile = undefined











cryMaxTools.basic.perforceMan.localToDepot = function localToDepot inputString =
(
	try
	(
		buildFilter = filterString (csexport.get_root_path()) "\\"
		buildString = ""
		for i = 1 to (buildFilter.count - 1) do
			buildString += buildFilter[i] + "\\"
			
		perforceFilter = filterString cryMaxTools.basic.perforceMan.perforceDir "/"
		perforceString = "//"
		for i = 1 to (perforceFilter.count - 1) do
			perforceString += perforceFilter[i] + (if i < (perforceFilter.count - 1) then "/" else "")
		
		pathString = substring inputString buildString.count inputString.count

		tempFilter = filterString pathString "\\"
		if tempFilter.count > 1 then
		(
			tempString = perforceString
			for i = 1 to tempFilter.count do
				tempString += "/" + tempFilter[i]
			
			return tempString
		)
		else
			return false
			
	)catch (return undefined)
)
localToDepot = undefined



cryMaxTools.basic.perforceMan.depotToLocal = function depotToLocal inputString =
(
	try
	(
		tempFilter = filterString inputString "/"
		if tempFilter.count > 1 then
		(
			tempString = (filterString cryMaxTools.basic.vars.buildPath "\\")[1]
			
			for i = 1 to tempFilter.count do
				tempString += "\\" + tempFilter[i]
		
			
			return tempString
		)
		else
			return false
	)
	catch(return undefined)
)
depotToLocal = undefined













cryMaxTools.basic.perforceMan.getFileStats = function getFileStats inputString =
(
	struct tempATFileStruct
	(
		depotFile,
		clientFile,
		headRev,
		haveRev,
		otherOpen,
		userOpen,
		exist
	)
	
	tempStruct = tempATFileStruct exist:true
	
	tempPath = cryMaxTools.basic.perforceMan.getPath inputString
	
	if (tempPath != false) and (tempPath != undefined) then
	(					
		cryMaxTools.basic.scmd ("p4 -s fstat " + tempPath + " > \"" + cryMaxTools.basic.perforceMan.tempFilePath + "\"") true
	
		tempString = ""
		while tempString == "" do
		(
			escapeEnable = true
			tempString = cryMaxTools.basic.perforceMan.readTempFile()
		)
		if tempString != undefined then
		(
			tempFilter = filterString tempString "\n"
			
			
			for i = 1 to tempFilter.count do
			(
				tempSecFilter = filterString tempFilter[i] " "
				if (findString tempFilter[i] "depotFile") != undefined then
					tempStruct.depotFile = tempSecFilter[tempSecFilter.count]
				if (findString tempFilter[i] "clientFile") != undefined then
					tempStruct.clientFile = tempSecFilter[tempSecFilter.count]
				if (findString tempFilter[i] "headRev") != undefined then
					tempStruct.headRev = tempSecFilter[tempSecFilter.count]
				if (findString tempFilter[i] "haveRev") != undefined then
					tempStruct.haveRev = tempSecFilter[tempSecFilter.count]
				if (findString tempFilter[i] "otherOpen0") != undefined then
					tempStruct.otherOpen = tempSecFilter[tempSecFilter.count]
				if (findString tempFilter[i] "actionOwner") != undefined then
					tempStruct.userOpen = true
				if (findString tempFilter[i] "no such file") != undefined then
					tempStruct.exist = false
			)
			
			return tempStruct
		)
	)
	return undefined
)
getFileStats = undefined





cryMaxTools.basic.perforceMan.getLatestRev = function getLatestRev inputString =
(
	if inputString[inputString.count] == "\\" then
		inputString += "..."
	
	
	local perforcePath = cryMaxTools.basic.perforceMan.getPath inputString
	if perforcePath != undefined then
		cryMaxTools.basic.scmd ("p4 sync -f " + perforcePath) true
	
	return true
)
getLatestRev = undefined






cryMaxTools.basic.perforceMan.open =  function perforceManOpen inputString =
(
	cryMaxTools.basic.perforceMan.getLatestRev inputString
	local tempPath = cryMaxTools.basic.perforceMan.getPath inputString
	if tempPath == "" or tempPath == undefined then
		return false
	else
	(
		cryMaxTools.basic.scmd ("p4 edit " + tempPath + " > \"" + cryMaxTools.basic.perforceMan.tempFilePath + "\"") true
		
		local tempString = ""
		while tempString == "" do
		(
			escapeEnable = true
			tempString = cryMaxTools.basic.perforceMan.readTempFile()
		)
		
		try 
		(
			if findString tempString "can't edit" != undefined then
				return false
		)catch( return false )
		
		cryMaxTools.basic.perforceMan.setWriteMessage inputString
		
		return true
	)
)
perforceManOpen = undefined





cryMaxTools.basic.perforceMan.cryAdd =  function perforceManAdd inputString =
(
	local tempBinary = ""
	local tempString = cryMaxTools.basic.str.lower (cryMaxTools.basic.str.getFilename inputString)
	--tempString = cryTools.cryAnim.base.lowercase fileType
	
	if (findString tempString ".caf") != undefined then tempBinary = "-t binary+l "
	if (findString tempString ".bip") != undefined then tempBinary = "-t binary+l "
	
	cryMaxTools.basic.scmd ("p4 add " + tempBinary + (cryMaxTools.basic.perforceMan.localToDepot inputString) ) true
	return true
)
perforceManAdd = undefined




cryMaxTools.basic.perforceMan.getPath =  function getPath inputString =
(
	cryMaxTools.basic.scmd ("p4 where " + inputString + " > \"" + cryMaxTools.basic.perforceMan.tempFilePath + "\"") true
	
	local tempString = ""
	while tempString == "" do
	(
		escapeEnable = true
		tempString = cryMaxTools.basic.perforceMan.readTempFile()
	)
	
	try ( return (filterString tempString " ")[1] ) catch (return undefined)
)
getPath = undefined



cryMaxTools.basic.perforceMan.getPerforcePath = function getPerforcePath =
(
	local perforceString = ""
	local buildFilter = filterString (csexport.get_root_path()) "\\"
	for i = 1 to buildFilter.count do
		perforceString += buildFilter[i] + (if i < buildFilter.count then "\\" else "")
						
	local perforceString = cryMaxTools.basic.perforceMan.getPath perforceString
	
	return perforceString
)
getPerforcePath = undefined





cryMaxTools.basic.perforceMan.checkForLoad =  function checkForLoad inputString =
(
	tempStruct = cryMaxTools.basic.perforceMan.getFileStats inputString
	if tempStruct != undefined then
	(
		if tempStruct.exist == true then
		(
			if tempStruct.otherOpen != undefined then
			(
				messageBox ("The file is currently opened by:\n\n" + tempStruct.otherOpen) title:("Perforce Check:    " + (cryMaxTools.basic.str.getFilename inputString))
			)
			else
			(
				if tempStruct.userOpen == undefined then
				(
					if (queryBox "Do you want to 'Open for Edit' ?" title:("Perforce Check:   " + (cryMaxTools.basic.str.getFilename inputString))) == true then
					(
						if tempStruct.headRev != tempStruct.haveRev then
						(
							cryMaxTools.basic.perforceMan.getLatestRev inputString
						)
						
						cryMaxTools.basic.perforceMan.open inputString
					)
					else
						return false
				)							
				return true
			)
		)	
		
		return false
	)
	else
		return undefined
)
checkForLoad = undefined







cryMaxTools.basic.perforceMan.checkForSave =  function perforceManCheckForSave inputString =
(
	
	local tempCBAStruct = cryMaxTools.basic.perforceMan.getFileStats (cryMaxTools.basic.vars.buildPath + "Game\\Animations\\Animations.cba")
	if tempCBAStruct != undefined then
	(
		if tempCBAStruct.headRev != tempCBAStruct.haveRev then
			if (cryMaxTools.basic.perforceMan.getLatestRev (cryTools.buildPathFull + "Game\\Animations\\Animations.cba") ) == true then
				print "Synched - Animations.cba"
	)
	
	local tempStruct = cryMaxTools.basic.perforceMan.getFileStats inputString
	
	
	if tempStruct != undefined then
	(
		if tempStruct.exist == true then
		(
			if tempStruct.headRev != tempStruct.haveRev then
			(
				cryMaxTools.basic.perforceMan.getLatestRev inputString
			)
			
			if tempStruct.userOpen == undefined then
			(
				if tempStruct.otherOpen != undefined then
				(
					return (queryBox ("The file is currently opened by:\n\n" + tempStruct.otherOpen + "\n\nOverwrite File?") title:("Perforce Check:    " + (cryMaxTools.basic.str.getFilename inputString)) beep:true)
				)
				if (queryBox "Do you want to 'Open for Edit' ?" title:("Perforce Check:   " + (cryMaxTools.basic.str.getFilename inputString))) == true then
				(
					print inputString
					cryMaxTools.basic.perforceMan.open inputString
					cryMaxTools.basic.perforceMan.localToDepot "J:\data\Game02_Production\Art\Animation\Human\Male\relaxed\relaxed_walkUphill_nw_forward_fast_01.bip"
					
					return true
				)
				else
				(
					cryMaxTools.basic.perforceMan.setWriteMessage inputString
					return false
				)
													
			)
			else
				return true						
		)
		else
		(
			cryMaxTools.basic.perforceMan.setWriteMessage inputString
			return false
		)
	)
	else
		return undefined
)
perforceManCheckForSave = undefined







cryMaxTools.basic.perforceMan.setWrite =  function setWrite inputString =
(
		--// set attribute to write --
	if (getFiles inputString).count > 0 then
	(
		if (getFileAttribute inputString #readOnly) == true then
		(
			setFileAttribute inputString #readOnly false
			return true
		)
		else
			return false
	)
	else
		return undefined
)
setWrite = undefined







cryMaxTools.basic.perforceMan.setWriteMessage =  function setWriteMessage inputString =
(	
	if (cryMaxTools.basic.perforceMan.setWrite inputString) == true then
	(
		if cryMaxTools.basic.vars.silentWarnings == false then
			messageBox ("Set File Attribute to Write") title:("File Check:    " + (cryMaxTools.basic.str.getFilename inputString))
		else
			print ("Set File Attribute to Write: " + (cryMaxTools.basic.str.getFilename inputString) )
				
		return true
	)
	else
		return false
)
setWriteMessage = undefined





cryMaxTools.basic.perforceMan.checkForAdd =  function checkForAdd inputString =
(				
	tempStruct = cryMaxTools.basic.perforceMan.getFileStats inputString
	if tempStruct != undefined then
	(
		if tempStruct.exist == false then
		(
			if (queryBox "The file is not in the database.\nAdd to Source Control?" title:("Perforce Check:   " + (cryMaxTools.basic.str.getFilename inputString))) == true then
			(
				cryMaxTools.basic.perforceMan.cryAdd inputString
			)
		)
	)
)
checkForAdd = undefined

























cryMaxTools.basic.str.lower = function lower inputString =
(
	local tempString = ""
		
	for i = 1 to inputString.count do
	(
		x = inputString[i]
		n = bit.charAsInt x
		if (n > 64) and (n < 90) then
			x = bit.intAsChar (n + 32)
		tempString += x
	)
	return tempString
)
lower = undefined



cryMaxTools.basic.str.upper = function upper inputString =
(
	local tempString = ""
		
	for i = 1 to inputString.count do
	(
		x = inputString[i]
		n = bit.charAsInt x
		if (n > 96) and (n < 122) then
			x = bit.intAsChar (n - 32)
		tempString += x
	)
	return tempString
)
upper = undefined










cryMaxTools.basic.str.getFilename = function getFilename inputString =
(
	if inputString != undefined and inputString != "" then
	(
		local tempFilter = filterString inputString "\\"
		return tempFilter[tempFilter.count]
	)
	else
		return ""
)
getFilename = undefined



cryMaxTools.basic.str.getExtension = function getExtension inputString =
(
	if inputString != undefined then
	(
		local tempFilter = filterString inputString "."
		if tempFilter.count > 1 then
			return tempFilter[tempFilter.count]
		else
			return ""
	)
	else
		return ""
)
getExtension = undefined







cryMaxTools.basic.str.getFilenameNoExt = function getFilenameNoExt inputString =
(
	if inputString != undefined then
	(
		local tempFilter = filterString (cryMaxTools.basic.str.getFilename inputString) "."
		if tempFilter.count > 1 then
		(
			local tempString = ""
			
			for i = 1 to (tempFilter.count - 1) do
				tempString += tempFilter[i]
		
			return tempString
		)
		else
			return ""
	)
	else
		return ""
)
getFilenameNoExt = undefined









cryMaxTools.basic.str.getDirectory = function getDirectory inputString =
(
	tempFilter = filterString inputString "\\"
	tempString = ""
	for i = 1 to (tempFilter.count - 1) do
		tempString += tempFilter[i] + "\\"
	
	return tempString
)
getDirectory = undefined







cryMaxTools.basic.str.cutStringToWidth = function cutStringToWidth str1 width align:#left =
(
	local newString = ""
	
	if str1 != "" then
	(
		if (getTextExtent str1)[1] > width then
		(
			--// check for path
			if findString str1 "\\" != undefined then
			(
				
				local tempFilter = filterString str1 "\\"
				local compressedPath = ""
				
				case align of
				(
					#left:
					(
						compressedPath = tempFilter[1] + "\\"
						
						for i = 2 to (tempFilter.count - 1) do
						(
							if (getTextExtent (compressedPath + tempFilter[i]))[1] < width then
								compressedPath += tempFilter[i] + (if i < (tempFilter.count) then "\\" else "")
							else
								exit
						)
					
						newString = compressedPath + ".."
					)
					
					#right:
					(
						compressedPath = "\\" + tempFilter[tempFilter.count] + "\\"
						
						for i = (tempFilter.count - 1) to 1 by -1 do
						(
							if (getTextExtent (tempFilter[i] + compressedPath ))[1] < width then
								compressedPath = (if i > 1 then "\\" else "") + tempFilter[i] + compressedPath
							else
								exit
						)
						
						newString = ".." + compressedPath
					)
				)
			)
		)
	)
	
	return newString
)
cutStringToWidth = undefined







cryMaxTools.basic.str.hasStr = function findStr str searchStr isCase:false =
(
	if isCase == false then
		return (findString str searchStr != undefined)
	else
	(
		local strFound = false
		
		for i = 1 to (str.count - searchStr.count + 1) do
		(
			if subString str i (searchStr.count) == searchStr then
			(
				strFound = true
				exit
			)
		)
		
		return strFound
	)
)







cryMaxTools.basic.misc.makeUnique = function makeUnique var =
(
	local newVal = #()
	
	if classOf var == Array then
	(
		for i = 1 to var.count do
			append newVal (cryMaxTools.basic.misc.makeUnique var[i])
	)
	else
	if superclassOf var == StructDef then
	(
		newVal = copy var
		local propNames = getPropNames var
		
		for i = 1 to propNames.count do
			setProperty newVal propNames[i] (cryMaxTools.basic.misc.makeUnique (getProperty var propNames[i]))
	)
	else
		newVal = copy var
	
	return newVal
)
makeUnique = undefined





cryMaxTools.basic.misc.compareStructures = function compareStructures var1 var2 started:undefined skipDotNet:true debug:false skipFirstArrayEntries:0 =
(
	if started == undefined then
	(
		if debug == true then print ("DEBUG MODE")
		
		global compareStructuresResult = true
		cryMaxTools.basic.misc.compareStructures var1 var2 started:true skipDotNet:skipDotNet debug:debug skipFirstArrayEntries:skipFirstArrayEntries

		local tempVar = compareStructuresResult
		compareStructuresResult = undefined
		
		return tempVar
	)
	else
	(
		if compareStructuresResult == false then
			return false
		
		
		if classOf var1 == Array and classOf var2 == Array then
		(
			if var1.count != var2.count then
			(
				if debug == true then print ("ARRAY COUNT CHECK    VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
				compareStructuresResult = false
				return false
			)
			else
				for i = (1 + skipFirstArrayEntries) to var1.count do
					cryMaxTools.basic.misc.compareStructures var1[i] var2[i] started:true skipDotNet:skipDotNet debug:debug
		)
		else
		if superclassOf var1 == StructDef and superclassOf var2 == StructDef then
		(
			local propNames1 = getPropNames var1
			local propNames2 = getPropNames var2
			
			if propNames1.count != propNames2.count then
			(
				if debug == true then print ("PROPERTY COUNT CHECK    VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
				compareStructuresResult = false
				return false
			)
			else
			(
				for i = 1 to propNames1.count do
				(
					if propNames1[i] != propNames2[i] then
					(
						if debug == true then print ("PROPERTY NAME CHECK    VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
						compareStructuresResult = false
						return false
					)
					else
					(
						local prop1 = getProperty var1 propNames1[i]
						local prop2 = getProperty var2 propNames2[i]
						
						cryMaxTools.basic.misc.compareStructures prop1 prop2 started:true skipDotNet:skipDotNet debug:debug
					)	
				)
			)
		)
		else
		(
			if var1 != var2 then
			(
				if skipDotNet == false then
				(
					if debug == true then print ("DOTNET CHECK    VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
					compareStructuresResult = false
					return false
				)
				else if classOf var1 == dotNetObject or classOf var2 == dotNetObject then
				(
					return true
				)
				else if classOf var1 == matrix3 and classOf var2 == matrix3 then
				(
					if var1.row1 != var2.row1 or var1.row2 != var2.row2 or var1.row3 != var2.row3 or var1.row4 != var2.row4 then
					(
						if debug == true then print ("Matrix3 CHECK   VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
						compareStructuresResult = false
						return false
					)
				)
				else
				(
					if debug == true then print ("VAR1 :::::::::::: " + var1 as String + "       VAR2 :::::::::::: " +  var2 as String)
					compareStructuresResult = false
					return false
				)
			)
		)
	)
	
	return true
)



cryMaxTools.basic.misc.groupBoxConverter = function groupBoxConverter gb flipHorizontal:false re:undefined labelOffset:1 edgeWidth:1 roundEdge:false =
(
		--// gb[1]	=	groupBox
		--// gb[2]	=	label
		--// gb[3]	=	topLeft
		--// gb[4]	=	topRight
		--// gb[5]	=	left
		--// gb[6]	=	right
		--// gb[7]	=	bottom
	
	--local posOffset = groupBoxEdgeWidth
	
	local reVal = 0
	if roundEdge == true then
		reVal = 1
	
	if re == undefined then
		local re = #(reVal,reVal,reVal,reVal)
	
	
	
	
	gb[2].text = gb[1].text
	local labelExtent = (getTextExtent gb[2].text)[1]
	
	
	
	if flipHorizontal == false then
	(
		gb[2].pos = gb[1].pos + [((gb[1].width / 2) - (labelExtent / 2)), 0]
		gb[3].pos = gb[1].pos + [re[1], 7]
		gb[4].pos = gb[2].pos + [(labelExtent + labelOffset - 3 - re[2]), 7]
		gb[7].pos = gb[1].pos + [re[3], (gb[1].height - edgeWidth)]
		
		gb[5].pos = gb[1].pos + [0, (7 + re[1])]
		gb[6].pos = gb[1].pos + [(gb[1].width - edgeWidth), (7 + re[2])]
	)
	else
	(
		gb[2].pos = gb[1].pos + [((gb[1].width / 2) - (labelExtent / 2)), (gb[1].height - 7)]
		gb[3].pos = gb[1].pos + [re[1], (gb[1].height - edgeWidth + 2)]
		gb[4].pos = gb[2].pos + [(labelExtent + labelOffset - 3 - re[2]), (9 - edgeWidth)]
		gb[7].pos = gb[1].pos + [re[3], 9]
		
		gb[5].pos = gb[1].pos + [0, (9 + re[3])]
		gb[6].pos = gb[1].pos + [(gb[1].width - edgeWidth), (9 + re[4])]
	)
	
	
	
	gb[3].height = gb[4].height = gb[5].width = gb[6].width = gb[7].height = edgeWidth
	gb[5].height = gb[6].height = gb[1].height - 7 - (re[1] + re[3])
	
	gb[3].width = gb[2].pos[1] - gb[1].pos[1] - labelOffset + re[1] + 1
	gb[4].width = gb[6].pos[1] - gb[4].pos[1] + edgeWidth - re[2]
	gb[7].width = gb[1].width - (re[3] + re[4])
	
	

	gb[1].visible = false
)
groupBoxConverter = undefined














	


tempFunction = undefined

-----------------------------
-- SHARED FUNCTIONS
-----------------------------

cryMaxTools.getChildren = fn getChildren sNode showHidden:true =
(--return all nodes in hierarchy below given node - sorted alphabetically
	aChildren = #(sNode)
	
	local i = 1
	while i <= aChildren.count do
	(
		for sObj in aChildren[i].children do
		(
			if showHidden or sObj.isHidden == false then
			(
				append aChildren sObj
			)
		)
		i += 1
	)

	deleteItem aChildren 1
	
	--sort by name
	fn fnSortByNodesName n1 n2 =
	(
		case of
		(
			(n1.name > n2.name): 1
			(n1.name < n2.name): -1
			default: 0
		)
	)
	qsort aChildren fnSortByNodesName	
	
	aChildren
)
getChildren = undefined



cryMaxTools.findRoot = fn findRoot sNode =
(--finds root of node's hierarchy
	if sNode.parent == undefined then
	(
		local result = sNode
	)
	else
	(
		local result = cryMaxTools.findRoot sNode.parent
	)
	result
)
findRoot = undefined




cryMaxTools.getBips = fn getBips =
(
	Bips = #()
	for obj in geometry do
	(
		if classof obj.controller == Vertical_Horizontal_Turn then 
		(
			append Bips obj
		)
	)
	return Bips
)
getBips = undefined

--// init vars \\--
cryMaxTools.basic.vars.toolsPath = (if globalCryMaxToolsPath == undefined then (csexport.get_root_path() + "\\Tools\\CryMaxTools\\") else globalCryMaxToolsPath + "CryMaxTools\\")
cryMaxTools.basic.perforceMan.perforceDir = cryMaxTools.basic.perforceMan.getPerforcePath()

fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Anim.ms")
fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Character.ms")
fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Exporter.ms")
fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Model.ms")
fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\Menu.ms")