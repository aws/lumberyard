gc light:true

try destroyDialog (cryMaxTools.basic.ROMan.get "settingsRO") catch()


rollout settingsRO "Settings"
(
	groupBox gbPerforce " Perforce " pos:[5,5] width:254 height:144
	
	groupBox gbOpen " Open " pos:[145-80+5+12,240-55-160-5] width:83 height:124
	label labOpenYes "Yes" pos:[150-80+5+12,240-35-160-5] width:32 height:16
	label labOpenNo "No" pos:[176-80+5+12,240-35-160-5] width:32 height:16
	label labOpenAsk "Ask" pos:[202-80+5+12,240-35-160-5] width:20 height:16
	
	groupBox gbAdd " Add " pos:[145-80+92+5+9,240-55-160-5] width:83 height:124
	label labAddYes "Yes" pos:[150-80+92+5+9,240-35-160-5] width:32 height:16
	label labAddNo "No" pos:[176-80+92+5+9,240-35-160-5] width:32 height:16
	label labAddAsk "Ask" pos:[202-80+92+5+9,240-35-160-5] width:20 height:16
	
	label labLoad "Load:" pos:[18-5,260-37-160] height:16
	label labSave "Save:" pos:[18-5,260-37+20-160] height:16
	label labExport "Export:" pos:[18-5,260-37+40-160] height:16
	label labSaveExport "Save/Export:" pos:[18-5,260-37+60-160] width:63 height:16
	
	radioButtons radOpenLoad "" pos:[152-80+5+12,260-37-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radOpenSave "" pos:[152-80+5+12,260-37+20-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radOpenExport "" pos:[152-80+5+12,260-37+40-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radOpenSaveExport "" pos:[152-80+5+12,260-37+60-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	
	radioButtons radAddLoad "" pos:[152-80+5+92+9,260-37-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radAddSave "" pos:[152-80+5+92+9,260-37+20-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radAddExport "" pos:[152-80+5+92+9,260-37+40-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	radioButtons radAddSaveExport "" pos:[152-80+5+92+9,260-37+60-160] width:31 height:16 labels:#(" "," ","") columns:3 default:3
	
	
	groupBox gbItems " Items " pos:[264-259,168-5-10] width:253 height:45
	dropDownList ddItems "" pos:[20+250-259,192-4-5-3-10] width:188 height:21
	button btnItemsEdit "Edit" pos:[195+250-259+24,192-3-5-3-10] width:40 height:20
	
	groupBox gbBoneExport " Used Bones to Export " pos:[264-259,168-5-10+48] width:253 height:45
	dropDownList ddBoneExport "" pos:[20+250-259,192-4-5-3-10+48] width:188 height:21
	button btnBoneExportEdit "Edit" pos:[195+250-259+24,192-3-5-3-10+48] width:40 height:20
	
	groupBox gbBasePath " Animation Base Path " pos:[264-259,168-5+56-5-12+50] width:253 height:45
	editText edBasePath "" pos:[7,168-5+80-5-5-12+50] width:212 text:"C:\\"
	button btnBasePath "[...]" pos:[230,168-5+79-5-5-12+50] width:20 height:20
	
	groupBox gbExportPath " Export Path " pos:[264-259,168-5+56-5-12+50+50] width:253 height:45
	editText edExportPath "" pos:[7,168-5+80-5-5-12+50+50] width:212 text:"C:\\"
	button btnExportPath "[...]" pos:[230,168-5+79-5-5-12+50+50] width:20 height:20
	
	
	
	
	
	
	
	on settingsRO open do
	(
		safePos = cryMaxTools.basic.settingsMan.load "settingsRO" intoRollout:true returnPos:true
		try 
			if classOf safePos[1].value[1] == Point2 then
				safePos = safePos[1].value[1]
			else
				safePos = [300,200]
		catch
			safePos = [300,200]
		
		if (cryMaxTools.basic.ROMan.get "loadSaveRO") != undefined then
		(
			if (cryMaxTools.basic.ROMan.get "loadSaveRO").open == true then
				if (cryMaxTools.basic.ROMan.get "loadSaveRO").btnSettings.checked == true then 
					safePos = [((getDialogPos (cryMaxTools.basic.ROMan.get "loadSaveRO"))[1] - 274), ((getDialogPos (cryMaxTools.basic.ROMan.get "loadSaveRO"))[2])]
		)
		else
		(
			cryMaxTools.anim.functions.UI.loadSettings()
		)
		
		setDialogPos (cryMaxTools.basic.ROMan.get "settingsRO") safePos
		
		if getDialogPos (cryMaxTools.basic.ROMan.get "settingsRO") == [-800,0] then
			setDialogPos (cryMaxTools.basic.ROMan.get "settingsRO") [300,200]
		
		try edBasePath.text = cryMaxTools.anim.vars.basePath catch()
		try edExportPath.text = cryMaxTools.anim.vars.exportPath catch()
	
		if cryMaxTools.anim.vars.itemList.count > 0 then
		(
			local itemArray = #()
			
			for i = 1 to cryMaxTools.anim.vars.itemList.count do
				append itemArray cryMaxTools.anim.vars.itemList[i].name
			
			
			ddItems.items = itemArray
		)
		
		local tempArray = #("Use Root Bone")
		if cryMaxTools.anim.vars.boneList.count > 0 then
		(
			append tempArray "------------------------------------------------------------"
			for i = 1 to cryMaxTools.anim.vars.boneList.count do
				if cryMaxTools.anim.vars.boneList[i].active == "true" then
					append tempArray cryMaxTools.anim.vars.boneList[i].name
		)
		
		ddBoneExport.items = tempArray
		
		
		
		
		if (local tempVar = cryMaxTools.anim.functions.UI.items.getIndex()) != 0 then
			ddItems.selection = tempVar
		
		
		radOpenLoad.state = cryMaxTools.anim.vars.perforce[1]
		radOpenSave.state = cryMaxTools.anim.vars.perforce[2]
		radOpenExport.state = cryMaxTools.anim.vars.perforce[3]
		radOpenSaveExport.state = cryMaxTools.anim.vars.perforce[4]
		radAddLoad.state = cryMaxTools.anim.vars.perforce[5]
		radAddSave.state = cryMaxTools.anim.vars.perforce[6]
		radAddExport.state = cryMaxTools.anim.vars.perforce[7]
		radAddSaveExport.state = cryMaxTools.anim.vars.perforce[8]
	)
	
	
	on settingsRO close do
	(
		cryMaxTools.anim.functions.UI.saveSettings()
		local saveArray = #("settingsRO")
		append saveArray (cryMaxTools.basic.ROMan.get "settingsRO")
		
		
		cryMaxTools.basic.settingsMan.save saveArray
		try
		(
			if cryMaxTools.anim.loadSave.settingsChange == undefined then
				try (cryMaxTools.basic.ROMan.get "loadSaveRO").btnSettings.checked = false catch()
		)catch()
		
	)
	
	
	
	on radOpenLoad changed value do cryMaxTools.anim.vars.perforce[1] = value
	on radOpenSave changed value do cryMaxTools.anim.vars.perforce[2] = value
	on radOpenExport changed value do cryMaxTools.anim.vars.perforce[3] = value
	on radOpenSaveExport changed value do cryMaxTools.anim.vars.perforce[4] = value
	on radAddLoad changed value do cryMaxTools.anim.vars.perforce[5] = value
	on radAddSave changed value do cryMaxTools.anim.vars.perforce[6] = value
	on radAddExport changed value do cryMaxTools.anim.vars.perforce[7] = value
	on radAddSaveExport changed value do cryMaxTools.anim.vars.perforce[8] = value
	
	
	on ddItems selected value do
	(
		local tempVar = cryMaxTools.anim.functions.UI.Items.use value
		if tempVar == false then
			ddItems.selection = 1
	)
	
	
	
	
	on btnItemsEdit pressed do
	(
		try cryMaxTools.basic.ROMan.remove "editItemListRO" #main catch()
		
		
		
		rollout editItemListRO "Edit Item List"
		(
			dotNetControl lbItems "System.Windows.Forms.ListView" pos:[1,1] height:185 width:440
			button btnSave "Save" pos:[8,195] height:20 width:80 toolTip:"Save item list"
			button btnAdd "Add" pos:[120,195] height:20 width:60 toolTip:"Adds new entry"
			button btnDelete "Delete" pos:[190,195] height:20 width:60 toolTip:"Deletes selected entry"
			button btnDeleteAll "Delete All" pos:[260,195] height:20 width:60 toolTip:"Clears whole list"
			button btnCancel "Cancel" pos:[350,195] height:20 width:80 toolTip:"Aborts dialog to edit item list"
			
			
			
			
			
			
			function updateExtent =
			(
				local newExtent = #(0,0,0,0,0,0,0,0)
				local columnExtent = #(0,0,0,0,0,0,0,0)
				local lbItems = (cryMaxTools.basic.ROMan.get "editItemListRO").lbItems
				
				for d = 0 to (lbItems.Columns.count - 1) do
				(
					columnExtent[d+1] = (getTextExtent (lbItems.Columns.item d).text)[1]
					
					if columnExtent[d+1] > newExtent[d+1] then
							newExtent[d+1] = columnExtent[d+1] + 12
				)
				
				
				for i = 0 to (lbItems.Items.count - 1) do
				(
					local tempItem = lbItems.Items.item i
					local textExtent = #(0,0,0,0,0,0,0,0)
					
					--// extent of the current filename
					for d = 0 to (lbItems.Columns.count - 1) do
						try ( textExtent[d+1] = (getTextExtent (tempItem.SubItems.item d).text)[1] + 12) catch()
						
					
					
					for f = 1 to textExtent.count do
					(
							--// if the current extent is higher than the result extent
						if textExtent[f] > newExtent[f] then
									--// result extent is current extent
							newExtent[f] = textExtent[f] --+ 12 --// additional number so entries are not cut
					)
						
				)
				
				for f = 1 to newExtent.count do
						--// applys the weidth of the column with the text extent
					(lbItems.Columns.item (f - 1)).width = newExtent[f]
			)
			
			
			function sortList =
			(
				local lbItems = (cryMaxTools.basic.ROMan.get "editItemListRO").lbItems
				
				local newArray = #()
				local oldArray = #()
				local sortedArray = #()
				if classOf lbItems == ActiveXControl then
				(
					local maxCount = lbItems.ListItems.count
					local forStart = 1
				)	
				else
				(
					local maxCount = lbItems.Items.count - 1
					local forStart = 0
				)
				
				for i = forStart to maxCount do
				(
					if classOf lbItems == ActiveXControl then
						append oldArray (#(lbItems.ListItems[i].text, lbItems.ListItems[i].ListSubItems[1].text, lbItems.ListItems[i].ListSubItems[2].text, lbItems.ListItems[i].ListSubItems[3].text, lbItems.ListItems[i].ListSubItems[4].text, lbItems.ListItems[i].ListSubItems[5].text, lbItems.ListItems[i].ListSubItems[6].text, lbItems.ListItems[i].ListSubItems[7].text))
					else
						append oldArray (#((lbItems.Items.item i).text, ((lbItems.Items.item i).SubItems.item 1).text, ((lbItems.Items.item i).SubItems.item 2).text, ((lbItems.Items.item i).SubItems.item 3).text, ((lbItems.Items.item i).SubItems.item 4).text, ((lbItems.Items.item i).SubItems.item 5).text, ((lbItems.Items.item i).SubItems.item 6).text, ((lbItems.Items.item i).SubItems.item 7).text))
				)
				
				for i = 1 to oldArray.count do
					append newArray oldArray[i][1]
				
				sort newArray
				
				for i = 1 to newArray.count do
				(
					findEntry = newArray[i]
					entryFound = 0
					for f = 1 to oldArray.count do
					(
						if oldArray[f][1] == newArray[i] then
						(
							append sortedArray oldArray[f]
							sortedArray[sortedArray.count][1] = newArray[i]
						)
					)
				)
				
				if classOf lbItems != ActiveXControl then
				(
					lbItems.BeginUpdate()
					lbItems.Items.clear()
				)
				else
					lbItems.ListItems.clear()
				
				for i = 1 to newArray.count do
				(
					if classOf lbItems == ActiveXControl then
					(
						local tempItem = lbItems.ListItems.Add text:sortedArray[i][1]
						tempItem.listSubItems.Add text:sortedArray[i][2]
						tempItem.listSubItems.Add text:sortedArray[i][3]
						tempItem.listSubItems.Add text:sortedArray[i][4]
						tempItem.listSubItems.Add text:sortedArray[i][5]
						tempItem.listSubItems.Add text:sortedArray[i][6]
						tempItem.listSubItems.Add text:sortedArray[i][7]
						tempItem.listSubItems.Add text:sortedArray[i][8]
					)
					else
					(	
						local tempItem = lbItems.Items.Add sortedArray[i][1]
						tempItem.SubItems.Add sortedArray[i][2]
						tempItem.SubItems.Add sortedArray[i][3]
						tempItem.SubItems.Add sortedArray[i][4]
						tempItem.SubItems.Add sortedArray[i][5]
						tempItem.SubItems.Add sortedArray[i][6]
						tempItem.SubItems.Add sortedArray[i][7]
						tempItem.SubItems.Add sortedArray[i][8]
					)
				)
				
				if classOf lbItems != ActiveXControl then
					lbItems.EndUpdate()
			)
			
			
			
			
			
			
			
			on editItemListRO open do
			(
				lbItems.GridLines = true			
				lbItems.AllowColumnReorder = true
				lbItems.View = lbItems.View.Details
				lbItems.LabelEdit = false
				lbItems.LabelWrap = true					
				lbItems.FullRowSelect = true
				lbItems.HideSelection = false
				lbItems.Sorting = lbItems.Sorting.Ascending
				
				lbItems.Columns.Add "ID"
				lbItems.Columns.Add "Name"
				lbItems.Columns.Add "External"
				lbItems.Columns.Add "Model"
				lbItems.Columns.Add "Reference"
				lbItems.Columns.Add "Parent"
				lbItems.Columns.Add "Rotation"
				lbItems.Columns.Add "Position"
				
				

				for i = 1 to cryMaxTools.anim.vars.itemList.count do
				(
					local lbItemsEntry = lbItems.Items.Add (i as String)
					lbItemsEntry.SubItems.Add cryMaxTools.anim.vars.itemList[i].name
					lbItemsEntry.SubItems.Add cryMaxTools.anim.vars.itemList[i].external
					
					local maxCount = cryMaxTools.anim.vars.itemList[i].model.count
					tempStringArray = #("","","","","")
					
					
					for f = 1 to maxCount do
					(
						tempStringArray[1] += cryMaxTools.anim.vars.itemList[i].model[f] + (if f < maxCount then ";" else "")
						tempStringArray[2] += cryMaxTools.anim.vars.itemList[i].reference[f] + (if f < maxCount then ";" else "")
						tempStringArray[3] += cryMaxTools.anim.vars.itemList[i].parent[f] + (if f < maxCount then ";" else "")
						tempStringArray[4] += cryMaxTools.anim.vars.itemList[i].rotation[f] as String + (if f < maxCount then ";" else "")
						tempStringArray[5] += cryMaxTools.anim.vars.itemList[i].position[f] as String + (if f < maxCount then ";" else "")
					)
					
					lbItemsEntry.SubItems.Add tempStringArray[1]
					lbItemsEntry.SubItems.Add tempStringArray[2]
					lbItemsEntry.SubItems.Add tempStringArray[3]
					lbItemsEntry.SubItems.Add tempStringArray[4]
					lbItemsEntry.SubItems.Add tempStringArray[5]
				)
				
				updateExtent()
				sortList()
			)
			
			
			on lbItems DoubleClick do
			(
											
				tempItem = lbItems.FocusedItem
				tempItem.checked = not tempItem.checked
																		
				if tempItem != undefined then
				(
					try cryMaxTools.basic.ROMan.remove "entryDetailsRO" #main catch()
					
					rollout entryDetailsRO "Entry Details"
					(
						label labID "ID :" pos:[8,10]
						label labName "Name :" pos:[8,30]
						label labExternal "External :" pos:[8,50]
						label labModel "Model :" pos:[8,70]
						label labReference "Reference :" pos:[8,90]
						label labParent "Parent :" pos:[8,110]
						label labRotation "Rotation :" pos:[8,130]
						label labPosition "Position :" pos:[8,150]
					
						edittext edID "" text:(cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.text pos:[70,10] fieldWidth:300
						edittext edName "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 1).text pos:[70,30] fieldWidth:300
						edittext edExternal "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 2).text pos:[70,50] fieldWidth:300
						edittext edModel "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 3).text pos:[70,70] fieldWidth:300
						edittext edReference "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 4).text pos:[70,90] fieldWidth:300
						edittext edParent "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 5).text pos:[70,110] fieldWidth:300
						edittext edRotation "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 6).text pos:[70,130] fieldWidth:300
						edittext edPosition "" text:((cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem.SubItems.item 7).text pos:[70,150] fieldWidth:300
						
						button btnSave "Save" pos:[40,180] height:20 width:80 toolTip:"Save item to item list"
						button btnSetOffset "Set Offset" pos:[150,180] height:20 width:80 toolTip:"Generates new offset of the selected item"
						button btnCancel "Cancel" pos:[260,180] height:20 width:80 toolTip:"Aborts edit dialog for the selected item"
						
						
						on btnSave pressed do
						(
							local tempItem = (cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem
							
							tempItem.text = edID.text
							(tempItem.SubItems.item 1).text = edName.text
							(tempItem.SubItems.item 2).text = edExternal.text
							(tempItem.SubItems.item 3).text = edModel.text
							(tempItem.SubItems.item 4).text = edReference.text
							(tempItem.SubItems.item 5).text = edParent.text
							(tempItem.SubItems.item 6).text = edRotation.text
							(tempItem.SubItems.item 7).text = edPosition.text
							
							(cryMaxTools.basic.ROMan.get "editItemListRO").updateExtent()
							(cryMaxTools.basic.ROMan.get "editItemListRO").sortList()
							
							cryMaxTools.basic.ROMan.remove "entryDetailsRO" "" #main
							--destroyDialog cryTools.cryAnim.UI.main.models.entryDetails
						)
						
						
						on btnSetOffset pressed do
						(
							local tempItem = (cryMaxTools.basic.ROMan.get "editItemListRO").lbItems.FocusedItem
							if tempItem != undefined then
							(
								local refArray = filterString (tempItem.SubItems.item 4).text ";"
								local parentArray = filterString (tempItem.SubItems.item 5).text ";"
								local rotString = ""
								local posString = ""
								
								for i = 1 to refArray.count do
								(
									
									
									local tempObj = (cryMaxTools.anim.functions.op.createSnapshot object:(getNodeByName refArray[i]))[1]
									local tempParent = getNodeByName parentArray[i]
									
									if (tempObj != undefined) and (tempParent != undefined) then
									(
										if tempParent.classID[1] != 37157 then
										(
											rotString += (in coordsys tempObj tempParent.rotation) as String + (if i < refArray.count then ";" else "")
											posString += (in coordsys tempObj tempParent.pos) as String + (if i < refArray.count then ";" else "")
										)
									)
									
									try (delete tempObj)catch()
								)
								
								edRotation.text = rotString
								edPosition.text = posString
							)
							else
							(
								messageBox "No Item selected." title:"Reset Offset"
							)
							index = undefined
						)
						
						
						
						on btnCancel pressed do
						(
							cryMaxTools.basic.ROMan.remove "entryDetailsRO" "" #main
							--destroyDialog cryTools.cryAnim.UI.main.models.entryDetails
						)
					)
					
					cryMaxTools.basic.ROMan.cryAdd "entryDetailsRO" entryDetailsRO #main
					entryDetailsRO = undefined
					--cryTools.cryAnim.UI.main.models.entryDetails = entryDetailsRO
					
						
					createDialog (cryMaxTools.basic.ROMan.get "entryDetailsRO") 385 210
					
				)
				tempItem = undefined
			)
			
			
			
			on btnAdd pressed do
			(
				local tempStringArray = (cryMaxToolsAnimItemStruct name:"" external:"" model:"" reference:"" parent:"" rotation:"" position:"")
				local tempArray = #()
				
				
				local objArray = selectByName title:("Select Nodes to be added") showHidden:true filter:cryMaxTools.anim.functions.UI.selectByNameFilterNode single:false
				
				if objArray != undefined then
				(
					local objLink = #()
					
					for obj in objArray do
					(
						global tempObj = obj
						
						objLink = selectByName title:("Select " + obj.name + " Reference") showHidden:true single:true filter:cryMaxTools.anim.functions.UI.selectByNameFilterParent
						if objLink != undefined then
						(
							objRotation = (in coordsys objLink obj.rotation) as String
							objPosition = (in coordsys objLink obj.position) as String
							
							objLink = objLink.name
						)
						else
						(
							objLink = ""
							objRotation = ""
							objPosition = ""
						)
						
						if obj.parent != undefined then
							objParent = obj.parent.name
						else
							objParent = ""
								
						append tempArray (cryMaxToolsAnimItemStruct name:obj.name model:obj.name external:obj.name parent:objParent reference:objLink rotation:objRotation position:objPosition )
					)
					tempObj = undefined
				)
				
				if tempArray.count > 0 then
				(
					tempStringArray.name = tempArray[1].name
					tempStringArray.external = tempArray[1].external
					
					for i = 1 to tempArray.count do
					(
						tempStringArray.model += tempArray[i].model + (if i != tempArray.count then ";" else "")
						tempStringArray.reference += tempArray[i].reference as String + (if i != tempArray.count then ";" else "")
						tempStringArray.parent += tempArray[i].parent as String + (if i != tempArray.count then ";" else "")
						tempStringArray.rotation += tempArray[i].rotation as String + (if i != tempArray.count then ";" else "")
						tempStringArray.position += tempArray[i].position as String + (if i != tempArray.count then ";" else "")
					)
					
					local lbItemsEntry = lbItems.Items.Add ((lbItems.Items.count + 1) as String)
					lbItemsEntry.SubItems.Add tempStringArray.name
					lbItemsEntry.SubItems.Add tempStringArray.external
					lbItemsEntry.SubItems.Add tempStringArray.model
					lbItemsEntry.SubItems.Add tempStringArray.reference
					lbItemsEntry.SubItems.Add tempStringArray.parent
					lbItemsEntry.SubItems.Add tempStringArray.rotation
					lbItemsEntry.SubItems.Add tempStringArray.position
					
					updateExtent()
					sortList()
				)
			)
			
			
			
			on btnSave pressed do
			(
				local tempArray = #()
				local tempList = #()
				
				for i = 0 to (lbItems.Items.count - 1) do
				(
					local tempItemArray = #()
					local tempItem = lbItems.Items.item i
					for d = 1 to (tempItem.SubItems.count - 1) do
					(
						if (tempItem.SubItems.item d).text != "" then tempItemArray[d] = (tempItem.SubItems.item d).text else tempItemArray[d] = " "
					)
				
					append tempArray (cryMaxToolsAnimItemStruct name:tempItemArray[1] external:tempItemArray[2] model:(filterString tempItemArray[3] ";") reference:(filterString tempItemArray[4] ";") parent:(filterString tempItemArray[5] ";") rotation:(filterString tempItemArray[6] ";") position:(filterString tempItemArray[7] ";"))
					
					for f = 1 to tempArray[tempArray.count].model.count do
					(
						tempArray[tempArray.count].rotation[f] = execute (tempArray[tempArray.count].rotation[f])
						tempArray[tempArray.count].position[f] = execute (tempArray[tempArray.count].position[f])
					)
				)
				
				cryMaxTools.anim.vars.itemList = tempArray
				
				
				
				for i = 1 to cryMaxTools.anim.vars.itemList.count do
					append tempList cryMaxTools.anim.vars.itemList[i].name
				
				(cryMaxTools.basic.ROMan.get "settingsRO").ddItems.items = tempList
				
				cryMaxTools.basic.ROMan.remove "editItemListRO" "" #main
			)
			
			on btnDelete pressed do
			(
				if lbItems.FocusedItem != undefined then					
					lbItems.FocusedItem.remove()
				
				updateExtent()
				sortList()
			)
				
			on btnDeleteAll pressed do
			(
				lbItems.Items.clear()
			)
			
			
			on btnCancel pressed do
			(
				(cryMaxTools.basic.ROMan.get "settingsRO").ddItems.selection = (cryMaxTools.basic.ROMan.get "settingsRO").ddItems.items.count - 1
				cryMaxTools.basic.ROMan.remove "editItemListRO" "" #main
			)
			
			
			on editItemListRO close do
			(
				try
				(
					local tempVar = cryMaxTools.anim.functions.UI.items.getIndex()
					
					if tempVar != 0 then
						(cryMaxTools.basic.ROMan.get "settingsRO").ddItems.selection = tempVar
				)catch()
			)
		)
		
		cryMaxTools.basic.ROMan.cryAdd "editItemListRO" editItemListRO #main
		editItemListRO = undefined
		
		
		createDialog (cryMaxTools.basic.ROMan.get "editItemListRO") 443 220
	)
	
	
	
	
	on btnBoneExportEdit pressed do
	(
			--// rollout to edit the bone list entry
		rollout entryDetailsRO "Entry Details"
		(
			label labName "Name :" pos:[8,10]
			label labExternal "External :" pos:[8,30]
			label labBones "Bones :" pos:[8,50]
		
				--// sets the name, file detection and bones column
			edittext edName "" text:((cryMaxTools.basic.ROMan.get "editBoneListRO").lbList.FocusedItem.SubItems.item 0).text pos:[70,10] fieldWidth:300
			edittext edExternal "" text:((cryMaxTools.basic.ROMan.get "editBoneListRO").lbList.FocusedItem.SubItems.item 1).text pos:[70,30] fieldWidth:300
			edittext edBones "" text:((cryMaxTools.basic.ROMan.get "editBoneListRO").lbList.FocusedItem.SubItems.item 2).text pos:[70,50] fieldWidth:245
				--// button to pick specific bones in the scene
			button btnPickBones "Pick" pos:[323,50] height:17 width:50 toolTip:"Opens dialog to select the bone associated to the filename detection"
			
				--// save the entry or cancel
			button btnSave "Save" pos:[100,80] height:20 width:80 toolTip:"Saves filename/bone detection"
			button btnCancel "Cancel" pos:[200,80] height:20 width:80 toolTip:"Aborts filename/bone detection"
			
			
			on btnSave pressed do
			(
					--// get currently selected item
				local tempItem = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList.FocusedItem
				
					--// set new name
				(tempItem.SubItems.item 0).text = edName.text
					--// set new file detection
				(tempItem.SubItems.item 1).text = edExternal.text
					--// set new bone list
				(tempItem.SubItems.item 2).text = edBones.text
								
				local tempListArray = #()
				
				cryMaxTools.anim.functions.UI.items.updateExtent()
					--// kills bone edit dialog
				destroyDialog (cryMaxTools.basic.ROMan.get "entryDetailsRO")
			)
			
			
			on btnCancel pressed do
			(
					--// kills bone edit dialog
				destroyDialog (cryMaxTools.basic.ROMan.get "entryDetailsRO")
			)
			
			on btnPickBones pressed do
			(
				local tempString = ""
					--// pick node from the scene
				objArray = selectByName title:("Select Nodes") showHidden:true
				
					--// if a node is selected
				if objArray != undefined then	
				(
						--// goes through the nodes
					for i = 1 to objArray.count do
							--// adds the name of the node with a ";" as seperator
						tempString += objArray[i].name + (if i != objArray.count then ";" else "")
						
						--// set the new bone list
					edBones.text = tempString
				)

			)
				
		)
		
		cryMaxTools.basic.ROMan.cryAdd "entryDetailsRO" entryDetailsRO #main
		entryDetailsRO = undefined
		
		
		
		
		--// rollout to edit the bone detection list
		rollout editBoneListRO "Edit Bone List"
		(
				--// list of all bone setups
			--activeXControl lbList "MSComctlLib.ListViewCtrl" pos:[1,1] height:185 width:440
			dotNetControl lbList "System.Windows.Forms.ListView" pos:[1,1] height:185 width:440
				--// save current setup
			button btnSave "Save" pos:[8,195] height:20 width:80 toolTip:"Saves filename/bone detection"
			button btnAdd "Add" pos:[120,195] height:20 width:60 toolTip:"Adds new entry"
			button btnDelete "Delete" pos:[190,195] height:20 width:60 toolTip:"Deletes selected entry"
			button btnDeleteAll "Delete All" pos:[260,195] height:20 width:60 toolTip:"Clears whole list"
			button btnCancel "Cancel" pos:[350,195] height:20 width:80 toolTip:"Abort filename/bone detection"
			
			
			on editBoneListRO open do
			(
				lbList.GridLines = true
				lbList.AllowColumnReorder = true
				lbList.View = lbList.View.Details
				lbList.LabelEdit = false
				lbList.LabelWrap = true
				lbList.MultiSelect = true
				lbList.FullRowSelect = true
				lbList.HideSelection = false
				lbList.CheckBoxes = true
					--// adds columns
				lbList.Columns.Add "Name"
				lbList.Columns.Add "External"
				lbList.Columns.Add "Bones"

					--// goes through the bone list
				for i = 1 to cryMaxTools.anim.vars.boneList.count do
				(
						--// adds a new entry
					local lbListEntry = lbList.Items.Add cryMaxTools.anim.vars.boneList[i].name cryMaxTools.anim.vars.boneList[i].name
					
						--// if entry is active
					if cryMaxTools.anim.vars.boneList[i].active == "true" then
							--// set list entry active
						lbListEntry.checked = true
						--// if entry is not active
					if cryMaxTools.anim.vars.boneList[i].active == "false" then
							--// set list entry not active
						lbListEntry.checked = false
						
						--// add external entry
					lbListEntry.SubItems.Add cryMaxTools.anim.vars.boneList[i].external
						--// add bone entry
					lbListEntry.SubItems.Add cryMaxTools.anim.vars.boneList[i].bones
				)
				
				cryMaxTools.anim.functions.UI.items.updateExtent()
			)
			
			
			
			on lbList DoubleClick do
			(					
				tempItem = lbList.FocusedItem
				tempItem.checked = not tempItem.checked
					--// if an item is selected
				if tempItem != undefined then
				(
						--// creates bone entry edit
					createDialog (cryMaxTools.basic.ROMan.get "entryDetailsRO") 385 110
					
				)
			)
			
			on btnAdd pressed do
			(
				local tempStringArray = (boneStruct name:"" external:"" bones:"")
				local tempArray = #()
				local lbList = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList
				
					--// get nodes to be added
				local objArray = selectByName title:("Select Nodes to be added") showHidden:true
				
					--// if a node is selected
				if objArray != undefined then
				(
						--// goes through node list
					for obj in objArray do
							--// add the nodes via boneStruct to a seperate list
						append tempArray (boneStruct name:obj.name external:obj.name bones:obj.name)
					
						--// goes through the node list
					for i = 1 to tempArray.count do
					(
							--// adds name(s) of the node(s)
						tempStringArray.name += tempArray[i].name + (if i != tempArray.count then ";" else "")
							--// adds file detection(s)
						tempStringArray.external += tempArray[i].external + (if i != tempArray.count then ";" else "")
							--// adds bone(s) list
						tempStringArray.bones += tempArray[i].bones + (if i != tempArray.count then ";" else "")
					)
					
						--// adds entry to the bone list
					local lbListEntry = lbList.Items.Add tempStringArray.name
					lbListEntry.checked = true
					lbListEntry.SubItems.Add tempStringArray.external
					lbListEntry.SubItems.Add tempStringArray.bones
				)
			)
			
			
			on btnSave pressed do
			(
				local boneChanged = false
				local tempArray = #()
				local tempList = #()
				local lbList = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList
				
					--// goes through the bone list
				for i = 0 to (lbList.Items.count - 1) do
				(								
					local tempItemArray = #()
					local tempItem = lbList.Items.item i
					
						--// save checked state
					tempItemArray[1] = tempItem.checked as String
						--// if nothing is typed in, the entry will have " " to filter correctly
					if (tempItem.SubItems.item 0).text != "" then tempItemArray[2] = (tempItem.SubItems.item 0).text else tempItemArray[2] = " "
					if (tempItem.SubItems.item 1).text != "" then tempItemArray[3] = (tempItem.SubItems.item 1).text else tempItemArray[3] = " "
					if (tempItem.SubItems.item 2).text != "" then tempItemArray[4] = (tempItem.SubItems.item 2).text else tempItemArray[4] = " "
				
						--// adds the entries to a seperate list
					append tempArray (boneStruct active:tempItemArray[1] name:tempItemArray[2] external:tempItemArray[3] bones:tempItemArray[4])
				)
				
				for i = 1 to tempArray.count do
				(
					try
					(
						if tempArray[i] != cryMaxTools.anim.vars.boneList[i] then
							boneChanged = true
					)catch()
				)
				
					--// sets new bone list					
				cryMaxTools.anim.vars.boneList = tempArray
				
				local stringArray = #("Use Root Bone")
				
				if tempArray.count > 0 then
				(
					append stringArray "------------------------"
					
					for i = 1 to tempArray.count do
						if tempArray[i].active == "true" then
							append stringArray tempArray[i].name
				)
				
				local tempSafe = (cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.items[(cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.selection]

				(cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.items = stringArray
				
				local newString = (cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.items[(cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.selection]
				
				if tempSafe != newString then
					(cryMaxTools.basic.ROMan.get "settingsRO").ddBoneExport.selection = 1
			
					--// kills bone list edit dialog
				destroyDialog (cryMaxTools.basic.ROMan.get "editBoneListRO")

				
			)
			
			on btnDelete pressed do
			(
				local lbList = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList
				
				if lbList.FocusedItem != undefined then
					lbList.FocusedItem.remove()					
			)
				
			on btnDeleteAll pressed do
			(
				local lbList = (cryMaxTools.basic.ROMan.get "editBoneListRO").lbList
					--// clears whole list
				lbList.Items.clear()
			)
			
			
			on btnCancel pressed do
			(
					--// kills bone list edit dialog
				destroyDialog (cryMaxTools.basic.ROMan.get "editBoneListRO")
			)
		)
		
		cryMaxTools.basic.ROMan.cryAdd "editBoneListRO" editBoneListRO #main
		editBoneListRO = undefined
		
			--// creates bone list edit dialog	
		createDialog (cryMaxTools.basic.ROMan.get "editBoneListRO") 443 220
	)
	
	
	on ddBoneExport selected value do
	(
		if ddBoneExport.selection == 2 then
			ddBoneExport.selection = 1
	)
	
	
	
	
	
	on btnBasePath pressed do
	(
		if (local basePath = getSavePath caption:"Select Base Animation Folder" initialDir:cryMaxTools.anim.vars.basePath) != undefined then
		(
			cryMaxTools.anim.vars.basePath = basePath + "\\"
			edBasePath.text = basePath
			
			try
			(
				cryMaxTools.anim.loadSave.settingsChange = true
				(cryMaxTools.basic.ROMan.get "loadSaveRO").updateList #character
				try (cryMaxTools.basic.ROMan.get "loadSaveCharacterRO").cbCharacterList.selection = 1 catch()
				cryMaxTools.anim.loadSave.settingsChange = undefined
			)catch()
		)
	)
	
	
	on edBasePath entered value do
	(
		cryMaxTools.anim.vars.basePath = value
		if value[value.count] != "\\" then
			cryMaxTools.anim.vars.basePath += "\\"
		
		(cryMaxTools.basic.ROMan.get "loadSaveRO").updateList #character
	)
	
	
	
	
	on btnExportPath pressed do
	(
		if (local exportPath = getSavePath caption:"Select Export Folder" initialDir:cryMaxTools.anim.vars.exportPath) != undefined then
		(
			cryMaxTools.anim.vars.exportPath = exportPath + "\\"
			edExportPath.text = exportPath
		)
	)
	
	
	
	on edExportPath entered value do
	(
		cryMaxTools.anim.vars.exportPath = value
		if value[value.count] != "\\" then
			cryMaxTools.anim.vars.exportPath += "\\"
		--(cryMaxTools.basic.ROMan.get "loadSaveRO").updateList #character
	)
	
	
	
)

cryMaxTools.basic.ROMan.cryAdd "settingsRO" settingsRO #main



createDialog (cryMaxTools.basic.ROMan.get "settingsRO") 264 (303+50) -800 0