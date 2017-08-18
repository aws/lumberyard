------------------------
-- CryMaxTools v2.0
-- Toolbox v2.0
-- by Sascha Herfort
------------------------

--COMMENTS ABOUT V2.0
--THE TOOLBOX WILL BE REDESIGNED TO SUPPORT THE FOLLOWING THINGS:
--DYNAMIC TAB CREATION TO ENABLE CUSTOMIZABLE TAB LAYOUT
--PLUG-IN FOLDER TO ENABLE EASY INSTALLATION OF NEW ROLLOUTS
--SAVE FEATURE FOR TAB LAYOUT AND PER-ROLLOUT SETTINGS

--##########################################################################################
--DEBUG VARIABLES
--##########################################################################################
bPrintFunctionNames = true
bPrintDebugInfo = true
bPrintUserInfo = true

sToolPath = cryMaxTools.basic.vars.toolsPath

if cryMaxTools.toolBox != undefined then
(--destroy toolbox if already present
	try
	(
		cui.FloatDialogBar cryMaxTools.toolBox
		cui.UnRegisterDialogBar cryMaxTools.toolBox
	)
	catch()
	destroyDialog cryMaxTools.toolBox
)

rollout rltCryMaxToolBox "CryToolBox"
(
	local sCryToolboxDock = sToolPath + "Icons\\cryToolboxDock_32i.bmp"
	local sCryToolboxDockMasks = sToolPath + "Icons\\cryToolboxDock_32a.bmp"
	local sCryToolboxTabScroll = sToolPath + "Icons\\cryToolboxTabScroll_32i.bmp"
	local sCryToolboxTabScrollMask = sToolPath + "Icons\\cryToolboxTabScroll_32a.bmp"
	
	button btnDockLeft "<---" height:16 width:48 pos:[0,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,1,1,1,1)
	button btnUndock "- - - -" height:16 width:96 pos:[48,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,2,2,2,2)
	button btnDockRight "--->" height:16 width:48 pos:[144,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,3,3,3,3)
	button btnTabScrollUp "-" height:16 width:16 pos:[0,16] images:#(sCryToolboxTabScroll,sCryToolboxTabScrollMask,4,1,1,3,3)
	button btnTabScrollDown "+" height:16 width:16 pos:[0,32] images:#(sCryToolboxTabScroll,sCryToolboxTabScrollMask,4,2,2,4,4)
	checkbutton ckbtnTab1 "Tab1" height:32 width:32 pos:[16,16]
	checkbutton ckbtnTab2 "Tab2" height:32 width:32 pos:[48,16]
	checkbutton ckbtnTab3 "Tab3" height:32 width:32 pos:[80,16]
	checkbutton ckbtnTab4 "Tab4" height:32 width:32 pos:[112,16]
	checkbutton ckbtnTab5 "Tab5" height:32 width:32 pos:[144,16]
	checkbutton ckbtnSettings "?" height:32 width:16 pos:[176,16] enabled:false
	subrollout rltToolHolder height:480 width:192 pos:[0,48]
	
	local aTabButtons = #(--store tab-button references for easy looping through them
		ckbtnTab1,
		ckbtnTab2,
		ckbtnTab3,
		ckbtnTab4,
		ckbtnTab5
	)
	
