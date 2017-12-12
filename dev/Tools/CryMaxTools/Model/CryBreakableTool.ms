
try(cbt.form.close()) catch()
global cbt = undefined
gc()

try(
	projectRoot = csexport.get_root_path()
	dotNet.loadAssembly (projectRoot + "/Tools/CryMaxTools/Model/CodersLab.Windows.Controls.TreeView.dll")
) catch (
	messageBox ("ERROR: Cannot find required library: " + projectRoot + "/Tools/CryMaxTools/Model/CodersLab.Windows.Controls.TreeView.dll")
	return()
)

function cryBreakablesRemoveCallbacks =
(
	try(
		callbacks.RemoveScripts #selectionSetChanged id:#callback_selection
		callbacks.RemoveScripts #selectedNodesPostDelete id:#callback_delete
		callbacks.RemoveScripts #nodeHide id:#callback_hide 
		callbacks.RemoveScripts #nodeUnhide id:#callback_unhide
		callbacks.RemoveScripts #sceneUndo id:#callback_undo
		callbacks.RemoveScripts #sceneRedo id:#callback_redo
		callbacks.RemoveScripts #nodeCreated id:#callback_nodeAdded
		callbacks.RemoveScripts #filePreOpen id:#callback_preOpen
		callbacks.RemoveScripts #filePostOpen id:#callback_postOpen
		
	) catch()
)
function cryBreak_objSelectionChanged =
(
	if cbt == undefined then
	(
		cryBreakablesRemoveCallbacks()
		return()
	)
	if cbt.tvSelectionHandled == false then
	(
		cbt.refreshTreeView()
		if cbt.bExpandTree then
			cbt.expandTreeView()
		cbt.updateDataGridData()
	)
)
function cryBreak_treeviewRedrawCallback =
(
	if cbt == undefined then
	(
		cryBreakablesRemoveCallbacks()
		return()
	)
	cbt.rebuildTreeView()
	cbt.refreshTreeView()
	cbt.expandTreeView()
	cbt.updateDataGridData()
)
function cryBreak_filePreOpen =
(
	try(
		cryBreakablesRemoveCallbacks()
	) catch()
)
function cryBreak_filePostOpen =
(
	try(
		cbt.rebuildTreeView()
		cbt.updateDataGridData()
		cryBreakablesAddCallbacks()
	) catch()
)
function cryBreakablesAddCallbacks =
(
	callbacks.addScript #selectionSetChanged "try(cryBreak_objSelectionChanged())catch()" id:#callback_selection 
	callbacks.addScript #selectedNodesPostDelete "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_delete 
	callbacks.addScript #nodeHide "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_hide 
	callbacks.addScript #nodeUnhide "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_unhide 
	callbacks.addScript #sceneUndo "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_undo 
	callbacks.addScript #sceneRedo "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_redo 
	callbacks.addScript #nodeCreated "try(cryBreak_treeviewRedrawCallback())catch()" id:#callback_nodeAdded 
	callbacks.addScript #filePreOpen "cryBreakablesRemoveCallbacks()" id:#callback_preOpen
	callbacks.addScript #filePostOpen "cryBreakablesAddCallbacks()" id:#callback_postOpen
)

struct objType_Joint
(
	labels = #("player_can_break", "Limit", "Bend", "Twist", "Pull", "Push", "Shift", "gameplay_critical"),
	udpKeys = #("player_can_break", "limit", "bend", "twist", "pull", "push", "shift", "gameplay_critical"),
	types = #(booleanClass, float, float, float, float, float, float, booleanClass)
)
struct objType_Mesh
(
	labels = #("Mass", "Density", "Box", "Capsule", "Cylinder", "Sphere", "NotAPrim", "Entity", "other_rendermesh", "no_hit_refinement", "no_explosion_occlusion", "colltype_player", "Dynamic"),
	udpKeys = #("mass", "density", "box", "capsule", "cylinder", "sphere", "notaprim", "entity", "other_rendermesh", "no_hit_refinement", "no_explosion_occlusion", "colltype_player", "dynamic"),
	types = #(float, float, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass, booleanClass)
)

