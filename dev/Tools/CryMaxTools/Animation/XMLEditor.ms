try destroyDialog XMLEditorRO catch()


rollout XMLEditorRO "XMLEditor"
(
	local defaultNodeBackColor = [255,255,255]
	local defaultNodeForeColor = [0,0,0]
	
	struct entryStruct (visible = true, active = true, value = "", comment = "", parent = 0, treeNode, saveNode, template = 0)
	
	struct templateEntryStruct (name = "", amount = 0, type = "", data = "", range = [0,0], backColor = defaultNodeBackColor, foreColor = defaultNodeForeColor, captionForParents = #{}, help = "", default = "", parent = 0)
	
	local template = #()
	local file = #()
	local errorArray = #()
	
	local counter = 0
	local comment = ""
	
	local nodeMenu = undefined
	
	local addNodeArray = #()
	local propertyArray = #()
	
	local deactivateStr = "XMLDeactivated_"
	local commentStr = "XMLComment"
	local selIndex = 0
	
	local onUpdate = false
	
	local showHidden = false
	
	
	dotNetControl lvNodes "System.Windows.Forms.TreeView" name:"lvNodes" pos:[5,5] width:350 height:390
	label labHelp "Help" pos:[472+50,5]
	dotNetControl edHelp "System.Windows.Forms.TextBox" name:"edHelp" pos:[310+50,20] width:350 height:60
	label labComment "Comment" pos:[460+50,80]
	dotNetControl edComment "System.Windows.Forms.TextBox" name:"edHelp" pos:[310+50,95] width:350 height:55
	dotNetControl lvProperties "System.Windows.Forms.DataGridView" name:"lvProperties" pos:[310+50,155] width:350 height:190
	
	dotNetControl labPropertyType "System.Windows.Forms.Label" name:"labPropertyType" text:"Property :                  None" pos:[360+3,110+240] width:180 height:18
	dotNetControl labPropertyValue "System.Windows.Forms.Label" name:"labPropertyValue" text:"Value :" pos:[360+3,132+240+3] width:45 height:18
	
	dotNetcontrol btnPropertyPath "System.Windows.Forms.Button" name:"btnPropertyPath" text:"..." pos:[360+50+15+263,130+240+3] width:20 height:18
	dotNetControl edPropertyValue "System.Windows.Forms.TextBox" name:"edPropertyValue" pos:[360+50+15,128+240+3] width:285 height:18
	dotNetControl cbPropertyValue "System.Windows.Forms.ComboBox" name:"cbPropertyValue" pos:[360+50+15,128+240+3] width:285 height:18
	dotNetControl spnPropertyValueInteger "System.Windows.Forms.NumericUpDown" name:"spnPropertyValueInteger" pos:[360+50+15,130+240+3] width:285 height:18
	dotNetControl spnPropertyValueFloat "System.Windows.Forms.NumericUpDown" name:"spnPropertyValueFloat" pos:[360+50+15,130+240+3] width:285 height:18
	
	
	
	
	function getTemplateEntry xmlNode parent =
	(
		local newArray = #()
		
		if xmlNode != undefined then
		(	
			local newEntry = templateEntryStruct()
			newEntry.name = xmlNode.name
			newEntry.parent = parent
			
				--// adds all attributes
			for i = 0 to (xmlNode.attributes.count - 1) do
			(
				local attrib = xmlNode.attributes.itemOf[i]
				
				case attrib.name of
				(
					"amount": newEntry.amount = execute attrib.value
					"type":	newEntry.type = attrib.value as name
					"data":	newEntry.data = attrib.value as name
					"range":
					(
						local tempRange = filterString attrib.value ".."
						newEntry.range = [(execute tempRange[1]), (execute tempRange[2])]
					)
					"backColor":	newEntry.backColor = execute ("[" + attrib.value + "]")
					"foreColor":		newEntry.foreColor = execute ("[" + attrib.value + "]")
					"captionForParents":		newEntry.captionForParents = execute ("#{" + attrib.value + "}")
					"help":	newEntry.help = attrib.value
					"default":	newEntry.default = attrib.value
				)
			)
			
			append newArray newEntry
			counter+=1
			
			local thisParent = counter
			
			for i = 0 to (xmlNode.childNodes.count - 1) do
				join newArray (getTemplateEntry xmlNode.childNodes.itemOf[i] (thisParent))
		)
		
		return newArray
	)
	
		
	function loadTemplateXML filepath =
	(
		if doesFileExist filepath then
		(
			local xmlFile = dotNetObject "system.xml.xmlDocument"
				
			xmlFile.load filepath
			
			local fileRoot = xmlFile.documentElement
			
			counter = 0
			template = getTemplateEntry fileRoot 0
			
			return true
		)
		
		return false
	)
	
	
	
	
	function getEntry xmlNode parent localParent:0 =
	(
		local newArray = #()
		
		if xmlNode != undefined then
		(
			if xmlNode.nodeType == xmlNode.nodeType.Comment then
				comment = xmlNode.value
			
			
			
			if xmlNode.nodeType != xmlNode.nodeType.Text then
			(
				local newEntry = entryStruct()
				local nodeName = xmlNode.name
				local foundDeactivate = findString nodeName deactivateStr
				
				if foundDeactivate != undefined then
				(
					newEntry.active = false
					nodeName = subString nodeName (deactivateStr.count + 1) nodeName.count
				)
				
				
				for i = 1 to template.count do
				(
					if template[i].type == #node and (cryMaxTools.basic.str.lower template[i].name) == (cryMaxTools.basic.str.lower nodeName) and template[i].parent == localParent then
					(
						newEntry.template = i
						exit
					)
				)
				
				
				if newEntry.template > 0 then
				(
					if xmlNode.hasAttributes == true or xmlNode.hasChildNodes == true then
					(
						newEntry.parent = parent
						
						if comment != "" then
						(
							newEntry.comment = comment
							comment = ""
						)
						
						append newArray newEntry
						
						counter+=1
						
						local thisParent = counter
						
						for i = 0 to (xmlNode.attributes.count - 1) do
						(
							local attribComment = ""
							
							local attrib = xmlNode.attributes.itemOf[i]
							if attrib.name == commentStr then
								attribComment = attrib.value
							
							newEntry.comment = attribComment
							
							
							for d = 1 to template.count do
							(
								if template[d].type == #attrib and (cryMaxTools.basic.str.lower template[d].name) == (cryMaxTools.basic.str.lower attrib.name) and template[d].parent == newEntry.template then
								(
									append newArray (entryStruct parent:(thisParent) value:attrib.value template:d)
									counter+=1
									exit
								)
							)
						)
						
						for i = 0 to (xmlNode.childNodes.count - 1) do
							join newArray (getEntry xmlNode.childNodes.itemOf[i] (thisParent) localParent:newEntry.template)
					)
				)
			)
		)
		
		return newArray
	)
		
		
		
	function loadXML filepath =
	(
		if doesFileExist filepath then
		(
			local xmlFile = dotNetObject "system.xml.xmlDocument"
				
			xmlFile.load filepath
			
			local fileRoot = xmlFile.documentElement
			
			counter = 0
			file = getEntry fileRoot 0
			
			return true
		)
		
		return false
	)
	
	
	
	
	
	
	function saveXML filepath =
	(
		local xmlFile = dotNetObject "system.xml.xmlDocument"
		
		for i = 1 to file.count do
		(
			local templatePart = template[file[i].template]
			local newEntry = undefined
			
			case templatePart.type of
			(
				#node:	newEntry = xmlFile.CreateElement ((if file[i].active == false then deactivateStr else "") + templatePart.name)
				#attrib:
				(
					newEntry = xmlFile.CreateAttribute templatePart.name
					newEntry.value = file[i].value
				)
			)
			
			file[i].saveNode = newEntry
			
			if file[i].parent > 0 and file[i].visible == true then
			(
				case templatePart.type of
				(
					#node:
					(
						if file[i].comment != "" then
							file[i].saveNode.SetAttribute commentStr file[i].comment
						
						file[file[i].parent].saveNode.AppendChild file[i].saveNode
					)
					#attrib:	file[file[i].parent].saveNode.SetAttributeNode file[i].saveNode
				)
			)
		)
		
		xmlFile.AppendChild file[1].saveNode
		xmlFile.save filepath
	)
	
	
	
	
	
	
	function getSelectedNode =
	(
		local index = 0
		
		if lvNodes.selectedNode != undefined then
		(
			for i = 1 to file.count do
			(
				if file[i].treeNode == lvNodes.selectedNode then
				(
					index = i
					exit
				)
			)
		)
		
		return index
	)
	
	
	
	function getMaxExtents strArray =
	(
		local maxExtent = 0
		
		if classOf strArray == Array then
		(
			for i = 1 to strArray.count do
			(
				local tempExtent = (getTextExtent strArray[i])[1]
				
				if tempExtent > maxExtent then
					maxExtent = tempExtent
			)
		)
		
		maxExtent += 16
		
		return maxExtent
	)
	
	
	
	function updateProperty =
	(
		onUpdate = true
		
		edPropertyValue.text = ""
		spnPropertyValueInteger.value = 0
		spnPropertyValueFloat.value = 0.0
		cbPropertyValue.selectedIndex = 0
		
		edPropertyValue.visible = false
		cbPropertyValue.visible = false
		spnPropertyValueInteger.visible = false
		spnPropertyValueFloat.visible = false
		btnPropertyPath.visible = false
		
		edPropertyValue.width = 285
		
		labPropertyType.text = "Property :                  None"
		
		if lvProperties.selectedCells.count > 0 then
		(
			local index = (lvProperties.selectedCells.item 0).rowIndex + 1
			if index > 0 and index <= propertyArray.count then
			(
				edPropertyValue.enabled = true
				
				local entryType = template[propertyArray[index].template].data
				
				case entryType of
				(
					#integer:		( usedElement = spnPropertyValueInteger ; usedElement.value = (execute propertyArray[index].value) )
					#float:			( usedElement = spnPropertyValueFloat ; usedElement.value = (execute propertyArray[index].value) )
					#boolean:		( usedElement = cbPropertyValue ; usedElement.selectedIndex = (if (cryMaxTools.basic.str.lower propertyArray[index].value) == "yes" or (cryMaxTools.basic.str.lower propertyArray[index].value) == "true" then 0 else  1 ))
					#string:			( usedElement = edPropertyValue ; usedElement.text = propertyArray[index].value )
					#path:			( usedElement = edPropertyValue ; usedElement.width = 260 ; btnPropertyPath.visible = true ; usedElement.text = propertyArray[index].value )
				)
				
				usedElement.visible = true
				
				labPropertyType.text = "Property :                  " + entryType as String
			)
			else
			(
				edPropertyValue.enabled = false
				edPropertyValue.visible = true
			)
		)
		
		
		
		onUpdate = false
		
		/*
		edPropertyValue.Refresh()
		cbPropertyValue.Refresh()
		spnPropertyValueInteger.Refresh()
		spnPropertyValueFloat.Refresh()
		btnPropertyPath.Refresh()
		*/
	)
	
	
	
	
	
	function updateProperties =
	(
		lvProperties.rows.Clear()
		edHelp.text = ""
		edComment.text = ""
		propertyArray = #()
		
		local entries = #("Property")
			
		if selIndex > 0 then
		(
			local tempCounter = 0
			
			for i = 1 to file.count do
			(
				local templatePart = template[file[i].template]
				
				if file[i].parent == selIndex then
				(
					if templatePart.type == #attrib then
					(	
						lvProperties.rows.add()
						
						((lvProperties.rows.item tempCounter).cells.item 0).value = "  " + templatePart.name
						((lvProperties.rows.item tempCounter).cells.item 1).value = "  " + file[i].value
						
						append entries templatePart.name
						append propertyArray file[i]
						
						tempCounter += 1
					)
				)
				
				if i == selIndex then
				(
					edComment.text = file[i].comment
					edHelp.text = templatePart.help
				)
			)
		)
		
		(lvProperties.columns.item 0).width = getMaxExtents entries
		(lvProperties.columns.item 1).width = lvProperties.width - (lvProperties.columns.item 0).width - 20
		
		updateProperty()
	)
	
	
	
	
	
	function updateNodes =
	(
		lvNodes.BeginUpdate()
		
		local expandArray = #()
		local topNodeIndex = 0
		
		for i = 1 to file.count do
		(
			if file[i].treeNode != undefined and template[file[i].template].type == #node then
			(
				if file[i].treeNode.isExpanded == true then
					append expandArray i
				
				if file[i].treeNode == lvNodes.topNode then
					topNodeIndex = i
			)
		)
		
		
		lvNodes.nodes.clear()
		
		for i = 1 to file.count do
		(
			file[i].treeNode = dotNetObject "System.Windows.Forms.TreeNode" ""
			
			if file[i].template > 0 then
			(
				local templatePart = template[file[i].template]
				
				file[i].treeNode.text = templatePart.name
				file[i].treeNode.backColor = file[i].treeNode.backColor.fromARGB templatePart.backColor[1] templatePart.backColor[2] templatePart.backColor[3]
				file[i].treeNode.foreColor = file[i].treeNode.foreColor.fromARGB templatePart.foreColor[1] templatePart.foreColor[2] templatePart.foreColor[3]
				
				if file[i].active == false then
					file[i].treeNode.foreColor = file[i].treeNode.foreColor.fromARGB 170 170 170
				
				if file[i].visible == false then
					file[i].treeNode.foreColor = file[i].treeNode.foreColor.fromARGB 200 0 0
				
				local parent = file[i].parent
				
				if parent > 0 then
				(
					if templatePart.captionForParents.count > 0 then
					(
						local tempParent = parent
						
						for d = 1 to templatePart.captionForParents.count do
						(	
							if templatePart.captionForParents[d] == true then
								file[tempParent].treeNode.text += "   ( " + file[i].value + " )"
							
							tempParent = file[tempParent].parent
						)
					)
					
					if showHidden == true or (showHidden == false and file[i].visible == true) then
						if templatePart.type == #node then
							file[parent].treeNode.nodes.Add file[i].treeNode
				)
			)
		)
		
		if file.count > 0 and file[1].visible == true then
		(
			lvNodes.nodes.Add file[1].treeNode
			(lvNodes.nodes.item 0).expand()
		)
		
		
		for i = 1 to expandArray.count do
			if template[file[expandArray[i]].template].type == #node then
				file[expandArray[i]].treeNode.expand()
			
		if selIndex > 0 and selIndex <= file.count then
			if template[file[selIndex].template].type == #node then
				lvNodes.selectedNode = file[selIndex].treeNode
			
		if lvNodes.selectedNode == undefined and lvNodes.nodes.count > 0 then
			lvNodes.selectedNode = lvNodes.nodes.item 0
			
		if topNodeIndex > 0 and topNodeIndex <= file.count then
			if template[file[topNodeIndex].template].type == #node then
				lvNodes.topNode = file[topNodeIndex].treeNode
			
		updateProperties()
			
		lvNodes.EndUpdate()
	)
	
		
		
	
	on XMLEditorRO open do
	(
		edHelp.multiline = true
		edHelp.ReadOnly = false
		edHelp.ScrollBars = edHelp.ScrollBars.Both
		edComment.multiline = true
		edComment.ScrollBars = edComment.ScrollBars.Both
		
		lvNodes.showLines = false
		lvNodes.fullRowSelect = true
		lvNodes.hideSelection = false
		
		lvProperties.RowHeadersVisible = false
		lvProperties.ReadOnly = true
		lvProperties.AllowUserToResizeColumns = false
		lvProperties.AllowUserToOrderColumns = false
		lvProperties.ShowCellToolTips = false
		
		
		lvProperties.ColumnHeadersDefaultCellStyle.alignment = lvProperties.ColumnHeadersDefaultCellStyle.alignment.MiddleCenter
		
		
		local RowTemplate = lvProperties.RowTemplate
		
		RowTemplate.resizable = RowTemplate.resizable.false
		RowTemplate.height = 18
		lvProperties.RowTemplate = RowTemplate
		
		lvProperties.ColumnHeadersHeightSizeMode = lvProperties.ColumnHeadersHeightSizeMode.DisableResizing
		lvProperties.ColumnHeadersHeight = 20
		
		
		
		lvProperties.ColumnCount = 2
		(lvProperties.Columns.item 0).name = "Property"
		(lvProperties.Columns.item 1).name = "Value"
		
		
		local typeStyle = (lvProperties.Columns.item 1).DefaultCellStyle
		typeStyle.Alignment = typeStyle.Alignment.MiddleLeft
		(lvProperties.Columns.item 0).DefaultCellStyle = typeStyle
		(lvProperties.Columns.item 1).DefaultCellStyle = typeStyle
		
		(lvProperties.Columns.item 0).width = 70
		(lvProperties.Columns.item 1).width = 175
		
		
		local sortMode = (lvProperties.Columns.item 0).SortMode.NotSortable
		(lvProperties.Columns.item 0).SortMode = sortMode
		(lvProperties.Columns.item 1).SortMode = sortMode
		
		cbPropertyValue.items.Add "true"
		cbPropertyValue.items.Add "false"
		cbPropertyValue.DropDownStyle = cbPropertyValue.DropDownStyle.DropDownList
	)
	
	
	on lvNodes MouseUp arg do
	(
		local tempNode = lvNodes.hitTest arg.location
		onUpdate = true
		
		if tempNode != undefined then
		(
			if tempNode.node != undefined then
			(
				lvNodes.selectedNode = tempNode.node
				selIndex = getSelectedNode()
			)
		)
		
		updateProperties()
		
		if arg.button == arg.button.right then
		(
			RCMenu nodeMenu
			(
				fn isVisible1 = (addNodeArray.count >= 1 )
				fn isVisible2 = (addNodeArray.count >= 2 )
				fn isVisible3 = (addNodeArray.count >= 3 )
				fn isVisible4 = (addNodeArray.count >= 4 )
				fn isVisible5 = (addNodeArray.count >= 5 )
				fn isVisible6 = (addNodeArray.count >= 6 )
				fn isVisible7 = (addNodeArray.count >= 7 )
				fn isVisible8 = (addNodeArray.count >= 8 )
				fn isVisible9 = (addNodeArray.count >= 9 )
				fn isVisible10 = (addNodeArray.count >= 10 )
				fn isVisible11 = (addNodeArray.count >= 11 )
				fn isVisible12 = (addNodeArray.count >= 12 )
				fn isVisible13 = (addNodeArray.count >= 13 )
				fn isVisible14 = (addNodeArray.count >= 14 )
				fn isVisible15 = (addNodeArray.count >= 15 )
				fn isVisible16 = (addNodeArray.count >= 16 )
				fn isVisible17 = (addNodeArray.count >= 17 )
				fn isVisible18 = (addNodeArray.count >= 18 )
				fn isVisible19 = (addNodeArray.count >= 19 )
				fn isVisible20 = (addNodeArray.count >= 20 )
				fn isVisible21 = (addNodeArray.count >= 21 )
				fn isVisible22 = (addNodeArray.count >= 22 )
				fn isVisible23 = (addNodeArray.count >= 23 )
				fn isVisible24 = (addNodeArray.count >= 24 )
				fn isVisible25 = (addNodeArray.count >= 25 )
				fn isVisible26 = (addNodeArray.count >= 26 )
				fn isVisible27 = (addNodeArray.count >= 27 )
				fn isVisible28 = (addNodeArray.count >= 28 )
				fn isVisible29 = (addNodeArray.count >= 29 )
				fn isVisible30 = (addNodeArray.count >= 30 )
				
				fn addNode index =
				(
					if addNodeArray[index] != undefined then
					(	
						append file (entryStruct template:addNodeArray[index] parent:selIndex)
						
						updateNodes()
					)
				)
				
				
				menuItem miLoad "Load"
				menuItem miLoadTemplate "Load Template"
				separator sep1
				menuItem miSave "Save"
				separator sep2
				subMenu "Add"
				(
					menuItem miAddNode1 filter:isVisible1 enabled:true
					menuItem miAddNode2 filter:isVisible2 enabled:true
					menuItem miAddNode3 filter:isVisible3 enabled:true
					menuItem miAddNode4 filter:isVisible4 enabled:true
					menuItem miAddNode5 filter:isVisible5 enabled:true
					menuItem miAddNode6 filter:isVisible6 enabled:true
					menuItem miAddNode7 filter:isVisible7 enabled:true
					menuItem miAddNode8 filter:isVisible8 enabled:true
					menuItem miAddNode9 filter:isVisible9 enabled:true
					menuItem miAddNode10 filter:isVisible10 enabled:true
					menuItem miAddNode11 filter:isVisible11 enabled:true
					menuItem miAddNode12 filter:isVisible12 enabled:true
					menuItem miAddNode13 filter:isVisible13 enabled:true
					menuItem miAddNode14 filter:isVisible14 enabled:true
					menuItem miAddNode15 filter:isVisible15 enabled:true
					menuItem miAddNode16 filter:isVisible16 enabled:true
					menuItem miAddNode17 filter:isVisible17 enabled:true
					menuItem miAddNode18 filter:isVisible18 enabled:true
					menuItem miAddNode19 filter:isVisible19 enabled:true
					menuItem miAddNode20 filter:isVisible20 enabled:true
					menuItem miAddNode21 filter:isVisible21 enabled:true
					menuItem miAddNode22 filter:isVisible22 enabled:true
					menuItem miAddNode23 filter:isVisible23 enabled:true
					menuItem miAddNode24 filter:isVisible24 enabled:true
					menuItem miAddNode25 filter:isVisible25 enabled:true
					menuItem miAddNode26 filter:isVisible26 enabled:true
					menuItem miAddNode27 filter:isVisible27 enabled:true
					menuItem miAddNode28 filter:isVisible28 enabled:true
					menuItem miAddNode29 filter:isVisible29 enabled:true
					menuItem miAddNode30 filter:isVisible30 enabled:true
				)
				menuItem miDelete "Delete"
				separator spe3
				menuItem miDeactivate "Deactivate"
				separator spe4
				menuItem miExpandAll "Expand All"
				menuItem miCollapseAll "Collapse All"
				
				
				on nodeMenu open do
				(
					addNodeArray = #()
					local addNodeUnusedArray = #()
					
					if selIndex > 0 then
					(
						if file[selIndex].active == true then
							miDeactivate.text = "Deactivate"
						else
							miDeactivate.text = "Activate"
						
						if file[selIndex].parent > 0 then
							miDeactivate.enabled = file[file[selIndex].parent].active
						
						for i = 1 to template.count do
							if template[i].type == #node and template[i].parent == file[selIndex].template then
								append addNodeArray i
						
						
							
						for i = 1 to file.count do
						(
							local templatePart = template[file[i].template]
							if templatePart.type == #node and file[i].parent == selIndex then
							(
								local tempCounter = 0
								for d = 1 to addNodeArray.count do
									if addNodeArray[d] == file[i].template then
										tempCounter += 1
								
								
								if templatePart.amount != 0 and tempCounter >= templatePart.amount then
									append addNodeUnusedArray file[i].template
							)
						)
					)
					else
						if template[1] != undefined then
							append addNodeArray 1
						
					for i = 1 to addNodeArray.count do
					(
						local tempString = (".text = \"" + template[addNodeArray[i]].name + "\"")
						execute ("XMLEditorRO.nodeMenu.miAddNode" + i as String + tempString)
						
						if findItem addNodeUnusedArray addNodeArray[i] != 0 then
							execute ("XMLEditorRO.nodeMenu.miAddNode" + i as String + ".enabled = false")
					)
				)
				
				
				
				
				on miLoadTemplate picked do
				(
					local filepath = getOpenFileName "Select Template XML"
					
					if filepath != undefined then
						loadTemplateXML filepath
				)
				
				on miLoad picked do
				(
					local filepath = getOpenFileName "Select XML"
					
					if filepath != undefined then
					(
						loadXML filepath
						updateNodes()
					)
				)

				on miSave picked do
				(
					local filepath = getSaveFileName "Select XML"
					
					if filepath != undefined then
						saveXML filepath
				)
				
				
				on miDeactivate picked do
				(
					if selIndex > 0 then
					(
						local templatePart = template[file[selIndex].template]
						local usedColor = file[selIndex].treeNode.foreColor.fromARGB templatePart.foreColor[1] templatePart.foreColor[2] templatePart.foreColor[3]
						
						
						file[selIndex].active = not file[selIndex].active
						
						if file[selIndex].active == false then
							usedColor = file[selIndex].treeNode.foreColor.fromARGB 170 170 170
						
						file[selIndex].treeNode.foreColor = usedColor
						
						for i = selIndex+1 to file.count do
						(
							if file[i].parent == file[selIndex].parent then
								exit
							
							if file[i].parent > file[selIndex].parent then
							(
								templatePart = template[file[i].template]
								file[i].active = file[selIndex].active
								
								file[i].treeNode.foreColor = usedColor
							)
						)
					)
				)
				
				
				on miDelete picked do
				(
					if selIndex > 0 then
					(
						for i = file.count to 1 by -1 do
						(
							if file[i].parent == selIndex then
								file[i].visible = false
						)
						
						file[selIndex].visible = false
						
						updateNodes()
					)
				)
				
				
				
				on miExpandAll picked do
				(
					if selIndex > 0 then
						file[selIndex].treeNode.ExpandAll()
				)
				
				
				
				on miCollapseAll picked do
				(
					if selIndex > 0 then
						file[selIndex].treeNode.Collapse()
				)
				
				
				
				
				
				
				on miAddNode1 picked do	addNode 1
				on miAddNode2 picked do	addNode 2
				on miAddNode3 picked do	addNode 3
				on miAddNode4 picked do	addNode 4
				on miAddNode5 picked do	addNode 5
				on miAddNode6 picked do	addNode 6
				on miAddNode7 picked do	addNode 7
				on miAddNode8 picked do	addNode 8
				on miAddNode9 picked do	addNode 9
				on miAddNode10 picked do	addNode 10
				on miAddNode11 picked do	addNode 11
				on miAddNode12 picked do	addNode 12
				on miAddNode13 picked do	addNode 13
				on miAddNode14 picked do	addNode 14
				on miAddNode15 picked do	addNode 15
				on miAddNode16 picked do	addNode 16
				on miAddNode17 picked do	addNode 17
				on miAddNode18 picked do	addNode 18
				on miAddNode19 picked do	addNode 19
				on miAddNode20 picked do	addNode 20
				on miAddNode21 picked do	addNode 21
				on miAddNode22 picked do	addNode 22
				on miAddNode23 picked do	addNode 23
				on miAddNode24 picked do	addNode 24
				on miAddNode25 picked do	addNode 25
				on miAddNode26 picked do	addNode 26
				on miAddNode27 picked do	addNode 27
				on miAddNode28 picked do	addNode 28
				on miAddNode29 picked do	addNode 29
				on miAddNode30 picked do	addNode 30
			)
			
			popupMenu nodeMenu pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]
		)
		
		onUpdate = false
	)
	
	
	
	on edComment TextChanged arg do
	(
		if onUpdate == false and selIndex > 0 then
			file[selIndex].comment = edComment.text
	)
	
	
	
	on btnPropertyPath MouseUp arg do
	(
		print "test"
		
	)
	
	
)

createDialog XMLEditorRO 715 400

XMLEditorRO.loadTemplateXML @"C:\CBATemplate.xml"
XMLEditorRO.loadXML @"C:\Animations.cba"
XMLEditorRO.updateNodes()
