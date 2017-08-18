try destroyDialog taskRO catch()
		
rollout animManSettingsRO "Settings"
(
	local taskArray = #()
	local taskUserArray = #()
	local templateArray = #()
	local filepath = ""
	
	local taskMode = #export
	
	local lastTask = 0
	local lastProcess = 0
	
	local onUpdate = false
	local resizeArray = #()
	
	local processRightMouseClickRC = undefined
	
	radiobuttons radLoadMode "" pos:[8,5+7] labels:#(" Load") default:0
	radiobuttons radSaveMode "" pos:[8,25+6] labels:#(" Save")
	
	groupbox gbMode "" pos:[3,0] width:61 height:54
	
	groupbox gbFile "" pos:[63,0] width:122 height:54
	
	
	button btnOpen "Open" pos:[5+68-5,5+6] height:18 width:55
	button btnSaveAs "Save As" pos:[62+68-5,5+6] height:18 width:55
	button btnSave "Save" pos:[5+68-5,25+6] height:18 width:112
	
	
	--groupbox gbTasks "" pos:[3,25+40+10] width:182 height:135
	--label labTasks "   Load Tasks   " pos:[55,23+40+10]
	dotNetControl lvTasks "System.Windows.Forms.ListView" name:"lvTasks" pos:[3,40+40+10-25] width:182 height:120
	
	--groupbox gbProcess "" pos:[3,25+145+40+10] width:182 height:50
	--label labProcess "   Process   " pos:[66,23+145+40+10]
	dotNetControl lvProcess "System.Windows.Forms.ListView" name:"lvProcess" pos:[3,40+145+40-33-5] width:182 height:120
	
	--groupbox gbProperty "" pos:[3,25+145+145+40+10] width:182
	--label labProperty "   Properties   " pos:[60,23+145+145+40+10]
	dotNetControl lvProperties "System.Windows.Forms.ListView" name:"lvProperties" pos:[3,40+145+145-11-10] width:182 height:120
	
	label labPropertyType "Type:    None" pos:[5,435] width:180 height:18
	label labPropertyValue "Value:" pos:[5,454]
	edittext edPropertyValue "" pos:[5+35,454] width:(150-5)
	dropdownlist ddPropertyValue "" pos:[5+38,452] width:(149-5) visible:false
	spinner spnPropertyValueInteger "" pos:[5+38,452] width:(148-5) visible:false type:#Integer range:[-64000, 64000, 0]
	spinner spnPropertyValueFloat "" pos:[5+38,452] width:(148-5) visible:false type:#float range:[-64000.0, 64000.0, 0.0]
	
	groupbox gpScript " Script " pos:[285+200,5] height:300 width:400--400
	dotNetControl edScript "System.Windows.Forms.TextBox" pos:[293+200,22] height:240 width:380 visible:false
	
	checkbutton btnShowScript "S C R I P T    >>" pos:[5,452+30] height:18 width:180 checked:false
	
	button btnScriptOpen "Open" pos:[293+200,270] height:20 width:60
	button btnScriptSave "Save As..." pos:[368+200,270] height:20 width:60
	button btnScriptClear "Clear" pos:[613+200,270] height:20 width:60
	
	
	
	
	fn sortByName entry1 entry2 =
	(
		if entry1.name > entry2.name then 1
		else 0
	)
	
	
	
	function updateProcessTemplates =
	(
		local tempArray = #()
		
		qsort templateArray sortByName
		
		for i = 1 to templateArray.count do
			append tempArray templateArray[i].name
		
		--gc light:true
	)
	
	
	
	
	
	function updateDimensions item newPos:0 =
	(	
		if item != undefined then
		(
			if classOf item == rolloutClass then
			(
				item.height = item.height + newPos
				
				return true
			)
			
			try item.BeginUpdate() catch()
			
			item.pos = [item.pos[1], (item.pos[2] + newPos)]
			
				--// handle listView \\--
			
			if classOf item == dotNetControl then
			(
				if (dotNetClass item) as String == "dotNetClass:System.Windows.Forms.ListView" then
				(
					local mul = 14
					local amount = item.items.count
					
					if item.checkBoxes == true then
					(
						mul = 17
						
						if amount == 0 then
							amount = 1
					)
					
					
					
					
					local newHeight = (amount * mul) + 18
					
					newPos += newHeight - item.height
					
					item.height = newHeight
					
					item.pos += [0,1]
					item.pos -= [0,1]
					
					case item.name of
					(
						"lvTasks":		updateDimensions lvProcess newPos:newPos
						"lvProcess":		updateDimensions lvProperties newPos:newPos
						"lvProperties":
						(
							updateDimensions labPropertyType newPos:newPos
							updateDimensions labPropertyValue newPos:newPos
							updateDimensions edPropertyValue newPos:newPos
							updateDimensions ddPropertyValue newPos:newPos
							updateDimensions spnPropertyValueInteger newPos:newPos
							updateDimensions spnPropertyValueFloat newPos:newPos
							
							updateDimensions btnShowScript newPos:newPos
							
							updateDimensions animManSettingsRO newPos:newPos
						)
					)
				)
			)
			
			
			
			try item.EndUpdate() catch()
		)
	)
	
	
	
	
	
	
	
	
	function updatePropertyValue =
	(
		edPropertyValue.text = ""
		spnPropertyValueInteger.value = 0
		spnPropertyValueFloat.value = 0.0
		ddPropertyValue.selection = 1
		
		labPropertyType.text = "Type:    None"
		
		if lvProperties.selectedItems.count > 0 then
		(
			local tempEntry = ((lvProperties.selectedItems.item 0).subItems.item 0)
			if tempEntry.text != undefined and tempEntry.text != "function" then
			(
				edPropertyValue.enabled = true
				ddPropertyValue.enabled = true
				spnPropertyValueInteger.enabled = true
				spnPropertyValueFloat.enabled = true
				
				edPropertyValue.visible = false
				ddPropertyValue.visible = false
				spnPropertyValueInteger.visible = false
				spnPropertyValueFloat.visible = false
				
				local usedClass = execute ("classOf " + tempEntry.text)
				local usedElement = undefined
				
				case usedClass of
				(
					Integer:			( usedElement = spnPropertyValueInteger ; usedElement.value = (execute tempEntry.text) )
					Float:			( usedElement = spnPropertyValueFloat ; usedElement.value = (execute tempEntry.text) )
					BooleanClass:	( usedElement = ddPropertyValue ; usedElement.selection = (if (execute tempEntry.text) == true then 1 else  2 ))
					String:			( usedElement = edPropertyValue ; usedElement.text = (execute tempEntry.text) )
				)
				
				
				usedElement.visible = true
				
				
				labPropertyType.text = try "Type:    " + usedClass as String catch("Type:    unknown")
			)
			else
			(
				edPropertyValue.enabled = false
				ddPropertyValue.enabled = false
				spnPropertyValueInteger.enabled = false
				spnPropertyValueFloat.enabled = false
			)
		)
		
		--gc light:true
	)
	
	
	
	
	
	
	function updateProperties forceUpdate:false keepSelection:false =
	(
		local safeSel = -1
		try safeSel = (lvProperties.selectedItems.item 0).index catch()
		
		lvProperties.items.clear()
		
		if keepSelection == false then
			edPropertyValue.text = ""
		
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
				
				local tempStream = stringStream animManRO.edScript.text
				
				
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
							local tempEntry = lvProperties.items.add "function" "function"
							tempEntry.subItems.add functionName
							tempEntry.ForeColor = tempEntry.ForeColor.red
							
							functionIndex = propIndex
							
							propIndex += 1
						)
						
						for i = 3 to paramFilter.count do
						(
							local optionalParam = filterString paramFilter[i] ":"
							if optionalParam.count == 1 then
								append paramArray (animManRO.propertiesStruct name:optionalParam[1] value:"" index:propIndex)
							else
								append paramArray (animManRO.propertiesStruct name:optionalParam[1] value:optionalParam[2] index:propIndex)
								
							
							propIndex += 1
						)
						
						
						for i = 1 to paramArray.count do
						(
							local newParam = ""
							if paramArray[i].value != "undefined" then
								newParam = paramArray[i].value
							
							local tempEntry = lvProperties.items.add newParam newParam
							tempEntry.subItems.add paramArray[i].name
						)
						
						
						append taskArray[lastTask].process[lastProcess].fnData (animManRO.functionStruct name:functionName properties:paramArray index:functionIndex)
					)
				)
				
				
				
				close tempStream
			)
			else
			(
				for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
				(
					local tempEntry = lvProperties.items.add "function" "function"
					tempEntry.subItems.add taskArray[lastTask].process[lastProcess].fnData[i].name
					tempEntry.ForeColor = tempEntry.ForeColor.red
					
					for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
					(
						local tempEntry = lvProperties.items.add taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value
						tempEntry.subItems.add taskArray[lastTask].process[lastProcess].fnData[i].properties[d].name
					)
				)
			)
			
			if keepSelection == true then
				try (lvProperties.items.item safeSel).selected = true catch()
		)
		
		updateDimensions lvTasks
		
		--gc light:true
	)
	
	
	
	
	
	function updateScript =
	(
		animManRO.edScript.text = ""
		
		if lastTask > 0 and lastProcess > 0 then
			if taskArray[lastTask] != undefined then
				if taskArray[lastTask].process[lastProcess] != undefined then
					animManRO.edScript.text = taskArray[lastTask].process[lastProcess].script.text
				
		onUpdate = false
		
		updateProperties()
	)
	
	
	
	
	
	
	function updateProcess =
	(
		lvProcess.items.clear()
		
		if lastTask > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				for i = 1 to taskArray[lastTask].process.count do
				(
					local newEntry = lvProcess.items.add taskArray[lastTask].process[i].name
					newEntry.checked = taskArray[lastTask].process[i].active
				)
				
				if lastProcess > 0 and lastProcess <= lvProcess.items.count then
					(lvProcess.items.item (lastProcess - 1)).selected = true
			)
		)
		
		
		updateScript()
		
		--gc light:true
	)
	
	
	
	
	function updateTasks =
	(
		
		lvTasks.items.clear()
		
		for i = 1 to taskArray.count do
		(
			local newEntry = lvTasks.items.add taskArray[i].name
			newEntry.checked = taskArray[i].active
			
			if taskArray[i].default == true then
				newEntry.backColor = newEntry.backColor.fromARGB 230 255 230
			else
				newEntry.backColor = newEntry.backColor.fromARGB 255 240 240
			
			
		)
		
		if lastTask > 0 and lastTask <= lvTasks.items.count then
			(lvTasks.items.item (lastTask - 1)).selected = true
		
		lastProcess = 1
		
		updateProcess()
				
		--gc light:true
	)
	
	
	
	
	
	
	
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
					
					for f = 1 to taskArray.count do
						if (taskArray[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newTask" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				append taskArray (animManRO.taskStruct name:newName)
				append taskArray[taskArray.count].process (animManRO.processStruct name:"newProcess01")
				
				lastTask = taskArray.count
				
				onUpdate = true
				updateTasks()
				
				--gc light:true
			)
			
			
			
			
			on taskRightMouseClickRC open do
			(
				if lvTasks.items.count == 0 then
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
					
					if lastTask > 0 then
						miSwitchDefault.checked = not taskArray[lastTask].default
					
					if animManRO.copyTask == undefined then
						miPaste.enabled = false
				)
			)
			
			
			on miNew picked do
				newEntry()
			
			
			
			on miRun picked do
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
									
									for f = 1 to taskArray[lastTask].process[i].fnData.count do
									(
										local execStr = taskArray[lastTask].process[i].fnData[f].name
										
										if taskArray[lastTask].process[i].fnData[f].properties.count > 0 then
										(
											for d = 1 to taskArray[lastTask].process[i].fnData[f].properties.count do
												execStr += " " + taskArray[lastTask].process[i].fnData[f].properties[d].name + ":" + taskArray[lastTask].process[i].fnData[f].properties[d].value as String
										)
										else
											execStr += "()"
										try
											execute ( execStr )
										catch
										(
											format "*** % ***\n" (scriptFound + "  Error in Function:  " + taskArray[lastTask].process[i].fnData[f].name)
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
				animManRO.copyTask = #()
				
				if lastTask > 0 then
				(
					for i = 0 to (lvTasks.selectedItems.count - 1) do
					(
						local index = (lvTasks.selectedItems.item i).index + 1
						if taskArray[index] != undefined then
							append animManRO.copyTask taskArray[index]
					)
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				for i = 1 to animManRO.copyTask.count do
				(
					newEntry()
					
					if animManRO.copyTask != undefined then
						if lastTask > 0 then
							if taskArray[lastTask] != undefined then
									taskArray[lastTask] = cryMaxTools.basic.misc.makeUnique animManRO.copyTask[i]
					
					onUpdate = true
					updateTasks()
				)
			)
			
			
			
			on miSwitchDefault picked do
			(
				for i = 0 to (lvTasks.selectedItems.count - 1) do
				(
					local index = (lvTasks.selectedItems.item i).index + 1
					taskArray[index].default = miSwitchDefault.checked
				)
				
				onUpdate = true
				
				updateTasks()
			)
			
			
			
			
			
			
			on miDelete picked do
			(
				if lvTasks.selectedItems.count > 0 then
				(
					local newTaskArray = #()
					local delArray = #{}
					
					for i = 0 to (lvTasks.selectedItems.count - 1) do
						delArray[(lvTasks.selectedItems.item i).index + 1] = true
					
					for i = 1 to taskArray.count do
					(
						if delArray[i] == false then
							append newTaskArray taskArray[i]
					)
						
					taskArray = newTaskArray
				)
				
				if taskArray.count == 0 then
					lastTask = 0
				else
					lastTask = 1
		
				onUpdate = true
				updateTasks()
				
				--gc light:true
			)
		)
		
		registerRightClickMenu taskRightMouseClickRC
		popUpMenu taskRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
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
					
					for f = 1 to taskArray[lastTask].process.count do
						if (taskArray[lastTask].process[f].name == tempName) and  (i > newNameMaxCount) then
							newNameMaxCount = i
				)
				
				newNameMaxCount += 1
				newName = "newProcess" + (if newNameMaxCount > 9 then "" else "0") + newNameMaxCount as String
				
				local newProcess = animManRO.processStruct name:newName
				newProcess.script.text = animManRO.edScript.text
				
				append taskArray[lastTask].process newProcess
				
				
				lastProcess = taskArray[lastTask].process.count
				
				onUpdate = true
				updateProcess()
				
				animManRO.edScript.text = ""
				
				--gc light:true
			)
			
			
			function removeEntry index =
			(
				if queryBox ("Remove " + templateArray[index].name + "?") == true then
				(
					deleteItem templateArray index
					updateProcessTemplates()
				)
				
				--gc light:true
			)
			
			
			
			function getEntry index =
			(
				if lastTask > 0 then
				(
					if taskArray[lastTask] != undefined then
					(
						append taskArray[lastTask].process templateArray[index] 
						
						lastProcessSelection = taskArray[lastTask].process.count
						
						onUpdate = true
						updateProcess()
					)
				)
				
				--gc light:true
			)
			
			
			
			
			on processRightMouseClickRC open do
			(
				if lastTask == 0 then
					miNew.enabled = false
				
				if lvProcess.items.count == 0 then
				(
					miRun.enabled = false
					miCopy.enabled = false
					miDelete.enabled = false
				)
			
					
				if animManRO.copyProcess == undefined then
					miPaste.enabled = false
				
				
				
				if lastTask > 0 and lastProcess > 0 then
				(
					local foundEntry = 0
					
					for i = 1 to templateArray.count do
					(
						if templateArray[i].name == taskArray[lastTask].process[lastProcess].name then
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
					if templateArray.count >= i then
					(
						local tempString = (".text = \"" + templateArray[i].name + "\"")
						execute ("taskRO.processRightMouseClickRC." + "miGetTemplate" + i as String + tempString)
						execute ("taskRO.processRightMouseClickRC." + "miRemoveTemplate" + i as String + tempString)
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
					
					for f = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
					(
						local execStr = taskArray[lastTask].process[lastProcess].fnData[f].name
						
						if taskArray[lastTask].process[lastProcess].fnData[f].properties.count > 0 then
						(
							for d = 1 to taskArray[lastTask].process[lastProcess].fnData[f].properties.count do
								execStr += " " + taskArray[lastTask].process[lastProcess].fnData[f].properties[d].name + ":" + taskArray[lastTask].process[lastProcess].fnData[f].properties[d].value as String
						)
						else
							execStr += "()"
						try
							execute ( execStr )
						catch
						(
							format "*** % ***\n" (scriptFound + "  Error in Function:  " + taskArray[lastTask].process[lastProcess].fnData[f].name)
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
				if lastTask > 0 and lastProcess > 0 then
				(
					if taskArray[lastTask] != undefined then
					(
						if taskArray[lastTask].process[lastProcess] != undefined then
						(
							local foundEntry = 0
							
							for i = 1 to templateArray.count do
							(
								if templateArray[i].name == taskArray[lastTask].process[lastProcess].name then
								(
									foundEntry = i
									exit
								)
							)
							
							if queryBox ((if foundEntry == 0 then "Add" else "Update") + " selected Process to the Templates?") == true then
							(
								if addEntry == true then
									append templateArray taskArray[lastTask].process[lastProcess]
								else
									templateArray[foundEntry] = taskArray[lastTask].process[lastProcess]
								
								updateProcessTemplates()
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
				if lvProcess.selectedItems.count > 0 then
				(
					local newProcessArray = #()
					local delArray = #{}
					
					for i = 0 to (lvProcess.selectedItems.count - 1) do
						delArray[(lvProcess.selectedItems.item i).index + 1] = true
					
					for i = 1 to taskArray[lastTask].process.count do
					(
						if delArray[i] == false then
							append newProcessArray taskArray[lastTask].process[i]
					)
						
					taskArray[lastTask].process = newProcessArray
				)
				
				if taskArray[lastTask].process.count == 0 then
					lastProcess = 0
				else
					lastProcess = 1
		
				onUpdate = true
				updateProcess()
				
				--gc light:true
			)
			
			
			on miCopy picked do
			(
				animManRO.copyProcess = #()
				
				if lastTask > 0 and lastProcess > 0 then
				(
					for i = 0 to (lvProcess.selectedItems.count - 1) do
					(
						local index = (lvProcess.selectedItems.item i).index + 1
						if taskArray[lastTask] != undefined then
							if taskArray[lastTask].process[index] != undefined then
								append animManRO.copyProcess taskArray[lastTask].process[index]
					)
				)
				
				--gc light:true
			)
			
			
			on miPaste picked do
			(
				for i = 1 to animManRO.copyProcess.count do
				(
					newEntry()
					
					if animManRO.copyProcess != undefined then
						if lastTask > 0 and lastProcess > 0 then
							if taskArray[lastTask] != undefined then
								if taskArray[lastTask].process[lastProcess] != undefined then
									taskArray[lastTask].process[lastProcess] = cryMaxTools.basic.misc.makeUnique animManRO.copyProcess[i]
								
					onUpdate = true
					updateProcess()
				)
			)
		)
		
		registerRightClickMenu processRightMouseClickRC
		popUpMenu processRightMouseClickRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
	
	
	
	
	

	
	
	function updateDialog skipMode:false =
	(
		if skipMode == false then
		(
			if animManRO.ddLoad.selection > 1 then
			(
				taskMode = #import
				radLoadMode.state = 1
				radSaveMode.state = 0
			)
		)
		
		case taskMode of
		(
			#import:
			(
				taskArray = cryMaxTools.basic.misc.makeUnique animManRO.importArray
				join taskArray (cryMaxTools.basic.misc.makeUnique animManRO.importUserArray)
				templateArray = cryMaxTools.basic.misc.makeUnique animManRO.importTemplateArray
				filepath = animManRO.importSettingsPath
				--labTasks.text = "   Load Tasks   "
			)
			#export:
			(
				taskArray = cryMaxTools.basic.misc.makeUnique animManRO.exportArray
				join taskArray (cryMaxTools.basic.misc.makeUnique animManRO.exportUserArray)
				templateArray = cryMaxTools.basic.misc.makeUnique animManRO.exportTemplateArray
				filepath = animManRO.exportSettingsPath
				--labTasks.text = "   Save Tasks   "
			)
		)
		
		
		
		if taskArray.count > 0 then
		(
			local mainLastTask = 1
			case taskMode of
			(
				#import:
				(
					if animManRO.ddLoad.selection > 1 then
						mainLastTask = animManRO.ddLoad.selection - 1
				)
				
				#export:
				(
					if animManRO.ddSave.selection > 0 then
						mainLastTask = animManRO.ddSave.selection
				)
			)
			
			lastTask = mainLastTask
		)
		
		
		if lastTask > 0 then
			if taskArray[lastTask].process.count > 0 then
				lastProcess = 1
		
		onUpdate = true
			
		updateTasks()
		updateProcessTemplates()
			
		--updateDimensions lvTasks
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	on animManSettingsRO open do
	(
		ddPropertyValue.items = #("true", "false")
		
		
		lvTasks.AllowDrop = true
		lvTasks.GridLines = true
		lvTasks.Checkboxes = true
		lvTasks.AllowColumnReorder = false
		lvTasks.View = lvTasks.View.Details
		lvTasks.LabelEdit = true
		lvTasks.LabelWrap = true
		lvTasks.MultiSelect = true
		lvTasks.FullRowSelect = true
		lvTasks.HideSelection = false
		lvTasks.HeaderStyle = lvTasks.HeaderStyle.Nonclickable
		--lvTasks.HeaderStyle = lvTasks.HeaderStyle.None
		lvTasks.BorderStyle = lvTasks.BorderStyle.FixedSingle
		
		lvTasks.Columns.Add "                       Task"
		(lvTasks.Columns.item 0).width = lvTasks.width - 4
		
		lvProcess.GridLines = true
		lvProcess.Checkboxes = true
		lvProcess.AllowColumnReorder = false
		lvProcess.View = lvProcess.View.Details
		lvProcess.LabelEdit = true
		lvProcess.LabelWrap = true
		lvProcess.MultiSelect = true
		lvProcess.FullRowSelect = true
		lvProcess.HideSelection = false
		lvProcess.HeaderStyle = lvProcess.HeaderStyle.Nonclickable
		lvProcess.BorderStyle = lvProcess.BorderStyle.FixedSingle
		
		lvProcess.Columns.Add "                     Process"
		(lvProcess.Columns.item 0).width = lvProcess.width - 4
		
		
		lvProperties.GridLines = true
		lvProperties.Checkboxes = false
		lvProperties.AllowColumnReorder = false
		lvProperties.View = lvProcess.View.Details
		lvProperties.LabelEdit = false
		lvProperties.LabelWrap = false
		lvProperties.MultiSelect = false
		lvProperties.FullRowSelect = true
		lvProperties.HideSelection = false
		lvProperties.HeaderStyle = lvProperties.HeaderStyle.Nonclickable
		lvProperties.BorderStyle = lvProperties.BorderStyle.FixedSingle
		
		lvProperties.Columns.Add "        Value"
		lvProperties.Columns.Add "     Property"
		
		(lvProperties.Columns.item 0).DisplayIndex = 1
		
		(lvProperties.Columns.item 0).width = (lvProperties.width - 4) / 2
		(lvProperties.Columns.item 1).width = (lvProperties.width - 4) / 2
		
		/*
		edScript.Multiline = true
		edScript.ScrollBars = edScript.Scrollbars.Both
		edScript.WordWrap = false
		edScript.AcceptsReturn = true
		edScript.AcceptsTab = true
		*/
		
		for item in animManSettingsRO.controls do
			if (local tempItem = animManRO.getUIItemDimensions item) != undefined then
				append resizeArray tempItem
		
		
		/*
		if animManRO.showScript == true then
		(
			btnShowScript.text = "<<    S C R I P T"
			btnShowScript.checked = true
			--taskRO.width = 880
		)
		else
		(
			btnShowScript.text = "S C R I P T    >>"
			btnShowScript.checked = false
			--taskRO.width = 485
		)
		*/
		
		updateDialog()
		--gc light:true
	)
	
	
	
	on radLoadMode changed value do
	(
		radSaveMode.state = 0
		taskMode = #import
		
		updateDialog skipMode:true
	)
	
	on radSaveMode changed value do
	(
		radLoadMode.state = 0
		taskMode = #export
		
		updateDialog skipMode:true
	)
	
	
	
	
	on lvTasks MouseDown arg do
	(
		local tempEntry = (lvTasks.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			animManRO.lastDraggedEntry = -1
		else
			animManRO.lastDraggedEntry = tempEntry
		
		--gc light:true
	)
	

	
	on lvTasks MouseMove arg do
	(
		if arg.button == arg.button.left and animManRO.lastDraggedEntry != -1 then
			animManRO.dragList lvTasks taskArray arg
	)
	
	
	
	
	on lvTasks MouseUp arg do
	(
		animManRO.lastDraggedEntry = -1
		animManRO.lastHoveredEntry = -1
		
		onUpdate = true
		lastProcess = 1
		updateProcess()
		
		if arg.button == arg.button.right then
			callTaskMenu()
	)
	
	
	
	
	
	
	on lvProcess MouseDown arg do
	(
		local tempEntry = (lvProcess.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			animManRO.lastDraggedEntry = -1
		else
			animManRO.lastDraggedEntry = tempEntry
		
		--gc light:true
	)
	

	
	on lvProcess MouseMove arg do
	(
		if arg.button == arg.button.left and animManRO.lastDraggedEntry != -1 then
			animManRO.dragList lvProcess taskArray[lastTask].process arg
	)
	
		
	
	
	on lvProcess MouseUp arg do
	(
		animManRO.lastDraggedEntry = -1
		animManRO.lastHoveredEntry = -1
		
		onUpdate = true
		updateScript()
		
		if arg.button == arg.button.right then
			callProcessMenu()
	)
	
	
	
	on lvTasks GotFocus arg do
	(
		if lvTasks.selectedItems.count > 0 then
			taskArray[(lvTasks.selectedItems.item 0).index + 1].name = (lvTasks.selectedItems.item 0).text
	)
	
	
	on lvProcess GotFocus arg do
	(
		if lvProcess.selectedItems.count > 0 then
			if taskArray[lastTask] != undefined then
				taskArray[lastTask].process[(lvProcess.selectedItems.item 0).index + 1].name = (lvProcess.selectedItems.item 0).text
	)
	
	
	
	
	
	
	on lvTasks SelectedIndexChanged arg do
	(
		if onUpdate == false then
		(
			if lvTasks.selectedItems.count > 0 then
				lastTask = (lvTasks.selectedItems.item 0).index + 1
			
			
			for i = 0 to (lvTasks.items.count - 1) do
			(
				local tempNode = lvTasks.items.item i
				
				if i != (lastTask - 1) then
				(
					if taskArray[i + 1].default == true then
						tempNode.backColor = tempNode.backColor.fromARGB 240 255 240
					else
						tempNode.backColor = tempNode.backColor.fromARGB 255 240 240
				)
				else
					tempNode.backColor = tempNode.backColor.fromArgb 230 230 255
				
				
			)
		)
	)
	
	
	
	on lvProcess SelectedIndexChanged arg do
	(
		if lvProcess.selectedItems.count > 0 then
			lastProcess = (lvProcess.selectedItems.item 0).index + 1
		
		
		for i = 0 to (lvProcess.items.count - 1) do
		(
			local tempNode = lvProcess.items.item i
			
			if i != (lastProcess - 1) then
				tempNode.backColor = tempNode.backColor.fromArgb 255 255 255
			else
				tempNode.backColor = tempNode.backColor.fromArgb 230 230 255
		)
	)
	
	
	
	
	
	on lvTasks ItemChecked arg do
	(
		if onUpdate == false then
			taskArray[arg.item.index + 1].active = arg.item.checked
	)
	
	
	on lvProcess ItemChecked arg do
	(
		if onUpdate == false then
			if taskArray[lastTask] != undefined then
				taskArray[lastTask].process[arg.item.index + 1].active = arg.item.checked
	)
	
	
	
	
	
	
	on lvProperties Click arg do
	(
		updatePropertyValue()
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	on edPropertyValue entered arg do
	(
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					if lvProperties.selectedItems.count > 0 then
					(
						if (lvProperties.selectedItems.item 0).index > 0 then
						(
							for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
								for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
									if taskArray[lastTask].process[lastProcess].fnData[i].properties[d].index == ((lvProperties.selectedItems.item 0).index) then
										taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value = "\"" + arg + "\""
							
							updateProperties keepSelection:true
						)
					)
				)
			)
		)
	)
	
	
	on ddPropertyValue selected value do
	(
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					if lvProperties.selectedItems.count > 0 then
					(
						if (lvProperties.selectedItems.item 0).index > 0 then
						(
							for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
								for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
									if taskArray[lastTask].process[lastProcess].fnData[i].properties[d].index == ((lvProperties.selectedItems.item 0).index) then
										taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value = (value == 1) as String
									
							updateProperties keepSelection:true
						)
					)
				)
			)
		)
	)
	
	
	
	
	on spnPropertyValueInteger changed value do
	(
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					if lvProperties.selectedItems.count > 0 then
					(
						if (lvProperties.selectedItems.item 0).index > 0 then
						(
							for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
								for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
									if taskArray[lastTask].process[lastProcess].fnData[i].properties[d].index == ((lvProperties.selectedItems.item 0).index) then
										taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value = value as String
									
							updateProperties keepSelection:true
						)
					)
				)
			)
		)
	)
	
	
	
	on spnPropertyValueFloat changed value do
	(
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					if lvProperties.selectedItems.count > 0 then
					(
						if (lvProperties.selectedItems.item 0).index > 0 then
						(
							for i = 1 to taskArray[lastTask].process[lastProcess].fnData.count do
								for d = 1 to taskArray[lastTask].process[lastProcess].fnData[i].properties.count do
									if taskArray[lastTask].process[lastProcess].fnData[i].properties[d].index == ((lvProperties.selectedItems.item 0).index) then
										taskArray[lastTask].process[lastProcess].fnData[i].properties[d].value = value as String
									
							updateProperties keepSelection:true
						)
					)
				)
			)
		)
	)
	
	
	
	
	
	
	
	
	on btnShowScript changed value do
	(
		animManRO.onUpdate = true
		
		if value == true then
		(
			btnShowScript.text = "<<    S C R I P T"
			animManRO.width += 490
		)
		else
		(
			btnShowScript.text = "S C R I P T    >>"
			animManRO.width -= 490
		)
		
		animManRO.showScript = value
		animManRO.saveSettings mode:#user
		
		animManRO.onUpdate = false
	)
	
	
	
	
	
	
	
	
	on btnSave pressed do
	(
		case taskMode of
		(
			#import:
			(
				animManRO.importArray = #()
				animManRO.importUserArray = #()
				
				for i = 1 to taskArray.count do
				(
					if taskArray[i].default == true then
						append animManRO.importArray taskArray[i]
					else
						append animManRO.importUserArray taskArray[i]
				)
				
				animManRO.importTemplateArray = templateArray
				animManRO.importSettingsPath = filePath
				animManRO.updateImportList()
				animManRO.ddLoad.selection = lastTask + 1
			)
			#export:
			(
				animManRO.exportArray = #()
				animManRO.exportUserArray = #()
				
				for i = 1 to taskArray.count do
				(
					if taskArray[i].default == true then
						append animManRO.exportArray taskArray[i]
					else
						append animManRO.exportUserArray taskArray[i]
				)
				
				animManRO.exportTemplateArray = templateArray
				animManRO.exportSettingsPath = filePath
				animManRO.updateExportList()
				animManRO.ddSave.selection = lastTask
			)
		)
		
		
		--try destroyDialog taskRO catch()
		
		if (queryBox "Save Settings to File?" title:("Save " + taskMode as String + " Settings")) == true then
		(
			animManRO.saveSettings mode:taskMode
			
			local foundUser = false
			
			for i = 1 to taskArray.count do
				if taskArray[i].default == false then
					foundUser = true
			
			if foundUser == true then
			(
				if taskMode == #import then
					animManRO.saveSettings mode:#importUser noCheckout:true
				
				if taskMode == #export then
					animManRO.saveSettings mode:#exportUser noCheckout:true
			)
		)
		
		taskMode = #none
			
		--gc light:true
	)
	
	
	on btnOpen pressed do
	(
		local secTaskMode = "import"
		if taskMode == #export then
			secTaskMode = "export"
		
		local loadUser = false
		
		if queryBox ("Load User " + secTaskMode + " Settings?") == true then
		(
			local newSettings = animManRO.loadSettings mode:((secTaskMode + "User") as Name) writeTo:true
			loadUser = true
		)
		
		local newSettings = animManRO.loadSettings mode:taskMode writeTo:true
		
		if loadUser == false then
		(
			templateArray = newSettings[1]
			taskArray = newSettings[2]
			filePath = newSettings[3]
		)
		else
		(
			join templateArray newSettings[1]
			join taskArray newSettings[2]
		)
		
		
		if taskArray.count > 0 then
			lastTask = 1
		
		if lastTask > 0 then
			if taskArray[lastTask].process.count > 0 then
				lastProcess = 1
		
		onUpdate = true
		
		
		updateProcessTemplates()
		updateTasks()
			
		--gc light:true
	)
	
	on btnSaveAs pressed do
	(
		local secTaskMode = "import"
		if animManRO.taskMode == #export then
			secTaskMode = "export"
		
		local foundUser = false
		local foundDefault = false
		
		for i = 1 to taskArray.count do
		(
			if taskArray[i].default == false then
				foundUser = true
			else
				foundDefault = true
		)
		
		if foundDefault == true then
		(
			if queryBox ("Save Default " + secTaskMode + " Settings?") == true then
			(
				local defaultArray = #()
				
				for i = 1 to taskArray.count do
					if taskArray[i].default == true then
						append defaultArray taskArray[i]
					
				animManRO.saveSettings mode:taskMode saveAs:true custom:#(templateArray,defaultArray)
			)
		)
			
		
		
		
		if foundUser == true then
		(
			if queryBox ("Save User " + secTaskMode + " Settings?") == true then
			(
				local userArray = #()
				
				for i = 1 to taskArray.count do
					if taskArray[i].default == false then
						append userArray taskArray[i]
				
				animManRO.saveSettings mode:((secTaskMode + "User") as Name) saveAs:true custom:#(#(),userArray)
			)
		)
	)
	
	
	
	

	
	
	
	
	on btnScriptOpen pressed do
	(
		local tempPath = ""
		
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					tempPath = taskArray[lastTask].process[lastProcess].script.path
			
					local openPath = getOpenFileName caption:"Open Script" filename:tempPath
					if openPath != undefined then
					(
						local ret = "\r\n"
						local tempString = ""
			
						local tempStream = openFile openPath mode:"r"
						
						while (eof tempStream) != true do
							tempString += (readLine tempStream) + ret
						
						close tempStream
		
						edScript.text = tempString
						
						
						taskArray[lastTask].process[lastProcess].script.path = openPath
					)
				)
			)
		)
		
		--gc light:true
	)
	
	
	
	
	
	on btnScriptSave pressed do
	(
		local tempPath = ""
		
		if lastTask > 0 and lastProcess > 0 then
		(
			if taskArray[lastTask] != undefined then
			(
				if taskArray[lastTask].process[lastProcess] != undefined then
				(
					tempPath = taskArray[lastTask].process[lastProcess].script.path
		
					local savePath = getSaveFileName caption:"Save Script" filename:tempPath
					if savePath != undefined then
					(
						local oldString = edScript.text as stringStream
						local newString = ""
						
						while eof oldString == false do
						(
							newString += readLine oldString
							newString = subString newString 2 (newString.count - 1)
							newString += "\n"
						)
						
						
						local tempStream = openFile savePath mode:"w"
						format edScript.text to:tempStream
						close tempStream
						
						taskArray[lastTask].process[lastProcess].script.path = savePath
					)
				)
			)
		)
		
		--gc light:true
	)
	
	
	
	
	on btnScriptClear pressed do
	(
		if edScript.text != "" then
			if queryBox "Clear the script?" title:"Script" == true then
				edScript.text = ""
	)
)

try addSubRollout animManRO.subRO animManSettingsRO rolledUp:false catch()