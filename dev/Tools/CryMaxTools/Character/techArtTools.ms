-------------------------------------------
-- CryMaxTools v2.0
-- Technical Artist Tools
-- by Chris Evans/Sascha Herfort/Marcus Krautwurst
---------------------------

rollout rltCryTechArt "Tech Art Tools"
(
	group "LOD-Creator"
	(
		button btnLODCreator "Create LODs for selected" height:16 width:160 align:#center
	)
	group "Bone Tools"
	(
		label boneadjustLBL "Adjust Bones:" align:#left across:2
		spinner boneTaper "Taper:" type:#float fieldWidth:25 offset:[0,0] align:#right
		spinner boneWidth "Width:" type:#float fieldWidth:25 range:[0,500,1] offset:[0,-5] align:#left across:2
		spinner boneHeight "Height:" type:#float fieldwidth:25 range:[0,500,1] offset:[0,-5] align:#right
		button nodes2bones "Convert Hierarchy to Bones" height:16 width:160 align:#center offset:[0,3]
		checkbox fromSelected "fromSelected" align:#left checked:true offset:[0,-5] across:2
		checkbox fromRoot "fromRoot" offset:[0,-5] align:#right
	)
	
	group "Transformation Tools"
	(
		button btnTransformResetHierarchy "Reset XForm Hierarchy" offset:[0,-2] height:16 width:160 align:#center
		
		button btnTransformControllerXferChild "Copy/Paste Controllers" offset:[0,3] height:16 width:160 align:#center		
		checkbox copyPos "Pos" checked:true offset:[0,-5] align:#left across:3
		checkbox copyRot "Rot" checked:true offset:[0,-5] align:#center
		checkbox copyScale "Scale" checked:false enabled:false offset:[0,-5] align:#right
		checkbox toChildren "to children" checked:true offset:[0,-5] align:#left across:2
		checkbox toSelection "to selected" enabled:false offset:[0,-5] align:#right
	)
	
	group "CDF Tools"
	(
		button btnExtractNodeToCDF "Selection to Attachment" height:16 width:160
		
	)

--tech art tools global funnctions
	fn fnNodes2Bones node =
	(
		try
		(
			if node.children != undefined then
			(
				newBones = #()
				
				for obj in (append (cryMaxTools.getChildren node) node) do
				(
					for child in obj.children do
					(
						if child.children != undefined then
						(
							test = BoneSys.createBone obj.transform.pos child.transform.pos obj.transform[3]
							test.name = (obj.name + "_bone")
							append newBones test
						)
					)
				)
				for obj in newBones do
				(
					try (obj.parent =  (getnodebyname (((getnodebyname (crytools.cutstring obj.name "_bone")).parent).name + "_bone"))) catch()
				)
			)
		)
		catch()
	)
	
	fn fnResetChildren sTempNode =
	(--resetXforms recursively all children of node and node itself
		local aOldChildren = #()
		for sObj in sTempNode.children do
		(
			append aOldChildren sObj
			deleteItem sTempNode.children sObj
		)
		ResetXForm sTempNode
		collapseStack sTempNode
		if aOldChildren.count > 0 then
		(
			for sObj in aOldChildren do
			(
				append sTempNode.children sObj
				fnResetChildren sObj
			)
		)
	)
	
	fn fnFindRoot sNode =
	(--finds root of node's hierarchy
		if sNode.parent == undefined then
		(
			local result = sNode
		)
		else
		(
			local result = fnFindRoot sNode.parent
		)
		return result
	)
	
	fn copyPasteController parentObj selOrChild what = 
	(
		if selOrChild == "children" then
		(
			for obj in parentObj.children do
			(
				obj.parent = undefined
				
					--// if p exists
				if (findString what "p") != undefined then
					obj.position.controller = parentObj.position.controller
					--// if r exists
				if (findString what "r") != undefined then
					obj.rotation.controller = parentObj.rotation.controller
			)
		)
	)
	
--tech art tools UI event handlers
	-- bone tools
	
	--LOD-Creator
	on btnLODCreator pressed do
	(
		fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\lodcreator.ms")
	)

	
	
	
	
	
	on boneWidth changed val do
	(
		if $ != undefined then
		(
			for obj in selection do
			(
				if classof obj == BoneGeometry then obj.width = val
			)
		)
	)
		
	on boneHeight changed val do
	(
		if $ != undefined then
		(
			for obj in selection do
			(
				if classof obj == BoneGeometry then obj.height = val
			)
		)
	)
	
	on nodes2bones pressed do
	(
		undo "nodes2bones" on
		(
			if $ == undefined then
			(
				messagebox "please select a node"
				return undefined
			)
			
			if fromSelected.checked == false then
			(
				fnNodes2Bones (cryMaxTools.findRoot $)
			)
			else
			(
				fnNodes2Bones $
			)
		)
	)
	
	on fromSelected changed state do
	(
		if fromselected.checked == true then fromRoot.checked = false
	)
	
	on fromRoot changed state do
	(
		if fromRoot.checked == true then fromSelected.checked = false
	)

	on boneTaper changed val do
	(
		if $ != undefined then
		(
			for obj in selection do
			(
				if classof obj == BoneGeometry then obj.taper = val
			)
		)
	)
	
	on btnTransformResetHierarchy pressed do
	(-- resetXform hierarchy
		if $ != undefined then
		(
			undo "Reset XForm Hierarchy" on
			(
				aMyHierarchy = $
				sMyRoot = fnFindRoot aMyHierarchy[1]
				fnResetChildren sMyRoot
			)
		)
	)
	
	on btnTransformControllerXferChild pressed do
	(--controller copy/paste
		if toChildren.checked == true then
		(
			undo "copy/paste controller" on
			(
				for mObj in selection do
				(
					if copyPos.checked and copyRot.checked == true then
					(
						copyPasteController mObj "children" "rp"
					)
				)
			)
		)
		if toSelection.checked == true then
		(
			
		)
	)
	
	
	on btnExtractNodeToCDF pressed do
	(
		local tempSel = #()
		if selection.count > 0 then
			tempSel = getCurrentSelection()
		else
			if (local tempSel2 = selectByName title:"Select Nodes to add/change in the CDF") != undefined then
				tempSel = (tempSel2) as Array
		
		if tempSel.count > 0 then
		(
			local parentBone = selectByName title:"Select the Node to Align the attachment to" single:true
			
			
			local cdfPath = getOpenFileName "Select CDF"
			if cdfPath != undefined then
			(
				local xmlFile = dotNetObject "system.xml.xmlDocument"
				xmlFile.load cdfPath
				
				local fileRoot = xmlFile.documentElement
				local attachmentRoot = fileRoot.childNodes.itemOf[1]
				
				for i = 1 to tempSel.count do
				(
					local obj = tempSel[i]
					local attachmentFound = -1
					
					for d = 0 to (attachmentRoot.childNodes.count - 1) do
					(
						if attachmentRoot.childNodes.itemOf[d].attributes.itemOf[0].value == obj.name then
						(
							attachmentFound = d
							exit
						)
					)
					
					
					local rot = obj.transform.rotation
					rot = rot.x as String + "," + rot.y as String + "," + rot.z as String + "," + rot.w as String
					
					local pos = (obj.transform.pos / 100.0) as String
					pos = subString pos 2 (pos.count - 2)
					
					if parentBone != undefined then
						parentBone = parentBone.name
					else
						parentBone = ""
					
					
					if attachmentFound >= 0 then
					(
						attachmentRoot.childNodes.itemOf[attachmentFound].attributes.itemOf[2].value = rot
						attachmentRoot.childNodes.itemOf[attachmentFound].attributes.itemOf[3].value = pos
						attachmentRoot.childNodes.itemOf[attachmentFound].attributes.itemOf[4].value = parentBone
					)
					else
					(
						local tempAttach = xmlFile.createElement "Attachment"
						tempAttach.setAttribute "AName" obj.name
						tempAttach.setAttribute "Type" "CA_BONE"
						tempAttach.setAttribute "Rotation" rot
						tempAttach.setAttribute "Position" pos
						tempAttach.setAttribute "BoneName" parentBone
						tempAttach.setAttribute "Bindings" ""
						tempAttach.setAttribute "Flags" "0"
						
						attachmentRoot.appendChild tempAttach
					)
				)
				
				xmlFile.save cdfPath
				
				messageBox "Updated CDF!"
			)
		)
	)
	
)
--add tech-art tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryTechArt" rltCryTechArt #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryTechArt")