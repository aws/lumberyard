--########## Find open edges in the model and copy the skinweights from the source to the target mesh onto vertices that share the same position
--########## by Marcus Krautwurst
try(destroyDialog rltFixOpenEdges )catch()

	iTreshold = 0.1
	aEdgeVertsSrc = #()
	aEdgeVertsTarg = #()
	aSharedVerts = #()
	aWeightSrc = #()
	aBonesAdd = #()
	pObjectSrc = undefined
	pObjectTarg = undefined
	
	struct sSharedVerts (src,targ)
	struct aVerts (id,pos)
	struct aWeights (aBoneIDs = #(),aWeights = #(),aBoneNames = #())
	
rollout rltFixOpenEdges "Fix Open Edges SkinWeight"
(	

	fn fnSkinMeshFilter pTarget = 
	(
		if pTarget.modifiers[#CrySkin] != undefined then true else false
	)

	fn MaxRedraw arg =
	(
		if arg == 1 then(cui.CommandPanelOpen = true;enableSceneRedraw())else (disableSceneRedraw();max create mode)	
		WM_SETREDRAW=0xB
		commandHWND = (windows.getChildHWND #max "Command Panel")
		windows.sendmessage (commandHWND[1]) WM_SETREDRAW arg 0
	)

	fn GetEdgeVerts pTarget = 
	(
		--This function returns all open edges
		aTarget = #()	
		pTarget.EditablePoly.SetSelection #Vertex #{}
		modPanel.setCurrentObject pTarget.baseObject
		subobjectlevel = 3
		max select all 	
		pTarget.EditablePoly.ConvertSelection #Border #Vertex
		subObjectLevel = 1
		local aVertIDs = (pTarget.GetSelection #vertex) as array
		
		for v = 1 to aVertIDs.count do
		(
			local tempStruct = aVerts id:aVertIDs[v] pos:pTarget.verts[aVertIDs[v]].pos			
			append aTarget tempStruct
		)	
		
		pTarget.EditablePoly.SetSelection #Vertex #{}
		subobjectLevel = 0	
		return aTarget
	)


	fn findSharedVertPos  =
	(
		--this function checks for shared positions on both meshes
		for i = 1 to aEdgeVertsSrc.count do
		(		
			for v = 1 to aEdgeVertsTarg.count do
			(						
				if length (aEdgeVertsSrc[i].pos-aEdgeVertsTarg[v].pos) <= iTreshold then
				(
					append aSharedVerts (sSharedVerts src:aEdgeVertsSrc[i].id targ:aEdgeVertsTarg[v].id)
				)
			)
		)
	)


	fn transferWeight =	
	(
		--this function copys the weights
		struct aWeights (aBoneIDs = #(),aWeights = #(),aBoneNames = #())
		local aWeightSrc = #()
		local aBoneNamesSrc = #()
		local aBoneNamesTarg = #()	
			
		SkinModSrc = pObjectSrc.modifiers[#CrySkin]
		SkinModTarg = pObjectTarg.modifiers[#CrySkin]
		
		select pObjectSrc
		subobjectlevel = 1
			
		
		-- Add bones to target that are not in the skinmod there yet
		
		iBoneCntSrc = skinOps.GetNumberBones SkinModSrc

		
		for each = 1 to iBoneCntSrc do
		(
			append aBoneNamesSrc (skinOps.GetBoneName SkinModSrc each 1)
		)
		-- Collect Bones from Source Mesh
		iBoneCnt = skinOps.GetNumberBones SkinModSrc	
		for each = 1 to iBoneCnt do
		(
			appendIfUnique aBoneNamesSrc (skinOps.GetBoneName SkinModSrc each 1)
		)
		-- Collect Bones from Target Mesh
		iBoneCnt = skinOps.GetNumberBones SkinModTarg
		for each = 1 to iBoneCnt do
		(
			appendIfUnique aBoneNamesTarg (skinOps.GetBoneName SkinModTarg each 1)
		)
		
		-- Start to add the bones that miss in the target
		for SrcBone = 1 to aBoneNamesSrc.count do
		(		
			findBone = findItem aBoneNamesTarg aBoneNamesSrc[SrcBone]
			if findBone == 0 then
			(
				appendIfUnique aBonesAdd aBoneNamesSrc[SrcBone]			
			)
		)
		
		for i = 1 to aBonesAdd.count do
		(
			skinops.addbone SkinModTarg (getNodeByName aBonesAdd[i]) 1		
		)
		
		-- Refresh the bone-array
		newBoneCnt = skinOps.GetNumberBones SkinModTarg
		for each = 1 to newBoneCnt do
		(
			appendIfUnique aBoneNamesTarg (skinOps.GetBoneName SkinModTarg each 1)
		)
		
		
		for b = 1 to aSharedVerts.count do
		(			
			getVertBoneInf = skinOps.GetVertexWeightCount SkinModSrc aSharedVerts[b].src
			aTempWeights = aWeights()
			for i = 1 to getVertBoneInf do
			(1
				local getBoneID = skinOps.GetVertexWeightBoneID SkinModSrc aSharedVerts[b].src i
				local getVertWeight = skinOps.GetVertexWeight SkinModSrc aSharedVerts[b].src i
				local getBoneName = skinOps.GetBoneName SkinModSrc getBoneID 1
				if getVertWeight > 0.001 then
				(
					appendIfUnique aTempWeights.aWeights getVertWeight
					appendIfUnique aTempWeights.aBoneNames getBoneName
				)
			)
			append aWeightSrc aTempWeights
		)

		select pObjectTarg
		subobjectlevel = 1
		

		
		for each = 1 to aSharedVerts.count do
		(
			for b = 1 to aWeightSrc[each].aBoneNames.count do
			(
				tempFound = finditem aBoneNamesTarg aWeightSrc[each].aBoneNames[b]
				appendIfUnique aWeightSrc[each].aBoneIDs tempFound
			)		
		)
		
		for i = 1 to aSharedVerts.count do
		(
			skinOps.ReplaceVertexWeights SkinModTarg aSharedVerts[i].targ aWeightSrc[i].aBoneIDs aWeightSrc[i].aWeights			
		)		
	)
	
	pickbutton pb_Src "Select Source" width:170 height:15 offset:[-1,0] filter:fnSkinMeshFilter
	pickbutton pb_Tgt "Select Target" width:170 height:15 offset:[0,-3] filter:fnSkinMeshFilter
	spinner spn_Tolerance "Tolerance" range:[0.0,10.0,0.1] type:#float across:2 tooltip:"Tolerance between vertices, 0 only copys weight if vertices share the exact same position"
	button btn_OK "Transfer" offset:[5,-3] width:82 height:19
	
	on rltFixOpenEdges open do
	(		
		MaxRedraw 1
		gc()		
	)
	
	on btn_OK pressed do
	(			
		if pObjectSrc != undefined and pObjectTarg != undefined do 
		(
			iTreshold = rltFixOpenEdges.spn_Tolerance.value				
			MaxRedraw 0
			
			select pObjectSrc
			max modify mode 
			aEdgeVertsSrc = GetEdgeVerts pObjectSrc 
			aEdgeVertsTarg = GetEdgeVerts pObjectTarg
			findSharedVertPos()
			transferWeight()
			
			select #(pObjectSrc,pObjectTarg)
			deselect objects
			MaxRedraw 1
			cui.commandpanelopen = true		
		)
	)
	
	on pb_Src picked pNode do
	(
		pObjectSrc = pNode
		pb_Src.text = "Source: " + pNode.name
	)
	
	on pb_Tgt picked pNode do
	(
		pObjectTarg = pNode
		pb_Tgt.text = "Target: " + pNode.name
	)
	
)


createDialog rltFixOpenEdges 180 60 style:#(#style_toolwindow,#style_sysmenu)
