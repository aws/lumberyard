try destroyDialog setupRO catch()
		
rollout setupRO "Manage Setups"
(
	local characterArray = #()
	
	local lastCharacter = 0
	local lastSetup = 0
	local lastSourceNode = 0
	local lastGameBone = 0
	
	local onUpdate = false
	
	local tabArray = #()
	
	local defaultTabColor = undefined
	
	local conditionRC = undefined
	
	dotNetControl tabModes "System.Windows.Forms.TabControl" pos:[290,22] height:18 width:250
	groupbox gpTabModes "" pos:[285+144-142,5] height:(36) width:260
	
		--// MODEL
	label labSetupModelPath "Max :  None" pos:[295,50] width:250 visible:false enabled:false
	edittext edSetupModelPath "" pos:[290,48+20] fieldWidth:185 height:17 visible:false enabled:false
	button btnSetupModelPathBrowse "Browse" pos:[485,46+21] height:18 width:45 visible:false enabled:false
	checkbox chkSetupModelAutoSync "Auto-Sync from Perforce" pos:[295,75+20] checked:true visible:false enabled:false
	checkbox chkSetupModelRemoveOld "Remove old Setups" pos:[295,95+17] checked:true visible:false enabled:false
	radiobuttons radSetupModelPreferLoad "" pos:[295+150,75+20] visible:false enabled:false labels:#("Prefer Load") default:0
	radiobuttons radSetupModelPreferMerge "" pos:[295+150,95+17] visible:false enabled:false labels:#("Prefer Merge") default:1
	
		--// BIPED
	label labSetupBipedPosePath "Pose :  None" pos:[295,50] visible:false
	edittext edSetupBipedPosePath "" pos:[290,48+20] fieldWidth:185 height:17 visible:false
	button btnSetupBipedPosePathBrowse "Browse" pos:[485,46+21] height:18 width:45 visible:false
	checkbox chkSetupBipedAutoSync "Auto-Sync Pose file from Perforce" pos:[295,75+20] checked:true visible:false
	checkbox chkSetupBipedRemoveOld "Remove old Pose Collections" pos:[295,95+17] checked:false visible:false
	
		--// ALIGN
	button btnSetupAlignSourceNodeSet "Source Node :  None" pos:[295,48] height:18 width:236
	button btnSetupAlignTargetNodeSet "Target Node :  None" pos:[295,48+20] height:18 width:236
	
	label labSetupAlignRotation "Rot :" pos:[295,50+45]
	edittext edSetupAlignRotation "" pos:[290+30,48+45] fieldWidth:170 height:17
	button btnSetupAlignRotPos "Set" pos:[295+205,48+45] height:38 width:30
	label labSetupAlignPosition "Pos :" pos:[295,50+45+20]
	edittext edSetupAlignPosition "" pos:[290+30,48+45+20] fieldWidth:170 height:17
	
	
		--// MISC
	label labSetupMiscSkipFirstFrame "Skip First :" pos:[295,50]
	spinner spnSetupMiscSkipFirstFrame "" pos:[380-35,50] fieldWidth:50 type:#Integer range:[0,9999999,0] visible:false
	label labSetupMiscSkipLastFrame "Skip Last :" pos:[295-25+150,50]
	spinner spnSetupMiscSkipLastFrame "" pos:[380+90,50] fieldWidth:50 type:#Integer range:[0,9999999,0] visible:false
	
	checkbox chkSetupMiscCustomRig "Crytek Custom Rig" pos:[295,90-18] checked:false visible:false
	edittext edSetupMappingPath "XAF Mapping File" pos:[295,90] fieldWidth:130 height:17 visible:false enabled:false
	button btnSetupMappingPathBrowse "..." pos:[515,90] height:18 width:18 visible:false enabled:false
	
	edittext edSetupXAFPosePath "XAF Pose File" pos:[295,90+20] fieldWidth:148 height:17 visible:false enabled:false
	button btnSetupXAFPosePathBrowse "..." pos:[515,90+20] height:18 width:18 visible:false enabled:false
	
	groupbox gpSetupOptions " Setup Options " pos:[285+144-142,5] height:(300-85-58+3-19) width:260
	
	
	
	groupbox gpCondition " Game Bone Export Condition " pos:[285+144-142,133] height:(300-85-58-3+21) width:260
	dotNetControl edCondition "System.Windows.Forms.TextBox" pos:[293+144-142,150] height:(122) width:236 visible:false
	dotNetControl lvCondition "System.Windows.Forms.ListView" pos:[293+144-142,150] height:(130) width:236 visible:true
	
	button btnConditionCheck "Check" pos:[293+144-142,130+147+4] height:18 width:70 visible:false
	button btnConditionUnique "Unique?" pos:[293+144-142+80,130+147+4] height:18 width:70 visible:false
	button btnConditionClear "Clear" pos:[293+144-142+187,130+147+4] height:18 width:50 visible:false
	
	label labPropertyValue "Value:" pos:[293,272+16]
	
	edittext edPropertyValue "" pos:[293+35,272+15] width:(150+53)
	dropdownlist ddPropertyValue "" pos:[293+38,270+15] width:(149+53) visible:false
	spinner spnPropertyValueInteger "" pos:[293+38,272+15] width:(148+53) visible:false type:#Integer range:[-64000, 64000, 0]
	spinner spnPropertyValueFloat "" pos:[293+38,272+15] width:(148+53) visible:false type:#float range:[-64000.0, 64000.0, 0.0]
	
	
	
	groupbox gbCharacter " Character " pos:[-1,5] height:(300-85-60-20) width:145
	dotNetControl lvCharacter "System.Windows.Forms.ListView" pos:[5,22] width:130 height:(240-50-60-20)
	
	groupbox gpSourceNodes " Source Nodes " pos:[-1,5+148-20] height:(300-85-60+20) width:145
	dotNetControl lvSourceNodes "System.Windows.Forms.ListView" pos:[5,22+148-20] width:130 height:(240-50-60+20)
	
	
	
	groupbox gbSetup " Setup " pos:[285-142,5] height:(300-85-60-20) width:145
	dotNetControl lvSetup "System.Windows.Forms.ListView" pos:[293-142,22] width:130 height:(240-50-60-20)
	
	groupbox gpGameBones " Game Bones " pos:[285-142,133] height:(300-85-60+20) width:145
	dotNetControl lvGameBones "System.Windows.Forms.ListView" pos:[293-142,22+148-20] width:130 height:(240-50-60+20)
	
	
	groupbox gpLoadSave "" pos:[-5,301] height:60 width:149
	button btnOpen "Open" pos:[5,312] height:18 width:64
	button btnSaveAs "Save As" pos:[71,312] height:18 width:64
	button btnSave "Save" pos:[5,332] height:18 width:130
	
	
	label labSourcePath "Source Path :" pos:[215-210+145,10+305] width:400 height:18
	edittext edSourcePath "" pos:[220+58-210+145,8+305] width:(320-55)
	button btnSourcePath "Browse" pos:[210+400-4-210-50-3+140,8+305] height:18 width:50
	
	label labGamePath "Game Path :" pos:[215-210+145,10+23+305-3] width:400 height:18
	edittext edGamePath "" pos:[220+58-210+145,8+23+305-3] width:(320-55)
	button btnGamePath "Browse" pos:[210+400-4-210-50-3+140,8+23+305-3] height:18 width:50
	
	
	
	
	
	
	
	
	
	function updateTab tabArray index =
	(
		for i = 1 to tabArray.count do
		(
			for d = 1 to tabArray[i].count do
				tabArray[i].visible = (i == index)
		)
	)
	
	
	
	
	function getCutPath str =
	(
		local newStr = ""
		
		if str != "" then
		(
			if (getTextExtent str)[1] > 200 then
			(
				local tempFilter = filterString str "\\"
				local filename = tempFilter[tempFilter.count]
				local cutPath = tempFilter[1]
				for i = 2 to (tempFilter.count - 1) do
				(
					if (getTextExtent (cutPath + " \\ .. \\ " + tempFilter[i] + filename))[1] < 200 then
						cutPath += "\\" + tempFilter[i]
					else
					(
						newStr = cutPath + " \\ .. \\ " + filename
						exit
					)
				)
			)
		)
		
		--gc light:true
		
		return newStr
	)
	
	
	
	
	function updateSetupOptions =
	(
		tabModes.enabled = (lastCharacter > 0 and lastSetup > 0)
		
		for i = 1 to tabArray.count do
		(
			for d = 1 to tabArray[i].count do
				tabArray[i][d].enabled = (lastCharacter > 0 and lastSetup > 0)
		)
		
		
		
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local setup = characterArray[lastCharacter].setup[lastSetup]
			
			local maxPath = getCutPath setup.model.path
			if maxPath == "" then
				maxPath = "None"
			
			local posePath = getCutPath setup.biped.path
			if posePath == "" then
				posePath = "None"
			
			
			
			labSetupModelPath.text = "Max :  " + maxPath
			edSetupModelPath.text = setup.model.path
			chkSetupModelAutoSync.checked = setup.model.autoSync
			chkSetupModelRemoveOld.checked = setup.model.removeOld
			radSetupModelPreferLoad.state = (if setup.model.preferLoad == true then 1 else 0)
			radSetupModelPreferMerge.state = (if setup.model.preferLoad == true then 0 else 1)
			
			labSetupBipedPosePath.text = "Pose :  " + posePath
			edSetupBipedPosePath.text = setup.biped.path
			chkSetupBipedAutoSync.checked = setup.biped.autoSync
			chkSetupBipedRemoveOld.checked = setup.biped.removeOld
			
			btnSetupAlignSourceNodeSet.text = "Source Node :  " + (if setup.align.sourceNode != "" then setup.align.sourceNode else "None")
			btnSetupAlignTargetNodeSet.text = "Target Node :  " + (if setup.align.targetNode != "" then setup.align.targetNode else "None")
			
			edSetupAlignRotation.text = setup.align.rot as String
			edSetupAlignPosition.text = setup.align.pos as String
			
			spnSetupMiscSkipFirstFrame.value = setup.misc.skipFirst
			spnSetupMiscSkipLastFrame.value = setup.misc.skipLast
			
			chkSetupMiscCustomRig.checked = setup.misc.customRig
			edSetupMappingPath.text = setup.misc.mappingFile
			edSetupXAFPosePath.text = setup.misc.poseFile
		)
		
		--gc light:true
	)
	
	
	
	
	

	
	function updatePaths =
	(
		edSourcePath.text = ""
		edGamePath.text = ""
		
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				edSourcePath.text = characterArray[lastCharacter].setup[lastSetup].sourcePath
				edGamePath.text = characterArray[lastCharacter].setup[lastSetup].gamePath
			)
		)
	)
	
	
	/*
	function updateCondition =
	(
		onUpdate = true
		
		edCondition.text = ""
		
		if lastCharacter > 0 and lastGameBone > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone] != undefined then
				(
					try edCondition.text = characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].condition catch()
				)
			)
		)
		
		onUpdate = false
	)
	*/
	
	
	function updateValues =
	(
		edPropertyValue.text = ""
		spnPropertyValueInteger.value = 0
		spnPropertyValueFloat.value = 0.0
		
		edPropertyValue.enabled = false
		ddPropertyValue.enabled = false
		spnPropertyValueInteger.enabled = false
		spnPropertyValueFloat.enabled = false
		
		
		if lvCondition.selectedItems.count > 0 then
		(
			local tempEntry = lvCondition.selectedItems.item 0
			
			if tempEntry != undefined then
			(
				local entryValue = (tempEntry.subItems.item 2).text
				local checkValue = (filterString tempEntry.text " ")[1]
				local classValue = (filterString checkValue "_")[1]
				
				edPropertyValue.enabled = true
				ddPropertyValue.enabled = true
				spnPropertyValueInteger.enabled = true
				spnPropertyValueFloat.enabled = true
				
				edPropertyValue.visible = false
				ddPropertyValue.visible = false
				spnPropertyValueInteger.visible = false
				spnPropertyValueFloat.visible = false
				
				local usedClass = undefined
				
				
				
				usedClass = case classValue of
				(
					"string":	String
					"int":		Integer
					"float":	Float
					"bool":	BooleanClass
				)
				
				
				
				
				if usedClass != undefined then
				(	
					local usedElement = undefined
					
					case usedClass of
					(
						Integer:			( usedElement = spnPropertyValueInteger ; usedElement.value = (execute entryValue) )
						Float:			( usedElement = spnPropertyValueFloat ; usedElement.value = (execute entryValue) )
						BooleanClass:	( usedElement = ddPropertyValue ; usedElement.selection = (if (execute entryValue) == true then 1 else  2 ))
						String:			( usedElement = edPropertyValue ; usedElement.text = (execute entryValue) )
					)
					
					usedElement.visible = true
				)
			)
		)
		
		--gc light:true
		onUpdate = false
	)
	
	
	
	
	
	
	function updateConditions keepSelection:true skipValue:false =
	(
		local saveSel = #()
		
		if keepSelection == true then
			for i = 0 to (lvCondition.selectedItems.count - 1) do
				append saveSel ((lvCondition.selectedItems.item i).index)
		
		
		onUpdate = true
				
		lvCondition.items.clear()
		
		if lastCharacter > 0 and lastSetup > 0 and lastGameBone > 0 then
		(
			local conArray = characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions
			
			for i = 1 to conArray.count do
			(
				local addSep = ""
				if conArray[i].parentLayer > 0 then
				(
					for i = 1 to conArray[i].parentLayer do
						addSep += "  "
				)
					
				local newEntry = lvCondition.items.add (addSep + conArray[i].name) (addSep + conArray[i].name)
				newEntry.subItems.add ""
				newEntry.subItems.add ""
				
				newEntry.checked = conArray[i].active
				
				
					--// STATE \\--
				if conArray[i].state > 0 then
					(newEntry.subItems.item 1).text = cryAM.stateArray[conArray[i].state]
				
				(newEntry.subItems.item 2).text = conArray[i].value
				
					--// GROUP \\--
				if conArray[i].group > 0 then
				(
					case conArray[i].group of
					(
						1: newEntry.backColor = newEntry.backColor.fromARGB 230 230 255
						2: newEntry.backColor = newEntry.backColor.fromARGB 255 230 230
						3: newEntry.backColor = newEntry.backColor.fromARGB 230 255 230
						4: newEntry.backColor = newEntry.backColor.fromARGB 255 255 220
					)
				)
			)
		)
		
		if keepSelection == true then
			for i = 1 to saveSel.count do
				try (lvCondition.items.item saveSel[i]).selected = true catch()
		
		if skipValue == false then
			updateValues()
	)
	
	
	
	
	
	
	function updateNodes mode =
	(
		onUpdate = true
		
		if lastCharacter > 0 and lastSetup > 0 then
		(
			case mode of
			(
				#sourceNodes:
				(
					local list = lvSourceNodes
					local listArray = characterArray[lastCharacter].setup[lastSetup].sourceNodes
				)
				#gameBones:
				(
					local list = lvGameBones
					local listArray = characterArray[lastCharacter].setup[lastSetup].gameBones
				)
			)
			
			if list != undefined then
			(
				list.items.clear()
				
				for i = 1 to listArray.count do
				(
					case mode of
					(
						#sourceNodes:	list.items.add listArray[i] listArray[i]
						#gameBones:		list.items.add listArray[i].name listArray[i].name
					)
				)
				
				
				
				
				if mode == #gameBones then
				(
					--if lastGameBone > 0 and lastGameBone <= lvGameBones.items.count then
						--(lvGameBones.items.item (lastGameBone - 1)).selected = true
					
					lastGameBone = 0
							
					updateConditions()
				)
				
			)
			
			
		)
		
		--gc light:true
	)
	
	
	
	
	
	
	function updateSetups =
	(
		onUpdate = true
		
		lvSetup.items.clear()
		
		if lastCharacter > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup.count < lastSetup then
					lastSetup = 1
				
				for i = 1 to characterArray[lastCharacter].setup.count do
				(
					local newEntry = lvSetup.items.add characterArray[lastCharacter].setup[i].name
					newEntry.checked = characterArray[lastCharacter].setup[i].active
				)
				
				if lastSetup > 0 and lastSetup <= lvSetup.items.count then
				(
					(lvSetup.items.item (lastSetup - 1)).selected = true
					if lastSetup > 2 then
						lvSetup.TopItem = (lvSetup.items.item (lastSetup - 3))
					else
						lvSetup.TopItem = (lvSetup.items.item (lastSetup - 1))
				)
			)
		)
		
		
		
		
		updatePaths()
		
		updateSetupOptions()
		
		updateNodes #sourceNodes
		updateNodes #gameBones
		
		
	)
	
	
	
	
	
	function updateCharacters =
	(
		onUpdate = true
		
		lvCharacter.items.clear()
		
		for i = 1 to characterArray.count do
		(
			local newEntry = lvCharacter.items.add characterArray[i].name
			newEntry.checked = characterArray[i].active
		)
		
		if lastCharacter > 0 and lastCharacter <= lvCharacter.items.count then
		(
			(lvCharacter.items.item (lastCharacter - 1)).selected = true
			if lastCharacter > 2 then
				lvCharacter.TopItem = (lvCharacter.items.item (lastCharacter - 3))
			else
				lvCharacter.TopItem = (lvCharacter.items.item (lastCharacter - 1))
		)
		
		
		onUpdate = false
		
		updateSetups()
	)
	
	
	
	
	
	
	
	function callCharacterMenu =
	(
		RCMenu characterRightMouseClickRC
		(
			menuItem miNew "New" enabled:true
			seperator sep1
			menuItem miLoad "Load" enabled:true
			seperator sep2
			menuItem miCopy "Copy" enabled:true
			menuItem miPaste "Paste" enabled:true
			seperator sep3
			menuItem miDelete "Delete" enabled:true
			
			
			function newEntry =
			(
				local newName = "newCharacter01"
				local newNameMaxCount = 0
				
				
				for i = 1 to 50 do
				(
					local tempName = "newCharacter" + (if i < 10 then "0" else "") + i as String
					
					for f = 1 to characterArray.count do
						if (characterArray[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newCharacter" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				append characterArray (cryAM_characterStruct name:newName)
				
				lastCharacter = characterArray.count
				lastSetup = characterArray[lastCharacter].setup.count
				
				if lastCharacter > 2 then
					lvCharacter.TopItem = (lvCharacter.items.item (lastCharacter - 3))
				else
					lvCharacter.TopItem = (lvCharacter.items.item (lastCharacter - 1))
				
				onUpdate = true
				updateCharacters()
			)
			
			
			
			
			on characterRightMouseClickRC open do
			(
				if lvCharacter.items.count == 0 then
				(
					miLoad.enabled = false
					miDelete.enabled = false
					miCopy.enabled = false
					miPaste.enabled = false
				)
				else
				(
					miDelete.enabled = true
					miPaste.enabled = cryAM.copyCharacter.count > 0
				)
			)
			
			
			on miNew picked do
				newEntry()
			
			
			
			on miLoad picked do
			(
				if lastTask > 0 then
				(
					if taskArray[lastTask] != undefined then
					(
						for i = 1 to taskArray[lastTask].process.count do
						(
							if taskArray[lastTask].process[i].active == true then
							(
							
								local runScript = ""
								local scriptFound = ""
								
								runScript = taskArray[lastTask].process[i].script.text
								scriptFound = taskArray[lastTask].process[i]
										
								
								if scriptFound != undefined then
									scriptFound = "in " + scriptFound.name + ":  "
								
								try
								(
										--// tries executing the script
									execute( runScript )
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
			)
			
			
			
			
			
			
			on miCopy picked do
			(
				if lastCharacter > 0 then
				(
					for i = 0 to (lvCharacter.selectedItems.count - 1) do
					(
						local index = (lvCharacter.selectedItems.item i).index + 1
						if characterArray[index] != undefined then
							append cryAM.copyCharacter characterArray[index]
					)
				)
			)
			
			
			on miPaste picked do
			(
				for i = 1 to cryAM.copyCharacter.count do
				(
					newEntry()
					
					if cryAM.copyCharacter != undefined then
						if lastCharacter > 0 then
							if characterArray[lastCharacter] != undefined then
								characterArray[lastCharacter] = (cryMaxTools.basic.misc.makeUnique cryAM.copyCharacter[i])
							
					onUpdate = true
					updateCharacters()
				)
			)
			
			
			
			
			
			
			on miDelete picked do
			(
				if lvCharacter.selectedItems.count > 0 then
				(
					local newCharacterArray = #()
					local delArray = #{}
					
					for i = 0 to (lvCharacter.selectedItems.count - 1) do
						delArray[(lvCharacter.selectedItems.item i).index + 1] = true
					
					for i = 1 to characterArray.count do
					(
						if delArray[i] == false then
							append newCharacterArray characterArray[i]
					)
						
					characterArray = newCharacterArray
				)
				
				if characterArray.count == 0 then
					lastCharacter = 0
				else
					lastCharacter = 1
		
				onUpdate = true
				updateCharacters()
			)
		)
		
		registerRightClickMenu characterRightMouseClickRC
		popUpMenu characterRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
	function callSetupMenuRC =
	(
		RCMenu setupRightMouseClickRC
		(
			menuItem miNew "New" enabled:true
			seperator sep1
			menuItem miRun "Run" enabled:true
			seperator sep2
			menuItem miCopy "Copy" enabled:true
			menuItem miPaste "Paste" enabled:true
			seperator sep3
			menuItem miDelete "Delete" enabled:true
			
			
			
			function newEntry =
			(
				local newName = "newSetup01"
				local newNameMaxCount = 0
				
				
				for i = 1 to 50 do
				(
					local tempName = "newSetup" + (if i < 10 then "0" else "") + i as String
					
					for f = 1 to characterArray[lastCharacter].setup.count do
						if (characterArray[lastCharacter].setup[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newSetup" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				append characterArray[lastCharacter].setup (cryAM_setupStruct name:newName)
				
				
				lastSetup = characterArray[lastCharacter].setup.count
				
				if lvSetup.items.count > 0 then
				(
					if lastSetup > 2 then
						lvSetup.TopItem = (lvSetup.items.item (lastSetup - 3))
					else if lastSetup > 1 then
						lvSetup.TopItem = (lvSetup.items.item (lastSetup - 2))
					else if lastSetup > 0 then
						lvSetup.TopItem = (lvSetup.items.item (lastSetup - 1))
				)
				
				
				onUpdate = true
				updateSetups()
				
				edCondition.text = ""
			)
			
			
			
			
			
			on setupRightMouseClickRC open do
			(
				if lastCharacter == 0 then
					miNew.enabled = false
				
				if lvSetup.items.count == 0 then
				(
					miCopy.enabled = false
					miDelete.enabled = false
				)
					
				miPaste.enabled = cryAM.copySetup.count > 0
			)
			
			
			
			
			on miNew picked do
				newEntry()
			
			
			
			on miRun picked do
			(
				local runScript = ""
				local scriptFound = ""
				
				if lastTask > 0 and lastProcess > 0 then
				(
					if taskArray[lastTask] != undefined then
					(
						if taskArray[lastTask].process[lastProcess] != undefined then
						(
							runScript = taskArray[lastTask].process[lastProcess].script.text
							scriptFound = taskArray[lastTask].process[lastProcess]
						)
					)
				)
						
				
				if scriptFound != undefined then
					scriptFound = "in " + scriptFound.name + ":  "
				
				try
				(
						--// tries executing the script
					execute( runScript )
				)
				catch
				(
						--// if an error occured, print the error message
					format "*** % ***\n" (scriptFound + (getCurrentException()))
				)
			)
			
			
			
			
			
			

			
			on miDelete picked do
			(
				if lvSetup.selectedItems.count > 0 then
				(
					local newSetupArray = #()
					local delArray = #{}
					
					for i = 0 to (lvSetup.selectedItems.count - 1) do
						delArray[(lvSetup.selectedItems.item i).index + 1] = true
					
					for i = 1 to characterArray[lastCharacter].setup.count do
					(
						if delArray[i] == false then
							append newSetupArray characterArray[lastCharacter].setup[i]
					)
						
					characterArray[lastCharacter].setup = newSetupArray
				)
				
				if characterArray[lastCharacter].setup.count == 0 then
					lastSetup = 0
				else
					lastSetup = 1
		
				onUpdate = true
				updateSetups()
			)
			
			
			on miCopy picked do
			(
				cryAM.copySetup = #()
				
				if lastCharacter > 0 then
				(
					for i = 0 to (lvSetup.selectedItems.count - 1) do
					(
						local index = (lvSetup.selectedItems.item i).index + 1
						if characterArray[lastCharacter] != undefined then
							if characterArray[lastCharacter].setup[index] != undefined then
								append cryAM.copySetup characterArray[lastCharacter].setup[index]
					)
				)
			)
			
			
			on miPaste picked do
			(
				for i = 1 to cryAM.copySetup.count do
				(
					newEntry()
					
					if cryAM.copySetup != undefined then
						if lastCharacter > 0 and lastSetup > 0 then
							if characterArray[lastCharacter] != undefined then
								if characterArray[lastCharacter].setup[lastSetup] != undefined then
									characterArray[lastCharacter].setup[lastSetup] = cryMaxTools.basic.misc.makeUnique cryAM.copySetup[i]
					
					onUpdate = true
					updateSetups()
				)
			)
		)
		
		registerRightClickMenu setupRightMouseClickRC
		popUpMenu setupRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function newCondition index =
	(
		if lastCharacter > 0 and lastSetup > 0 and lastGameBone > 0 then
		(
			local tempValue = undefined
			
			tempValue = case (filterString cryAM.conditionArray[index] "_")[1] of
			(
				"string": "\"\""
				"int":		"0"
				"float":	"0.0"
				"bool":	"false"
			)
			
			append characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions (cryAM_conditionStruct name:cryAM.conditionArray[index] variableID:index value:tempValue)
		)
		
		updateConditions()
	)
	
	
	
	
	
	function setProp op value =
	(
		if lastCharacter > 0 and lastSetup > 0 and lastGameBone > 0 then
		(
			for i = 0 to (lvCondition.selectedItems.count - 1) do
			(
				local index = (lvCondition.selectedItems.item i).index + 1
				
				local condition = characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions[index]
				if condition != undefined then
				(
					case op of
					(
						#condition:	condition.state = value
						#group:		condition.group = value
						#value:		condition.value = value
						#parent:
						(
							if value > 0 then
								condition.parentLayer += 1
							
							if value < 0 then
								condition.parentLayer -= 1
						)
					)
				)
			)
			
			updateConditions()
		)
	)
	
	
	
	
	
	

	
	
	
	
	
	
	function callBonesMenu =
	(
		RCMenu bonesRightMouseClickRC
		(
			menuItem miAdd "Add" enabled:true
			separator sep1
			menuItem miCopy "Copy" enabled:true
			menuItem miPaste "Paste" enabled:true
			seperator sep2
			menuItem miDelete "Delete" enabled:true
			seperator sep3
			menuItem miDeleteAll "Delete All" enabled:true
			
			
			on bonesRightMouseClickRC open do
			(
				if lastCharacter == 0 or lastSetup == 0 then
					miAdd.enabled = false
				
				local count = case cryAM.taskMode of
				(
					#sourceNodes: lvSourceNodes.selectedItems.count
					#gameBones: lvGameBones.selectedItems.count
				)
				
				if count == 0 then
				(
					miDelete.enabled = false
					miDeleteAll.enabled = false
				)
			)
			
			
			
			
			on miAdd picked do
			(
				if lastCharacter > 0 and lastSetup > 0 then
				(
					case cryAM.taskMode of
					(
						#sourceNodes:	local listArray = characterArray[lastCharacter].setup[lastSetup].sourceNodes
						#gameBones:		local listArray = characterArray[lastCharacter].setup[lastSetup].gameBones
					)
					
					if selection.count > 0 then
					(
						local addArray = selection as Array
					
						for i = 1 to addArray.count do
						(
							local found = false
							
							for f = 1 to listArray.count do
							(
								case cryAM.taskMode of
								(
									#sourceNodes:
									(
										if addArray[i].name == listArray[f] then
											found = true
									)
									#gameBones:
									(
										if addArray[i].name == listArray[f].name then
											found = true
									)
								)
							)
							if found == false then
							(
								case cryAM.taskMode of
								(
									#sourceNodes:	append listArray addArray[i].name
									#gameBones:		append listArray (cryAM_gameBoneStruct name:addArray[i].name)
								)
							)
						)
						
						qsort listArray cryAM.compareNodes
						
						if cryAM.taskMode == #gameBones then
							lastGameBone = characterArray[lastCharacter].setup[lastSetup].gameBones.count
						
						updateNodes cryAM.taskMode
					)
				)
			)
			
			
			
			
			
			on miCopy picked do
			(
				cryAM.copyGameBone = #()
				
				local boneArray = characterArray[lastCharacter].setup[lastSetup].gameBones
				
				for i = 0 to (lvGameBones.selectedItems.count - 1) do
				(
					local index = (lvGameBones.selectedItems.item i).index + 1
					
					if boneArray[index] != undefined then
						append cryAM.copyGameBone boneArray[index]
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				if lastSetup > 0 then
				(
					local boneArray = characterArray[lastCharacter].setup[lastSetup].gameBones
					
					for i = 1 to cryAM.copyGameBone.count do
						append boneArray (cryMaxTools.basic.misc.makeUnique cryAM.copyGameBone[i])
					
					onUpdate = true
					
					updateNodes #gameBones
				)
			)
			
			

			
			on miDelete picked do
			(
				if lastCharacter > 0 then
				(
					case cryAM.taskMode of
					(
						#sourceNodes:
						(
							local list = lvSourceNodes
							local listArray = characterArray[lastCharacter].setup[lastSetup].sourceNodes
						)
						#gameBones:
						(
							local list = lvGameBones
							local listArray = characterArray[lastCharacter].setup[lastSetup].gameBones
						)
					)
					
					
						
						
					if list.selectedItems.count > 0 then
					(	
						local newArray = #()
						local delArray = #{}
						
						for i = (list.selectedItems.count - 1) to 0 by -1 do
							deleteItem listArray ((list.selectedItems.item i).index + 1)
					)
					
					updateNodes cryAM.taskMode
				)
			)
			
			
			on miDeleteAll picked do
			(
				if lastCharacter > 0 then
				(
				
					case cryAM.taskMode of
					(
						#sourceNodes:
						(
							characterArray[lastCharacter].setup[lastSetup].sourceNodes = #()
						)
						#gameBones:
						(
							characterArray[lastCharacter].setup[lastSetup].gameBones = #()
						)
					)
					
					updateNodes cryAM.taskMode
				)
			)
			
		)
		
		registerRightClickMenu bonesRightMouseClickRC
		popUpMenu bonesRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
	
	function callConditionMenu =
	(
		RCMenu conditionRC
		(
			fn fnFilter1 = cryAM.conditionArray.count >= 1
			fn fnFilter2 = cryAM.conditionArray.count >= 2
			fn fnFilter3 = cryAM.conditionArray.count >= 3
			fn fnFilter4 = cryAM.conditionArray.count >= 4
			fn fnFilter5 = cryAM.conditionArray.count >= 5
			fn fnFilter6 = cryAM.conditionArray.count >= 6
			fn fnFilter7 = cryAM.conditionArray.count >= 7
			fn fnFilter8 = cryAM.conditionArray.count >= 8
			fn fnFilter9 = cryAM.conditionArray.count >= 9
			fn fnFilter10 = cryAM.conditionArray.count >= 10
			fn fnFilter11 = cryAM.conditionArray.count >= 11
			fn fnFilter12 = cryAM.conditionArray.count >= 12
			fn fnFilter13 = cryAM.conditionArray.count >= 13
			fn fnFilter14 = cryAM.conditionArray.count >= 14
			fn fnFilter15 = cryAM.conditionArray.count >= 15
			fn fnFilter16 = cryAM.conditionArray.count >= 16
			fn fnFilter17 = cryAM.conditionArray.count >= 17
			fn fnFilter18 = cryAM.conditionArray.count >= 18
			fn fnFilter19 = cryAM.conditionArray.count >= 19
			fn fnFilter20 = cryAM.conditionArray.count >= 20
			
			subMenu "Add"
			(
				menuItem miAdd1 "Add1" filter:fnFilter1
				menuItem miAdd2 "Add1" filter:fnFilter2
				menuItem miAdd3 "Add1" filter:fnFilter3
				menuItem miAdd4 "Add1" filter:fnFilter4
				menuItem miAdd5 "Add1" filter:fnFilter5
				menuItem miAdd6 "Add1" filter:fnFilter6
				menuItem miAdd7 "Add1" filter:fnFilter7
				menuItem miAdd8 "Add1" filter:fnFilter8
				menuItem miAdd9 "Add1" filter:fnFilter9
				menuItem miAdd10 "Add1" filter:fnFilter10
				menuItem miAdd11 "Add1" filter:fnFilter11
				menuItem miAdd12 "Add1" filter:fnFilter12
				menuItem miAdd13 "Add1" filter:fnFilter13
				menuItem miAdd14 "Add1" filter:fnFilter14
				menuItem miAdd15 "Add1" filter:fnFilter15
				menuItem miAdd16 "Add1" filter:fnFilter16
				menuItem miAdd17 "Add1" filter:fnFilter17
				menuItem miAdd18 "Add1" filter:fnFilter18
				menuItem miAdd19 "Add1" filter:fnFilter19
				menuItem miAdd20 "Add1" filter:fnFilter20
			)
			separator sep1
			subMenu "Condition"
			(
				menuItem miHas "Has" checked:false
				menuItem miHasCase "HAS" checked:false
				separator sep2
				menuItem miEqual "==" checked:false
				menuItem miUnequal "!=" checked:false
				separator sep3
				menuItem miLess "<" checked:false
				menuItem miMore ">" checked:false
				separator sep4
				menuItem miLessEqual "<=" checked:false
				menuItem miMoreEqual ">=" checked:false
			)
			subMenu "Group"
			(
				menuItem miAND "AND" checked:false
				menuItem miOR "OR" checked:false
				separator sep6
				menuItem miNAND "NOT AND" checked:false
				menuItem miNOR "NOT OR" checked:false
			)
			separator sep7
			menuItem miMoveIn "Move In" enabled:false
			menuItem miMoveOut "Move Out" enabled:false
			separator sep8
			menuItem miCopy "Copy" enabled:false
			menuItem miPaste "Paste" enabled:false
			separator sep9
			menuItem miDelete "Delete" enabled:false
			seperator sep10
			menuItem miDeleteAll "Delete All" enabled:false
			
			
			on conditionRC open do
			(
				if lvCondition.selectedItems.count > 0 then
				(
					miDelete.enabled = true
					miCopy.enabled = true
					
					miMoveIn.enabled = true
					miMoveOut.enabled = true
				)
				
				miPaste.enabled = cryAM.copyCondition.count > 0
					
				
				if lvCondition.items.count > 0 then
					miDeleteAll.enabled = true
				
				for i = 1 to cryAM.conditionArray.count do
					execute ("setupRO.conditionRC.miAdd" + i as String + ".text = \"" + cryAM.conditionArray[i] + "\"")
				
				if lastCharacter > 0 and lastSetup > 0 and lastGameBone > 0 and lvCondition.selectedItems.count > 0 then
				(
					local selIndex = (lvCondition.selectedItems.item 0).index + 1
					
						--// STATE \\--
					case characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions[selIndex].state of
					(
						1: miHas.checked = true
						2: miHasCase.checked = true
						3: miEqual.checked = true
						4: miUnequal.checked = true
						5: miLess.checked = true
						6: miMore.checked = true
						7: miLessEqual.checked = true
						8: miMoreEqual.checked = true
					)
					
						--// GROUP \\--
					case characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions[selIndex].group of
					(
						1: miAND.checked = true
						2: miOR.checked = true
						3: miNAND.checked = true
						4: miNOR.checked = true
					)
				)
			)
			
			
			
			
			on miAdd1 picked do	newCondition 1
			on miAdd2 picked do	newCondition 2
			on miAdd3 picked do	newCondition 3
			on miAdd4 picked do	newCondition 4
			on miAdd5 picked do	newCondition 5
			on miAdd6 picked do	newCondition 6
			on miAdd7 picked do	newCondition 7
			on miAdd8 picked do	newCondition 8
			on miAdd9 picked do	newCondition 9
			on miAdd10 picked do	newCondition 10
			on miAdd11 picked do	newCondition 11
			on miAdd12 picked do	newCondition 12
			on miAdd13 picked do	newCondition 13
			on miAdd14 picked do	newCondition 14
			on miAdd15 picked do	newCondition 15
			on miAdd16 picked do	newCondition 16
			on miAdd17 picked do	newCondition 17
			on miAdd18 picked do	newCondition 18
			on miAdd19 picked do	newCondition 19
			on miAdd20 picked do	newCondition 20
			
			
			
			on miHas picked do			setProp #condition 1
			on miHasCase picked do	setProp #condition 2
			on miEqual picked do		setProp #condition 3
			on miUnequal picked do	setProp #condition 4
			on miLess picked do			setProp #condition 5
			on miMore picked do		setProp #condition 6
			on miLessEqual picked do	setProp #condition 7
			on miMoreEqual picked do	setProp #condition 8
			
			on miAND picked do			setProp #group 1
			on miOR picked do			setProp #group 2
			on miNAND picked do		setProp #group 3
			on miNOR picked do			setProp #group 4
			
				
			on miMoveIn picked do		setProp #parent 1
			on miMoveOut picked do	setProp #parent -1
			
			
			
			
			on miCopy picked do
			(
				cryAM.copyCondition = #()
				
				local conArray = characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions
				
				for i = 0 to (lvCondition.selectedItems.count - 1) do
				(
					local index = (lvCondition.selectedItems.item i).index + 1
					
					if conArray[index] != undefined then
						append cryAM.copyCondition conArray[index]
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				if lastGameBone > 0 then
				(
					local conArray = characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions
					
					for i = 1 to cryAM.copyCondition.count do
						append conArray (cryMaxTools.basic.misc.makeUnique cryAM.copyCondition[i])
					
					onUpdate = true
					
					updateConditions()
				)
			)
			
			
			
			
			
			
			
			
			
			on miDelete picked do
			(
				if lvCondition.selectedItems.count > 0 then
				(
					for i = (lvCondition.selectedItems.count - 1) to 0 by -1 do
						deleteItem characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions ((lvCondition.selectedItems.item i).index + 1)
				)
				
				updateConditions()
			)
			
			
			on miDeleteAll picked do
			(
				if lastCharacter > 0 and lastSetup > 0 and lastGameBone > 0 then
				(
					characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions = #()
					
					updateConditions()
				)
			)
			
		)
		
		registerRightClickMenu conditionRC
		popUpMenu conditionRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
		
		
		
	)

		
	
	
	
	
	
	
	
	
	
	
	
	
	
	on setupRO open do
	(
		append tabArray (#(	labSetupModelPath, edSetupModelPath, btnSetupModelPathBrowse, chkSetupModelAutoSync , chkSetupModelRemoveOld, radSetupModelPreferLoad, radSetupModelPreferMerge ))
		append tabArray (#(	labSetupBipedPosePath, edSetupBipedPosePath, btnSetupBipedPosePathBrowse, chkSetupBipedAutoSync, chkSetupBipedRemoveOld ))
		append tabArray (#(	btnSetupAlignSourceNodeSet, btnSetupAlignTargetNodeSet, labSetupAlignRotation, edSetupAlignRotation, btnSetupAlignRotPos, labSetupAlignPosition, edSetupAlignPosition ))
		append tabArray (#(	spnSetupMiscSkipFirstFrame, spnSetupMiscSkipLastFrame, labSetupMiscSkipFirstFrame, labSetupMiscSkipLastFrame, chkSetupMiscCustomRig, edSetupMappingPath, btnSetupMappingPathBrowse, edSetupXAFPosePath, btnSetupXAFPosePathBrowse ))
		
		
		ddPropertyValue.items = #("true", "false")
		
		edCondition.Multiline = true
			
		tabModes.TabPages.add "  Model / Rig  "
		tabModes.TabPages.add "  Biped  "
		tabModes.TabPages.add "  Align  "
		tabModes.TabPages.add "  Misc  "
		
		
		lvCharacter.GridLines = true
		lvCharacter.Checkboxes = true
		lvCharacter.AllowColumnReorder = false
		lvCharacter.View = lvCharacter.View.Details
		lvCharacter.LabelEdit = true
		lvCharacter.LabelWrap = true
		lvCharacter.MultiSelect = true
		lvCharacter.FullRowSelect = true
		lvCharacter.HideSelection = false
		lvCharacter.HeaderStyle = lvCharacter.HeaderStyle.None
		lvCharacter.BackColor = lvCharacter.BackColor.FromARGB 230 230 255
		
		lvCharacter.Columns.Add ""
		(lvCharacter.Columns.item 0).width = lvCharacter.width - 4
		
		
		lvSetup.GridLines = true
		lvSetup.Checkboxes = true
		lvSetup.AllowColumnReorder = false
		lvSetup.View = lvSetup.View.Details
		lvSetup.LabelEdit = true
		lvSetup.LabelWrap = true
		lvSetup.MultiSelect = true
		lvSetup.FullRowSelect = true
		lvSetup.HideSelection = false
		lvSetup.HeaderStyle = lvSetup.HeaderStyle.None
		lvSetup.BackColor = lvSetup.BackColor.FromARGB 255 230 230
		
		lvSetup.Columns.Add ""
		(lvSetup.Columns.item 0).width = lvSetup.width - 4
		
		
		lvCondition.GridLines = true
		lvCondition.Checkboxes = true
		lvCondition.AllowColumnReorder = false
		lvCondition.View = lvCondition.View.Details
		lvCondition.LabelEdit = false
		lvCondition.LabelWrap = false
		lvCondition.MultiSelect = true
		lvCondition.FullRowSelect = true
		lvCondition.HideSelection = false
		
		lvCondition.Columns.Add "Variable"
		lvCondition.Columns.Add "State"
		lvCondition.Columns.Add "Value"
		
		
		(lvCondition.Columns.item 1).width = 38
		(lvCondition.Columns.item 0).width = (lvCondition.width - 4) / 2 - ((lvCondition.Columns.item 1).width / 2)
		(lvCondition.Columns.item 2).width = (lvCondition.width - 4) / 2 - ((lvCondition.Columns.item 1).width / 2)
		
		
		lvSourceNodes.GridLines = true
		lvSourceNodes.AllowColumnReorder = false
		lvSourceNodes.View = lvSourceNodes.View.Details
		lvSourceNodes.LabelEdit = true
		lvSourceNodes.LabelWrap = false
		lvSourceNodes.MultiSelect = true
		lvSourceNodes.FullRowSelect = true
		lvSourceNodes.HideSelection = false
		lvSourceNodes.HeaderStyle = lvSourceNodes.HeaderStyle.None
		lvSourceNodes.BackColor = lvSourceNodes.BackColor.FromARGB 255 230 230
		
		lvSourceNodes.Columns.Add ""
		(lvSourceNodes.Columns.item 0).width = lvSourceNodes.width - 4
		
		
		lvGameBones.GridLines = true
		lvGameBones.AllowColumnReorder = false
		lvGameBones.View = lvGameBones.View.Details
		lvGameBones.LabelEdit = true
		lvGameBones.LabelWrap = false
		lvGameBones.MultiSelect = true
		lvGameBones.FullRowSelect = true
		lvGameBones.HideSelection = false
		lvGameBones.HeaderStyle = lvGameBones.HeaderStyle.None
		lvGameBones.BackColor = lvGameBones.BackColor.FromARGB 255 230 230
		
		lvGameBones.Columns.Add ""
		(lvGameBones.Columns.item 0).width = lvGameBones.width - 4
		
		
		
		edCondition.ScrollBars = edCondition.Scrollbars.Both
		edCondition.WordWrap = false
		edCondition.AcceptsReturn = true
		edCondition.AcceptsTab = true
		
		
		characterArray = cryMaxTools.basic.misc.makeUnique cryAM.characterArray
		
		
		
		if characterArray.count > 0 then
		(
			lastCharacter = 1
			
			if cryAM._.cbCharacter.selectedIndex > 0 then
				lastCharacter = cryAM._.cbCharacter.selectedIndex
		)	
				
		
		
		
		if lastCharacter > 0 then
		(
			if characterArray[lastCharacter].setup.count > 0 then
			(
				lastSetup = 1
				
				if cryAM._.cbSetup.selectedIndex >= 0 then
					lastSetup = cryAM._.cbSetup.selectedIndex + 1
			)
				
			
			if characterArray[lastCharacter].setup[lastSetup].sourceNodes.count > 0 then
				lastSourceNode = 1
			
			if characterArray[lastCharacter].setup[lastSetup].gameBones.count > 0 then
				lastGameBone = 1
		)
		
			
		onUpdate = true
		updateCharacters()
		
		updateTab tabArray 1
	)
	
	
	on setupRO close do
	(
		try animManROMenu.miShowCharSetup.checked = false catch()
	)
	
	
	
	on tabModes Selected arg do
	(
		if arg != undefined then
			updateTab tabArray (arg.TabPageIndex + 1)
	)
	
	
	
	
	on radSetupModelPreferLoad changed value do
	(
		radSetupModelPreferMerge.state = 0
		
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].model.preferLoad = true
	)
	
	on radSetupModelPreferMerge changed value do
	(
		radSetupModelPreferLoad.state = 0
		
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].model.preferLoad = false
	)
	
	
	
	
	
	on chkSetupModelAutoSync changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].model.autoSync = value
	)
	
	
	
	on chkSetupModelRemoveOld changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].model.removeOld = value
	)
	
	
	
	on chkSetupBipedAutoSync changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].biped.autoSync = value
	)
	
	
	
	on chkSetupBipedRemoveOld changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].biped.removeOld = value
	)
	
	
	
	
	
	
	
	
	on edSetupModelPath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if value != "" then
			(
				local fullPathFound = (doesFileExist value)
				
				if (doesFileExist (cryAM.rootProjectPath + value)) or fullPathFound then
				(
					if fullPathFound then
						value = subString value (rootProjectPath.count + 1) value.count
					
					characterArray[lastCharacter].setup[lastSetup].model.path = value
				)
				else
					messageBox "Could not find specified Model" title:"Select Model Path"
			)
			else
				characterArray[lastCharacter].setup[lastSetup].model.path = ""
		)
		
		
		updateSetupOptions()
	)
	
	
	
	on edSetupBipedPosePath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if value != "" then
			(
				if doesFileExist (cryAM.rootProjectPath + value) then
					characterArray[lastCharacter].setup[lastSetup].biped.path = value
				else
					messageBox "Could not find specified Pose File" title:"Select Pose File Path"
			)
			else
				characterArray[lastCharacter].setup[lastSetup].biped.path = ""
		)
		
		
		updateSetupOptions()
	)
	
	
	
	on edSetupAlignRotation entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local tempValue = execute value
			if classOf tempValue != Quat then
				tempValue = quat 1
			
			characterArray[lastCharacter].setup[lastSetup].align.rot = tempValue
			
			edSetupAlignRotation.text = tempValue as String
		)
	)
	
	
	
	on edSetupAlignPosition entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local tempValue = execute value
			if classOf tempValue != Point3 then
				tempValue = [0,0,0]
			
			characterArray[lastCharacter].setup[lastSetup].align.pos = tempValue
			
			edSetupAlignPosition.text = tempValue as String
		)
	)
	

	
	
	
	
	
	on btnSetupModelPathBrowse pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getOpenFileName "Select Model File" initialDir:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].model.path) types:"3ds Max (*.max)|*.max"
			if newPath != undefined then
			(
				characterArray[lastCharacter].setup[lastSetup].model.path = subString newPath (cryAM.rootProjectPath.count + 1) newPath.count
				edSetupModelPath.text = characterArray[lastCharacter].setup[lastSetup].model.path
				
			)
		)
	)
	
	
	on btnSetupBipedPosePathBrowse pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getOpenFileName "Select Biped Pose File" initialDir:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].biped.path) types:"(*.CPY)|*.cpy"
			if newPath != undefined then
			(
				characterArray[lastCharacter].setup[lastSetup].model.path = subString newPath (cryAM.rootProjectPath.count + 1) newPath.count
				edSetupModelPath.text = characterArray[lastCharacter].setup[lastSetup].model.path
			)
		)
	)
	
	
	
	on btnSetupAlignSourceNodeSet pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local tempSel = undefined
			if selection.count > 0 then
				tempSel = (selection as Array)[1]
			else
			(
				local newSel = selectByName title:"Select the Source Node" single:true
				if newSel != undefined then
					tempSel = newSel
			)
			
			if tempSel != undefined then
				tempSel = tempSel.name
			else
				tempSel = ""
			
			characterArray[lastCharacter].setup[lastSetup].align.sourceNode = tempSel
			btnSetupAlignSourceNodeSet.text = "Source Node :  " + (if tempSel != "" then tempSel else "None")
		)
	)
	
	
	
	
	on btnSetupAlignTargetNodeSet pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local tempSel = undefined
			if selection.count > 0 then
				tempSel = (selection as Array)[1]
			else
			(
				local newSel = selectByName title:"Select the Target Node" single:true
				if newSel != undefined then
					tempSel = newSel
			)
			
			if tempSel != undefined then
				tempSel = tempSel.name
			else
				tempSel = ""
			
			characterArray[lastCharacter].setup[lastSetup].align.targetNode = tempSel
			btnSetupAlignTargetNodeSet.text = "Target Node :  " + (if tempSel != "" then tempSel else "None")
		)
	)
	
	
	
	
	on btnSetupAlignRotPos pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local setup = characterArray[lastCharacter].setup[lastSetup]
			
			if setup.align.sourceNode != "" and setup.align.targetNode != "" then
			(
				local sourceNode = getNodeByName setup.align.sourceNode
				local targetNode = getNodeByName setup.align.targetNode
				
				if sourceNode != undefined and targetNode != undefined then
				(
					local locTM = sourceNode.transform * inverse targetNode.transform
					
					setup.align.rot = locTM.rotation
					setup.align.pos = locTM.pos
				)
			)
			else
			(
				setup.align.rot = quat 1
				setup.align.pos = [0,0,0]
			)
			
			
			edSetupAlignRotation.text = setup.align.rot as String
			edSetupAlignPosition.text = setup.align.pos as String
		)
	)
	
	
	
	on spnSetupMiscSkipFirstFrame changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].misc.skipFirst = value
	)
	
	on spnSetupMiscSkipLastFrame changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].misc.skipLast = value
	)
	
	
	on chkSetupMiscCustomRig changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].misc.customRig = value
	)
	
	
	on edSetupMappingPath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].misc.mappingFile = value
	)
	
	on btnSetupMappingPathBrowse pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getOpenFileName "Select Mapping File" initialDir:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].misc.mappingFile) types:"XML Animation Map File (*.xmm)|*.xmm"
			if newPath != undefined then
			(
				characterArray[lastCharacter].setup[lastSetup].misc.mappingFile = newPath
				edSetupMappingPath.text = newPath
			)
		)
	)
	
	on edSetupXAFPosePath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
			characterArray[lastCharacter].setup[lastSetup].misc.poseFile = value
	)
	
	on btnSetupXAFPosePathBrowse pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getOpenFileName "Select Mapping File" initialDir:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].misc.poseFile) types:"XML Animation File (*.xaf)|*.xaf"
			if newPath != undefined then
			(
				characterArray[lastCharacter].setup[lastSetup].misc.poseFile = newPath
				edSetupXAFPosePath.text = newPath
			)
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	on lvCharacter MouseDown arg do
	(
		local tempEntry = (lvCharacter.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			cryAM.lastDraggedEntry = -1
		else
			cryAM.lastDraggedEntry = tempEntry
	)
	

	
	on lvCharacter MouseMove arg do
	(
		if arg.button == arg.button.left and cryAM.lastDraggedEntry != -1 then
			cryAM.dragList lvCharacter characterArray arg
	)
	
	
	
	on lvCharacter MouseUp arg do
	(
		cryAM.lastDraggedEntry = -1
		cryAM.lastHoveredEntry = -1
		
		setupRO.onUpdate = true
		updateSetups()
		
		if arg.button == arg.button.right then
			callCharacterMenu()
	)
	
	
	
	
	
	on lvSetup MouseUp arg do
	(
		cryAM.lastDraggedEntry = -1
		cryAM.lastHoveredEntry = -1
		
		setupRO.onUpdate = true
		updatePaths()
		
		updateSetupOptions()
		
		updateNodes #sourceNodes
		updateNodes #gameBones
		
		if arg.button == arg.button.right then
			callSetupMenuRC()
	)
	
	
	
	
	
	
	on lvSetup MouseDown arg do
	(
		local tempEntry = (lvSetup.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			cryAM.lastDraggedEntry = -1
		else
			cryAM.lastDraggedEntry = tempEntry
	)
	

	
	on lvSetup MouseMove arg do
	(
		if arg.button == arg.button.left and cryAM.lastDraggedEntry != -1 then
			cryAM.dragList lvSetup characterArray[lastCharacter].setup arg
	)
	
	
	
	
	on lvCondition MouseUp arg do
	(
		cryAM.lastDraggedEntry = -1
		cryAM.lastHoveredEntry = -1
		
		onUpdate = true
		
		updateValues()
		
		if arg.button == arg.button.right then
			callConditionMenu()
	)
	
	
	
	
	
	
	
	on lvCondition MouseDown arg do
	(
		local tempEntry = (lvCondition.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			cryAM.lastDraggedEntry = -1
		else
			cryAM.lastDraggedEntry = tempEntry
	)
	

	
	on lvCondition MouseMove arg do
	(
		if arg.button == arg.button.left and cryAM.lastDraggedEntry != -1 then
			cryAM.dragList lvCondition characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions arg
	)
	
	
	
	

	
	
	
	
	on lvSourceNodes MouseUp arg do
	(
		if arg.button == arg.button.right then
		(
			cryAM.taskMode = #sourceNodes
			callBonesMenu()
		)
	)
	
	
	
	on lvGameBones MouseUp arg do
	(
		onUpdate = true
		
		updateConditions()
		
		if arg.button == arg.button.right then
		(
			cryAM.taskMode = #gameBones
			callBonesMenu()
		)
	)
	
	
	

	
	
	on lvCharacter GotFocus arg do
	(
		if lvCharacter.selectedItems.count > 0 then
			characterArray[(lvCharacter.selectedItems.item 0).index + 1].name = (lvCharacter.selectedItems.item 0).text
	)
	
	
	on lvSetup GotFocus arg do
	(
		if lvSetup.selectedItems.count > 0 then
			if characterArray[lastCharacter] != undefined then
				characterArray[lastCharacter].setup[(lvSetup.selectedItems.item 0).index + 1].name = (lvSetup.selectedItems.item 0).text
	)
	
	
	on lvSourceNodes GotFocus arg do
	(
		if lvSourceNodes.selectedItems.count > 0 then
			if characterArray[lastCharacter] != undefined then
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
					characterArray[lastCharacter].setup[lastSetup].sourceNodes[(lvSourceNodes.selectedItems.item 0).index + 1] = (lvSourceNodes.selectedItems.item 0).text
	)
	
	
	on lvGameBones GotFocus arg do
	(
		if lvGameBones.selectedItems.count > 0 then
			if characterArray[lastCharacter] != undefined then
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
					characterArray[lastCharacter].setup[lastSetup].gameBones[(lvGameBones.selectedItems.item 0).index + 1].name = (lvGameBones.selectedItems.item 0).text
	)
	
	
	
	on lvCharacter SelectedIndexChanged arg do
	(
		if lvCharacter.selectedItems.count > 0 then
			lastCharacter = (lvCharacter.selectedItems.item 0).index + 1
		
		
		for i = 0 to (lvCharacter.items.count - 1) do
		(
			local tempNode = lvCharacter.items.item i
			
			if i != (lastCharacter - 1) then
				tempNode.backColor = tempNode.backColor.fromArgb 230 230 255
			else
				tempNode.backColor = tempNode.backColor.fromArgb 255 230 230
		)
	)
	
	
	
	on lvSetup SelectedIndexChanged arg do
	(
		if lvSetup.selectedItems.count > 0 then
			lastSetup = (lvSetup.selectedItems.item 0).index + 1
		
		
		for i = 0 to (lvSetup.items.count - 1) do
		(
			local tempNode = lvSetup.items.item i
			
			if i != (lastSetup - 1) then
				tempNode.backColor = tempNode.backColor.fromArgb 255 230 230
			else
				tempNode.backColor = tempNode.backColor.fromArgb 230 230 255
		)
	)
	
	
	
	on lvGameBones SelectedIndexChanged arg do
	(
		if lvGameBones.selectedItems.count > 0 then
			lastGameBone = (lvGameBones.selectedItems.item 0).index + 1
		
		
		for i = 0 to (lvGameBones.items.count - 1) do
		(
			local tempNode = lvGameBones.items.item i
			
			if i != (lastGameBone - 1) then
				tempNode.backColor = tempNode.backColor.fromArgb 255 230 230
			else
				tempNode.backColor = tempNode.backColor.fromArgb 230 230 255
		)
		
		onUpdate = true
		
		updateConditions()
	)
	
	
	
	
	
	
	on lvCharacter ItemChecked arg do
	(
		if setupRO.onUpdate == false then
			characterArray[arg.item.index + 1].active = arg.item.checked
	)
	
	
	on lvSetup ItemChecked arg do
	(
		if setupRO.onUpdate == false then
			if characterArray[lastCharacter] != undefined then
				characterArray[lastCharacter].setup[arg.item.index + 1].active = arg.item.checked
	)
	
	
	on lvCondition ItemChecked arg do
	(
		if setupRO.onUpdate == false then
			if characterArray[lastCharacter] != undefined then
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
					if characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone] != undefined then
						characterArray[lastCharacter].setup[lastSetup].gameBones[lastGameBone].conditions[arg.item.index + 1].active = arg.item.checked
	)
	
	
	
	
	
	
	
	
	on edPropertyValue entered arg do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
				(
					local boneID = (lvGameBones.selectedItems.item 0).index + 1
					
					if boneID > 0 then
					(
						for i = 0 to (lvCondition.selectedItems.count - 1) do
						(
							local selIndex = (lvCondition.selectedItems.item i).index + 1
							
							characterArray[lastCharacter].setup[lastSetup].gameBones[boneID].conditions[selIndex].value = "\"" + arg as String + "\""
						)
						
						updateConditions skipValue:true
					)
				)
			)
		)
	)
	
	
	on ddPropertyValue selected value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
				(
					local boneID = (lvGameBones.selectedItems.item 0).index + 1
					
					if boneID > 0 then
					(
						for i = 0 to (lvCondition.selectedItems.count - 1) do
						(
							local selIndex = (lvCondition.selectedItems.item i).index + 1
							
							characterArray[lastCharacter].setup[lastSetup].gameBones[boneID].conditions[selIndex].value = (value == 1) as String
						)
						
						updateConditions skipValue:true
					)
				)
			)
		)
	)
	
	
	
	
	on spnPropertyValueInteger changed value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
				(
					local boneID = (lvGameBones.selectedItems.item 0).index + 1
					
					if boneID > 0 then
					(
						for i = 0 to (lvCondition.selectedItems.count - 1) do
						(
							local selIndex = (lvCondition.selectedItems.item i).index + 1
							
							characterArray[lastCharacter].setup[lastSetup].gameBones[boneID].conditions[selIndex].value = value as String
						)
						
						updateConditions skipValue:true
					)
				)
			)
		)
	)
	
	
	
	on spnPropertyValueFloat changed value do
	(
		onUpdate = true
		
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if characterArray[lastCharacter] != undefined then
			(
				if characterArray[lastCharacter].setup[lastSetup] != undefined then
				(
					local boneID = (lvGameBones.selectedItems.item 0).index + 1
					
					if boneID > 0 then
					(
						for i = 0 to (lvCondition.selectedItems.count - 1) do
						(
							local selIndex = (lvCondition.selectedItems.item i).index + 1
							
							characterArray[lastCharacter].setup[lastSetup].gameBones[boneID].conditions[selIndex].value = value as String
						)
						
						updateConditions skipValue:true
					)
				)
			)
		)
	)
	
	
	

	
	
	
	on edSourcePath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if value != "" then
			(
				if value[value.count] != "\\" then
					value += "\\"
			)
			
			characterArray[lastCharacter].setup[lastSetup].sourcePath = value
		)
		else
			characterArray[lastCharacter].setup[lastSetup].sourcePath = ""
			
		edSourcePath.text = characterArray[lastCharacter].setup[lastSetup].sourcePath
		edGamePath.text = characterArray[lastCharacter].setup[lastSetup].gamePath
	)
	
	
	
	on edGamePath entered value do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			if value != "" then
			(
				if value[value.count] != "\\" then
					value += "\\"
			)
			
			characterArray[lastCharacter].setup[lastSetup].gamePath = value
		)
		else
			characterArray[lastCharacter].setup[lastSetup].gamePath = ""
		
		edSourcePath.text = characterArray[lastCharacter].setup[lastSetup].sourcePath
	)
	
	
	
	
	
	
	
	
	
	on btnSourcePath pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getSavePath "Select Source Folder" initialDir:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].sourcePath)
			if newPath != undefined then
			(
				if newPath[newPath.count] != "\\" then
					newPath += "\\"
				
				characterArray[lastCharacter].setup[lastSetup].sourcePath = cryAM.updateRootPath newPath #source get:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].sourcePath)
				edSourcePath.text = characterArray[lastCharacter].setup[lastSetup].sourcePath
			)
		)
	)
	
	
	on btnGamePath pressed do
	(
		if lastCharacter > 0 and lastSetup > 0 then
		(
			local newPath = getSavePath "Select Game Folder" initialDir:characterArray[lastCharacter].setup[lastSetup].gamePath
			if newPath != undefined then
			(
				if newPath[newPath.count] != "\\" then
					newPath += "\\"
				
				characterArray[lastCharacter].setup[lastSetup].gamePath = cryAM.updateRootPath newPath #game get:(cryAM.rootProjectPath + characterArray[lastCharacter].setup[lastSetup].gamePath)
				edGamePath.text = characterArray[lastCharacter].setup[lastSetup].gamePath
			)
		)
	)
	
	

	
	
	on btnSave pressed do
	(
		local saveSel = cryAM._.cbCharacter.selectedIndex
		
		if saveSel > -1 then
			saveSel = cryAM.characterArray[cryAM._.cbCharacter.selectedIndex].name
		
		cryAM.characterArray = characterArray
		cryAM.updateCharacterList skipSetupList:true
		
		cryAM.taskMode = #none
		
		for i = 1 to cryAM.characterArray.count do
		(
			if cryAM.characterArray[i].name == saveSel and cryAM.characterArray[i].active == true then
			(
				cryAM.onUpdate = true
				cryAM._.cbCharacter.selectedIndex = i
				cryAM.onUpdate = false
			)
		)
		
		
		
		if cryAM._.cbCharacter.selectedIndex == -1 then
		(
			for i = characterArray.count to 1 by -1 do
			(
				if characterArray[i].active == true then
				(
					cryAM.onUpdate = true
					cryAM._.cbCharacter.selectedIndex = i
					cryAM.onUpdate = false
					exit
				)
			)
		)
		
		
		
		try destroyDialog setupRO catch()
		
		--cryAM.updateSetupList()
		cryAM.updateByCharacter()
		
		
		if queryBox "Save Settings to File?" title:("Save Character Settings") == true then
		(
			cryAM.saveSettings mode:#character
			cryAM.saveSettings mode:#user
		)
	)
	
	
	on btnOpen pressed do
	(
		local newSettings = cryAM.loadSettings mode:cryAM.taskMode writeTo:true
		
		characterArray = newSettings[1]
		
		if characterArray.count > 0 then
			lastCharacter = 1
		
		
		
		if lastCharacter > 0 then
		(
			if characterArray[lastCharacter].setup.count > 0 then
				lastSetup = 1
			
			if characterArray[lastCharacter].setup[lastSetup].sourceNodes.count > 0 then
				lastSourceNode = 1
			
			if characterArray[lastCharacter].setup[lastSetup].gameBones.count > 0 then
				lastGameBone = 1
		)
		
			
		onUpdate = true
		updateCharacters()
	)
	
		
	on btnSaveAs pressed do
		cryAM.saveSettings mode:cryAM.taskMode saveAs:true custom:characterArray
	
	
)

createDialog setupRO (680-143) (296+85-20-6)