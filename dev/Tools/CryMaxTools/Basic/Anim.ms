gc light:true






struct timeStruct
(
	time,
	cons = #()
)


struct conStruct
(
	ID,
	name,
	properties = #()
)


struct propStruct
(
	name = #none,
	value = 0.0
)




function getLastController baseCon keepLayer:false =
(
	local returnArray = #()
	
	if baseCon[1] == undefined then
		append returnArray baseCon
	else
	(
		for i = 1 to 100 do
		(
			if baseCon[i] != undefined then
			(
				if keepLayer == false then
					join returnArray (getLastController baseCon[i])
				else
					append returnArray baseCon[i]
			)
			else
				exit
		)
	)
	
	return returnArray
)




function getLastCustAttribute obj =
(
	local custAttrArray = #()
	
	for i = 1 to obj.modifiers.count do
	(
		try
		(
			for f = 1 to obj.modifiers[i].custAttributes.count do
				join custAttrArray (getLastController obj.modifiers[i].custAttributes[f])
		)catch()
		
	)	
	return custAttrArray
)


function getControllers obj =
(
	local conArray = #()
	
		--// non-biped
	if obj.classID[1] != 37157 then
	(
		join conArray (getLastController obj.controller)
		join conArray (getLastCustAttribute obj)
	)
	else
	(
			--// biped root controller
		if classOf obj.controller == Vertical_Horizontal_Turn then
			join conArray (getLastController obj.controller keepLayer:true)
			--// biped slave controller
		else
			append conArray obj.controller
	)
	return conArray
)



