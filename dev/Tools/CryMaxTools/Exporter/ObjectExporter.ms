---------------------------
-- CryMaxTools v2.0
-- Object Exporter v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryExportObject "CryEngine2 Object Export" width:190 height:492
(
	button btnObjectAddSelected "Add Selected" offset:[-12,0] width:86 height:16 align:#left across:2 toolTip:"Creates a new item in the list and assigns the selected scene objects to it."
	button btnObjectRemove "Remove" offset:[10,0] width:86 height:16 align:#right enabled:false toolTip:"Removes the select items from the list."
	button btnObjectRefreshList "Refresh" offset:[-12,-5] width:86 height:16 align:#left across:2 toolTip:"Syncs the list with the scene"
	button btnObjectClearList "Clear List" offset:[5,-5] width:86 height:16 align:#center enabled:false toolTip:"Removes all items from the list."
	multiListBox mlbxObjectNodeList "" offset:[-1,-5] width:172 height:10 align:#center toolTip:"Double-clicking an item will select the associated objects in the scene."
	
	editText edtObjectRenameName "" offset:[-15,3] fieldWidth:126 height:16 align:#left across:2 enabled:false
	button btnObjectRename "Rename" offset:[10,3] width:46 height:17 align:#right enabled:false 
	dropDownList ddlObjectFileType "" items:#("Geometry (*.cgf)", "Character (*.chr)", "Animated Geometry (*.cga)", "Animation for cga (*.anm)") offset:[0,-5] width:172 align:#center enabled:false 
	
	button btnObjectAppendSelected "Append" offset:[-12,3] width:90 height:16 align:#left across:2 enabled:false toolTip:"Assigns the selected scene objects to the selected item in the list."
	button btnObjectDetachSelected "Detach" offset:[10,3] width:82 height:16 align:#right enabled:false toolTip:"Unlinks the selected scene objects from the selected item in the list."
	
	group "Advanced Options"
	(
		checkBox chkFlagMerge "Merge Geometry" align:#left offset:[-4,-5] toolTip:"When this flag is set, all assigned objects will be merged during export, reducing the drawcall count for the object."
		checkBox chkFlagVertexColors "Store Vertex Colors" align:#left offset:[-4,-5] toolTip:"When this flag is set, the vertex colors of all assigned objects will be exported to the game, increasing the memory consumption."
		spinner spnObjectMorphTargetThreshold "Morph Min Offset:" align:#left offset:[-4,-5] width:160 height:16 range:[0,100,.01] scale:0.001 type:#worldunits offset:[-5,-2] enabled:false toolTip:"This value defines the minimal distance that a vertex in a morph target must have to the same vertex in the base mesh before it gets exported.\nHigher values reduce memory consumption, but may decrease quality." --range/scale values in centimeters
	)
	
	button btnObjectExportTest "Test Export Selected" offset:[-1,3] width:172 height:16 align:#center enabled:false toolTip:"Creates a temporary COLLADA file, containing only the selected items from the list.\nThis may be quicker and desirable for testing purposes."
	button btnObjectExport "E X P O R T   A L L !" offset:[-1,-5] width:172 height:24 align:#center toolTip:"Creates a COLLADA file containing all the items from list."
	button btnObjectExportPreview "Preview" offset:[-12,-5] width:86 height:16 align:#left across:2 enabled:false
	button btnObjectExplore "Explore..." offset:[10,-5] width:86 height:16 align:#right toolTip:"Reveals the export destination in the explorer."
	
--########################################################################################
--OBJECT EXPORTER GLOBAL VARIABLES
--########################################################################################
	
--########################################################################################
--OBJECT EXPORTER GLOBAL FUNCTIONS
--########################################################################################
	fn fnUpdateListItemProperties =
	(--updates rename field, filetype list, flags and advanced options for selected items in multilistbox
		local aSelection = mlbxObjectNodeList.selection
		
		case aSelection.numberSet of
		(
			0: 
			(--NO ITEM IN THE LIST/NO ITEM SELECTED
				btnObjectRemove.enabled = false
				btnObjectClearList.enabled = mlbxObjectNodeList.items.count > 0 --list can only be cleared when it contains items
				
				edtObjectRenameName.text = ""
				edtObjectRenameName.enabled = false
				btnObjectRename.enabled = false
				
				ddlObjectFileType.selection = 0
				ddlObjectFileType.enabled = false
				
				btnObjectAppendSelected.enabled = false
				btnObjectDetachSelected.enabled = false
				
				chkFlagMerge.state = false
				chkFlagMerge.enabled = false
				chkFlagVertexColors.state = false
				chkFlagVertexColors.enabled = false
				
				spnObjectMorphTargetThreshold.value = 0
				spnObjectMorphTargetThreshold.enabled = false
				spnObjectMorphTargetThreshold.indeterminate = true
			)
			1:
			(--ONE ITEM SELECTED
				local iIndex = (aSelection as array)[1]
				
				btnObjectRemove.enabled = true
				btnObjectClearList.enabled = true
				
				edtObjectRenameName.text = cryMaxTools.export.object.fnGetExportFileName iIndex
				edtObjectRenameName.enabled = true
				btnObjectRename.enabled = true
				
				ddlObjectFileType.selection = case (cryMaxTools.export.object.fnGetExportFileType iIndex) of
				(
					"cgf": 1
					"chr": 2
					"cga": 3
					"anm": 4
					default: 0
				)
				ddlObjectFileType.enabled = true
				
				btnObjectAppendSelected.enabled = true
				btnObjectDetachSelected.enabled = true
				
				chkFlagMerge.triState = if (cryMaxTools.export.object.fnGetDoNotMergeFlag iIndex) then 0 else 1 --set triState - 0=unchecked, 1=checked - doNotMerge is shown as Merge Geometry, so shown state needs to be inverted
				chkFlagMerge.enabled = cryMaxTools.export.object.fnGetDoNotMergeAllowed iIndex
				chkFlagVertexColors.triState = if (cryMaxTools.export.object.fnGetStoreVertexColorsFlag iIndex) then 1 else 0 --set triState - 0=unchecked, 1=checked
				chkFlagVertexColors.enabled = true
				
				spnObjectMorphTargetThreshold.enabled = cryMaxTools.export.object.fnDoesNodeHaveMorphs iIndex --check if morphs are present
				if spnObjectMorphTargetThreshold.enabled then
				(
					spnObjectMorphTargetThreshold.value = cryMaxTools.export.object.fnGetMorphThreshold iIndex
				)
				else
				(
					spnObjectMorphTargetThreshold.value = 0
				)
				spnObjectMorphTargetThreshold.indeterminate = not spnObjectMorphTargetThreshold.enabled --hide value
			)
			default:
			(--MULTIPLE ITEMS SELECTED - SHOW MATCHING SETTINGS. HIDE NON-MATCHING
				btnObjectRemove.enabled = true
				btnObjectClearList.enabled = true
				
				edtObjectRenameName.text = ""
				edtObjectRenameName.enabled = true
				btnObjectRename.enabled = true
				
				--determin and set filetype
				local aFileTypesOfSelectedNodes = #()
				for i in (aSelection as array) do
				(--collect used filetypes of selected nodes
					appendIfUnique aFileTypesOfSelectedNodes (cryMaxTools.export.object.fnGetExportFileType i)
				)
				if aFileTypesOfSelectedNodes.count == 1 then
				(--if same type used everywhere, then show it
					ddlObjectFileType.selection = case aFileTypesOfSelectedNodes[1] of
					(
						"cgf": 1
						"chr": 2
						"cga": 3
						"anm": 4
						default: 0
					)
				)
				else
				(--else show none
					ddlObjectFileType.selection = 1
				)
				ddlObjectFileType.enabled = true
				
				btnObjectAppendSelected.enabled = false
				btnObjectDetachSelected.enabled = false
				
				--determin and set doNotMerge checkbox
				local aDoNotMergePermissionOfSelectedNodes = #()
				for i in (aSelection as array) do
				(--collect whether selected nodes may have doNotMerge flag
					appendIfUnique aDoNotMergePermissionOfSelectedNodes (cryMaxTools.export.object.fnGetDoNotMergeAllowed i)
				)
				if (findItem aDoNotMergePermissionOfSelectedNodes true) != 0 then
				(--only enable checkbox if some node may have doNotMerge flag
					chkFlagMerge.enabled = true
					
					local aDoNotMergeFlagsOfSelectedNodes = #()
					for i in (aSelection as array) do
					(--collect doNotMerge flag states of selected nodes
						appendIfUnique aDoNotMergeFlagsOfSelectedNodes (cryMaxTools.export.object.fnGetDoNotMergeFlag i)
					)
					if aDoNotMergeFlagsOfSelectedNodes.count == 1 then
					(--if same state everywhere then show it
						chkFlagMerge.triState = if aDoNotMergeFlagsOfSelectedNodes[1] then 0 else 1 --set triState - 0=unchecked, 1=checked
					)
					else
					(--else show indeterminate state
						chkFlagMerge.triState = 2 --set triState - 2=indeterminate
					)
				)
				else
				(--no doNotMerge allowed, then disable
					chkFlagMerge.enabled = false
					chkFlagMerge.state = true
				)
				
				--determin and set storeVertexColors checkbox
				chkFlagVertexColors.enabled = true
				local aStoreVertexColorsFlagsOfSelectedNodes = #()
				for i in (aSelection as array) do
				(--collect storeVertexColors flag states of selected nodes
					appendIfUnique aStoreVertexColorsFlagsOfSelectedNodes (cryMaxTools.export.object.fnGetStoreVertexColorsFlag i)
				)
				if aStoreVertexColorsFlagsOfSelectedNodes.count == 1 then
				(--if same state everywhere then show it
					chkFlagVertexColors.triState = if aStoreVertexColorsFlagsOfSelectedNodes[1] then 1 else 0 --set triState - 0=unchecked, 1=checked
				)
				else
				(--else show indeterminate state
					chkFlagVertexColors.triState = 2 --set triState - 2=indeterminate
				)
				
				--determin and set morph threshold checkbox
				local aMorphModifiersOfSelectedNodes = #()
				for i in (aSelection as array) do
				(--collect whether selected nodes have morph modifiers
					appendIfUnique aMorphModifiersOfSelectedNodes (cryMaxTools.export.object.fnDoesNodeHaveMorphs i)
				)
				if (findItem aMorphModifiersOfSelectedNodes true) != 0 then
				(--only enable spinner if some node has morph modifier
					spnObjectMorphTargetThreshold.enabled = true
					
					local aMergeThresholdsOfSelectedNodes = #()
					for i in (aSelection as array) do
					(--collect merge thresholds of selected nodes
						appendIfUnique aMergeThresholdsOfSelectedNodes (cryMaxTools.export.object.fnGetMorphThreshold i)
					)
					if aMergeThresholdsOfSelectedNodes.count == 1 then
					(--if same treshold everywhere then show it
						spnObjectMorphTargetThreshold.value = aMergeThresholdsOfSelectedNodes[1]
					)
					else
					(--else show indeterminate state
						spnObjectMorphTargetThreshold.value = 0
						spnObjectMorphTargetThreshold.indeterminate = true --hide value
					)
				)
				else
				(--no morph mod, then hide value and disable
					spnObjectMorphTargetThreshold.value = 0
					spnObjectMorphTargetThreshold.indeterminate = true --hide value
					spnObjectMorphTargetThreshold.enabled = false
				)
			)
		)
	)
	
	fn fnUpdateExportNodeList =
	(--puts nodes in global export node array into listbox - will be called in a post-file-open callback!
		local aOldSelection = (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.selection --store selection to reselect it afterwards
		local aOldSelectedNames = for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.items.count where aOldSelection[i] collect (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.items[i] --collect names - when items were removed, indices of items in list shift - restore selection by name then
		
		cryMaxTools.export.object.fnUpdate()
		
		local aItems = #()
		for i=1 to crymaxtools.export.object.aExportNodes.count do
		(--generate list item names from cryExportNode names and number of assigned objects
			append aItems ((cryMaxTools.export.object.fnGetExportFileName i) + "." + (cryMaxTools.export.object.fnGetExportFileType i) + " (" + (cryMaxTools.export.object.aExportNodes[i].children.count as string) + " obj)")
		)
		(cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.items = aItems --put item names to list
		
		if aItems.count >= aOldSelection.count then
		(--if item count matches or grows, restore old selection by index
			(cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.selection = aOldSelection
		)
		else
		(--if item count shrinks, restore old selection by name
			local aNewSelection = #{}
			
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.items.count do
			(--find items in new list that were selected before
				aNewSelection[i] = (findItem aOldSelectedNames (cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.items[i]) != 0
			)
			
			(cryMaxTools.basic.ROMan.get "rltCryExportObject").mlbxObjectNodeList.selection = aNewSelection
		)
		
		(cryMaxTools.basic.ROMan.get "rltCryExportObject").fnUpdateListItemProperties()
	)
	
--########################################################################################
--OBJECT EXPORTER UI EVENT HANDLERS
--########################################################################################
	on rltCryExportObject open do
	(--initialize
		fnUpdateExportNodeList()
	)
	
	on rltCryExportObject close do
	(--remove callbacks
		callbacks.removeScripts id:#updateExportNodeList
	)
	
--### NODE LIST MODIFICATION BUTTONS BEGIN ###
	on mlbxObjectNodeList selectionEnd do
	(--changing selection of the list updates property fields
		fnUpdateListItemProperties()
	)
	
	on mlbxObjectNodeList doubleclicked iIndex do
	(--doubleclicking an item in the list selects the childnodes of the exportnode
		if IsValidNode cryMaxTools.export.object.aExportNodes[iIndex] then
		(--only export if node exists
			select cryMaxTools.export.object.aExportNodes[iIndex].children
		)
		else
		(--if node not existent, update list
			fnUpdateExportNodeList()
		)
	)
	
	/*
	on pbtnObjectPick picked sMyObject do
	(--adds exportnode of picked object to listbox or creates node if not present
		iNewIndex = mlbxObjectNodeList.items.count + 1 --index of newly added item
		sNewNode = cryMaxTools.export.object.fnSetExportNode sMyObject
		
		fnUpdateExportNodeList()
		
		if sNewNode != undefined then
		(--select new item in list and update
			mlbxObjectNodeList.selection = #{iNewIndex}
			fnUpdateListItemProperties()
		)
	)
	*/
	
	on btnObjectAddSelected pressed do
	(--adds exportnode of selected objects to listbox or creates node if not present
		iNewIndex = mlbxObjectNodeList.items.count + 1 --index of newly added item
		
		local sNewNode = cryMaxTools.export.object.fnSetExportNodeForMultipleNodes $selection
		
		fnUpdateExportNodeList()
		
		if sNewNode != undefined then
		(--select new item in list and update
			mlbxObjectNodeList.selection = #{iNewIndex}
			fnUpdateListItemProperties()
		)
	)
	
	on btnObjectRefreshList pressed do
	(--refresh listbox
		fnUpdateExportNodeList()
	)
	
	on btnObjectClearList pressed do
	(--remove all exportnodes
		while cryMaxTools.export.object.aExportNodes.count > 0 do
		(
			cryMaxTools.export.object.fnRemoveExportNode 1
		)
		
		fnUpdateExportNodeList()
	)
	
	on btnObjectRemove pressed do
	(--removes selected listbox item and according exportnode
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple remove
			cryMaxTools.export.object.fnRemoveExportNode (mlbxObjectNodeList.selection as array)[1]
		)
		else if mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - remove one by one
			cryMaxTools.export.object.fnRemoveMultipleExportNodes mlbxObjectNodeList.selection
		)
		
		fnUpdateExportNodeList()
	)
--### NODE LIST MODIFICATION BUTTONS END ###
	
	
--### CRYEXPORT NODE MODIFICATION BUTTONS BEGIN ###
	on edtObjectRenameName entered sNewName do
	(--changes names of selected exportnodes and updates list
		if sNewName != "" and mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple rename
			cryMaxTools.export.object.fnSetExportFileName (mlbxObjectNodeList.selection as array)[1] sNewName
		)
		else if sNewName != "" and mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - rename one by one - auto rename function will be triggered
			for each in mlbxObjectNodeList.selection do
			(
				cryMaxTools.export.object.fnSetExportFileName each sNewName
			)
		)
		
		fnUpdateExportNodeList()
	)
	
	on btnObjectRename pressed do
	(--changes names of selected exportnodes and updates list
		local sNewName = edtObjectRenameName.text
		
		if sNewName != "" and mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple rename
			cryMaxTools.export.object.fnSetExportFileName (mlbxObjectNodeList.selection as array)[1] sNewName
		)
		else if sNewName != "" and mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - rename one by one - auto rename function will be triggered
			for each in mlbxObjectNodeList.selection do
			(
				cryMaxTools.export.object.fnSetExportFileName each sNewName
			)
		)
		
		fnUpdateExportNodeList()
	)
	
	on ddlObjectFileType selected iIndex do
	(--changes filetype of exportnode and updates list
		local sNewFileType = case iIndex of
		(
			1: "cgf"
			2: "chr"
			3: "cga"
			4: "anm"
			default: ""
		)
		
		if sNewFileType != "" and mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple filetype change
			cryMaxTools.export.object.fnSetExportFileType (mlbxObjectNodeList.selection as array)[1] sNewFileType
		)
		else if sNewFileType != "" and mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - change filetype one by one
			for each in mlbxObjectNodeList.selection do
			(
				cryMaxTools.export.object.fnSetExportFileType each sNewFileType
			)
		)
		
		fnUpdateExportNodeList()
	)
	
	on btnObjectAppendSelected pressed do
	(--adds selected objects to selected cryExportNode
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(
			iNewIndex = mlbxObjectNodeList.items.count + 1 --index of newly added item
			
			local sNewNode = cryMaxTools.export.object.fnSetExportNodeForMultipleNodes $selection iIndex:(mlbxObjectNodeList.selection as array)[1]
			
			fnUpdateExportNodeList()
			
			if sNewNode != undefined then
			(--select new item in list and update
				mlbxObjectNodeList.selection = #{iNewIndex}
				fnUpdateListItemProperties()
			)
		)
	)
	
	on btnObjectDetachSelected pressed do
	(--detaches selected objects from selected cryExportNode
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(
			iNewIndex = mlbxObjectNodeList.items.count + 1 --index of newly added item
			
			local sNewNode = cryMaxTools.export.object.fnDetachNodesFromExportNode (mlbxObjectNodeList.selection as array)[1] $selection
			
			fnUpdateExportNodeList()
		)
	)
	
	on chkFlagMerge changed state do
	(--changes doNotMerge flags state
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple flag change
			cryMaxTools.export.object.fnSetDoNotMergeFlag (mlbxObjectNodeList.selection as array)[1] (not state) --checkbox is on when merging happens, off otherwise - flag is opposite
		)
		else if mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - change flag one by one
			local aDoNotMergePermissionOfSelectedNodes = #()
			for i in (mlbxObjectNodeList.selection as array) do
			(--collect whether selected nodes may have doNotMerge flag
				appendIfUnique aDoNotMergePermissionOfSelectedNodes (cryMaxTools.export.object.fnGetDoNotMergeAllowed i)
			)
			if (findItem aDoNotMergePermissionOfSelectedNodes false) != 0 then
			(--if at least one item does not have doNotMerge permission - then checkbox will set to indeterminate by update function, which cause false state on next click - this results in endless loop when item without doNotMerge permission is selected
				local aDoNotMergeFlagsOfSelectedNodes = #()
				for i in (mlbxObjectNodeList.selection as array) do
				(--collect doNotMerge flag states of selected nodes
					appendIfUnique aDoNotMergeFlagsOfSelectedNodes (cryMaxTools.export.object.fnGetDoNotMergeFlag i)
				)
				if aDoNotMergeFlagsOfSelectedNodes.count == 1 and aDoNotMergeFlagsOfSelectedNodes[1] == false then
				(--if doNotMerge flag is not set in any selected item - then try to set it
					chkFlagMerge.state = false --checkbox is on when merging happens, off otherwise - flag is opposite
					for each in mlbxObjectNodeList.selection do
					(
						cryMaxTools.export.object.fnSetDoNotMergeFlag each true --checkbox is on when merging happens, off otherwise - flag is opposite
					)
				)
				else
				(--if some selected nodes have doNotMergeFlag set - then try to set it to false
					chkFlagMerge.state = true --checkbox is on when merging happens, off otherwise - flag is opposite
					for each in mlbxObjectNodeList.selection do
					(
						cryMaxTools.export.object.fnSetDoNotMergeFlag each false --checkbox is on when merging happens, off otherwise - flag is opposite
					)
				)
			)
			else
			(--all items may have doNotMerge flag, then set it
				for each in mlbxObjectNodeList.selection do
				(
					cryMaxTools.export.object.fnSetDoNotMergeFlag each (not state) --checkbox is on when merging happens, off otherwise - flag is opposite
				)
			)
		)
		
		fnUpdateExportNodeList()
	)
	
	on chkFlagVertexColors changed state do
	(--changes storeVertexColors flags state
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple flag change
			cryMaxTools.export.object.fnSetStoreVertexColorsFlag (mlbxObjectNodeList.selection as array)[1] state
		)
		else if mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - change flag one by one
			for each in mlbxObjectNodeList.selection do
			(
				cryMaxTools.export.object.fnSetStoreVertexColorsFlag each state
			)
		)
		
		fnUpdateExportNodeList()
	)
	
	on spnObjectMorphTargetThreshold changed value do
	(--changes morphThreshold value
		if mlbxObjectNodeList.selection.numberSet == 1 then
		(--single item selected - simple UDP change
			cryMaxTools.export.object.fnSetMorphThreshold (mlbxObjectNodeList.selection as array)[1] value
		)
		else if mlbxObjectNodeList.selection.numberSet > 1 then
		(--multiple items selected - change UDP one by one
			for each in mlbxObjectNodeList.selection do
			(
				cryMaxTools.export.object.fnSetMorphThreshold each value
			)
		)
		
		fnUpdateExportNodeList()
	)
--### CRYEXPORT NODE MODIFICATION BUTTONS END ###
	
	
--### EXPORT BUTTONS BEGIN ###
	on btnObjectExportTest pressed do
	(--perform test export with only selected list items
		cryMaxTools.export.object.fnExportSelection mlbxObjectNodeList.selection
	)
	
	on btnObjectExport pressed do
	(--perform export
		cryMaxTools.export.object.fnExport()
	)
	
	on btnObjectExplore pressed do
	(--reveals the current scene filepath in a new explorer window or throws an error when not saved
		if maxFilePath == "" then messagebox "The file has not yet been saved!" title:"Error!"
		else shellLaunch "explorer.exe" maxFilePath
	)
	
	on btnObjectCheck pressed do
	(
		for each in cryMaxTools.export.object.aExportNodes do
		(
			local aVertexErrors = (cryMaxTools.export.object.fnCheckVertexDistance each)
			if aVertexErrors != true then
			(
				cryMaxTools.model.poly.fnSelectVertices aVertexErrors[1][1] aVertexErrors[1][2] bClearSelection:true
			)
			print ("Vertices: " + aVertexErrors as string)
			print ("Material: " + (cryMaxTools.export.object.fnCheckMaterialSetup each) as string)
			print ("Morphs: " + (cryMaxTools.export.object.fnCheckNonZeroMorphs each) as string)
		)
	)
	
	on btnObjectMaterialExport pressed do
	(--export material file
		cryMaxTools.export.object.fnExportMaterial()
	)
	
	on btnObjectMaterialImport pressed do
	(--import material file
		cryMaxTools.export.object.fnImportMaterial()
	)
--### EXPORT BUTTONS END ###
)

--add exporter rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryExportObject" rltCryExportObject #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExportObject")

--install callbacks to update list when file opened/merged/imported/created or max reseted
callbacks.addScript #filePostOpen "(cryMaxTools.basic.ROMan.get \"rltCryExportObject\").fnUpdateExportNodeList()" id:#updateExportNodeList
callbacks.addScript #filePostMerge "(cryMaxTools.basic.ROMan.get \"rltCryExportObject\").fnUpdateExportNodeList()" id:#updateExportNodeList
callbacks.addScript #postImport "(cryMaxTools.basic.ROMan.get \"rltCryExportObject\").fnUpdateExportNodeList()" id:#updateExportNodeList
callbacks.addScript #systemPostNew "(cryMaxTools.basic.ROMan.get \"rltCryExportObject\").fnUpdateExportNodeList()" id:#updateExportNodeList
callbacks.addScript #systemPostReset "(cryMaxTools.basic.ROMan.get \"rltCryExportObject\").fnUpdateExportNodeList()" id:#updateExportNodeList