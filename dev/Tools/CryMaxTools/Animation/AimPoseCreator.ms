try destroyDialog dbc catch()





rollout dbc "   Directional Blend Creator"
(
	local mainMenu = RCMenu mainMenu
	(
		subMenu " File "
		(
			menuItem miOpen "Open"
			separator sep1
			menuItem miSave "Save"
			menuItem miSaveAs "Save As"
			separator sep2
			menuItem miExit "Exit"
		)
		
		subMenu " Presets "
		(
			menuItem miAddPreset "Add Preset"
			menuItem miAddPoses "Add Poses"
			separator miSep1
			menuItem miCutPreset "Cut"
			menuItem miCopyPreset "Copy"
			menuItem miPastePreset "Paste"
			separator miSep2
			menuItem miDeletePreset "Delete"
		)
		
		subMenu " Nodes "
		(
			menuItem miAddNode "Add"
			menuItem miReplaceNode "Replace"
			menuItem miDeleteNode "Delete"
			separator miSep3
			menuItem miSetInfluence "Set Influence"
			separator miSep4
			
			subMenu "Parent"
			(
				menuItem miSetParent "Node"
				--menuItem miSetSceneParent "Scene"
				separator miSep5
				menuItem miRemoveParent "Remove"
				separator miSep6
				menuItem miAutoParent "Auto"
			)
			separator miSep7
			
			subMenu "Align"
			(
				menuItem miSetAlign "Node"
				menuItem miSetAlignScene "Scene"
				separator miSep71
				menuItem miRemoveAlign "Remove"
			)
		)
		
		subMenu " Poses "
		(
			menuItem miUpdateBasePose "Update Base Pose"
			separator miSep8
			menuItem miUpdateOffsetPoses "Update Offset Poses"
			menuItem miRemoveOffsetPoses "Remove Offset Poses"
			separator miSep9
			menuItem miCreatePoses "Create Poses"
		)
		
		subMenu " Tools "
		(
			menuItem miAlignToSelection "Align to Selection"
			separator miSep10
			menuItem miZeroSelection "Zero out Selection"
		)
		
		
		function updateMenu =
		(
			local presetsSelection = false
			local presetVisible = dbc.selPreset != undefined and dbc.selPreset.selection == false
					
			miAddPoses.enabled = presetVisible
			miCutPreset.enabled = presetVisible
			miCopyPreset.enabled = presetVisible
			miPastePreset.enabled = dbc.copyPreset != undefined or dbc.copyPose != undefined
			miDeletePreset.enabled = presetVisible
			
			
			
			local nodeSelection = dbc.selPreset != undefined and dbc.selPreset.selection == false
			local hasNodes = dbc.lvNodes.selectedCells.count > 0
			local noSelection = hasNodes and nodeSelection
			
			miAddNode.enabled = nodeSelection
			miReplaceNode.enabled = noSelection
			miDeleteNode.enabled = noSelection
			miSetInfluence.enabled = noSelection
			
			miSetParent.enabled = noSelection
			miRemoveParent.enabled = noSelection
			miAutoParent.enabled = noSelection
			
			miSetAlign.enabled = hasNodes
			miSetAlignScene.enabled = hasNodes
			miRemoveAlign.enabled = hasNodes
			
			
			
			
			miUpdateBasePose.enabled = dbc.btnUpdateBasePose.enabled
			miUpdateOffsetPoses.enabled = dbc.btnUpdateOffsetPoses.enabled
			miRemoveOffsetPoses.enabled = dbc.btnRemoveOffsetPoses.enabled
			miCreatePoses.enabled = dbc.btnCreatePoses.enabled
			
			miAlignToSelection.enabled = dbc.btnSelectionToAlign.enabled
			miZeroSelection.enabled = dbc.btnSelectionToAlignZero.enabled
		)
		
			--// FILE
		
		
		on miOpen picked do
			dbc.loadPresets()
		
		on miSave picked do
			dbc.savePresets()
		
		on miSaveAs picked do
			dbc.savePresets saveAs:true
		
		on miExit picked do
			try destroyDialog dbc catch()
		
		
		
		
			--// PRESETS
		
		
		on miAddPreset picked do
			dbc.addPresetEntry()
		
		on miAddPoses picked do
			dbc.addPoseEntry()
		
		on miCutPreset picked do
			dbc.cutPresetEntry()
		
		on miCopyPreset picked do
			dbc.copyPresetEntry()
		
		on miPastePreset picked do
			dbc.pastePresetEntry()
		
		on miDeletePreset picked do
			dbc.deletePresetEntry()
		
		
		
		
		
		
		
			--// NODES
		
		
		on miAddNode picked do
			dbc.addNodeEntry()
		
		on miReplaceNode picked do
			dbc.replaceNodeEntry()
		
		on miDeleteNode picked do
			dbc.deleteNodeEntry()
		
		on miSetInfluence picked do
			dbc.setNodeInfluence()
		
		
		
		on miSetParent picked do
			dbc.setNodeParent()
		
		on miRemoveParent picked do
			dbc.removeNodeParent()
		
		on miAutoParent picked do
			dbc.nodeAutoParent()
		
		
		
		on miSetAlign picked do
			dbc.setNodeAlign()
		
		on miSetAlignScene picked do
			dbc.setSceneAlign()
		
		on miRemoveAlign picked do
			dbc.removeNodeAlign()
		
		
		
			--// POSES
		
		on miUpdateBasePose picked do
			dbc.updateBasePose()
		
		on miUpdateOffsetPoses picked do
			dbc.updateOffsetPoses()
		
		on miRemoveOffsetPoses picked do
			dbc.removeOffsetPoses()
		
		on miCreatePoses picked do
			dbc.createPose 1 all:true
		
		
		
			--// TOOLS
		
		
		on miSelectionToAlign picked do
			dbc.selectionToAlign()
		
		on miSelectionToAlignZero picked do
			dbc.selectionToAlignZero()
	)
	
	
	
	struct poseFrameStruct
	(
		yaw = 0.0,
		pitch = 0.0
	)
	
	struct poseStruct
	(
		ID = 0,
		frames = #()
	)
	
	
	
	struct nodeStruct
	(
		node = "",
		defaultTM = (matrix3 1),
		offsetTM = (matrix3 1),
		rotOffset = (quat 1),
		parent,
		defaultPoseTM = (matrix3 1),
		offsetPoseTM = (matrix3 1),
		delta,
		tempKeys = #(),
		baseAnim = #(),
		localBaseAnim = #(),
		aimIKPoses = #(),
		influence = 0.0,
		align,
		poses = #()
	)
	
	
	
	
	struct posePresetStruct
	(
		name,
		frames = #(),
		yaw = 0.0,
		pitch = 0.0,
		frameCount = 3,
		tn = undefined
	)
	
	
	struct presetStruct
	(
		name = "",
		children = #(),
		tn = undefined,
		nodes = undefined,
		poses = undefined,
		selection = false
	)
	
	
	
	
	
	
	
	
	
	
	local sourcePath = ""
	
	local xmlFile = undefined
	
	local presets = undefined
	
	local nodes = #()
	local poses = undefined
	
	local alignNodeTo = undefined
	local alignNodeDefaultTM = (matrix3 1)
	
	local previewUsedCount = 0
	
	local filePath = ""
	
	
	local tempFrameIndex = 0
	
	local toParentNodes = #()
	
	local toEditNodes = #()
	local doEditPos = undefined
	local doDrag = -1
	local doEdit = -1
	local doEditDrag = false
	
	local copyPreset = undefined
	local copyPose = undefined
	local cutPart = undefined
	local cutActive = false
	
	
	local setValueOP = undefined
	local setValueVar = undefined
	local setValueHeader = undefined
	local setValueInit = ""
	
	
	local selPreset = undefined
	local selParentPreset = undefined
	local selPose = undefined
	
	local useSelection = false
	
	local savedPresets = #()
	
	
	dotNetControl lvPresets "System.Windows.Forms.TreeView" pos:[5,5] width:215 height:300
	dotNetControl lvNodes "System.Windows.Forms.DataGridView" pos:[160+68,5] height:300 width:417
	
	
	
	
	
	groupbox gbPoses " Poses " pos:[555+100,5] height:120 width:340
	
	spinner spnFrameCount "Horizontal Frames : " pos:[565+100,20] height:18 fieldWidth:30 type:#integer range:[3,19,0] scale:2.0
	spinner spnSetYaw "Yaw : " pos:[710+100,20] height:18 fieldWidth:40 range:[0,180,0]
	spinner spnSetPitch "Pitch : " pos:[805+100,20] height:18 fieldWidth:40 range:[0,180,0]
	
	
	button btnUpdateBasePose "Update Base Pose" pos:[555+100,135] height:20 width:150
	checkbox chkUseOffsetPoses "" pos:[555+160+100,135] height:20 width:20 checked:true
	button btnUpdateOffsetPoses "Update Offset Poses" pos:[555+160+100+16,135] height:20 width:(157-16)
	button btnRemoveOffsetPoses "-" pos:[555+160+150+10+100,137] height:15 width:15
	button btnCreatePoses "Create Poses" pos:[555+100,160] height:30 width:340
	
	
	label labInfo "--------------------------" pos:[560+100,200] height:50 width:340
	
	
	groupbox gbAlignTools " Tools " pos:[555+100,200+60] height:(105-60) width:340
	checkbox chkUseAlign "Use Align in List" pos:[575+100,218+60] width:100 checked:true
	button btnSelectionToAlign "Align" pos:[700+100,218+60] height:18 width:55
	button btnSelectionToAlignZero "Zero" pos:[770+100,218+60] height:18 width:55
	
	
	button btnPose01 "1" pos:[570,25] height:14 width:14 visible:false
	button btnPose02 "2" pos:[570,25] height:14 width:14 visible:false
	button btnPose03 "3" pos:[570,25] height:14 width:14 visible:false
	button btnPose04 "4" pos:[570,25] height:14 width:14 visible:false
	button btnPose05 "5" pos:[570,25] height:14 width:14 visible:false
	button btnPose06 "6" pos:[570,25] height:14 width:14 visible:false
	button btnPose07 "7" pos:[570,25] height:14 width:14 visible:false
	button btnPose08 "8" pos:[570,25] height:14 width:14 visible:false
	button btnPose09 "9" pos:[570,25] height:14 width:14 visible:false
	button btnPose10 "10" pos:[570,25] height:14 width:14 visible:false
	
	button btnPose11 "11" pos:[570,25] height:14 width:14 visible:false
	button btnPose12 "12" pos:[570,25] height:14 width:14 visible:false
	button btnPose13 "13" pos:[570,25] height:14 width:14 visible:false
	button btnPose14 "14" pos:[570,25] height:14 width:14 visible:false
	button btnPose15 "15" pos:[570,25] height:14 width:14 visible:false
	button btnPose16 "16" pos:[570,25] height:14 width:14 visible:false
	button btnPose17 "17" pos:[570,25] height:14 width:14 visible:false
	button btnPose18 "18" pos:[570,25] height:14 width:14 visible:false
	button btnPose19 "19" pos:[570,25] height:14 width:14 visible:false
	button btnPose20 "20" pos:[570,25] height:14 width:14 visible:false
	
	button btnPose21 "21" pos:[570,25] height:14 width:14 visible:false
	button btnPose22 "22" pos:[570,25] height:14 width:14 visible:false
	button btnPose23 "23" pos:[570,25] height:14 width:14 visible:false
	button btnPose24 "24" pos:[570,25] height:14 width:14 visible:false
	button btnPose25 "25" pos:[570,25] height:14 width:14 visible:false
	button btnPose26 "26" pos:[570,25] height:14 width:14 visible:false
	button btnPose27 "27" pos:[570,25] height:14 width:14 visible:false
	button btnPose28 "28" pos:[570,25] height:14 width:14 visible:false
	button btnPose29 "29" pos:[570,25] height:14 width:14 visible:false
	button btnPose30 "30" pos:[570,25] height:14 width:14 visible:false
	
	button btnPose31 "31" pos:[570,25] height:14 width:14 visible:false
	button btnPose32 "32" pos:[570,25] height:14 width:14 visible:false
	button btnPose33 "33" pos:[570,25] height:14 width:14 visible:false
	button btnPose34 "34" pos:[570,25] height:14 width:14 visible:false
	button btnPose35 "35" pos:[570,25] height:14 width:14 visible:false
	button btnPose36 "36" pos:[570,25] height:14 width:14 visible:false
	button btnPose37 "37" pos:[570,25] height:14 width:14 visible:false
	button btnPose38 "38" pos:[570,25] height:14 width:14 visible:false
	button btnPose39 "39" pos:[570,25] height:14 width:14 visible:false
	button btnPose40 "40" pos:[570,25] height:14 width:14 visible:false
	
	button btnPose41 "41" pos:[570,25] height:14 width:14 visible:false
	button btnPose42 "42" pos:[570,25] height:14 width:14 visible:false
	button btnPose43 "43" pos:[570,25] height:14 width:14 visible:false
	button btnPose44 "44" pos:[570,25] height:14 width:14 visible:false
	button btnPose45 "45" pos:[570,25] height:14 width:14 visible:false
	button btnPose46 "46" pos:[570,25] height:14 width:14 visible:false
	button btnPose47 "47" pos:[570,25] height:14 width:14 visible:false
	button btnPose48 "48" pos:[570,25] height:14 width:14 visible:false
	button btnPose49 "49" pos:[570,25] height:14 width:14 visible:false
	button btnPose50 "50" pos:[570,25] height:14 width:14 visible:false
	
	button btnPose51 "51" pos:[570,25] height:14 width:14 visible:false
	button btnPose52 "52" pos:[570,25] height:14 width:14 visible:false
	button btnPose53 "53" pos:[570,25] height:14 width:14 visible:false
	button btnPose54 "54" pos:[570,25] height:14 width:14 visible:false
	button btnPose55 "55" pos:[570,25] height:14 width:14 visible:false
	button btnPose56 "56" pos:[570,25] height:14 width:14 visible:false
	button btnPose57 "57" pos:[570,25] height:14 width:14 visible:false
	button btnPose58 "58" pos:[570,25] height:14 width:14 visible:false
	button btnPose59 "59" pos:[570,25] height:14 width:14 visible:false
	button btnPose60 "60" pos:[570,25] height:14 width:14 visible:false
	
	
	
	
	function getPoseFrames frameCount yaw pitch =
	(
		local poseFrameArray = #()
				
		local yawStep = (frameCount - 1) / 2
		
		local yawAngleStep = yaw / yawStep
		local pitchStep = 1
		
		
		
		for i = pitchStep to -pitchStep by -1 do
			for d = -yawStep to yawStep do
				append poseFrameArray (poseFrameStruct yaw:(yawAngleStep * d) pitch:(pitch * i))

		return poseFrameArray
	)	
	
	
	

	
	
	
	
	function sortPresetsByNames n1 n2 =
	(
			--// used for deleting entries
		if n1.name != undefined and n2.name != undefined then
		(
			if toLower n1.name > toLower n2.name then
				return 1
			else if toLower n1.name == toLower n2.name then
				return 0
			else
				return -1
		)
		else
			return -1
	)
	
	
	
	
	
	function sortPresets root:undefined start:false =
	(
		if start == true then
		(
			root = presets
			qsort root sortPresetsByNames
			
			for i = 1 to root.count do
				sortPresets root:root[i]
		)
		else
		(
			if root != undefined then
			(
				if root.poses != undefined then
					qsort root.poses sortPresetsByNames
			
				qsort root.children sortPresetsByNames
				
				for i = 1 to root.children.count do
					sortPresets root:root.children[i]
			)
		)
		
		return true
	)
	
	
	
	
	
	function updatePreview =
	(
		if poses != undefined then
		(
			local maxPoses = previewUsedCount
			
			
			if previewUsedCount < poses.frames.count then
				maxPoses = poses.frames.count
			
			local leftOffset = 330
			local topLeftPos = [dbc.width - leftOffset,50]
			
			local maxYawStep = poses.frameCount - 1
			local yawStep = poses.yaw / maxYawStep
			local yawPixelStep = (leftOffset - 32) / maxYawStep
			
			local maxPitchStep = 2
			local pitchStep = poses.pitch
			local pitchPixelStep = 25
			
			local btnCount = 1
			
			local btnArray = #()
			local poseBtn = undefined
			
			for i = 1 to 60 do
			(
				poseBtn = execute ("dbc.btnPose" + (if i > 9 then "" else "0") + i as String)
				poseBtn.visible = false
				
				append btnArray poseBtn
			)
			
			
			
			for i = 0 to maxPitchStep do
			(
				for d = 0 to maxYawStep do
				(
					local newPos = topLeftPos + [(yawPixelStep * d), (pitchPixelStep * i)]
					
					
					poseBtn = btnArray[btnCount]
					
					poseBtn.pos = newPos
					poseBtn.visible = true
					
					btnCount += 1
				)
			)
			
			previewUsedCount = maxPoses
		)
		else
		(
			for i = 1 to 60 do
			(
				local poseBtn = execute ("dbc.btnPose" + (if i > 9 then "" else "0") + i as String)
				poseBtn.visible = false
			)
		)
	)
	

	
	
	
	
	
	function getValues =
	(
		if toParentNodes.count > 0 then
		(
			if lvNodes.selectedCells.count > 0 then
			(
				local rowIndex = (lvNodes.selectedCells.item 0).rowIndex
				local parent = undefined
				
				if rowIndex < lvNodes.rowCount then
					parent = nodes[rowIndex + 1].node
				
				for i = 1 to toParentNodes.count do
				(
					if nodes[toParentNodes[i] + 1].node == parent then
					(
						parent = undefined
						
						if nodes[toParentNodes[i] + 1].parent == undefined then
							parent = "Scene"
					)
					
					
					nodes[toParentNodes[i] + 1].parent = parent
					((lvNodes.rows.item toParentNodes[i]).cells.item 1).value = parent
				)
			)
			
			toParentNodes = #()
			
			return true
		)
		
		spnFrameCount.value = 0
		spnSetYaw.value = 0.0
		spnSetPitch.value = 0.0
		
		
		if poses != undefined then
		(
			spnFrameCount.value = poses.frameCount
			spnSetYaw.value = poses.yaw
			spnSetPitch.value = poses.pitch
		)
		
		
		try
		(
			if nodes.count > 0 and lvNodes.selectedCells.count > 0 then
			(
				local rowIndex = (lvNodes.selectedCells.item (lvNodes.selectedCells.count - 1)).rowIndex
				local columnIndex = (lvNodes.selectedCells.item (lvNodes.selectedCells.count - 1)).ColumnIndex
				
				spnInfluence.value = nodes[rowIndex+1].influence
				
				local poseIndex = 1
				
				if columnIndex > 2 then
					poseIndex = columnIndex - 3
			)
		)catch()
		
	)
	
	
	
	function setValue con =
	(
		if nodes.count > 0 then
		(
			for i = 0 to (lvNodes.selectedCells.count - 1) do
			(
				local selCell = lvNodes.selectedCells.item i
				local rowIndex = selCell.rowIndex
				if rowIndex > (nodes.count - 1) then
					exit
				
			
				columnIndex = selCell.columnIndex
				
				case con of
				(
					#influence:
					(
						((lvNodes.Rows.item rowIndex).cells.item 2).value = spnInfluence.value as String
						nodes[rowIndex+1].influence = spnInfluence.value
						
						local newBackStyle = dotNetObject "System.Windows.Forms.DataGridViewCellStyle"
						
						if spnInfluence.value >= 0.0 then
						(
							local counterValue = 255.0 - (150.0 * spnInfluence.value) / 100
							newBackStyle.backColor = newBackStyle.backColor.FromArgb 255 counterValue counterValue
						)
						else
						(
							local counterValue = 255.0 + (150.0 * spnInfluence.value) / 100
							newBackStyle.backColor = newBackStyle.backColor.FromArgb counterValue counterValue 255 
						)
						
						((lvNodes.Rows.item rowIndex).cells.item 0).style = newBackStyle
						
					)
					
					#yaw:
					(
						(lvNodes.Columns.item columnIndex).name = "Pose " + (columnIndex-3) as String + "    Y: " + spnYaw.value as String + "  P: " + spnPitch.value as String
						poses.frames[columnIndex-3].yaw = spnYaw.value
						
						if ddPosePreset.selection != 2 then
							ddPosePreset.selection = 2
						
						updatePreview()
					)
					
					#pitch:
					(
						(lvNodes.Columns.item columnIndex).name = "Pose " + (columnIndex-3) as String + "    Y: " + spnYaw.value as String + "  P: " + spnPitch.value as String
						poses.frames[columnIndex-3].pitch = spnPitch.value
						
						if ddPosePreset.selection != 2 then
							ddPosePreset.selection = 2
						
						updatePreview()
					)
				)
			)
		)
	)
	
	
	
	
		--// check, if the dot product of both quats are negative and multiplicate one of the quats by -1
	function normalizeQuat q1 q2 =
	(
		if ((q1.x * q2.x) + (q1.y * q2.y) + (q1.z * q2.z) + (q1.w * q2.w)) < 0 then
			q1 *= -1
		
		return q1
	)
	
	
	
	
	
	
	
	

	
	
	function updateOffsets range:undefined offset:false =
	(	
		for i = 1 to nodes.count do
		(
			--try
			(
				if useSelection == true then
					local updateNode = (selection as Array)[1]
				else
					local updateNode = getNodeByName nodes[i].node
				
				if updateNode != undefined then
				(
					nodes[i].rotOffset = (matrix3 1).rotation * inverse updateNode.transform.rotation
					nodes[i].defaultTM = updateNode.transform--(matrix3 1) * inverse updateNode.transform
					
					if useSelection == true then
						alignNodeDefaultTM = updateNode.transform
					
					if nodes[i].align != undefined then
					(
						local alignNode = getNodeByName nodes[i].align
						
						if alignNode != undefined then
							nodes[i].offsetTM = updateNode.transform * inverse alignNode.transform
					)
				)
			)--catch()
		)
		
		getValues()
	)
	
	
	
	
	
	
	function updateInfo =
	(
		local outputText = ""
		
		if nodes != undefined and nodes.count > 0 then
		(
			local hasDefaultTM = if (not isIdentity nodes[1].defaultTM) then "Yes" else "No"
			outputText += "Nodes (" + nodes.count as String + "):     Base Pose available :  " + hasDefaultTM
			
			if poses != undefined then
			(
				local useOffsetPoses = ""
				local hasOffsetPoses = if (nodes[1].poses[selPose.tn.index + 1] != undefined) then "Yes" else "No"
				
				if hasOffsetPoses == "Yes" then
					useOffsetPoses = if chkUseOffsetPoses.checked == true then "Used : Yes" else "Used : No"
						
				outputText += "\n\nPoses (" + poses.frames.count as String + "):     Offset Poses available : " + hasOffsetPoses + "     " + useOffsetPoses
			)
		)
		
		labInfo.text = outputText
	)
	
	
	
	
	
	
	
	
	
	
	function updateControls =
	(
		local enableBySelection = false
		local enableByPoses = false
		local enableByAlign = false
		local enableByBasePose = false
		local enableByOffsets = false
		local enableByPresets = false
		
		if nodes != undefined and nodes.count > 0 then
		(
			enableByPresets = true
			
			for i = 1 to nodes.count do
				if nodes[i].align != undefined then
					enableByAlign = true
				
			if not isIdentity nodes[1].defaultTM then
				enableByBasePose = true
			
			if poses != undefined then
				if nodes[1].poses[selPose.tn.index + 1] != undefined then
					enableByOffsets = true
		)
		
		if selPreset != undefined and selPreset.selection == true then
			enableBySelection = true
		
		
		if poses != undefined then
			enableByPoses = true
		
		
		
		spnFrameCount.enabled = enableByPoses
		spnSetYaw.enabled = enableByPoses
		spnSetPitch.enabled = enableByPoses
		
		btnUpdateBasePose.enabled = enableByPresets
		btnUpdateOffsetPoses.enabled = enableByPoses and enableByBasePose --and not enableBySelection
		btnRemoveOffsetPoses.enabled = enableByPoses and enableByOffsets and not enableBySelection
		btnCreatePoses.enabled = enableByPoses and enableByBasePose
		
		btnSelectionToAlign.enabled = enableBySelection
		btnSelectionToAlignZero.enabled = enableBySelection
		
		mainMenu.updateMenu()
	)
	
	
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	
	
	
	function updateList keepSelection:true =
	(
		lvNodes.SuspendLayout()
		
		if presets != undefined then
		(
			lvNodes.ColumnCount = 4
			lvNodes.RowCount = 0
			
			
			
			if nodes != undefined then
			(	
				lvNodes.RowCount = nodes.count
				
				
				for i = 1 to nodes.count do
				(
					((lvNodes.Rows.item (i-1)).Cells.item 0).value = nodes[i].node
					((lvNodes.Rows.item (i-1)).Cells.item 1).value = nodes[i].parent
					((lvNodes.Rows.item (i-1)).Cells.item 2).value = nodes[i].influence as String
					((lvNodes.Rows.item (i-1)).Cells.item 3).value = nodes[i].align

					local newBackStyle = dotNetObject "System.Windows.Forms.DataGridViewCellStyle"
					
					
					if nodes[i].influence >= 0 then
					(
						local counterValue = 255.0 - (150.0 * nodes[i].influence) / 100
						newBackStyle.backColor = newBackStyle.backColor.FromArgb 255 counterValue counterValue
					)
					else
					(
						local counterValue = 255.0 + (150.0 * nodes[i].influence) / 100
						newBackStyle.backColor = newBackStyle.backColor.FromArgb counterValue counterValue 255
					)
					
					
					((lvNodes.Rows.item (i-1)).cells.item 0).style = newBackStyle
				)
			)
		)
		else
		(
			spnPoseCount.value = 0
			spnInfluence.value = 0.0
			spnYaw.value = 0.0
			spnPitch.value = 0.0
			
			lvNodes.ColumnCount = 4
			lvNodes.RowCount = 0
			
			
			try
			(
				local resetStyle = dotNetObject "System.Windows.Forms.DataGridViewCellStyle"
				((lvNodes.Rows.item 0).cells.item 0).style = resetStyle
			)catch()
		)
		
		lvNodes.ResumeLayout()
	)
	
	
	
	
	
	
	
	function getPreset root:undefined parentRoot:undefined =
	(
		if root == undefined then
		(
			selPreset = undefined
			selPose = undefined
			selParentPreset = undefined
			
			root = presets
			
			for i = 1 to root.count do
				getPreset root:root[i] parentRoot:root
		)
		else
		(
			if root != undefined then
			(
				if root.tn == lvPresets.selectedNode then
				(
					selPreset = root
					selParentPreset = parentRoot
				)
				else
				(
					if root.poses != undefined then
					(
						for i = 1 to root.poses.count do
						(
							if root.poses[i].tn == lvPresets.selectedNode then
							(
								selPose = root.poses[i]
								selPreset = root
								selParentPreset = parentRoot
							)
						)
					)
				)
				
				for i = 1 to root.children.count do
					getPreset root:root.children[i] parentRoot:root.children
			)
		)
	)
	
	
	
	
	
	
	function updatePresetSelection useMouse:true forceUpdate:false =
	(
		local oldPreset = selPreset
		
		getPreset()
		
		local presetChange = false
		nodes = undefined
		poses = undefined
		
		
		useSelection = true
		
		if selPreset != undefined then
		(
			local requestUpdate = true
			
			useSelection = selPreset.selection
			
				--nodes = cryMaxTools.basic.misc.makeUnique selPreset.nodes
			nodes = selPreset.nodes
		
			if oldPreset == selPreset then
				requestUpdate = false

			
			if requestUpdate == true then
				updateList()
		)
		
		if selPose != undefined then
			--poses = cryMaxTools.basic.misc.makeUnique selPose
			poses = selPose
		
		getValues()
		updateControls()
		updatePreview()
		updateInfo()
	)
	
	
	
	
	
	
	
	
	function updatePresetList root:undefined rootTN:undefined =
	(
		local requestUpdate = false
		
		if root == undefined then
		(
			lvPresets.BeginUpdate()
			sortPresets start:true
		
		
			local tempSel = ""
			
			if lvPresets.selectedNode != undefined then
				tempSel = lvPresets.selectedNode.text
			
			
			lvPresets.nodes.clear()
			
			root = presets
			rootTN = lvPresets
			
			for i = 1 to root.count do
				updatePresetList root:root[i] rootTN:lvPresets
			
			requestUpdate = true
		)
		else
		(
			if root.name != undefined then
			(
				local tempNode = rootTN.nodes.add root.name
				
				
				if root.nodes == undefined then
				(
					tempNode.imageIndex = tempNode.selectedImageIndex = 1
				)
				else if root.selection == false then
				(
					tempNode.imageIndex = tempNode.selectedImageIndex = 0
					tempNode.backColor = tempNode.backColor.fromArgb 230 255 230
				)
				else
				(
					tempNode.imageIndex = tempNode.selectedImageIndex = 0
					tempNode.backColor = tempNode.backColor.fromArgb 255 220 220
				)
				 
				
				root.tn = tempNode
				
				
				if root.poses != undefined then
				(
					for d = 1 to root.poses.count do
					(
						if root.poses[d].name != undefined then
						(
							local tempPose = tempNode.nodes.add root.poses[d].name
							tempPose.tag = #("poses", d)
							tempPose.backColor = tempPose.backColor.fromArgb 240 240 255
							tempPose.imageIndex = tempPose.selectedImageIndex = 2
							
							if tempSel == tempPose.text then
								lvPresets.selectedNode = tempPose
							
							root.poses[d].tn = tempPose
						)
					)
				)
			
			
				for i = 1 to root.children.count do
					updatePresetList root:root.children[i] rootTN:tempNode
			)
		)
		
		
		if requestUpdate == true then
		(
			lvPresets.ExpandAll()
		
			if lvPresets.selectedNode == undefined and lvPresets.nodes.count > 0 then
				lvPresets.selectedNode = (lvPresets.nodes.item 0)
			
			lvPresets.EndUpdate()
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function reconstructPose poseIndex startFrame =
	(
		if poses != undefined then
		(
			for i = 1 to nodes.count do
			(
				local baseTM = nodes[i].defaultTM
				local localTM = undefined
				local tempTM = baseTM
				
				if nodes[i].parent != undefined then
				(
					for d = 1 to nodes.count do
						if nodes[i].parent == nodes[d].node then
							tempTM = (baseTM * inverse nodes[d].defaultTM) * nodes[d].defaultPoseTM
				)
				
				
				
				
				local rotTM = matrix3 1
					
				rotate rotTM (eulerangles poses.frames[poseIndex].pitch 0 0)
				rotate rotTM (eulerangles 0 0 poses.frames[poseIndex].yaw)
				
				
				local normalizedNewRot = normalizeQuat (baseTM * rotTM).rotation   baseTM.rotation
				local newRot = slerp baseTM.rotation normalizedNewRot (nodes[i].influence / 100.0)
				
				local newTM = matrix3 1
				
				newTM.rotation = newRot
				scale newTM baseTM.scale
				newTM.pos = tempTM.pos
				
				nodes[i].defaultPoseTM = newTM
				
				nodes[i].offsetPoseTM = matrix3 1
				
				local curNode = getNodeByName nodes[i].node
				
				if curNode != undefined then
				(
					at time ((startFrame + poseIndex - 1) as Time)
						if nodes[i].defaultPoseTM.rotation != curNode.transform.rotation or nodes[i].defaultPoseTM.pos != curNode.transform.pos then
							nodes[i].offsetPoseTM = curNode.transform * inverse nodes[i].defaultPoseTM
				)
			)
		)
	)
	
	
	
	
	
	function updatePoses =
	(
		if poses != undefined then
		(
			local selPose = dbc.selPose.tn.Index + 1
			local testTM2 = matrix3 1
			local tooTiny = true
			local testTM1 = matrix3 1
			local defaultTM = matrix3 1
			local t = animationRange.start.frame as Integer
		
			for f = 1 to poses.frames.count do
			(
				reconstructPose f t
			
				for i = 1 to nodes.count do
				(
					if nodes[i].node == "Selection" then
						local curNode = (getCurrentSelection())[1]
					else
						local curNode = getNodeByName nodes[i].node
					
					if curNode != undefined then
					(
						defaultTM = nodes[i].defaultTM
						
						tooTiny = true
						testTM1 = nodes[i].offsetPoseTM
						
						
						for h = 1 to 4 do
							if distance testTM1[h] testTM2[h] > 0.0001 then
								tooTiny = false
							
						if tooTiny == false then
						(
							if nodes[i].poses[selPose] == undefined then
								nodes[i].poses[selPose] = poseStruct ID:selPose
							
							nodes[i].poses[selPose].frames[f] = nodes[i].offsetPoseTM
						)
						else
							try nodes[i].poses[selPose].frames[f] = undefined catch()
					)
					else
						print ("Could not find Node:  " + nodes[i].node)
				)
			)
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function savePresets saveAs:undefined op:undefined root:undefined param:undefined index:undefined =
	(
		case op of
		(
			undefined:
			(
				if filePath == "" or saveAs == true then
					local newPath = getSaveFileName caption:"Save DirectionBlends Template" filename:filePath
				else
					local newPath = filePath
				
				
				if newPath != undefined then
				(
					xmlFile = dotNetObject "system.xml.xmlDocument"
					local fileRoot = xmlFile.createElement "DirectionalBlends"
					xmlFile.appendChild fileRoot
					
				
					for i = 2 to presets.count do
						savePresets saveAs:saveAs op:"Preset" root:fileRoot param:presets[i]
					
					cryMaxTools.basic.perforceMan.open newPath
			
					xmlFile.save newPath
					
					filePath = newPath
					
					savedPresets = cryMaxTools.basic.misc.makeUnique presets
				)
			)
			
			
			
			"Preset":
			(
				if param.name != undefined then
				(
					local child = xmlFile.createElement "Preset"
					
					child.setAttribute "Name" param.name
					
					if param.poses != undefined then
					(
						local childPoses = xmlFile.createElement "Poses"
						
						for i = 1 to param.poses.count do
							savePresets saveAs:saveAs op:"Pose" root:childPoses param:param.poses[i]
						
						child.appendChild childPoses
					)
					
					
					if param.nodes != undefined then
					(
						for i = 1 to param.nodes.count do
							savePresets saveAs:saveAs op:"Node" root:child param:param.nodes[i]
					)
					
					
					root.appendChild child
					
					for i = 1 to param.children.count do
						savePresets saveAs:saveAs op:"Preset" root:child param:param.children[i]
				)
			)
			
			"Pose":
			(
				local child = xmlFile.createElement op
				child.setAttribute "Name" param.name
				child.setAttribute "Frames" (param.frameCount as String)
				child.setAttribute "Yaw" (param.yaw as String)
				child.setAttribute "Pitch" (param.pitch as String)
				root.appendChild child
			)
			
			
			"Node":
			(
				local child = xmlFile.createElement op
				child.setAttribute "Name" param.node
				child.setAttribute "Influence" (param.influence as String)
				
				if param.parent != undefined then
					child.setAttribute "Parent" param.parent
				
				if param.align != undefined then
					child.setAttribute "Align" param.align
				
				for i = 1 to param.poses.count do
					if param.poses[i] != undefined then
						savePresets saveAs:saveAs op:"NodePose" root:child param:param.poses[i]
				
				root.appendChild child
			)
			
			"NodePose":
			(
				local child = xmlFile.createElement "Pose"
				child.setAttribute "ID" (param.ID as String)
				
				for i = 1 to param.frames.count do
					if param.frames[i] != undefined then
						savePresets saveAs:saveAs op:"NodeFrame" root:child param:param.frames[i] index:i
					
				root.appendChild child
			)
			
			"NodeFrame":
			(
				local child = xmlFile.createElement ("Frame" + index as String)
				child.setAttribute "TM" (param as String)
				
				root.appendChild child
			)
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	function loadPresets load:undefined op:undefined root:undefined parentID:0 indexID:0 rootArray:undefined =
	(
		case op of
		(
			undefined:
			(
				if load == undefined then
					local newPath = getOpenFileName caption:"Open Aim Pose Template" filename:filePath
				else
				(
					if (getFiles (sourcePath + "aimPose_presets.xml")).count == 0 then
						local newPath = getOpenFileName caption:"Open Aim Pose Template" filename:filePath
					else
						local newPath = filePath
				)
				
				
				
				
				if newPath != undefined then
				(
					local xmlFile = dotNetObject "system.xml.xmlDocument"
					
					xmlFile.load newPath
					
					local fileRoot = xmlFile.documentElement
					
					local selFrames = getPoseFrames 5   160.0   80.0
					local selPoses = #(posePresetStruct name:"Poses" yaw:160.0 pitch:80.0 frameCount:5 frames:selFrames)
					local selNodes = #(nodeStruct node:"Selection" influence:100.0)
					presets = #(presetStruct name:"--- Selection ---" nodes:selNodes poses:selPoses selection:true)
					
					
					for i = 0 to (fileRoot.childNodes.count - 1) do
						loadPresets load:load op:fileRoot.childNodes.itemOf[i].name root:fileRoot.childNodes.itemOf[i] indexID:(i+2) rootArray:presets
					
					
					savedPresets = cryMaxTools.basic.misc.makeUnique presets
					
					filePath = newPath
				)
				
			
				
			)
		
			
			"Preset":
			(
				local presetNodes = #()
				local presetName = root.attributes.itemOf[0].value
				local presetPoses = #()
				local childPresets = #()
				
				for i = 0 to (root.childNodes.count - 1) do
				(
					case root.childNodes.itemOf[i].name of
					(
						"Poses":		presetPoses = (loadPresets load:load op:root.childNodes.itemOf[i].name root:root.childNodes.itemOf[i])
						"Node":		append presetNodes (loadPresets load:load op:root.childNodes.itemOf[i].name root:root.childNodes.itemOf[i])
					)
				)
				
				if presetNodes.count == 0 then
					presetNodes = undefined
				if presetPoses.count == 0 then
					presetPoses = undefined
				
				
				local newEntry = (presetStruct name:presetName nodes:presetNodes poses:presetPoses)
				
				append rootArray newEntry
				
				for i = 0 to (root.childNodes.count - 1) do
				(
					case root.childNodes.itemOf[i].name of
					(						
						"Preset":		loadPresets load:load op:root.childNodes.itemOf[i].name root:root.childNodes.itemOf[i] rootArray:newEntry.children
					)
				)
			)
			
			"Poses":
			(
				local posePresets = #()
				
				for i = 0 to (root.childNodes.count - 1) do
					append posePresets (loadPresets load:load op:"Pose" root:root.childNodes.itemOf[i])
				
				return posePresets
			)
			
			"Pose":
			(
				local poseName = root.attributes.itemOf[0].value
				local poseFrameCount = root.attributes.itemOf[1].value as Integer
				local poseYaw = root.attributes.itemOf[2].value as Float
				local posePitch = root.attributes.itemOf[3].value as Float
				
				local poseFrameArray = getPoseFrames poseFrameCount poseYaw posePitch
				
				return (posePresetStruct name:poseName frames:poseFrameArray frameCount:poseFrameCount yaw:poseYaw pitch:posePitch)
			)
			
			"PoseFrame":
			(
				global tempBla = root
				return (poseFrameStruct yaw:(execute root.attributes.itemOf[0].value) pitch:(execute root.attributes.itemOf[1].value) )
			)
			
			"Node":
			(
				local nodePoseArray = #()
				local nodeName = ""
				local nodeInfluence = 100.0
				local nodeParent = undefined
				local nodeAlign = undefined
				local nodePoseArray = #()
				
				for i = 0 to (root.attributes.count - 1) do
				(
					case root.attributes.itemOf[i].name of
					(
						"Name":	nodeName= root.attributes.itemOf[i].value
						"Influence":	nodeInfluence = (root.attributes.itemOf[i].value as float)
						"Parent":	nodeParent = root.attributes.itemOf[i].value
						"Align":	nodeAlign = root.attributes.itemOf[i].value
					)
				)
				
				
				for i = 0 to (root.childNodes.count - 1) do
				(
					local nodePose = (loadPresets load:load op:"NodePose" root:root.childNodes.itemOf[i])
					
					if nodePose.ID > 0 then
						nodePoseArray[nodePose.ID] = nodePose
				)
				
				return (nodeStruct node:nodeName influence:nodeInfluence align:nodeAlign parent:nodeParent poses:nodePoseArray)
			)
			
			"NodePose":
			(
				local poseID = root.attributes.itemOf[0].value as Integer
				local poseFrameArray = #()
				
				for i = 0 to (root.childNodes.count - 1) do
				(
					local frame = (loadPresets load:load op:"NodeFrame" root:root.childNodes.itemOf[i])
					poseFrameArray[tempFrameIndex] = frame
				)
				
				return (poseStruct ID:poseID frames:poseFrameArray)
			)
			
			"NodeFrame":
			(
				tempFrameIndex = (substring root.name 6 root.name.count) as Integer
				return (execute root.attributes.itemOf[0].value)
			)
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function initVars =
	(
		sourcePath = ""
		local sourceFilter = filterString (getSourceFileName()) "\\"
		for i = 1 to (sourceFilter.count - 1) do
			sourcePath += sourceFilter[i] + "\\"
		
		filePath = sourcePath + "aimPose_presets.xml"
		
		
		--lvPresets.ShowLines = false
		--lvPresets.FullRowSelect = true
		lvPresets.HideSelection = false
		lvPresets.LabelEdit = true
		lvPresets.ShowPlusMinus = false
		lvPresets.ShowRootLines = false
		lvPresets.Indent = 12
		
		lvPresets.BorderStyle = lvPresets.BorderStyle.FixedSingle
		
		imageList = dotNetObject "System.Windows.Forms.ImageList"
		imageList.images.add ((dotNetClass "System.Drawing.Image").fromFile (sourcePath + "dbc_noNodes.png"))
		imageList.images.add ((dotNetClass "System.Drawing.Image").fromFile (sourcePath + "dbc_nodes.png"))
		imageList.images.add ((dotNetClass "System.Drawing.Image").fromFile (sourcePath + "dbc_poses.png"))
		
		lvPresets.imageList = imageList
		
		lvPresets.SelectedImageIndex = 2
		
		
		
		
		lvNodes.RowHeadersVisible = false
		lvNodes.ReadOnly = true
		
		lvNodes.BackgroundColor = lvNodes.BackgroundColor.DarkGray
		lvNodes.GridColor = lvNodes.GridColor.LightGray
		
		local ColumnHeadersDefaultCellStyle = lvNodes.ColumnHeadersDefaultCellStyle
		ColumnHeadersDefaultCellStyle.alignment = ColumnHeadersDefaultCellStyle.alignment.MiddleCenter
		lvNodes.ColumnHeadersDefaultCellStyle = ColumnHeadersDefaultCellStyle
		
		lvNodes.SelectionMode = lvNodes.SelectionMode.FullRowSelect
		
		local defaultCellStyle = lvNodes.DefaultCellStyle
		defaultCellStyle.SelectionBackColor = defaultCellStyle.SelectionBackColor.FromArgb 100 100 100
		defaultCellStyle.SelectionForeColor = defaultCellStyle.SelectionForeColor.FromArgb 200 200 200
		lvNodes.DefaultCellStyle = defaultCellStyle
		
		
		local RowTemplate = lvNodes.RowTemplate
		RowTemplate.resizable = RowTemplate.resizable.false
		RowTemplate.height = 20
		
		lvNodes.RowTemplate = RowTemplate
		lvNodes.ColumnHeadersHeightSizeMode = lvNodes.ColumnHeadersHeightSizeMode.DisableResizing
		lvNodes.ColumnHeadersHeight = 20
		
		lvNodes.AllowUserToAddRows = false
		
		
		lvNodes.ColumnCount = 4
		(lvNodes.Columns.item 0).name = "Node"
		(lvNodes.Columns.item 1).name = "Parent"
		(lvNodes.Columns.item 2).name = "Influence"
		(lvNodes.Columns.item 3).name = "Align"
		
		(lvNodes.Columns.item 0).frozen = true
		
		local influenceStyle = (lvNodes.Columns.item 2).DefaultCellStyle
		influenceStyle.Alignment = influenceStyle.Alignment.MiddleCenter
		(lvNodes.Columns.item 2).DefaultCellStyle = influenceStyle
		
		
		
		
		--loadSettings load:true
		
		
			
		loadPresets load:true
			
		updatePresetList forceUpdate:true
			
		mainMenu.updateMenu()
	)
	
	
	
	
	
	
	function setAlign =
	(
		for i = 1 to nodes.count do
		(
			if nodes[i].align != undefined then
			(
				local alignNode = getNodeByName nodes[i].align
				
				if useSelection == true then
					local curNode = (selection as Array)[1]
				else
					local curNode = getNodeByName nodes[i].node
				
				if (alignNode != undefined and curNode != undefined) or nodes[i].align == "Scene" then
				(
					local offsetTM = nodes[i].offsetTM
					local alignTM = nodes[i].defaultTM
					
					if nodes[i].align != "Scene" then
						alignTM = alignNode.transform
						
					if useSelection == true then
						offsetTM.pos.y = (curNode.transform * inverse alignTM).pos.y
					
					local newTM = offsetTM * alignTM
					
				
					
					
					with animate on
					(
						if curNode.classID[1] == 37157 then
						(
							biped.setTransform curNode #pos newTM.pos true
							biped.setTransform curNode #rotation newTM.rotation true
						)
						else
						(	
							local tempTM = matrix3 1
							tempTM.rotation = newTM.rotation
							tempTM.pos = newTM.pos
							scale tempTM curNode.transform.scale
							curNode.transform = tempTM
						)
					)
				)
			)
		)
	)
	
	
	
	
	
	function setPose poseIndex orgRot:undefined =
	(	
		if nodes != undefined and poses != undefined then
		(
			local selPoseIndex = selPose.tn.index + 1
			local t = animationRange.start.frame as Integer
			
			for i = 1 to nodes.count do
			(
				local rotTM = matrix3 1
				
				
				rotate rotTM (eulerangles poses.frames[poseIndex].pitch 0 0)
				rotate rotTM (eulerangles 0 0 poses.frames[poseIndex].yaw)
				
				
				
				if useSelection == false then
				(
					reconstructPose poseIndex t
					
					local curNode = getNodeByName nodes[i].node
					local defaultTM = nodes[i].defaultPoseTM
					
					if curNode != undefined then
					(
						local offsetTM = matrix3 1
						
						if chkUseOffsetPoses.checked == true and nodes[i].poses[selPoseIndex] != undefined and nodes[i].poses[selPoseIndex].frames[poseIndex] != undefined then
							offsetTM = nodes[i].poses[selPoseIndex].frames[poseIndex]
						
						local newTM = offsetTM * defaultTM
						
						with animate on
						(
							if curNode.classID[1] == 37157 then
							(
								biped.setTransform curNode #pos newTM.pos true
								biped.setTransform curNode #rotation newTM.rotation true
							)
							else
							(
								local tempTM = matrix3 1
								tempTM.rotation = newTM.rotation
								--// apply original scale back on
								scale tempTM defaultTM.scale
								tempTM.pos = newTM.pos
								
								curNode.transform = tempTM
							)
						)
					)
				)
				else
				(
					for obj in selection do
					(
						if orgRot == undefined then
							local tempRot = obj.transform.rotation
						else
							local tempRot = orgRot
						
						local normalizedNewRot = normalizeQuat (alignNodeDefaultTM * rotTM).rotation tempRot
						local newRot = slerp alignNodeDefaultTM.rotation normalizedNewRot (nodes[1].influence / 100.0)
						
						
						with animate on
						(
							if obj.classID[1] == 37157 then
								biped.setTransform obj #rotation newRot true
							else
							(
								local tempTM = matrix3 1
								tempTM.rotation = newRot
								tempTM.pos = obj.transform.pos
								scale tempTM obj.transform.scale
								obj.transform = tempTM
							)
						)
					)
				)
			)
			
			if chkUseAlign.checked == true then
				setAlign()
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	function createPose poseIndex nodeIndex:undefined all:undefined =
	(
		if poses != undefined then
		(
			local frames = poses.frames.count
			
			if nodeIndex == undefined then
			(
				undo "set pose" on
				(
					if all == undefined then
						setPose poseIndex
					else
					(
						if nodes.count > 0 then
						(
							animationRange = interval animationRange.start ((animationRange.start.frame as Integer) + frames - 1)
							for i = 1 to frames do
							(
								sliderTime = ((animationRange.start.frame as Integer) + i-1) as Time
								setPose i 
							)
						)
					)	
				)
			)
		)
	)

	
	
	
	
	on dbc open do
		initVars()
	
	
	on dbc close do
	(
		if cryMaxTools.basic.misc.compareStructures presets savedPresets skipFirstArrayEntries:1 == false then
			if queryBox "Save changes before closing?" == true then
				savePresets()
	)
	
	
	
	
	
	
	
	on lvNodes ColumnHeaderMouseClick value do
	(
		try
		(
			((lvNodes.Rows.item 0).cells.item 0).selected = true
			((lvNodes.Rows.item 0).cells.item 0).selected = false
		)catch()
		
		for i = 0 to (lvNodes.Columns.count - 1) do
		(
			for f = 0 to (lvNodes.Rows.count - 2) do
				((lvNodes.Rows.item f).cells.item i).selected = (if i == value.ColumnIndex then true else false)
		)
	)
	
	
	
	
	
	
	
	function setValueDialog mode header initText:"" =
	(
		setValueOP = mode
		setValueHeader = header
		setValueInit = initText
		
		rollout setValueDialogRO header
		(
			edittext edText "" pos:[2,5] height:18 fieldWidth:160
			spinner spnNumber pos:[2,5] height:18 fieldWidth:160 visible:false type:#integer
			spinner spnFloat pos:[2,5] height:18 fieldWidth:160 visible:false type:#float
			
			button btnOk "Ok" pos:[175,5] height:20 width:50
			button btnCancel "Cancel" pos:[230,5] height:20 width:50
			
			on setValueDialogRO open do
			(
				dbc.setValueVar = dbc.setValueInit
				--setValueDialogRO.caption = dbc.setValueHeader as String
				edText.text = dbc.setValueInit as String
				try spnNumber.value = dbc.setValueInit as Integer catch()
				try spnFloat.value = dbc.setValueInit as Float catch()
				
				case dbc.setValueOP of
				(
					#number:
					(
						edText.visible = false
						spnNumber.visible = true
					)
					
					#float:
					(
						edText.visible = false
						spnFloat.visible = true
					)
				)
			)
			
			
			on btnOk pressed do
			(
				local var = edText.text
				case dbc.setValueOP of
				(
					#number: var = spnNumber.value
					#float: var = spnFloat.value
				)
				
				dbc.setValueVar = var
				
				destroyDialog setValueDialogRO
			)
			
			
			on btnCancel pressed do
				destroyDialog setValueDialogRO
		)
		
		
		createDialog setValueDialogRO 285 28 modal:true
	)
	
	
	
	
	
	
	
	
	
	
	
	on lvPresets AfterSelect arg do
	(
		updatePresetSelection()
		
		mainMenu.updateMenu()
	)
	
	
	
	
	function deletePresetEntry =
	(
		local changed = false
					
		if selPose != undefined then
		(
			if queryBox "Delete Selected Poses?" == true then
			(
				selPose.name = undefined
				changed = true
			)
		)
		else if selPreset != undefined then
		(
			if queryBox "Delete Selected Preset?" == true then
			(
				selPreset.name = undefined
				changed = true
			)
		)
		
		if changed == true then
			updatePresetList()
	)
		
	
	
	
	
	
	
	function addPresetEntry newEntry:(presetStruct name:"NewPreset") =
	(
		local usedPreset = false
		
		if selPreset != undefined and selPreset.selection == false then
		(
			if queryBox "Add Preset to Selection?" == true then
				usedPreset = selPreset.children
			else
				usedPreset = selParentPreset
		)
		else
			usedPreset = selParentPreset
		
		if usedPreset != false then
		(
			if usedPreset == undefined then
				usedPreset = #(newEntry)
			else
				append usedPreset newEntry
			
			updatePresetList()
		)
	)
	
	
	
	function addPoseEntry newEntry:(posePresetStruct name:"NewPoses") =
	(
		if newEntry.frames.count == 0 then
		(
			local newFrames = #()
			for i = 1 to 9 do
				append newFrames (poseFrameStruct())
					
			newEntry.frames = newFrames
		)
		
		if selPreset.poses == undefined then
			selPreset.poses = #(newEntry)
		else
			append selPreset.poses newEntry
		
		updatePresetList()
	)
	
	
	
	function cutPresetEntry =
	(
		copyPreset = undefined
		copyPose = undefined
		
		if selPose != undefined then
		(
			copyPose = cryMaxTools.basic.misc.makeUnique selPose
			cutPart = selPose
		)
		else if selPreset != undefined then
		(
			copyPreset = cryMaxTools.basic.misc.makeUnique selPreset
			cutPart = selPreset
		)
		
		cutActive = true
		
		mainMenu.updateMenu()
	)
	
	
	
	function copyPresetEntry =
	(
		copyPreset = undefined
		copyPose = undefined
		
		if selPose != undefined then
			copyPose = cryMaxTools.basic.misc.makeUnique selPose
		else if selPreset != undefined then
			copyPreset = cryMaxTools.basic.misc.makeUnique selPreset
		
		cutActive = false
		
		mainMenu.updateMenu()
	)
	
	
	
	
	function pastePresetEntry =
	(
			--// Preset
		if copyPreset != undefined and selPreset != undefined then
		(
			local usedPreset = false
	
			if selPreset != undefined and selPreset.selection == false then
			(
				if queryBox "Paste Preset to Selection?" == true then
					usedPreset = selPreset.children
				else
					usedPreset = selParentPreset
			)
			else
				usedPreset = selParentPreset
			
			if usedPreset != false then
			(
				if usedPreset == undefined then
					usedPreset = #(copyPreset)
				else
					append usedPreset copyPreset
			)
		)
			--// Pose
		else if copyPose != undefined and selPreset != undefined then
		(
			if selPreset.poses != undefined then
				append selPreset.poses copyPose
			else
				selPreset.poses = #(copyPose)
		)
		
		if cutActive == true and cutPart != undefined then
			cutPart.name = undefined
		
		--sortPresets start:true
		updatePresetList()
		
		if cutActive == true then
			cutPart = undefined
	)
	
	
	
	
	
	
	on lvPresets keyUp arg do
	(
		if arg.keyCode == arg.keyCode.Delete then
			deletePresetEntry()
		
		if arg.keyCode == arg.keyCode.Insert then
			addPresetEntry()
		
		if arg.keyCode == arg.keyCode.F2 and lvPresets.SelectedNode != undefined then
			lvPresets.SelectedNode.BeginEdit()
	)
	
	
	
	on lvPresets AfterLabelEdit arg do
	(
		if selPose != undefined then
			selPose.name = arg.label
		else if selPreset != undefined then
		(
			if selPreset.selection == false then
				selPreset.name = arg.label
			else
				arg.CancelEdit = true
		)
	)
	
	on lvPresets MouseDoubleClick arg do
	(
		lvPresets.ExpandAll()
		
	)
	
	
	
	on lvPresets MouseUp arg do
	(
		mainMenu.updateMenu()
		
		if arg.button == arg.button.right then
		(
			rcmenu presetRC
			(
				menuItem miAddPreset "Add Preset"
				menuItem miAddPoses "Add Poses"
				separator miSep1
				menuItem miCut "Cut"
				menuItem miCopy "Copy"
				menuItem miPaste "Paste"
				separator miSep2
				menuItem miDelete "Delete"
				
				
				on presetRC open do
				(
					local presetVisible = selPreset != undefined and selPreset.selection == false
					
					miAddPoses.enabled = presetVisible
					miSep1.enabled = presetVisible
					miCut.enabled = presetVisible
					miCopy.enabled = presetVisible
					miPaste.enabled = copyPreset != undefined or copyPose != undefined
					miSep2.enabled = presetVisible
					miDelete.enabled = presetVisible
				)
				
				
				on miAddPreset picked do
					addPresetEntry()
				
				on miAddPoses picked do
					addPoseEntry()
				
				on miDelete picked do
					deletePresetEntry()
				
				on miCut picked do
					cutPresetEntry()
				
				on miCopy picked do
					copyPresetEntry()
				
				on miPaste picked do
					pastePresetEntry()
			)
			
			registerRightClickMenu presetRC

			popUpMenu presetRC pos:[(mouse.screenpos[1] - 2), (mouse.screenpos[2] - 2)]
			
		)
	)
	
	
	
	
	
	
	
	
	on lvNodes SelectionChanged arg do
	(
		getValues()
		
		if doDrag == -1 and doEdit == -1 then
		(
			toEditNodes = #()
		
			for i = 0 to (lvNodes.selectedCells.count - 1) do
				appendIfUnique toEditNodes ((lvNodes.selectedCells.item i).rowIndex)
				
			sort toEditNodes
		)
		
		mainMenu.updateMenu()
	)
	
	
	
	on lvNodes KeyDown arg do
	(
		if arg.Control == true or arg.Alt == true then
		(
			if doEdit == -1 and doDrag == -1 then
			(
				toEditNodes = #()
			
				for i = 0 to (lvNodes.selectedCells.count - 1) do
					appendIfUnique toEditNodes ((lvNodes.selectedCells.item i).rowIndex)
					
				sort toEditNodes
					
					
				if arg.Control == true then
				(
					doEditPos = lvNodes.pointToClient lvNodes.mousePosition
					local selNode = lvNodes.hitTest doEditPos.x doEditPos.y
					
					doEdit = selNode.rowIndex
					doDrag = -1	--// CANCEL
				)
			)
		)
	)
	
	
	
	on lvNodes keyUp arg do
	(
		if doEdit > -1 then
		(
				--// CANCEL EDIT
			for i = 1 to toEditNodes.count do
				((lvNodes.rows.item toEditNodes[i]).cells.item 2).value = (nodes[toEditNodes[i]+1].influence as Float) as String
				
			doEdit = -1
		)
		
		if doDrag > -1 then
			doDrag = -1
	)
	
	
	
	on lvNodes MouseDown arg do
	(	
		if arg.button == arg.button.middle then
		(
			doEditPos = lvNodes.pointToClient lvNodes.mousePosition
			local selNode = lvNodes.hitTest doEditPos.x doEditPos.y
				
			if doEdit == -1 then
				doDrag = selNode.rowIndex
		)	
			
		
		if arg.button == arg.button.right then
		(
			if doEdit > -1 then
			(
				for i = 1 to toEditNodes.count do
					((lvNodes.rows.item toEditNodes[i]).cells.item 2).value = (nodes[toEditNodes[i]+1].influence as Float) as String
					
				doEdit = -1
			)
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function sortEditNodesPlus n1 n2 =
	(
		if n1 < n2 then
			return 1
		else
			return -1
	)
	
	function sortEditNodesMinus n1 n2 =
	(
		if n1 > n2 then
			return 1
		else
			return -1
	)
	
	
	
	
	on lvNodes MouseMove arg do
	(
		if arg.button == arg.button.middle then
		(
			if doEdit > -1 or doDrag > -1 then
				doEditDrag = true
			
			
				--// EDIT STUFF
			if doEdit > -1 then
			(
				for i = 1 to toEditNodes.count do
				(
					local newValue = ((doEditPos.y - arg.y) / 4.0) as Integer
					
					
					local tempValue = nodes[toEditNodes[i]+1].influence + newValue
					
					if tempValue > 100.0 then
						tempValue = 100.0
					
					if tempValue < 0.0 then
						tempValue = 0.0
					
					((lvNodes.rows.item toEditNodes[i]).cells.item 2).value = tempValue as String
				)
			)
			
				--// DRAG STUFF
			if doDrag > -1 then
			(
				local mPos = lvNodes.pointToClient lvNodes.mousePosition
				local newRowIndex = (lvNodes.hitTest mPos.x mPos.y).rowIndex
				
				if doDrag != newRowIndex then
				(
					local offset = newRowIndex - doDrag
					
					if offset < 0 then
						qsort toEditNodes sortEditNodesMinus
					else
						qsort toEditNodes sortEditNodesPlus
					
					if toEditNodes[1] + offset >= 0 and toEditNodes[1] + offset < lvNodes.rowCount then
					(
						for i = 1 to toEditNodes.count do
						(
							for d = 0 to ((lvNodes.rows.item toEditNodes[i]).cells.count - 1) do
							(
								local tempValue = ((lvNodes.rows.item toEditNodes[i]).cells.item d).value
								((lvNodes.rows.item toEditNodes[i]).cells.item d).value = ((lvNodes.rows.item (toEditNodes[i] + offset)).cells.item d).value
								((lvNodes.rows.item (toEditNodes[i] + offset)).cells.item d).value = tempValue
							)
						)
						
						for i = 1 to toEditNodes.count do
						(	
							local tempValue = nodes[toEditNodes[i]+1]
							nodes[toEditNodes[i]+1] = nodes[toEditNodes[i] + 1 + offset]
							nodes[toEditNodes[i] + 1 + offset] = tempValue
							
							toEditNodes[i] += offset
						)
						
						for i = 0 to (lvNodes.rowCount - 1) do
						(
							if findItem toEditNodes i > 0 then
								(lvNodes.rows.item i).selected = true
							else
								(lvNodes.rows.item i).selected = false
						)
						
						doDrag = newRowIndex
					)
				)
			)
		)
	)
	
	
	
	function addNodeEntry =
	(
		for obj in selection do
		(
			local nodeFound = false
			
			if nodes != undefined then
			(
				for i = 1 to nodes.count do
				(
					if nodes[i].node == obj.name then
					(
						nodeFound = true
						exit
					)
				)
			)
			else
			(
				nodes = #()
				selPreset.nodes = nodes
			)
			
			if nodeFound == false then
			(
				local influence = 100.0
				
				if nodes.count > 0 then
					influence = nodes[nodes.count].influence
				
				append nodes (nodeStruct node:obj.name influence:influence)
			)
			else
				messageBox "Node already in the list"
		)
		
		updateList()
	)
	
	
	
	
	
	
	function replaceNodeEntry =
	(
		if lvNodes.selectedCells.count > 0 then
		(
			for obj in selection do
			(
				local nodeFound = false
				
				for i = 1 to nodes.count do
				(
					if nodes[i].node == obj.name then
					(
						nodeFound = true
						exit
					)
				)
				
				if nodeFound == false then
				(
					if (lvNodes.selectedCells.item 0).rowIndex + 1 > nodes.count then
						exit
					
					nodes[(lvNodes.selectedCells.item 0).rowIndex+1].node = obj.name
					exit
				)
			)
			
			updateList()
		)
	)
	
	
	
	
	
	function deleteNodeEntry =
	(
		if queryBox "Remove selected Entries?" == true then
		(
			local removeArray = #()
			local newArray = #()
			
			for i = 0 to (lvNodes.selectedCells.count - 1) do
				appendIfUnique removeArray (lvNodes.selectedCells.item i).rowIndex
			
			
			for i = nodes.count to 1 by -1 do
			(
				if findItem removeArray (i-1) != 0 then
					deleteItem nodes i
			)
			
			updateList()
		)
	)
	
	
	
	
	
	function setNodeInfluence =
	(
		setValueDialog #float "Node(s) Influence" initText:(((lvNodes.rows.item (lvNodes.selectedCells.item 0).rowIndex).cells.item 2).value as Float)
		
		for i = 1 to toEditNodes.count do
		(
			nodes[toEditNodes[i]+1].influence = setValueVar as Float
			((lvNodes.rows.item toEditNodes[i]).cells.item 2).value = setValueVar as String
		)
	)
	
	
	
	
	function setNodeParent =
	(
		toParentNodes = #()
						
		for i = 0 to (lvNodes.selectedCells.count - 1) do
		(
			local rowIndex = (lvNodes.selectedCells.item i).rowIndex
			if rowIndex < lvNodes.rowCount then
				append toParentNodes rowIndex
		)
	)
	
	
	
	
	
	
	
	function removeNodeParent =
	(
		toParentNodes = #()
						
		for i = 0 to (lvNodes.selectedCells.count - 1) do
		(
			local rowIndex = (lvNodes.selectedCells.item i).rowIndex
			if rowIndex < lvNodes.rowCount then
				append toParentNodes rowIndex
		)
		
		for i = 1 to toParentNodes.count do
		(
			nodes[toParentNodes[i] + 1].parent = undefined
			((lvNodes.rows.item toParentNodes[i]).cells.item 1).value = undefined
		)
	)
	
	
	
	
	function nodeAutoParent =
	(
		if queryBox "Override Parents in the list with automated parent detection?" == true then
		(
			for i = 1 to nodes.count do
			(
				local curNode = getNodeByName nodes[i].node
				
				if curNode != undefined then
				(
					local tempParent = curNode.parent
					local foundParent = false
					
					while tempParent != undefined do
					(
						for d = 1 to nodes.count do
							if tempParent.name == nodes[d].node then
								foundParent = true
							
						if foundParent == true then
							exit
						
						tempParent = tempParent.parent
					)
					
					
					if foundParent == true then
						nodes[i].parent = tempParent.name
					else
						nodes[i].parent = undefined
					
					((lvNodes.rows.item (i-1)).cells.item 1).value = nodes[i].parent
				)
			)
		)
	)
	
	
	
	
	function setNodeAlign =
	(
		if lvNodes.selectedRows.count > 0 then
		(
			local sel = getCurrentSelection()
			
			if sel.count > 0 then
			(
				for i = 0 to (lvNodes.selectedRows.count - 1) do
				(
					nodes[i+1].align = sel[1].name
					((lvNodes.selectedRows.item i).cells.item 3).value = sel[1].name
						
					if useSelection == true then
						alignNodeTo = sel[1].name
				)
			)
		)
	)
	
	
	
	
	
	function setSceneAlign =
	(
		if lvNodes.selectedRows.count > 0 then
		(
			for i = 0 to (lvNodes.selectedRows.count - 1) do
			(
				((lvNodes.selectedRows.item i).cells.item 3).value = "Scene"
				nodes[i+1].align = "Scene"
				
				if useSelection == true then
					alignNodeTo = "Scene"
			)
		)
	)
	
	
	
	
	
	
	function removeNodeAlign =
	(
		if lvNodes.selectedRows.count > 0 then
		(
			for i = 0 to (lvNodes.selectedRows.count - 1) do
			(
				((lvNodes.selectedRows.item i).cells.item 3).value = undefined
				nodes[i+1].align = undefined
				nodes[i+1].offsetTM = matrix3 1
				
				if useSelection == true then
					nodes[i+1].rotOffset = quat 1
			)
		)
	)
	
	
	
	
	
	
	on lvNodes MouseUp arg do
	(
		mainMenu.updateMenu()
		
		if doEdit > -1 then
		(
			for i = 1 to toEditNodes.count do
				nodes[toEditNodes[i]+1].influence = ((lvNodes.rows.item toEditNodes[i]).cells.item 2).value as Float
		)
		
		if doEditDrag == false then
		(
			if arg.button == arg.button.right then
			(
				rcmenu nodeRC
				(
					menuItem miAdd "Add"
					menuItem miReplace "Replace"
					menuItem miDelete "Delete"
					separator miSep1
					menuItem miSetInfluence "Set Influence"
					separator miSep2
					
					subMenu "Parent"
					(
						menuItem miSetParent "Node"
						separator miSep6
						menuItem miRemoveParent "Remove"
						separator miSep7
						menuItem miAutoParent "Auto"
					)
					separator miSep3
					
					subMenu "Align"
					(
						menuItem miSetAlign "Node"
						menuItem miSetAlignScene "Scene"
						separator miSep8
						menuItem miRemoveAlign "Remove"
					)
					
				
					on nodeRC open do
					(
						local nodeSelection = selPreset != undefined and selPreset.selection == false
						local hasNodes = dbc.lvNodes.selectedCells.count > 0
						local noSelection = hasNodes and nodeSelection
						
						miAdd.enabled = nodeSelection
						miReplace.enabled = noSelection
						miDelete.enabled = noSelection
						miSetInfluence.enabled = noSelection
						
						miSetParent.enabled = noSelection
						miRemoveParent.enabled = noSelection
						miAutoParent.enabled = noSelection
						
						miSetAlign.enabled = hasNodes
						miSetAlignScene.enabled = hasNodes
						miRemoveAlign.enabled = hasNodes
					)
					
					
					
					
					on miAdd picked do
						addNodeEntry()
					
					on miReplace picked do
						replaceNodeEntry()
					
					on miDelete picked do
						deleteNodeEntry()
					
					on miSetInfluence picked do
						setNodeInfluence()
					
					
					
					on miSetParent picked do
						setNodeParent()
					
					on miRemoveParent picked do
						removeNodeParent()
					
					on miAutoParent picked do
						nodeAutoParent()
					
					
					
					on miSetAlign picked do
						setNodeAlign()
					
					on miSetAlignScene picked do
						setSceneAlign()
					
					on miRemoveAlign picked do
						removeNodeAlign()
				)
				
				registerRightClickMenu nodeRC

				popUpMenu nodeRC pos:[(mouse.screenpos[1] - 2), (mouse.screenpos[2] - 2)]
				
			)
		)
		
		
		doEdit = -1
		doDrag = -1
		doEditDrag = false
	)
	
	
	
	
	
	
	on spnFrameCount changed arg do
	(
		if poses != undefined then
		(
			poses.frameCount = arg
			poses.frames = getPoseFrames poses.frameCount poses.yaw poses.pitch
			
			updatePreview()
		)
	)
	
	
	
	on spnSetYaw changed arg do
	(
		if poses != undefined then
		(
			poses.yaw = arg
			poses.frames = getPoseFrames poses.frameCount poses.yaw poses.pitch
		)
	)
	
	
	on spnSetPitch changed arg do
	(
		if poses != undefined then
		(
			poses.pitch = arg
			poses.frames = getPoseFrames poses.frameCount poses.yaw poses.pitch
		)
	)
	
	
	
	
	


	

	
	
	function updateOffsetPoses =
	(
		if queryBox "Override Offsets?" == true then
		(
			updatePoses()
			updateInfo()
			updateControls()
		)
	)
	
	
		
	on btnUpdateOffsetPoses pressed do
		updateOffsetPoses()
	
	
	
	function removeOffsetPoses =
	(
		if poses != undefined then
		(
			if queryBox "Remove Offsets?" == true then
			(
				for i = 1 to nodes.count do
					nodes[i].poses[selPose.tn.index + 1] = undefined
				
				updateControls()
				updateInfo()
			)
		)
	)
	
	
	on btnRemoveOffsetPoses pressed do
		removeOffsetPoses()
	
	
	
	
	function updateBasePose =
	(
		updateOffsets range:#all
		updateInfo()
		updateControls()
	)
	
	
	
	on btnUpdateBasePose pressed do
		updateBasePose()
	
	on chkUseOffsetPoses changed value do
		updateInfo()
	
		
	
	
	
	on btnPose01 pressed do		createPose 1
	on btnPose02 pressed do		createPose 2
	on btnPose03 pressed do		createPose 3
	on btnPose04 pressed do		createPose 4
	on btnPose05 pressed do		createPose 5
	on btnPose06 pressed do		createPose 6
	on btnPose07 pressed do		createPose 7
	on btnPose08 pressed do		createPose 8
	on btnPose09 pressed do		createPose 9
	on btnPose10 pressed do		createPose 10
	
	on btnPose11 pressed do		createPose 11
	on btnPose12 pressed do		createPose 12
	on btnPose13 pressed do		createPose 13
	on btnPose14 pressed do		createPose 14
	on btnPose15 pressed do		createPose 15
	on btnPose16 pressed do		createPose 16
	on btnPose17 pressed do		createPose 17
	on btnPose18 pressed do		createPose 18
	on btnPose19 pressed do		createPose 19
	on btnPose20 pressed do		createPose 20
	
	on btnPose21 pressed do		createPose 21
	on btnPose22 pressed do		createPose 22
	on btnPose23 pressed do		createPose 23
	on btnPose24 pressed do		createPose 24
	on btnPose25 pressed do		createPose 25
	on btnPose26 pressed do		createPose 26
	on btnPose27 pressed do		createPose 27
	on btnPose28 pressed do		createPose 28
	on btnPose29 pressed do		createPose 29
	on btnPose30 pressed do		createPose 30
		
	on btnPose31 pressed do		createPose 31
	on btnPose32 pressed do		createPose 32
	on btnPose33 pressed do		createPose 33
	on btnPose34 pressed do		createPose 34
	on btnPose35 pressed do		createPose 35
	on btnPose36 pressed do		createPose 36
	on btnPose37 pressed do		createPose 37
	on btnPose38 pressed do		createPose 38
	on btnPose39 pressed do		createPose 39
	on btnPose40 pressed do		createPose 40
		
	on btnPose41 pressed do		createPose 41
	on btnPose42 pressed do		createPose 42
	on btnPose43 pressed do		createPose 43
	on btnPose44 pressed do		createPose 44
	on btnPose45 pressed do		createPose 45
	on btnPose46 pressed do		createPose 46
	on btnPose47 pressed do		createPose 47
	on btnPose48 pressed do		createPose 48
	on btnPose49 pressed do		createPose 49
	on btnPose50 pressed do		createPose 50
		
	on btnPose51 pressed do		createPose 51
	on btnPose52 pressed do		createPose 52
	on btnPose53 pressed do		createPose 53
	on btnPose54 pressed do		createPose 54
	on btnPose55 pressed do		createPose 55
	on btnPose56 pressed do		createPose 56
	on btnPose57 pressed do		createPose 57
	on btnPose58 pressed do		createPose 58
	on btnPose59 pressed do		createPose 59
	on btnPose60 pressed do		createPose 60
	
	
	on btnCreatePoses pressed do
		createPose 1 all:true
		
	
	
	function selectionToAlign =
	(
		undo "selection to align" on
			setAlign()
	)
	
	
	on btnSelectionToAlign pressed do
		selectionToAlign()
	
	
	
	
	
	function selectionToAlignZero =
	(
		local newTM = matrix3 1
		
		if nodes[1].align != undefined then
			newTM = nodes[1].offsetTM--(matrix3 1) * nodes[1].posOffset
		
		undo "zero selection" on
		(
			with animate on
			(
				for obj in selection do
				(
					if obj.classID[1] == 37157 then
						biped.setTransform obj #rotation newTM.rotation true
					else
					(
						local tempTM = obj.transform
						tempTM.rotation = newTM.rotation
						tempTM.pos = obj.transform.pos
						obj.transform = tempTM
					)
				)
			)
		)
	)
	
	
	on btnSelectionToAlignZero pressed do
		selectionToAlignZero()
)



createDialog dbc 1000 310 style:#(#style_minimizebox, #style_sysmenu, #style_titlebar) menu:dbc.mainMenu