struct cryBreakableTool
(
	dNColor = dotNetClass "System.Drawing.Color",
	dnKeys = dotNetClass "System.Windows.Forms.Keys",
	clrStyle = #(),

	refreshButImage = (dotNetClass "System.Drawing.Image").FromFile (cryMaxTools.basic.vars.toolsPath + "Icons\\cry_refresh.png"),
	hideButImage1 = (dotNetClass "System.Drawing.Image").FromFile (cryMaxTools.basic.vars.toolsPath + "Icons\\cry_hide.png"),
	hideButImage2 = (dotNetClass "System.Drawing.Image").FromFile (cryMaxTools.basic.vars.toolsPath + "Icons\\cry_unhide.png"),

	clrRowheadBack = dNColor.fromArgb 236 233 216,
	
	tvExpanding = false,
	tvLastTopNode = undefined,
	bMouseIsDown = false,
	bShowHidden = true,
	modify = undefined,
	matColumn = undefined,
	matEditHandled = false,
	tvSelectionHandled = false,
	lastVisibleTVCount = 0,
	pickedNode = undefined,
	pickedNodes = #(),
	isDragging = false,
	lastHoverNode = undefined,
	selectedNodes = #(),
	treeNodes = #(),
	bExpandTree = true,
	oldCellVal = undefined,
	oldCellVals = #(),
	udpCopy = "",
	rpScrollPos = dotNetClass "System.Drawing.Point",

	dgvNumRows = 12,
	dgvRowHeight = 19,

	w1 = 150,
	w2 = 200,

/*
	theFont = dotNetObject "System.Drawing.Font" "Verdana" 11 (dotNetClass "System.Drawing.FontStyle").Bold (dotNetClass "System.Drawing.GraphicsUnit").Pixel
dnTreeView.Font = theFont
	*/
	
	toolTip1 =  dotNetObject "System.Windows.Forms.Tooltip",
	maxHwnd = DotNetObject "MaxCustomControls.Win32HandleWrapper" (DotNetObject "System.IntPtr" (Windows.GetMAXHWND())),
	form = dotNetObject "maxCustomControls.maxform" name:"form" pos:[0,0],

	crySplit = dotNetObject "System.Windows.Forms.SplitContainer" name:"crySplit",
	tvObjList = dotNetObject "CodersLab.Windows.Controls.TreeView" name:"tvObjList",
	panLeftPanel = dotNetObject "System.Windows.Forms.Panel" name:"panLeftPanel",
	panRightPanel = dotNetObject "System.Windows.Forms.Panel" name:"panRightPanel",
	
	panLeftButPanel = dotNetObject "System.Windows.Forms.Panel" name:"panLeftButPanel",
	butTVRefresh = dotNetObject "System.Windows.Forms.Button" name:"butTVRefresh",
	butTVShowHidden = dotNetObject "System.Windows.Forms.Button" name:"butTVShowHidden",
		
	panLeftTVPanel = dotNetObject"System.Windows.Forms.Panel" name:"panLeftTVPanel",
		
	panRightButPanel = dotNetObject "System.Windows.Forms.Panel" name:"panRightButPanel",
	labMatId = dotNetObject "System.Windows.Forms.Label" name:"labMatId",
	tbMatId = dotNetObject "System.Windows.Forms.TextBox" name:"tbMatId",
	butMakePhysMeshes = dotNetObject "System.Windows.Forms.Button" name:"butMakePhysMeshes",
	butMakeJoints = dotNetObject "System.Windows.Forms.Button" name:"butMakeJoints",
	
	dgObjProps = dotNetObject "System.Windows.Forms.DataGridView" name:"dgObjProps",

	panUDPPanel = dotNetObject "System.Windows.Forms.Panel" name:"panUDPPanel",	
	labUDPLabel = dotNetObject "System.Windows.Forms.Label" name:"labUDPLabel",
	butUDPArrow = dotNetObject "System.Windows.Forms.Button" name:"butUDPArrow",
	tbUDPBox = dotNetObject "System.Windows.Forms.TextBox" name:"tbUDPBox",
	
	panMatPanel = dotNetObject "System.Windows.Forms.Panel" name:"panMatPanel",
	labMatLabel = dotNetObject "System.Windows.Forms.Label" name:"labMatLabel",
	dgMatList = dotNetObject "System.Windows.Forms.DataGridView" name:"dgMatList",
	
		
	/***********************************************************************************/
	/******************        M A T E R I A L L I S T   V I E W   F U N C T I O N S       ************/
		
	function makeMatListRow mat id =
	(
		local dgr = dotNetObject "System.Windows.Forms.DataGridViewRow"
		if mat==undefined and id==undefined then
		(

		) else (
			
			for c=0 to (dgMatList.ColumnCount-1) do
			(
				case c of
				(
					0: ( -- HIDE MAT BUTTON
							dgc = dotNetObject "System.Windows.Forms.DataGridViewButtonCell"	
							dgc.value = "H"
							dgc.tag = id
							dgr.Cells.Add dgc
						)
						
					1: ( -- SHOW MAT BUTTON
							dgc = dotNetObject "System.Windows.Forms.DataGridViewButtonCell"
							dgc.value = "S"
							dgc.tag = id
							dgr.Cells.Add dgc
							dgc.ReadOnly = true
						)
					
					2: ( -- SELECT MAT BUTTON
							if id > 0 then
							(
								dgc = dotNetObject "System.Windows.Forms.DataGridViewButtonCell"
								dgc.value = "E"
								dgc.tag = id
								dgr.Cells.Add dgc
							) else (
								dgc = dotNetObject "System.Windows.Forms.DataGridViewTextBoxCell"	
								dgc.value = ""
								dgc.tag = id
								dgr.Cells.Add dgc
								dgc.ReadOnly = true
							)
						)

					3: ( -- MAT ID #
							dgc = dotNetObject "System.Windows.Forms.DataGridViewTextBoxCell"	
							if id > 0 then
								dgc.value = id
							else dgc.value = ""
							dgc.tag = id
							dgr.Cells.Add dgc
							dgc.ReadOnly = true
						)

					4: ( -- MATERIAL NAME
							dgc = dotNetObject "System.Windows.Forms.DataGridViewTextBoxCell"	
							dgc.value = mat.name
							dgc.tag = dotNetMXSValue mat
							dgr.Cells.Add dgc
						)

					5: ( -- MATERIAL TYPE
							dgc = dotNetObject "System.Windows.Forms.DataGridViewTextBoxCell"	
							try(dgc.value = (mat.shaderByName as string)) catch(dgc.value = ((classOf mat) as string))
							--dgc.tag = dotNetMXSValue mat
							dgr.Cells.Add dgc
						)
						
					6: ( -- PHYSICALIZED
							dgc = dotNetObject "System.Windows.Forms.DataGridViewCheckBoxCell"	
							try(dgc.value = mat.physicalizeMaterial) catch(dgc.value = false)
							--dgc.tag = dotNetMXSValue mat
							dgr.Cells.Add dgc
						)
				)
			)
		)
		dgr.Height = dgvRowHeight
		return dgr
	),
	
	function makeMaterialList matList = 
	(
		dgMatList.Height = (matList.count + 1) * (dgvRowHeight + 1)
		panMatPanel.Height  = dgMatList.Height + 30
		for i=1 to matList.count do
		(
			dgMatList.Rows.Add (makeMatListRow matList[i] (i-1))
			if (classOf matList[i]) == Multimaterial then
				(dgMatList.Rows.item (dgMatList.Rows.count-1)).DefaultCellStyle = clrStyle[3]
		)
		dgMatList.CurrentCell = undefined
	),
	
	function updateMaterialList =
	(
		local matList = #()
		
		dgMatList.SuspendLayout()
		dgMatList.Rows.Clear()
		
		for i=0 to (tvObjList.SelectedNodes.count-1) do
		(
			local obj = (tvObjList.SelectedNodes.item i).tag.value
			if (isValidNode obj) then
			(
				local mat = obj.material
				if mat != undefined then
				(
					appendIfUnique matList mat
					if (classOf mat) == Multimaterial then
					(
						for j=1 to mat.numsubs do
						(
							appendIfUnique matList mat.materialList[j]
						)
					)
				)
			)
		)
		makeMaterialList matList
		
		dgMatList.ResumeLayout()
	),
	
	/*********************************************************************************************/
	/******************        D A T A G R I D   V I E W    U D P   F U N C T I O N S       ************/
	
	function getUDPValues type sUdp = 
	(
		local retKeys = #()
		local retVals = #()
		
		aUdp = filterString (toLower sUdp) "\r\n" splitEmptyTokens:false

		for i=1 to aUdp.count do
		(
			udpTokens = #()
			udpTokens = filterString aUdp[i] "=" splitEmptyTokens:false
			for j=1 to udpTokens.count do
			(
				udpTokens[j] = trimRight (trimLeft udpTokens[j])
			)
			if udpTokens.count > 0 then
			(
				append retKeys udpTokens[1]
				if udpTokens.count == 2 then
				(
					append retVals udpTokens[2]
				) else (
					append retVals true
				)
			)
		)
				
		return #(retKeys,retVals)
	),

	function getObjTypeString obj = 
	(
		local retvalue = ""
		if obj != undefined then
		(
			retvalue = (superClassOf obj) as string
			if (findString obj.name "$joint") == 1 then
				retvalue = "Joint Helper"
			if (findString (toLower obj.name) "$lod") == 1 then
				retvalue = "LOD Node"
			if retvalue == "GeometryClass" then
				retvalue = "Mesh Object"
			if retvalue == "helper" then
				retvalue = "Dummy"
		)
		return retvalue
	),

	function getObjectType obj =
	(
		local retvalue = objType_Mesh()
		
		if (findString obj.name "$joint") == 1 then
			retvalue = objType_Joint()

		return retvalue
	),
	
	function makeDataGridRow type val =
	(
		local dgr = dotNetObject "System.Windows.Forms.DataGridViewRow"
		for c=0 to (dgObjProps.ColumnCount-1) do
		(
			local dgc = dotNetObject "System.Windows.Forms.DataGridViewTextBoxCell"

			if c > 0 then
			(
				dgc.Style = clrStyle[1]
				if type == booleanClass then
				(
					dgc = undefined
					dgc = dotNetObject "System.Windows.Forms.DataGridViewCheckBoxCell"
				)
			)
			dgc.Value = val[c+1]
			dgr.Cells.Add dgc
		)
		dgr.Height = dgvRowHeight
		return dgr
	),
	
	function makeDataGridView type =
	(
		dgObjProps.Rows.Add (makeDataGridRow string #("Object:",""))
		dgObjProps.Rows.Add (makeDataGridRow string #("Type:",""))
		dgvNumRows = type.labels.count+1
		for r=2 to dgvNumRows do
		(
			local val = ""
			if type.types[r-1] == booleanClass then val = false
			dgObjProps.Rows.Add (makeDataGridRow type.types[r-1] #(type.labels[r-1],val))
		)
		dgObjProps.CurrentCell = undefined
		dgObjProps.Height = dgObjProps.Rows.count * (dgvRowHeight + 0.3)
		panUDPPanel.Top = dgObjProps.Bottom + 5
		panMatPanel.Top = cbt.panUDPPanel.Bottom + 10
	),
	
	function refreshUDPBox =
	(
		local sUdp = ""
		if tvObjList.selectedNodes.count == 1 then
		(
			obj = (tvObjList.selectedNodes.item(0)).tag.value
			sUdp = getUserPropBuffer obj
		) else (
			
		)
		tbUDPBox.Text = sUdp
	),
	
	function updateDataGridData =
	(
		dgObjProps.Refresh()
		dgObjProps.Rows.Clear()
		
		tbUDPBox.Text = ""

		dgObjProps.suspendLayout()

		if tvObjList.selectedNodes.count <= 1 then
		(
			local thisUDP = #()
			local obj = undefined
			local type = objType_Joint()
			local objName = ""
			local sUdp = ""

			if tvObjList.selectedNodes.count == 1 then
			(
				if (tvObjList.selectedNodes.item(0) == (tvObjList.nodes.item 0)) then
					return()

				obj = (tvObjList.selectedNodes.item(0)).tag.value
				type = getObjectType obj
				objName = obj.name
				sUdp = getUserPropBuffer obj
			)
			
			makeDataGridView type
			
			(dgObjProps.Columns.item 1).DefaultCellStyle = clrStyle[1]
			tbUDPBox.backColor = clrStyle[1].backColor

			(dgObjProps.Item 1 0).value = objName
			(dgObjProps.item 1 1).value = getObjTypeString obj
			tbUDPBox.Text = sUdp

			--print ("type.udpKeys.count = " + (type.udpKeys.count as string))
			thisUDP = getUDPValues type sUdp
			
			/*
			for i=1 to thisUDP[1].count do
			(
				print ("UDP["+(i as string)+"] " + (thisUDP[1][i] as string) + " = " + (thisUDP[2][i] as string))
				for j=1 to type.udpKeys.count do
				(
					if thisUDP[1][i] == type.udpKeys[j] then
					(
						print ("Clearing " + type.udpKeys[j] + "...")
						(dgObjProps.Item 1 (j+1)).Style = clrStyle[1]
						(dgObjProps.Item 0 (j+1)).value = type.labels[j]
						(dgObjProps.Item 1 (j+1)).value = thisUDP[2][i]
						break
					)
				)

				if i > type.udpKeys.count then
				(
					--dgObjProps.Rows.add()
					(dgObjProps.Item 1 (j+1)).Style = clrStyle[1]
					(dgObjProps.Item 0 (i+1)).value = thisUDP[1][i]
					(dgObjProps.Item 1 (i+1)).value = thisUDP[2][i]
				)
			)
			*/
			
			for i=1 to type.udpKeys.count do
			(
				local gotMatch = false
				--print ("UDP["+(i as string)+"] " + (thisUDP[1][i] as string) + " = " + (thisUDP[2][i] as string))
				for j=1 to thisUDP[1].count do
				(
					if thisUDP[1][j] == type.udpKeys[i] then
					(
						gotMatch = true
						--print ("setting "+ ((i+1) as string) +": "+ type.labels[j] + " = " + (thisUDP[2][i] as string))
						(dgObjProps.Item 0 (i+1)).value = type.labels[i]
						(dgObjProps.Item 1 (i+1)).value = thisUDP[2][j]
						break
					)
					
					--if j > type.udpKeys.count then
					--(
						--dgObjProps.Rows.add()
					--	(dgObjProps.Item 1 (j+1)).Style = clrStyle[1]
					--	(dgObjProps.Item 0 (j+1)).value = thisUDP[1][i]
					--	(dgObjProps.Item 1 (j+1)).value = thisUDP[2][i]
					--)
					
				)
					
				if gotMatch then
				(
					--print ("Matched " + (type.udpKeys[i] as string))  --(dgObjProps.Item 0 (i+1)).value)
					(dgObjProps.Item 1 (i+1)).Style = clrStyle[1]
				) else (
					(dgObjProps.Item 1 (i+1)).Style = clrStyle[8]
				)
			)
			
		) else (
			local firstType = getObjectType (tvObjList.selectedNodes.item 0).tag.value
			makeDataGridView firstType

			tbUDPBox.backColor = clrStyle[2].backColor
			(dgObjProps.Columns.item 1).DefaultCellStyle = clrStyle[2]
			(dgObjProps.Item 1 0).value = "multiple selected..."
			
			local summaryUDPLabs = #()
			local summaryUDPVals = #()
			local flatUDPKeys = firstType.udpKeys
			-- first, get all the struct labels/types that we need in an array
			for i=0 to (tvObjList.selectedNodes.count-1) do
			(
				obj = (tvObjList.selectedNodes.item(i)).tag.value
				local type = getObjectType obj
				for k=1 to type.udpKeys.count do
				(
					if (appendIfUnique flatUDPKeys type.udpKeys[k]) == true then
						dgObjProps.Rows.Add (makeDataGridRow type.types[k] #(type.labels[k],""))
				)

				local thisUDP = #()
					
				sUdp = getUserPropBuffer obj
				thisUDP = getUDPValues type sUdp
				append thisUDP[1] "Type"
				append thisUDP[2] (getObjTypeString obj)
				if i==0 then
				(
					append summaryUDPLabs "Type"
					append summaryUDPVals thisUDP[2][thisUDP[2].count]
				)
				for j=1 to thisUDP[1].count do
				(
					local idx = findItem summaryUDPLabs thisUDP[1][j]
					if idx > 0 then
					(
						if summaryUDPVals[idx] != thisUDP[2][j] then summaryUDPVals[idx] = "..."
					) else (
						summaryUDPLabs = append summaryUDPLabs thisUDP[1][j]
						summaryUDPVals = append summaryUDPVals thisUDP[2][j]
					)
				)
				for j=1 to summaryUDPLabs.count do
				(
					local idx = findItem thisUDP[1] summaryUDPLabs[j]
					if idx > 0 then
					(
						if thisUDP[2][idx] != summaryUDPVals[j] then summaryUDPVals[j] = "..."
					) else
						summaryUDPVals[j] = "..."
				)
			)
			
			for i=1 to summaryUDPLabs.count do
			(
				if summaryUDPLabs[i] == "Type" then
				(
					(dgObjProps.item 1 1).style = undefined
					(dgObjProps.Item 1 1).value = summaryUDPVals[i]
				) else (
					for r = 2 to (dgObjProps.RowCount-1) do
					(
						local sumLab = ""
						local rowLab = ""
						try(
							sumLab = toLower summaryUDPLabs[i]
							rowLab = toLower (dgObjProps.item 0 r).value
						) catch()
						if (sumLab == rowLab) then
						(
							(dgObjProps.item 1 r).style = undefined
							(dgObjProps.Item 1 r).value = summaryUDPVals[i]
							if summaryUDPVals[i] != "..." then
								(dgObjProps.item 1 r).style = clrStyle[1]
						)
					)
				)
			)
		)
		dgObjProps.resumeLayout()
		updateMaterialList()
	),

	function applyObjectUDP obj type udpKey sVal =
	(
		local aUdp = #()
		local sUdp = getUserPropBuffer obj
		aUdp = getUDPValues type sUdp
		
		sUdp = ""
		local idx = 0
		local found = false

		for u=1 to aUdp[1].count do
		(
			if (aUdp[1][u] != undefined) and (udpKey != undefined) then
			(
				if (tolower aUdp[1][u]) == (tolower udpKey) then
				(
					aUdp[2][u] = sVal
					found = true
				)
			)
		)
		if found==false and (udpKey != undefined) then
		(
			append aUdp[1] udpKey
			append aUdp[2] sVal
		)
		
		setUserPropBuffer obj ""
		for u=1 to aUdp[2].count do
		(
			for i=1 to type.udpKeys.count do
			(
				if (tolower aUdp[1][u]) == type.udpKeys[i] then
				(
					idx = i
					break
				)
			)
			if idx > 0 then
			(
				if aUdp[2][u] != "" and aUdp[2][u] != "undefined" and aUdp[2][u] != "false" then
				(
					if aUdp[2][u] == "true" or aUdp[2][u] == true then
						sUdp = (sUdp + aUdp[1][u] + "\r\n")
					else
					(
						if aUdp[2][u] != undefined then
							sUdp = (sUdp + aUdp[1][u] + "=" + (aUdp[2][u] as string) + "\r\n")
					)
				) else (
					print ("clearing the value for " + aUdp[1][u])
				)
			) else (
				print ("Warning: Idx < 0!")
				if aUdp[2][u] != "" and aUdp[2][u] != "undefined" and aUdp[2][u] != "false" then
				(
					if aUdp[2][u] == true then
						sUdp = (sUdp + aUdp[1][u] + "\r\n")
					else
						sUdp = (sUdp + aUdp[1][u] + "=" + (aUdp[2][u] as string) + "\r\n")
				) else (
					if aUdp[2][u] != "undefined" then
						sUdp = (sUdp + aUdp[1][u] + "\r\n")
				)
			)
		)
		--print ("Setting UDP = " + sUdp)
		--print ("----------------")
		setUserPropBuffer obj sUdp
	),

	/*****************************************************************************/
	/******************        T R E E   V I E W   F U N C T I O N S       ************/
	
	function setTreeNodeColor treeNode =
	(
		treeNode.ForeColor = undefined

		if treeNode.tag.value != "" then
		(
			if (getUserPropBuffer treeNode.tag.value) != "" then
				treeNode.BackColor = clrStyle[6].BackColor
		
			if ((treeNode.tag.value).isHidden == true) then
				treeNode.ForeColor = dNColor.fromArgb 200 0 0 --clrStyle[5].foreColor
		)
	),
	
	function addTreeNode thisobj  = 
	(
		childNodes = #()
		for child in thisobj.children do
		(
			local childTreeNode = addTreeNode child
			if childTreeNode != undefined then
				append childNodes childTreeNode
		)
		if thisObj.isHidden and bShowHidden == false then 
			return undefined
			
		local treeNode = dotNetObject "System.Windows.Forms.TreeNode" thisobj.name childNodes
		treeNode.tag = dotNetMXSValue thisobj
		setTreeNodeColor treeNode
		appendIfUnique treeNodes treeNode
		return treeNode
	),
	
	function findNodeInChildren item target =
	(
		found = undefined
		if item.tag.value == target.tag.value then
			return item
		for i = 0 to (target.nodes.count-1) do
		(
			if item.tag.value == (target.nodes.item(i)).tag.value then
			(
				return target.nodes.item(i)
			) else (
				if (target.nodes.item(i)).nodes.count > 0 then
					return findNodeInChildren item (target.nodes.item(i))
			)
		)
		return()
	),
	
	function isNodeInHierarchy obj target =
	(
		match = false
		if target == obj then
		(
			match = true
		) else (
			if target != undefined and target != "" then
			(
				if target.parent == obj then
				(
					match = true
				) else 
					match = isNodeInHierarchy obj target.parent
			) 
		)
		return match
	),

	function isNodeInList thisNode list =
	(
		if list != undefined then
		(
			for i=0 to (list.count-1) do
			(
				if thisNode == list.item(i) then
					return true
			)
		)
		return false
	),

	function updateSelectedNodes =
	(
		for i=1 to (treeNodes.count) do 
		(
			for obj in selection do
			(
				if (treeNodes[i]).Tag.value == obj then
				(
					tvObjList.SelectedNodes.Add(treeNodes[i])
					break
				)
			)
		)
	),

	function expandTreeView =
	(
		for i = 0 to (tvObjList.selectedNodes.count-1) do
		(
			(tvObjList.selectedNodes.item(i)).expandAll()
			thisParent = (tvObjList.selectedNodes.item(i)).parent
			while thisParent != (tvObjList.nodes.item 0) do
			(
				thisParent.expandAll()
				thisParent = thisParent.parent
			)
		)
		bExpandTree=true
		if tvObjList.SelectedNodes.count > 0 then
		(
			if tvLastTopNode == undefined then
				tvLastTopNode = tvObjList.Nodes.Item 0
			tvObjList.TopNode = tvLastTopNode
			tvObjList.TopNode.EnsureVisible()
		)
		tvExpanding = false
	),
	
	function refreshTreeView =
	(
		tvObjList.selectedNodes.clear()
		for i = 1 to treeNodes.count do
			setTreeNodeColor treeNodes[i]

		updateSelectedNodes()
	),
	
	function rebuildTreeView =
	(
		--print "RebuildTreeView"
		tvObjList.BeginUpdate()
		
		tvObjList.nodes.clear()
		treeNodes = #()
		
		local topNodes = #()
		
		for obj in objects do
		(
			if obj.parent == undefined then
			(
				if (obj.name != "cryExporterSettingsNode" and obj.name != "cryAnimationExportNode") then
				(
					if obj.isHidden and bShowHidden == false then continue	-- ** This will hide all children too
					local treeNode = addTreeNode obj
					append topNodes treeNode
				)
			)
		)

		local rootTreeNode = dotNetObject "System.Windows.Forms.TreeNode" "Scene Root" topNodes
		rootTreeNode.tag = dotNetMXSValue ""
		tvObjList.nodes.Add rootTreeNode

		--tvObjList.sort()
		rootTreeNode.Expand()
		refreshTreeView()
		tvObjList.EndUpdate()
	),

	function selectObjsFromList =
	(
		tvSelectionHandled = true
		local selObjs = #()
		for i = 0 to (tvObjList.SelectedNodes.count-1) do
		(
			if (tvObjList.SelectedNodes.item i) != (tvObjList.nodes.item 0) then
				append selObjs (tvObjList.SelectedNodes.item(i)).tag.value
		)
		try(select selObjs) catch(rebuildTreeView())
		--selectedNodes = nodes
		tvSelectionHandled = false
		
	),
	
	function callRightMouseClickRightPanel =
	(
		RCMenu rightMouseClickRightPanelRC
		(
			menuItem miRefresh "Refresh" enabled:true
			menuItem miApplyMat "Apply to Selected" enabled:true
			
			on rightMouseClickRightPanelRC open do
			(
				if dgMatList.CurrentCell == undefined then
				(
					miApplyMat.enabled = false
				)
			)
			
			on miRefresh picked do
			(
				cbt.updateDataGridData()
				cbt.updateMaterialList()
			)
			on miApplyMat picked do
			(
				local mat = (cbt.dgMatList.item 3 (cbt.dgMatList.CurrentCell.RowIndex)).tag.value
				for obj in selection do
					try (obj.material = mat) catch()
				cbt.updateMaterialList()
			)
		)
		popUpMenu rightMouseClickRightPanelRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
	),
	
	function callRightMouseClickTreeView =
	(
		RCMenu rightMouseClickTreeRC
		(
			menuItem miRefresh "Refresh Tree" enabled:true
			menuItem miShowHidden "Show Hidden Objects" checked:(cbt.bShowHidden) enabled:true
			separator sep0
			menuItem miHide "Hide Selected" enabled:true
			menuItem miShow "Show Selected" enabled:true
			menuItem miShowSub "Show All Hidden Submeshes" enabled:true
			separator sep1
			menuItem miColor "Randomize Wire Colors" enabled:true
			separator sep2
			menuItem miCopy "Copy UDP to clipboard" enabled:true
			menuItem miPaste "Paste UDP to Selected" enabled:true
			menuItem miClear "Clear UDP" enabled:true

			on rightMouseClickTreeRC open do
			(
				if selection.count != 1 then
					miCopy.enabled = false
				if cbt.udpCopy == "" then
					miPaste.enabled = false
				if selection.count > 0 then
				(
					miColor.enabled = true
					if (getUserPropBuffer selection[1]) == "" then
						miCopy.enabled = false
				) else (
					miColor.enabled = false
					miCopy.enabled = false
					miPaste.enabled = false
					miClear.enabled = false
				)
			)
			
			on miRefresh picked do
			(
				cbt.rebuildTreeView()
				cbt.expandTreeView()
			)
			on miShowHidden picked do
			(
				cbt.bShowHidden = (not cbt.bShowHidden)
				cbt.rebuildTreeView()
				cbt.expandTreeView()
			)
			on miHide picked do
			(
				cryBreakablesRemoveCallbacks()
				for obj in selection do
					try(obj.isHidden = true) catch()
				cryBreakablesAddCallbacks()
				cbt.refreshTreeView()
			)
			on miShow picked do
			(
				cryBreakablesRemoveCallbacks()
				for obj in selection do
					try(obj.isHidden = false) catch()
				cryBreakablesAddCallbacks()
				cbt.refreshTreeView()
			)
			on miShowSub picked do
			(
				for i=1 to cbt.treeNodes.count do
				(
					if cbt.treeNodes[i].tag.value != "" and cbt.treeNodes[i].tag.value != undefined then
						if (superclassOf cbt.treeNodes[i].tag.value) == GeometryClass then
							cbt.treeNodes[i].tag.value.unhideAll #Face
				)
			)
			on miColor picked do
			(
				for obj in selection do
				(
					obj.wireColor = random (color 0 0 0) (color 255 255 255)
				)
			)
			on miCopy picked do
			(
				if selection.count == 1 then
				(
					cbt.udpCopy = getUserPropBuffer selection[1]
				)
			)
			on miPaste picked do
			(
				for obj in selection do
				(
					setUserPropBuffer obj cbt.udpCopy
				)
				cbt.updateDataGridData()
			)
			on miClear picked do
			(
				for obj in selection do
					setUserPropBuffer obj ""
				cbt.updateDataGridData()
				cbt.refreshTreeView()
			)
		)
		popUpMenu rightMouseClickTreeRC pos:[(mouse.screenpos[1] - 5), (mouse.screenpos[2] - 5)]					
	),
	
	function dgObjProps_endEdit arg =
	(
		local c = arg.ColumnIndex
		local r = arg.RowIndex
		local val = ((dgObjProps.Item c r).EditedFormattedValue as string)
		
		if selection.count > 0 then
		(	-- If user changed the object name...
			if r == 0 then
			(
				local found=false
				for i=1 to selection.count do
				(
					for obj in objects do
						if obj.name == val then (
							found=true
							break
						)
					if found == false then
						try(selection[i].name = val) catch()
					else
						selection[i].name = uniqueName val
					val = selection[i].name
				)
				rebuildTreeView()
				expandTreeView()
			)

			if r > 1 then
			(
				-- clear the custom style
				(dgObjProps.item c r).style = undefined
				
				if (val == "" or val == undefined) then
					val = ""

				for obj in selection do
				(
					local objType = getObjectType obj
					local dataType = undefined
					local idx = 0
					for i=1 to objType.labels.count do
					(
						if (dgObjProps.item 0 r).value == objType.labels[i] then
							idx = i
					)
					
					if idx > 0 then
					(
						dataType = objType.udpKeys[idx]
						if val == "" or val == "true" or val == "false" then
						(
							if (objType.types[idx]) != float then
							(
								try (val = (val as (objType.types[idx]))) catch (val = "")
							)
						) else (
							try (val = (val as (objType.types[idx]))) catch (val = "")
						)
					)
					if val == undefined or val == "undefined" then 
						val = ""
					else val = (val as string)
					(dgObjProps.Item c r).value = val
					applyObjectUDP obj objType dataType val
				)
			)
			
			refreshTreeView()
		)
		refreshUDPBox()
	),

	function removeNonGeoFromSel =
	(
		desel = #()
		
		for obj in selection do
		(
			
			if (superclassOf obj) != GeometryClass then
			(
				append desel obj
			)
		)
		if desel.count > 0 then
			deselect desel
	),
	
	function hideMat id =
	(
		removeNonGeoFromSel()
		
		max modify mode
		if modify != undefined then
			collapseStack selection
		
		if id == 0 then
		(
			for i=1 to selection.count do
			(
				if (superclassOf selection[i]) == GeometryClass then
				(
					selection[i].SetSelection #Face #{1..(selection[i].NumFaces)}
					selection[i].Hide #Face
				)
			)			
		) else (
			for i=1 to selection.count do
			(
				if (superclassOf selection[i]) == GeometryClass then
				(
					selection[i].selectByMaterial id
					selection[i].Hide #Face
				)
			)
		) 
		
		if modify != undefined then
		(
			subObjectLevel = 0
			collapseStack selection
			modPanel.addModToSelection modify
			subobjectlevel = 4
--			modify.selectByMaterialID = id
			modify.ButtonOp #SelectByMaterial
			
			forceCompleteRedraw()
		)
	),
	
	function showMat id =
	(
		removeNonGeoFromSel()

		max modify mode
		if modify != undefined then
			collapseStack selection
		
		if id == 0 then
		(
			for i=1 to selection.count do
			(
				if (superclassOf selection[i]) == GeometryClass then
					selection[i].unhideAll #Face
			)
		) else (
			for i=1 to selection.count do
			(
				if (superclassOf selection[i]) == GeometryClass then
				(
					local hiddenFaces = polyop.getHiddenFaces selection[i]
					local showFaces = #()
					for f=1 to selection[i].faces.count do
					(
						if (polyop.getFaceMatID selection[i] f) == id then
						(
							append showFaces f
							hiddenFaces = deleteItem hiddenFaces (findItem hiddenFaces f)
						)
						selection[i].unhideAll #Face
						polyop.setHiddenFaces selection[i] hiddenFaces
					)
				)
			)
		)
		
		if modify != undefined then
		(
			subObjectLevel = 0
			collapseStack selection
			modPanel.addModToSelection modify
			subobjectlevel = 4
--			modify.selectByMaterialID = id
			modify.ButtonOp #SelectByMaterial
			
			forceCompleteRedraw()
		)
	),
	
	function selectByMat id =
	(
		removeNonGeoFromSel()

		if id < 0 then
			return()

		max modify mode
		showMat id
		
		subObjectLevel = 0
		collapseStack selection

		modify = undefined
		modify = Edit_Poly()
		modify.selectByMaterialClear = true
		modPanel.addModToSelection modify
		subobjectlevel = 4
		modify.selectByMaterialID = id
		modify.ButtonOp #SelectByMaterial
		
		forceCompleteRedraw()
	),
	
	function dgObjProps_editKeyUp arg =
	(
		--print (arg.KeyValue)
		if  (arg.KeyData == cbt.dnKeys.Return) or (arg.KeyData == dnKeys.Enter) then
		(
			local val = dgObjProps.CurrentCell.EditedFormattedValue
			for i = 0 to (dgObjProps.SelectedCells.Count-1) do
			(
				local cell = (dgObjProps.SelectedCells.item i)
				cell.Value = val
				dgObjProps_endEdit cell
			)
			dgObjProps.CurrentCell = (dgObjProps.item dgObjProps.CurrentCell.ColumnIndex (dgObjProps.CurrentCell.RowIndex+1))
		)
	),

	function gotoNextRow arg =
	(
		local c = arg.ColumnIndex
		local r = arg.RowIndex
		cbt.dgObjProps.EndEdit()
		--cbt.dgObjProps.CurrentCell = (cbt.dgObjProps.item c (r+1))
	),
	
/**************************************************************************************************************/	
/************************************  PHYS MESHES AND JOINTS *******************************************/
	
	function bbIntersectsbb bb1 bb2 =
	(
		return ((bb1[1].x <= bb2[2].x) and (bb1[2].x >= bb2[1].x) and (bb1[1].y <= bb2[2].y) and (bb1[2].y >= bb2[1].y) and (bb1[1].z <= bb2[2].z) and (bb1[2].z >= bb2[1].z))
	),
	
	function pointIntersectsbb pt bb =
	(
		for i=1 to 3 do
		(
			pt[i] = (floor (1000 * pt[i])) / 1000
			bb[1][i] = (floor (1000 * bb[1][i])) / 1000
			bb[2][i] = (floor (1000 * bb[2][i])) / 1000
		)
		return ((pt.x <= bb[2].x) and (pt.x >= bb[1].x) and (pt.y <= bb[2].y) and (pt.y >= bb[1].y) and (pt.z <= bb[2].z) and (pt.z >= bb[1].z))
	),
	
	function makePhysMeshes =
	(
		local matid = cbt.tbMatId.text as number
		for i=1 to selection.count do
		(
			local newmesh = snapshot selection[i]
			newmesh.material = selection[i].material
			convertTo newmesh Editable_Poly
			local faces = #()
			for f=1 to newmesh.faces.count do
				append faces f
			polyop.setFaceMatID newmesh faces matid
			polyop.attach selection[i] newmesh
		)
	),
	
	function makeJoints =
	(
		local done = #()
		local jointCount = 0
		local jointCounts = #()
		for i=1 to selection.count do
		(
			jointCounts[i] = 0
			local bb1 = nodeLocalBoundingBox selection[i]
			
			-- only one object selected, so just create one joint at its center
			if selection.count == 1 then
			(
				local dum = Dummy()
				dum.boxSize = [10, 10, 10]
				dum.position = selection[1].center
				dum.name = uniqueName ("$joint0")
				dum.parent = selection[i].parent
				jointCount += 1
			)
			
			for j=2 to selection.count do
			(
				if jointCounts[j] == undefined then jointCounts[j] = 0
				if (done[j] == true) or (jointCounts[j] == 2) or (selection[i] == selection[j]) then continue
					
				local bb2 = nodeLocalBoundingBox selection[j]
				if (bbIntersectsbb bb1 bb2) or (bbIntersectsbb bb2 bb1) or (intersects selection[i] selection[j]) then
				(
					print ("BBs intersect: " + selection[i].name + ", " + selection[j].name)
					local numV = 0
					local avgV = undefined
					local matchedVerts = #()
					for v2=1 to selection[j].vertices.count do
					(
						if pointIntersectsbb selection[j].vertices[v2].position bb1 then
						(
							local numV2 = 0
							local v2p = selection[j].vertices[v2].position
							
							for v1 = 1 to selection[i].vertices.count do
							(
								local v1p = selection[i].vertices[v1].position
								--if (close_enough v1p.x v2p.x 10) and (close_enough v1p.y v2p.y 10) and (close_enough v1p.z v2p.z 10) then
								if (distance v1p v2p) < 0.01 then
								(
									numV2 += 1
									numV += 1
									if avgV == undefined then
										avgV = v2p
									else
										avgV += v2p
									append matchedVerts v2p
									break
								)
							)
							if numV2 == 0 then
							(
								numV += 1
								if avgV == undefined then
									avgV = v2p
								else
									avgV += v2p
								append matchedVerts v2p
								break
							)
						)
					)
					
					if avgV == undefined then
						print "avgV = undefined"
					if numV == 0 then
						print "numV = 0"
					
					if avgV != undefined and numV > 0 then
					(
						local dum = Dummy()
						dum.boxSize = [10, 10, 10]
						dum.position = (avgV / numV)
						dum.name = uniqueName ("$joint0")
						dum.parent = selection[i].parent
						jointCount += 1
						jointCounts[j] += 1
					) else (
						-- cannot find any intersecting points, but if user only selected 2 objects, then create a helper anyway
						if selection.count == 2 then
						(
							
						)
					)
				)
			)
			done[i] = true
		)
		print ("Created " + (jointCount as string) + " Joint helpers.")
		
		rebuildTreeView()
		expandTreeView()
	),
	
	function SetAutoScrollPosition sender p =
	(
		p.x = abs p.x
		p.y = abs p.y
		sender.AutoScrollPosition = p
		--print ("Resetting scroll pos = " + (p.x as string) + ", " + (p.y as string))
	),
	
/**************************************************************************************************************/	
/******************************************  EVENT HANDLERS **************************************************/
	function initEventHandlers =
	(
		function event_panRightPanel_Scroll arg =
		(
			cbt.rpScrollPos = cbt.panRightPanel.AutoScrollPosition
		)
		dotNet.addEventHandler cbt.panRightPanel "Scroll" event_panRightPanel_Scroll

		function event_dgObjProps_CellBeginEdit arg =
		(
			local c = arg.ColumnIndex
			local r = arg.RowIndex
			-- set backColor to white, to indicate editing
			(cbt.dgObjProps.item c r).style = cbt.clrStyle[1] 
			cbt.oldCellVal = (cbt.dgObjProps.item c r).value
			cbt.oldCellVals = #()
			for i=0 to (cbt.dgObjProps.SelectedCells.Count-1) do
			(
				append cbt.oldCellVals (cbt.dgObjProps.SelectedCells.item i).value
			)
		)
		dotNet.addEventHandler cbt.dgObjProps "CellBeginEdit" event_dgObjProps_CellBeginEdit
		
		function event_dgMatList_CellBeginEdit arg =
		(
			local c = arg.ColumnIndex
			local r = arg.RowIndex

			cbt.oldCellVal = (cbt.dgMatList.item c r).EditedFormattedValue
		)
		dotNet.addEventHandler cbt.dgMatList "CellBeginEdit" event_dgMatList_CellBeginEdit
		
		function event_dgMatList_CellEndEdit arg =
		(
			local c = arg.ColumnIndex
			local r = arg.RowIndex
			local newval = (cbt.dgMatList.item c r).EditedFormattedValue as string

			if c != 3 then return()	-- only support editing the material names
			
			if cbt.oldCellVal != newval then
			(
				-- rename the material:
				(cbt.dgMatList.item cbt.matColumn r).tag.value.name = newval
				
				if (classOf (cbt.dgMatList.item cbt.matColumn 0).tag.value) == Multimaterial then
				( -- also rename the submat slot name
					(cbt.dgMatList.item cbt.matColumn 0).tag.value.names[(cbt.dgMatList.item 0 r).tag] = newval
				)
			)
			cbt.oldCellVal = undefined
		)
		dotNet.addEventHandler cbt.dgMatList "CellEndEdit" event_dgMatList_CellEndEdit
		
		function event_dgMatList_CellClick arg =
		(
			--print "Cell click"
			local c = arg.ColumnIndex
			local r = arg.RowIndex
			
			if c<0 or r<0 then return()
			
			case c of
			(
				0: cbt.hideMat (cbt.dgMatList.item c r).tag
				1: cbt.showMat (cbt.dgMatList.item c r).tag
				2: cbt.selectByMat ((cbt.dgMatList.item c r).tag - 1)
			)
		)
		dotNet.addEventHandler cbt.dgMatList "CellClick" event_dgMatList_CellClick
		
		function event_dgMatList_CellDoubleClick arg =
		(
			cbt.SetAutoScrollPosition cbt.panRightPanel cbt.rpScrollPos

			local c = arg.ColumnIndex
			local r = arg.RowIndex
			
			if r <= 0 then return()
			
			local mat = (cbt.dgMatList.item cbt.matColumn r).tag.value
			MatEditor.Open()
			
			local slot = (findItem meditMaterials mat)
			if slot != 0 then
			(
				activeMeditSlot = slot
			) else (
				activeMeditSlot = 1
				setMeditMaterial 1 mat
			)
		)
		dotNet.addEventHandler cbt.dgMatList "CellDoubleClick" event_dgMatList_CellDoubleClick

		function event_dgMatList_MouseDown arg =
		(
			--print "Mouse down"
			cbt.SetAutoScrollPosition cbt.panRightPanel cbt.rpScrollPos
		)
		dotNet.addEventHandler cbt.dgMatList "MouseDown" event_dgMatList_MouseDown
		
		function event_dgMatList_MouseUp arg =
		(
			if arg.button == arg.button.right then
				cbt.callRightMouseClickRightPanel()
		)
		dotNet.addEventHandler cbt.dgMatList "MouseUp" event_dgMatList_MouseUp
		
		function event_dgObjProps_CellEndEdit arg =
		(
			cbt.dgObjProps_endEdit arg
		)
		dotNet.addEventHandler cbt.dgObjProps "CellEndEdit" event_dgObjProps_CellEndEdit
		
		function event_panRightPanel_MouseUp arg =
		(
			if arg.button == arg.button.right then
				cbt.callRightMouseClickRightPanel()
		)
		dotNet.addEventHandler cbt.panRightPanel "MouseUp" event_panRightPanel_MouseUp

		function event_dgObjProps_MouseDown arg =
		(
			cbt.SetAutoScrollPosition cbt.panRightPanel cbt.rpScrollPos
			if arg.button == arg.button.right then
				cbt.callRightMouseClickRightPanel()
		)
		dotNet.addEventHandler cbt.dgObjProps "MouseDown" event_dgObjProps_MouseDown

		function event_dgObjProps_EditingControlShowing arg =
		(
			dotNet.removeEventHandler arg.Control "KeyUp" cbt.dgObjProps_editKeyUp
			dotNet.addEventHandler arg.Control "KeyUp" cbt.dgObjProps_editKeyUp
		)
		dotNet.addEventHandler cbt.dgObjProps "EditingControlShowing" event_dgObjProps_EditingControlShowing
		
		function event_dgObjProps_KeyUp arg =
		(
			if (arg.KeyData == cbt.dnKeys.Delete) then
			(
				if cbt.dgObjProps.CurrentCell.ColumnIndex == 1 then
				(
					for i=0 to (cbt.dgObjProps.SelectedCells.count-1) do
					(
						(cbt.dgObjProps.SelectedCells.item i).Value = ""
						cbt.dgObjProps_endEdit (cbt.dgObjProps.SelectedCells.item i)
					)
				)
			)
		)
		dotNet.addEventHandler cbt.dgObjProps "KeyUp" event_dgObjProps_KeyUp
		
		function event_dgObjProps_Leave arg =
		(
			cbt.dgObjProps.CurrentCell = undefined
		)
		dotNet.addEventHandler cbt.dgObjProps "Leave" event_dgObjProps_Leave
		
		function event_tvObjList_KeyUp arg =
		(
			if (arg.KeyData == cbt.dnKeys.Delete) then
			(
				--print ("DELETE")
				if cbt.dgObjProps.CurrentCell == undefined then
					if selection.count > 0 then
					(
						local newsel = (cbt.tvObjList.SelectedNodes.item 0).Parent
						delete selection
						cbt.tvObjList.SelectedNodes.Clear()
						cbt.tvObjList.SelectedNodes.Add newsel
						
						cbt.rebuildTreeView()
						cbt.updateDataGridData()
					)
			)
		)
		dotNet.addEventHandler cbt.tvObjList "KeyUp" event_tvObjList_KeyUp
		
		function event_tvObjList_DoubleClick arg =
		(
			max properties
		)
		dotNet.addEventHandler cbt.tvObjList "NodeMouseDoubleClick" event_tvObjList_DoubleClick
		
		function event_tvObjList_MouseLeave arg =
		(
			if cbt.isDragging then 
			(
				if cbt.lastHoverNode != undefined then
					cbt.lastHoverNode.backColor = undefined
				cbt.refreshTreeView()
			)
			cbt.isDragging = false
		)
		dotNet.addEventHandler cbt.tvObjList "MouseLeave" event_tvObjList_MouseLeave
		
		function event_tvObjList_MouseMove arg =
		(
			if cbt.bMouseIsDown and cbt.tvExpanding == false then
			(
				local tempNode = cbt.tvObjList.hitTest arg.location
				if tempNode.node != (cbt.tvObjList.nodes.item 0) then
				(
					if tempNode.node != cbt.pickedNode then
						cbt.isDragging = true
				)
				
				if cbt.isDragging then
				(
					if cbt.lastHoverNode != undefined then
						cbt.setTreeNodeColor cbt.lastHoverNode
					
					if tempNode.node != cbt.pickedNode then
					(
						if cbt.pickedNodes.IndexOf(tempNode.node) == -1 then
						(
							cbt.lastHoverNode = tempNode.node
							tempNode.node.backColor = cbt.clrStyle[7].backColor
						)
					)
				)
			)
		)
		dotNet.addEventHandler cbt.tvObjList "MouseMove" event_tvObjList_MouseMove
		
		function event_tvObjList_BeforeCollapse arg =
		(
			cbt.pickedNode = undefined
			cbt.pickedNodes = undefined
			cbt.isDragging = false
			cbt.tvExpanding = true
			--cbt.tvLastTopNode = cbt.tvObjList.TopNode
		)
		dotNet.addEventHandler cbt.tvObjList "BeforeCollapse" event_tvObjList_BeforeCollapse
		
		function event_tvObjList_BeforeExpand arg =
		(
			cbt.pickedNode = undefined
			cbt.pickedNodes = undefined
			cbt.isDragging = false
			cbt.tvExpanding = true
			--cbt.tvLastTopNode = cbt.tvObjList.TopNode
		)
		dotNet.addEventHandler cbt.tvObjList "BeforeExpand" event_tvObjList_BeforeExpand

		function event_tvObjList_AfterCollapse arg =
		(
			--cbt.tvLastTopNode = cbt.tvObjList.TopNode
		)
		dotNet.addEventHandler cbt.tvObjList "AfterCollapse" event_tvObjList_AfterCollapse
		
		function event_tvObjList_AfterExpand arg =
		(
			cbt.refreshTreeView()
			--cbt.tvLastTopNode = cbt.tvObjList.TopNode
		)
		dotNet.addEventHandler cbt.tvObjList "AfterExpand" event_tvObjList_AfterExpand
		
		function event_tvObjList_MouseDown arg =
		(
			local tempNode = cbt.tvObjList.hitTest arg.location

			cbt.bMouseIsDown = true
			cbt.dgObjProps.CurrentCell = undefined
			cbt.pickedNode = tempNode.node
			cbt.pickedNodes = cbt.tvObjList.selectedNodes
			cbt.isDragging = false
			cbt.tvLastTopNode = cbt.tvObjList.TopNode
		)
		dotNet.addEventHandler cbt.tvObjList "MouseDown" event_tvObjList_MouseDown
	
		function event_tvObjList_MouseUp arg =
		(
			cbt.bMouseIsDown = false
			
			if cbt.tvExpanding then
			(
				cbt.tvExpanding = false
				return()
			)
			
			if arg.button == arg.button.right then
			(
				cbt.callRightMouseClickTreeView()
				return()
			)
			
			local tempNode = cbt.tvObjList.hitTest arg.location
			local updateMe = true
			
			cbt.bExpandTree = false
			
			if cbt.pickedNode != undefined then
			(
				-- if DRAGGED
				if cbt.isDragging and (cbt.isNodeInList tempNode.node cbt.pickedNodes) == false then
				(
					if tempNode.node == undefined then
					(
						-- parent to world
						for i=0 to (cbt.pickedNodes.count-1) do
							(cbt.pickedNodes.item(i)).tag.value.parent = undefined
					) else (
						for i=0 to (cbt.pickedNodes.count-1) do
						(
							if (cbt.isNodeInHierarchy (cbt.pickedNodes.item(i)).tag.value tempNode.node.tag.value) == false then
							(
								if (cbt.pickedNodes.item(i)).tag.value.parent != tempNode.node.tag.value then
								(
									if tempNode.node.tag.value == "" then
										(cbt.pickedNodes.item(i)).tag.value.parent = undefined
									else
										(cbt.pickedNodes.item(i)).tag.value.parent = tempNode.node.tag.value
								)
							)
						)
					)
					cbt.rebuildTreeView()
					cbt.expandTreeView()
				)
			)
			cbt.selectObjsFromList()
			
			cbt.pickedNode = undefined
			cbt.isDragging = false
			cbt.updateDataGridData()
			cbt.tvObjList.TopNode = cbt.tvLastTopNode
			--print ("FINAL TOPNODE = " + (cbt.tvLastTopNode.tag.value as string))
		)
		dotNet.addEventHandler cbt.tvObjList "MouseUp" event_tvObjList_MouseUp

		function event_butTVRefresh_MouseUp arg =
		(
			cryBreak_treeviewRedrawCallback()			
		)
		dotNet.addEventHandler cbt.butTVRefresh "MouseUp" event_butTVRefresh_MouseUp
		
		function event_butTVShowHidden_MouseUp arg =
		(
			if cbt.butTVShowHidden.Image == cbt.hideButImage1 then
				cbt.butTVShowHidden.Image = cbt.hideButImage2
			else
				cbt.butTVShowHidden.Image = cbt.hideButImage1
			cbt.bShowHidden = (not cbt.bShowHidden)
			cryBreak_treeviewRedrawCallback()
		)
		dotNet.addEventHandler cbt.butTVShowHidden "MouseUp" event_butTVShowHidden_MouseUp

		function event_tbUDPBox_Leave =
		(
			for obj in selection do
			(
				setUserPropBuffer obj cbt.tbUDPBox.Text
			)
			
			cbt.updateDataGridData()
		)
		dotNet.addEventHandler cbt.tbUDPBox "Leave" event_tbUDPBox_Leave
		
		function event_tbMatId_Leave arg =
		(
			local val = (cbt.tbMatId.text as number)
			if val == undefined then val = 1
			cbt.tbMatId.text = val as string
		)
		dotNet.addEventHandler cbt.tbMatId "Leave" event_tbMatId_Leave
		
		function event_butMakePhysMeshes_MouseUp arg =
		(
			if selection.count > 0 then
				cbt.makePhysMeshes()
		)
		dotNet.addEventHandler cbt.butMakePhysMeshes "MouseUp" event_butMakePhysMeshes_MouseUp
		
		function event_butMakeJoints_MouseUp arg =
		(
			if selection.count > 0 then
				cbt.makeJoints()
		)
		dotNet.addEventHandler cbt.butMakeJoints "MouseUp" event_butMakeJoints_MouseUp
		
		function event_main_Closing =
		(
			try(deleteAllChangeHandlers()) catch()
			try(
				cryBreakablesRemoveCallbacks()
				callbacks.RemoveScripts #filePreOpen id:#callback_filePreOpen
				callbacks.RemoveScripts #filePostOpen id:#callback_filePostOpen
			) 
			catch()
			gc()
		)
		dotNet.addEventHandler cbt.form "Closing" event_main_Closing
		
		function event_main_Resize =
		(
			cbt.SetAutoScrollPosition cbt.panRightPanel cbt.rpScrollPos
		)
		dotNet.addEventHandler cbt.form "Resize" event_main_Resize
		
		function event_butUDPArrow_Click =
		(
			if cbt.butUDPArrow.Text == "-" then
			(
				cbt.panUDPPanel.Height -= 110
				cbt.tbUDPBox.height = 4
				cbt.butUDPArrow.Text = "+"
			) else (
				cbt.panUDPPanel.Height += 110
				cbt.tbUDPBox.height = 120
				cbt.butUDPArrow.Text = "-"
			)
			
			cbt.panMatPanel.Top = cbt.panUDPPanel.Bottom + 10
		)
		dotNet.addEventHandler cbt.butUDPArrow "Click" event_butUDPArrow_Click
		
	),

	fn dgObjProps_mouseClick e s = (enableAccelerators = false),

	function initUI =
	(
		local dockStyle = dotNetClass "System.Windows.Forms.DockStyle"
		
		local anchorStyles = dotNetClass "System.Windows.Forms.AnchorStyles"
		local anchorNone = anchorStyles.None
		local anchorLeftRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Right
		local anchorLeftTop = dotNet.combineEnums anchorStyles.Left anchorStyles.Top
		local anchorLeftTopRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Top anchorStyles.Right
		local anchorLeftBottom = dotNet.combineEnums anchorStyles.Left anchorStyles.Bottom
		local anchorLeftBottomRight = dotNet.combineEnums anchorStyles.Left anchorStyles.Bottom anchorStyles.Right
		local anchorTop = anchorStyles.Top
		local anchorTopBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Bottom
		local anchorTopLeftBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Left anchorStyles.Bottom
		local anchorTopRightBottom = dotNet.combineEnums anchorStyles.Top anchorStyles.Right anchorStyles.Bottom
		local anchorTopBottomLeftRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Bottom anchorStyles.Left anchorStyles.Right
		local anchorTopRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Right
		local anchorBottom = dotNet.combineEnums anchorStyles.Bottom
		local anchorDefault = dotNet.combineEnums anchorStyles.Top anchorStyles.Left
		local anchorTopLeftRight = dotNet.combineEnums anchorStyles.Top anchorStyles.Left anchorStyles.Right
		
		for i = 1 to 8 do
		(
			append clrStyle (dotNetObject "System.Windows.Forms.DataGridViewCellStyle")
			clrStyle[clrStyle.count].backColor = clrStyle[clrStyle.count].backColor.white
		)
		clrStyle[1].foreColor = clrStyle[1].foreColor.fromArgb 0 0 0
		clrStyle[1].backColor = clrStyle[1].backColor.fromArgb 255 255 255
		
		clrStyle[2].foreColor = clrStyle[2].foreColor.fromArgb 0 0 0
		clrStyle[2].backColor = clrStyle[2].backColor.fromArgb 200 200 200
		
		clrStyle[3].backColor = clrStyle[3].backColor.fromArgb 225 224 210
		
		clrStyle[4].foreColor = clrStyle[4].ForeColor.fromArgb 0 101 14
		clrStyle[4].backColor = clrStyle[4].backColor.fromArgb 225 255 255
		
		clrStyle[5].foreColor = clrStyle[5].ForeColor.fromArgb 200 0 0
		clrStyle[5].backColor = clrStyle[5].backColor.fromArgb 255 255 255

		clrStyle[6].foreColor = clrStyle[6].ForeColor.fromArgb 0 0 0
		clrStyle[6].backColor = clrStyle[6].backColor.fromArgb 230 230 230
		
		clrStyle[7].foreColor = clrStyle[7].ForeColor.fromArgb 0 0 0
		clrStyle[7].backColor = clrStyle[7].backColor.fromArgb 200 200 255

		clrStyle[8].foreColor = clrStyle[8].ForeColor.fromArgb 0 0 0
		clrStyle[8].backColor = clrStyle[8].backColor.fromArgb 240 240 240
		
		form.suspendLayout()

		  toolTip1.AutoPopDelay = 5000
		  toolTip1.InitialDelay = 500
		  toolTip1.ReshowDelay = 500
		  toolTip1.ShowAlways = true
		
		--MAIN FORM
		form.width = 640 + form.width - form.clientRectangle.width
		form.height = 500 + form.height - form.clientRectangle.height

		-- SPLITTER Container
			crySplit.SplitterWidth = 2
			crySplit.Dock = dockStyle.Fill
			crySplit.Width = form.width
			crySplit.Panel1MinSize = 50
			crySplit.Panel2MinSize = 50
			crySplit.SplitterDistance = 250
			crySplit.AutoScroll = true
			crySplit.backColor = dNColor.fromArgb 100 100 100
			crySplit.Panel1.backColor = clrStyle[1].backColor
			crySplit.Panel2.backColor = clrStyle[1].backColor
		
		-- LEFTPANEL
			panLeftPanel.Anchor = anchorTopBottomLeftRight
			panLeftPanel.Dock = dockStyle.Fill
			panLeftPanel.Width = crySplit.Width/2
			panLeftPanel.height = (form.height)
			panLeftPanel.backColor = clrStyle[1].backColor
			panLeftPanel.AutoScroll = false

		-- LEFT BUTTON PANEL
			panLeftButPanel.Anchor = anchorTopLeftRight
			panLeftButPanel.Width = panLeftPanel.Width
			panLeftButPanel.Height = 22
			panLeftButPanel.backColor = clrStyle[3].backColor

		-- LEFT BUTTONS
			butTVRefresh.Image = refreshButImage
			butTVRefresh.Height = 22
			butTVRefresh.Width = 22
			toolTip1.SetToolTip butTVRefresh "Refresh Tree View"
			 
			butTVShowHidden.Image = hideButImage1
			butTVShowHidden.Left = butTVRefresh.Width
			butTVShowHidden.Height = 22
			butTVShowHidden.Width = 22
			toolTip1.SetToolTip butTVShowHidden "Show/Hide hidden objects"
			
		-- LEFT TREEVIEW PANEL
			panLeftTVPanel.Anchor = anchorTopBottomLeftRight
			panLeftTVPanel.Width = panLeftPanel.Width
			panLeftTVPanel.Height = panLeftPanel.Height - panLeftButPanel.Height
			panLeftTVPanel.Top = panLeftButPanel.Bottom
			
				-- TREE VIEW
					tvObjList.selectionMode = tvObjList.selectionMode.MultiSelect --SameLevel
					tvObjList.showLines = true
					tvObjList.fullRowSelect = true
					tvObjList.hideSelection = false
					tvObjList.Width = panLeftPanel.Width
					tvObjList.Height = panLeftTVPanel.Height
					tvObjList.Anchor = anchorTopBottomLeftRight
					tvObjList.Scrollable = true

		-- RIGHTPANEL
			panRightPanel.Anchor = anchorTopBottomLeftRight
			panRightPanel.Dock = dockStyle.Fill
			panRightPanel.backColor = clrStyle[3].backColor
			panRightPanel.AutoScroll = true

			rpScrollPos = panRightPanel.AutoScrollPosition
			print ("Scroll = " + (rpScrollPos.x as string) +"," + (rpScrollPos.y as string))
			
		-- RIGHT BUTTON PANEL
			--panRightButPanel.backColor = clrStyle[2].backColor
			panRightButPanel.Anchor = anchorTopLeftRight
			panRightButPanel.Width = panRightPanel.Width
			panRightButPanel.Top = 1
			panRightButPanel.Height = 40

					-- RIGHT BUTTONS
					labMatId.Text = "MatID:"
					labMatId.Top = 5
					labMatId.Height = 20
					labMatId.Width = 40

					tbMatId.Text = "1"
					tbMatId.Top = 0
					tbMatId.Left = labMatId.Right
					tbMatId.Height = 20
					tbMatId.Width = 30
					toolTip1.SetToolTip tbMatId "Material ID for Phys Meshes"
					
					butMakePhysMeshes.Text = "Make Phys Meshes"
					butMakePhysMeshes.Top = 0
					butMakePhysMeshes.Left = tbMatId.Right
					butMakePhysMeshes.Height = 20
					butMakePhysMeshes.Width = 140
					toolTip1.SetToolTip butMakePhysMeshes "Creates duplicate of all selected objects' sub-meshes, assigned with material MatID"
					
					butMakeJoints.Text = "Make Joints"
					butMakeJoints.Top = 0
					butMakeJoints.Height = 20
					butMakeJoints.Left = butMakePhysMeshes.Right
					butMakeJoints.Width = 140
					toolTip1.SetToolTip butMakeJoints "Create joint helpers between all selected objects"
			
		-- DATA GRID VIEW - OBJECT PROPERTIES
			dgObjProps.Anchor = anchorTopLeftRight
			dgObjProps.Top = panRightButPanel.Bottom
			dgObjProps.Width = panRightPanel.Width
			dgObjProps.foreColor = dNColor.fromArgb 0 0 0
			dgObjProps.backColor = clrStyle[3].backColor
			dgObjProps.BackgroundColor = clrStyle[1].backColor
			dgObjProps.ColumnCount = 2
			dgObjProps.ScrollBars = dgObjProps.ScrollBars.None
			makeDataGridView (objType_Joint())
			dgObjProps.ColumnHeadersVisible = false
			dgObjProps.RowHeadersVisible = false
			dgObjProps.ReadOnly = false
			dgObjProps.AllowUserToResizeColumns = true
			dgObjProps.AllowUserToOrderColumns = false
			dgObjProps.AllowUserToResizeRows = false
			dgObjProps.AllowUserToAddRows = false
			dgObjProps.ShowCellToolTips = false
			dgObjProps.height = dgObjProps.RowCount * (dgvRowHeight + 0.25)

			(dgObjProps.Columns.item 0).ReadOnly = true
			(dgObjProps.Columns.item 0).DefaultCellStyle.Alignment = (dgObjProps.Columns.item 0).DefaultCellStyle.Alignment.MiddleLeft
			(dgObjProps.Columns.item 0).DefaultCellStyle.BackColor = clrRowheadBack
			(dgObjProps.Columns.item 1).DefaultCellStyle.Alignment = (dgObjProps.Columns.item 1).DefaultCellStyle.Alignment.MiddleLeft
			dgObjProps.ColumnHeadersDefaultCellStyle.alignment = dgObjProps.ColumnHeadersDefaultCellStyle.alignment.MiddleLeft
			
			(dgObjProps.Columns.item 1).AutoSizeMode = (dgObjProps.Columns.item 1).AutoSizeMode.Fill
			(dgObjProps.Columns.item 0).width = w1
			(dgObjProps.Columns.item 1).width = w2
			
			(dgObjProps.item 1 1).ReadOnly = true  -- Object Type is read-only cell

		-- PANEL: UDP PANEL
			panUDPPanel.anchor = anchorTopLeftRight
			panUDPPanel.Top = dgObjProps.Bottom + 3
			panUDPPanel.Width = panRightPanel.Width
					
				-- BUTTON: >
					butUDPArrow.Top = 0
					butUDPArrow.Left = 4
					butUDPArrow.Width = 16
					butUDPArrow.Height = 16
					butUDPArrow.Text = "+"
					butUDPArrow.FlatStyle = butUDPArrow.FlatStyle.System
				
				-- LABEL: UDP 
					labUDPLabel.Top = butUDPArrow.Top + 1
					labUDPLabel.Left = butUDPArrow.Left + butUDPArrow.Width + 2
					labUDPLabel.Height = 20
					labUDPLabel.Width = panRightPanel.Width
					labUDPLabel.Text = "UDP Buffer:"
		
				-- TEXT BOX: UDP BOX
					tbUDPBox.anchor = anchorTopLeftRight
					tbUDPBox.Top = labUDPLabel.Top + labUDPLabel.Height
					tbUDPBox.multiLine = true
					tbUDPBox.width = panRightPanel.Width + 20
					tbUDPBox.height = 4
					tbUDPBox.backColor = clrStyle[1].backColor
					tbUDPBox.AcceptsReturn = true
					tbUDPBox.AcceptsTab = false
					tbUDPBox.WordWrap = false
					tbUDPBox.ScrollBars = tbUDPBox.ScrollBars.Vertical
					tbUDPBox.AutoSize = false
		
			panUDPPanel.Height = labUDPLabel.Height + tbUDPBox.Height

		-- PANEL: MATERIAL PANEL
			panMatPanel.Anchor = anchorTopLeftRight
			panMatPanel.Top = cbt.panUDPPanel.Bottom + 10
			panMatPanel.Height = 100
			panMatPanel.Width = panRightPanel.Width
		
				-- LABEL: MATERIAL
					labMatLabel.Anchor = anchorTopLeftRight
					labMatLabel.Top = 0
					labMatLabel.Left = 0
					labMatLabel.Height = 18
					labMatLabel.Text = "Materials:"
				
				-- DATAGRID:  MATERIAL LIST
					dgMatList.Anchor = anchorTopLeftRight
					dgMatList.Top = labMatLabel.Bottom
					dgMatList.Width = panRightPanel.Width
					
					matColumn = 4
					dgMatList.ColumnCount = 7
					dgMatList.MultiSelect = false
					dgMatList.RowHeadersVisible = false

					dgMatList.AllowUserToOrderColumns = false
					dgMatList.AllowUserToResizeColumns = false
					
					dgMatList.AllowUserToAddRows = false
					dgMatList.AllowUserToDeleteRows = false
					dgMatList.AllowUserToResizeRows = false
					
					dgMatList.ColumnHeadersHeightSizeMode = dgMatList.ColumnHeadersHeightSizeMode.AutoSize
					
					dgMatList.ShowCellToolTips = false
					dgMatList.ColumnHeadersHeight = dgvRowHeight
					dgMatList.height = 3 * (dgvRowHeight + 1)
					(dgMatList.Columns.item 0).width = 25
					(dgMatList.Columns.item 0).ReadOnly = true
					(dgMatList.Columns.item 0).SortMode = (dgMatList.Columns.item 0).SortMode.NotSortable
					(dgMatList.Columns.item 1).width = 25
					(dgMatList.Columns.item 1).ReadOnly = true
					(dgMatList.Columns.item 1).SortMode = (dgMatList.Columns.item 1).SortMode.NotSortable
					(dgMatList.Columns.item 2).width = 25
					(dgMatList.Columns.item 2).ReadOnly = true
					(dgMatList.Columns.item 2).SortMode = (dgMatList.Columns.item 2).SortMode.NotSortable
					(dgMatList.Columns.item 3).Name = "ID"
					(dgMatList.Columns.item 3).width = 25
					(dgMatList.Columns.item 3).ReadOnly = true
					(dgMatList.Columns.item 3).SortMode = (dgMatList.Columns.item 3).SortMode.NotSortable
					(dgMatList.Columns.item 4).Name = "MATERIAL"
					(dgMatList.Columns.item 4).width = 130
					(dgMatList.Columns.item 4).Resizable = (dgMatList.Columns.item 4).Resizable.true
					(dgMatList.Columns.item 4).AutoSizeMode = (dgMatList.Columns.item 4).AutoSizeMode.Fill
					(dgMatList.Columns.item 4).SortMode = (dgMatList.Columns.item 4).SortMode.NotSortable
					(dgMatList.Columns.item 5).Name = "TYPE"
					(dgMatList.Columns.item 5).width = 100
					(dgMatList.Columns.item 5).ReadOnly = true
					(dgMatList.Columns.item 5).AutoSizeMode = (dgMatList.Columns.item 5).AutoSizeMode.Fill
					(dgMatList.Columns.item 5).SortMode = (dgMatList.Columns.item 5).SortMode.NotSortable
					(dgMatList.Columns.item 6).Name = "PHYS"
					(dgMatList.Columns.item 6).width = 40
					(dgMatList.Columns.item 6).ReadOnly = true
					(dgMatList.Columns.item 6).SortMode = (dgMatList.Columns.item 6).SortMode.NotSortable
					dgMatList.ScrollBars = dgMatList.ScrollBars.None
					makeMaterialList #()

		form.Controls.Add crySplit

		panLeftPanel.Controls.Add panLeftButPanel
			panLeftButPanel.Controls.Add butTVRefresh
			panLeftButPanel.Controls.Add butTVShowHidden
			
		panLeftPanel.Controls.Add panLeftTVPanel

		panLeftTVPanel.Controls.Add tvObjList
		
		panRightPanel.Controls.Add panRightButPanel
			panRightButPanel.Controls.Add labMatId
			panRightButPanel.Controls.Add tbMatId
			panRightButPanel.Controls.Add butMakePhysMeshes
			panRightButPanel.Controls.Add butMakeJoints
			
		panRightPanel.Controls.Add dgObjProps

		panRightPanel.Controls.Add panUDPPanel
			panUDPPanel.Controls.Add butUDPArrow
			panUDPPanel.Controls.Add labUDPLabel
			panUDPPanel.Controls.Add tbUDPBox
		
		panRightPanel.Controls.Add panMatPanel
			panMatPanel.Controls.Add labMatLabel
			panMatPanel.Controls.Add dgMatList
		
		crySplit.Panel1.Controls.Add panLeftPanel
		crySplit.Panel2.Controls.Add panRightPanel

		form.resumeLayout()

		rebuildTreeView()
		updateDataGridData()
		expandTreeView()
	
		initEventHandlers()		
		
		form.visible = false
		form.text =  "CryENGINE Breakable Tool"
		--dotNet.setLifetimeControl form #dotnet
		form.show(maxHwnd)
		form.visible = true
		refreshButImage = undefined
		hideButImage = undefined
		gc()
	)
)


cryBreakablesRemoveCallbacks()
callbacks.RemoveScripts #filePreOpen id:#callback_filePreOpen
callbacks.RemoveScripts #filePostOpen id:#callback_filePostOpen

cbt = cryBreakableTool()
cbt.initUI()

cryBreakablesAddCallbacks()
callbacks.addScript #filePreOpen "try(cryBreak_filePreOpen())catch()" id:#callback_filePreOpen
callbacks.addScript #filePostOpen "try(cryBreak_filePostOpen())catch()" id:#callback_filePostOpen



--createDialog ro 656 400 style:#(#style_resizing,#style_titlebar, #style_sysmenu, #style_minimizebox, #style_maximizebox)


/*
try(removeSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "ro")) catch()
try(cryMaxTools.basic.ROMan.cryAdd "ro" ro #main) catch()
try(addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "ro")) catch()
*/