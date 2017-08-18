-------------------------------------------
-- CryMaxTools v2.0
-- Exporter Tools
-- by Sascha Herfort
---------------------------

rollout rltCryExporterTools "Exporter Tools"
(
	group "Asset-Validation"
	(
		button btnCHRValidator "CHR-Validator" offset:[0,3] width:160 height:24 align:#center
		
	)
	
	
	group "Export without Phys Bones"
	(--for CRYEngine2 exporter
		label lblInfo1 "Set export filetype to CHR!" align:#left
		
		listBox lbxCHRsWithPhys "CHRs with Phys Bones" offset:[0,3] width:160 height:10 align:#center
		button btnAddWithPhys "Add Selected" offset:[-5,-5] width:80 height:16 align:#left across:2
		button btnClearWithPhys "Clear List" offset:[5,-5] width:80 height:16 align:#right enabled:true
		
		listBox lbxCHRsWithoutPhys "CHRs withOUT Phys Bones" offset:[0,8] width:160 height:10 align:#center
		button btnAddWithoutPhys "Add Selected" offset:[-5,-5] width:80 height:16 align:#left across:2
		button btnClearWithoutPhys "Clear List" offset:[5,-5] width:80 height:16 align:#right enabled:true
		
		button btnExportWithoutPhys "E X P O R T !" offset:[0,3] width:160 height:24 align:#center
	)
	
	--### GLOBAL VARIABLES BEGIN ###
	local aNodesToExportWithPhys = #()
	local aNodesToExportWithoutPhys = #()
	local bUseSphereToCleanMesh = true
	--### GLOBAL VARIABLES END ###
	
	--### FUNCTION DEFINITIONS BEGIN ###
	fn fnFindPhysBonesInScene =
	(--returns a list of bones/nodes that have a 'phys' node in the scene
		local aResult = #()
		
		for each in $objects do
		(
			local aNameFiltered = filterString each.name " "
			if toLower aNameFiltered[aNameFiltered.count] == "phys" then
			(
				append aResult each
			)
		)
		
		aResult
	)
	
	fn fnGetNodeHierarchyDepth nNode =
	(--returns the number of parent levels above the node - 0 = no parents
		local nCurrentNode = nNode.parent
		local iResult = 0
		
		while nCurrentNode != undefined do
		(
			nCurrentNode = nCurrentNode.parent
			iResult += 1
		)
		
		iResult
	)
	
	fn fnGetMinHierarchyDepthNode aNodes =
	(--returns the node inside the array with the smallest hierarchy depth
		local nResult = aNodes[1]
		for i = 2 to aNodes.count do
		(
			--print (aNodes[i].name + ": " + (fnGetNodeHierarchyDepth aNodes[i]) as string + " < " + nResult.name + ": " + (fnGetNodeHierarchyDepth nResult) as string + "?")
			if (fnGetNodeHierarchyDepth aNodes[i]) < (fnGetNodeHierarchyDepth nResult) then
			(
				--print (aNodes[i].name + ": " + (fnGetNodeHierarchyDepth aNodes[i]) + " < " + nResult.name + ": " + (fnGetNodeHierarchyDepth nResult) + "!")
				nResult = aNodes[i]
			)
		)
		
		nResult
	)
	
	fn fnSaveNodeLists =
	(--saves names of selected nodes into per-file dummy node
		local nExporterSettingsNode = cryMaxTools.export.fnGetExporterSettingsNode()
		
		local sNodesToExportWithPhys = ""
		for each in aNodesToExportWithPhys do
		(
			if IsValidNode each then
			(
				sNodesToExportWithPhys += each.name + ";"
			)
		)
		local sNodesToExportWithoutPhys = ""
		for each in aNodesToExportWithoutPhys do
		(
			if IsValidNode each then
			(
				sNodesToExportWithoutPhys += each.name + ";"
			)
		)
		
		setUserProp nExporterSettingsNode "aNodesToExportWithPhys" sNodesToExportWithPhys
		setUserProp nExporterSettingsNode "aNodesToExportWithoutPhys" sNodesToExportWithoutPhys
	)
	
	fn fnLoadNodeLists =
	(--loads names of selected nodes from per-file dummy node
		local nExporterSettingsNode = cryMaxTools.export.fnGetExporterSettingsNode()
		
		if (getUserProp nExporterSettingsNode "aNodesToExportWithPhys") != undefined then
		(
			local aNodeNamesWithPhys = filterString (getUserProp nExporterSettingsNode "aNodesToExportWithPhys") ";"
			aNodesToExportWithPhys = for each in aNodeNamesWithPhys where (getNodeByName each) != undefined collect (getNodeByName each)
		)
		if (getUserProp nExporterSettingsNode "aNodesToExportWithoutPhys") != undefined then
		(
			local aNodeNamesWithoutPhys = filterString (getUserProp nExporterSettingsNode "aNodesToExportWithoutPhys") ";"
			aNodesToExportWithoutPhys = for each in aNodeNamesWithoutPhys where (getNodeByName each) != undefined collect (getNodeByName each)
		)
	)
	
	fn fnUpdateListBoxes =
	(--updates the UI lists
		local aItems = #()
		for i=1 to aNodesToExportWithPhys.count do
		(--generate list item names from array node names
			append aItems aNodesToExportWithPhys[i].name
		)
		lbxCHRsWithPhys.items = aItems --put item names to list
		
		aItems = #()
		for i=1 to aNodesToExportWithoutPhys.count do
		(--generate list item names from array node names
			append aItems aNodesToExportWithoutPhys[i].name
		)
		lbxCHRsWithoutPhys.items = aItems --put item names to list
	)
	--### FUNCTION DEFINITIONS END ###
	
	--### UI CALLBACKS BEGIN ###
	on rltCryExporterTools open do
	(--load on open
		fnLoadNodeLists()
		fnUpdateListBoxes()
	)
	
	on rltCryExporterTools close do
	(--save on close
		fnSaveNodeLists()
	)
	
	on btnCHRValidator pressed do
	(
		fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\CHR_Validator.ms")
	)
	
	
	
	on btnAddWithPhys pressed do
	(--add selected nodes to export with phys bone list
		for each in $selection do
		(
			appendIfUnique aNodesToExportWithPhys each
		)
			
		for each in aNodesToExportWithPhys where findItem aNodesToExportWithoutPhys each != 0 do
		(
			deleteItem aNodesToExportWithoutPhys (findItem aNodesToExportWithoutPhys each)
		)
			
		fnSaveNodeLists()
		fnUpdateListBoxes()
	)
	
	on btnClearWithoutPhys pressed do
	(--clear without phys list
		aNodesToExportWithoutPhys = #()
		fnSaveNodeLists()
		fnUpdateListBoxes()
	)
	
	on btnAddWithoutPhys pressed do
	(--add selected nodes to export without phys bone list
		for each in $selection do
		(
			appendIfUnique aNodesToExportWithoutPhys each
		)
		
		for each in aNodesToExportWithoutPhys where findItem aNodesToExportWithPhys each != 0 do
		(
			deleteItem aNodesToExportWithPhys (findItem aNodesToExportWithPhys each)
		)
		
		fnSaveNodeLists()
		fnUpdateListBoxes()
	)
	
	on btnClearWithPhys pressed do
	(--clear with phys list
		aNodesToExportWithPhys = #()
		fnSaveNodeLists()
		fnUpdateListBoxes()
	)
	
	on btnExportWithoutPhys pressed do
	(-- perform export
		--get nodes to export from selection - physicalize only "Grunt_Base"
		if aNodesToExportWithPhys.count > 0 then
		(--export CHRs with phys bones first
			max utility mode
			UtilityPanel.OpenUtility CryEngine2_Exporter
			csExport.export.set_node_list aNodesToExportWithPhys
			csExport.export.export_nodes()
			for each in aNodesToExportWithPhys do
			(--process exported files with RC
				cryMaxTools.basic.scmd (cryMaxTools.basic.vars.buildPath + "\\bin32\\rc\\rc.exe \"" + maxFilePath + each.name + ".chr\"" + " /refresh") true
			)
		)
		
		if aNodesToExportWithoutPhys.count > 0 then
		(--export CHRs without phys bones second
			undo "Export without phys bones" on
			(
				local aPhysBones = fnFindPhysBonesInScene()
				local nKeepThisPhysBone = fnGetMinHierarchyDepthNode aPhysBones --this bone will keep its phys bone and get it's geometry cleared, because at least one must exist
				local nClearThisBoneMesh = getNodeByName (subString nKeepThisPhysBone.name 1 (nKeepThisPhysBone.name.count - 5)) ignoreCase:true
				deleteItem aPhysBones (findItem aPhysBones nKeepThisPhysBone) --remove nKeepThisPhysBone from phys bones to be disabled
				
				for each in aPhysBones do each.name = (subString each.name 1 (each.name.count - 4)) + "DISABLEDPHYS"
				
				local mClearModifier = undefined
				local sClearMeshUDPs = getUserPropBuffer nClearThisBoneMesh
				if bUseSphereToCleanMesh then
				(
					cryMaxTools.export.fnSetUPDFlag nClearThisBoneMesh "sphere" true
				)
				else
				(
					mClearModifier = edit_poly()
					mClearModifier.name = "clearMesh"
					addModifier nClearThisBoneMesh mClearModifier
					select #(nClearThisBoneMesh)
					max modify mode
					subObjectLevel = 1
					mClearModifier.SetSelection #Vertex #{1..nClearThisBoneMesh.verts.count}
					mClearModifier.buttonOp #DeleteVertex
					
					cryMaxTools.export.fnSetUPDFlag nClearThisBoneMesh "sphere" false
					cryMaxTools.export.fnSetUPDFlag nClearThisBoneMesh "box" false
					cryMaxTools.export.fnSetUPDFlag nClearThisBoneMesh "capsule" false
					cryMaxTools.export.fnSetUPDFlag nClearThisBoneMesh "cylinder" false
				)
				
				max utility mode
				UtilityPanel.OpenUtility CryEngine2_Exporter
				csExport.export.set_node_list aNodesToExportWithoutPhys
				csExport.export.export_nodes()
				for each in aNodesToExportWithoutPhys do
				(--process exported files with RC
					cryMaxTools.basic.scmd (cryMaxTools.basic.vars.buildPath + "\\bin32\\rc\\rc.exe \"" + maxFilePath + each.name + ".chr\"" + " /refresh") true
				)
				
				if not bUseSphereToCleanMesh then
				(
					deleteModifier nClearThisBoneMesh mClearModifier
					mClearModifier = undefined
				)
				setUserPropBuffer nClearThisBoneMesh sClearMeshUDPs
				for each in aPhysBones do each.name = (subString each.name 1 (each.name.count - 12)) + "phys"
			)
		)
	)
	--### UI CALLBACKS END ###
)

--add exporter tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryExporterTools" rltCryExporterTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryExporterTools")