--------------------------------
-- CryMaxTools v2.0
-- Animation Exporter v1.0
-- by Sascha Herfort
--------------------------------

callbacks.removeScripts id:#getAnimationRanges

rollout rltCryExportAnimation "CryEngine2 Animation Export" width:190 height:457
(
	local rangeArray =#()
	local boneArray = #()
	
		--// sort start+
	local sortMode = 0
	
	local groupBoxArray = #()
	
	local rangeHeightChangeArray = #(#(),#())
	local rangePosChangeArray = #(#(),#())
	
	local boneHeightChangeArray = #(#(),#())
	local bonePosChangeArray = #(#(),#())
	
	
	local lastRangesSelected = #(0)
	local lastBonesSelected = #()
	local lastExportedPath = ""
	
	local gbBMP = bitmap 50 50 color:(color 0 0 180)--color:(color 1 29 111)
	
		
		--// groupBoxAppearance
	local groupBoxEdgeWidth = 1
	local groupBoxLabelExtent = 8
	local groupBoxRoundEdge = true
	local v1 = (279+32)
	local v2 = -90 + 16

		
	local rangeRO = undefined
	
		
	groupbox gbAnimationRanges " Ranges " pos:[3,100-5+v2-16] width:170 height:(289+32+3)
	label labAnimationRanges "" pos:[-5,-5]
	imgTag itAnimationRanges1 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationRanges2 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationRanges3 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationRanges4 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationRanges5 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	
	button btnAnimationGetTimeTags "Get TimeTags" pos:[8,20+3] width:160 height:16
	button btnExportRange "ExportRanges" pos:[8,20+16+3] width:80 height:16
	button btnImportRange "ImportRanges" pos:[80+8,20+16+3] width:80 height:16
	radioButtons radAnimationTimeTagPreSuffix "" pos:[15,21+32+3] labels:#(" by prefix    "," by suffix") default:1 columns:2
	
	
	
	button btnAnimationAddRange "Add" pos:[8,110+v2+17+16+3] width:43 height:16 enabled:true
	button btnAnimationRemoveRange "Remove" pos:[51,110+v2+17+16+3] width:49 height:16 enabled:false
	button btnAnimationClearRanges "Clear List" pos:[101,110+v2+17+16+3] width:55 height:16 enabled:false
	checkbutton btnOpenAnimationRangeList ">" pos:[101+56,110+v2+17+16+3] width:11 height:16
	
	dotNetControl lvRanges "System.Windows.Forms.ListView" pos:[8,126+v2+17+16+3] width:160 height:215
	
	spinner spnAnimationSubRangeStart "From:" pos:[9,345+v2+17+16+3] width:70 height:16 range:[-9999,9999,1] type:#integer align:#left
	spinner spnAnimationSubRangeEnd "To:" pos:[98,345+v2+17+16+3] width:70 height:16 range:[-9999,9999,100] type:#integer	
	
		
	groupbox gbAnimationBones " Bones " pos:[3,5+v1+3] width:170 height:90
	label labAnimationBones "" pos:[-5,-5]
	imgTag itAnimationBones1 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationBones2 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationBones3 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationBones4 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationBones5 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
		
	dotNetControl lvBones "System.Windows.Forms.ListView" pos:[8,20+16+v1+3] width:160 height:50
	
	button btnAnimationSetBone "Set Bones" pos:[8,20+v1+3] width:(79) height:16 enabled:true
	button btnAnimationRemoveBone "Remove Bones" pos:[(54+22+12),20+v1+3] width:(80) height:16 enabled:false
		
	
	
	
	dropdownlist ddAnimationExportType "" pos:[4,485-85+10+3] width:170 height:4 items:#("Character Animation  (*.caf)", "Geometry Animation  (*.anm)", "Custom Format")
	edittext edAnimationExportType "" pos:[123, 485-85+10+5] width:50 height:17 visible:false
	
	groupbox gbAnimationExportSelected " Export Selected " pos:[3,425+20+3] width:170 height:48
	label labAnimationExportSelected "" pos:[-5,-5]
	imgTag itAnimationExportSelected1 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportSelected2 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportSelected3 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportSelected4 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportSelected5 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	
	button btnAnimationExportSelected "E X P O R T    S E L E C T E D" pos:[8,526-83+20+3] width:160 height:24
	
	
	
	
	groupbox gbAnimationExportAll " Export All Ranges " pos:[3,425+30+20+8+3] width:170 height:88
	label labAnimationExportAll "" pos:[-5,-5]
	imgTag itAnimationExportAll1 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportAll2 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportAll3 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportAll4 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	imgTag itAnimationExportAll5 "" pos:[-5,-5] width:2 height:2 bitmap:gbBMP style:#gbBMPp_tile
	
	
	label labAnimationExportPath "          No Export Path Set" pos:[8,445+25+20+8+3] width:160 visible:true
	imgTag itAnimationExportPath pos:[6,440+25+20+8+3] width:164 height:22 toolTip:"No Export Path Set"
	button btnAnimationSetExportPath "Set Export Path" pos:[8,463+25+20+8+3] width:95 height:16
	button btnAnimationExplore "Explore" pos:[109,463+25+20+8+3] width:58 height:16 enabled:false
	button btnAnimationExportAll "E X P O R T   R A N G E S" pos:[8,483+25+20+8+3] width:160 height:24 enabled:false
	
	
	
	
	
	
	
	
	function updateRangeUI =
	(
		if rangeRO != undefined then
		(
			local newPos = lvRanges.PointToScreen (dotNetObject "System.Drawing.Point" -2 -2)
			setDialogPos rangeRO [newPos.x, newPos.y]
		)
	)
	
	
	
	
	
	
	
	
	function callRangeRO =
	(
		rollout rangeRO ""
		(
			dotNetControl lvBigRanges "System.Windows.Forms.ListView" pos:[0,0] width:300 height:200
			
			
			
			on rangeRO open do
			(
				lvBigRanges.GridLines = true
				lvBigRanges.AllowColumnReorder = false
				lvBigRanges.View = lvBigRanges.View.Details
				lvBigRanges.LabelEdit = true
				lvBigRanges.LabelWrap = true
				lvBigRanges.MultiSelect = true
				lvBigRanges.FullRowSelect = true
				lvBigRanges.HideSelection = false
								
				lvBigRanges.columns.add "Name"
				lvBigRanges.columns.add "<"
				lvBigRanges.columns.add ">"
				lvBigRanges.columns.add "Bones"
				lvBigRanges.columns.add "Type"
				

				lvBigRanges.ShowItemToolTips = true
				(lvBigRanges.columns.item 0).displayIndex = 2
				
				rltCryExportAnimation.updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
			)
			
			on rangeRO resized arg do
			(
				lvBigRanges.width = arg.x
				lvBigRanges.height = arg.y
			)
			
			
			
			on lvBigRanges AfterLabelEdit arg do
			(
				if arg.label != undefined then
				(
					if rltCryExportAnimation.lastRangesSelected[1] > 0 then
						rltCryExportAnimation.rangeArray[rltCryExportAnimation.lastRangesSelected[1] + 1].animName = arg.label
					else
						(lvBigRanges.items.item rltCryExportAnimation.lastRangesSelected[1]).text = "*current range"
					
					rltCryExportAnimation.updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
					rltCryExportAnimation.updateRanges keepSelection:true
				)
			)
			
			
			
			
			on lvBigRanges MouseUp arg do
			(
				if rltCryExportAnimation.rangeArray.count > 0 then
				(
					if lvBigRanges.selectedItems.count == 0 then
					(
						for i = 1 to rltCryExportAnimation.lastRangesSelected.count do
							(lvBigRanges.items.item rltCryExportAnimation.lastRangesSelected[i]).selected = true
					)
					else
					(
						rltCryExportAnimation.lastRangesSelected = #()
						for i = 0 to (lvBigRanges.selectedItems.count - 1) do
							append rltCryExportAnimation.lastRangesSelected (lvBigRanges.selectedItems.item i).index
						
						if rltCryExportAnimation.lastRangesSelected.count > 1 and findItem rltCryExportAnimation.lastRangesSelected 0 != 0 then
						(
							deleteItem rltCryExportAnimation.lastRangesSelected (findItem rltCryExportAnimation.lastRangesSelected 0)
							(lvBigRanges.items.item 0).selected = false
						)
					)
				)
				
				for i = 0 to (lvBigRanges.items.count - 1) do
					(rltCryExportAnimation.lvRanges.items.item i).selected = (lvBigRanges.items.item i).selected
				
				rltCryExportAnimation.updateRangeValues overwriteList:lvBigRanges
				rltCryExportAnimation.updateAmountRanges onlyBones:true
				rltCryExportAnimation.updateType overwriteList:lvBigRanges
			)
			
			
			
			on lvBigRanges ColumnClick arg do
			(
				local tempMode = sortMode
				if tempMode < 0 then
					tempMode *= -1
				
				if (tempMode - 1) == arg.Column then
					sortMode *= -1
				else
					sortMode = arg.Column + 1
				
				rltCryExportAnimation.updateRanges overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
				rltCryExportAnimation.updateRanges()
			)
			
			
			
			on lvBigRanges DoubleClick arg do
			(
				if rltCryExportAnimation.lastRangesSelected[1] > 0 then
					try ( animationRange = interval rltCryExportAnimation.rangeArray[(rltCryExportAnimation.lastRangesSelected[1] + 1)].start rltCryExportAnimation.rangeArray[(rltCryExportAnimation.lastRangesSelected[1] + 1)].end )
					catch( try animationRange = interval rltCryExportAnimation.rangeArray[(rltCryExportAnimation.lastRangesSelected[1] + 1)].start (rltCryExportAnimation.rangeArray[(rltCryExportAnimation.lastRangesSelected[1] + 1)].end + 1) catch())
			)

			
			on lvBigRanges Resize  do
			(
			)
			
			on lvBigRanges ItemMouseHover arg do
			(
				if arg.item != undefined then
					rltCryExportAnimation.updateToolTip arg.item.index
			)
		)
		
		local newPos = lvRanges.PointToScreen (dotNetObject "System.Drawing.Point" -2 -2)
		
		createDialog rangeRO 300 200 newPos.x newPos.y style:(#(#style_resizing, #style_toolwindow))
	)
	
	
	
	
	
	
	
	
	
	function updateBones =
	(
		lvBones.items.clear()
		
		local boneArray = #()
		
		for i = 1 to lastRangesSelected.count do
		(
			local index = lastRangesSelected[i] + 1
			
			for d = 1 to rangeArray[index].rootNodes.count do
			(
				if findItem boneArray rangeArray[index].rootNodes[d] == 0 then
					append boneArray rangeArray[index].rootNodes[d]
			)
		)
		
		if boneArray.count > 0 then
		(
			for i = 1 to boneArray.count do
				lvBones.items.add boneArray[i] boneArray[i]
			
			lvBones.enabled = true
		)
		else
		(
			lvBones.items.add "  Can't Export without Bones"
			lvBones.enabled = false
		)
		
		if lvBones.selectedItems.count > 0 then
			btnAnimationRemoveBone.enabled = true
	)
	
	
	
	
		--// TODO
	function getValidBones objArray =
	(
		
		return objArray
	)
	
	
	
	
	
	
	
	
	
	
	
	function updateRangeValues overwriteList:undefined =
	(
		local foundBones = false
		local useList = lvRanges
		
		if overwriteList != undefined then
			useList = overwriteList
		
		if useList.selectedItems.count > 0 then
		(
			
			btnAnimationRemoveRange.enabled = true
			
			for i = 0 to (useList.selectedItems.count - 1) do
			(
				local index = (useList.selectedItems.item i).index + 1
				
				if rangeArray[index].start == "start" then
				(
					spnAnimationSubRangeStart.value = animationRange.start.frame as Integer
					spnAnimationSubRangeEnd.value = animationRange.end.frame as Integer
					
					btnAnimationRemoveRange.enabled = false
				)
				else
				(
					spnAnimationSubRangeStart.value = rangeArray[index].start
					spnAnimationSubRangeEnd.value = rangeArray[index].end
				)
				
				if rangeArray[index].rootNodes.count > 0 then
					foundBones = true
			)
		)
		else
		(
			spnAnimationSubRangeStart.value = animationRange.start.frame as Integer
			spnAnimationSubRangeEnd.value = animationRange.end.frame as Integer
		)
		
		btnAnimationExportSelected.enabled = foundBones
			
		
		updateBones overwriteList:overwriteList
		cryMaxTools.export.anim.fnSetRanges rangeArray
	)
	
	
	
	
	function validateRange index =
	(
		if rangeArray[index + 1] != undefined then
		(
			if rangeArray[index + 1].rootNodes.count > 0 then
				return true
		)
		
		return false
	)
	
	
	
	
	
	
	
	function updateToolTip index overwriteList:undefined = 
	(
		local useList = lvRanges
		if overwriteList != undefined then
			useList = overwriteList
		
		if rangeArray[index+1] != undefined then
		(
			local rootNodesSeperated = ""
			for i = 1 to rangeArray[index+1].rootNodes.count do
				rootNodesSeperated += (if i == 1 then "\n" else "") + "               " + rangeArray[index+1].rootNodes[i] + (if i < rangeArray[index+1].rootNodes.count then "\n" else "")
				
			local exportTypeStr1 = ""
			local exportTypeStr2 = ""
				
			case ddAnimationExportType.selection of
			(
				1:( exportTypeStr1 = "Caf "; exportTypeStr2 = ".caf" )
				2:( exportTypeStr1 = "Anm "; exportTypeStr2 = ".anm" )
			)
			
			local currentHover = false
			if rangeArray[index + 1].start == "start" then
				currentHover = true
			
			if currentHover == true and lvRanges.selectedItems.count == 1 and lastRangesSelected[1] == 0 then
			(
				spnAnimationSubRangeStart.value = animationRange.start.frame
				spnAnimationSubRangeEnd.value = animationRange.end.frame
			)
			
			local validRange = validateRange index
				
			(lvRanges.items.item index).toolTipText =	"Filename:   " + (if currentHover == true then "New " + exportTypeStr1 + "File" else rangeArray[index+1].animName + exportTypeStr2) + \
																		"\n-----\nRange:       " + (if currentHover == true then ((animationRange.start.frame as Integer) as String) else rangeArray[index+1].start as String) + \
																		"   -->   " + (if currentHover == true then ((animationRange.end.frame as Integer) as String) else rangeArray[index+1].end as String) + \
																		"\n-----\nValid ?      "  + (if validRange == true then "Yes" else "No  (no bones)") + "\n-----\nBones:" + rootNodesSeperated
			
			
		)
	)
	
	
	
	
	function compareNodes node1 node2 dir:0 =
	(
		local result = 0
		
		
		
		if dir == 1 or dir == -1 then
		(
			try
				local node1Filter = filterString node1.animName "\\"
			catch
				local node1Filter = filterString node1 "\\"
			node1 = node1Filter[node1Filter.count]
			try
				local node2Filter = filterString node2.animName "\\"
			catch
				local node2Filter = filterString node2 "\\"
			node2 = node2Filter[node2Filter.count]
			
			local nodeLength = node1.count
			if node2.count > node1.count then
				nodeLength = node2.count
			
			
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
		)
		
		
		
		if dir == 2 or dir == -2 or dir == 3 or dir == -3 then
		(
			local val1 = 0
			local val2 = 0
			
			if dir == 2 or dir == -2 then
			(
				val1 = node1.start
				val2 = node2.start
			)
			else
			(
				val1 = node1.end
				val2 = node2.end
			)
			
			try
			(
				if val1 < val2 then
					result = -1
				else
					result = 1
			)
			catch
			(
				if classOf val1 != Integer then
					result = -1
				else
					result = 1
			)
		)
			
			
		
		
		
		if dir < 0 then
			result *= -1
		
		return result
	)
	
	
	

	
	
	
	
	
	
	function updateAmountRanges onlyBones:undefined =
	(
		local rangeLines = 2 + rangeArray.count
		local boneLines = 1
		if lvBones.items.count >= 2 then
			boneLines = lvBones.items.count
		
		
		
		if rangeLines > 14 then
			rangeLines = 14
		
		if onlyBones == undefined then
		(
			for i = 1 to rangeHeightChangeArray[1].count do
				rangeHeightChangeArray[1][i].height = rangeHeightChangeArray[2][i] - (14 * (14 - rangeLines))
			
			for i = 1 to rangePosChangeArray[1].count do
				rangePosChangeArray[1][i].pos = rangePosChangeArray[2][i] - [0,(14 * (14 - rangeLines))]
		)
			
		
		for i = 1 to (boneHeightChangeArray[1].count - 1) do
			boneHeightChangeArray[1][i].height = boneHeightChangeArray[2][i] -  (14 * (3 - boneLines))
		
		
		for i = 1 to bonePosChangeArray[1].count do
			bonePosChangeArray[1][i].pos = bonePosChangeArray[2][i] - [0,((14 * (14 - rangeLines))  + (14 * (3 - boneLines)))]
		
		boneHeightChangeArray[1][3].height = boneHeightChangeArray[2][3] - ((14 * (14 - rangeLines)) + (14 * (3 - boneLines)))
		
		
		
		local defaultRE = 0
		if groupBoxRoundEdge == true then
			defaultRE = 1
		
		cryMaxTools.basic.misc.groupBoxConverter groupBoxArray[1] roundEdge:true labelOffset:8
		cryMaxTools.basic.misc.groupBoxConverter groupBoxArray[2] flipHorizontal:true re:(#(defaultRE,defaultRE,0,0)) roundEdge:true labelOffset:8
		cryMaxTools.basic.misc.groupBoxConverter groupBoxArray[3] roundEdge:true labelOffset:8
		cryMaxTools.basic.misc.groupBoxConverter groupBoxArray[4] flipHorizontal:true re:(#(defaultRE,defaultRE,0,0)) roundEdge:true labelOffset:8
		
		lvBones.Refresh()
	)
	
	
	
	function updateType onlyUI:false overwriteList:undefined =
	(
		local useList = lvRanges
		if overwriteList != undefined then
			useList = overwriteList
		
		if onlyUI == false then
		(
			if useList.selectedItems.count > 0 then
			(
				local index = (useList.selectedItems.item 0).index + 1
				if rangeArray[index] != undefined then
				(
					case rangeArray[index].type of
					(
						"caf":		ddAnimationExportType.selection = 1
						"anm":	ddAnimationExportType.selection = 2
						default:	ddAnimationExportType.selection = 3
					)
					
					edAnimationExportType.text = rangeArray[index].type
				)
			)
			else
				ddAnimationExportType.selection = 1
		)
		
		
		
		if ddAnimationExportType.selection > 2 then
		(
			ddAnimationExportType.width = 120
			edAnimationExportType.visible = true
		)
		else
		(
			ddAnimationExportType.width = 170
			edAnimationExportType.visible = false
		)
	)
	
	
	
	
	function updateRanges keepSelection:undefined noSort:false overwriteList:undefined overwriteBones:false overwriteType:false =
	(
		local maxExtents = #(0,0,0)
		local foundBones = false
		
		local useList = lvRanges
		local secList = undefined
		
		if overwriteList != undefined then
			useList = overwriteList
		
		useList.items.clear()
		
		
		
		if noSort == false then
		(
			local startSort = 1
			if rangeArray.count > 1 then
				startSort = 2
			
			qsort rangeArray compareNodes start:startSort dir:sortMode
		)
		
		
		for i = 1 to rangeArray.count do
		(
			local animNameExtent = (getTextExtent rangeArray[i].animName)[1]
			local startExtent = (getTextExtent (rangeArray[i].start as String))[1]
			local endExtent = (getTextExtent (rangeArray[i].end as String))[1]
			
				
			if animNameExtent > maxExtents[1] then maxExtents[1] = animNameExtent
			if startExtent > maxExtents[2] then maxExtents[2] = startExtent
			if endExtent > maxExtents[3] then maxExtents[3] = endExtent
			
			
			local tempEntry = useList.items.add rangeArray[i].animName
			tempEntry.subItems.add (rangeArray[i].start as String)
			tempEntry.subItems.add (rangeArray[i].end as String)
			tempEntry.subItems.add ""
			tempEntry.subItems.add ""
			
			
			if overwriteBones == true then
			(
				local tempRootNodes = ""
				
				for d = 1 to rangeArray[i].rootNodes.count do
					tempRootNodes += rangeArray[i].rootNodes[d] + (if d < rangeArray[i].rootNodes.count then " ; " else "")
				
				(tempEntry.subItems.item 3).text = tempRootNodes
				
				if maxExtents[4] == undefined then
					maxExtents[4] = (getTextExtent "Bones")[1]
				
				local boneExtent = (getTextExtent (tempRootNodes))[1]
				if boneExtent > maxExtents[4] then maxExtents[4] = boneExtent
			)
			
			
			if overwriteType == true then
			(
				(tempEntry.subItems.item 4).text = rangeArray[i].type
				
				if maxExtents[5] == undefined then
					maxExtents[5] = (getTextExtent "Type")[1]
				
				local typeExtent = (getTextExtent (rangeArray[i].type))[1]
				if typeExtent > maxExtents[5] then maxExtents[5] = typeExtent
			)
			
			
				--// no bones
			if rangeArray[i].rootNodes.count == 0 then
				tempEntry.foreColor = tempEntry.foreColor.fromARGB 200 0 0
			else
				tempEntry.foreColor = tempEntry.foreColor.fromARGB 0 0 0
			
			
			
			if i > 1 and rangeArray[i].rootNodes.count > 0 then
				foundBones = true
		)
		
		local allExtents = 0
		for i = 1 to maxExtents.count do
		(
			maxExtents[i] += (if maxExtents[i] > 0 then 13 else 0)
			allExtents += maxExtents[i]
		)
		
		
		if allExtents < lvRanges.width - 16 then
			maxExtents[1] = lvRanges.width - maxExtents[2] - maxExtents[3] - 4 - (if rangeArray.count > 13 then 14 else 0)
	
		
		
		for i = 1 to maxExtents.count do
			if maxExtents[i] != undefined then
				(useList.columns.item (i-1)).width = maxExtents[i]
		
		
				
		if keepSelection == true then
		(
			for i = 1 to lastRangesSelected.count do
				(useList.items.item lastRangesSelected[i]).selected = true
		)
		
		
		
		
		
		btnAnimationClearRanges.enabled = (rangeArray.count > 1)
		btnAnimationRemoveRange.enabled = (findItem lastRangesSelected 0 == 0)
		btnAnimationExportAll.enabled = (rangeArray.count > 1 and foundBones == true)
		
		
		
		updateRangeValues overwriteList:overwriteList
		updateAmountRanges overwriteList:overwriteList
		
		useList.Refresh()
		
		updateType overwriteList:overwriteList
	)
	
	
	
	
	
	
	
	
	
	
	function updateFromScene =
	(
		try
		(
			lastRangesSelected = #(0)
			
			cryMaxTools.export.anim.fnGetRanges()
			
			if cryMaxTools.export.anim.animRanges.count == 0 then
				rangeArray = #((rangeStruct animName:"*current range" start:"start" end:"end" rootNodes:#()))
			else
				rangeArray = #()
		
			join rangeArray cryMaxTools.export.anim.animRanges
			
			updateRanges keepSelection:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		)
		catch (print "Error in Getting Ranges! Probably corrupt CryAnimationExportNode!")
		
		try
		(
			cryMaxTools.export.anim.fnGetExportSettings()
			
			ddAnimationExportType.selection = cryMaxTools.export.anim.exportType
			
			
			if cryMaxTools.export.anim.sAnimExportPath != "" then
			(
				labAnimationExportPath.text = cryMaxTools.basic.str.cutStringToWidth cryMaxTools.export.anim.sAnimExportPath 160 align:#right
				itAnimationExportPath.toolTip = cryMaxTools.export.anim.sAnimExportPath
				btnAnimationExplore.enabled = true
			)
			else
			(
				labAnimationExportPath.text = "          No Export Path Set"
				itAnimationExportPath.toolTip = "No Export Path Set"
				btnAnimationExplore.enabled = false
			)
		)
		catch (print "Error in Getting Settings! Probably corrupt CryAnimationExportNode!")
	

		
		
	)
	
	
	
	
	
	
	function setRangeBones =
	(
		local newBones = #()
		
		if selection.count > 0 then
			newBones = getCurrentSelection()
		else
			newBones = selectByName title:"Select Bones to Add" showHidden:true
		
		if newBones != undefined then
		(
			for i = 1 to lastRangesSelected.count do
			(
				rangeArray[lastRangesSelected[i] + 1].rootNodes = #()
				
				for d = 1 to newBones.count do
					append rangeArray[lastRangesSelected[i] + 1].rootNodes newBones[d].name
			)
			
			updateRanges keepSelection:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		)
	)
	
	
	
	
	function removeBone =
	(
		if lvBones.selectedItems.count > 0 then
		(
			for f = 0 to (lvBones.selectedItems.count - 1) do
			(
				for i = 1 to lastRangesSelected.count do
				(
					local index = lastRangesSelected[i] + 1
					
					for d = 1 to rangeArray[index].rootNodes.count do
					(
						if rangeArray[index].rootNodes[d] == (lvBones.selectedItems.item f).text then
						(
							deleteItem rangeArray[index].rootNodes d
							exit
						)
					)
				)
			)
			
			updateBones()
			updateRanges keepSelection:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
			
			if lvBones.selectedItems.count == 0 then
				btnAnimationRemoveBone.enabled = false
		)
	)
	
	
	
	
	
	function clearBoneList =
	(
		
		for i = 1 to lastRangesSelected.count do
			rangeArray[(lastRangesSelected[i] + 1)].rootNodes = #()
		
		
		updateBones()
		updateRanges keepSelection:true
		
		if rangeRO != undefined then
			if rangeRO.open == true then
				updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		
		btnAnimationRemoveBone.enabled = false
	)
	
	
	
	
	
	
	
	
	
	function addRange custom:undefined noUpdate:false =
	(
		if custom == undefined then
		(
			local lastNewAnim = 1
			for i = 1 to rangeArray.count do
			(
				local animFilter = filterString rangeArray[i].animName "_"
				if animFilter[1] == "newAnim" then
				(
					local tempIndex = execute animFilter[2]
					if tempIndex > lastNewAnim - 1 then
						lastNewAnim = tempIndex + 1
				)
			)
			
			local newAnimString = "newAnim_" + (if lastNewAnim < 10 then "0" else "") + lastNewAnim as String
			local type = edAnimationExportType.text as Name
			
			case ddAnimationExportType.selection of
			(
				1: type = "caf"
				2: type = "anm"
			)
			
			append rangeArray (rangeStruct animName:newAnimString start:(animationRange.start.frame as Integer) end:(animationRange.end.frame as Integer) rootNodes:#() type:type)
		)
		else
			append rangeArray custom
		
		lastRangesSelected = #(rangeArray.count - 1)
		
		if noUpdate == false then
		(
			updateRanges keepSelection:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		)
	)
	
	
	
	function removeRange =
	(
		for i = (lvRanges.selectedItems.count - 1) to 0 by -1 do
			if rangeArray[((lvRanges.selectedItems.item i).index + 1)].start != "start" then
				deleteItem rangeArray ((lvRanges.selectedItems.item i).index + 1)
		
		
		lastRangesSelected = #(rangeArray.count - 1)
		
		updateRanges keepSelection:true
		
		if rangeRO != undefined then
			if rangeRO.open == true then
				updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
	)
	
	
	
	
	function clearRangeList =
	(
		for i = rangeArray.count to 2 by -1 do
			deleteItem rangeArray i
		
		lastRangesSelected = #(0)
		updateRanges keepSelection:true
		
		if rangeRO != undefined then
			if rangeRO.open == true then
				updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	function getPrefix str1 =
	(
		local searchSign = "_"
		local tempString = ""
		
		if findString str1 " " != undefined then
			searchString = " "
			
		local tempFilter = filterString str1 searchSign
		
		
		for i = 1 to (tempFilter.count - 1) do
			tempString += tempFilter[i] + (if i < (tempFilter.count - 1) then searchSign else "")
			
		return tempString
	)
	
	function getSuffix str1 =
	(
		local searchSign = "_"
		local searchPart = ""
		
		if findString str1 " " != undefined then
			searchSign = " "
		
		local tempFilter = filterString str1 searchSign
		
		if tempFilter.count > 1 then
		(
			for i = 2 to tempFilter.count do
				searchPart += tempFilter[i] + (if i < tempFilter.count then searchSign else "")
		)
		else
			searchPart = tempFilter[tempFilter.count]
		
		return searchPart
	)
	
	
	
	
	
	
	
	function getTimeTags op =
	(
		struct timeTagStruct (tagTime = 0f, tagName = "", match = 0, searchTag = "")
		
		local tagArray = #()
		
		for i = 1 to FrameTagManager.GetTagCount() do
		(
			local tagID = FrameTagManager.GetTagID i
			local tagTime = FrameTagManager.GetTimeByID tagID
			local tagName = FrameTagManager.GetNameByID tagID
			
			append tagArray (timeTagStruct tagTime:tagTime tagName:tagName)
		)
		
		
		for i = 1 to tagArray.count do
		(
			local searchPart1 = ""
			local foundMatch = 0
			
			case op of
			(
				#prefix:	searchPart1 = getPrefix tagArray[i].tagName
				#suffix:	searchPart1 = getSuffix tagArray[i].tagName
			)
			
			for d = 1 to tagArray.count do
			(
				if i != d and tagArray[d].match == 0 then
				(
					case op of
					(
						#prefix:	searchPart2 = getPrefix tagArray[d].tagName
						#suffix:	searchPart2 = getSuffix tagArray[d].tagName
					)
					
					if searchPart1 == searchPart2 then
					(
						foundMatch = d
						exit
					)
				)
			)
			
			tagArray[i].match = foundMatch
			
			if foundMatch > 0 then
				tagArray[i].searchTag = searchPart1
		)
		
		local tagsSorted = true
		
		while tagsSorted == false do
		(
			escapeEnable = true
			tagsSorted = true
			
			for i = 1 to (tagArray.count - 1) do
			(
				if tagArray[i].tagTime > tagArray[i+1].tagTime then
				(
					local tempTag = copy tagArray[i]
					tagArray[i] = tagArray[i+1]
					tagArray[i+1] = tempTag
					
					tagsSorted = false
				)
			)
		)
		
		
		for i = 1 to tagArray.count do
		(
			if tagArray[i].match > 0 then
				if tagArray[tagArray[i].match].match == 0 then
					tagArray[tagArray[i].match].match = -1
			
		)
		
		for i = 1 to tagArray.count do
		(
			local startTime = 0
			local endTime = 0
			local tagName = tagArray[i].searchTag
			
			if tagArray[i].match >= 0 then
			(
				startTime = tagArray[i].tagTime.frame as Integer
				
				if tagArray[i].match > 0 then
					endTime = tagArray[tagArray[i].match].tagTime.frame as Integer
				else
				(
					endTime = tagArray[i].tagTime.frame as Integer
					tagName = tagArray[i].tagName
				)
				
				addRange custom:(rangeStruct animName:tagName start:startTime end:endTime rootNodes:#()) noUpdate:true
			)
		)
		
		updateRanges keepSelection:true
	)
	
	
	
	
	
	
	
	function callBoneMenu =
	(
		RCMenu boneMenuRC
		(
			menuItem miSetBones "Set Bones" enabled:true
			seperator sep1
			menuItem miRemove "Remove" enabled:true
			menuItem miClearList "Clear List" enabled:true
			
			
			on boneMenuRC open do
			(
				if lvBones.items.count == 0 then
					miClearList.enabled = false
				
				if lvBones.selectedItems.count == 0 then
					miRemove.enabled = false
				
				if lastRangesSelected.count == 0 then
					miSetBones.enabled = false
			)
			
			
			on miSetBones picked do
				setRangeBones()
			
			
			on miRemove picked do
				removeBone()
			
			
			on miClearList picked do
				clearBoneList()
		)
		
		registerRightClickMenu boneMenuRC
		popUpMenu boneMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	
	
	
	
	function callRangeMenu =
	(
		RCMenu rangeMenuRC
		(
			menuItem miAdd "Add" enabled:true
			seperator sep1
			menuItem miRemove "Remove" enabled:true
			menuItem miClearList "Clear List" enabled:true
			seperator sep3
			subMenu "Get TimeTags"
			(
				menuItem miGetTimeTagsSuffix "start/end first" enabled:true
				menuItem miGetTimeTagsPrefix "start/end last" enabled:true
			)
			
			
			on rangeMenuRC open do
			(
				if rangeArray.count < 2 then
					miClearList.enabled = false
				
				if FrameTagManager.GetTagCount() == 0 then
				(
					miGetTimeTagsPrefix.enabled = false
					miGetTimeTagsSuffix.enabled = false
				)
				
				if lvRanges.selectedItems.count == 0 then
					miRemove.enabled = false
				else
					if (lvRanges.items.item 0).selected == true and lvRanges.selectedItems.count == 1 then
						miRemove.enabled = false
			)
			
			
			
			on miAdd picked do
				addRange()
			
			
			
			on miRemove picked do
				removeRange()
			
			
			on miClearList picked do
				clearRangeList()
			
			
			
			on miGetTimeTagsPrefix picked do
				getTimeTags #prefix
			
			on miGetTimeTagsSuffix picked do
				getTimeTags #suffix
			
		)
		
		registerRightClickMenu rangeMenuRC
		popUpMenu rangeMenuRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
	)
	
	
	
	
	

	
	
	
	
	
	
	
--animation exporter UI event handlers
	on rltCryExportAnimation open do
	(--initialize
		append groupBoxArray (#(gbAnimationRanges, labAnimationRanges, itAnimationRanges1, itAnimationRanges2, itAnimationRanges3, itAnimationRanges4, itAnimationRanges5))
		append groupBoxArray (#(gbAnimationBones, labAnimationBones, itAnimationBones1, itAnimationBones2, itAnimationBones3, itAnimationBones4, itAnimationBones5))
		append groupBoxArray (#(gbAnimationExportSelected, labAnimationExportSelected, itAnimationExportSelected1, itAnimationExportSelected2, itAnimationExportSelected3, itAnimationExportSelected4, itAnimationExportSelected5))
		append groupBoxArray (#(gbAnimationExportAll, labAnimationExportAll, itAnimationExportAll1, itAnimationExportAll2, itAnimationExportAll3, itAnimationExportAll4, itAnimationExportAll5))
		
		
		
		lvBones.AllowColumnReorder = false
		lvBones.View = lvBones.View.Details
		lvBones.LabelEdit = false
		lvBones.LabelWrap = false
		lvBones.MultiSelect = true
		lvBones.FullRowSelect = true
		lvBones.HideSelection = false
		lvBones.HeaderStyle = lvBones.HeaderStyle.None
		
		lvBones.columns.add ""
		(lvBones.columns.item 0).width = lvBones.width - 4		
			
		
		lvRanges.GridLines = true
		lvRanges.AllowColumnReorder = false
		lvRanges.View = lvRanges.View.Details
		lvRanges.LabelEdit = true
		lvRanges.LabelWrap = true
		lvRanges.MultiSelect = true
		lvRanges.FullRowSelect = true
		lvRanges.HideSelection = false
		
		
		
		lvRanges.columns.add "Name"
		lvRanges.columns.add "<"
		lvRanges.columns.add ">"
		
		lvRanges.ShowItemToolTips = true
		(lvRanges.columns.item 0).displayIndex = 2
		
		
		
		rangeHeightChangeArray[1] = #(	gbAnimationRanges, lvRanges )
		rangePosChangeArray[1] = #(	gbAnimationBones, lvBones, btnAnimationSetBone, btnAnimationRemoveBone,  \
													spnAnimationSubRangeStart, spnAnimationSubRangeEnd )
	
													
													
		boneHeightChangeArray[1] = #(	gbAnimationBones, lvBones, rltCryExportAnimation )
		bonePosChangeArray[1] = #(		gbAnimationExportSelected, gbAnimationExportAll, labAnimationExportPath, \
													itAnimationExportPath, btnAnimationSetExportPath, btnAnimationExplore, \
													ddAnimationExportType, edAnimationExportType, btnAnimationExportSelected, btnAnimationExportAll )
													

		
		for i = 1 to rangeHeightChangeArray[1].count do
			append rangeHeightChangeArray[2] rangeHeightChangeArray[1][i].height
		
		for i = 1 to rangePosChangeArray[1].count do
			append rangePosChangeArray[2] rangePosChangeArray[1][i].pos
		
		for i = 1 to boneHeightChangeArray[1].count do
			append boneHeightChangeArray[2] boneHeightChangeArray[1][i].height
		
		for i = 1 to bonePosChangeArray[1].count do
			append bonePosChangeArray[2] bonePosChangeArray[1][i].pos
		
		
		updateFromScene()
	)
	
	on rltCryExportAnimation close do
	(--remove callbacks
		callbacks.removeScripts id:#getAnimationRanges
		
		try destroyDialog rangeRO catch()
	)

	
	on btnAnimationSetBone pressed do
		setRangeBones()
	
	
	on btnAnimationRemoveBone pressed do
		if queryBox "Remove selected Bone(s) ?" title:"Animation Export" == true then
			removeBone()
	
	
		
	on btnOpenAnimationRangeList changed value do
	(
		if value == true then
			callRangeRO()
		else
			try destroyDialog rangeRO catch()
	)
		
	
	on lvBones MouseUp arg do
	(	
		btnAnimationRemoveBone.enabled = lvBones.selectedItems.count > 0
	)
	
	
	
	on lvRanges MouseUp arg do
	(
		if rangeArray.count > 0 then
		(
			if lvRanges.selectedItems.count == 0 then
			(
				for i = 1 to lastRangesSelected.count do
					(lvRanges.items.item lastRangesSelected[i]).selected = true
			)
			else
			(
				lastRangesSelected = #()
				for i = 0 to (lvRanges.selectedItems.count - 1) do
					append lastRangesSelected (lvRanges.selectedItems.item i).index
				
				if lastRangesSelected.count > 1 and findItem lastRangesSelected 0 != 0 then
				(
					deleteItem lastRangesSelected (findItem lastRangesSelected 0)
					(lvRanges.items.item 0).selected = false
				)
			)
		)
		
		updateRangeValues()
		updateAmountRanges onlyBones:true
		updateType()
	)
	
	
	
	on lvRanges ColumnClick arg do
	(
		local tempMode = sortMode
		if tempMode < 0 then
			tempMode *= -1
		
		if (tempMode - 1) == arg.Column then
			sortMode *= -1
		else
			sortMode = arg.Column + 1
		
		updateRanges()
	)
	
	
	
	
	on lvRanges AfterLabelEdit arg do
	(
		if arg.label != undefined then
		(
			if lastRangesSelected[1] > 0 then
				rangeArray[lastRangesSelected[1] + 1].animName = arg.label
			else
				(lvRanges.items.item lastRangesSelected[1]).text = "*current range"
			
			updateRanges()
		)
	)
	
	
	
	
	on lvRanges DoubleClick arg do
	(
		if lastRangesSelected[1] > 0 then
			try ( animationRange = interval rangeArray[(lastRangesSelected[1] + 1)].start rangeArray[(lastRangesSelected[1] + 1)].end )
			catch( try animationRange = interval rangeArray[(lastRangesSelected[1] + 1)].start (rangeArray[(lastRangesSelected[1] + 1)].end + 1) catch())
	)

	
	
	
	on lvRanges ItemMouseHover arg do
	(
		if arg.item != undefined then
			updateToolTip arg.item.index
	)
	
	
	
	
	
	on btnAnimationAddRange pressed do
		addRange()
	
	on btnAnimationRemoveRange pressed do
		if queryBox "Remove selected Range(s) ?" title:"Animation Export" == true then
			removeRange()
	
	on btnAnimationClearRanges pressed do
		if queryBox "Remove All Ranges ?" title:"Animation Export" == true then
			clearRangeList()
	
	
	
	on spnAnimationSubRangeStart changed arg do
	(
		if (lvRanges.selectedItems.item 0).index == 0 then
			spnAnimationSubRangeStart.value = animationRange.start.frame as Integer
		else
		(
			for i = 1 to lastRangesSelected.count do
				rangeArray[(lastRangesSelected[i] + 1)].start = arg
			
			updateRanges keepSelection:true noSort:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true noSort:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		)
	)
	
	
		
	on spnAnimationSubRangeEnd changed arg do
	(
		if (lvRanges.selectedItems.item 0).index == 0 then
			spnAnimationSubRangeEnd.value = animationRange.end.frame as Integer
		else
		(
			for i = 1 to lastRangesSelected.count do
				rangeArray[(lastRangesSelected[i] + 1)].end = arg
			
			updateRanges keepSelection:true noSort:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true noSort:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
		)
	)
	
	

	
	on btnAnimationGetTimeTags pressed do
	(
		getTimeTags (if radAnimationTimeTagPreSuffix.state == 1 then #prefix else #suffix)
	)
	
	
	on btnExportRange pressed do
	(
		 output_name = getSaveFileName caption:"Range Filename" types:"range file (*.txt)|*.txt|All Files (*.*)|*.*|"

		 if output_name != undefined then 
		 (
			output_file = createfile output_name
			 
			for i = 2 to rangeArray.count do
			(
				local boneName = ""
				if (rangeArray[i].rootNodes.count > 0) then
				(
					boneName = rangeArray[i].rootNodes[1]
				)
				local startT = formattedPrint (rangeArray[i].start As Integer) format:"03u"
				local endT = formattedPrint (rangeArray[i].end As Integer) format:"03u"
				format "%\t%\t%.%\t%\n" startT endT rangeArray[i].animName rangeArray[i].type boneName to:output_file				
			)
			 
			close output_file
			edit output_name
		 )

	)
	
	on btnImportRange pressed do
	(
		 input_name = getOpenFileName caption:"Range Filename" types:"range file (*.txt)|*.txt|All Files (*.*)|*.*|"
		
		 if input_name != undefined then 
		 (
			input_file = openfile input_name
			 
			-- Wipe existing range array
			for i = rangeArray.count to 2 by -1 do
				deleteItem rangeArray i
			
			lastRangesSelected = #(0)
			 
			 print "Load In Array!"

			-- Load in array
			while not eof input_file do
			(
				local fullLine = readLine input_file
				linestream = fullLine as stringstream
				
				local start = readToken linestream
				local end = readToken linestream
				local fullFilename = readToken linestream
				local bonename = readToken linestream
				
				local splitFilename = filterString fullFilename "."
				
				if (bonename == undefined) then
				(
					append rangeArray (rangeStruct animName:splitFilename[1] start:(start as Integer) end:(end as Integer) rootNodes:#() type:splitFilename[2])
				)
				else
				(
					append rangeArray (rangeStruct animName:splitFilename[1] start:(start as Integer) end:(end as Integer) rootNodes:#(bonename) type:splitFilename[2])
				)
			)

			print "start Frames"
			for i = 1 to rangeArray.count do
			(
				print rangeArray[i].start
			)

			-- Update UI
			updateRanges keepSelection:true
			
			if rangeRO != undefined then
				if rangeRO.open == true then
					updateRanges keepSelection:true overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
			 
			close input_file
			edit input_name
		 )

	)

	on btnAnimationSetExportPath pressed do
	(
		cryMaxTools.export.anim.fnSetExportSettings "" dialog:true
		if cryMaxTools.export.anim.sAnimExportPath != "" then
		(
			labAnimationExportPath.text = cryMaxTools.basic.str.cutStringToWidth cryMaxTools.export.anim.sAnimExportPath 160 align:#right
			itAnimationExportPath.toolTip = cryMaxTools.export.anim.sAnimExportPath
			btnAnimationExplore.enabled = true
		)
		else
			btnAnimationExplore.enabled = false
	)
	
	
	on ddAnimationExportType selected arg do
	(
		updateType onlyUI:true
		
		case ddAnimationExportType.selection of
		(
			1:	edAnimationExportType.text = "caf"
			2:	edAnimationExportType.text = "anm"
		)
		
		
		for i = 0 to (lvRanges.selectedItems.count - 1) do
		(
			local index = (lvRanges.selectedItems.item i).index + 1
			
			rangeArray[index].type = edAnimationExportType.text
		)
		
		if rangeRO != undefined then
			if rangeRO.open == true then
				updateRanges overwriteList:rangeRO.lvBigRanges overwriteBones:true overwriteType:true
	)
	
	on edAnimationExportType changed value do
	(
		for i = 0 to (lvRanges.selectedItems.count - 1) do
		(
			local index = (lvRanges.selectedItems.item i).index + 1
			
			rangeArray[index].type = value
		)
	)
	
	
	
	on btnAnimationExportSelected pressed do
	(
		local exportArray = #()
		for i = 1 to lastRangesSelected.count do
			append exportArray (lastRangesSelected[i] + 1)
		
		
		local exportStatus = cryMaxTools.export.anim.fnExport useSelection:exportArray customOpenPath:lastExportedPath
		
		if lastRangesSelected[1] == 0 and exportStatus != "" then
			lastExportedPath = exportStatus
	)
	
	
	
	on btnAnimationExportAll pressed do
	(--perform export
		cryMaxTools.export.anim.fnExport()
	)
	
	on btnAnimationExplore pressed do
	(--reveals the current scene filepath in a new explorer window or throws an error when not saved		
		shellLaunch "explorer.exe" cryMaxTools.export.anim.sAnimExportPath
	)
)

--add exporter rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryExportAnimation" rltCryExportAnimation #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExportAnimation")

callbacks.addScript #filePostOpen "(cryMaxTools.basic.ROMan.get \"rltCryExportAnimation\").updateFromScene()" id:#getAnimationRanges
callbacks.addScript #systemPostReset "(cryMaxTools.basic.ROMan.get \"rltCryExportAnimation\").updateFromScene()" id:#getAnimationRanges
callbacks.addScript #systemPostNew "(cryMaxTools.basic.ROMan.get \"rltCryExportAnimation\").updateFromScene()" id:#getAnimationRanges