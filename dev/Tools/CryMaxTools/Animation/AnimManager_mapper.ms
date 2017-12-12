try destroyDialog mapNodesRO catch()

rollout mapNodesRO "Retarget Nodes" width:488 height:336
(
	local foundMap = 0
	local mapList = #()
	local oldTMArray = #()
	local lastDraggedEntry = -1
	local lastHoveredEntry = -1
	local dragList = #()
	
	local nodeList = #()
	
	groupBox gbMappedBones " Mapped " pos:[8,8] width:320 height:288
	listBox lbTargetBones "" pos:[16,24] width:304 height:18 visible:false
	dotNetControl lvMappedBones "System.Windows.Forms.ListView" pos:[16,24] height:240 width:304
	
	button btnRefreshMap "Refresh" pos:[16,269] width:57 height:20
	button btnClearMap "Clear" pos:[271,269] width:50 height:20
	
	
	button btnMap "<<" pos:[344,107] width:32 height:32
	button btnUnmap ">>" pos:[344,165] width:32 height:32
	
	
	groupBox gbTargetBones " Target " pos:[392,8] width:198 height:288
	listBox lbTargetBones2 "" pos:[400,24] width:182 height:18 visible:false
	dotNetControl lvTargetBones "System.Windows.Forms.ListView" pos:[400,24] height:240 width:182
	editText edTargetMask "" pos:[396,270] width:186 height:16
	
	groupBox gbSourceBones " Source " pos:[392+200-3,8] width:198 height:288
	listBox lbSourceBones "" pos:[400+200-3,24] width:182 height:18 visible:false
	dotNetControl lvSourceBones "System.Windows.Forms.ListView" pos:[400+200-3,24] height:240 width:182
	editText edSourceMask "" pos:[396+200-3,270] width:186 height:16
	
	
	
	button btnAutoMap "Auto Match" pos:[16+62,269] width:189 height:20
	button btnLoad "Load" pos:[203+190,304+20] width:95 height:20
	button btnSave "Save" pos:[265+190+35,304+20] width:95 height:20
	
	progressbar pbAutoMapCurrent "" pos:[332+60,304+25-23] width:(56+340) height:5
	progressbar pbAutoMapOverall "" pos:[332+60,314+23-26] width:(56+340) height:5
	
	button btnCancel "Cancel" pos:[396+200,304+20] width:95 height:20
	button btnAccept "Accept" pos:[493+200,304+20] width:95 height:20
	
	
	
	
	
	groupBox gbRetarget " Retarget " pos:[8,330-30] width:74 height:45
	checkbutton btnUpdate "Live" pos:[15,347-30] width:60 height:20
	
	groupBox gbUpdate " Update " pos:[81,330-30] width:121 height:45
	
	button btnUpdateSel "Sel" pos:[90,347-30] width:50 height:20
	button btnUpdateAll "All" pos:[145,347-30] width:50 height:20
	
	groupBox gbPosRot "" pos:[201,330-30] width:(389-260) height:45
	
	label labPos "Pos:" pos:[215,350-30]
	checkBox chkPos "" pos:[240,350-30] width:10
	edittext edPos "" pos:[248,348-30] fieldWidth:142 visible:false
	
	label labRot "Rot:" pos:[403-200+65,350-30]
	checkBox chkRot "" pos:[428-200+65,350-30] width:10
	edittext edRot "" pos:[436,348-30] fieldWidth:142 visible:false
	
	
	


	
	function isSpecialSign char =
	(
		local index = bit.charAsInt char
		
		if index < 48 or index > 122 or (index < 65 and index > 57) or (index > 90 and index < 97) then
			return true
		
		--gc light:true
		
		return false
	)



	function getMatchPoints str1 str2 =
	(
		local convertedStr1 = ""
		local convertedStr2 = ""
		
			--// remove from each string all special signs
		
		for i = 1 to (if str1.count > str2.count then str1.count else str2.count) do
		(
			if i <= str1.count then
				if isSpecialSign str1[i] == false then
					convertedStr1 += str1[i]
			
			if i <= str2.count then
				if isSpecialSign str2[i] == false then
					convertedStr2 += str2[i]
		)
		
			--// convert the strings to lower case
		
		convertedStr1 = cryMaxTools.basic.str.lower convertedStr1
		convertedStr2 = cryMaxTools.basic.str.lower convertedStr2
		
		
		local missMatch = 0
		local startMatchStr1 = 0
		local startMatchStr2 = 0
		
		local str1 = convertedStr1
		local str2 = convertedStr2
		
			--// if str2 is bigger than str1, switch them
		
		if convertedStr2.count > convertedStr1.count then
		(
			str1 = convertedStr2
			str2 = convertedStr1
		)
		
		local foundMatch = false
		
		
			--// go through each char and check, if they are equal
			--// check also, if the next 2 chars are equal too.
			--// this is quite a limitation, as it means it only works correctly, if you have at least 3 chars equal in each name you want to check
		
		for i = 1 to str1.count do
		(
			if foundMatch == false then
			(
				for f = 1 to str2.count do
				(
					if foundMatch == false then
					(
						if str1[i] == str2[f] then
						(
							if i < (str1.count - 1) and f < (str2.count - 1) then
							(
								if (str1[i+1] == str2[f+1]) and (str1[i+2] == str2[f+2]) then
								(
									startMatchStr1 = i
									startMatchStr2 = f
									
									foundMatch = true
								)
							)
						)
					)
				)
			)
		)
		
			--// switch, if found start point difference differs from before
		
		if (str2.count - startMatchStr2) > (str1.count - startMatchStr1) then
		(
			local tempStr = str1
			str1 = str2
			str2 = tempStr
			
			local tempStart = startMatchStr1
			startMatchStr1 = startMatchStr2
			startMatchStr2 = tempStart
		)
		
		
			--// check from each start point onwards the rest of the nodes.
			--// str1 should have the bigger rest to check than str2
			--// if the counter exceeds str2's range, than there are probably unequal chars in the rest of the str1's chars
			--// if there is any missmatch in chars, increment missMatch
		
		for i = 1 to (str1.count - startMatchStr1) do
		(
			if str2[startMatchStr2 + i] != undefined then
			(
				if str1[startMatchStr1 + i] != str2[startMatchStr2 + i] then
					missMatch += 1
			)
			else
				missMatch += 1
		)
		
		--gc light:true
		
		return (missMatch == 0)
	)
	
	



	function getMapping fixTgt:undefined currentProgress:undefined overallProgress:undefined =
	(
		local checkedArray = #{}
		--mapList = #()
		
		if currentProgress != undefined then
			currentProgress.value = 0.0
		
		if overallProgress != undefined then
			overallProgress.value = 0.0
		
		local maxCount = 0
		
		for i = 1 to nodeList.count do
		(
			if nodeList[i].targetVis == true or nodeList[i].sourceVis == true then
				maxCount += 1
		)
		
		local maxCounter = 0
		
			--// checks each name in array1
		for srcI = 1 to nodeList.count do
		(
			if nodeList[srcI].targetVis == true then
			(
				maxCounter += 1
				
				--nodeList[srcI].sourceMapID = 0
				nodeList[srcI].sourceMap = false
				--nodeList[srcI].targetMapID = 0
				nodeList[srcI].targetMap = false
				
					--// checks each name in array2
				for cmpI = 1 to nodeList.count do
				(
					if nodeList[cmpI].sourceVis == true then
					(
						if checkedArray[cmpI] == false and nodeList[srcI].nodeName != nodeList[cmpI].nodeName then
						(
							if getMatchPoints nodeList[srcI].nodeName nodeList[cmpI].nodeName == true then
							(
								--nodeList[srcI].sourceMapID = cmpI
								nodeList[srcI].sourceMap = true
								--nodeList[srcI].targetMapID = srcI
								nodeList[srcI].targetMap = false
								
								nodeList[srcI].sourceName = nodeList[cmpI].nodeName
								
								--nodeList[cmpI].targetMapID = srcI
								nodeList[cmpI].targetMap = true
								--nodeList[cmpI].sourceMapID = cmpI
								nodeList[cmpI].sourceMap = false
								
								checkedArray[cmpI] = true
								
								appendIfUnique mapList nodeList[srcI]
								
								exit
							)
						
							
							if currentProgress != undefined then
								currentProgress.value = (cmpI * 100.) / nodeList.count
						)
					)
				)
				
				if overallProgress != undefined then
					overallProgress.value = (maxCounter * 100.) / maxCount
			)
		)
		
		if overallProgress != undefined then
			overallProgress.value = 100
		
		if currentProgress != undefined then
			currentProgress.value = 100
		
		--gc light:true
	)

	
	
	
	
	
	
	
	
	
	
	function getExtendedString srcStr maxExtent =
	(
		local strExtent = (getTextExtent srcStr)[1]
		local returnString = srcStr
		
		while strExtent < maxExtent do
		(
			returnString += " "
			strExtent = (getTextExtent returnString)[1]
		)
		
		--gc light:true
		
		return returnString
	)
	
	
	
	
	function getLongestStringExtent list:undefined =
	(
		local longestExtent = 0
		local usedList = nodeList
		
		if list == #mapTarget or list == #mapSource then
			usedList = mapList
		
		
		
		for i = 1 to usedList.count do
		(
			local tempExtent = 0
			local visible = false
			case list of
			(
				#target:		if usedList[i].targetVis == true then	visible = true
				#source:	if usedList[i].sourceVis == true then	visible = true
				#mapTarget:		visible = true
				#mapSource:		visible = true
			)
			
			if visible == true then
			(
				if list == #mapTarget or list == #mapSource then
				(
					if list == #mapSource then
						tempExtent = (getTextExtent mapList[i].sourceName)[1]
					else
						tempExtent = (getTextExtent usedList[i].nodeName)[1]
				)
				else
					tempExtent = (getTextExtent usedList[i].nodeName)[1]
				
				
				
				if tempExtent > longestExtent then
					longestExtent = tempExtent
			)
		)
		
		if longestExtent > 0 then
			longestExtent += 15
		
		--gc light:true
		
		return longestExtent
	)
	
	
	
	
	function getTargetSelIndex =
	(
		local counter = 0
		local selIndex = 0
		
		if lvMappedBones.selectedItems.count > 0 then
			selIndex = (lvMappedBones.selectedItems.item 0).index + 1
		
		return selIndex
	)
	
	
	
	
	function updatePosRot =
	(
		local index = getTargetSelIndex()
			
		if index > 0 then
		(
			chkPos.checked = mapList[index].posActive
			chkRot.checked = mapList[index].rotActive
				
			edPos.text = mapList[index].pos as String
			edRot.text = mapList[index].rot as String
		)
		else
		(
			chkPos.checked = false
			chkRot.checked = false
				
			edPos.text = ""
			edRot.text = ""
		)
	)
	
	
	
	
	
	
	function updateList list =
	(
		case list of
		(
			#map:
			(
				lvMappedBones.BeginUpdate()
				lvMappedBones.items.clear()
				
				for i = 1 to mapList.count do
				(
					local tempEntry = lvMappedBones.items.add mapList[i].nodeName mapList[i].nodeName
					if mapList[i].sourceMap == true then
						tempEntry.SubItems.Add mapList[i].sourceName
				)
				
				
				(lvMappedBones.columns.item 0).width = getLongestStringExtent list:#mapTarget
				(lvMappedBones.columns.item 1).width = getLongestStringExtent list:#mapSource
				
				
				
				if lvMappedBones.items.count > 0 then
				(
					lvMappedBones.selectedItems.clear()
					(lvMappedBones.items.item 0).selected = true
				)
				
				lvMappedBones.EndUpdate()
				
				updatePosRot()
			)
			
			#target:
			(
				local tempFilter = filterString edTargetMask.text " "
				local startIndex = 1
				local showAll = tempFilter[1] == "*"
				local prefixFound = false
				if showAll == true then
					startIndex = 2
				
				local prefixFound = false
				
				
				if tempFilter.count > (startIndex - 1) then
				(
					for i = 1 to nodeList.count do
						if (subString nodeList[i].nodeName 1 tempFilter[startIndex].count) == tempFilter[startIndex] then
							prefixFound = true
				)
				
					
				for i = 1 to nodeList.count do
				(
					local matchFound = true
					
					nodeList[i].targetVis = false
					
					if tempFilter.count > 0 then
					(
						for f = startIndex to tempFilter.count do
						(
							if f == startIndex then
							(
								if prefixFound == true then
									if (subString nodeList[i].nodeName 1 tempFilter[f].count) != tempFilter[f] then
										matchFound = false
							)
							
							if findString nodeList[i].nodeName tempFilter[f] == undefined then
								matchFound = false
						)
					)
					
					
					if matchFound == true then
					(
						if showAll == true then
							nodeList[i].targetVis = true
						else
						(
							if nodeList[i].targetMap == false and nodeList[i].sourceMap == false then
								nodeList[i].targetVis = true
						)
					)
				)
				
				lvTargetBones.BeginUpdate()
				
				lvTargetBones.items.clear()
				
				for i = 1 to nodeList.count do
				(
					if nodeList[i].targetVis == true and nodeList[i].targetMap == false then
					(
						local tempEntry = lvTargetBones.items.add nodeList[i].nodeName nodeList[i].nodeName
						--if nodeList[i].sourceMap == false then
							--tempEntry.SubItems.Add nodeList[nodeList[i].sourceMapID].nodeName
					)
				)
				
				
				(lvTargetBones.columns.item 0).width = getLongestStringExtent list:#target
				
				
				
				if lvTargetBones.items.count > 0 then
				(
					lvTargetBones.selectedItems.clear()
					(lvTargetBones.items.item 0).selected = true
				)
				
				lvTargetBones.EndUpdate()
			)
			
			#source:
			(
				local listArray = #()
				sourceListVis = #()
				
				local tempFilter = filterString edSourceMask.text " "
				local startIndex = 1
				local showAll = tempFilter[1] == "*"
				local prefixFound = false
				if showAll == true then
					startIndex = 2
				
				
				if tempFilter.count > (startIndex - 1) then
				(
					for i = 1 to nodeList.count do
						if (subString nodeList[i].nodeName 1 tempFilter[startIndex].count) == tempFilter[startIndex] then
							prefixFound = true
				)


				for i = 1 to nodeList.count do
				(
					local matchFound = true
					
					nodeList[i].sourceVis = false
					
					if tempFilter.count > 0 then
					(
						for f = startIndex to tempFilter.count do
						(
							if f == startIndex then
							(
								if prefixFound == true then
									if (subString nodeList[i].nodeName 1 tempFilter[f].count) != tempFilter[f] then
										matchFound = false
							)
							
							if findString nodeList[i].nodeName tempFilter[f] == undefined then
								matchFound = false
						)
					)
					
					
					if matchFound == true then
					(
						if showAll == true then
							nodeList[i].sourceVis = true
						else
						(
							if nodeList[i].targetMap == false and nodeList[i].sourceMap == false then
								nodeList[i].sourceVis = true
						)
					)
				)
				
				
				
				lvSourceBones.BeginUpdate()
				
				lvSourceBones.items.clear()
				
				for i = 1 to nodeList.count do
				(
					if nodeList[i].sourceVis == true and nodeList[i].sourceMap == false then
					(
						local tempEntry = lvSourceBones.items.add nodeList[i].nodeName nodeList[i].nodeName
						--if nodeList[i].sourceMap == false then
							--tempEntry.SubItems.Add nodeList[nodeList[i].sourceMapID].nodeName
					)
				)
				
				
				
				(lvSourceBones.columns.item 0).width = getLongestStringExtent list:#source
				
				if lvSourceBones.items.count > 0 then
				(
					lvSourceBones.selectedItems.clear()
					(lvSourceBones.items.item 0).selected = true
				)
				
				lvSourceBones.EndUpdate()
			)
		)
		
		--gc light:true
	)
	
	
	
	
	function updateNodePosRot index:undefined =
	(
		if mapList.count > 0 then
		(
			with redraw off ; undo off
			(
				for i = 1 to mapList.count do
				(
					if index != undefined then
						if index != i then
							continue
							
					local targetNode = getNodeByName mapList[i].nodeName
					local sourceNode = getNodeByName mapList[i].sourceName
					
					local newPos = [0,0,0]
					local newRot = quat 1
					local newTM = matrix3 1
					
						
					if targetNode != undefined and sourceNode != undefined then
					(
						newPos = (targetNode.transform * inverse sourceNode.transform).pos
						newRot = inverse (targetNode.transform * inverse sourceNode.transform).rotation
						newTM = targetNode.transform * inverse sourceNode.transform
					)
					
					mapList[i].pos = newPos
					mapList[i].rot = newRot
					mapList[i].tm = newTM
				)
			)
		)
		
		--gc light:true
	)
	
	
	
	
	
	function applyNodePosRot =
	(
		with undo off
		(
			for i = 1 to mapList.count do
			(
				local targetNode = getNodeByName mapList[i].nodeName
				local sourceNode = getNodeByName mapList[i].sourceName
				
				if targetNode != undefined and sourceNode != undefined then
				(
					local newTM = targetNode.transform
					
					newTM.rotation = inverse mapList[i].rot
					newTM.pos = mapList[i].pos
					newTM = newTM * sourceNode.transform
					
					
						--// Biped \\--
					with animate on
					(
						if targetNode.classID[1] == 37157 then
						(
							if mapList[i].rotActive == true then
								biped.setTransform targetNode #rotation newTM.rotation true
							if mapList[i].posActive == true then
							(
								biped.setTransform targetNode #pos newTM.pos true
								biped.setTransform targetNode #rotation newTM.rotation true
							)
						)
						else
						(
							local tempTM = targetNode.transform
							try
							(
								if mapList[i].rotActive == true then
									tempTM.rotation = newTM.rotation
								if mapList[i].posActive == true then
									tempTM.rotation = newTM.pos
							)catch()
							
							targetNode.transform = tempTM
						)
					)
				)
			)
		)
		
		--gc light:true
	)
	
	
	function applyNodeOldPosRot =
	(
		with undo off
		(
			if oldTMArray.count > 0 then
			(
				for i = 1 to mapList.count do
				(
					local targetNode = getNodeByName mapList[i].nodeName
					if targetNode != undefined then
					(
						try
						(
							if targetNode.classID[1] == 37157 then
							(
								biped.setTransform targetNode #rotation oldTMArray[i].rotation true
								biped.setTransform targetNode #pos oldTMArray[i].pos true
							)
							else
								targetNode.transform = oldTMArray[i]
						)catch()
					)
				)
				
				oldTMArray = #()
			)
		)
	)
	
	
	
	function clearCurrentMapping skipAsk:undefined skipVis:false =
	(
		if mapList.count > 0 then
		(
			local query = true
			
			if skipAsk == undefined then
				local query = queryBox "Remove Mapping?"
				
			if query == true then
			(
				for i = 1 to mapList.count do
				(
					local targetID = 0
					local sourceID = 0
					
					for d = 1 to nodeList.count do
					(
						if mapList[i].nodeName == nodeList[d].nodeName then
							targetID = d
						
						if mapList[i].sourceName == nodeList[d].nodeName then
							sourceID = d
					)
					
					if sourceID > 0 then
					(
						nodeList[sourceID].targetMap = false
						nodeList[sourceID].sourceMap = false
						nodeList[sourceID].sourceName = ""
						
						nodeList[sourceID].rot = (quat 1)
						nodeList[sourceID].pos = [0,0,0]
					)
					
					if targetID > 0 then
					(
						nodeList[targetID].targetMap = false
						nodeList[targetID].sourceMap = false
					)
				)
				
				mapList = #()
			)
		)
	)
	
	
	
	
	
	
	
	function saveMapping filepath =
	(
		local xmlFile = dotNetObject "system.xml.xmlDocument"
		local fileRoot = xmlFile.createElement "RetargetNodesFile"
		
		local settingsRoot =  xmlFile.createElement "Settings"
		settingsRoot.setAttribute "TargetMask" edTargetMask.text
		settingsRoot.setAttribute "SourceMask" edSourceMask.text
		
		local mappedNodes = xmlFile.createElement "Mapping"
	
		for i = 1 to mapList.count do
		(
			local target = xmlFile.createElement "Node"
			target.setAttribute "Target" mapList[i].nodeName
			target.setAttribute "Source" mapList[i].sourceName
			target.setAttribute "posActive" (mapList[i].posActive as String)
			target.setAttribute "pos" (mapList[i].pos as String)
			target.setAttribute "rotActive" (mapList[i].rotActive as String)
			target.setAttribute "rot" (mapList[i].rot as String)
			target.setAttribute "tm" (mapList[i].tm as String)
			
			mappedNodes.appendChild target
		)
	
		fileRoot.appendChild settingsRoot
		fileRoot.appendChild mappedNodes
		
		xmlFile.appendChild fileRoot
		
		try (deleteFile filepath) catch()
		
		cryMaxTools.basic.perforceMan.open filepath
	
		xmlFile.save filepath
		
		--gc light:true
	)
	
	
	
	
	
	
	
	
	function loadMapping filepath =
	(
		if (getFiles filePath).count > 0 then
		(
			clearCurrentMapping skipAsk:true
			
			local xmlFile = dotNetObject "system.xml.xmlDocument"
	
			xmlFile.load filepath
			
			local fileRoot = xmlFile.documentElement
			
			local settings = fileRoot.childNodes.itemOf[0]
			edTargetMask.text = settings.attributes.itemOf[0].value
			edSourceMask.text = settings.attributes.itemOf[1].value
			local nodes = fileRoot.childNodes.itemOf[1]
			
			
			for i = 0 to (nodes.childNodes.count - 1) do
			(
				local target = nodes.childNodes.itemOf[i]
				local targetName = target.attributes.itemOf[0].value
				local sourceName = target.attributes.itemOf[1].value
				
				local targetID = 0
				local sourceID = 0
				
				for i = 1 to nodeList.count do
				(
					if nodeList[i].nodeName == targetName then
						targetID = i
					
					if nodeList[i].nodeName == sourceName then
						sourceID = i
				)
				
				
				if targetID > 0 and sourceID > 0 then
				(
					nodeList[targetID].sourceMap = sourceID > 0
					nodeList[sourceID].targetMap = targetID > 0
					nodeList[targetID].sourceName = sourceName
					nodeList[targetID].posActive = execute(target.attributes.itemOf[2].value)
					nodeList[targetID].pos = execute(target.attributes.itemOf[3].value)
					nodeList[targetID].rotActive = execute(target.attributes.itemOf[4].value)
					nodeList[targetID].rot = execute(target.attributes.itemOf[5].value)
					
					appendIfUnique mapList nodeList[targetID]
				)
			)
			
			updateList #target
			updateList #source
			updateList #map
		)
		
		--gc light:true
	)
	
	
	
	
	
	
	
	
	
	
	
	
	on mapNodesRO open do
	(
		local tempNodeArray = cryMaxTools.anim.functions.op.sortRootChildren (objects as Array) dependent:false
		
		
		for i = 1 to tempNodeArray.count do
			append nodeList (cryMaxToolsAnimMapStruct nodeName:tempNodeArray[i].name)
		
		mapList = cryMaxTools.anim.vars.map
		
		
		
		for i = 1 to nodeList.count do
		(
			for d = 1 to mapList.count do
			(
				if nodeList[i].nodeName == mapList[d].nodeName then
					nodeList[i].sourceMap = true
				
				if nodeList[i].nodeName == mapList[d].sourceName then
					nodeList[i].targetMap = true
			)
		)
		
		
		
		lvMappedBones.GridLines = true
		lvMappedBones.AllowColumnReorder = false
		lvMappedBones.View = lvMappedBones.View.Details
		lvMappedBones.LabelEdit = false
		lvMappedBones.LabelWrap = false
		lvMappedBones.MultiSelect = true
		lvMappedBones.FullRowSelect = true
		lvMappedBones.HideSelection = false
		
		
		lvMappedBones.Columns.Add "Target"
		lvMappedBones.Columns.Add "Source"
		(lvMappedBones.Columns.item 0).width = 150
		(lvMappedBones.Columns.item 1).width = 0
		
		
		
		lvTargetBones.GridLines = true
		lvTargetBones.AllowColumnReorder = false
		lvTargetBones.View = lvTargetBones.View.Details
		lvTargetBones.LabelEdit = false
		lvTargetBones.LabelWrap = false
		lvTargetBones.MultiSelect = true
		lvTargetBones.FullRowSelect = true
		lvTargetBones.HideSelection = false
		lvTargetBones.HeaderStyle = lvTargetBones.HeaderStyle.None
		
		lvTargetBones.Columns.Add ""
		(lvTargetBones.Columns.item 0).width = 150
		
		
		lvSourceBones.GridLines = true
		lvSourceBones.AllowColumnReorder = false
		lvSourceBones.View = lvSourceBones.View.Details
		lvSourceBones.LabelEdit = false
		lvSourceBones.LabelWrap = false
		lvSourceBones.MultiSelect = true
		lvSourceBones.FullRowSelect = true
		lvSourceBones.HideSelection = false
		lvSourceBones.HeaderStyle = lvSourceBones.HeaderStyle.None
		
		lvSourceBones.Columns.Add ""
		(lvSourceBones.Columns.item 0).width = 150
		
		updateList #target
		updateList #source
		updateList #map
		
		--gc light:true
	)
	
	
	on mapNodesRO close do
	(
		applyNodeOldPosRot()
	)
	
	
	
	
	on btnUpdateSel pressed do
	(
		updateNodePosRot index:(getTargetSelIndex())
		
		if btnUpdate.checked == true then
			applyNodePosRot()
		
		updatePosRot()
	)
	
	
	on btnUpdateAll pressed do
	(
		updateNodePosRot()
		
		if btnUpdate.checked == true then
			applyNodePosRot()
		
		updatePosRot()
	)
	
	
	
	on lvMappedBones MouseDown arg do
	(
		local tempEntry = (lvMappedBones.hitTest arg.x arg.y).item
		
		if tempEntry == undefined then
			lastDraggedEntry = -1
		else
			lastDraggedEntry = tempEntry
		
		--gc light:true
	)
	

	
	on lvMappedBones MouseMove arg do
	(
		if arg.button == arg.button.left and lastDraggedEntry != -1 then
			dragList lvMappedBones mapList arg
	)
	
		
	
	
	on lvMappedBones MouseUp arg do
	(
		lastDraggedEntry = -1
		lastHoveredEntry = -1
		
		onUpdate = true
		updatePosRot()
	)
	
	
	
	on btnCancel pressed do
	(
		if queryBox "Save before closing?" == true then
		(
			local savePath = getSaveFileName caption:"Save Retarget Script" types:"CryMaxTools RetargetScript (*.crs)|*.crs"
			if savePath != undefined then
				saveMapping savePath
		)
		
		try destroyDialog mapNodesRO catch()
		
		--gc light:true
	)
	
	on btnAccept pressed do
	(
		cryMaxTools.anim.vars.map = mapList
		
		try destroyDialog mapNodesRO catch()
		
		--gc light:true
	)
	
	
	
	on edTargetMask changed value do
		updateList #target
	
	
	on edSourceMask changed value do
		updateList #source
	
	
	
	on btnRefreshMap pressed do
	(
		local safeList = #()
							
		for i = 1 to mapList.count do
			append safeList mapList[i]
		
			
		nodeList = #()
		mapList = #()
			
			
		for i = 1 to objects.count do
			append nodeList (cryMaxToolsAnimMapStruct nodeName:objects[i].name)
		
		for i = 1 to safeList.count do
		(
			local foundID = false
			
			
			for f = 1 to nodeList.count do
			(
				if safeList[i].nodeName == nodeList[f].nodeName then
				(
					if foundID == false then
					(
						for d = 1 to nodeList.count do
						(
							if safeList[i].sourceName == nodeList[d].nodeName then
							(
								nodeList[f].sourceMap = true
								nodeList[d].targetMap = true
								append mapList safeList[i]
								foundID = true
							
								exit
							)
						)
					)
				)
			)
		)
			
		updateList #target
		updateList #source
		updateList #map
		--gc light:true
	)
	
	
	
	
	
	on btnAutoMap pressed do
	(
		local ready = true
		
		
		if nodeList.count > 0 then
		(
			local foundMap = mapList.count > 0
			
			if foundMap == true then
				ready = (queryBox "Overwrite existing Map?")
		)
				
		if ready == true then
		(
			clearCurrentMapping skipAsk:true skipVis:true
			getMapping overallProgress:pbAutoMapOverall currentProgress:pbAutoMapCurrent fixTgt:edTargetMask.text fixSrc:edSourceMask.text
			
			updateList #target
			updateList #source
			updateList #map
		)
		
		--gc light:true
	)
	
	
	on btnClearMap pressed do
	(
		clearCurrentMapping()
		
		updateList #target
		updateList #source
		updateList #map
	)
	
	
	
	
	
	on btnMap pressed do
	(
		local maxSel = lvSourceBones.selectedItems.count
		if lvTargetBones.selectedItems.count > maxSel then
			maxSel = lvTargetBones.selectedItems.count
		
		
		local selSourceArray = #()
		local selTargetArray = #()
		
		
		for i = 0 to (maxSel - 1) do
		(
			try append selSourceArray (lvSourceBones.selectedItems.item i).index catch()
			try append selTargetArray (lvTargetBones.selectedItems.item i).index catch()
		)
		
		
		sort selSourceArray
		sort selTargetArray
		
		
		for i = 1 to maxSel do
		(
			if selSourceArray[i] != undefined and selTargetArray[i] != undefined then
			(
				local targetID = 0
				local sourceID = 0
				
				local targetName = (lvTargetBones.items.item selTargetArray[i]).text
				local sourceName = (lvSourceBones.items.item selSourceArray[i]).text
				
				for i = 1 to nodeList.count do
				(
					if nodeList[i].nodeName == targetName then
						targetID = i
					
					if nodeList[i].nodeName == sourceName then
						sourceID = i
				)
				
				if targetID > 0 and sourceID > 0 then
				(
					nodeList[targetID].sourceMap = true
					nodeList[sourceID].targetMap = true
					
					nodeList[targetID].sourceName = nodeList[sourceID].nodeName
					
					appendIfUnique mapList nodeList[targetID]
				)
			)
		)
		
				
				
		try local tempTargetSel = (lvTargetBones.selectedItems.item 0).index catch()
		try local tempSourceSel = (lvSourceBones.selectedItems.item 0).index catch()
		
		updateList #target
		updateList #source
		updateList #map
		
		lvTargetBones.selectedItems.clear()
		lvSourceBones.selectedItems.clear()
		
		if lvTargetBones.items.count >= (tempTargetSel + 1) then
			(lvTargetBones.items.item tempTargetSel).selected = true
		
		if lvSourceBones.items.count >= (tempSourceSel + 1) then
			(lvSourceBones.items.item tempSourceSel).selected = true
		
		--gc light:true
	)
	
	
	
	
	on btnUnmap pressed do
	(
		local tempTargetSel = 0
		local tempSourceSel = 0
		
		try tempTargetSel = (lvTargetBones.selectedItems.item 0).index catch()
		try tempSourceSel = (lvSourceBones.selectedItems.item 0).index catch()
		
		for h = (lvMappedBones.selectedItems.count - 1) to 0 by -1 do
		(
			local targetID = 0
			local sourceID = 0
			
			local targetName = (lvMappedBones.selectedItems.item h).text
			
			for i = 1 to nodeList.count do
			(
				if nodeList[i].nodeName == targetName then
					targetID = i
			)
			
			
			if targetID > 0 then
			(
				for i = 1 to nodeList.count do
					if nodeList[i].nodeName == nodeList[targetID].sourceName then
						sourceID = i
				
				nodeList[targetID].sourceMap = false
				
				if sourceID > 0 then
					nodeList[sourceID].targetMap = false
				
				
				deleteItem mapList ((lvMappedBones.selectedItems.item h).index + 1)
			)
		)
		
		updateList #target
		updateList #source
		updateList #map
		
		(lvMappedBones.columns.item 0).width = getLongestStringExtent list:#mapTarget
		(lvMappedBones.columns.item 1).width = getLongestStringExtent list:#mapSource
		
		lvTargetBones.selectedItems.clear()
		lvSourceBones.selectedItems.clear()
		
		if lvTargetBones.items.count > tempTargetSel then
			(lvTargetBones.items.item tempTargetSel).selected = true
		
		if lvSourceBones.items.count > tempSourceSel then
			(lvSourceBones.items.item tempSourceSel).selected = true
		
		--gc light:true
	)
	
	
	
	on btnUpdate changed value do
	(
		btnAutoMap.enabled = not value
		btnLoad.enabled = not value
		btnSave.enabled = not value
		
		
		if value == true then
		(	
			for i = 1 to nodeList.count do
				if nodeList[i].sourceMap == false then
					oldTMArray[i] = (getNodeByName nodeList[i].nodeName).transform
			
			applyNodePosRot()
		)
		else
		(
			updateNodePosRot()
			updatePosRot()
			applyNodeOldPosRot()
		)
		
		--gc light:true
	)
	
	
	
	on chkPos changed value do
	(
		local index = getTargetSelIndex()
		
		if index > 0 then
			mapList[index].posActive = value
		
		--gc light:true
	)

	on chkRot changed value do
	(
		local index = getTargetSelIndex()
		
		if index > 0 then
			mapList[index].rotActive = value
		
		--gc light:true
	)
	
	
	
	on btnSave pressed do
	(
		local savePath = getSaveFileName caption:"Save Retarget Script" types:"CryMaxTools RetargetScript (*.crs)|*.crs|Animation Manager Retarget (*.amr)|*.amr"
		if savePath != undefined then
			saveMapping savePath useAMR:(cryMaxTools.basic.str.getExtension savePath == "amr")
		
		--gc light:true
	)
	
	
	on btnLoad pressed do
	(
		if queryBox "Overwrite current mapping?" == true then
		(
			local loadPath = getOpenFileName caption:"Open Retarget Script" types:"CryMaxTools RetargetScript (*.crs)|*.crs"
			if loadPath != undefined then
				loadMapping loadPath
		)
		
		--gc light:true
	)
)

createDialog mapNodesRO (598+200) (382-30)