--########################################################################################
--TOOLBOX GLOBAL VARIABLES
--########################################################################################
	struct cryMaxToolboxTab
	(--struct for a tab in the toolbox, containing all necessary data
		sName = "", --name of the tab - will be used as tooltip for icon - assumes unique name string
		bmpIcon = undefined, --icon bitmap - assumes valid bitmap value or undefined
		bmpIconMask = undefined, --icon-mask bitmap - assumes valid bitmap value or undefined
		aRollouts = #(), --array of rollout-names of contained rollouts
		iPreferredIndex = undefined --if set the tab will be immovable by user and will attempt to take the set index - positive indices will be filled in first (low to high), then user defined tabs, then negative indices (low to high) - 1 will always be the first tab / -1 will always be the last tab
	)
	
	struct cryMaxToolboxRollout
	(--struct for a rollout in a tab, containing all necessary data
		sName = "", --name of the rollout - must be unique - will be used in rolloutmanager
		sPreferredTab = undefined, --name of preferred tab to be in - if undefined will go to 'unassigned' tab
		sFilepath = "", --location of script file
		rltRollout = "", --reference to the actual rollout - this will be added to the subrolloutholder later
		bStatic = true --if true, user cannot customize location of rollout
	)
	
	local iCurrentTab = 0 --stores index in aTabs array of currently opened tab
	local iTabBarScrollOffset = 0 --stores the scrolling offset in the tab-bar - used when more than 5 tabs present
	local aRollouts = #( --all loaded rollouts - predefined rollouts will be loaded on start - specify only filepath, the rest will come from the file
		cryMaxToolboxRollout sFilepath:(sToolPath + "\\Basic\\ToolBoxSettingsV2.ms")
	)
	local aTabs = #( --array of cryMaxToolboxTab instances - one per tab
		cryMaxToolboxTab sName:"Unassigned" iPreferredIndex:-2,
		cryMaxToolboxTab sName:"Settings" iPreferredIndex:-1
	)
	
--########################################################################################
--TOOLBOX GLOBAL FUNCTIONS
--########################################################################################