function getKeyInstance con keyIndex:1 =
(
	try
	(
		local isBiped = false
		local propArray = #()
		local key = undefined
		local propNames = #()
		
		try 
		(
			if con.rootNode.classID[1] == 37157 then
				isBiped = true
		)catch()
		
		
		if isBiped == false then
		(
			local tempCon = (classOf con.controller)()
			addNewKey tempCon 0
			key = tempCon.keys[1]
		)
		else
		(
			if con.keys.count == 0 then
				biped.addNewkey con 0
			
			key = biped.getKey con 1
		)
		
		try
		(
			propNames = getPropNames key
				
			for i = 1 to propNames.count do
				append propArray (propStruct name:propNames[i] value:(getProperty key propNames[i]))
		)
		catch()
		
		
	)
	catch
		return (#())
	
	
	return propArray
)








function getKeyedControllers obj t skipRange:false =
(	
	local returnArray = #()
	local conArray = getControllers obj
	local keyTime = t
	
	for i = 1 to conArray.count do
	(
		
		local conName = ""
		
		try
		(
			if conArray[i].keys != undefined then
			(
				if conArray[i].keys.count > 0 then
				(
					for f = 1 to conArray[i].keys.count do
					(
						if conArray[i].keys[f].time == keyTime or skipRange == true then
						(
							local propArray = #()
							local key = undefined
							local isBiped = false
							
							
							try
							(
								if conArray[i].rootNode.classID[1] == 37157 then
									isBiped = true
							)catch()
							
							if isBiped == true then
							(
								at time keyTime
									key = biped.getKey conArray[i] f
								
								conName = "Biped"
							)
							else
							(
								key = conArray[i].keys[f]
								conName = conArray[i].name
							)
							
							local cmpKey = getKeyInstance conArray[i] keyIndex:f
							
							
							
							local propNames = getPropNames key
									
							for d = 3 to propNames.count do
							(
								local tempProp = getProperty key propNames[d]
								--if tempProp != cmpKey[d].value or conName == "Biped" then
									append propArray (propStruct name:propNames[d] value:tempProp)
							)
							
							if propArray.count > 0 then
								append returnArray (conStruct ID:i name:conName properties:propArray)
							
						)
					)
				)
			)
		)catch()
	)
		
	
	
	return returnArray
)






function getKeys obj =
(
	local keyArray = #()
	
	for i = animationRange.start to animationRange.end do
		append keyArray (timeStruct time:i cons:(getKeyedControllers obj i))
			
	return keyArray
)







function getChildren theNode =
(
	local nodeArray = #()
	for obj in theNode.children do
	(
		append nodeArray obj
		join nodeArray (getChildren obj)
	)
	return nodeArray
)





function sortRootChildren nodeArray =
(
	try local tempParent = getNodeByName nodeArray[1].name
	catch local tempParent = getNodeByName nodeArray[1]
	
	while tempParent.parent != undefined do
	(
		enableEscape = true
		tempParent = tempParent.parent
	)
	
	local root = tempParent
	
	nodeAllArray = #(root)
	join nodeAllArray (cryMaxTools.anim.functions.OP.getChildren root)
	
	try
	(
		for i = 1 to nodeArray.count do
			nodeArray[i] = nodeArray[i].name
	)catch()
	
	
	
	for i = 1 to nodeAllArray.count do
		nodeAllArray[i] = nodeAllArray[i].name
	
	local notParentedArray = #()
	
	
	for i = 1 to nodeAllArray.count do
	(
		if findItem nodeArray nodeAllArray[i] == 0 then
			nodeAllArray[i] = undefined
	)
	
	for i = 1 to nodeArray.count do
		if findItem nodeAllArray nodeArray[i] == 0 then
			append notParentedArray nodeArray[i]
	
	local resultArray = #()
	
	for i = 1 to nodeAllArray.count do
		if nodeAllArray[i] != undefined then
			append resultArray nodeAllArray[i]
	
	join resultArray notParentedArray
		
	for i = 1 to resultArray.count do
		resultArray[i] = getNodeByName resultArray[i]
		
	return resultArray
	
)



function combineKey templateKey newKey =
(
	local propArray = #()
	
	if templateKey.count == 0 then
		return newKey
	
	for i = 3 to templateKey.count do
	(
		local found = 0
		
		for f = 1 to newKey.count do
		(
			if templateKey[i].name == newKey[f].name then
			(
				found = f
				exit
			)
		)
		
		if found > 0 then
			append propArray newKey[found]
		else
			append propArray (copy templateKey[i])
	)
	
	return propArray
)






function saveAMAOLD animPath objs:undefined offset:#() =
(
	dotNet.loadAssembly "system.xml"
	local xmlFile = dotNetObject "system.xml.xmlDocument"
	
	local fileRoot = xmlFile.createElement "Anim"
	xmlFile.appendChild fileRoot
	
	try (deleteFile animPath) catch()
	
	local rangeRoot =  xmlFile.createElement "Time"
	rangeRoot.setAttribute "Start" (((animationRange.start.ticks as Integer) / 160.0) as String)
	rangeRoot.setAttribute "Stop" (((animationRange.end.ticks as Integer) / 160.0) as String)
	fileRoot.appendChild rangeRoot
	
	
	if objs == undefined then
	(
		if selection.count > 0 then
			local objs = getCurrentSelection()
		else
			local objs = objects as Array
	)
	
	
	objs = cryMaxTools.anim.functions.OP.sortRootChildren objs
	
	local tempSel = getCurrentSelection()
	local osObjectArray = #()
	clearSelection()
	
	
	for i = 1 to objs.count do
	(
		local obj = objs[i]
		
		if obj.classID[1] == 37157 then
		(
			if (biped.getNode obj #lArm link:4) == obj or (biped.getNode obj #rArm link:4) == obj or (biped.getNode obj #lleg link:3) == obj or (biped.getNode obj #rleg link:3) == obj then
			(
				select obj
				if obj.controller.rootNode.controller.osObject != undefined then
					osObjectArray[i] = obj.controller.rootNode.controller.osObject.name
			)
		)
	)
	
	
	
	for t = animationRange.start.ticks to animationRange.end.ticks do
	(
		local hasKeys = false
		
		local frameTime = t / 160.0
		
		local timeRoot = xmlFile.createElement "Key"
		timeRoot.setAttribute "Frame" (frameTime as String)
		
		
		
		at time (frameTime as Time)
		(
			for h = 1 to objs.count do
			(
				local obj = objs[h]
				
				local conArray = getKeyedControllers obj (frameTime as Time)
				
				if conArray.count > 0 then
				(
					local objRoot = xmlFile.createElement "node"
					objRoot.setAttribute "Name" obj.name
					
					hasKeys = true
					
					local offsetTM = matrix3 1
					for i = 1 to offset.count do
						if obj.name == offset[i][1] then
							offsetTM = offset[i][2]
					
					objRoot.setAttribute "TM" (obj.transform as String)
					objRoot.setAttribute "Offset" (offsetTM as String)
						
					if osObjectArray[h] != undefined then
						objRoot.setAttribute "OSObject" (osObjectArray[h] as String)
					
					if obj.classID[1] == 37157 then
					(
						if (biped.getEulerActive obj) == true then
						(
							objRoot.setAttribute "Euler" ("#" + (biped.getEulerOrder obj) as String)
						)
					)
					
					
					for i = 1 to conArray.count do
					(
						local conRoot = xmlFile.createElement "Con"
						conRoot.setAttribute "ID" (conArray[i].ID as String)
						conRoot.setAttribute "Name" conArray[i].name
						

						
						for d = 1 to conArray[i].properties.count do
						(
							if conArray[i].properties[d].name != #ikPivots then
							(
								local tempValue = conArray[i].properties[d].value
								if classOf tempValue == Name then
									tempValue = "#" + (tempValue as String)
								else
									tempValue = tempValue as String
								
								conRoot.setAttribute (conArray[i].properties[d].name as String) tempValue
							)
						)
						
						objRoot.appendChild conRoot
					)
					
					timeRoot.appendChild objRoot
				)
			)
		)
		
			
		if hasKeys == true then
			fileRoot.appendChild timeRoot
	)
	
	clearSelection()
	select tempSel
	
	xmlFile.save animPath
	
	return true
)







function loadAMAOLD animPath objs:undefined useFrame:false =
(
	local tempSel = getCurrentSelection()
	clearSelection()
	
	local osObjectArray = #()
	
	dotNet.loadAssembly "system.xml"
	local xmlFile = dotNetObject "system.xml.xmlDocument"
	
	xmlFile.load animPath
	local fileRoot = xmlFile.documentElement
	
		--// start / stop part
	local startTime = (execute fileRoot.childNodes.itemOf[0].attributes.itemOf[0].value)
	local stopTime = (execute fileRoot.childNodes.itemOf[0].attributes.itemOf[1].value)
	
	animationRange = interval startTime stopTime
	
	--with redraw off
	(
			
		with animate on
		(
			
				--// keys
			for i = 1 to (fileRoot.childNodes.count - 1) do
			(
				local keyNode = fileRoot.childNodes.itemOf[i]
				local keyTime = (execute keyNode.attributes.itemOf[0].value) as Time
				
				if useFrame != false then
					if keyTime != useFrame then
						continue
			
				
					--// nodes
				for d = 0 to (keyNode.childNodes.count - 1) do
				(
					local objNode = keyNode.childNodes.itemOf[d]
					local obj = getNodeByName objNode.attributes.itemOf[0].value
					
					
					if obj != undefined then
					(
						local objConArray = getControllers obj
						local objTM = execute objNode.attributes.itemOf[1].value
						local objOffsetTM = execute objNode.attributes.itemOf[2].value
						local newTM = objOffsetTM * objTM
						local foundCon = #{}
						local isBiped = obj.classID[1] == 37157
						local bipedIKActive = 0
						local bipedIKBlend = 0.0
						local bipedPrevKeyActive = false
						local tempKey = undefined
						local tempKeyTime = keyTime
						local foundLink = false
						local isBipedLimb = false
						local key = undefined
						
						
						
						if isBiped then
						(
							for k = 0 to (objNode.attributes.count - 1) do
							(
								case objNode.attributes.itemOf[k].name of
								(
									"OSObject":
									(
										if (findItem osObjectArray obj.name == 0) then
										(
											local osObject = getNodeByName objNode.attributes.itemOf[k].value
											select obj
											obj.controller.rootNode.controller.osObject = osObject
											clearSelection()
											
											append osObjectArray obj.name
										)
									)
									
									"Euler":
									(
										biped.setEulerActive obj (execute objNode.attributes.itemOf[k].value)
									)
								)
							)
							
							
							
							for g = 1 to (biped.maxNumNodes obj) do
							(
								local tempNode = (biped.getNode obj g)
								
								if tempNode != undefined then
								(
									for j = 1 to biped.maxNumLinks tempNode do
									(
										if biped.getNode obj g link:j == obj then
										(
											foundLink = true
											exit
										)
									)
									
									if foundLink == true then
										exit
								)
							)
							
							isBipedLimb = (biped.getNode obj #lArm link:4) == obj or (biped.getNode obj #rArm link:4) == obj or (biped.getNode obj #lleg link:3) == obj or (biped.getNode obj #rleg link:3) == obj or (biped.getNode obj #prop1) == obj or (biped.getNode obj #prop2) == obj or (biped.getNode obj #prop3) == obj or foundLink == false
							
						)
						
						tempKeyTime = (keyTime.frame as Integer) as Time
						
						
						sliderTime = tempKeyTime
						
						
						
						
						
						
							--// controllers
						for f = 0 to (objNode.childNodes.count - 1) do
						(
							local conNode = objNode.childNodes.itemOf[f]
							
							local conIndex = execute conNode.attributes.itemOf[0].value
							local conName = conNode.attributes.itemOf[1].value
							
							
							local cmpKey = getKeyInstance objConArray[conIndex]
							
							local propValArray = #()
							
							
							for t = 2 to (conNode.attributes.count - 1) do
							(
								local propName = (conNode.attributes.itemOf[t].name as Name)
								local propValue = execute (conNode.attributes.itemOf[t].value)
								
								append propValArray (propStruct name:propName value:propValue)
							)
							
							local propArray = combineKey cmpKey propValArray
							
							
							local curCon = objConArray[conIndex]
							
							if obj.classID[1] == 37157 then
							(
								
								biped.addNewKey objConArray[conIndex] tempKeyTime
								for t = 1 to objConArray[conIndex].keys.count do
								(
									if objConArray[conIndex].keys[t].time == tempKeyTime then
									(
										key = biped.getKey objConArray[conIndex] t
									)
								)
							)
							else
							(
								if objConArray[conIndex].name == conName then
								(
									addNewKey objConArray[conIndex] tempKeyTime
									for t = 1 to objConArray[conIndex].keys.count do
									(
										if objConArray[conIndex].keys[t].time == tempKeyTime then
											key = objConArray[conIndex].keys[t]
									)
								)
							)
							
							
							
							for s = 1 to propArray.count do
							(
								local skip = false
								
								
								--if isBipedLimb then
								(
									--// HACK \\-- Biped hack to reset the ikSpace after the actual transformation update of the node
									if propArray[s].name == #ikSpace then
									(
										skip = true
										
										if propArray[s].value == 1 and isBipedLimb then
										(
											bipedIKActive = propArray[s].value
											--setProperty key #ikSpace 0
										)
									)
									
									if propArray[s].name == #ikBlend then
									(
										skip = true
										
										if propArray[s].value > 0  and isBipedLimb then
										(
											bipedIKBlend = propArray[s].value
											--setProperty key #ikBlend 0.0
										)
									)
									
									if propArray[s].name == #ikJoinedPivot then
									(
										skip = true
										
										if propArray[s].value == true  and isBipedLimb then
										(
											bipedPrevKeyActive = propArray[s].value
											--setProperty key #ikJoinedPivot false
										)
									) 
								)
									
								if skip == false then
									try setProperty key propArray[s].name propArray[s].value catch()	
							)
							
							
							try foundCon[1] = findString objConArray[conIndex].name "Scale" != undefined or foundCon[1] catch()
							try foundCon[2] = (findString objConArray[conIndex].name "Position" != undefined) or foundCon[2] catch()
							try foundCon[3] = findString objConArray[conIndex].name "Rotation" != undefined or foundCon[3] catch()
						)
						
						
							
						
						
						
						if isBiped then
						(
								--// HACK \\-- Biped hack to reset the ikSpace after the actual transformation update of the node
							if isBipedLimb then
								biped.setTransform obj #pos newTM.pos true
							
							biped.setTransform obj #rotation newTM.rotation true
								
							
							
							if isBipedLimb then
							(
								if key != undefined then
								(
									/*
									if bipedIKActive == 1 then
									(	
										setProperty key #ikSpace 1
									)
									
									if bipedIKBlend > 0.0 then
									(
										setProperty key #ikBlend bipedIKBlend
									)
									
									if bipedPrevKeyActive == true then
									(
										setProperty key #ikJoinedPivot true
									)
									*/
								)
							)
						)
						else
						(
								--// update controllers with offset
							if foundCon[1] and objOffsetTM.scale != [0,0,0] then
								(getTMController obj).scale = newTM.scale
							
							if foundCon[2] and objOffsetTM.pos != [0,0,0] then
								(getTMController obj).position = newTM.pos
							
							if foundCon[3] and objOffsetTM.rotation != quat 1 then
								(getTMController obj).rotation = newTM.rotation
						)
					)
				)
			)
		)
	)
	
	clearSelection()
	select tempSel
)

















function saveAMA animPath objs:undefined offset:#() useTicks:false clampFrames:false =
(
	dotNet.loadAssembly "system.xml"
	local xmlFile = dotNetObject "system.xml.xmlDocument"
	
	local fileRoot = xmlFile.createElement "Anim"
	xmlFile.appendChild fileRoot
	
	try (deleteFile animPath) catch()
	
	local rangeRoot =  xmlFile.createElement "Time"
	
	local animStart = animationRange.start.frame as Integer
	local animEnd = animationRange.end.frame as Integer
	
	if useTicks == true then
	(
		animStart = animationRange.start.ticks as float
		animEnd = animationRange.end.ticks as float
	)
	
	rangeRoot.setAttribute "Start" (animStart as String)
	rangeRoot.setAttribute "Stop" (animEnd as String)
	fileRoot.appendChild rangeRoot
	
	
	if objs == undefined then
	(
		if selection.count > 0 then
			local objs = getCurrentSelection()
		else
			local objs = objects as Array
	)
	
	
	objs = cryMaxTools.anim.functions.OP.sortRootChildren objs
	
	local tempSel = getCurrentSelection()
	local osObjectArray = #()
	clearSelection()
	
	
	for i = 1 to objs.count do
	(
		local obj = objs[i]
		
		if obj.classID[1] == 37157 then
		(
			if (biped.getNode obj #lArm link:4) == obj or (biped.getNode obj #rArm link:4) == obj or (biped.getNode obj #lleg link:3) == obj or (biped.getNode obj #rleg link:3) == obj then
			(
				select obj
				if obj.controller.rootNode.controller.osObject != undefined then
					osObjectArray[i] = obj.controller.rootNode.controller.osObject.name
			)
		)
	)
	
	--print offset
	
	for h = 1 to objs.count do
	(
		local obj = objs[h]
		local hasKeys = false
		
		
		local offsetTM = matrix3 1
		for i = 1 to offset.count do
			if obj.name == offset[i][1] then
				offsetTM = offset[i][2]
			
		
		
		local objRoot = xmlFile.createElement "node"
		objRoot.setAttribute "Name" obj.name
		objRoot.setAttribute "Offset" (offsetTM as String)
		
		if osObjectArray[h] != undefined then
			objRoot.setAttribute "OSObject" (osObjectArray[h] as String)
		
		if obj.classID[1] == 37157 then
		(
			if (biped.getEulerActive obj) == true then
			(
				objRoot.setAttribute "Euler" ("#" + (biped.getEulerOrder obj) as String)
			)
		)
		
		
		for t = animStart to animEnd do
		(
			--local frameTime = t / 160.0
			local frameTime = t
			
			if useTicks == true then
				frameTime = t / 160.0
			
			if clampFrames == true then
				frameTime = frameTime as Integer
			
			local timeRoot = xmlFile.createElement "Key"
			timeRoot.setAttribute "Frame" (frameTime as String)
			
			
			at time (frameTime as Time)
			(
				timeRoot.setAttribute "TM" (obj.transform as String)
				local conArray = getKeyedControllers obj (frameTime as Time)
				
				
				if conArray.count > 0 then
				(
					hasKeys = true
					
					for i = 1 to conArray.count do
					(
						local conRoot = xmlFile.createElement "Con"
						conRoot.setAttribute "ID" (conArray[i].ID as String)
						conRoot.setAttribute "Name" conArray[i].name
						

						
						for d = 1 to conArray[i].properties.count do
						(
							if conArray[i].properties[d].name != #ikPivots then
							(
								local tempValue = conArray[i].properties[d].value
								if classOf tempValue == Name then
									tempValue = "#" + (tempValue as String)
								else
									tempValue = tempValue as String
								
								conRoot.setAttribute (conArray[i].properties[d].name as String) tempValue
							)
						)
						
						timeRoot.appendChild conRoot
					)
					
					objRoot.appendChild timeRoot
				)
			)
		)
		
			
		if hasKeys == true then
			fileRoot.appendChild objRoot
	)
	
	clearSelection()
	select tempSel
	
	xmlFile.save animPath
	
	return true
)




function getInvertTime range t =
(
	return ()
)


function loadAMA animPath objs:undefined useFrame:false invertTime:false =
(
	local tempSel = cryMaxTools.anim.functions.op.sortRootChildren (getCurrentSelection())
	clearSelection()
	
	local osObjectArray = #()
	
	dotNet.loadAssembly "system.xml"
	local xmlFile = dotNetObject "system.xml.xmlDocument"
	
	xmlFile.load animPath
	local fileRoot = xmlFile.documentElement
	
		--// start / stop part
	local startTime = (execute fileRoot.childNodes.itemOf[0].attributes.itemOf[0].value)
	local stopTime = (execute fileRoot.childNodes.itemOf[0].attributes.itemOf[1].value)
	
	animationRange = interval startTime stopTime
	
	disableSceneRedraw()
	--with redraw off
	(
			
		with animate on
		(
			
				--// keys
			for i = 1 to (fileRoot.childNodes.count - 1) do
			(
				local objNode = fileRoot.childNodes.itemOf[i]
				local obj = getNodeByName objNode.attributes.itemOf[0].value
				
				if obj != undefined then
				(
					local objConArray = getControllers obj
					local objOffsetTM = execute objNode.attributes.itemOf[1].value
					local isBiped = obj.classID[1] == 37157
					local isBipedLimb = false
					local foundLink = false
					
					
					if isBiped then
					(
						for k = 0 to (objNode.attributes.count - 1) do
						(
							case objNode.attributes.itemOf[k].name of
							(
								"OSObject":
								(
									if (findItem osObjectArray obj.name == 0) then
									(
										local osObject = getNodeByName objNode.attributes.itemOf[k].value
										select obj
										obj.controller.rootNode.controller.osObject = osObject
										clearSelection()
										
										append osObjectArray obj.name
									)
								)
								
								"Euler":
								(
									biped.setEulerActive obj (execute objNode.attributes.itemOf[k].value)
								)
							)
						)
						
						
						
						for g = 1 to (biped.maxNumNodes obj) do
						(
							local tempNode = (biped.getNode obj g)
							
							if tempNode != undefined then
							(
								for j = 1 to biped.maxNumLinks tempNode do
								(
									if biped.getNode obj g link:j == obj then
									(
										foundLink = true
										exit
									)
								)
								
								if foundLink == true then
									exit
							)
						)
						
						isBipedLimb = (biped.getNode obj #lArm link:4) == obj or (biped.getNode obj #rArm link:4) == obj or (biped.getNode obj #lleg link:3) == obj or (biped.getNode obj #rleg link:3) == obj or (biped.getNode obj #prop1) == obj or (biped.getNode obj #prop2) == obj or (biped.getNode obj #prop3) == obj or foundLink == false
						
					)
					
				
				
						--// nodes
					for d = 0 to (objNode.childNodes.count - 1) do
					(
						local keyNode = objNode.childNodes.itemOf[d]
						local keyTime = (execute keyNode.attributes.itemOf[0].value) as Time
						
						if useFrame != false then
							if keyTime != useFrame then
								continue
			
					
					
					
						local objTM = execute keyNode.attributes.itemOf[1].value
						local newTM = objOffsetTM * objTM
						local foundCon = #{}
						local bipedIKActive = 0
						local bipedIKBlend = 0.0
						local bipedPrevKeyActive = false
						local tempKey = undefined
						local tempKeyTime = keyTime
						
						local key = undefined
						
						
						
						tempKeyTime = (keyTime.frame as Integer) as Time
						
						if invertTime == true then
							tempKeyTime = stopTime + (startTime - tempKeyTime)
						
						--at time tempKeyTime
						(
							sliderTime = tempKeyTime
							
							
							
								--// controllers
							for f = 0 to (keyNode.childNodes.count - 1) do
							(
								local conNode = keyNode.childNodes.itemOf[f]
								
								local conIndex = execute conNode.attributes.itemOf[0].value
								local conName = conNode.attributes.itemOf[1].value
								
								
								local cmpKey = getKeyInstance objConArray[conIndex]
								
								local propValArray = #()
								
								
								for t = 2 to (conNode.attributes.count - 1) do
								(
									local propName = (conNode.attributes.itemOf[t].name as Name)
									local propValue = execute (conNode.attributes.itemOf[t].value)
									
									append propValArray (propStruct name:propName value:propValue)
								)
								
								local propArray = combineKey cmpKey propValArray
								
								
								local curCon = objConArray[conIndex]
								
								if obj.classID[1] == 37157 then
								(
									
									biped.addNewKey objConArray[conIndex] tempKeyTime
									for t = 1 to objConArray[conIndex].keys.count do
									(
										if objConArray[conIndex].keys[t].time == tempKeyTime then
										(
											key = biped.getKey objConArray[conIndex] t
										)
									)
								)
								else
								(
									if objConArray[conIndex].name == conName then
									(
										addNewKey objConArray[conIndex] tempKeyTime
										for t = 1 to objConArray[conIndex].keys.count do
										(
											if objConArray[conIndex].keys[t].time == tempKeyTime then
												key = objConArray[conIndex].keys[t]
										)
									)
								)
								
								
								
								for s = 1 to propArray.count do
								(
									local skip = false
									
									
									--if isBipedLimb then
									(
										--// HACK \\-- Biped hack to reset the ikSpace after the actual transformation update of the node
										if propArray[s].name == #ikSpace then
										(
											skip = true
											
											if propArray[s].value == 1 and isBipedLimb then
											(
												bipedIKActive = propArray[s].value
												setProperty key #ikSpace 0
											)
										)
										
										if propArray[s].name == #ikBlend then
										(
											skip = true
											
											if propArray[s].value > 0  and isBipedLimb then
											(
												bipedIKBlend = propArray[s].value
												setProperty key #ikBlend 0.0
											)
										)
										
										if propArray[s].name == #ikJoinedPivot then
										(
											skip = true
											
											if propArray[s].value == true  and isBipedLimb then
											(
												bipedPrevKeyActive = propArray[s].value
												setProperty key #ikJoinedPivot false
											)
										) 
									)
										
									--if skip == false then
										try setProperty key propArray[s].name propArray[s].value catch()	
								)
								
								
								try foundCon[1] = findString objConArray[conIndex].name "Scale" != undefined or foundCon[1] catch()
								try foundCon[2] = (findString objConArray[conIndex].name "Position" != undefined) or foundCon[2] catch()
								try foundCon[3] = findString objConArray[conIndex].name "Rotation" != undefined or foundCon[3] catch()
							)
							
							
								
							
							
							
							if isBiped then
							(
									--// HACK \\-- Biped hack to reset the ikSpace after the actual transformation update of the node
								if isBipedLimb then
									biped.setTransform obj #pos newTM.pos true
								
								biped.setTransform obj #rotation newTM.rotation true
									
								
								
								if isBipedLimb then
								(
									if key != undefined then
									(
										if bipedIKActive == 1 then
										(	
											setProperty key #ikSpace 1
										)
										
										if bipedIKBlend > 0.0 then
										(
											setProperty key #ikBlend bipedIKBlend
										)
										
										if bipedPrevKeyActive == true then
										(
											setProperty key #ikJoinedPivot true
										)
									)
								)
							)
							else
							(
									--// update controllers with offset
								if foundCon[1] and objOffsetTM.scale != [0,0,0] then
									(getTMController obj).scale = newTM.scale
								
								if foundCon[2] and objOffsetTM.pos != [0,0,0] then
									(getTMController obj).position = newTM.pos
								
								if foundCon[3] and objOffsetTM.rotation != quat 1 then
									(getTMController obj).rotation = newTM.rotation
							)
						)
						
						--completeRedraw()
					)
				)
			)
		)
	)
	
	enableSceneRedraw()
	
	clearSelection()
	select tempSel
)



























-- Basic functionality needed for anim tools


	--// BASIC STRUCTURES \\--



struct cryMaxToolsAnimOPFunctionStruct
(
	moveToFloor,
	getSelectedBipPart,
	getBipRoots,
	getBipNames,
	getBaseBip,
	resetRotation,
	rotateAnim,
	moveToBodyMass,
	bipToZero2,
	bipToZero3,
	setTrajectories,
	resetLocator,
	autoLocator,
	sortList,
	getChildren,
	getChildExtent,
	createSnapshot,
	lockRotation,
	pasteTransform,
	getTimeLineRange,
	nodeDependsOn,
	sortRootChildren
)



struct cryMaxToolsAnimUIItemStruct
(
	use,
	getIndex,
	updateExtent
)





struct cryMaxToolsAnimUIFunctionStruct
(
	loadSettings,
	saveSettings,
	updateRefModel,
	loadMapping,
	updateRetarget,
	runRetarget,
	loadMaxModel,
	loadAnim,
	saveAnim,
	saveAnimCheck,
	exportAnim,
	loadModel,
	checkExport,
	readHTR,
	importHTRFile,
	selectByNameFilterNode,
	selectByNameFilterParent,
	restartAndContinue,
	
	items = cryMaxToolsAnimUIItemStruct(),
	
	setMacros,
	setMenu
)




	
struct cryMaxToolsAnimFunctionStruct
(
	op = cryMaxToolsAnimOPFunctionStruct(),
	UI = cryMaxToolsAnimUIFunctionStruct()
)	



struct cryMaxToolsAnimMapperStruct
(
	targetList = #(),
	sourceList = #(),
	mapList = #(),
	mapPosRotList = #(),
	mappedChar = 0
)




struct cryMaxToolsAnimItemStruct ( name, external, model, parent, reference, rotation, position )
struct cryMaxToolsAnimBoneStruct ( active, name, external, bones )
struct cryMaxToolsAnimMapStruct
(
	nodeName = "",
	sourceName = "",
	targetVis = true,
	targetMap = false,
	sourceVis = true,
	sourceMap = false,
	
	rot = (quat 1),
	rotActive = true,
	pos = [0,0,0],
	posActive = false,
	tm = matrix3 1
)



struct cryMaxToolsAnimVarsStruct
(
	basePath = "C:\\",
	exportPath = "C:\\",
	itemList = #(),
	boneList = #(),
	perforce = #(3,3,3,3,3,3,3,3),
	refModel = #(),
	map = #(),
	tempINIPath = ((getDir #temp) + "\cryAnim_temp.ini"),
	animOpenPath = "",
	animSavePath = "",
	animExportPath = "",
	bipRoot,
	updateList,
	delSkel = true,
	transformList = #(),
	various = #(),
	keyPerFrame = true,
	useColladaExporter = false,
	usePerforceLoad = false,
	usePerforceSave = true,
	
	lockRotation
)



struct bipPartSt ( object, part, name, pivotSel )
struct objectSt ( selected, parent, child, isPivot )
struct pivotSelSt ( index, pivotName, position, pivPoint, pivObject )
struct partNameSt ( name, leftRight, bodyPart )
struct pivPointSt ( name, pointPos, snapTM )
struct pivotDistanceStruct ( distance, name )
struct pivotLastSt (	index = 6, pivotName, position, pivObject, pivPoint = pivPointSt() )
struct selectionSt (	pivot = #(), selection = #() )
struct partSt ( LHand = (pivotLastSt pivotName:"p6_(Bip01 L Hand)" pivObject:"Bip01 L Hand"), RHand = (pivotLastSt pivotName:"p6_(Bip01 R Hand)" pivObject:"Bip01 R Hand"), LFoot = (pivotLastSt pivotName:"p6_(Bip01 L Foot)" pivObject:"Bip01 L Foot"), RFoot = (pivotLastSt pivotName:"p6_(Bip01 R Foot)" pivObject:"Bip01 R Foot"), tempSelection )
struct pointEdgeSt ( pivot, selection )
struct activePivotSt ( interPivot, interPos )
struct locStruct ( start, rotate, cycle, position )

struct frameStruct (frame = 0, pos = [0,0,0], rot = (quat 1))
struct HTRBoneStruct (boneName = "", parent = undefined, frames = #())











struct cryMaxToolsAnimAlignFunctionStruct
(
	getPivotPointPos,
	getNearestSnapshot,
	moveBipPart,
	moveRotBipPart,
	createPivotEdge,
	createPivotPoint,
	createPointEdge,
	initVars,
	interactPivotPoint,
	setPivotPoint,
	clickAction,
	deletePivotPoints,
	switchViewPort,
	callPivotSelect,
	pivotPoint
)


struct cryMaxToolsAnimAlignVarsStruct
(
	pointList = selectionSt(),
	edgeList = selectionSt(),
	pivotLast = partSt()
)



struct cryMaxToolsAnimAlignStruct
(
	functions = cryMaxToolsAnimAlignFunctionStruct(),
	vars = cryMaxToolsAnimAlignVarsStruct(),
	pivotSelect
)



struct cryMaxToolsAnimStruct
(
	loadSave,
	align = cryMaxToolsAnimAlignStruct(),
	mapper = cryMaxToolsAnimMapperStruct(),
	batchProcess,
	settings,
	
	functions = cryMaxToolsAnimFunctionStruct(),
	vars = cryMaxToolsAnimVarsStruct()
)



cryMaxTools.anim = cryMaxToolsAnimStruct()








cryMaxTools.anim.functions.UI.loadSettings = function loadSettings =
(
	local iBASE = 1
	local iExport = 2
	local iITEM = 3
	local iBONE = 4
	local iPERFORCE = 5
	
	local baseSettings = cryMaxTools.basic.settingsMan.load "animBaseSettings"
	
	if baseSettings != false then 
	(
		cryMaxTools.anim.vars.basePath = baseSettings[iBASE].value[1]
		cryMaxTools.anim.vars.exportPath = baseSettings[iEXPORT].value[1]
		
		if baseSettings[iITEM] != undefined then
		(
			local itemArray = #()
			local tempStringArray = #()
			
			for i = 1 to baseSettings[iITEM].value.count do
			(
				
				if baseSettings[iITEM].value[i] != "" then
				(							
					local tempFilter = filterString baseSettings[iITEM].value[i] "#"

					append tempStringArray (cryMaxToolsAnimItemStruct name:(tempFilter[1] as String) external:(tempFilter[2] as String) model:(tempFilter[3] as String) reference:(tempFilter[4] as String) parent:(tempFilter[5] as String) rotation:(tempFilter[6] as String) position:(tempFilter[7] as String))
				)
			)
			
			
			for i = 1 to tempStringArray.count do
			(
				local tempArray = #()
				
				if tempStringArray[i].name != "" then
				(
					itemArray[i] = cryMaxToolsAnimItemStruct()
					itemArray[i].name = tempStringArray[i].name
					itemArray[i].external = tempStringArray[i].external
					itemArray[i].model = filterString tempStringArray[i].model ";"
					itemArray[i].reference = filterString tempStringArray[i].reference ";"
					itemArray[i].parent = filterString tempStringArray[i].parent ";"
					itemArray[i].position = filterString tempStringArray[i].position ";"
					itemArray[i].rotation = filterString tempStringArray[i].rotation ";"
					
					for f = 1 to itemArray[i].model.count do
					(
						itemArray[i].position[f] = execute (itemArray[i].position[f])
						itemArray[i].rotation[f] = execute (itemArray[i].rotation[f])
					)
				)
			)
		
			cryMaxTools.anim.vars.itemList = itemArray
		)
		
		
		
		
		if baseSettings[iBONE] != undefined then
		(
			local boneArray = #()
			
			for i = 1 to baseSettings[iBONE].value.count do
			(
				if value != "" then
				(
					local tempFilter = filterString baseSettings[iBONE].value[i] "#"
					append boneArray (cryMaxToolsAnimBoneStruct active:tempFilter[1] name:tempFilter[2] external:tempFilter[3] bones:tempFilter[4])
				)
			)
			
			cryMaxTools.anim.vars.boneList = boneArray
		)
		
		
		
		
		if baseSettings[iPERFORCE] != undefined then
		(
			local perforceArray = #()
			
			for i = 1 to baseSettings[iPERFORCE].value.count do
			(
				if value != "" then
				(
					append perforceArray baseSettings[iPERFORCE].value[i]
				)
			)
			
			cryMaxTools.anim.vars.perforce = perforceArray
		)
	)
)
loadSettings = undefined



cryMaxTools.anim.functions.UI.saveSettings = function saveSettings =
(
	local saveArray = #("animBaseSettings")
	append saveArray ("basePath*@" + cryMaxTools.anim.vars.basePath)
	append saveArray ("exportPath*@" + cryMaxTools.anim.vars.exportPath)
	
	local itemString = ""
	
	for i = 1 to cryMaxTools.anim.vars.itemList.count do
	(
		tempVar = cryMaxTools.anim.vars.itemList[i]
		local tempStringArray = (cryMaxToolsAnimItemStruct name:"" external:"" model:"" reference:"" parent:"" rotation:"" position:"")
		local maxCount = tempVar.model.count
		
		for d = 1 to maxCount do
		(
			tempStringArray.model += tempVar.model[d] + (if d < maxCount then ";" else "")
			tempStringArray.reference += tempVar.reference[d] as String + (if d < maxCount then ";" else "")
			tempStringArray.parent += tempVar.parent[d] as String + (if d < maxCount then ";" else "")
			tempStringArray.rotation += tempVar.rotation[d] as String + (if d < maxCount then ";" else "")
			tempStringArray.position += tempVar.position[d] as String + (if d < maxCount then ";" else "")
		)
		
		itemString += "@" + tempVar.name + "#" + tempVar.external + "#" + tempStringArray.model + "#" + tempStringArray.reference + "#" + tempStringArray.parent + "#" + tempStringArray.rotation + "#" + tempStringArray.position + "|"
		
	)
	
	append saveArray ("items*" + itemString)
	
	local boneString = ""
	
	for i = 1 to cryMaxTools.anim.vars.boneList.count do
	(
		tempVar = cryMaxTools.anim.vars.boneList[i]
		boneString += "@" + tempVar.active + "#" + tempVar.name + "#" + tempVar.external + "#" + tempVar.bones + "|"
	)
	
	append saveArray ("bones*" + boneString)
	
	
	local perforceString = ""
	
	for i = 1 to cryMaxTools.anim.vars.perforce.count do
		perforceString += cryMaxTools.anim.vars.perforce[i] as String + "|"
	
	append saveArray ("perforce*" + perforceString)
	
	
	cryMaxTools.basic.settingsMan.save saveArray
)
saveSettings = undefined










cryMaxTools.anim.functions.UI.checkExport = function checkExport op filepath =
(
	abortBool = false
	
	tempArray = filterString (cryMaxTools.basic.str.lower filepath) "\\"
	tempBuildPath = (filterString (cryMaxTools.basic.str.lower cryMaxTools.basic.vars.buildPath) "\\")
	
	case op of
	(
		#save:
		(
			/* NEED TO CHECK
			if (biped.getCurrentLayer $'Bip01'.controller) > 0 then
			(
				messageBox "Collapse Layers or choose Base Layer to Save" title:"Save Biped"
				return false
			)
			*/
			
			return true
		)

		#saveExport:
		(
			if (biped.getCurrentLayer $'Bip01'.controller) > 0 then
			(
				messageBox "Collapse Layers or choose Base Layer to Save" title:"Save Biped"
				return false
			)
			
			cryMaxTools.anim.functions.UI.checkExport #createFolder filepath
			return true
		)
		#export:
		(
			return true
		)
		
		#ProductionToGame:
		(
			try
			(
				filepathFilter = filterString filepath "\\"
				workingDirFilter = filterString cryMaxTools.anim.vars.basePath "\\"
				cafSavePath = ""
				
				if filepathFilter.count != workingDirFilter.count then
				(
					for i = (workingDirFilter.count + 1) to filepathFilter.count do
						cafSavePath += filepathFilter[i] + (if i != filepathFilter.count then "\\" else "")
								
				)
				
				return (cryMaxTools.anim.functions.UI.checkExport #getCaf cafSavePath)
			)
			catch ( return cryMaxTools.anim.vars.animExportPath )
			
		)
		
		#getCaf:
		(
			local tempArray = filterString filepath "."
			return (tempArray[1] + ".caf")
		)
		
		
		#createFolder:
		(
			if filePath != undefined then
			(					
				tempPath = filterString filepath "\\"
				maxItems = tempPath.count
				for i = 1 to tempPath.count do
				(
					if findString tempPath[i] "." != undefined then
					(
						maxItems = i - 1
						exit
					)
				)
				tempString = ""
				for i = 1 to maxItems do
				(
					if i == 1 then
						tempString = tempPath[i]
					else
						tempString += "\\" + tempPath[i]
						
					makeDir tempString
				)
				return tempString
			)
			else return false
		)
	)
)
checkExport = undefined








cryMaxTools.anim.functions.UI.loadMapping = function loadMapping filepath =
(
	local nodeList = #()
	
	if doesFileExist filePath then
	(	
		local xmlFile = dotNetObject "system.xml.xmlDocument"

		xmlFile.load filepath
		
		local fileRoot = xmlFile.documentElement
		
		local settings = fileRoot.childNodes.itemOf[0]
		local nodes = fileRoot.childNodes.itemOf[1]
		local sourceList = #()
		
		for i = 0 to (nodes.childNodes.count - 1) do
		(
			local target = nodes.childNodes.itemOf[i]
			local targetName = target.attributes.itemOf[0].value
			local sourceName = target.attributes.itemOf[1].value
			
			local posActive = execute(target.attributes.itemOf[2].value)
			local pos = execute(target.attributes.itemOf[3].value)
			local rotActive = execute(target.attributes.itemOf[4].value)
			local rot = execute(target.attributes.itemOf[5].value)
			
			--local targetID = 0
			--local sourceID = 0
			
			append nodeList (cryMaxToolsAnimMapStruct nodeName:targetName sourceName:sourceName targetMap:true sourceMap:true posActive:posActive pos:pos rotActive:rotActive rot:rot)
			append sourceList (cryMaxToolsAnimMapStruct nodeName:sourceName)
		)
		
		
		
		
		
		
		/*
		for i = 1 to nodeList.count do
		(
			local targetName = nodeList[i].nodeName
			
			for d = 1 to nodeList.count do
				if d != i and targetName == nodeList[d].sourceName then
					nodeList[d].sourceMap = true
		)
		*/
		
		
		try
		(
			mapNodesRO.nodeList = nodeList
			mapNodesRO.mapList = nodeList
			
			mapNodesRO.updateList #target
			mapNodesRO.updateList #source
			mapNodesRO.updateList #map
		)catch()
	)
	
	cryMaxTools.anim.vars.map = nodeList
	
	return filepath
)
loadMapping = undefined









cryMaxTools.anim.functions.UI.updateRetarget = function updateRetarget loadPath:undefined =
(
	--cryMaxTools.anim.vars.retarget.target = #()
	cryMaxTools.anim.vars.map = #()
	--cryMaxTools.anim.vars.mapPosRot = #()
	
	local ready = false
	local useMapper = false
	local loadPath
	
	
	if loadPath != undefined then
	(
		if (getFiles loadPath).count != 0 then
			ready = true
	)
	else
	(
		for i = 1 to cryMaxTools.anim.vars.refModel.count do
		(
			if cryMaxTools.anim.vars.refModel[i][2] == true then
			(
				if cryMaxTools.anim.vars.refModel[i][3][1] > 2 then
				(
					loadPath = cryMaxTools.anim.vars.refModel[i][3][(cryMaxTools.anim.vars.refModel[i][3][1] - 2)]
					ready = true
				)
			)
		)
		
		if ready == false then
		(
			try
			(
				if cryMaxTools.anim.mapper.mapList.count > 0 then
				(
					useMapper = true
					ready = true
				)
			)catch()
		)
	)
	
	
	
	if ready == true then
	(
		local notFoundArray = #()
		local returnStruct = cryMaxToolsAnimRetargetStruct()
		
		
		for obj in objects do
			append returnStruct.target obj.name
		
		if useMapper == false then
		(
			for i = 1 to 500 do
			(
				local targetNode = getINISetting loadPath ("Node" + i as String) "Target"
				if targetNode == "" then
					continue
				
				local sourceNode = getINISetting loadPath ("Node" + i as String) "Source"
				local usePos = getINISetting loadPath ("Node" + i as String) "UsePos"
				local pos = getINISetting loadPath ("Node" + i as String) "Pos"
				local useRot = getINISetting loadPath ("Node" + i as String) "UseRot"
				local rot = getINISetting loadPath ("Node" + i as String) "Rot"
				
				local foundIndex = findItem returnStruct.target targetNode
				if foundIndex > 0 then
				(
					returnStruct.map[foundIndex] = (if sourceNode != "" then sourceNode else undefined)
					local posRotArray = #(false, false, [0,0,0], (quat 0 0 0 1))
					if usePos != "" then posRotArray[1] = usePos == "true"
					if useRot != "" then posRotArray[2] = useRot == "true"
					try (if pos != "" then posRotArray[3] = execute(pos)) catch(print ("ErrorGetPos@" + targetNode))
					try (if rot != "" then posRotArray[4] = execute(rot)) catch(print ("ErrorGetRot@" + targetNode))
					
					returnStruct.mapPosRot[foundIndex] = posRotArray
				)
				else
					append notFoundArray targetNode
				
			)
		)
		else
		(
			for i = 1 to cryMaxTools.anim.mapper.targetList.count do
			(
				if cryMaxTools.anim.mapper.mapList[i] != undefined then
				(
					local foundIndex = findItem returnStruct.target cryMaxTools.anim.mapper.targetList[i]
					if foundIndex > 0 then
					(
						--print ("Found@" + foundIndex as String)
						returnStruct.map[foundIndex] = cryMaxTools.anim.mapper.mapList[i]
						returnStruct.mapPosRot[foundIndex] = cryMaxTools.anim.mapper.mapPosRotList[i]
					)
					else
						append notFoundArray cryMaxTools.anim.mapper.targetList[i]
				)
			)
		)
		
		if notFoundArray.count > 0 then
		(
			local tempString = "Could not find:\n\n"
			for i = 1 to (if notFoundArray.count < 40 then notFoundArray.count else 40) do
				tempString += notFoundArray[i] + "\n"
			
			if notFoundArray.count > 40 then
				tempString += "\n..."
			
			--messageBox tempString
		)
		
		cryMaxTools.anim.vars.retarget = returnStruct
		
		return returnStruct
	)
	else
	(
		if loadPath != undefined and useMapper == false then
			messageBox "Error in Retarget."
		if useMapper == true and loadPath == undefined then
			messageBox "Could not retarget with Mapper"
		
		return false
	)
)
updateRetarget = undefined






cryMaxTools.anim.functions.UI.updateRefModel = function getRefModel folder =
(
	local tempFilter = filterString folder "\\"
	local tempRoot = ""
	local refRoot = ""
	
	for i = 1 to tempFilter.count do
	(
		tempRoot += tempFilter[i] + "\\"
		if (getFiles (tempRoot + "referenceModel.cmt")).count > 0 then
			refRoot = tempRoot
	)
	
	local refPath = refRoot + "referenceModel.cmt"
	local refModelArray = #()
	
	if (getFiles refPath).count > 0 then
	(
		for i = 1 to 100 do
		(
			local refModelPath = getINISetting refPath ("Model" + i as String) "Path"
			local refModelUsed = getINISetting refPath ("Model" + i as String) "Used"
			
			
			
			if refModelPath != "" then
			(
				local tempArray = #(refModelPath)
				if refModelUsed != "" then
					tempArray[2] = true
				else
					tempArray[2] = false
				
				
				
				
				local tempUsed = getINISetting refPath ("Model" + i as String) "ScriptUsed"
					--// 1 = None, 2 = Mapper 3..., 4 = scrip1 
				local scriptUsed = 1
				
				if tempUsed != "" then
					scriptUsed = tempUsed as Integer
				
				local scriptArray = #((scriptUsed as Integer))
				
				
				for f = 1 to 100 do
				(
					local tempScript = getINISetting refPath ("Model" + i as String) ("Script" + f as String)
					
					if tempScript != "" then
						append scriptArray tempScript
					else
						exit
				)
				
				append tempArray scriptArray
				append refModelArray tempArray
			)
			else
				exit
		)
	)
	
	cryMaxTools.anim.vars.refModel = refModelArray
	cryMaxTools.anim.functions.UI.updateRetarget()
	
	
	return refModelArray
)
updateRefModel = undefined














cryMaxTools.anim.functions.UI.runRetarget = function runRetarget =
(
	local error = false
	
	if cryMaxTools.anim.vars.map.count > 0 then
	(	
		local bipedFound = false
		
		for i = 1 to cryMaxTools.anim.vars.map.count do
		(
			local tempNode = getNodeByName cryMaxTools.anim.vars.map[i].nodeName
			if tempNode != undefined then
				if tempNode.classID[1] == 37157 then
					bipedFound = true
		)
		
		undo "Retarget Anim" on ; with redraw off
		(
			with animate on
			(
				for f = animationRange.start to animationRange.end do
				(
					if bipedFound == true then
						sliderTime = f
					
					at time f
					(
						for i = 1 to cryMaxTools.anim.vars.map.count do
						(
							local targetNode = getNodeByName cryMaxTools.anim.vars.map[i].nodeName
							local sourceNode = getNodeByName cryMaxTools.anim.vars.map[i].sourceName
							
							if targetNode == undefined or sourceNode == undefined then
							(
								error = true
								continue
							)
							
							
							local posOffset = ""
							local rotOffset = ""
							local used = #(false, false)
							
							used[1] = cryMaxTools.anim.vars.map[i].posActive
							used[2] = cryMaxTools.anim.vars.map[i].rotActive
							
							posOffset = cryMaxTools.anim.vars.map[i].pos
							rotOffset = cryMaxTools.anim.vars.map[i].rot
							
							
							local newTM = targetNode.transform
							
							
							if used[2] == true then
								newTM.rotation = inverse rotOffset
							
							newTM.pos = [0,0,0]
							
							if used[1] == true then
								newTM.pos = posOffset
							
							newTM = newTM * sourceNode.transform
						
								--// Biped \\--
							
							if targetNode.classID[1] == 37157 then
							(
								--if used[1] == true or (used[1] == true and used[2] == true) then
								if used[1] == true then
									biped.setTransform targetNode #pos newTM.pos true
								if used[2] == true then
								biped.setTransform targetNode #rotation newTM.rotation true
							)
							else
							(
								targetNode.transform = newTM
							)
						)
					)
				)
			)
		)
	)
)
runRetarget = undefined








cryMaxTools.anim.functions.UI.loadMaxModel = function loadMaxModel filepath =
(
	local tempVar = loadMaxFile filepath useFileUnits:true quiet:true
	
	
	if tempVar != false then
	(
		try
		(
			$'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
			$'Bip01 L ForeTwist'.parent = $'Bip01 L Forearm'
			print "TwistBones re-parented to Forearm"
		)
		catch
		(
			print "No Biped Loaded..."
		)
		
		try
		(
			select $Bip01
			setCommandPanelTaskMode #motion
		)catch()
		
		return true
	)
	else
	(
		print ("File: " + filepath + "    not found")
		
		return false
	)
	
	
)
loadMaxModel = undefined








cryMaxTools.anim.functions.UI.readHTR = function readHTR filePath =
(
	struct frameStruct (frame = 0, pos = [0,0,0], rot = (quat 1))
	struct boneStruct (boneName = "", parent = undefined, frames = #())
	
	
	
	local boneArray = #()
	
	if (getFiles filePath).count > 0 then
	(
		local pathStream = openFile filePath mode:"r"
		local mode = #none
		local boneIndex = 0
		local globalFound = false
		
		while (eof pathStream) != true do
		(
			local newLine = readLine pathStream
			
			if findString newLine "[BasePosition]" != undefined then
				mode = #none
			
			
			
			case mode of
			(
				#hierarchy:
				(
					local tempBone = filterString newLine " "
					
					local boneName = tempBone[1]
					local boneParent = undefined
					
					if tempBone[2] != "GLOBAL" then
						boneParent = tempBone[2]
					
					
					append boneArray (boneStruct boneName:boneName parent:boneParent)
				)
				
				#keys:
				(
					
					
					if newLine[1] == "[" then
						boneIndex += 1
					else
					(
						local frameFilter = filterString newLine " "
						local frameIndex = frameFilter[1] as integer
						local framePos = [(frameFilter[2] as float), (frameFilter[3] as float), (frameFilter[4] as float)]
						local frameRot = (eulerangles (frameFilter[5] as float) (frameFilter[6] as float) (frameFilter[7] as float)) as quat
						
						append boneArray[boneIndex].frames (frameStruct frame:(frameIndex-1) pos:framePos rot:frameRot)
					)
				)
			)
			
			
			if findString newLine "#CHILD" != undefined then
				mode = #hierarchy
			
			
			
			if findString newLine "#Fr	Tx" != undefined then
				mode = #keys
		)
		
		close pathStream
	)
	
	return boneArray
)
readHTR = undefined



cryMaxTools.anim.functions.UI.importHTRFile = function importHTRFile filepath additive:false =
(
	local boneArray = cryMaxTools.anim.functions.UI.readHTR filepath
	local rootBone = undefined
	local nodeArray = #()
	
	global boneArrayBla = boneArray
	
	for i = 1 to boneArray.count do
	(
		local tempPoint = point size:10 name:boneArray[i].boneName wireColor:black
		if boneArray[i].parent != undefined then
			tempPoint.parent = getNodeByName boneArray[i].parent
		else
			if rootBone == undefined then
				rootBone = tempPoint
		
		append nodeArray tempPoint
			
		with animate on
		(
			for f = 1 to boneArray[i].frames.count do
			(
				at time (boneArray[i].frames[f].frame as Time)
				(
					if boneArray[i].parent != undefined then
					(
						local parent = getNodeByName boneArray[i].parent
						local tempTM = matrix3 1
						tempTM.rotation = boneArray[i].frames[f].rot
						tempTM.pos = (boneArray[i].frames[f].pos / 10.0)
						
						local newTM = matrix3 1
						
						if additive == false then
							newTM = tempTM * parent.transform
						else
						(
							local additiveNode = getNodeByName ("#A_" + boneArray[i].boneName)
							if additiveNode != undefined and additiveNode.parent != undefined then
								at time animationRange.start
									newTM = (tempTM * (additiveNode.transform * inverse additiveNode.parent.transform)) * additiveNode.parent.transform
						)
						tempPoint.transform = newTM
					)
					else
					(
						local tempTM = matrix3 1
						tempTM.rotation = boneArray[i].frames[f].rot
						tempTM.pos = (boneArray[i].frames[f].pos / 10.0)
						tempPoint.transform = tempTM
					)
				)
			)
		)
	)
	
	
	
	return nodeArray
)
importHTRFile = undefined













cryMaxTools.anim.functions.UI.loadAnim = function loadAnim filepath quiet:undefined skipMax:undefined skipPerforce:undefined bipRoot:undefined usedSkel:undefined XAFMapping:"" XAFPose:"" =
(
	local error = false
	
	local tempMode = getCommandPanelTaskMode()
	setCommandPanelTaskMode #modify
	
	if cryMaxTools.anim.vars.usePerforceLoad == true then
	(
		cryMaxTools.basic.perforceMan.cryAdd filepath
		cryMaxTools.basic.perforceMan.open filepath
	)
	
	case (cryMaxTools.basic.str.lower (cryMaxTools.basic.str.getExtension filepath)) of
	(
		"bip":
		(
			--loadRefModel filepath quiet:quiet
				
			
			local baseBip = cryMaxTools.anim.functions.op.getBaseBip op:#load
			local loadedBiped = undefined
			
			if bipRoot != undefined then
				baseBip = bipRoot[1]
			
			try
			(
				if cryAM.usedSkel != undefined then
				(
					if classOf cryAM.usedSkel == Array then
						baseBip = cryAM.usedSkel[1]
					else
						baseBip = cryAM.usedSkel
				)
			)catch()
				
			
			if baseBip != undefined then
			(
				if filepath != undefined then
				(				
					tempStatus = undefined
					if cryMaxTools.basic.perforceMan.perforceDir != "" and skipPerforce == undefined then
					(
						case cryMaxTools.anim.vars.perforce[1] of
						(
							1: tempStatus = cryMaxTools.basic.perforceMan.open filepath
							2: tempStatus = false
							3: tempStatus = false--cryMaxTools.basic.perforceMan.checkForLoad filepath
						)
					)
					else
						tempStatus = false
					
					if tempStatus != undefined then
					(
						local tempFilterFilename = cryMaxTools.basic.str.getFilename filepath
						local tempUndoString = ("L - " + cryMaxTools.basic.str.getFilename filepath)
						if baseBip.controller.rootNode.controller.figureMode == false then
						(
							undo tempUndoString on
							(
								if (loadedBiped = biped.loadBipFile baseBip.controller filepath) == true then
								(
									cryMaxTools.anim.vars.animOpenPath = filepath
									cryMaxTools.anim.vars.animSavePath = filepath
									
									
									
									if (local exportPath = cryMaxTools.anim.functions.UI.checkExport #ProductionToGame filepath) != false then
										cryMaxTools.anim.vars.animExportPath = exportPath
									
									animationRange = cryMaxTools.anim.functions.op.getTimeLineRange()
									
									local itemFound = false
									for i = 1 to cryMaxTools.anim.vars.itemList.count do
									(
										if (findString filepath cryMaxTools.anim.vars.itemList[i].external) != undefined then
										(
											cryMaxTools.anim.functions.UI.items.use i
											itemFound = true
										)
									)
									if itemFound == false then
										cryMaxTools.anim.functions.UI.items.use 1
								)
							)
						)
					)
				)
			)
			else
				error = true
		)
		
		"max":
		(
			quiet = true
			
			if skipMax == undefined then
			(
				if quiet == undefined then
				(
					if (queryBox "Load max file?") == true then
						cryMaxTools.anim.functions.UI.loadMaxModel filepath
				)
				else
					cryMaxTools.anim.functions.UI.loadMaxModel filepath
			)
		)
		
		"fbx":
		(
			loadRefModel filepath quiet:quiet
			
			--// load the NullScene which has nothing in it, just to reset the scene without reset dialog
			--resetMaxFile #noPrompt
			
				--// imports the fbx file in queue								
			if (importFile filepath) != true then
				error = true
			
				--// creates a dummy used for "Object Export" in CryEngine2 exporter
			


			
				--// tries to get the animation range, if it can't, the animation range is in most cases 0,0 which 3dmax can't handle
			try
			(
					--// sets animation range by the key times of the pelvis
				animationRange = interval $'Bip01 Pelvis'.pos.controller.keys[1].time $'Bip01 Pelvis'.pos.controller.keys[$'Bip01 Pelvis'.pos.controller.keys.count].time
			) catch( animationRange = interval 0 1 )
			
			--cryMaxTools.anim.functions.UI.updateRetarget()
			--cryMaxTools.anim.functions.UI.runRetarget()
		)
		
		
		"xaf":
		(
			local customRig = false
			local charID = -1
			local setupID = -1
			
			try 
			(
				local charID = cryAM._.cbCharacter.selectedIndex
				local setupID = cryAM._.cbSetup.selectedIndex + 1
				
				customRig = cryAM.characterArray[charID].setup[setupID].misc.customRig
			)catch()
			
			--print ("CustomRig : " +  customRig as String)
			
			
			if customRig == false then
			(
				local nodes = #()

				if selection.count > 0 then
					nodes = selection as array
				
				if cryAM.usedSkel.count > 0 then
				(
					nodes = #()
					
					if classOf cryAM.usedSkel[1] == String then
					(
						for i = 1 to cryAM.usedSkel.count do
						(
							local tempNode = getNodeByName cryAM.usedSkel[i]
							if tempNode != undefined then
								append nodes tempNode
						)
					)
					else
						nodes = cryAM.usedSkel
				)
			

				for i = 1 to nodes.count do
					try deleteKeys nodes[i] #allkeys catch()

				
				print ((if XAFMapping == "" then "No Mapping used" else ("Used Mapping: " + XAFMapping)) + "    " + (if XAFPose == "" then "No Pose used" else ("Used Pose: " + XAFPose)))
					
				
				LoadSaveAnimation.loadAnimation filepath nodes relative:false insert:(XAFPose != "") insertTime:(if XAFPose != "" then 1f else 0f) useMapFile:(XAFMapping != "") mapFileName:XAFMapping
				
				if XAFPose != "" then
					LoadSaveAnimation.loadAnimation XAFPose nodes relative:false insert:false insertTime:0f useMapFile:(XAFMapping != "") mapFileName:XAFMapping

				local userAttributes = #()
				local userValues = #()

				LoadSaveAnimation.getUserAttributes filepath &userAttributes &userValues
				
				local tempRange = cryMaxTools.anim.functions.op.getTimeLineRange selNode:nodes
				tempRange.start = 0f
				
				animationRange = tempRange

				for i = 1 to userAttributes.count do
				(
					if findString userAttributes[i] "range" != undefined then
					(
						local tempRange = filterString userValues[i] "-"
						try
							animationRange = interval (execute tempRange[1]) (execute tempRange[2])
						catch()

						exit
					)
				)
			)
			else
			(
				if charID > -1 and setupID > -1 then
				(
					local masterNode = undefined
					try
						masterNode = getNodeByName (cryAM.characterArray[charID].setup[setupID].name + "_Master")
					catch()
					
					if masterNode != undefined then
					(
						
						local tempSel = getCurrentSelection()
						
						select masterNode
						
						completeRedraw()
						
						try
						(
							masterNode.modifiers[1].custattributes[1].RemoveAnimation()
							completeRedraw()
							masterNode.modifiers[1].custattributes[1].LoadAnimationFn filepath false
							completeRedraw()
							
							print "Loaded with Crytek Custom Rig load function"
						)
						catch (print "Error in loading custom rig XAF file.")
						
						--clearSelection()
						--select tempSel
					)
				)
			)
				
		)

		
		"htr":
		(
			global HTRBones = cryMaxTools.anim.functions.UI.importHTRFile filepath
			
			for i = 1 to HTRBones.count do
			(
				local selNode = HTRBones[i]
				if selNode != undefined then
				(
					if selNode.pos.controller.keys.count > 0 then
					(
						animationRange = cryMaxTools.anim.functions.op.getTimeLineRange selNode:selNode
						exit
					)
				)
			)
		)
		
		
		"ama":
		(
			loadAMA filepath
		)
		
		
	)
	
	setCommandPanelTaskMode tempMode
	
	return (not error)
)
loadAnim = undefined


















cryMaxTools.anim.functions.UI.saveAnim = function saveAnim filepath bipRoot:undefined usePerforce:false useTicks:false clampFrames:false =
(
	local savedAnim = false
	
	if cryMaxTools.basic.perforceMan.perforceDir != "" then
	(
		if cryMaxTools.anim.vars.usePerforceSave == true then
		(
			local result = cryMaxTools.basic.perforceMan.open filepath
			try cryAM.addLog ("         ---                    Perforce :      Open for Edit " + (if result == true then "Succeeded" else "Failed!!")) isAdvanced:true catch()
		)
	)
	
	cryMaxTools.anim.functions.UI.checkExport #createFolder (cryMaxTools.basic.str.getDirectory filepath)
	cryMaxTools.basic.perforceMan.setWrite filepath
	
	
	case (cryMaxTools.basic.str.lower (cryMaxTools.basic.str.getExtension filepath)) of
	(
		"bip":
		(
			if bipRoot != undefined then
			(
				if cryMaxTools.anim.vars.keyPerFrame == true then
					biped.saveBipFileSegment bipRoot.controller filepath animationRange.start animationRange.end #keyPerFrame
				else
					biped.saveBipFileSegment bipRoot.controller filepath animationRange.start animationRange.end
					
				savedAnim = true
			)
			else
				return false
		)
		
		"max":
		(
			saveMaxFile filepath
			
			savedAnim = true
			
		)
		
		"caf":
		(
			local safePanel = getCommandPanelTaskMode()
			local useCollada = cryMaxTools.anim.vars.useColladaExporter
			
			local animationRangeSave = animationRange
			
			try
			(
				local skipFirst = cryAM.characterArray[cryAM.cbCharacter.selectedIndex].setup[cryAM.cbSetup.selectedIndex + 1].misc.skipFirst
				local skipLast = cryAM.characterArray[cryAM.cbCharacter.selectedIndex].setup[cryAM.cbSetup.selectedIndex + 1].misc.skipLast
				
				if skipFirst > 0 then
					animationRange = interval (animationRange.start + 1) animationRange.end
				
				if skipLast > 0 then
					animationRange = interval animationRange.start (animationRange.end - 1)
			)catch()
			
			
			
			if useCollada == true then
			(
				try
					cryengine2.export.export_anim filePath cryAM.usedSkel
				catch
				(
					print "No Collada Exporter found, using normal cryExport"
					useCollada = false
				)
			)
			
			if useCollada == false then
			(
				try
				(
					UtilityPanel.OpenUtility CryEngine3_Exporter
					
					local tempDummy = undefined
					local exportNodes = csexport.export.get_node_list()
					local foundNodes = true
					
					
					
					if exportNodes.count > 0 then
					(
						for i = 1 to exportNodes.count do
							if exportNodes[i] == undefined then
								foundNodes = false
					)
					else
						foundNodes = false
					
					if foundNodes == false then
					(
						local tempDummy = getNodeByName "exportDummy"
						
						if tempDummy == undefined then
							tempDummy = box name:"exportDummy"
						
						tempDummy.isHidden = true
						
						csexport.export.set_node_list (#(tempDummy))
					)
					
					try
					(
						if cryAM.usedSkel != undefined then
						(
							try csexport.use_object_bones = false catch()
							csexport.export.set_bone_list cryAM.usedSkel
						)
					)catch()
					
					csexport.export.export_anim filePath
					
					
				)
				catch(print ("Failed to export: " + filePath))
			)
			
			
			setCommandPanelTaskMode mode:safePanel
			animationRange = animationRangeSave
			
			savedAnim = true
		)
		
		"xaf":
		(
			local customRig = false
			local charID = -1
			local setupID = -1
			
			try 
			(
				local charID = cryAM.cbCharacter.selectedIndex
				local setupID = cryAM.cbSetup.selectedIndex + 1
				
				customRig = cryAM.characterArray[charID].setup[setupID].misc.customRig
			)catch()
			
			
			if customRig == false then
			(	
				local nodes = #()
				
				if selection.count > 0 then
					nodes = selection as array
				
				try
				(
					if cryAM.usedSkel.count > 0 then
						nodes = cryAM.usedSkel
				)catch()
				
				LoadSaveAnimation.saveAnimation filepath nodes (#()) (#())
				
				savedAnim = true
			)
			else
			(
				if charID > -1 and setupID > -1 then
				(
					local masterNode = undefined
					try
						masterNode = getNodeByName (cryAM.characterArray[charID].setup[setupID].name + "_Master")
					catch()
					
					if masterNode != undefined then
					(
						local tempSel = getCurrentSelection()
						
						select masterNode
						
						completeRedraw()
						
						try
						(
							masterNode.modifiers[1].custattributes[1].saveAnimationFn filepath false
							print "Saved with Crytek Custom Rig load function"
						)
						catch (print "Error in loading custom rig XAF file.")
						
						clearSelection()
						select tempSel
					)
				)
			)
		)
		
		"ama":
		(
			local offsetArray = #()
			
			if cryMaxTools.anim.vars.map.count > 0 then
			(
				for i = 1 to cryMaxTools.anim.vars.map.count do
				(
					if cryMaxTools.anim.vars.map[i].sourceMapID > 0 then
					(
						local tempTM = matrix3 1
						tempTM.rotation = cryMaxTools.anim.vars.map[i].rot
						tempTM.pos = cryMaxTools.anim.vars.map[i].pos
						
						append offsetArray (#(cryMaxTools.anim.vars.map[cryMaxTools.anim.vars.map[i].sourceMapID].nodeName, tempTM))
					)
				)
			)
			
			saveAMA filepath offset:offsetArray useTicks:useTicks clampFrames:clampFrames
			
			savedAnim = true
		)
	)
	
	if savedAnim == true then
	(
		if cryMaxTools.basic.perforceMan.perforceDir != "" and perforceAdd == true then
		(
			cryMaxTools.basic.perforceMan.cryAdd filepath
			try cryAM.addLog "          ---            Perforce :      Added" isAdvanced:true catch()
		)
	)
	
	cryMaxTools.anim.vars.updateList = undefined
)
saveAnim = undefined









cryMaxTools.anim.functions.UI.saveAnimCheck = function saveAnimCheck filepath quiet:undefined export:undefined useTicks:false clampFrames:false perforceAdd:false =
(
	if export != undefined then
	(--//generate export path
		local tempString = subString filepath (cryMaxTools.anim.vars.basePath.count + 1) filepath.count
		tempString = (subString tempString 1 (tempString.count - 3) ) + "caf"
		tempString = cryMaxTools.anim.vars.exportPath + tempString
		
		filepath = tempString
	)
		
	cryMaxTools.anim.vars.animSavePath = filepath
	
	
	case (cryMaxTools.basic.str.lower (cryMaxTools.basic.str.getExtension filepath)) of
	(
		"bip":
		(
			if selection.count > 0 then
			(
					--// biped found \\--
				if selection[1].classID[1] == 37157 then
					cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath bipRoot:selection[1].controller.rootNode perforceAdd:perforceAdd
				else
				(
					if quiet == undefined then
						messageBox "No Biped selected"
				)
			)
			else
			(
				local bipRoots = cryMaxTools.anim.functions.op.getBipRoots()
				
				try
				(
					if cryAM.usedSkel.count > 0 then
						bipRoots = #(cryAM.usedSkel[1])
				)catch()
				
				
				if bipRoots.count > 0 then
				(
					if bipRoots.count > 1 then
					(
						try (cryMaxTools.basic.ROMan.get "saveAnimBipedRO") catch()
						
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
							)
							
							
							on btnUse pressed do
							(
								if lbBiped.selection > 0 then
								(
									destroyDialog saveAnimBipedRO
									cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath bipRoot:bipRoots[lbBiped.selection]
								)
							)
							
							
							on btnCancel pressed do
								destroyDialog saveAnimBipedRO
						)
						
						cryMaxTools.basic.ROMan.cryAdd "saveAnimBipedRO" saveAnimBipedRO #main
						saveAnimBipedRO = undefined
						
						createDialog (cryMaxTools.basic.ROMan.get "saveAnimBipedRO") 120 120
					)
					else
						cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath bipRoot:bipRoots[1]
				)
			)
		)
		
		"max":
			cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath perforceAdd:perforceAdd
		
		"caf":
			cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath perforceAdd:perforceAdd
		
		"xaf":
			cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath perforceAdd:perforceAdd
		
		"ama":
			cryMaxTools.anim.functions.UI.saveAnim cryMaxTools.anim.vars.animSavePath perforceAdd:perforceAdd useTicks:useTicks clampFrames:clampFrames
		
		default: print ("File type not found")--messageBox "File type not found"
	)
)
saveAnimCheck = undefined








cryMaxTools.anim.functions.UI.selectByNameFilterNode = function selectByNameFilterNode obj =
(
	if obj.classID[1] != 37157 then
		return true
	else
		return false
)
selectByNameFilterNode = undefined




cryMaxTools.anim.functions.UI.selectByNameFilterParent = function selectByNameFilterParent objParent =
(
	try
	(
		if objParent.name != tempObj.name then
			return true
		else
			return false
	)
	catch return false
)
selectByNameFilterParent = undefined







cryMaxTools.anim.functions.UI.restartAndContinue = function restartAndContinue str openMaxFile:true =
(
	local timeDateFilter = (filterString localTime " ")
	local dateVar = filterString timeDateFilter[1] "."
	local timeDateStr = ""
	
	if dateVar.count == 1 then
		dateVar = filterString timeDateFilter[1] "/"
	
	local timeVar = filterString timeDateFilter[2] ":"
	
	for i = 1 to dateVar.count do
		timeDateStr += dateVar[i] + "_"
	
	for i = 1 to timeVar.count do
		timeDateStr += "_" + timeVar[i]
	
	
	local folderPath = (getDir #scripts) + "\\CryMaxRestart\\"
	local scriptPath = "restartAndContinue_" + timeDateStr + ".ms"
	local scriptFolderPath = folderPath + scriptPath
	
	--local numberStr = ""
	
	makeDir folderPath all:true
	
	/*
	for i = 1 to 100 do
	(
		numberStr = (if i < 10 then "0" else "") + i as String
		if doesFileExist (scriptPath + numberStr + ".ms") == false then
			exit
	)
	*/
			
	--if numberStr != "" then
	--(
		
		
		local file = createFile scriptFolderPath
		
		if openMaxFile == true then
		(
			local tempSel = getCurrentSelection()
			local selStr = ""
			
			if tempSel.count > 0 then
			(
				selStr += "select #("
				for i = 1 to tempSel.count do
					selStr += "$'" + tempSel[i].name + "'" + (if i < tempSel.count then "," else ")")
			)
				
			local maxFileStr = "loadMaxFile @\"" + maxFilePath + maxFileName + "\" quiet:true\n" + selStr + "\n\n"
			format maxFileStr to:file
		)
		
		--local removeMeStr = "deleteFile @\"" + scriptPath + "\""
		
		format str to:file
		--format removeMeStr to:file
		close file
		
		quitMAX #noPrompt
		shellLaunch (getDir #maxRoot + "3dsmax.exe") ("-U MAXScript " + "CryMaxRestart\\" + scriptPath)
	--)
)









cryMaxTools.anim.functions.UI.items.use = function use itemSelect =
(
	if cryMaxTools.anim.vars.itemList.count > 0 then
	(
		local itemArray = cryMaxTools.anim.vars.itemList
		
		try
		(
			local tempNodeForDelete = ($_IC_* as array)
			for i = 1 to tempNodeForDelete.count do
				delete tempNodeForDelete[i]
		)catch()
		
		if itemArray[itemSelect].name != "" then
		(
			if $Bip01 != undefined then
			(
				local itemUsed = #()
				local itemFound = #()
				
				for i = 1 to itemArray.count do
				(
					if i == itemSelect then
					(
						for d = 1 to itemArray[i].model.count do
						(
							local tempNode = getNodeByName itemArray[i].model[d]
							
							if tempNode == undefined then
							(
								mergeMAXfile (cryMaxTools.basic.vars.buildPath + "tools\\maxscript\\ref\\weapon_positions.max") #(itemArray[itemSelect].model[d]) #alwaysReparent #skipDups #useMergedMtlDups 
								try
								(
									local tempNode = getNodebyName itemArray[itemSelect].model
									
									append itemUsed tempNode
									append itemFound d
								)catch()
							)
							else
							(
								if (findItem itemFound d) == 0 then
								(
									append itemUsed tempNode
									append itemFound d
								)
								for f = 1 to itemArray[i].model.count do
								(
									if f != d and (findItem itemFound f) == 0 then
									(
										if itemArray[i].model[d] == itemArray[i].model[f] then
										(
											local tempCopy = (cryMaxTools.anim.functions.op.createSnapshot object:(getNodeByName itemArray[i].model[f]) )[1]
											tempCopy.name = "_IC_" + tempCopy.name
											tempCopy.isFrozen = (getNodeByName itemArray[i].model[f]).isFrozen
											
											append itemUsed tempCopy
											append itemFound f
										)	
									)
								)
							)
						)
					)
					else
					(
						for d = 1 to itemArray[i].model.count do
						(
							local tempNode = getNodeByName itemArray[i].model[d]
							
							if tempNode != undefined then
							(
								if tempNode.isHidden == false then
									tempNode.isHidden = true
							)
						)
					)	
				)
				
				for i = 1 to itemUsed.count do
				(
					if itemUsed[i].isHidden == true then
						itemUsed[i].isHidden = false
				)
				
				
				
				
				for i = 1 to itemArray[itemSelect].model.count do
				(
					try
					(
						if (local nodeRef = getNodeByName itemArray[itemSelect].reference[i]) != undefined then
						(
							in coordsys nodeRef
							(
								if (local nodeParent = getNodeByName itemArray[itemSelect].parent[i]) != undefined then
								(
									try
									(
										itemUsed[i].transform = nodeParent.transform
										itemUsed[i].parent = nodeParent
									)catch()
									
									with animate off
									(
										nodeParent.rotation = itemArray[itemSelect].rotation[i]
										nodeParent.position = itemArray[itemSelect].position[i]
									)
								)
							)
						)
					)catch()
				)
				
				
				try (cryMaxTools.basic.ROMan.get "settingsRO").ddItems.selection = itemSelect catch()
				return true
			)
		)
	)
	return false
)
use = undefined





cryMaxTools.anim.functions.UI.items.getIndex = function getIndex =
(
	if cryMaxTools.anim.vars.itemList.count > 0 then
	(
		for i = 1 to cryMaxTools.anim.vars.itemList.count do
		(
			local tempModels = cryMaxTools.anim.vars.itemList[i].model
			local found = false
			for d = 1 to tempModels.count do
			(
				local tempNode = getNodeByName tempModels[d]
				if tempNode != undefined then
				(
					if tempNode.isHidden == false then
						found = true
					else
						found = false
				)
			)
			if found == true then
					return i
		)
		return 1
	)
	else
		return 0
)
getIndex = undefined






cryMaxTools.anim.functions.UI.items.updateExtent = function updateExtent =
(
	newExtent = #(0,0,0)
	local lbList = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList
	
	for i = 0 to (lbList.Items.count - 1) do
	(
		local tempItem = lbList.Items.item i
		local textExtent = #(0,0,0)
		--// extent of the current filename
		textExtent[1] = (getTextExtent (tempItem.SubItems.item 0).text)[1] + 12
		textExtent[2] = (getTextExtent (tempItem.SubItems.item 1).text)[1]
		textExtent[3] = (getTextExtent (tempItem.SubItems.item 2).text)[1]
		
		for f = 1 to textExtent.count do
				--// if the current extent is higher than the result extent
			if textExtent[f] > newExtent[f] then
						--// result extent is current extent
				newExtent[f] = textExtent[f] + 12 --// additional number so entries are not cut
	)
	
	for f = 1 to newExtent.count do
		if (lbList.Columns.item (f - 1)).width < newExtent[f] then
			--// applys the weidth of the column with the text extent
			(lbList.Columns.item (f - 1)).width = newExtent[f]
)
updateExtent = undefined













cryMaxTools.anim.functions.op.getBipRoots = function getBipRoots =
(
	local rootArray = #()
	for obj in Objects do
	(
		if obj.classID[1] == 37157 then
			if (findItem rootArray obj.controller.rootNode) == 0 then
				append rootArray obj.controller.rootNode
	)
	
	return rootArray
)
getBipRoots = undefined












cryMaxTools.anim.functions.op.getBaseBip = function getBaseBip op:undefined =
(
	local baseBip = undefined
	try
	(
		baseBip = selection[1].controller.rootNode
	)
	catch
	(
		baseBip = (cryMaxTools.anim.functions.op.getBipRoots())[1]
	)
		
	if baseBip == undefined then
	(
		if op == #load then
			messageBox "No Biped in Scene." title:"Load File"
		else
			print "No Biped in Scene"
	)
	
	return baseBip
)
getBaseBip = undefined









cryMaxTools.anim.functions.op.getBipNames = function getBipNames index =
(
	if selection.count > 0 then
	(
		local tempSel = (selection as Array)[1]
		if tempSel.classID[1] == 37157 then
		(
			case index of
			(
				/*Hand*/1:	( tempNode = biped.getNode tempSel #rArm link:4 )
				/*Foot*/2:	( tempNode = biped.getNode tempSel #rLeg link:3 )
				/*Toe0*/3:	( tempNode = biped.getNode tempSel #rToes link:1 )
				/*Finger*/4: ( return "Finger" )
			)
	
	
				--// stores the last part of the string
			tempFilterString = (filterString tempNode.name " ")[3]
		
			return tempFilterString
		)	
	)
	
	return undefined
)
getBipNames = undefined








cryMaxTools.anim.functions.op.getSelectedBipPart = function getSelectedBipPart =
(
		--// defines using "standard" constructor
	tempFilter = bipPartSt object:(objectSt isPivot:false) part:(partNameSt()) pivotSel:(pivotSelSt pivPoint:(pivPointSt()) )

		--// gets selected biped object
	selectedBipPart = getCurrentSelection()
	
		--// stores the selected object in tempFilter	
	if selectedBipPart[1] != undefined then
		tempFilter.object.selected = selectedBipPart[1]
	else return undefined
	
	if tempFilter.object.selected.superClassID != 16 then
		return undefined
	
		--// filters the name using space (" ") for seperation
	tempFilterString = (filterString selectedBipPart[1].name " ")		
	-- check if selected part is child of bip01
	tempNodeParent = #()
	validBipedPart = false
	for i = 1 to 20 do
	(
		if tempNodeParent.count == 0 then
				--// initialise with the selected object
			append tempNodeParent tempFilter.object.selected
		else
				--// if the latest object is not undefined, then add the parent to the array
			if tempNodeParent[tempNodeParent.count] != undefined then
				append tempNodeParent tempNodeParent[tempNodeParent.count].parent
			--// if parent is undefined (end of parentList) then break the loop
		if tempNodeParent[tempNodeParent.count] == undefined then
			break
	)
	if selectedBipPart[1].classID[1] != 37157 then
		return undefined
	else
			--// if Bip01 is in in the parent list, then set validBipedPart to true
		validBipedPart = true
	
			
		------// stores last part (body part, for example: "Foot" or "Hand" etc.) in .part
		----// necessary for resetRotation to get every possible biped part
		--// if there is more than 2 substrings (i.e. "Bip01 L Hand" or "Bip01 R Foot")		
	if tempFilterString.count > 2 then
	(
		tempFilter.part.leftRight = tempFilterString[2]
		tempFilter.part.bodyPart = tempFilterString[3]
		tempFilter.part.name = tempFilterString[2] + tempFilterString[3]
	)
		--// if there is less than 3 substrings (i.e. "Bip01 Spine" or "Bip01 Head"
	else
	(
		tempFilter.part.leftRight = ""
		tempFilter.part.bodyPart = tempFilterString[tempFilterString.count]
		tempFilter.part.name = tempFilterString[tempFilterString.count]
	)
	
	if findString tempFilter.part.bodyPart "Finger" != undefined then
		tempFilter.part.bodyPart = substring tempFilter.part.bodyPart 1 6
		
		--// stores the name of the selected object
	tempFilter.name = tempFilter.object.selected.name
	
	ok = false
	tempBipNames = #()
	
	for i = 1 to 3 do
		tempBipNames[i] = cryMaxTools.anim.functions.op.getBipNames i

	
	function getObjectParent baseObject = 
	(
		try
		(
			for i = 1 to 20 do
			(
				if baseObject != undefined then
				(
					if findString baseObject.parent.name "Hand" != undefined  then
						return baseObject.parent
					else
						baseObject = baseObject.parent
				)				
			)
		)
		catch ()
		
		return undefined
	)
	
	
	case tempFilter.part.bodyPart of
	(		
		/*Hand*/(cryMaxTools.anim.functions.op.getBipNames 1): ( ok = true )
		/*Finger*/(cryMaxTools.anim.functions.op.getBipNames 4): ( ok = true
				  tempFilter.part.bodyPart = (cryMaxTools.anim.functions.op.getBipNames 1)
				  tempFilter.part.name = tempFilter.part.leftRight + tempFilter.part.bodyPart
				  tempFilter.object.child = tempFilter.object.selected
				  tempFilter.object.selected = getObjectParent tempFilter.object.child
				  tempFilter.name = tempFilter.object.selected.name
				)
		/*Foot*/(cryMaxTools.anim.functions.op.getBipNames 2):  ( ok = true )
		/*Toe0*/(cryMaxTools.anim.functions.op.getBipNames 3): ( ok = true 
					  --// only necessary for resetRotation() to check, if the toe or finger is selected
				  tempFilter.part.bodyPart = (cryMaxTools.anim.functions.op.getBipNames 2)
				  tempFilter.part.name = tempFilter.part.leftRight + tempFilter.part.bodyPart
				  tempFilter.object.child = tempfilter.object.selected
				  tempFilter.object.selected = tempFilter.object.selected.parent
				  tempFilter.name = tempFilter.object.selected.name
				)
	)
	
	
	if tempFilter.object.selected != $Bip01 then
		tempFilter.object.parent = tempFilter.object.selected.parent
		
	if ok == true then
		tempFilter.object.isPivot = true
	
	if tempFilter.object.isPivot == true then
	(
		try (tempFilter.pivotSel.index = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".index") )catch( print "Error getting pivotSel.index" )
		try (tempFilter.pivotSel.pivotName = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".pivotName") ) catch(print "Error getting pivotSel.pivotName" )
		try (tempFilter.pivotSel.position = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".position") ) catch( print "Error getting pivotSel.position" )
		try (tempFilter.pivotSel.pivObject = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".pivObject") ) catch( print "Error getting pivotSel.pivObject" )
		try (tempFilter.pivotSel.pivPoint.name = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".pivPoint.name") ) catch( print "Error getting pivotSel.pivPoint.name" )
		try (tempFilter.pivotSel.pivPoint.pointPos = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".pivPoint.pointPos") ) catch( print "Error getting pivotSel.pivPoint.pointPos" )
		try (tempFilter.pivotSel.pivPoint.snapTM = execute ("cryMaxTools.anim.align.vars.pivotLast." + tempFilter.part.name + ".pivPoint.snapTM") ) catch( print "Error getting pivotSel.pivPoint.snapTM" )
	)
			
	
	if validBipedPart == true then
		return tempFilter
	else
			--// if no biped part for pivot selection is selected, then return
		return undefined
)
getSelectedBipPart = undefined




crymaxTools.anim.functions.op.sortList = function sortList sortArray =
(
	local returnArray = #()
	local sortList = #()
	
	for i = 1 to sortArray.count do
		append sortList sortArray[i].name
	
	sortList = sort sortList
	
	for i = 1 to sortList.count do
	(
		for f = 1 to sortArray.count do
			if sortArray[f].name == sortList[i] then
				append returnArray sortArray[f]
	)
	
	return returnArray
)
sortList = undefined





cryMaxTools.anim.functions.op.getChildren = function getChildren baseObject showHidden:undefined =
(
	function childToArray childArray showHidden =
	(
		childArray = cryMaxTools.anim.functions.op.sortList childArray
		
		local tempArray = #()
		for i = 1 to childArray.count do
		(
			if childArray[i].classID[1] == 37157 then
			(
				if showHidden == undefined then
				(
					if childArray[i].isHidden == false and childArray[i].isFrozen == false then				
						append tempArray childArray[i]
				)
				else
					append tempArray childArray[i]
			)
			else
				append tempArray childArray[i]
		)
		
		return tempArray
	)
	
	
	local baseArray = childToArray baseObject.children showHidden
	

	for obj in baseArray do
	(
		if obj.children.count != 0 then
		(
			local tempArray = childToArray obj.children showHidden
			join baseArray tempArray
		)
	)			
	
	
	return baseArray
)
getChildren = undefined







cryMaxTools.anim.align.functions.getPivotPointPos = function getPivotPointPos index baseNode =
(
	vertPos = undefined
	
	selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	
	local baseMesh = snapshotAsMesh baseNode
	
	case (selectedBipPart.part.bodyPart) of
	(
		
			--// checks what biped part is selected
		/*Foot*/(cryMaxTools.anim.functions.op.getBipNames 2):
		(
			local child = undefined
			try ( child = baseNode.children[1] ) catch()
			
				
			
			
			--case (index) of
			--(
				if index > getNumVerts baseMesh then
				(
					if child != undefined then
					(
						local childMesh = snapshotAsMesh child
						vertPos = in coordsys world (getVert childMesh (index - (getNumVerts baseMesh)))
					)
				)
				else
					try vertPos = (in coordsys world (getVert baseMesh index)) catch()
				
				
					/*
					--// returns the pivot position values of the selected biped object
				1: vertPos = (in coordsys world baseNode.transform.pos)
				2: vertPos = (in coordsys world (getVert baseNode 7))
				3: vertPos = (in coordsys world ( ((getVert baseNode 6) + (getVert baseNode 7)) / 2) )
				4: vertPos = (in coordsys world (getVert baseNode 6))
				5: vertPos = (in coordsys world (getVert baseNode 3))
				6: vertPos = (in coordsys world ( ((getVert baseNode 2) + (getVert baseNode 3)) / 2) )
				7: vertPos = (in coordsys world (getVert baseNode 2))
				8: vertPos = (in coordsys world (getVert child 2))
				9: vertPos = (in coordsys world ( ((getVert child 2) + (getVert child 6)) / 2) )
				10: vertPos = (in coordsys world (getVert child 6))
					*/
			--)
		)
		/*Hand*/(cryMaxTools.anim.functions.op.getBipNames 1):
		(
			local childArray = cryMaxTools.anim.functions.op.getChildren selectedBipPart.object.selected
			
			
			
			
			if index > getNumVerts baseMesh then
			(
				counter = getNumVerts baseMesh
				
				for i = 1 to childArray.count do
				(
					if findString childArray[i].name "Finger" != undefined then
					(
						local foundEntry = false
						local childMesh = snapshotAsMesh childArray[i]
						
						for d = 1 to (getNumVerts childMesh) do
						(
							counter += 1
							
							if index == counter then
							(
								vertPos = in coordsys world (getVert childMesh d)
								foundEntry = true
								exit
							)
						)
						
						if foundEntry == true then
							exit
					)
				)
			)
			else
				vertPos = in coordsys world (getVert baseMesh index)
			
			/*
			case (index) of
			(				
					--// STUPID EDIT POLY --
				1: vertPos = (in coordsys world baseNode.transform.pos )
				2: vertPos = (in coordsys world (getVert baseNode 8))
				3: vertPos = (in coordsys world ( ((getVert baseNode 8) + (getVert baseNode 4)) / 2) )
				4: vertPos = (in coordsys world (getVert baseNode 4))
				5: vertPos = (in coordsys world (getVert baseNode 7))
				6: vertPos = (in coordsys world ( ((getVert baseNode 7) + (getVert baseNode 3)) / 2) )
				7: vertPos = (in coordsys world (getVert baseNode 3))
			)
			
			/*
			if index > 7 then
				vertPos = (in coordsys world ( ((getVert childArray[index - 7] 7) + (getVert childArray[index - 7] 3)) / 2) )
			*/
		)	
	)
		
	return vertPos
)
getPivotPointPos = undefined







cryMaxTools.anim.align.functions.getNearestSnapshot = function getNearestSnapshot noPivot:false =
(	
	local obj = (getCurrentSelection())[1]
	
	if obj != undefined then
	(
		local foundPivot = 0
		local foundTarget = undefined
		
		for tempObj in objects do
			if tempObj.name == ("_" + obj.name + "_pivotTarget") then
				foundTarget = tempObj
		
		for i = 1 to obj.children.count do
			if obj.children[i].name == ("_" + obj.name + "_pivotSource") then
				foundPivot = i
		
		
		if foundTarget != undefined then
		(
			if noPivot == false then
			(
				if foundPivot > 0 then
					return true
			)
			else
				return true
		)
	)
	
	return undefined
)
getNearestSnapshot = undefined










cryMaxTools.anim.align.functions.moveBipPart = function moveBipPart = 
(
	local obj = (getCurrentSelection())[1]
	
	if obj != undefined then
	(
		if (cryMaxTools.anim.align.functions.getNearestSnapshot noPivot:true) == true then
		(
			local foundPivot = 0
			local foundTarget = undefined
		
			for tempObj in objects do
				if tempObj.name == ("_" + obj.name + "_pivotTarget") then
					foundTarget = tempObj
		
			for i = 1 to obj.children.count do
				if obj.children[i].name == ("_" + obj.name + "_pivotSource") then
					foundPivot = i
			
				
			if foundTarget != undefined then
			(
				local newTM = obj.transform
				newTM.pos = foundTarget.transform.pos
				
				if foundPivot > 0 then
				(
					local pivTM = obj.children[foundPivot].transform * inverse obj.transform
					local locPivPos = obj.children[foundPivot].transform.pos - obj.transform.pos
					
					local newPivotTM = foundTarget.transform
					newPivotTM.pos -= locPivPos
				
					newTM.pos = newPivotTM.pos
				)

				if obj.classID[1] == 37157 then
				(
					for i = 1 to 15 do
					(
						local tempRot = obj.transform.rotation
						biped.setTransform obj #pos newTM.pos true
						biped.setTransform obj #rotation newTM.rotation true
					)
				)
				else
					obj.transform = newTM
				
				return true
			)
		)
	)
	
	return false
)
moveBipPart = undefined






/*
cryMaxTools.anim.align.functions.moveRotBipPart = function moveRotBipPart pivotPoint:undefined =
(
		--// get selected object and check if not unselected
	local pivotIndex = undefined
	
	if ( selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart() ) != undefined then
	(
		--if selectedBipPart.object.isPivot == true then
		(
			if selectedBipPart.pivotSel.index == undefined then
				pivotIndex = 1
			else
				pivotIndex = selectedBipPart.pivotSel.index
		
				
			--// gets nearest snapshot
			
			
			if pivotPoint == undefined then
			(
				tempSnap = cryMaxTools.anim.align.functions.getNearestSnapshot()
				if tempSnap != undefined then
				(
						--// gets the rotation of snapshot
					tempRot = tempSnap.rotation
						--// inverts the angle of rotation
					tempRot.w = tempRot.w - (tempRot.w * 2)
				)
			)
			else
				if selectedBipPart.pivotSel.pivPoint.snapTM != undefined then
					tempRot = selectedBipPart.pivotSel.pivPoint.snapTM.rotation
			
			if tempRot != undefined then
			(
					--// checks, if the toe pivot is selected
				if pivotIndex <= 7 then
				(
						--// if available, then apply rotation of the snapshot to the selected biped object
					biped.setTransform selectedBipPart.object.selected #rotation tempRot true
				)
				else
				(
						--// if toe is selected, then apply rotation to the toe (child of foot)
					biped.setTransform selectedBipPart.object.selected.children[1] #rotation tempRot true
				)
			)
			else
				return false
		)
	)
		--// tries to translate the biped object to the snapshot
	if (cryMaxTools.anim.align.functions.moveBipPart pivotPoint:pivotPoint) == true then	
		return true
	else
		return false
)
moveRotBipPart = undefined

*/

cryMaxTools.anim.align.functions.moveRotBipPart = function moveRotBipPart =
(
	local obj = (getCurrentSelection())[1]
		
	if obj != undefined then
	(
		local foundPivot = 0
		local foundTarget = undefined
	
		for tempObj in objects do
			if tempObj.name == ("_" + obj.name + "_pivotTarget") then
				foundTarget = tempObj
		
		for i = 1 to obj.children.count do
			if obj.children[i].name == ("_" + obj.name + "_pivotSource") then
				foundPivot = i
	
		
		if foundTarget != undefined then
		(
			local newTM = foundTarget.transform
			
			if foundPivot > 0 then
			(
				local pivTM = obj.children[foundPivot].transform * inverse obj.transform
				local newPivTM = pivTM * newTM
				newTM.pos -= (newPivTM.pos - newTM.pos)
			)
			
			
			if obj.classID[1] == 37157 then
			(
				for i = 1 to 15 do
				(
					biped.setTransform obj #pos newTM.pos true
					biped.setTransform obj #rotation newTM.rotation true
					biped.setTransform obj #pos newTM.pos true
				)
			)
			else
			(
				local tempTM = obj.transform
				tempTM.rotation = newTM.rotation
				tempTM.pos = newTM.pos
				obj.transform = tempTM
			)
		)
	)
)
moveRotBipPart = undefined












cryMaxTools.anim.align.functions.createPivotEdge = function createPivotEdge index positionFrom positionTo =
(
	if (alignLayer = layerManager.newLayerFromName "alignTool") == undefined then
		alignLayer = layerManager.getLayerFromName "alignTool"
		
	alignLayer.current = true
				
		--// gets selected biped object
	selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	
		--// creates a cylinder and stores it in edgeList
	cryMaxTools.anim.align.vars.edgeList.pivot[index] = cylinder()
		--// creates and stores name, and other various values
	cryMaxTools.anim.align.vars.edgeList.pivot[index].name = ("e" + (index as String) + "_(" + selectedBipPart.name + ")")
	cryMaxTools.anim.align.vars.edgeList.pivot[index].radius = 0.1
	cryMaxTools.anim.align.vars.edgeList.pivot[index].sides = 5
		--// sets position to the inital position
	cryMaxTools.anim.align.vars.edgeList.pivot[index].position = positionFrom
		--// calculates distance between 2 pivot points and applies to the height
	cryMaxTools.anim.align.vars.edgeList.pivot[index].height = distance positionFrom positionTo
	
				
	
	if selectedBipPart.pivotSel.index != undefined then
	(
			--// checks, if this pivotEdge was used before	
		--if cryTools.cryAnim.align._v.edgeList.pivot[index].index == selectedBipPart.pivotSel.index then
		if index == selectedBipPart.pivotSel.index then
				--// if used, set the color to red
			cryMaxTools.anim.align.vars.edgeList.pivot[index].wireColor = (color 255 0 0)
		else
				--// otherwise to "edgeBlue"
			cryMaxTools.anim.align.vars.edgeList.pivot[index].wireColor = (color 20 50 100)
	)
	else 
		cryMaxTools.anim.align.vars.edgeList.pivot[index].wireColor = (color 20 50 100)
	
		--// generates a tape with target for excact rotation of the cylinder
	tempTape = tape pos:positionFrom target:(targetObject pos:positionTo)
		--// applies transform from generated tape to the cylinder
	cryMaxTools.anim.align.vars.edgeList.pivot[index].transform = tempTape.transform
		--// the cylinder is rotated 180°, so it will be rotated to the other side of the pivot
	cryMaxTools.anim.align.vars.edgeList.pivot[index].position = positionTo
	
		--// temporary tape will be deleted
	cryMaxTools.anim.align.vars.edgeList.selection[index] = cylinder radius:0.5 sides:5 height:cryMaxTools.anim.align.vars.edgeList.pivot[index].height
	cryMaxTools.anim.align.vars.edgeList.selection[index].name = ("sE" + (index as String) + "_(" + selectedBipPart.name + ")")
	cryMaxTools.anim.align.vars.edgeList.selection[index].transform = tempTape.transform
	cryMaxTools.anim.align.vars.edgeList.selection[index].position = positionTo
		--// hides the node (intersectRay still finds the mesh intersection of hidden nodes)
	cryMaxTools.anim.align.vars.edgeList.selection[index].isHidden = true

	delete tempTape
	
	tempBaseLayer = layerManager.getLayer 0
	tempBaseLayer.current = true
	
	return true
)
createPivotEdge = undefined









cryMaxTools.anim.align.functions.createPivotPoint = function createPivotPoint index position =
(
	if position == undefined then
	(
		print ("Index : " + index as String)
		return false
	)
	
	if (alignLayer = layerManager.newLayerFromName "alignTool") == undefined then
		alignLayer = layerManager.getLayerFromName "alignTool"
	
	alignLayer.current = true


		--// gets selected object
	selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
		--// generates sphere (point)	
	cryMaxTools.anim.align.vars.pointList.pivot[index] = sphere()
		--// sets name and other values
	cryMaxTools.anim.align.vars.pointList.pivot[index].name = ("p" + (index as String) + "_(" + selectedBipPart.name + ")")				
	cryMaxTools.anim.align.vars.pointList.pivot[index].radius = 0.5
	cryMaxTools.anim.align.vars.pointList.pivot[index].segs = 10		
	cryMaxTools.anim.align.vars.pointList.pivot[index].position = position
	
	
	if selectedBipPart.pivotSel.index != undefined then
	(
			--// checks, if the pivot was used before
		--if cryTools.cryAnim.align._v.pointList.pivot[index]. == selectedBipPart.pivotSel.index then
		if index == selectedBipPart.pivotSel.index then
				--// if used, the color will be set to red
			cryMaxTools.anim.align.vars.pointList.pivot[index].wireColor = (color 255 0 0)
		else
				--// otherwise to blue
			cryMaxTools.anim.align.vars.pointList.pivot[index].wireColor = (color 0 0 255)
	)
	else
		cryMaxTools.anim.align.vars.pointList.pivot[index].wireColor = (color 0 0 255)
				
		--// generates the selection area for pivot points		
	cryMaxTools.anim.align.vars.pointList.selection[index] = sphere pos:cryMaxTools.anim.align.vars.pointList.pivot[index].pos radius:1
	cryMaxTools.anim.align.vars.pointList.selection[index].name = ("sP" + (index as String) + "_(" + selectedBipPart.name + ")")				
	
		--// hides the node (intersectRay still finds the mesh intersection of hidden nodes)
	cryMaxTools.anim.align.vars.pointList.selection[index].isHidden = true
	
	tempBaseLayer = layerManager.getLayer 0
	tempBaseLayer.current = true

	
	return true
)
createPivotPoint = undefined






cryMaxTools.anim.align.functions.createPointEdge = function createPointEdge =
(
		--// gets selected object
	local selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	local editMeshAdded = #()
	
	if selectedBipPart != undefined then
	(
		with redraw off
		(
			local baseMeshNotFound = undefined
			
			try ( getVert selectedBipPart.object.selected 1 ) catch ( baseMeshNotFound = true)
			
			local baseMesh = snapshotAsMesh selectedBipPart.object.selected
			/*
			if baseMeshNotFound == true then
			(
				addModifier selectedBipPart.object.selected (edit_mesh())
				append editMeshAdded selectedBipPart.object.selected
			)
			*/
			
			local counter = 0
			
				--// checks, what body part is selected
			case (selectedBipPart.part.bodyPart) of
			(
				/*Foot*/(cryMaxTools.anim.functions.op.getBipNames 2):
				(
					local child = undefined
					try ( child = selectedBipPart.object.selected.children[1] ) catch()
					
					for i = 1 to (getNumVerts baseMesh) do
					(
						counter += 1
							--// creates pivot points for the hand
						cryMaxTools.anim.align.functions.createPivotPoint counter (cryMaxTools.anim.align.functions.getPivotPointPos counter selectedBipPart.object.selected )
					)
					
					
					if child != undefined then
					(
						local childMesh = snapshotAsMesh child
						
						/*
						if meshNotFound == true then
						(
							addModifier child (edit_mesh())
							append editMeshAdded child
						)
						*/
						
						for i = 1 to (getNumVerts childMesh) do
						(
							counter += 1
							
							cryMaxTools.anim.align.functions.createPivotPoint counter (cryMaxTools.anim.align.functions.getPivotPointPos counter selectedBipPart.object.selected)
						)
					)
					
					
					
					/*
					for i = 1 to 10 do
					(
							--// generates pivot points
						cryMaxTools.anim.align.functions.createPivotPoint i (cryMaxTools.anim.align.functions.getPivotPointPos i selectedBipPart.object.selected)
							--// generates pivot edges
						case i of
						(
							2:( cryMaxTools.anim.align.functions.createPivotEdge 1 ( cryMaxTools.anim.align.functions.getPivotPointPos 2 selectedBipPart.object.selected )	( cryMaxTools.anim.align.functions.getPivotPointPos 5 selectedBipPart.object.selected ) )
							5:( cryMaxTools.anim.align.functions.createPivotEdge 2 ( cryMaxTools.anim.align.functions.getPivotPointPos 5 selectedBipPart.object.selected )	( cryMaxTools.anim.align.functions.getPivotPointPos 7 selectedBipPart.object.selected ) )
							7:( cryMaxTools.anim.align.functions.createPivotEdge 3 ( cryMaxTools.anim.align.functions.getPivotPointPos 7 selectedBipPart.object.selected )	( cryMaxTools.anim.align.functions.getPivotPointPos 4 selectedBipPart.object.selected ) )
							4:( cryMaxTools.anim.align.functions.createPivotEdge 4 ( cryMaxTools.anim.align.functions.getPivotPointPos 4 selectedBipPart.object.selected )	( cryMaxTools.anim.align.functions.getPivotPointPos 2 selectedBipPart.object.selected ) )
						)
					)
					*/
				)
				/*Hand*/(cryMaxTools.anim.functions.op.getBipNames 1):
				(
						--// gets children of the selected object (fingers)
					local childArray = cryMaxTools.anim.functions.op.getChildren selectedBipPart.object.selected
					
					
					
					for i = 1 to (getNumVerts baseMesh) do
					(
						counter += 1
							--// creates pivot points for the hand
						cryMaxTools.anim.align.functions.createPivotPoint counter (cryMaxTools.anim.align.functions.getPivotPointPos counter selectedBipPart.object.selected )
						
					)
					
					
					
					for i = 1 to (childArray.count) do
					(
						if findString childArray[i].name "Finger" != undefined then
						(
							local childMesh = snapshotAsMesh childArray[i]
							
							for d = 1 to (getNumVerts childMesh) do
							(
								counter += 1
									--// creates pivot points for the fingers
								cryMaxTools.anim.align.functions.createPivotPoint counter (cryMaxTools.anim.align.functions.getPivotPointPos counter selectedBipPart.object.selected )
							)
						)
					)
				)
			)
		)
	)
	else
		#stop
)
createPointEdge = undefined






cryMaxTools.anim.align.functions.initVars = function initVars =
(
		--// if there are still pivots in the scene, delete old pivots by deletePivotPoints() function
	try
	(
		if cryMaxTools.anim.align.vars.pointList.pivot.count > 0 then
			cryMaxTools.anim.align.functions.deletePivotPoints()
	) catch()

	return true
)
initVars = undefined







cryMaxTools.anim.align.functions.interactPivotPoint = function interactPivotPoint =
(
		--// gets selected bip object
	local selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	
		--// var initialisation part			
	local worldRay = undefined
	local tempDistance = #()
		
	local activePivot = #()
	local nearestPivot = undefined
	
		--// stores all points and edges in pointEdgeSt structure via array
	local pointEdgeList = pointEdgeSt pivot:(cryMaxTools.anim.align.vars.pointList.pivot + cryMaxTools.anim.align.vars.edgeList.pivot) selection:(cryMaxTools.anim.align.vars.pointList.selection + cryMaxTools.anim.align.vars.edgeList.selection)
		
	for i = 1 to pointEdgeList.selection.count do
	(
			--// generates a ray from the mousecursor position into the viewport
		worldRay = mapScreenToWorldRay (in coordsys screen mouse.pos)
			--// checks, if the mouse cursor is over a pivot point or edge (worldRay intersects with geometry)
		tempPivotRay = intersectRay pointEdgeList.selection[i] worldRay
		
			--// if the ray intersects with the pivot selection
		if tempPivotRay != undefined then
		(
				--// if the pivot selection is not the pivot selected already
			if pointEdgeList.pivot[i].name != selectedBipPart.pivotSel.pivotName then
					--// adds the hovered pivot point or edge to the activePivot list struct
				append activePivot (activePivotSt interPivot:pointEdgeList.pivot[i] interPos:tempPivotRay.pos)
		)
	)			
	
		--// if a pivot is hovered
	if activePivot.count > 0 then
	(
			--// if more than 1 pivot is hovered (happens when in left or front view for example
		if activePivot.count > 1 then
		(
			for i = 1 to activePivot.count do
					--// stores all distances between the cursor pos (screen) and intersection position of the pivot point or edge
				tempDistance[i] = (distance worldRay.pos activePivot[i].interPos)									
			
			for i = 1 to activePivot.count do
			(
				if nearestPivot == undefined then
					-- if there is no nearestPivot before, make one
					nearestPivot = activePivot[i]
				else
						--// checks, if the distance of the activePivot before, has a bigger distance then the current one
					if (tempDistance[i]) < ( distance worldRay.pos nearestPivot.interPos ) then
							--// if so, the current activePivot is the new one
						nearestPivot = activePivot[i]
			)
				--// interPivot (the pivot point or edge) is the new nearestPivot
			nearestPivot = nearestPivot.interPivot
		)
		else
				--// if there is no further pivot point or edge in line, take the first one in list
			nearestPivot = activePivot[1].interPivot
	)
	else
			--// if no pivot point or edge is hovered, set nearestPivot to undefined
		nearestPivot = undefined

	if nearestPivot != undefined then
	(
			--// if the hovered nearest pivot is not an already selected one
		if nearestPivot.name != selectedBipPart.pivotSel.pivotName then
		(
				--// set the wireColor to the hovered color
			nearestPivot.wireColor = (color 250 190 30)
				--// and set to tempSelection - used for clickAction
			cryMaxTools.anim.align.vars.pivotLast.tempSelection = nearestPivot
		)
	)
	else
			--// if there is no pivot selected, set tempSelection - used for clickAction - to undefined
		cryMaxTools.anim.align.vars.pivotLast.tempSelection = undefined

	for i = 1 to pointEdgeList.pivot.count do
	(
		if pointEdgeList.pivot[i].name != selectedBipPart.pivotSel.pivotName then
		(
			if pointEdgeList.pivot[i] != nearestPivot then
			(
					--// if no pivot point or edge is already selected or hovered, then apply wireColor
				if (substring pointEdgeList.pivot[i].name 1 1) == "p" then
						--// for points
					pointEdgeList.pivot[i].wireColor = (color 0 0 255)
				else
						--// for edges
					pointEdgeList.pivot[i].wireColor = (color 20 50 100)
			)
		)
	)
)
interactPivotPoint = undefined









cryMaxTools.anim.align.functions.setPivotPoint = function setPivotPoint tempPivot =
(
	local selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	
		--// filters the name to have only the number of the pivot
	local tempFilterString =  filterString tempPivot.name "_"
	tempFilterString = substring tempFilterString[1] 2 tempFilterString[1].count

		--// sets the pivotList.pivotLast part to the selected pivot
	execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".index = " + tempFilterString )
	execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".pivotName = \"" + tempPivot.name + "\"")
	execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".position = " + (tempPivot.pos as String))
	local childArray = cryMaxTools.anim.functions.op.getChildren selectedBipPart.object.selected
	local tempPivotIndex = tempFilterString as Integer		
	
		
	if tempPivotIndex <= 7 then
		execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + selectedBipPart.object.selected.name + "\"" )
	else
	(
		if selectedBipPart.part.bodyPart == "Hand" then
			execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + childArray[tempPivotIndex - 7].name + "\"" )
		else
			execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name +  ".pivObject = \"" + childArray[1].name + "\"" )
	)
	return true
)
setPivotPoint = undefined





cryMaxTools.anim.align.functions.clickAction = function clickAction =
(
	local selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
		--// checks, if there was a pivot selected before
	if selectedBipPart == undefined then return false
	
		--// if no pivot is selected before
	if selectedBipPart.pivotSel.index == undefined then
	(			
			--// if there is a pivot currently hovered
		if cryMaxTools.anim.align.vars.pivotLast.tempSelection != undefined then
		(
				--// on mouse click, set color of the current pivot to red
			cryMaxTools.anim.align.vars.pivotLast.tempSelection.wireColor = (color 255 0 0)

				--// stores all the infos in the state variable for use of other _f					
			cryMaxTools.anim.align.functions.setPivotPoint cryMaxTools.anim.align.vars.pivotLast.tempSelection					
		)
	)
		--// if a pivot were selected before		
	else
	(
			--// if a pivot is currently hovered
		if cryMaxTools.anim.align.vars.pivotLast.tempSelection != undefined then
		(					
				--// on mouseclick sets color to red
			cryMaxTools.anim.align.vars.pivotLast.tempSelection.wireColor = (color 255 0 0)
				--// stores the values in the variable for the use of other _f
			cryMaxTools.anim.align.functions.setPivotPoint cryMaxTools.anim.align.vars.pivotLast.tempSelection
				
			for i = 1 to cryMaxTools.anim.align.vars.pointList.pivot.count do
			(
					--// checks, if the pivot in list is not the current pivot
				if cryMaxTools.anim.align.vars.pointList.pivot[i] != cryMaxTools.anim.align.vars.pivotLast.tempSelection then
						--// and sets the color to blue (because of not selected)
					cryMaxTools.anim.align.vars.pointList.pivot[i].wireColor = (color 0 0 255)							
			)

			for i = 1 to cryMaxTools.anim.align.vars.edgeList.pivot.count do
			(
					--// checks, if the current pivot is not the current edge
				if cryMaxTools.anim.align.vars.edgeList.pivot[i] != cryMaxTools.anim.align.vars.pivotLast.tempSelection then
						--// and sets the color to "edgeBlue"
					cryMaxTools.anim.align.vars.edgeList.pivot[i].wireColor = (color 20 50 100)
			)
		)				
	)
)
clickAction = undefined







cryMaxTools.anim.align.functions.deletePivotPoints = function deletePivotPoints =
(
	for i = 1 to cryMaxTools.anim.align.vars.pointList.pivot.count do
	(				
		try
		(		
				--// tries to delete the pivot points
			delete cryMaxTools.anim.align.vars.pointList.pivot[i]
			delete cryMaxTools.anim.align.vars.pointList.selection[i]
		)catch()
	)
	
	for i = 1 to cryMaxTools.anim.align.vars.edgeList.pivot.count do
	(
		try
		(
				--// tries to delete the pivot edges
			delete cryMaxTools.anim.align.vars.edgeList.pivot[i]
			delete cryMaxTools.anim.align.vars.edgeList.selection[i]
		)catch()
	)
		
	cryMaxTools.anim.align.vars.pointList.pivot = #()
	cryMaxTools.anim.align.vars.pointList.selection = #()
	
	cryMaxTools.anim.align.vars.edgeList.pivot = #()
	cryMaxTools.anim.align.vars.edgeList.selection = #()
)
deletePivotPoints = undefined









cryMaxTools.anim.align.functions.switchViewPort = function switchViewPort =
(
	local numViews = viewport.numViews
	local viewSize = getViewSize()
	
	
	case numViews of
	(
		4:
		(
			if mouse.screenpos[1] < viewSize[1] then
			(
				if mouse.screenpos[2] < (viewSize[2]+74) then
					if viewport.activeViewport != 1 then viewport.activeViewport = 1
				if mouse.screenpos[2] > (viewSize[2]+74) then 
					if viewport.activeViewport != 3 then viewport.activeViewport = 3
			)
			
			if mouse.screenpos[1] > viewSize[1] then
			(
				if mouse.screenpos[2] < (viewSize[2]+74) then
					if viewport.activeViewport != 2 then viewport.activeViewport = 2
				if mouse.screenpos[2] > (viewSize[2]+74) then
					if viewport.activeViewport != 4 then viewport.activeViewport = 4
			)
		)
	)
)
switchViewPort = undefined





/*
cryMaxTools.anim.align.functions.callPivotSelect = function callPivotSelect =
(
	local selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	if selectedBipPart == undefined then return undefined
	if selectedBipPart.object.isPivot == false then return false
	
	
	cryMaxTools.anim.align.pivotSelect = tool tempTool
	(
		on start do
		(
				--// initialise variables for global use
			cryMaxTools.anim.align.functions.initVars()
			cryMaxTools.anim.align.functions.createPointEdge()
		)
			
		on freeMove do
		(
				--// makes the recognition of mouse interaction
			cryMaxTools.anim.align.functions.switchViewPort()
			cryMaxTools.anim.align.functions.interactPivotPoint()
		)
		
		on mousePoint click do
		(
				--// switches trough pivots and highlights them, as well sets global variables for other _f
			cryMaxTools.anim.align.functions.switchViewPort()
			cryMaxTools.anim.align.functions.clickAction()
			cryMaxTools.anim.align.functions.pivotPoint #delete pointUpdate:true
		)
		
		on mouseMove click do
		(	
				--// after the first click is finished, it waits for the mouse click to start the function
			if click == 1 then
					--// switches trough pivots and highlights them, as well sets global variables for other _f
				cryMaxTools.anim.align.functions.clickAction()
			
			cryMaxTools.anim.align.functions.pivotPoint #delete pointUpdate:true
			
				--// makes the recognition of mouse interaction
			cryMaxTools.anim.align.functions.switchViewPort()
			cryMaxTools.anim.align.functions.interactPivotPoint()
		)
		
		on stop do
		(
				--// deletes the used nodes
			cryMaxTools.anim.align.functions.deletePivotPoints()
		)
	)
	tempTool = undefined
	
	startTool cryMaxTools.anim.align.pivotSelect
)
callPivotSelect = undefined
*/







cryMaxTools.anim.align.functions.callPivotSelect = function callPivotSelect =
(
	local obj = (getCurrentSelection())[1]
	
	if obj != undefined then
	(
		local foundPivot = 0
		local foundTarget = undefined
		local radius = distance obj.min obj.max
		
		for tempObj in objects do
			if tempObj.name == ("_" + obj.name + "_pivotTarget") then
				foundTarget = tempObj
		
		
		for i = 1 to obj.children.count do
			if obj.children[i].name == ("_" + obj.name + "_pivotSource") then
				foundPivot = i
		
		local tempPiv = undefined
		
		with animate off
		(
			if foundPivot == 0 then
			(
				tempPiv = sphere name:("_" + obj.name + "_pivotSource") radius:(radius / 30.0) segs:10  parent:obj wireColor:(color 255 0 0)
				tempPiv.transform = obj.transform
			)
			else
				tempPiv = obj.children[foundPivot]
		)
			
			
		if foundTarget == undefined then
		(
			set animate off
			animButtonState = false
			
			local tempTarget = sphere name:("_" + obj.name + "_pivotTarget") radius:(radius / 25.0) segs:10 wireColor:(color 0 0 255)
			
			local tempTM = ((transmatrix [5,0,0]) * obj.transform)
			tempTarget.transform = tempTM
			
			
			select tempPiv
			max move
			setRefCoordSys #local
			
			completeRedraw()
		)
		else
			with animate off
				foundTarget.transform = tempPiv.transform
	)
)
callPivotSelect = undefined


















--///////////////////////////////////////////////////////////////////////////////////////////////////////////
--################################################################
--		DEF:cryTools.cryAnim._f.resetRotation
----------------------------------------------------------------------------------------------------------------------------------
--			applies the parent rotation to the child
--################################################################
--///////////////////////////////////////////////////////////////////////////////////////////////////////////

cryMaxTools.anim.functions.op.resetRotation = function resetRotation =
(
	if selection.count > 0 then
	(
		for obj in selection do
		(
			if obj.classID[1] == 37157 then
			(
				if obj.parent != undefined then
				(
					local newRotation = obj.parent.transform.rotation
					
					if findString obj.name "Toe" != undefined then
						newRotation = (matrix3 obj.parent.transform.row2 -obj.parent.transform.row1 obj.parent.transform.row3 obj.transform.row4).rotation
					if findString obj.name "Hand" != undefined then
						newRotation = (matrix3 obj.parent.transform.row1 -obj.parent.transform.row3 obj.parent.transform.row2 obj.transform.row4).rotation
					
					biped.setTransform obj #rotation newRotation true
				)
			)
			else
			(
				if obj.parent != undefined then
				(
					local newMatrix = obj.transform
					
					newMatrix.rotation = obj.parent.transform.rotation
					newMatrix.position = obj.transform.position
					obj.transform = newMatrix
				)
			)
		)
	)
)
resetRotation = undefined




cryMaxTools.anim.functions.op.rotateAnim = function rotateAnim animDir range:undefined =
(
	if classOf animDir == Integer or classOf animDir == Float then
	(
		if selection.count > 0 then
			local usedNode = (getCurrentSelection())[1]
		else
			if $Bip01 != undefined then
				usedNode = $Bip01
			
		if usedNode != undefined then
		(
			if usedNode.classID[1] == 37157 then
			(
				usedNode = usedNode.controller.rootNode
				local undoString = "rotate " + animDir as String
				undo undoString on
				(
					if range != undefined then
					(
						at time animationRange.start
						(
							with animate off
							(
								usedNode.controller.moveAllMode = true
								rotate usedNode (eulerangles 0 0 animDir)
								biped.collapseMoveAllMode usedNode.controller
								usedNode.controller.moveAllMode = false
							)
						)
						
						
						
						if $Locator_Locomotion != undefined then
						(
							if $Locator_Locomotion.classID[1] == 37157 then
							(
								at time animationRange.start
								(
									with animate off
									(
										rotRoot = box pos:usedNode.transform.pos name:"rotRoot"	
										locClone = (cryMaxTools.anim.functions.op.createSnapshot object:$Locator_Locomotion)[1]
										locClone.parent = rotRoot
									)
								)
								
								for i = animationRange.start to animationRange.end do
								(
									with animate on
										at time i	
											locClone.transform = $Locator_Locomotion.transform
								)
								
								with animate off
								(
									at time animationRange.start
										rotate rotRoot (eulerangles 0 0 animDir)
								)
								
								
								with redraw off
								(
									for i = animationRange.start to animationRange.end do
									(
										sliderTime = i
										(
											with animate on
											(
												biped.setTransform $Locator_Locomotion #rotation locClone.transform.rotation true
												biped.setTransform $Locator_Locomotion #pos locClone.transform.pos true
											)
										)
									)
								)
							)
						)
						
						sliderTime = animationRange.start + 1
						sliderTime = animationRange.start
					)
					else
					(
						with animate on
							rotate usedNode (eulerangles 0 0 animDir)
						
						if $Locator_Locomotion != undefined then
						(
							if $Locator_Locomotion.classID[1] == 37157 then
							(
								with animate off
								(
									rotRoot = box pos:usedNode.transform.pos name:"rotRoot"	
									locClone = (cryMaxTools.anim.functions.op.createSnapshot object:$Locator_Locomotion)[1]
									locClone.parent = rotRoot
									rotate rotRoot (eulerangles 0 0 animDir)
								)
								
								with animate on
								(
									biped.setTransform $Locator_Locomotion #rotation locClone.transform.rotation true
									biped.setTransform $Locator_Locomotion #pos locClone.transform.pos true
								)
							)
						)
					)
				)
			)
		)
	)
	try 
	(
		delete rotRoot
		delete locClone
	)catch()
)
rotateAnim = undefined








cryMaxTools.anim.functions.op.bipToZero2 = function bipToZero2 =
(
	try
		biped.setTransform $Bip01 #pos [0,0,($Bip01.transform.pos[3])] true
	catch()
)
bipToZero2 = undefined





cryMaxTools.anim.functions.op.bipToZero3 = function bipToZero3 =
(
	try
	(
		biped.setTransform $Bip01 #pos [0,0,0] true
	)
	catch ()
)
bipToZero3 = undefined







cryMaxTools.anim.functions.op.setTrajectories = function setTrajectories dir =
(
	selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
	
	case dir of
	(
		#show:
		(
			for obj in $selection do
				setTrajectoryOn obj true
			
			return true
		)
		#hide:
		(
			for obj in Objects do
				setTrajectoryOn obj false
				
			return true
		)
		#checkHide:
		(			
			for obj in Objects do
				if (getTrajectoryOn obj) == true then
					return true

			return false
		)
	)
)
setTrajectories = undefined







cryMaxTools.anim.functions.op.resetLocator = function resetLocator forceDir:undefined =
(
	if $Locator_Locomotion != undefined then
	(
		try
		(			
			biped.deleteKeys $'Locator_Locomotion'.controller #allKeys		
			biped.addNewKey $'Locator_Locomotion'.controller animationRange.start
		
			propKey = biped.getKey $'Locator_Locomotion'.controller 1

			propKey.posSpace = 0
			propKey.rotSpace = 0
			
			sliderTime = animationRange.start
			
			bipRot = $Bip01.transform.rotation as eulerangles
			
			biped.setTransform $'Locator_Locomotion' #pos (in coordsys world [0,0,0]) true
			
			if forceDir == undefined then
			(
				if bipRot.z > 0 and bipRot.z < 180 then
				(
						--// new, proper character rotation
					biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 0 1) true
				)
				else
				(
						--// used for Crysis but old character rotation
					biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 -1 0) true
				)
			)
			else
				biped.setTransform $'Locator_Locomotion' #rotation (quat 0 0 -1 0) true
				
			local tempItem = getCurrentSelection()
			if findItem tempItem $Locator_Locomotion == 0 then
				select $'Locator_Locomotion'
			
			return true
		)
		catch( return false)
	)
	else
		print "No Locator in Scene"
)
resetLocator = undefined










cryMaxTools.anim.functions.op.moveToBodyMass = function moveToBodyMass =
(
	function getMaxValue root index =
	(	
		if root != undefined then
		(
			maxValue = #(index, root.transform.pos[index], root.transform.pos[index])	
			
				for obj in root.children do
					if obj.isHidden == false and obj.isFrozen == false then
						maxValue = cryMaxTools.anim.functions.op.getChildExtent obj maxValue
				
			return maxValue
		)
	)



	partPos = [0,0,0]	

	for i = 1 to 3 do
	(
		local newPos = getMaxValue $'Bip01 Spine' i
		newPos = (newPos[2] + newPos[3]) / 2
		partPos[i] = newPos
	)

	
	diffPos = ($'Bip01 Pelvis'.transform.pos - $'Bip01 Spine3'.transform.pos) / 2
	bodyPos = $'Bip01 Pelvis'.transform.pos - diffPos

	
	partPos = partPos * 1/3
	bodyPos = bodyPos * 2/3
	
	finalPos = partPos + bodyPos
	finalPos[3] = 0
	
	biped.setTransform $Locator_Locomotion #pos finalPos true
	
	return true
)
moveToBodyMass = undefined







cryMaxTools.anim.functions.op.autoLocator = function autoLocator =
(
	function setCon node time =
	(
		try
		(
			for i = 1 to node.controller.keys.count do
			(
				try
				(
					tempKey = biped.getKey node.controller i
					if tempKey.time == time then
					(
						tempKey.continuity = 0
					)
				)
				catch()
			)
		)
		catch()
	)



	function calcLoc =
	(
		cycleLoc = true
		rotateLoc = #none
		posChange = false
		startLoc = #none
		locoCycle = false
		
		
		
		at time animationRange.start
		(
			startRot = $Bip01.transform.rotation as eulerangles
			startPos = $Bip01.transform.pos
			
			startLFootPos = $'Bip01 L Toe0'.transform.pos
			startRFootPos = $'Bip01 R Toe0'.transform.pos
		)
		
		at time animationRange.end
		(
			endRot = $Bip01.transform.rotation as eulerangles
			endPos = $Bip01.transform.pos
		)
		
		at time ((animationRange.start + animationRange.end) / 2)
			midRot = $Bip01.transform.rotation as eulerangles
		
		at time 2f
		(
			startMidPos = $Bip01.transform.pos
			
			startMidLFootPos = $'Bip01 L Toe0'.transform.pos
			startMidRFootPos = $'Bip01 R Toe0'.transform.pos
		)
		
		at time (animationRange.end - 2)
			endMidPos = $Bip01.transform.pos
		
		

		diffRot = #(endRot.x, endRot.y, endRot.z)
		diffRot[1] -= startRot.x
		diffRot[2] -= startRot.y
		diffRot[3] -= startRot.z
		
		diffMidRot = #(midRot.x, midRot.y, midRot.z)
		diffMidRot[1] -= startRot.x
		diffMidRot[2] -= startRot.y
		diffMidRot[3] -= startRot.z
		
		
		
		diffPos = endPos - startPos	
		diffStartPos = startMidPos - startPos
		diffEndPos = endPos - endMidPos
		
		diffLFootPos = startLFootPos - startMidLFootPos
		diffRFootPos = startRFootPos - startMidRFootPos
		
		diffRotPositiv = copy diffRot #noMap
		
		for i = 1 to 3 do
		(
			if diffPos[i] < 0 then diffPos[i] *= -1
			if diffPos[i] > 10 then posChange = true
			if diffRotPositiv[i] < 0 then diffRotPositiv[i] *= -1
			if diffRotPositiv[i] > 2 then cycleLoc = false	
			--if diffMidRot[i] < 0 then diffMidRot[i] *= -1
			
			if diffLFootPos[i] < 0 then diffLFootPos[i] *= -1
			if diffLFootPos[i] > 10 then locoCycle = true
			
			if diffRFootPos[i] < 0 then diffRFootPos[i] *= -1			
			if diffRFootPos[i] > 10 then locoCycle = true
		)
		
		
		
		if cycleLoc == false then locoCycle = false 
		
		
		diffStartPos = distance startPos startMidPos
		diffEndPos = distance endMidPos endPos
			
		
		
		if diffStartPos < 5 and diffStartPos > 0.4 then
		(
			if locoCycle == false then
				startLoc = #start
		)
		if diffStartPos > 4 and diffEndPos < 2 then
			if locoCycle == false then
				if cycleLoc == false then
					startLoc = #stop	
		
		
		
		
		if diffRot[3] < 140 and diffRot[3] > 80 then
			rotateLoc = #left
		if diffRot[3] < -40 and diffRot[3] > -100 then
			rotateLoc = #right
		
		
		if diffMidRot[3] > -220 and diffMidRot[3] < -140 then	
		(
			if diffRot[3] < -140 and diffRot[3] > -180 then
				rotateLoc = #revL
		)
		
		if diffMidRot[3] > -140 and diffMidRot[3] < -60 then	
		(
			if diffRot[3] < -130 and diffRot[3] > -170 then
				rotateLoc = #revR
		)

	--	print ("diffMidRot = " + diffMidRot as String)
	--	print ("diffRot = " + diffRot as String)
	--	print ("diffEndPos = " + diffEndPos as String)
	--	print ("diffStartPos = " + diffStartPos as String)
	--	print ("diffLFootPos = " + diffLFootPos as String)
	--	print ("diffRFootPos = " + diffRFootPos as String)
	--	print ("startLoc = " + startLoc as String)
	--	print ("rotateLoc = " + rotateLoc as String)
	--	print ("cycleLoc = " + cycleLoc as String)
	--	print ("posChange = " + posChange as String)
	--	print ("locoCycle = " + locoCycle as String)
		
		
		
		local tempValue = locStruct start:startLoc rotate:rotateLoc cycle:cycleLoc position:posChange
		
	--	print tempValue
			
		return tempValue
	)


	analyseLoc = calcLoc()
	
	cryMaxTools.anim.functions.op.resetLocator()
	
	cryMaxTools.anim.functions.op.moveToBodyMass()
	
	if analyseLoc.rotate != #none then
	(
		at time 10f
		(
			biped.setTransform $Locator_Locomotion #pos $Locator_Locomotion.transform.pos true
			setCon $Locator_Locomotion 10f
		)
		
		at time animationRange.end
			cryMaxTools.anim.functions.op.moveToBodyMass()
		
		
		with animate on
		(
			at time 16f
			(
				case analyseLoc.rotate of
				(
					#left: rotate $Locator_Locomotion (eulerangles 0 0 89)
					#right: rotate $Locator_Locomotion (eulerangles 0 0 -89)
					#revL: rotate $Locator_Locomotion (eulerangles 0 0 179)
					#revR: rotate $Locator_Locomotion (eulerangles 0 0 -179)
				)
				setCon $Locator_Locomotion 16f
			)
			
			at time animationRange.end
			(
				case analyseLoc.rotate of
				(
					#left: rotate $Locator_Locomotion (eulerangles 0 0 89)
					#right: rotate $Locator_Locomotion (eulerangles 0 0 -89)
					#revL: rotate $Locator_Locomotion (eulerangles 0 0 179)
					#revR: rotate $Locator_Locomotion (eulerangles 0 0 -179)
				)	
			)
		)
	)
	else
	(
		if analyseLoc.position == true then
		(
			if analyseLoc.start == #start then
			(
				at time 10f
				(
					biped.setTransform $Locator_Locomotion #pos $Locator_Locomotion.transform.pos true
					setCon $Locator_Locomotion 10f
				)
				
				at time animationRange.end
					cryMaxTools.anim.functions.op.moveToBodyMass()
			)
			else
			(
				at time animationRange.end
					cryMaxTools.anim.functions.op.moveToBodyMass()
			)
		)
		else
		(
			if analyseLoc.start == #start then
				at time animationRange.end
					cryMaxTools.anim.functions.op.moveToBodyMass()
		)
	)
	
	sliderTime = animationRange.start + 1
	sliderTime = animationRange.start
)




	
	
	










cryMaxTools.anim.functions.op.getChildExtent = function getChildExtent root maxValue =
(
	if root.transform.pos[maxValue[1]] > maxValue[2] then
		maxValue[2] = root.transform.pos[maxValue[1]]

	if root.transform.pos[maxValue[1]] < maxValue[3] then
		maxValue[3] = root.transform.pos[maxValue[1]]

	if root.children != undefined then
	(
		for obj in root.children do
			if obj.isHidden == false and obj.isFrozen == false then
				cryMaxTools.anim.functions.op.getChildExtent obj maxValue
	)
	return maxValue
)
getChildExtent = undefined






cryMaxTools.anim.functions.op.createSnapshot = function createSnapshot children:unsupplied object:unsupplied =
(
	baseArray = #()
	snapArray = #()

	function copyMesh baseObject =
	(
		try
		(
			local tempSnap = snapshot baseObject
			tempSnap.parent = undefined
			tempSnap.transform = baseObject.transform
			tempSnap.wirecolor = baseObject.wirecolor
			return tempSnap
		)
		catch
			return undefined
	)


	if object == unsupplied then
	(	
		for obj in $selection do
		(
			local newArray = #( obj )
			if children == true then
				join newArray (cryMaxTools.anim.functions.op.getChildren obj)

			for obj in newArray do
			(
				append snapArray (copyMesh obj)
			)
				
			append baseArray newArray
		)
	)		
	else
	(
		append snapArray (copyMesh object)
	)
	
	if snapArray.count > 0 then
		return snapArray
	else
		return undefined
)
createSnapshot = undefined





cryMaxTools.anim.functions.op.lockRotation = function lockRotation =
(
	if selection.count > 0 then
	(
		if cryMaxTools.anim.vars.lockRotation == undefined then
		(
			cryMaxTools.anim.vars.lockRotation = $.transform.rotation
			return true
		)
		else
			return false
	)
)
lockRotation = undefined








cryMaxTools.anim.functions.op.pasteTransform = function pasteTransform obj value op =
(
	try
	(
		case op of
		(
			#transform:
			(
				biped.setTransform obj #rotation value.rotation true
				biped.setTransform obj #pos value.pos true
				biped.setTransform obj #rotation value.rotation true
			)
			#pos:
			(
				biped.setTransform obj #pos value.pos true
			)
			#rotation:
			(
				biped.setTransform obj #rotation value.rotation true
				biped.setTransform obj #rotation value.pos true
			)
		)
	)
	catch
	(
		try
		(
			local tempTransform = obj.transform
			
			case op of
			(	
				#transform:
				(
					tempTransform = value
				)
				#pos:
				(
					tempTransform.pos = value.pos
					tempTransform.rotation = obj.rotation
				)
				#rotation:
				(
					tempTransform.rotation = value.rotation
					tempTransform.pos = obj.pos
				)
			)
			obj.transform = tempTransform
		)
		catch()
	)
)
pasteTransform = undefined







cryMaxTools.anim.functions.op.getTimeLineRange = function getTimeLineRange selNode:undefined =
(
	local minRange = 0f
	local maxRange = 1f
	local keysFound = false

	if selNode == undefined then
	(
		if selection.count > 0 then
			local tempNodeArray = selection as Array
		else
			local tempNodeArray = Objects as Array
	)
	else
	(
		if classOf selNode == Array then
			local tempNodeArray = selNode
		else
			local tempNodeArray = #(selNode)
	)

	for obj in tempNodeArray do
	(
		local cons = getControllers obj
		
		for i = 1 to cons.count do
		(
			try
			(
				if cons[i].keys.count > 0 then
				(
					keysFound = true
					
					if minRange > cons[i].keys[1].time.frame then
						minRange = cons[i].keys[1].time.frame
						
					if maxRange < cons[i].keys[cons[i].keys.count].time.frame then
						maxRange = cons[i].keys[cons[i].keys.count].time.frame
				)
			)catch()
		)
	)
	
	if keysFound == false then
		newTimeRange = interval 0 1

	try
	(
		if minRange == maxRange then
			maxRange += 1
		
		newTimeRange = interval minRange maxRange
	)
	catch ()
	
	return newTimeRange
)
getTimeLineRange = undefined






cryMaxTools.anim.functions.op.nodeDependsOn = function nodeDependsOn obj =
(
	local refArray = #()
	
	if obj != undefined then
	(
		append refArray obj
		
		local depArray = refs.dependentNodes obj
		
		for i = 1 to depArray.count do
			join refArray (cryMaxTools.anim.functions.op.nodeDependsOn depArray[i])
	)
	
	return refArray
)







cryMaxTools.anim.functions.op.sortRootChildren = function sortRootChildren nodeArray dependent:false =
(
	local resultArray = #()
	local nodeAllArray = #()
	
	if classOf nodeArray == Array then
	(
		if nodeArray.count > 0 then
		(
			try local tempParent = getNodeByName nodeArray[1].name
			catch local tempParent = getNodeByName nodeArray[1]
			
			while tempParent.parent != undefined do
			(
				enableEscape = true
				tempParent = tempParent.parent
			)
			
			local root = tempParent
			
			
			append nodeAllArray root
			join nodeAllArray (cryMaxTools.anim.functions.OP.getChildren root)
			
			global tempShit = nodeAllArray
			
			try
			(
				for i = 1 to nodeArray.count do
					nodeArray[i] = nodeArray[i].name
			)catch()
			
			
			
			for i = 1 to nodeAllArray.count do
				nodeAllArray[i] = nodeAllArray[i].name
			
			local notParentedArray = #()
			
			
			for i = 1 to nodeAllArray.count do
			(
				if findItem nodeArray nodeAllArray[i] == 0 then
					nodeAllArray[i] = undefined
			)
			
			for i = 1 to nodeArray.count do
				if findItem nodeAllArray nodeArray[i] == 0 then
					append notParentedArray nodeArray[i]
			
			for i = 1 to nodeAllArray.count do
				if nodeAllArray[i] != undefined then
					append resultArray nodeAllArray[i]
			
			join resultArray notParentedArray
				
			for i = 1 to resultArray.count do
				resultArray[i] = getNodeByName resultArray[i]
			
			
			
			if dependent == true then
			(
				local newArray = #()
				
				for i = 1 to resultArray.count do
				(
					appendIfUnique newArray resultArray[i]
					
					local depArray = cryMaxTools.anim.functions.op.nodeDependsOn resultArray[i]
					
					for d = 1 to depArray.count do
						if findItem resultArray depArray[d] != 0 then
							appendIfUnique newArray depArray[d]
				)
				
				resultArray = newArray
			)
		)
	)
	
	return resultArray
)
sortRootChildren = undefined






cryMaxTools.anim.align.functions.pivotPoint = function pivotPoint operation pointUpdate:undefined =
(
	undo off
	(
		if (selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart() ) != undefined then
		(
			case operation of
			(
				#check:
					return (selectedBipPart.pivotSel.pivPoint.snapTM != undefined)
				
				#create:
				(
					ready = true
					
					addModifier selectedBipPart.object.selected (edit_mesh())
					
					local tempPos = [0,0,0]
					
					local tempSnap = (cryMaxTools.anim.functions.op.createSnapshot object:(getNodeByName selectedBipPart.pivotSel.pivObject))[1]
					
					if pointUpdate != undefined then
						tempSnap.transform = selectedBipPart.pivotSel.pivPoint.snapTM
					
					tempSnap.wireColor = (color 255 0 0)
					tempSnap.showFrozenInGray = false
					tempSnap.isFrozen = true
					tempSnap.name = ("pivSnap_(" + selectedBipPart.name + ")")
					
					
					case selectedBipPart.pivotSel.pivotName[1] of
					(
						"p":
						(
							tempPos = (cryMaxTools.anim.align.functions.getPivotPointPos selectedBipPart.pivotSel.index tempSnap)
							tempObject = sphere pos:tempPos radius:0.2
						)
						
						"e":
						(
							posTo = [0,0,0]
							
							case selectedBipPart.pivotSel.index of
							(
								1:( tempPos = cryMaxTools.anim.align.functions.getPivotPointPos 2 tempSnap ; posTo = cryMaxTools.anim.align.functions.getPivotPointPos 5 tempSnap )
								2:( tempPos = cryMaxTools.anim.align.functions.getPivotPointPos 5 tempSnap ; posTo = cryMaxTools.anim.align.functions.getPivotPointPos 7 tempSnap )
								3:( tempPos = cryMaxTools.anim.align.functions.getPivotPointPos 7 tempSnap ; posTo = cryMaxTools.anim.align.functions.getPivotPointPos 4 tempSnap )
								4:( tempPos = cryMaxTools.anim.align.functions.getPivotPointPos 4 tempSnap ; posTo = cryMaxTools.anim.align.functions.getPivotPointPos 2 tempSnap )
							)
							
							tempObject = cylinder()
							
							tempObject.radius = 0.2
							tempObject.sides = 5
								--// sets position to the inital position
							tempObject.position = tempPos
								--// calculates distance between 2 pivot points and applies to the height
							tempObject.height = distance tempPos posTo
							
							
								--// generates a tape with target for excact rotation of the cylinder
							tempTape = tape pos:tempPos target:(targetObject pos:posTo)
								--// applies transform from generated tape to the cylinder
							tempObject.transform = tempTape.transform
								--// the cylinder is rotated 180°, so it will be rotated to the other side of the pivot
							in coordsys tempObject rotate tempObject (eulerangles 180 0 0)
							
							delete tempTape
						)
					)
					
					deleteModifier selectedBipPart.object.selected 1
					
					
					tempObject.wireColor = (color 255 0 0)
					tempObject.showFrozenInGray = false
					tempObject.isFrozen = true
					tempObject.name = ("pivPoint_(" + selectedBipPart.name + ")")
					
					
					execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.name = \"" + tempSnap.name + "\"")
					execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.snapTM = " + (tempSnap.transform as String))
					execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.pointPos = " + (tempObject.transform.pos as String))
				)
				#delete:
				(
					try
					(
						execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.name = undefined")
						
						if pointUpdate == undefined then
							execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.snapTM = undefined")
						
						execute ("cryMaxTools.anim.align.vars.pivotLast." + selectedBipPart.part.name + ".pivPoint.pointPos = undefined")
						delete (getNodeByName selectedBipPart.pivotSel.pivPoint.name)
						delete (getNodeByName ("pivPoint" + ( substring selectedBipPart.pivotSel.pivPoint.name 8 selectedBipPart.pivotSel.pivPoint.name.count )) )
						
						if pointUpdate != undefined then
							cryMaxTools.anim.align.functions.pivotPoint #create pointUpdate:true
						
						return true										
					)catch()
					
					return false
				)
			)
		)
		return undefined
	)
)
pivotPoint = undefined










cryMaxTools.anim.functions.op.moveToFloor = function moveToFloor value:undefined getHeight:undefined =
(
	if ( selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart() ) != undefined then
	(
		if selectedBipPart.object.isPivot == true then
		(
				--// creates a matrix to rotate the selected object 
			tempMatrix = selectedBipPart.object.selected.transform
			
				--// creates the height of the foot out of the distance between the heighest vert (5) and lowest vert (6)
			if value == undefined then
			(
				addModifier selectedBipPart.object.selected (edit_mesh())
				tempHeight = distance (getVert selectedBipPart.object.selected 5) (getVert selectedBipPart.object.selected 6)
				deleteModifier selectedBipPart.object.selected 1
			)
			else
				tempHeight = value
			
			if getHeight != undefined then
				return tempHeight
			
				--// sets new rotation to the matrix, so that it keeps the horizontal rotation
			tempMatrix = matrix3 [0, 0, -1] [tempMatrix.row2.x, tempMatrix.row2.y, 0] [tempMatrix.row3.x, tempMatrix.row3.y, 0] [tempMatrix.row4.x, tempMatrix.row4.y, tempHeight]
				--// sets the foot to the floor				
			selectedBipPart.object.selected.transform = tempMatrix
			biped.setTransform selectedBipPart.object.selected #pos [tempMatrix.pos[1], tempMatrix.pos[2], tempMatrix.pos[3]] true
			
			return true
		)
		else
			return undefined
	)
)
moveToFloor = undefined











cryMaxTools.anim.functions.UI.setMacros = function setMacros op =
(
	case op of
	(
		#delete:
		(
			local animToolsMacros = getFiles (getDir #UI + "MacroScripts\\CryMaxTools*.mcr")
	
			for i = 1 to animToolsMacros.count do				
				deleteFile animToolsMacros[i]
	
			menuMan.updateMenubar()				
		)
		
		#update:
		(			
			ATFilesArray = # \
			( \
				"ATSetToCrytekShader",
				"ATAnimToolsDlg", \
				"ATBipToZero2", \
				"ATBipToZero3", \
				"ATCreateSingleSnapshot", \
				"ATCreateChildrenSnapshot", \
				"ATHideAllTrajectories", \
				"ATMoveRot", \
				"ATMoveToFloor", \
				"ATMoveToPivPoint", \
				"ATMoveRotToPivPoint", \
				"ATMoveToSnapshot", \
				"ATPivotSelect", \
				"ATResetRotation", \
				"ATShowTrajectory", \
				"ATCopyTransform", \
				"ATPasteTransform", \
				"ATPastePosition", \
				"ATPasteRotation" \
			)
				
			localMacros = getFiles ((getDir #UI) + "\\MacroScripts\\CryMaxToolsAnim*")
			for i = 1 to ATFilesArray.count do
			(					
				for f = 1 to 40 do
				(
					if localMacros.count == 0 then
						exit
					else
					(							
						try
						(
							if (findString localMacros[f] ATFilesArray[i]) != undefined then
								deleteItem localMacros f
						)
						catch()								
					)
				)
			)
			
			if localMacros.count != 0 then
			(
				for i = 1 to localMacros.count do
				(
					deleteFile localMacros[i]
					print ("Deleted unused MacroScript:  " + localMacros[i])
				)
			)
		)
	)
	
	
	
	
	
	macroScript ATSetToCrytekShader category:"CryMaxTools Material" toolTip:"set to Crytek Shader"
	(					
		on isVisible do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			return true
		)
		on execute do
		(
			try
			(
				local curMat = medit.GetCurMtl()
				
				print curMat

				if curMat != undefined then
				(
					if classOf curMat == Multimaterial then
					(
						for i = 1 to curMat.count do
							curMat[i].shaderType = 2
					)
					else if classOf curMat == Standardmaterial then
						curMat.shaderType = 2
				)
			)catch()
		)
	)
	
	
	
	
	
	
	macroScript ATMirrorRotation category:"CryMaxTools Animation" toolTip:"mirror rotation"
	(					
		on isVisible do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			return true
		)
		on execute do
		(
			try
			(
				fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\MirrorRotation.ms")
			)catch()
		)
	)
	
	
	
	
	
	macroScript ATMirrorAnimation category:"CryMaxTools Animation" toolTip:"mirror animation"
	(					
		on isVisible do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			return true
		)
		on execute do
		(
			try
			(
				fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\MirrorRotation.ms")
			)catch()
		)
	)
	
		
	

	macroScript ATCreateSingleSnapshot category:"CryMaxTools Animation" toolTip:"create single snapshot"
	(
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if selection.count > 0 then
						return true
					else
						return false
				)
			)catch( return false )
		)
		
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					with animate off
						cryMaxTools.anim.functions.op.createSnapshot()
				)
			)catch()
		)
	)	
		
		
		
		
	macroScript ATCreateChildrenSnapshot category:"CryMaxTools Animation" toolTip:"create children snapshot"
	(
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if selection.count > 0 then
						return true
					else
						return false
				)
			)catch( return false )
		)
		
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					with animate off
					(
						if $selection.count > 0 then
						(
							local baseArray = cryMaxTools.anim.functions.op.createSnapshot children:true
							if baseArray != undefined then
								group baseArray prefix:"Snapshot_"
						)
						else
							print "Nothing selected"
					)
				)
			)catch()
		)
	)


	macroScript ATPivotSelect category:"CryMaxTools Animation" toolTip:"create pivot"
	(					
		on isVisible do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			if selection.count > 0 then
				return true
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo off
						cryMaxTools.anim.align.functions.callPivotSelect()
				)
			)catch()
		)
	)
	
	
	
	
	
	
	
	macroScript ATTogglePivPoint category:"CryMaxTools Animation" toolTip:"toggle pivot point"
	(					
		on isVisible do
		(
			undo "" off
			(
				try
				(
					if cryMaxTools != undefined then
					(
						selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
						if selectedBipPart != undefined then
							return true
						else
							return false
					)
				)catch( return false )
			)
		)
		on isEnabled do
		(
			undo "" off
			(
				try
				(
					if cryMaxTools != undefined then
					(
						selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
						if selectedBipPart != undefined then
							if selectedBipPart.object.isPivot == true then
								return true
							else
								return false
					)
				)catch( return false )
			)
		)
		on execute do
		(
			undo "" off
			(
				try
				(
					if cryMaxTools != undefined then
					(
						if (selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart() ) != undefined then
						(
							if selectedBipPart.pivotSel.index == undefined then
								print "Select a Pivot first" title:"Pivot Point"
							else
							(
								undo off
								(
									if (cryMaxTools.anim.align.functions.pivotPoint #check) == true then
										cryMaxTools.anim.align.functions.pivotPoint #delete
									else
										cryMaxTools.anim.align.functions.pivotPoint #create
								)
							)
						)
						else
						(
							if selection.count > 0 then
								print "No Biped Object selected."
							else
								print "Nothing selected"
						)
					)
				)catch()
			)
		)
	)
	
	
	
				
					
	macroScript ATMoveToSnapshot category:"CryMaxTools Animation" toolTip:"move to snapshot"
	(				
		on isVisible  do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					return (cryMaxTools.anim.align.functions.getNearestSnapshot noPivot:true != undefined)
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(					
					undo "move to snap" on
						cryMaxTools.anim.align.functions.moveBipPart()
				)
			)catch()
		)
	)
				
				
				
	macroScript ATMoveToPivPoint category:"CryMaxTools Animation" toolTip:"move to pivot point"
	(				
		on isVisible  do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
					if selectedBipPart != undefined then
					(
						if selectedBipPart.pivotSel.pivPoint.name != undefined then
							return true
						else
							return false
					)
				)
				return false
			)catch( return false )
		)
		
		on isEnabled do
		(
			try
			(
				selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
				if selectedBipPart != undefined then
					if selectedBipPart.object.isPivot == true then
						return true
						
				return false
			)catch( return false )
		)
		
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo "move to pivot point" on
						cryMaxTools.anim.align.functions.moveBipPart pivotPoint:true
				)
			)catch()
		)
	)
	
	
	
	
	macroScript ATMoveRotToPivPoint category:"CryMaxTools Animation" toolTip:"move/rotate to pivot point"
	(	
		on isVisible  do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
					if selectedBipPart != undefined then
					(
						if selectedBipPart.pivotSel.pivPoint.name != undefined then
							return true
						else
							return false
					)
				)
				return false
			)catch( return false )
		)
		on isEnabled do
		(
			try
			(
				selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
				if selectedBipPart != undefined then
					if selectedBipPart.object.isPivot == true then
						return true
						
				return false
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo "move/rot to pivot point" on
						cryMaxTools.anim.align.functions.moveRotBipPart pivotPoint:true
				)
			)catch()
		)
	)
	
	
		
				
	macroScript ATMoveRot category:"CryMaxTools Animation" toolTip:"move/rotate to snapshot"
	(	
		on isVisible  do
		(
			if cryMaxTools != undefined then
				return true
			else
				return false
		)
		on isEnabled do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if (cryMaxTools.anim.align.functions.getNearestSnapshot noPivot:true) != undefined then
						return true
					else
						return false
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo "move/rot to snap" on
						cryMaxTools.anim.align.functions.moveRotBipPart()
				)
			)catch()
		)
	)
				
				
	macroScript ATMoveToFloor category:"CryMaxTools Animation" toolTip:"move to floor"
	(	
		on isVisible  do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
					if selectedBipPart != undefined then
						return true
					else
						return false
				)
			)catch( return false )
		)
		on isEnabled do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
					if selectedBipPart != undefined then
						if (selectedBipPart.part.bodyPart == "Foot") and (selectedBipPart.object.child == undefined)then
							return true
						else
							return false
				)
				else
					print ("No aTools Structure loaded...")
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo "move to floor" on
						cryMaxTools.anim.functions.op.moveToFloor()
				)
			)catch()
		)
	)
				
				
	macroScript ATResetRotation category:"CryMaxTools Animation" toolTip:"reset rotation"
	(	
		on isVisible  do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					selectedBipPart = cryMaxTools.anim.functions.op.getSelectedBipPart()
					if selectedBipPart != undefined then
						return true
					else
						return false
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					undo "reset rotation" on
						cryMaxTools.anim.functions.op.resetRotation()
				)
			)catch()
		)
	)			
	
		
				
	macroScript ATBipToZero2 category:"CryMaxTools Animation" toolTip:"bip to [0,0]"
	(
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if $Bip01 != undefined then
						return true
					else
						return false
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					cryMaxTools.anim.functions.op.bipToZero2()
				)
			)catch()
		)
	)
	
					
	macroScript ATBipToZero3 category:"CryMaxTools Animation" toolTip:"bip to [0,0,0]"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if $Bip01 != undefined then
						return true
					else
						return false
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					cryMaxTools.anim.functions.op.bipToZero3()
				)
			)catch()
		)
	)
	
			
	macroScript ATShowTrajectory category:"CryMaxTools Animation" toolTip:"show trajectory"
	(	
		on isVisible do
		(
			try
			(
				if (cryMaxTools != undefined) and ((maxVersion())[1] < 9000) then
				(				
					for obj in $selection do
						if getTrajectoryOn obj == false then
							return true
					
					return false
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					cryMaxTools.anim.functions.op.setTrajectories #show
				)
			)catch()
		)
	)


	macroScript ATHideAllTrajectories category:"CryMaxTools Animation" toolTip:"hide all trajectories"
	(	
		on isVisible do
		(
			try
			(
				if (cryMaxTools != undefined) and ((maxVersion())[1] < 9000) then
				(
					return cryMaxTools.anim.functions.op.setTrajectories #checkHide
				)
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					crymaxTools.anim.functions.op.setTrajectories #hide
				)
			)catch()
		)
	)
	
	
	
	macroScript ATCopyTransform category:"CryMaxTools Animation" toolTip:"copy transform"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if $selection.count > 0 then
						return true
				)
				return false
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					try deleteFile cryMaxTools.basic.vars.tempFilePath catch()
					for d = 1 to $selection.count do
					(								
						setINISetting cryMaxTools.basic.vars.tempFilePath ("Object" + d as String) "1" ($selection[d].transform as String)
					)
				)
			)catch()
		)
	)


	macroScript ATPasteTransform category:"CryMaxTools Animation" toolTip:"paste transform"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(				
					if $selection.count > 0 then
						return true
				)
				return false
			)catch( return false )
		)
		on isEnabled do
		(
			try
			(
				if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					return true
				else
					return false
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					(
						if $selection.count > 0 then
						(
							for d = 1 to $selection.count do
							(
								local newTran = getINISetting cryMaxTools.basic.vars.tempFilePath ("Object" + d as String) "1"
								if newTran != "" then
								(
									newTran = execute ( newTran )
									cryMaxTools.anim.functions.op.pasteTransform $selection[d] newTran #transform
								)
							)
						)
					)			
				)
			)catch()
		)
	)
	
	
	macroScript ATPastePosition category:"CryMaxTools Animation" toolTip:"paste position"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(				
					if $selection.count > 0 then
						return true
				)
				return false	
			)catch( return false )
		)
		on isEnabled do
		(
			try
			(
				if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					return true
				else
					return false
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					(
						if $selection.count > 0 then
						(
							for d = 1 to $selection.count do
							(
								local newTran = getINISetting cryMaxTools.basic.vars.tempFilePath ("Object" + d as String) "1"
								if newTran != "" then
								(
									newTran = execute ( newTran )
									cryMaxTools.anim.functions.op.pasteTransform $selection[d] newTran #pos
								)
							)
						)
					)			
				)	
			)catch()
		)
	)
	
	
	macroScript ATPasteRotation category:"CryMaxTools Animation" toolTip:"paste rotation"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
				(				
					if $selection.count > 0 then
						return true
				)
				return false
			)catch( return false )
		)
		on isEnabled do
		(
			try
			(
				if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					return true
				else
					return false
			)catch( return false )
		)
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if (getFiles cryMaxTools.basic.vars.tempFilePath).count > 0 then
					(
						if $selection.count > 0 then
						(
							for d = 1 to $selection.count do
							(
								local newTran = getINISetting cryMaxTools.basic.vars.tempFilePath ("Object" + d as String) "1"
								if newTran != "" then
								(
									newTran = execute ( newTran )
									cryMaxTools.anim.functions.op.pasteTransform $selection[d] newTran #rotation
								)
							)
						)
					)			
				)
			)catch()
		)
	)
	
	
	macroScript ATSetTimeLineRange category:"CryMaxTools Animation" toolTip:"set time range"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					return true
				else
					return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if (tempRange = cryMaxTools.anim.functions.op.getTimeLineRange() ) != undefined then
						animationRange = tempRange
				)
			)catch()
		)
	)
	
	
)
setMacros = undefined







