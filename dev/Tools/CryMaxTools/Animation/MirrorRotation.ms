try destroyDialog mirrorRO catch()


mirrorRO = undefined

rollout mirrorRO "Mirror Rotation"
(
	local axisArray = #()
	local saveTM = #()
	
	
	groupbox gbMirror " Mirror Axis " pos:[5,5] width:130 height:80
	
	label labMirrorX "X" pos:[25,25]
	checkbox radMirrorX "" pos:[40,25] labels:#("") columns:1 default:1
	
	label labMirrorY "Y" pos:[25,45]
	checkbox radMirrorY "" pos:[40,44] labels:#("") columns:1 default:0
	
	label labMirrorZ "Z" pos:[25,65]
	checkbox radMirrorZ "" pos:[40,64] labels:#("") columns:1 default:0
	
	
	label labMirrorXY "XY" pos:[80,25]
	checkbox radMirrorXY "" pos:[100,24] labels:#("") columns:1 default:0
	
	label labMirrorYZ "YZ" pos:[80,45]
	checkbox radMirrorYZ "" pos:[100,44] labels:#("") columns:1 default:0
	
	label labMirrorZX "ZX" pos:[80,65]
	checkbox radMirrorZX "" pos:[100,64] labels:#("") columns:1 default:0
	
	
	groupbox gbRef " Reference System " pos:[5,95] width:130 height:43
	dropdownlist ddRef "" pos:[10,110] width:120 items:#("            World", "            Local", "           Screen", "-------------------------------------", "             Pick")
	
	
	groupbox gbUpAxis "Up Axis" pos:[5,145] width:130 height:80
	
	label labUpX "X" pos:[25,25+140]
	checkbox radUpX "" pos:[40,25+140] labels:#("") columns:1 default:1
	
	label labUpY "Y" pos:[25,45+140]
	checkbox radUpY "" pos:[40,44+140] labels:#("") columns:1 default:0
	
	label labUpZ "Z" pos:[25,65+140]
	checkbox radUpZ "" pos:[40,64+140] labels:#("") columns:1 default:0
	
	
	label labUpXY "XY" pos:[80,25+140]
	checkbox radUpXY "" pos:[100,24+140] labels:#("") columns:1 default:0
	
	label labUpYZ "YZ" pos:[80,45+140]
	checkbox radUpYZ "" pos:[100,44+140] labels:#("") columns:1 default:0
	
	label labUpZX "ZX" pos:[80,65+140]
	checkbox radUpZX "" pos:[100,64+140] labels:#("") columns:1 default:0
	
	
	label labUse "Use :" pos:[5,230]
	
	label labUsePos "Pos" pos:[40+5,230]
	checkbox chkUsePos "" pos:[65+5,230] checked:true
	
	label labUseRot "Rot" pos:[40+50+5,230]
	checkbox chkUseRot "" pos:[65+50+5,230] checked:true
	
	
	checkbutton btnPreview "Preview" pos:[5,230+25] width:130 height:20
	button btnMirror "Mirror" pos:[5,265+25] width:130 height:20
	
	
	
	
	function getMappingAxis tm1 tm2 =
	(
		local mappingTM = tm1 * inverse tm2
		local mappingAxis = [0,0,0]
		local mappingDir = [1,1,1]
		
		for i = 1 to 3 do
		(
			local axisOrig = mappingTM[i]
			local axisNormal = mappingTM[i]
			
			local maxValue = 0.0
			local axisID = 1
			local axisFlipped = false
			
			for d = 1 to 3 do
			(
				if axisNormal[d] < 0 then
					axisNormal[d] *= -1
				
				if axisNormal[d] > maxValue then
				(
					maxValue = axisNormal[d]
					axisID = d
				)
			)
			
			mappingAxis[i] = axisID
			
			if axisOrig[axisID] < 0 then
				mappingDir[i] = -1
		)
		
		
		return (#(mappingAxis, mappingDir))
	)



	function mirrorTM objTM axis planeAxis:undefined refTM:undefined rotOnly:undefined posOnly:undefined =
	(
		local originalTM = objTM
		local spaceTM = matrix3 1
		
		
		if refTM != undefined then
			spaceTM = refTM
		
		if classOf spaceTM == Matrix3 and classOf originalTM == Matrix3 then
		(
			local mappingAxis = getMappingAxis spaceTM originalTM 
			
			local localTM = originalTM * inverse spaceTM
			
			local mirrorAxis = [1,1,1]
			local scaleAxis = [1,1,1]
			local rotateAxis = [0,0,0]
			
			local mirrorAxisID = 0
			local scaleAxisID = 0
			local rotateAxisID = 0
			
			
			mirrorAxisID = case axis of
			(
				#X:		1
				#Y:		2
				#Z:		3
				
				#XZ:		1
				#ZX:		1
				
				#XY:		2
				#YX:		2
				
				#YZ:		3
				#ZY:		3
			)
			
			if planeAxis != undefined then
			(
				scaleAxisID = case planeAxis of
				(
					#X:		3
					#Y:		1
					#Z:		2
					
					#XY:		3
					#XZ:		2
					
					#YX:		3
					#YZ:		1
					
					#ZX:		2
					#ZY:		1
				)
			)
			else
			(
				local maxDist = 0
				local axisID = mappingAxis[1][mirrorAxisID]
				
				scaleAxisID = case axisID of
				(
					1:		2
					2:		3
					3:		1
				)
			)
			
			
			local rotateAxisID = case mirrorAxisID of
			(
				1:		3
				2:		1
				3:		2
			)
			
			
			mirrorAxis[mirrorAxisID] = -1
			scaleAxis[scaleAxisID] = -1
			rotateAxis[rotateAxisID] = 1
			
			

				--// first scale the local matrix in the mirrorAxis direction, then flip it on the scaleAxis direction
			local mirrorTM = (scaleMatrix scaleAxis)*(localTM*(scaleMatrix mirrorAxis))

				--// rotate an identity matrix 180° alongside the last axis (non-scale and non-mirror axis)
			local rotateTM = rotate (matrix3 1) (eulerangles (rotateAxis[1]*180) (rotateAxis[2]*180) (rotateAxis[3]*180))
				
				--// add the rotateTM (rotated 180°) to the mirrorTM
			mirrorTM = mirrorTM * rotateTM

				--// add the mirrorTM to the matrix in coordsys of $Point03
			local newTM = mirrorTM * spaceTM
				
				
			if posOnly == true then
			(
				local tempPos = newTM.pos
				newTM.rotation = originalTM.rotation
				newTM.pos = tempPos
			)
			
			if rotOnly == true then
				newTM.pos = originalTM.pos
			
			
			return newTM
		)
		
		return objTM
	)

	
	
	
	
	function startMirroring usedTM:undefined =
	(
		local mirrorAxis = 0
		local upAxis = 0
		local posOnly = false
		local rotOnly = false
		
		local refTM = matrix3 1
		
		posOnly = (chkUsePos.checked == true and chkUseRot.checked == false)
		rotOnly = (chkUseRot.checked == true and chkUsePos.checked == false)
			
		
		local axisNames = #(#X,#Y,#Z,#XY,#YZ,#ZX)
		
		for i = 1 to 12 do
		(
			if i <= 6 then
			(
				if axisArray[i].checked == true then
					mirrorAxis = axisNames[i]
			)
			else
			(
				if axisArray[i].checked == true then
					upAxis = axisNames[i - 6]
			)
		)
		
		
		
		
		
		
		
		if mirrorAxis != 0 then
		(
			local selArray = getCurrentSelection()
			
			for i = 1 to selArray.count do
			(
				local obj = selArray[i]
				local refTM = matrix3 1
				
				if ddRef.selection < 4 then
				(
					case ddRef.selection of
					(
						2:		refTM = obj.transform
						3:		refTM = inverse (getViewTM())
					)
				)
				else
				(
					local tempNode = getNodeByName ddRef.selected
					
					if tempNode != undefined then
						refTM = tempNode.transform
				)
				
				
				local baseTM = obj.transform
				if usedTM != undefined then
					baseTM = usedTM
				
				obj.transform = mirrorTM  baseTM mirrorAxis planeAxis:(if upAxis != 0 then upAxis else undefined) posOnly:posOnly rotOnly:rotOnly refTM:refTM
			)
		)
	)
	
	
	
	
	function updateAxis index overwrite:undefined =
	(
		local start = 1
		local end = 6
		
		if index > 6 then
		(
			start = 7
			end = 12
			
		)
		
		for i = start to end do
		(
			if i != index then
				axisArray[i].checked = false
		)

		if overwrite == undefined then
			axisArray[index].checked = true
		else
			axisArray[index].checked = overwrite
		
		if btnPreview.checked == true then
			startMirroring usedTM:saveTM
	)
	
	
	
	
	
	on mirrorRO open do
	(
		axisArray = #( radMirrorX, radMirrorY, radMirrorZ, radMirrorXY, radMirrorYZ, radMirrorZX, radUpX, radUpY, radUpZ, radUpXY, radUpYZ, radUpZX )
		
		updateAxis 1
	)
	
	
	on radMirrorX changed value do
		updateAxis 1
	
	on radMirrorY changed value do
		updateAxis 2
	
	on radMirrorZ changed value do
		updateAxis 3
	
	on radMirrorXY changed value do
		updateAxis 4
	
	on radMirrorYZ changed value do
		updateAxis 5
	
	on radMirrorZX changed value do
		updateAxis 6
	
	
	
	on radUpX changed value do
		updateAxis 7 overwrite:value
	
	on radUpY changed value do
		updateAxis 8 overwrite:value
	
	on radUpZ changed value do
		updateAxis 9 overwrite:value
	
	on radUpXY changed value do
		updateAxis 10 overwrite:value
	
	on radUpYZ changed value do
		updateAxis 11 overwrite:value
	
	on radUpZX changed value do
		updateAxis 12 overwrite:value


	
	
	on ddRef selected value do
	(
		if value == 4 then
			ddRef.selection = 3
		
		if value == 5 then
		(
			local tempNode = pickObject count:1 forceListenerFocus:false
			if tempNode != undefined then
			(
				local tempArray = ddRef.items
				append tempArray tempNode.name
				ddRef.items = tempArray
				
				ddRef.selection = tempArray.count
			)
			else
				ddRef.selection = 1
		)
		
		if btnPreview.checked == true and (ddRef.selection < 4 or ddRef.selection > 5) then
			startMirroring usedTM:saveTM
	)
	
	
	
	
	on chkUsePos changed value do
	(
		if value == false and chkUseRot.checked == false then
			chkUsePos.checked = true
		
		if btnPreview.checked == true then
			startMirroring usedTM:saveTM
	)
	
	on chkUseRot changed value do
	(
		if value == false and chkUsePos.checked == false then
			chkUseRot.checked = true
		
		if btnPreview.checked == true then
			startMirroring usedTM:saveTM
	)
	
	
	on btnPreview changed value do
	(
		if value == true then
		(
			saveTM = #()
			
			for obj in selection do
				append saveTM obj.transform
			
			startMirroring usedTM:saveTM
		)
		else
		(
			with animate off
			(
				local selArray = getCurrentSelection()
				
				for i = 1 to saveTM.count do
					selArray[i].transform = saveTM[i]
			)
		)
		
	)
	
	
	on btnMirror pressed do
	(
		if btnPreview.checked == true then
			btnPreview.checked = false
		else
			startMirroring()
	)
)


createDialog mirrorRO 140 315