--### STRING FUNCTIONS BEGIN ###
	fn fnSplitNumberSuffix sString =
	(--splits off the number suffix of a string and returns an array with the mainstring and the suffix - assumes a string input
		if bPrintFunctionNames then (print "fnSplitNumberSuffix")
		if bPrintDebugInfo then (print ("fnSplitNumberSuffix in: " + sString as string))
		
		local sMainPart = trimRight sString "0123456789" --cut off numbers
		local sNumberPart = subString sString (sMainPart.count+1) -1 --put numberpart into variable
		
		if bPrintDebugInfo then (print ("fnSplitNumberSuffix out: " + #(sMainPart,sNumberPart) as string))
		#(sMainPart,sNumberPart)
	)
	
	fn fnSortStringWithNumberSuffix sString1 sString2 =
	(--qsort function to sort strings with number suffix - compares suffix numerically - assumes strings input
		local aSplitString1 = fnSplitNumberSuffix sString1
		aSplitString1[2] = aSplitString1[2] as integer --convert suffix to int for number comparisson
		local aSplitString2 = fnSplitNumberSuffix sString2
		aSplitString2[2] = aSplitString2[2] as integer --convert suffix to int for number comparisson
		
		case of
		(
			(aSplitString1[1] > aSplitString2[1]): 1
			(aSplitString1[1] < aSplitString2[1]): -1
			default:
			(
				case of
				(
					(aSplitString1[2] > aSplitString2[2]): 1
					(aSplitString1[2] < aSplitString2[2]): -1
					default: 0
				)
			)
		)
	)
	
	fn  fnMakeNameWithNumberSuffixUnique aNames sNewName =
	(--returns a name that will be unique in the list - splits off suffix and compares numercially - assumes array of strings and string input
		if bPrintFunctionNames then (print "fnMakeNameWithNumberSuffixUnique")
		if bPrintDebugInfo then (print ("fnMakeNameWithNumberSuffixUnique in: " + aNames as string + " " + sNewName as string))
		
		local sResult = sNewName
		qSort aNames fnSortStringWithNumberSuffix
		
		for each in aNames do
		(--test against all existing names
			if matchPattern sResult pattern:each ignoreCase:true then
			(--if new names matches current name in array
				local aSplitString = fnSplitNumberSuffix sResult
				local sMainPart = aSplitString[1]
				local sNumberPart = aSplitString[2]
				local iNumberPartLength = sNumberPart.count
				
				--increment numberpart
				sNumberPart = ((sNumberPart as integer ) + 1) as string
				while iNumberPartLength > sNumberPart.count do
				(
					sNumberPart = "0" + sNumberPart
				)
				
				sResult = sMainPart + sNumberPart
			)
		)
		
		if bPrintDebugInfo then (print ("fnMakeNameWithNumberSuffixUnique out: " + sResult as string))
		sResult
	)
--### STRING FUNCTIONS END ###

	
--### TAB FUNCTIONS BEGIN ###
	fn fnGetTabNames =
	(--returns an array of all existing tab names 
		if bPrintFunctionNames then (print "fnGetTabNames")
		
		local aTabNames = #()
		
		for i = 1 to aTabs.count do
		(--make name unique
			append aTabNames aTabs[i].sName
		)
		
		if bPrintDebugInfo then (print ("fnGetTabNames out: " + aTabNames as string))
		aTabNames
	)
	
	fn fnGetTabByName sName =
	(--returns the cryMaxToolboxTab instance with the given name or undefined
		if bPrintFunctionNames then (print "fnGetTabByName")
		if bPrintDebugInfo then (print ("fnGetTabByName in: " + sName as string))
		
		local sResult = undefined
		local aTabNames = fnGetTabNames()
		local iIndex = findItem aTabNames sName
		
		if iIndex != 0 then
		(--if name has been found, return tab
			sResult = cryMaxTools.toolBox.aTabs[iIndex]
		)
		
		if bPrintDebugInfo then (print ("fnGetTabByName out: " + sResult as string))
		sResult
	)
	
	fn fnMakeTabNameUnique sName =
	(--returns a name that will be unique in the tab list - assumes a string input
		if bPrintFunctionNames then (print "fnMakeTabNameUnique")
		if bPrintDebugInfo then (print ("fnMakeTabNameUnique in: " + sName as string))
		
		local sResult = fnMakeNameWithNumberSuffixUnique (fnGetTabNames()) sName
		
		if bPrintDebugInfo then (print ("fnMakeTabNameUnique out: " + sResult as string))
		sResult
	)
	
	fn fnAddTab sName:"newTab" sIcon: sIconMask: aRollouts:#() bStatic:false =
	(--creates a new tab and adds it to the list
		if bPrintFunctionNames then (print "fnAddTab")
		if bPrintDebugInfo then (print ("fnAddTab in: " + sName as string + " " + sIcon as string + " " + sIconMask as string + " " + aRollouts as string  + " " + bStatic as string))
		
		if classOf sName == string then
		(--check name validity
			local sTabName = fnMakeTabNameUnique sName
			
			local bmpIcon = undefined
			if classOf sIcon == string then
			(--check icon bitmap validity
				try
				(--try-catch to avoid max9 runtime error
					bmpIcon = openBitmap sIcon
				)
				catch
				(
					bmpIcon = undefined
				)
			)
			
			local bmpIconMask = undefined
			if classOf sIconMask == string then
			(--check icon-mask bitmap validity
				try
				(--try-catch to avoid max9 runtime error
					bmpIconMask = openBitmap sIconMask
				)
				catch
				(
					bmpIconMask = undefined
				)
			)
			
			local tNewTab = cryMaxToolboxTab sName:sTabName bmpIcon:bmpIcon bmpIconMask:bmpIconMask aRollouts:aRollouts bStatic:bStatic
			if bPrintDebugInfo then (print ("fnAddTab out: " + tNewTab as string))
			append aTabs tNewTab
			
			true
		)
		else
		(
			"Error! Name is not a string!"
		)
	)
	
	fn fnDeleteTab sName: iIndex: =
	(--deletes a tab from the list - by name or index in aTabs array - returns false if tab not existent
		if bPrintFunctionNames then (print "fnDeleteTab")
		if bPrintDebugInfo then (print ("fnDeleteTab in: " + sName as string + " " + iIndex as string))
		
		if sName == unsupplied then
		(--if name not supplied, try using index
			if classOf iIndex == integer and iIndex > 0 and iIndex <= (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").aTabs.count then
			(
				deleteItem cryMaxTools.toolBox.aTabs iIndex
				true
			)
		)
		else if classOf sName == string then
		(--if name supplied, find it and delete if found
			local iIndexToDelete = findItem (fnGetTabNames()) sName
			if iIndexToDelete != 0 then
			(
				deleteItem cryMaxTools.toolBox.aTabs iIndexToDelete
				true
			)
		)
		else
		(
			"Error! No valid name or index supplied."
		)
	)
	
	fn fnChangeTab sName sIcon: sIconMask: aRollouts: sRolloutToAdd: sRolloutToRemove: =
	(--changes an existing tab in the list - returns false if tab not existent
		if bPrintFunctionNames then (print "fnChangeTab")
		
	)
	
	fn fnUpdateTabsInToolbox =
	(--updates all tab-buttons in the toolbox including rollouts - updates tab-scrolling button states
		if bPrintFunctionNames then (print "fnUpdateTabsInToolbox")
		
		for i = 1 to cryMaxTools.toolBox.aTabButtons.count do
		(--go through all tab-buttons and apply data
			local tCurrentTab = aTabs[i + cryMaxTools.toolBox.iTabBarScrollOffset]
			
			if tCurrentTab != undefined then
			(--if tab available for button - apply data
				cryMaxTools.toolBox.aTabButtons[i].enabled = true
				cryMaxTools.toolBox.aTabButtons[i].checked = false
				cryMaxTools.toolBox.aTabButtons[i].pos = [i*32 - 16,16]
				cryMaxTools.toolBox.aTabButtons[i].caption = tCurrentTab.sName
				cryMaxTools.toolBox.aTabButtons[i].toolTip = tCurrentTab.sName
				
				if tCurrentTab.bmpIcon != undefined and tCurrentTab.bmpIconMask != undefined then
				(
					cryMaxTools.toolBox.TabButtons[i].images = #(tCurrentTab.bmpIcon,tCurrentTab.bmpIconMask,1,1,1,1,1)
				)
			)
			else
			(--if no tab avialable for button, hide it
				cryMaxTools.toolBox.aTabButtons[i].enabled = false
				cryMaxTools.toolBox.aTabButtons[i].checked = false
				cryMaxTools.toolBox.aTabButtons[i].pos = [-32,-32]
				cryMaxTools.toolBox.aTabButtons[i].caption = ""
				cryMaxTools.toolBox.aTabButtons[i].toolTip = ""
				cryMaxTools.toolBox.aTabButtons[i].images = undefined
			)
			
			if i == (cryMaxTools.toolBox.iCurrentTab - cryMaxTools.toolBox.iTabBarScrollOffset) then
			(--set button of current tab to checked
				cryMaxTools.toolBox.aTabButtons[i].checked = true
			)
			else
			(--set all other buttons to false
				cryMaxTools.toolBox.aTabButtons[i].checked = false
			)
		)
		
		if cryMaxTools.toolBox.iTabBarScrollOffset < 1 then
		(--if impossible to scroll up/left then disable button - else enable
			cryMaxTools.toolBox.btnTabScrollUp.enabled = false
		)
		else
		(
			cryMaxTools.toolBox.btnTabScrollUp.enabled = true
		)
		
		if (cryMaxTools.toolBox.aTabs.count - 5) < cryMaxTools.toolBox.iTabBarScrollOffset then
		(--if impossible to scroll down/right then disable button - else enable
			cryMaxTools.toolBox.btnTabScrollDown.enabled = false
		)
		else
		(
			cryMaxTools.toolBox.btnTabScrollDown.enabled = true
		)
	)
	
	fn fnScrollTabBar iDirection: =
	(--modifies the tab barr offset so that hidden tabs can become visible - if direction unsupplied, will jump to zero offset
		if bPrintFunctionNames then (print "fnScrollTabBar")
		if bPrintDebugInfo then (print ("fnScrollTabBar in: " + iDirection as string))
		
		case iDirection of
		(
			"right":
			(--scroll right 5 positions
				if (cryMaxTools.toolBox.aTabs.count - 5) > cryMaxTools.toolBox.iTabBarScrollOffset then
				(
					cryMaxTools.toolBox.iTabBarScrollOffset += 5
				)
			)
			"left":
			(--scroll left 5 positions
				if cryMaxTools.toolBox.iTabBarScrollOffset > 0 then
				(
					cryMaxTools.toolBox.iTabBarScrollOffset -= 5
				)
			)
			unsupplied: cryMaxTools.toolBox.iTabBarScrollOffset = 0
		)
		
		fnUpdateTabsInToolbox()
	)
--### TAB FUNCTIONS END ###
	
	
--### ROLLOUT FUNCTIONS BEGIN ###
	fn fnGetRolloutNames =
	(--returns an array of all the rollout names, sorted like the global aRollouts array
		if bPrintFunctionNames then (print "fnGetRolloutNames")
		
		local aResult = for each in cryMaxTools.toolBox.aRollouts collect each.sName
		
		if bPrintDebugInfo then (print ("fnGetRolloutNames out: " + aResult as string))
		aResult
	)
	
	fn fnGetRolloutByName sName =
	(--returns the cryMaxToolboxRollout instance with the given name or undefined
		if bPrintFunctionNames then (print "fnGetRolloutByName")
		if bPrintDebugInfo then (print ("fnGetRolloutByName in: " + sName as string))
		
		local sResult = undefined
		local aRolloutNames = fnGetRolloutNames()
		local iIndex = findItem aRolloutNames sName
		
		if iIndex != 0 then
		(--if name has been found, return rollout
			sResult = cryMaxTools.toolBox.aRollouts[iIndex]
		)
		
		if bPrintDebugInfo then (print ("fnGetRolloutByName out: " + sResult as string))
		sResult
	)
	
	fn fnSetRolloutsInToolbox aRollouts =
	(--removes all rollouts from the toolbox and adds the specified ones - assumes array of valid cryMaxToolboxRollout instances or rollout names or rollout references
		if bPrintFunctionNames then (print "fnSetRolloutsInToolbox")
		if bPrintDebugInfo then (print ("fnSetRolloutsInToolbox in: " + aRolloutNames as string))
		
		for i=1 to cryMaxTools.toolBox.rltToolHolder.rollouts.count do
		(--remove all rollouts
			removeSubRollout cryMaxTools.toolBox.rltToolHolder cryMaxTools.toolBox.rltToolHolder.rollouts[1]
		)
		
		for i=1 to aRollouts.count do
		(--set rollout references of supllied rollouts
			case of
			(
				(classOf aRollouts[i] == string):
				(--rollout name supplied, get rollout reference and add
					local sRollout = fnGetRolloutByName aRollouts[i]
					addSubRollout cryMaxTools.toolBox.rltToolHolder sRollout.rltRollout
				)
				(isKindOf aRollouts[i] cryMaxTools.toolBox.cryMaxToolboxRollout):
				(--cryMaxToolboxRollout instance supplied, get rollout reference and add
					addSubRollout cryMaxTools.toolBox.rltToolHolder aRollouts[i].rltRollout
				)
				(isKindOf aRollouts[i] RolloutClass):
				(--rollout reference supplied - add it
					addSubRollout cryMaxTools.toolBox.rltToolHolder aRollouts[i]
				)
			)
		)
	)
	
	fn fnSetRolloutsInToolboxByButton iIndex =
	(--removes all rollouts from the toolbox and adds those assigned to the current tab of the button - takes scroll offset into consideration
		if bPrintFunctionNames then (print "fnSetRolloutsInToolboxByButton")
		if bPrintDebugInfo then (print ("fnSetRolloutsInToolboxByButton in: " + iIndex as string))
		
		if iIndex > 0 then
		(--if index points to a button, load rollouts
			local iTabIndex = iIndex + cryMaxTools.toolBox.iTabBarScrollOffset
			local aRolloutsFromTab = aTabs[iTabIndex].aRollouts
			
			fnSetRolloutsInToolbox aRolloutsFromTab
			
			cryMaxTools.toolBox.iCurrentTab = iTabIndex
			
			fnUpdateTabsInToolbox()
		)
		else
		(--if index is 0, clear rollouts
			fnSetRolloutsInToolbox #()
			
			cryMaxTools.toolBox.iCurrentTab = 0
			
			fnUpdateTabsInToolbox()
		)
	)
	
	fn fnAssignRolloutToTab sRollout sTab =
	(--assigns a rollout to a tab - assumes name strings or instance of cryMaxToolboxRollout and cryMaxToolboxTab
		if bPrintFunctionNames then (print "fnAssignRolloutToTab")
		if bPrintDebugInfo then (print ("fnAssignRolloutToTab in: " + sRollout as string + " " + sTab as string))
		
		if classOf sRollout == string then
		(--rollout name has been passed, get rollout
			sRollout = fnGetRolloutByName sRollout
		)
		
		if classOf sTab == string then
		(--tab name has been passed, get tab
			sTab = fnGetTabByName sTab
		)
		
		if sRollout != undefined and sTab != undefined then
		(--if tab and rollout are valida after find by name, assign if not yet assigned
			appendIfUnique sTab.aRollouts sRollout
		)
	)
	
	fn fnInitializeRolloutAssignments =
	(--assigns all loaded rollouts to tabs - removes missing rollouts - assignes unassigned rollouts to 'Unassigned' tab - loads rollout files
		if bPrintFunctionNames then (print "fnInitializeRolloutAssignments")
		
		local aRolloutsUnassigned = fnGetRolloutNames() --contains rollouts that have not yet been assigned
		
		for i = 1 to cryMaxTools.toolBox.aTabs.count do
		(--go through tabs and delete non-existing desired rollouts - mark existing rollouts as assigned
			local j = 1
			while j <= cryMaxTools.toolBox.aTabs[i].aRollouts.count do
			(--go through tab's desired rollouts and delete those that don't exist
				local sRollout = fnGetRolloutByName cryMaxTools.toolBox.aTabs[i].aRollouts[j]
				
				if sRollout == undefined then
				(
					deleteItem cryMaxTools.toolBox.aTabs[i].aRollouts j
				)
				else
				(
					local iIndexUnprocessed = findItem aRolloutsUnassigned sRollout.sName
					if iIndexUnprocessed != 0 then
					(--delete assigned rollout from unassigned list if still in there
						deleteItem aRolloutsUnassigned iIndexUnprocessed
					)
					
					j += 1
				)
			)
		)
		
		local sUnassignedTab = fnGetTabByName "Unassigned"
		while aRolloutsUnassigned.count != 0 do
		(--go through unassigned list and assign them to their preferred tab if available - else assign to unassigned tab
			local sRollout = fnGetRolloutByName aRolloutsUnassigned[1]
			
			if sRollout.sPreferredTab != undefined then
			(--find preferred tab and assign if avaialbe
				local sTab = fnGetTabByName sRollout.sPreferredTab
				if sTab != undefined then
				(--if tab exists, assign rollout and remove from unassigned list
					fnAssignRolloutToTab sRollout sTab
				)
				else
				(--else put to unassigned tab
					fnAssignRolloutToTab sRollout sUnassignedTab
				)
			)
			else
			(--else put to unassigned tab
				fnAssignRolloutToTab sRollout sUnassignedTab
			)
			
			deleteItem aRolloutsUnassigned 1
		)
	)
	
	fn fnLoadRolloutFromFile sFilePath =
	(--loads a rollout from a specified file, if valid and returns it - else undefined
		if bPrintFunctionNames then (print "fnLoadRolloutFromFile")
		if bPrintDebugInfo then (print ("fnLoadRolloutFromFile in: " + sFilePath as string))
		
		if doesFileExist sFilePath then
		(--check if file is present
			local sFileStream = openFile sFilePath --open script as filestream
			local sNewCryMaxToolboxRollout = undefined --cryMaxToolboxRollout instance as defined in the file
			local sRolloutName = undefined --name of the rollout in the file - will be deleted after processing
			
			if (skipToString sFileStream "--### ROLLOUT CONTAINER DEFINITION BELOW ###\n") != undefined then
			(--rollout container definition has been found
				sNewCryMaxToolboxRollout = readLine sFileStream
				
				if (skipToString sFileStream "--### ROLLOUT DEFINITION BEGIN ###\n") != undefined then
				(--rollout definition has been found - store rollout name
					local iRolloutDefinitionPosition = filePos sFileStream --store position in file where rollout is defined
					seek sFileStream (iRolloutDefinitionPosition + 8) --find position where global rolllout variable is declared
					sRolloutName = readDelimitedString sFileStream " " --read global rollout variable name
					
					seek sFileStream iRolloutDefinitionPosition --return to rollout definition begin
					
					if bPrintDebugInfo then
					(--if debugmode is on, do not catch errors
						execute sFileStream --create rollout
						sNewCryMaxToolboxRollout = execute sNewCryMaxToolboxRollout --read cryMaxToolBoxRollout instance from file
						sNewCryMaxToolboxRollout.rltRollout = execute sRolloutName --put rollout into cryMaxToolBoxRollout instance
						
						execute (sRolloutName + " = undefined") --clear global rollout variable
						
						sNewCryMaxToolboxRollout --return cryMaxToolBoxRollout instance
					)
					else
					(
						try
						(--try executing the rollout definition, then store the rollout in the cryMaxToolBoxRollout instance and clear global rollout variable
							execute sFileStream --create rollout
							sNewCryMaxToolboxRollout = execute sNewCryMaxToolboxRollout --read cryMaxToolBoxRollout instance from file
							sNewCryMaxToolboxRollout.rltRollout = execute sRolloutName --put rollout into cryMaxToolBoxRollout instance
							
							execute (sRolloutName + " = undefined") --clear global rollout variable
							
							sNewCryMaxToolboxRollout --return cryMaxToolBoxRollout instance
						)
						catch()
					)
				)
				else
				(
					if bPrintDebugInfo then (print "fnLoadRolloutFromFile: could not find \"--### ROLLOUT DEFINITION BEGIN ###\" in file.")
					undefined
				)
			)
			else
			(
				if bPrintDebugInfo then (print "fnLoadRolloutFromFile: could not find \"--### ROLLOUT CONTAINER DEFINITION BELOW ###\" in file.")
				undefined
			)
		)
		else
		(
			if bPrintDebugInfo then (print "fnLoadRolloutFromFile: could not find file.")
			undefined
		)
	)
	
	fn fnLoadRollouts =
	(--loads all predefined rollouts and all valid rollouts in the plug-in folder - very strict - for plug-in coding rules, please refer to example file: (to be created)
		if bPrintFunctionNames then (print "fnLoadRollouts")
		
		if cryMaxTools.toolBox.aRollouts[1].sName == "" then
		(--if name of first rollout is undefined, rollouts have not yet been loaded, proceed
			for i = 1 to cryMaxTools.toolBox.aRollouts.count do
			(--loop through predefined rollouts and load them
				cryMaxTools.toolBox.aRollouts[i] = fnLoadRolloutFromFile cryMaxTools.toolBox.aRollouts[i].sFilepath
			)
		)
	)
--### ROLLOUT FUNCTIONS END ###
	
--########################################################################################
--TOOLBOX UI EVENT HANDLERS
--########################################################################################
	
--### TOOLBOX OPEN/CLOSE BEGIN ###
	on rltCryMaxToolBox open do
	(
		local aToolboxSettings = cryMaxTools.basic.settingsMan.load "toolboxSettings" --load toolbox settings from INI -- old code using crappy save/load function - please clean up!
		
		if aToolboxSettings != false then
		(--if settings present, load them
			local p2TempPos = filterString aToolboxSettings[1].value[1] "[,]"
			p2TempPos = point2 (p2TempPos[1] as float) (p2TempPos[2] as float)
			local fTempHeight = aToolboxSettings[2].value[1] as float
			
			try(SetDialogPos cryMaxTools.toolBox p2TempPos)
			catch(print "Couldn't load Toolbox position")
			try(cryMaxTools.toolBox.height = fTempHeight)
			catch(print "Couldn't load Toolbox size")
			
			local sTempDockState = aToolboxSettings[3].value[1]
			if sTempDockState == "cui_dock_left" then
			(
				cui.RegisterDialogBar cryMaxTools.toolBox style:#()
				cui.DockDialogBar cryMaxTools.toolBox #cui_dock_left
			)
			if sTempDockState == "cui_dock_right" then
			(
				cui.RegisterDialogBar cryMaxTools.toolBox style:#()
				cui.DockDialogBar cryMaxTools.toolBox #cui_dock_right
			)
		)
		
		fnLoadRollouts()
		fnInitializeRolloutAssignments()
		fnUpdateTabsInToolbox()
	)
	
	on rltCryMaxToolBox close do
	(
		local aToolboxSettings = #("toolboxSettings")
		append aToolboxSettings ("position*@" + (GetDialogPos cryMaxTools.toolBox) as string) --store toolbox position
		append aToolboxSettings ("height*@" + (GetDialogSize cryMaxTools.toolBox)[2] as string) --store toolbox size
		
		try(sTempDockState = cui.getDockState cryMaxTools.toolBox)catch(sTempDockState = "")
		append aToolboxSettings ("dockState*@" + sTempDockState) --store dockstate
		
		cryMaxTools.basic.settingsMan.save aToolboxSettings
	)
--### TOOLBOX OPEN/CLOSE END ###
	
	
--### TOOLBOX DOCKING/RESIZING BEGIN ###
	on btnDockLeft pressed do
	(
		try
		(
			local sDockState = cui.getDockState cryMaxTools.toolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState == undefined then
		(--register if not yet done
			cui.RegisterDialogBar cryMaxTools.toolBox style:#() maxsize:[190,1200]
		)
		cui.DockDialogBar cryMaxTools.toolBox #cui_dock_left
		
		cryMaxTools.toolBox.rltToolHolder.height = cryMaxTools.toolBox.height - cryMaxTools.toolBox.rltToolHolder.pos[2]
	)
	
	on btnDockRight pressed do
	(
		try
		(
			local sDockState = cui.getDockState cryMaxTools.toolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState == undefined then
		(--register if not yet done
			cui.RegisterDialogBar cryMaxTools.toolBox style:#() maxsize:[190,1200]
		)
		cui.DockDialogBar cryMaxTools.toolBox #cui_dock_right
		
		cryMaxTools.toolBox.rltToolHolder.height = rltCryMaxToolBox.height - cryMaxTools.toolBox.rltToolHolder.pos[2]
	)
	
	on btnUndock pressed do
	(
		try
		(
			local sDockState = cui.getDockState cryMaxTools.toolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState != undefined then
		(--float and undock if registerd
			cui.FloatDialogBar cryMaxTools.toolBox
			cui.UnRegisterDialogBar cryMaxTools.toolBox
		)
		
		cryMaxTools.toolBox.rltToolHolder.height = cryMaxTools.toolBox.height - cryMaxTools.toolBox.rltToolHolder.pos[2]
	)
	
	on rltCryMaxToolBox resized p2Size do
	(
		cryMaxTools.toolBox.rltToolHolder.height = p2Size[2] - cryMaxTools.toolBox.rltToolHolder.pos[2] --keep rollout holder as big as possible
	)
--### TOOLBOX DOCKING/RESIZING END ###
	
	
--### BUTTONS BEGIN ###
	on btnTabScrollUp pressed do
	(--scroll left/up through tabs
		fnScrollTabBar iDirection:"left"
	)
	
	on btnTabScrollDown pressed do
	(--scroll right/down through tabs
		fnScrollTabBar iDirection:"right"
	)
	
	on ckbtnTab1 changed state do
	(--load assigned rollouts
		if state then
		(--if set to true, load rollouts
			fnSetRolloutsInToolboxByButton 1
		)
		else
		(--if set to false, clear toolbox
			fnSetRolloutsInToolboxByButton 0
		)
	)
	
	on ckbtnTab2 changed state do
	(--load assigned rollouts
		if state then
		(--if set to true, load rollouts
			fnSetRolloutsInToolboxByButton 2
		)
		else
		(--if set to false, clear toolbox
			fnSetRolloutsInToolboxByButton 0
		)
	)
	
	on ckbtnTab3 changed state do
	(--load assigned rollouts
		if state then
		(--if set to true, load rollouts
			fnSetRolloutsInToolboxByButton 3
		)
		else
		(--if set to false, clear toolbox
			fnSetRolloutsInToolboxByButton 0
		)
	)
	
	on ckbtnTab4 changed state do
	(--load assigned rollouts
		if state then
		(--if set to true, load rollouts
			fnSetRolloutsInToolboxByButton 4
		)
		else
		(--if set to false, clear toolbox
			fnSetRolloutsInToolboxByButton 0
		)
	)
	
	on ckbtnTab5 changed state do
	(--load assigned rollouts
		if state then
		(--if set to true, load rollouts
			fnSetRolloutsInToolboxByButton 5
		)
		else
		(--if set to false, clear toolbox
			fnSetRolloutsInToolboxByButton 0
		)
	)
--### BUTTONS END ###
)

cryMaxTools.toolBox = rltCryMaxToolBox
createDialog cryMaxTools.toolBox height:512 width:192 style:#(#style_titlebar, #style_resizing, #style_sysmenu) lockWidth:true