cryMaxTools.anim.functions.UI.setMenu = function setMenu =
(
	local animToolsName = "CryMaxTools Animation"

	while (try menuMan.unRegisterMenu (menuMan.findMenu "Context CryMaxTools Animation") catch(undefined)) == true do ()
	while (try menuMan.unRegisterMenu (menuMan.findMenu "create snapshot") catch(undefined)) == true do ()
	while (try menuMan.unRegisterMenu (menuMan.findMenu "paste") catch(undefined)) == true do ()
	while (try menuMan.unRegisterMenu (menuMan.findMenu "mirror") catch(undefined)) == true do ()
	
	
	-- Menu
	local ATm = menuMan.createMenu "Context CryMaxTools Animation"
	-- Seperator
	local ATSep = menuMan.createSeparatorItem()
	-- List of menUItems
	local ATmItem = undefined
		
	-- adding menUItems and set names
		
	ATmItemSubMenu = menuMan.createMenu "mirror"
	ATmItemSubMenUItem1 = menuMan.createActionItem "ATMirrorRotation" animToolsName ; ATmItemSubMenUItem1.setTitle "rotation" ; ATmItemSubMenUItem1.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem1 -1
	ATmItemSubMenUItem2 = menuMan.createActionItem "ATMirrorAnimation" animToolsName ; ATmItemSubMenUItem2.setTitle "animation" ; ATmItemSubMenUItem2.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem2 -1
	--
	ATmItem = menuMan.createSubMenUItem "mirror" ATmItemSubMenu ; ATmItem.setTitle "mirror"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createActionItem "ATBipToZero2" animToolsName; ATmItem.setTitle "bip to [0,0]"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATBipToZero3" animToolsName; ATmItem.setTitle "bip to [0,0,0]"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createActionItem "ATShowTrajectory" animToolsName; ATmItem.setTitle "show trajectory"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATHideAllTrajectories" animToolsName; ATmItem.setTitle "hide all trajectories"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createActionItem "ATMoveToPivPoint" animToolsName; ATmItem.setTitle "move to pivot point"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATMoveRotToPivPoint" animToolsName; ATmItem.setTitle "move/rotate to pivot point"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATMoveToSnapshot" animToolsName; ATmItem.setTitle "move to snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATMoveRot" animToolsName; ATmItem.setTitle "move/rotate to snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createActionItem "ATResetRotation" animToolsName; ATmItem.setTitle "reset rotation"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATMoveToFloor" animToolsName; ATmItem.setTitle "move to floor"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItemSubMenu = menuMan.createMenu "create snapshot"
	ATmItemSubMenUItem1 = menuMan.createActionItem "ATCreateSingleSnapshot" animToolsName ; ATmItemSubMenUItem1.setTitle "single" ; ATmItemSubMenUItem1.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem1 -1
	ATmItemSubMenUItem2 = menuMan.createActionItem "ATCreateChildrenSnapshot" animToolsName ; ATmItemSubMenUItem2.setTitle "children" ; ATmItemSubMenUItem2.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem2 -1
	--
	ATmItem = menuMan.createSubMenUItem "create snapshot" ATmItemSubMenu ; ATmItem.setTitle "create snapshot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createActionItem "ATCopyTransform" animToolsName; ATmItem.setTitle "copy"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--
	ATmItemSubMenu = menuMan.createMenu "paste"
	ATmItemSubMenUItem1 = menuMan.createActionItem "ATPasteTransform" animToolsName ; ATmItemSubMenUItem1.setTitle "transform" ; ATmItemSubMenUItem1.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem1 -1
	ATmItemSubMenUItem2 = menuMan.createActionItem "ATPastePosition" animToolsName ; ATmItemSubMenUItem2.setTitle "position" ; ATmItemSubMenUItem2.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem2 -1
	ATmItemSubMenUItem3 = menuMan.createActionItem "ATPasteRotation" animToolsName ; ATmItemSubMenUItem3.setTitle "rotation" ; ATmItemSubMenUItem3.setUseCustomTitle true; ATmItemSubMenu.addItem ATmItemSubMenUItem3 -1
	--
	ATmItem = menuMan.createSubMenUItem "paste" ATmItemSubMenu ; ATmItem.setTitle "paste"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	--
	ATmItem = menuMan.createSeparatorItem(); ATm.addItem ATmItem -1
	--					
	ATmItem = menuMan.createActionItem "ATPivotSelect" animToolsName; ATmItem.setTitle "create pivot"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
	ATmItem = menuMan.createActionItem "ATTogglePivPoint" animToolsName; ATmItem.setTitle "toggle pivot point"; ATmItem.setUseCustomTitle true; ATm.addItem ATmItem -1
		
	-- QuadMenu
	local ATqM = menuMan.getQuadMenu 1
	-- lower left corner of QuadMenu
	local ATqMCorner = ATqM.getMenu 4
		-- generates subMenu to put in the Menu
	local ATqMSub = menuMan.createSubMenUItem "Context CryMaxTools Animation" ATm
	-- sets the subMenu to be flat
	ATqMSub.setDisplayFlat true
	-- adding subMenu to QuadMenuCorner
	ATqMCorner.addItem  ATqMSub -1

		--// initialize when struct --
	deleteAllChangeHandlers id:#lockRotation
)
setMenu = undefined




cryMaxTools.anim.functions.UI.setMacros #update
cryMaxTools.anim.functions.UI.setMenu()