--Import/export object tracks between 3dMax and CryEngine Sandbox. Import/export real-keys and tangents instead of baked keys
--By Xiaomao Wu, Mathias Linder and Sascha Herfort
-- Aug. 25, 2010

rollout rltCryImExportSandbox "Trackview Import/Export" width:190 height:492
(
	struct sequenceKeyStruct (time, value, easeIn = [0,0], easeOut = [0,0])
	struct sequenceTrackStruct (defaultValue = 0, keys = #())
	struct sequenceStruct (name, start = 0, end = 0, root)
	local sequences = undefined
	local layerXmlObject = dotNetObject "system.xml.xmlDocument"
	local sLayerPath = undefined
	
	button btnSetLayer "Choose Layer" offset:[0,3] width:172 height:16 align:#center
	button btnSelectedLayer "No Sandbox layer selected" offset:[0,-5] width:172 height:24 align:#center enabled:false
		
	dropdownlist sequencesInLayer "" offset:[0,3] width:172 hieght:16 align:#center
		
	button btnImport "Import" offset:[0,3] width:172 height:16 align:#center enabled:false
	button btnExport "Export" offset:[0,3] width:172 height:16 align:#center enabled:false
	
	label lblInfo1 "1. Click \"Choose Layer\" to choose" offset:[-8,0] align:#left
	label lblInfo2 "    a layer from a level in the game" offset:[-8,-5] align:#left
	label lblInfo3 "    folder." offset:[-8,-5] align:#left
	
	label lblInfo4 "2. Select a sequence in the combo-" offset:[-8,0] align:#left
	label lblInfo5 "    Box." offset:[-8,-5] align:#left
	
	label lblInfo6 "3. Click \"Import\" to import tracks" offset:[-8,0] align:#left
	label lblInfo7 "    from Sandbox, \"Export\" to ex-" offset:[-8,-5] align:#left
	label lblInfo8 "    port tracks to Sandbox." offset:[-8,-5] align:#left

	label lblInfo9 "Tip:" offset:[-8,0] align:#left
	label lblInfo10 "   A *.lyr.backup file will be" offset:[-8,-5] align:#left
	label lblInfo11 "   automatically generated. You" offset:[-8,-5] align:#left
	label lblInfo12 "   can recover data from that file." offset:[-8,-5] align:#left
		
	function getTrackviewSequences filename =
	(
		local sequenceArray = #()
		
		if doesFileExist filename then
		(
			layerXmlObject.load filename
						
			local fileRoot = layerXmlObject.documentElement
			
			for i = 0 to (fileRoot.childNodes.count - 1) do
			(
				local fileRootChild = fileRoot.childNodes.itemOf[i]
				
				case fileRootChild.name of
				(
					"Layer":
					(
						for d = 0 to (fileRootChild.childNodes.count - 1) do
						(
							local layerChild = fileRootChild.childNodes.itemOf[d]
							
							case layerChild.name of
							(
								"LayerObjects":
								(
									for h = 0 to (layerChild.childNodes.count - 1) do
									(
										local layerObjChild = layerChild.childNodes.itemOf[h]
										local objName = ""
										
										for f = 0 to (layerObjChild.attributes.count - 1) do
										(
											local layerAttr = layerObjChild.attributes.itemOf[f]
											
											case layerAttr.name of
											(
												"Type":	objName = layerAttr.value
											)
										)
										
										case objName of
										(
											"SequenceObject":
											(
												for f = 0 to (layerObjChild.childNodes.count - 1) do
												(
													local objChild = layerObjChild.childNodes.itemOf[f]
													
													case objChild.name of
													(
														"Sequence":
														(
															local seqName = ""
															local seqStart = -1
															local seqEnd = -1
															local seqRoot = objChild
															
															for t = 0 to (objChild.attributes.count - 1) do
															(
																case objChild.attributes.itemOf[t].name of
																(
																	"Name":			seqName = objChild.attributes.itemOf[t].value
																	"StartTime":	seqStart = (execute objChild.attributes.itemOf[t].value) * frameRate
																	"EndTime":		seqEnd = (execute objChild.attributes.itemOf[t].value) * frameRate
																)
																
															)
															
															append sequenceArray (sequenceStruct name:seqName start:seqStart end:seqEnd root:seqRoot)
														)
													)
												)
											)
										)
									)
								)
							)
						)
					)
				)
			)
		)
		return sequenceArray
	)

    function importSequenceData sequence offsetToZero:false =
	(
		if sequence != undefined then
		(
			local flags = 0
			local offset = [0,0,0]
			local prevKey = undefined
			animationRange = interval sequence.start sequence.end
			
			for i = 0 to (sequence.root.childNodes.count - 1) do
			(
				case sequence.root.childNodes.itemOf[i].name of
				(
					"Nodes":
					(
						local nodesRoot = sequence.root.childNodes.itemOf[i]
						
						for d = 0 to (nodesRoot.childNodes.count - 1) do
						(
							case nodesRoot.childNodes.itemOf[d].name of
							(
								"Node":
								(
									local nodeRoot = nodesRoot.childNodes.itemOf[d]
									local nodeName = ""
									local nodePos = [0,0,0]
									local nodeRot = quat 1
									local nodeScale = [1,1,1]
									
									for f = 0 to (nodeRoot.attributes.count - 1) do
									(
										case nodeRoot.attributes.itemOf[f].name of
										(
											"Name":		nodeName = nodeRoot.attributes.itemOf[f].value
											"Pos":		nodePos = execute ("[" + nodeRoot.attributes.itemOf[f].value + "]")
											"Rotate":
											(
												local tempFilter = filterString nodeRoot.attributes.itemOf[f].value ","
												nodeRot = execute ("quat " + tempFilter[1] + " " + tempFilter[2] + " " + tempFilter[3] + " " + tempFilter[4])
											)
											"Scale":		nodeScale = execute ("[" + nodeRoot.attributes.itemOf[f].value + "]")
										)
									)
									
									if offsetToZero == true then
										if length offset == 0.0 and length nodePos != 0.0 then
											offset = nodePos * 100
									
									if nodeName != "" then 
									(
										local newDummy = getNodeByName nodeName
										
										if newDummy == undefined then
											newDummy = point name:nodeName
										
										local newTM = newDummy.transform
										
										deleteKeys newDummy #allKeys
										
										newTM.scale = nodeScale
										newTM.rotation = nodeRot
										newTM.pos = (nodePos * 100.0)
										
										newDummy.transform = newTM
										
										for f = 0 to (nodeRoot.childNodes.count - 1) do
										(
											case nodeRoot.childNodes.itemOf[f].name of
											(
												"Track":
												(
													local trackRoot = nodeRoot.childNodes.itemOf[f]
													local paramID = 0
													local typeID = 0
													local start = startTime
													local end = endTime
													
													for h = 0 to (trackRoot.attributes.count - 1) do
													(
														case trackRoot.attributes.itemOf[h].name of
														(
															"ParamId":		paramID = execute trackRoot.attributes.itemOf[h].value
															"Type":			typeID = execute trackRoot.attributes.itemOf[h].value
															"Flags":			flags = execute trackRoot.attributes.itemOf[h].value
															"StartTime":	start = execute trackRoot.attributes.itemOf[h].value
															"EndTime":		end = execute trackRoot.attributes.itemOf[h].value
														)
													)
													
													local trackArray = #()
													
													for h = 0 to (trackRoot.childNodes.count - 1) do
													(
														case trackRoot.childNodes.itemOf[h].name of
														(
															"NewSubTrack":
															(
																local subTrackRoot = trackRoot.childNodes.itemOf[h]
																local defaultValue = 0.0
																local keyArray = #()
																
																for g = 0 to (subTrackRoot.childNodes.count - 1) do
																(
																	case subTrackRoot.childNodes.itemOf[g].name of
																	(
																		"Key":
																		(
																			local keyTime = 0
																			local keyValue = 0
																			local keyFlags = 0
																			local keyEaseIn = [0,0]
																			local keyEaseOut = [0,0]
																			local keyRoot = subTrackRoot.childNodes.itemOf[g]
																			
																			for j = 0 to (keyRoot.attributes.count - 1) do
																			(
																				case keyRoot.attributes.itemOf[j].name of
																				(
																					"time":	keyTime = (execute keyRoot.attributes.itemOf[j].value) * frameRate
																					"value":	keyValue = (execute ("[" + keyRoot.attributes.itemOf[j].value + "]"))[2]
																					"ds":		keyEaseIn = execute ("[" + keyRoot.attributes.itemOf[j].value + "]")
																					"dd":		keyEaseOut = execute ("[" + keyRoot.attributes.itemOf[j].value + "]")
																				)
																			)
																			append keyArray (sequenceKeyStruct time:keyTime value:keyValue easeIn:keyEaseIn easeOut:keyEaseOut)
																		)
																	)
																)
																append trackArray (sequenceTrackStruct defaultValue:defaultValue keys:keyArray)
															)
														)
													)
													
													local conIndex = 0
													
													case typeID of
													(
														5: conIndex = 1
														6: conIndex = 2
													)
													
													if conIndex > 0 then
													(
														for h = 1 to trackArray.count do
														(
															for g = 1 to trackArray[h].keys.count do
															(
																addNewKey newDummy.controller[conIndex][h] trackArray[h].keys[g].time --= trackArray[h].defaultValue + trackArray[h].keys[g].value
																local tempKey = undefined
																
																for k = 1 to newDummy.controller[conIndex][h].keys.count do
																	if newDummy.controller[conIndex][h].keys[k].time == trackArray[h].keys[g].time then
																		tempKey = newDummy.controller[conIndex][h].keys[k]
																
																if tempKey != undefined then
																(
																	setProperty tempKey #value (trackArray[h].keys[g].value * (if conIndex == 1 then 100.0 else 1))
																	
																	local tempInVec = [0,0,0]
																	local currKeyTime = trackArray[h].keys[g].time
																	local preKeyTime = undefined
																	local nextKeyTime = undefined
																	if g>1 then
																	(
																		preKeyTime = trackArray[h].keys[g-1].time
																	)
																	
																	tempInVec[1] = trackArray[h].keys[g].easeIn[1]*30
																	tempInVec[2] = trackArray[h].keys[g].easeIn[2]
																	
																	local tempOutVec = [0,0,0]
																	tempOutVec[1] = trackArray[h].keys[g].easeOut[1]*30
																	tempOutVec[2] = trackArray[h].keys[g].easeOut[2]
																	
																	local tempOutVecNext = [0, 0, 0]
																	if g<trackArray[h].keys.count then
																	(
																		nextKeyTime = trackArray[h].keys[g+1].time
																	)
																	
																	setProperty tempKey #inTangentType #custom
																	setProperty tempKey #outTangentType #custom
																	local factor = (if conIndex == 1 then 100.0 else 1.0)
																	if  g>1 and (abs(currKeyTime-preKeyTime) > 1e-6) then
																		setProperty tempKey #inTangentLength ( tempInVec[1] /(currKeyTime-preKeyTime) )
																	else
																		setProperty tempKey #inTangentLength 0.0																			
																	
																	if (g<trackArray[h].keys.count) and (abs(nextKeyTime-currKeyTime) > 1e-6) then
																		setProperty tempKey #outTangentLength ( tempOutVec[1] /(nextKeyTime-currKeyTime) )
																	else
																		setProperty tempKey #outTangentLength 0.0
																		
																	setProperty tempKey #x_locked false 
																	setProperty tempKey #y_locked false 
																	setProperty tempKey #z_locked false
																			
																	if abs (tempInVec[1]) > 1e-6 then
																		setProperty tempKey #inTangent (-factor*tempInVec[2]*0.1876/tempInVec[1])
																	else
																		setProperty tempKey #inTangent 0.0
																			
																	if abs (tempOutVec[1]) > 1e-6 then
																		setProperty tempKey #outTangent (factor*tempOutVec[2]*0.1876/tempOutVec[1])
																	else
																		setProperty tempKey #outTangent 0.0	
																)
															)
														)
													)
												)
											)
										)
										newDummy.pos -= offset
									)
								)
							)
						)
					)
				)
			)
		)
	)

	function getTrackviewSequence layerPath index offsetToZero:false =
	(
		if doesFileExist layerPath then
		(
			sequences = getTrackviewSequences layerPath
			
			if index != undefined then
			(
				if classOf index == Integer then
					importSequenceData sequences[index] offsetToZero:offsetToZero
				
				if classOf index == String then
				(
					for i = 1 to sequences.count do
						if sequences[i].name == index then
							importSequenceData sequences[i] offsetToZero:offsetToZero
				)
			)
		)
	)	
		
	function writeSubTrack subTrackRoot keys isTranslation =
	(
		local factor = (if isTranslation then 100.0 else 1.0)
		local keyCnt = keys.count
		for i = 1 to keyCnt do
		(
			local xmlObjectNode = layerXmlObject.createElement "Key"
			local strTime = ( ( (keys[i].time.frame as Integer)/30.0) as string )
			local strValue = strTime + "," + ( (keys[i].value/factor) as string)
			local ds = [0, 0]
			local dd= [0, 0]
			if(i>1) then
			(
				ds[1] = keys[i].inTangentLength*( keys[i].time.frame as Integer- keys[i-1].time.frame as Integer)/30.0
				ds[2] = keys[i].inTangent*ds[1]*30.0/(-factor*0.1876)																			
			)
			if(i<keys.count) then
			(	
				dd[1] = keys[i].outTangentLength*(keys[i+1].time.frame as Integer - keys[i].time.frame as Integer)/30.0
				dd[2] = keys[i].outTangent*dd[1]*30.0/(factor*0.1876)						
			)
			local strDs = ds[1] as string + "," + ds[2] as string
			local strDd = dd[1] as string + "," + dd[2] as string
			
			xmlObjectNode.setAttribute "time" strTime
			xmlObjectNode.setAttribute "value" strValue
			xmlObjectNode.setAttribute "flags" "64"
			xmlObjectNode.setAttribute "ds" strDs
			xmlObjectNode.setAttribute "dd" strDd
			
			subTrackRoot.appendChild xmlObjectNode
		)
	)
	
	function exportSequence sequence offsetToZero:false =
	(
		if sequence != undefined then
		(
			for i = 0 to (sequence.root.childNodes.count - 1) do
			(
				case sequence.root.childNodes.itemOf[i].name of
				(
					"Nodes":
					(
						local nodesRoot = sequence.root.childNodes.itemOf[i]
						
						for d = 0 to (nodesRoot.childNodes.count - 1) do -- for each object in the current sequence
						(
							case nodesRoot.childNodes.itemOf[d].name of
							(
								"Node":
								(
									local nodeRoot = nodesRoot.childNodes.itemOf[d]
									
									local nodeName = ""
									local nodePos = [0,0,0]
									local nodeRot = quat 1
									local nodeScale = [1,1,1]
									
									for f = 0 to (nodeRoot.attributes.count - 1) do -- Node attributes
									(
										case nodeRoot.attributes.itemOf[f].name of
										(
											"Name":		nodeName = nodeRoot.attributes.itemOf[f].value
											"Pos":		nodePos = execute ("[" + nodeRoot.attributes.itemOf[f].value + "]")
											"Rotate":
											(
												local tempFilter = filterString nodeRoot.attributes.itemOf[f].value ","
												nodeRot = execute ("quat " + tempFilter[1] + " " + tempFilter[2] + " " + tempFilter[3] + " " + tempFilter[4])
											)
											"Scale":		nodeScale = execute ("[" + nodeRoot.attributes.itemOf[f].value + "]")
										)
									)
									
									-- look for matching objects in Max
									local matchedObj = undefined
									for i=1 to selection.count do
									(
										if selection[i].name == nodeName then
										(
											matchedObj = selection[i]
											print "Matched object found, exporting its tracks!"
											print matchedObj.name
										)
									)
									
									if nodeName != "" then
									(													
										for f = 0 to (nodeRoot.childNodes.count - 1) do
										(
											case nodeRoot.childNodes.itemOf[f].name of
											(
												"Track":
												(
													local trackRoot = nodeRoot.childNodes.itemOf[f]
													local paramID = 0
													local typeID = 0
													local start = startTime
													local end = endTime
													
													for h = 0 to (trackRoot.attributes.count - 1) do
													(
														case trackRoot.attributes.itemOf[h].name of
														(
															"ParamId":		paramID = execute trackRoot.attributes.itemOf[h].value
															"Type":			typeID = execute trackRoot.attributes.itemOf[h].value
															"Flags":			flags = execute trackRoot.attributes.itemOf[h].value
															"StartTime":	start = execute trackRoot.attributes.itemOf[h].value
															"EndTime":		end = execute trackRoot.attributes.itemOf[h].value
														)
													)
													for h = 0 to (trackRoot.childNodes.count - 1) do
													(
														case trackRoot.childNodes.itemOf[h].name of
														(
															"NewSubTrack":
															(
																local subTrackRoot = trackRoot.childNodes.itemOf[h]
																
																if matchedObj != undefined and paramID == 1 and typeID == 5 then--position tracks
																(
																	local attribDefaultVal = subTrackRoot.getAttribute "defaultValue"
																	subTrackRoot.RemoveAll()
																	subTrackRoot.setAttribute "Flags" "0"
																	subTrackRoot.setAttribute "defaultValue" attribDefaultVal
																	
																	if h == 0 then
																	(																		
																		local mKeys = matchedObj.pos.controller.X_Position.controller.keys
																		writeSubTrack subTrackRoot mKeys true																																	
																	)
																	else if h == 1 then
																	(
																		local mKeys = matchedObj.pos.controller.Y_Position.controller.keys
																		writeSubTrack subTrackRoot mKeys	true																				
																	)
																	else if h == 2 then
																	(
																		local mKeys = matchedObj.pos.controller.Z_Position.controller.keys
																		writeSubTrack subTrackRoot mKeys	 true			
																	)
																)
																
																if matchedObj != undefined and paramID == 2 and typeID == 6 then--rotation tracks
																(
																	local attribDefaultVal = subTrackRoot.getAttribute "defaultValue"
																	subTrackRoot.RemoveAll()
																	subTrackRoot.setAttribute "Flags" "0"
																	subTrackRoot.setAttribute "defaultValue" attribDefaultVal
																	
																	if h == 0 then
																	(																		
																		local mKeys = matchedObj.rotation.controller.X_Rotation.controller.keys
																		writeSubTrack subTrackRoot mKeys	false																	
																	)
																	else if h == 1 then
																	(
																		local mKeys = matchedObj.rotation.controller.Y_Rotation.controller.keys
																		writeSubTrack subTrackRoot mKeys false		
																	)
																	else if h == 2 then
																	(
																		local mKeys = matchedObj.rotation.controller.Z_Rotation.controller.keys
																		writeSubTrack subTrackRoot mKeys false			
																	)
																	
																)
																
															)
														)
													)
												)
											)
										)

									)
								)
							)
						)
					)
				)
			)
		)
	)


	on btnSetLayer pressed do
	(
		sLayerPath =  getOpenFilename caption:"Load Layer File" types:"Layer File (*.lyr)|*.lyr" initialDir:sGameDataDir
		
		if sLayerPath != undefined then
		(
			local fltLayerPath = filterString sLayerPath "\\/"
			local cnt = fltLayerPath.count
			btnSelectedLayer.caption = fltLayerPath[cnt-2] + ": " +fltLayerPath[cnt]
			btnSelectedLayer.enabled = true
			btnImport.enabled = true
			btnExport.enabled = true
			
			sequences = getTrackviewSequences sLayerPath
			local nameArray = #()
			for i = 1 to sequences.count do
				append nameArray sequences[i].name
			sequencesInLayer.items = nameArray		
		)
		else
		(
			btnSetLayer.caption = "Set Layer"
			btnSelectedLayer.enabled = false
			btnImport.enabled = false
			btnExport.enabled = false
		)
	)
	
	on btnImport pressed do
	(
		if sequencesInLayer.selection > 0 then
			getTrackviewSequence sLayerPath sequencesInLayer.selection offsetToZero:false
	)
	
	on btnExport pressed do
	(
		cnt = selection.count
		exportSequence sequences[sequencesInLayer.selection] offsetToZero:false
		deleteFile (sLayerPath+".backup")
		copyFile sLayerPath (sLayerPath+".backup")
		layerXmlObject.Save sLayerPath -- overwrite the current layer file
	)

)

cryMaxTools.basic.ROMan.cryAdd "rltCryImExportSandbox" rltCryImExportSandbox #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryImExportSandbox")