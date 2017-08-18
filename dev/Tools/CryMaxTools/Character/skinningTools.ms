--######################################################################################
-- CRYMAXTOOLS V2.0
-- SKINNING TOOLS
-- BY SASCHA HERFORT
--######################################################################################

rollout rltCrySkinning "Skinning Tools"
(
	group "Average Weights"
	(
		button btnAverageWeightsExecute "Average Weights" width:110 height:16 align:#left offset:[0,-2] across:2
		button btnAverageWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-2]
	)
	group "Blur Weights"
	(
		spinner spnBlurWeightsRadius "Radius" width:80 height:16 range:[0.1,1000,10] type:#float align:#left offset:[0,-2] across:2
		checkBox chkBlurWeightsAlongEdges "Edgedist." width:80 height:16 align:#right offset:[8,-2] checked:false  tooltip:"Use edge length instead of spacial distance"
		checkbutton ckbtnBlurWeightsOptimize "Optimize" width:100 height:16 align:#left offset:[0,-5] across:2 tooltip:"Click to refresh optimization data"
		checkBox chkBlurWeightsAutoOptimize "Auto" align:#right offset:[0,-5] checked:false tooltip:"Automatically refreshes on object change"
		button btnBlurWeightsExecute "Blur Weights" width:110 height:16 align:#left offset:[0,-5] across:2
		button btnBlurWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-5]  tooltip:"Undo last blur"
	)
	
	group "Backup/Restore Skin"
	(
		button btnBackupSkinData "Backup in scene" width:150 height:16 tooltip:"Backup skinweights for all characters in the scene"
		button btnRestoreSkinData "Restore on selected" width:150 height:16 tooltip:"Restore skinweights for selected (works only if a skindata with following name is present: Skindata_ObjectName)" offset:[0,-5]
		button btnRemoveLODBones "Remove bones in LODs" width:150 height:16 tooltip:"Opens up a dialog to choose chrparams file to remove bones on selected objects automatically - will skin vertices to next valid parent bone)" offset:[0,-5]
	)
	
	group "Fix Open Edge Skinning"
	(
		label lblFixOpenEdgeSkinning1 "Transfer weights on open edges"
		label lblFixOpenEdgeSkinning2 "from first to second selection"
		button btnFixOpenEdgeSkinning "Fix Open Edge Weights" width:150 height:16
	)
	
	group "Grab Weights from Surface"
	(
		spinner spnGrabWeightsBias "Ray Cast Bias" width:160 height:16 range:[-10,10,0.1] type:#float scale:0.01 align:#right offset:[0,-2]
		label lblGrabWeightsDirection "Direction:" offset:[-2,-5] align:#left across:2
		radioButtons rdoGrabWeightsDirection width:100 height:16 labels:#("x", "y", "z", "n") columns:4 default:4 align:#right offset:[-10,-5]
		checkBox chkGrabWeightsFlipRay "Flip Ray" width:80 height:16 align:#right offset:[4,-5] checked:false tooltip:"Cast rays in the inverse direction of the vertices' normal." across:2
		checkBox chkGrabWeightsFlipMesh "Flip Mesh" width:79 height:16 align:#right offset:[12,-5] checked:false tooltip:"Invert normals of mesh to grab weights from."
		button btnGrabWeightsExecute "Grab from Surface" width:110 height:16 align:#left offset:[0,-5] across:2
		button btnGrabWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-5]
	)
	group "Replace Bone"
	(
		button btnReplaceBoneGetOld "< get selected"  width:80 height:16 align:#right offset:[-1,0]
		dropdownlist ddlReplaceBoneOldBone "Old Bone:" items:#("no skin", "selected") selection:1 align:#left offset:[0,-22]
		button btnReplaceBoneGetNew "< get selected"  width:80 height:16 align:#right offset:[-1,0]
		dropdownlist ddlReplaceBoneNewBone "New Bone:" items:#("no skin", "selected") selection:2 align:#right offset:[0,-22]
		button btnReplaceBoneExecute "Replace Bone" width:110 height:16 align:#left offset:[0,-5] across:2
		button btnReplaceBoneRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-5]
	)
	group "Select by Weight"
	(
		dropdownlist ddlSelectByWeightBone "" items:#("no skin selected") selection:1 align:#left offset:[0,-2]
		spinner spnSelectByWeightLowerThreshold "From:" width:80 height:16 range:[0.0,1.0,0.0] type:#float align:#left offset:[1,-5] across:2
		spinner spnSelectByWeightUpperThreshold "To:" width:80 height:16 range:[0.0,1.0,0.0] type:#float align:#left offset:[1,-5]
	)
	group "Bake to Bones"
	(
		label instruct "Select verts in the deforming mesh" offset:[0,-2]
		label instruct2 "these will generate bones" offset:[0,-5]
		button btnBake2Bones "Bake Deformation to Bones" height:16 width:160
		button btnBlendAllBones "Blend All Bones" height:16 width:160 offset:[0,-5]
	)
	
--##########################################################################################
--GLOBALS FOR SKIN TOOLS
--##########################################################################################
	local sMySkin = undefined --skin modifier of currently selected object
	local aSelectedVerts = #() --list of selected vertices
	local aOldVertWeights = #() --2D array storing old weights for all target verts
	local aOldVertBones = #() --2D array storing old bones for all target verts
	local oMyVertsOctreeRoot = undefined --octree root node
	local sMyObject = undefined
	local bBlurOptimize = false --true if optimization is active
	local bBlurEdgeOptimize = false --true if edge optimization is active
	local iOptimizeMaxNodesPerLeaf = 64
	local bBlurWeightsRadiusSpinning = false
	local aOldSelection = #()
	local iCurrentFrame = 0
	local iMyRefFrame = 0
	local fnMyGetWeight = skinOps.GetVertexWeight
	local fnMyGetWeightCount = skinOps.GetVertexWeightCount
	local fnMyGetWeightBoneID = skinOps.GetVertexWeightBoneID
	local aMyVerts = #()
	local sRefFrameChange = undefined
	local aConnections = #() --vertex connections
	local aDistances = #() --distances
	
--##########################################################################################
--FUNCTIONS FOR SKIN TOOLS
--##########################################################################################
	fn fnGetSelectedVerts sTempSkin =
	( --returns list with selected vertices
		local iVertCount = aMyVerts.count --skinOps.GetNumberVertices sTempSkin
		local aTempArray = #()
		for i = 1 to iVertCount do
		(
			if (getSkinOps sel:sTempSkin).IsVertexSelected sTempSkin i == 1 then
			(
				append aTempArray i
			)
		)
		
		aTempArray
	)
	
	fn fnFindBoneIDInWeightList sTempSkin iTempID iTempVert =
	( --returns index of bone in weightlist of vertex or 0 if it cant be found
		local iWeightCount = (getSkinOps sel:sTempSkin).GetVertexWeightCount sTempSkin iTempVert
		local iResult = 0
		for i = 1 to iWeightCount do(
			iBoneID = (getSkinOps sel:sTempSkin).GetVertexWeightBoneID sTempSkin iTempVert i
			if iBoneID == iTempID do(iResult = i)
		)
		
		iResult
	)
	
	struct nVertsOctreeNode
	(--defined octree node struct and its functions
		p3Min, --bounding box minimum point
		p3Max, --bounding box maximum point
		aChildNodes = #(undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined), --children unless node is a leaf
		aNodeVerts = undefined, --vertices if node is a leaf
		iMaxVertsPerLeaf = 20, --maximum amount of vertices before subdivision
		
		fn fnIsVertInBoundingBox p3Min p3Max iVert =
		( --returns true if vertex is within the specified bounding box
			local p3VertexPosition = aMyVerts[iVert].pos
			if p3VertexPosition.x >= p3Min.x and p3VertexPosition.y >= p3Min.y and p3VertexPosition.z >= p3Min.z then
			(
				if p3VertexPosition.x <= p3Max.x and p3VertexPosition.y <= p3Max.y and p3VertexPosition.z <= p3Max.z then
					(
						true
					)
				else(false)
			)
			else(false)
		),
		
		fn fnPopulate =
		( --populates the octree with vertices
			if aNodeVerts.count > iMaxVertsPerLeaf then
			(
				local p3ParentSize = (p3Max - p3Min)*.5
				local p3CurMin
				local p3CurMax
				local aNewNodeVerts = #()
				
				for i = 0 to 1 do
				(
					for j = 0 to 1 do
					(
						for k = 0 to 1 do
						(
							p3CurMin = p3Min + [p3ParentSize.x*i, p3ParentSize.y*j, p3ParentSize.z*k]
							p3CurMax = p3Min + p3ParentSize + [p3ParentSize.x*i, p3ParentSize.y*j, p3ParentSize.z*k]
							local l = 1
							local end = aNodeVerts.count
							aNewNodeVerts = #()
							while l <= end do
							( --go through parent node verts and move them to child if inside current boundingbox
								if (fnIsVertInBoundingBox p3CurMin p3CurMax aNodeVerts[l]) then
								( --if current vertex is in current boundingbox then move it to new child node and remove from parent node
									append aNewNodeVerts aNodeVerts[l]
									deleteItem aNodeVerts l
									end -= 1
								)
								else
								( --else go to next vertex in parent node
									l += 1
								)
							)
							if aNewNodeVerts.count != 0 then
							(
								aChildNodes[i*4 + j*2 + k + 1] = nVertsOctreeNode p3Min:p3CurMin p3Max:p3CurMax aNodeVerts:aNewNodeVerts iMaxVertsPerLeaf:iMaxVertsPerLeaf
								
								aChildNodes[i*4 + j*2 + k + 1].fnPopulate()
							)
						)
					)
				)
				aNodeVerts = undefined
			)
		),
		
		fn fnPrintStatistics iLevel:0 iNodeNumber:0=
		(--print vertex count per node
			iChildNodeNum = 0
			if iLevel == 0 then
			(
				print "### Vertex Octree Statistics ###"
			)
			sIndentation = ""
			for i = 0 to iLevel-1 do
			(
				sIndentation += "  "
			)
			if aNodeVerts == undefined then
			(
				print (sIndentation + "Level " + iLevel as string + "." + iNodeNumber as string + " has children:")
				for i = 1 to aChildNodes.count do
				(
					if aChildNodes[i] != undefined then
					(
						iChildNodeNum += 1
						iChildNodeNum += aChildNodes[i].fnPrintStatistics iLevel:(iLevel+1) iNodeNumber:i
					)
				)
			)
			else
			(
				print (sIndentation + "Level " + iLevel as string + "." + iNodeNumber as string + " has " + aNodeVerts.count as string + " vertices.")
			)
			if iLevel == 0 then
			(
				print ("Total Nodes: " + (iChildNodeNum + 1) as string)
				OK
			)
			else
			(
				iChildNodeNum
			)
		),
		
		fn fnGetAllVerts aArray =
		(--returns all vertices of octree
			if aNodeVerts == undefined then
			(
				for i = 1 to aChildNodes.count do
				(
					aChildNodes[i].fnGetAllVerts aArray
				)
			)
			else
			(
				for iVert in aNodeVerts do
				(
					append aArray iVert
				)
			)
		),
		
		fn fnSphereIntersectBox p3Min p3Max p3Center fRadius =
		(--returns true if the box and sphere intersect
			local fDistanceOnAxis = 0
			local fDistanceSquared = 0
			for i = 1 to 3 do
			(
				local fVal = p3Center[i]
				if fVal < p3Min[i] then
				(
					fDistanceOnAxis = fVal - p3Min[i]
					fDistanceSquared += fDistanceOnAxis*fDistanceOnAxis
				)
				else if fVal > p3Max[i] then
				(
					fDistanceOnAxis = fVal - p3Max[i]
					fDistanceSquared += fDistanceOnAxis*fDistanceOnAxis
				)
			)
			fDistanceSquared <= fRadius*fRadius
		),
		
		fn fnFindVertsInSphere p3Center fRadius aVertArray aMultiplierArray =
		(--returns all vertices in the sphere and their weights according to the distance to the center
			if aNodeVerts == undefined then
			(
				for i = 1 to aChildNodes.count do
				(
					if aChildNodes[i] != undefined then
					(
						if (fnSphereIntersectBox aChildNodes[i].p3Min aChildNodes[i].p3Max p3Center fRadius) then
						(
							aChildNodes[i].fnFindVertsInSphere p3Center fRadius aVertArray aMultiplierArray
						)
					)
				)
			)
			else
			(
				for iVert in aNodeVerts do
				(
					fDist = distance aMyVerts[iVert].pos p3Center
					if fDist < fRadius then
					(
						append aVertArray iVert
						append aMultiplierArray (fRadius - fDist)
					)
				)
			)
		)
	)
	
	fn fnStoredVertexConnections sPoly =
	(--stores vertex connections and distances to arrays: aConnections and aDistances
		local aIndices = #{1..sPoly.numVerts}
		local aPolyVerts = sPoly.verts
		local fnMyGetFacesUsingVert = polyop.getFacesUsingVert
		local fnMyGetVertsUsingFace = polyop.getVertsUsingFace
		
		for iVert in aIndices do
		(
			local aFaces = fnMyGetFacesUsingVert sPoly iVert
			local aVerts = (fnMyGetVertsUsingFace sPoly aFaces) - #{iVert}
			append aConnections aVerts
			
			local aCurrentDistances = #()
			local p3CurrentPos = aPolyVerts[iVert].pos
			for each in aVerts do
			(
				append aCurrentDistances (distance p3CurrentPos aPolyVerts[each].pos)
			)
			append aDistances aCurrentDistances
		)
	)
	
	fn fnBlurUnOptimize =
	(--destroys octree and and disabled optimize flag
		aConnections = #()
		aDistances = #()
		bBlurEdgeOptimize = false
		
		oMyVertsOctreeRoot = undefined
		bBlurOptimize = false
		ckbtnBlurWeightsOptimize.caption = "Optimize"
		ckbtnBlurWeightsOptimize.checked = false
		if sRefFrameChange != undefined then
		(--remove reference change change handler
			deleteChangeHandler sRefFrameChange
		)
	)
	
	fn fnGeometryChange = 
	(--if reference frame changed then unoptimize
		if sMySkin == undefined then
		(
			fnBlurUnOptimize()
		)
		else if sMySkin.ref_frame != iMyRefFrame then
		(
			fnBlurUnOptimize()
		)
	)
	
	fn fnBlurOptimize iMaxNodesPerLeaf =
	(--generates octree and sets optimize flag
		try
		(
			sMyskin = modPanel.getCurrentObject()
			if sMySkin.name != "CrySkin" then
			(
				sMySkin = undefined
			)
		)
		catch
		(
			sMySkin = undefined
		)
		if sMySkin != undefined then
		(
			iOctreeTime = timeStamp()
			iMyRefFrame = sMySkin.ref_frame
			at time iMyRefFrame
			(--optimize at reference frame of skin modifier
				oMyVertsOctreeRoot = undefined
				aBoundingBox = #([0,0,0],[0,0,0])
				local aAllVerts = #()
				for i = 1 to aMyVerts.count do(
					p3Pos = aMyVerts[i].pos
					if p3Pos.x < aBoundingBox[1].x then (aBoundingBox[1].x = p3Pos.x)
					if p3Pos.y < aBoundingBox[1].y then (aBoundingBox[1].y = p3Pos.y)
					if p3Pos.z < aBoundingBox[1].z then (aBoundingBox[1].z = p3Pos.z)
					if p3Pos.x > aBoundingBox[2].x then (aBoundingBox[2].x = p3Pos.x)
					if p3Pos.y > aBoundingBox[2].y then (aBoundingBox[2].y = p3Pos.y)
					if p3Pos.z > aBoundingBox[2].z then (aBoundingBox[2].z = p3Pos.z)
					append aAllVerts i
				)
				oMyVertsOctreeRoot = nVertsOctreeNode p3Min:(aBoundingBox[1]*1.01) p3Max:(aBoundingBox[2]*1.01) aNodeVerts:aAllVerts iMaxVertsPerLeaf:iMaxNodesPerLeaf
				oMyVertsOctreeRoot.fnPopulate()
				
				--disabling optimized edge distance blur - too confusing and hardly useful
				/*
				if chkBlurWeightsAlongEdges.state then
				(--optimize edge connections
					fnStoredVertexConnections $
					bBlurEdgeOptimize = true
				)
				*/
				
				bBlurOptimize = true
				ckbtnBlurWeightsOptimize.caption = "Optimization Active"
				ckbtnBlurWeightsOptimize.state = true
			)
			print ("### Optimization took: " + formattedprint ((timeStamp() - iOctreeTime)/1000.0) format:".3f" + " seconds ###")
			
			sRefFrameChange = when geometry sMyObject changes do fnGeometryChange() --install reference frame change handler
		)
		else fnBlurUnOptimize()
	)
	
	fn fnUpdateSelectByWeightBoneList =
	(--update names in replace bone drop down list
		if sMySkin == undefined or sMyObject == undefined then
		(
			ddlSelectByWeightBone.items = #("no skin selected")
			ddlSelectByWeightBone.selection = 1
			ddlSelectByWeightBone.enabled = false
		)
		else
		(
			local aBoneNames = #()
			for i = 1 to (getSkinOps sel:sMySkin).GetNumberBones sMySkin do
			(
				append aBoneNames ((getSkinOps sel:sMySkin).GetBoneName sMySkin i 1)
			)
			ddlSelectByWeightBone.items = aBoneNames
			ddlSelectByWeightBone.height = aBoneNames.count*14
			ddlSelectByWeightBone.enabled = true
			ddlSelectByWeightBone.selection = 1
		)
	)
	
	fn fnUpdateReplaceBoneLists =
	(--update names in replace bone drop down list
		if sMySkin == undefined or sMyObject == undefined then
		(
			ddlReplaceBoneOldBone.items = #("no skin")
			ddlReplaceBoneOldBone.selection = 1
			ddlReplaceBoneNewBone.items = #("selected")
			ddlReplaceBoneNewBone.selection = 1
			ddlReplaceBoneOldBone.enabled = false
			ddlReplaceBoneNewBone.enabled = false
		)
		else
		(
			local aBoneNames = #()
			for i = 1 to (getSkinOps sel:sMySkin).GetNumberBones sMySkin do
			(
				append aBoneNames ((getSkinOps sel:sMySkin).GetBoneName sMySkin i 1)
			)
			ddlReplaceBoneOldBone.items = aBoneNames
			ddlReplaceBoneNewBone.items = aBoneNames
			ddlReplaceBoneOldBone.height = aBoneNames.count*14
			ddlReplaceBoneNewBone.height = aBoneNames.count*14
			ddlReplaceBoneOldBone.enabled = true
			ddlReplaceBoneNewBone.enabled = true
			ddlReplaceBoneOldBone.selection = 1
			ddlReplaceBoneNewBone.selection = 2
		)
	)
	
	fn fnUpdateSelection =
	(--update variables and octree
		if rltCrySkinning.open == true then
		(
			sMyObject = $
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
			if sMyObject != undefined and sMyskin != undefined then
			(
				aMyVerts = sMyObject.verts
				if chkBlurWeightsAutoOptimize.state == true then
				(
					fnBlurOptimize iOptimizeMaxNodesPerLeaf
				)
				else
				(
					fnBlurUnOptimize()
				)
			)
			else
			(
				fnBlurUnOptimize()
			)
			fnUpdateReplaceBoneLists()
			fnUpdateSelectByWeightBoneList()
		)
	)
	
	fn fnGetVertsByEdgeDistance sPoly iIndex fDistance =
	(--returns a bitArray containing vertices within the distance along the surface
		local fAcrossFaceDistance = sqrt(2)
		local aValidVerts = #{iIndex} --stores vertices that are in range
		local aValidDistances = #() --stores distances for verts in range
		aValidDistances[iIndex] = 0.0
		local aLastVerts = #{iIndex} --stores verts added in last iteration
		local aNewVerts = #{} --stores verts being added in this iteration
		
		while aLastVerts.numberSet > 0 do
		(
			aNewVerts = #{}
			
			for iVert in aLastVerts do
			(
				if not bBlurEdgeOptimize then
				(--only necessary when using fixed edge lengths
					--travel along edges first
					local aEdges = polyop.getEdgesUsingVert sPoly iVert
					local aVerts = polyop.getVertsUsingEdge sPoly aEdges
					aVerts = aVerts - aValidVerts --remove already added verts
					for each in aVerts do
					(
						local fCurrentDistance = aValidDistances[iVert] + 1.0
						if fCurrentDistance <= fDistance then
						(--vert is valid - add it
							aValidVerts[each] = true
							aNewVerts[each] = true
							aValidDistances[each] = fCurrentDistance
						)
					)
				)
				
				--travel along faces second
				if bBlurEdgeOptimize then
				(
					local aVerts = aConnections[iVert]
				)
				else
				(
					local aFaces = polyop.getFacesUsingVert sPoly iVert
					local aVerts = polyop.getVertsUsingFace sPoly aFaces
				)
				aVerts = (aVerts - aValidVerts) as array --remove already added verts
				for i = 1 to aVerts.count do
				(
					if bBlurEdgeOptimize then
					(
						local fCurrentDistance = aValidDistances[iVert] + aDistances[iVert][i]
					)
					else
					(
						local fCurrentDistance = aValidDistances[iVert] + fAcrossFaceDistance
					)
					if fCurrentDistance <= fDistance then
					(--vert is valid - add it
						aValidVerts[aVerts[i]] = true
						aNewVerts[aVerts[i]] = true
						aValidDistances[aVerts[i]] = fCurrentDistance
					)
				)
			)
			
			aLastVerts = aNewVerts
		)
		
		local aResultMultipliers = for each in aValidDistances where each != undefined collect (fDistance - each)
		#(aValidVerts as array,aResultMultipliers)
	)
	
	fn fnSelectVerticesByWeight sBoneName fLowerThreshold fUpperThreshold =
	(--selects vertices with a certain weight to a bone
		--CONTINUE HERE!!!
	)
	
	on rltCrySkinning open do
	(
		local aSkinningToolsSettings = cryMaxTools.basic.settingsMan.load "skinningToolsSettings"
		if aSkinningToolsSettings != false then
		(
			spnBlurWeightsRadius.value = aSkinningToolsSettings[1].value[1] as float
		)
		
		if rltCrySkinning.open then
		(
			fnUpdateSelection()
		)
	)
	
	on rltCrySkinning rolledUp state do
	(
		if state then
		(
			fnUpdateSelection()
		)
		else
		(
			fnBlurUnOptimize()
		)
	)
	
	on rltCrySkinning close do
	(
		local aSkinningToolsSettings = #("skinningToolsSettings")
		append aSkinningToolsSettings ("spnBlurWeightsRadius*@" + spnBlurWeightsRadius.value as string)
		cryMaxTools.basic.settingsMan.save aSkinningToolsSettings
		
		fnBlurUnOptimize()
		callbacks.removeScripts id:#skinningUpdateSelection
	)
	
--##########################################################################################
--AVERAGE WEIGHTS
--##########################################################################################
	on btnAverageWeightsExecute pressed do
	(--average weights
		sMyObject = $
		sMySkin = undefined
		if sMyObject != undefined then
		(
			sMyskin = modPanel.getCurrentObject()
			if sMySkin.name != "CrySkin" then
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			aSelectedVerts = fnGetSelectedVerts sMySkin
			
			if aSelectedVerts.count > 0 then
			(--if vertex selection is not empty proceed
				btnAverageWeightsRestore.enabled = false
				btnBlurWeightsRestore.enabled = false
				btnGrabWeightsRestore.enabled = false
				btnReplaceBoneRestore.enabled = false
				for iVert in aSelectedVerts do
				(--stores old weights for undo function
					aOldWeights = #()
					aOldBones = #()
					for i = 1 to (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin iVert do
					(
						append aOldBones ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
						append aOldWeights ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
					)
					append aOldVertBones aOldBones
					append aOldVertWeights aOldWeights
				)
				
				aBoneIDList = #() --contains a list of all the bones that affect at least one of the selected vertices
				for iVert in aSelectedVerts do( --unnormalize vertices and create aBoneIDList
					(getSkinOps sel:sMySkin).unNormalizeVertex sMySkin iVert true
					iWeightCount = (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin iVert
					for i = 1 to iWeightCount do(
						iBoneID = (getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i
						if (findItem aBoneIDList iBoneID) == 0 do(
							append aBoneIDList iBoneID
						)
					)
				)
				for i = 1 to aBoneIDList.count do(
					local fSum = 0
					for iVert in aSelectedVerts do(
						aCurBoneInWeightList =  fnFindBoneIDInWeightList sMySkin aBoneIDList[i] iVert
						if aCurBoneInWeightList != 0 do(
							fSum += ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin iVert aCurBoneInWeightList)
						)
					)
					for iVert in aSelectedVerts do(
						(getSkinOps sel:sMySkin).SetVertexWeights sMySkin iVert aBoneIDList[i] (fSum/aSelectedVerts.count)
					)
				)
				for iVert in aSelectedVerts do(
					(getSkinOps sel:sMySkin).unNormalizeVertex sMySkin iVert false
				)
				btnAverageWeightsRestore.enabled = true
			)
		)
		--else(messageBox "Please select the CrySkin modifier of an object!")
	)
	
	on btnAverageWeightsRestore pressed do
	(--undo average weights
		for i = 1 to aSelectedVerts.count do
		(
			(getSkinOps sel:sMySkin).unNormalizeVertex sMySkin aSelectedVerts[i] true
			(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
			(getSkinOps sel:sMySkin).unNormalizeVertex sMySkin aSelectedVerts[i] false
		)
		aSelectedVerts = #()
		aOldVertWeights = #()
		aOldVertBones = #()
		btnAverageWeightsRestore.enabled = false
	)
	
--########################################################################################
--BLUR WEIGHTS
--########################################################################################
	on spnBlurWeightsRadius buttondown do
	(--start displaying blur radius preview
		if sMyskin != undefined then
		(
			iMyRefFrame = sMySkin.ref_frame
			bBlurWeightsRadiusSpinning = true
			aOldSelection = fnGetSelectedVerts sMySkin
		)
	)
	
	on spnBlurWeightsRadius changed fNewRadius bSpinning do
	(--update blur radius preview
		if bSpinning and bBlurWeightsRadiusSpinning and bBlurOptimize and not chkBlurWeightsAlongEdges.state then 
		(
			aNewSelection = #()
			for iVert in aOldSelection do
			(
				aTempSelection = #()
				aTempSelectionMultipliers = #()
				at time iMyRefFrame(
					oMyVertsOctreeRoot.fnFindVertsInSphere aMyVerts[iVert].pos fNewRadius aTempSelection aTempSelectionMultipliers
				)
				for iIndex in aTempSelection do
				(
					appendIfUnique aNewSelection iIndex
				)
			)
			(getSkinOps sel:sMySkin).SelectVertices sMySkin aNewSelection
		)
		else if bSpinning and bBlurWeightsRadiusSpinning and bBlurEdgeOptimize then
		(
			local aNewSelection = #{}
			for iVert in aOldSelection do
			(
				at time iMyRefFrame(
					aTempSelection = (fnGetVertsByEdgeDistance $ iVert fNewRadius)[1] as bitArray
				)
				aNewSelection += aTempSelection
			)
			
			(getSkinOps sel:sMySkin).SelectVertices sMySkin aNewSelection
		)
	)
	
	on spnBlurWeightsRadius buttonup do
	(--stop displaying blur radius preview
		if sMySkin != undefined then
		(
			bBlurWeightsRadiusSpinning = false
			(getSkinOps sel:sMySkin).SelectVertices sMySkin aOldSelection
		)
	)
	
	on chkBlurWeightsAlongEdges changed state do
	(--optimize edges if necessary
		fnBlurUnOptimize()
		
		ckbtnBlurWeightsOptimize.enabled = not state
		
		/*
		if state and not bBlurEdgeOptimize and bBlurOptimize then
		(
			fnBlurOptimize iOptimizeMaxNodesPerLeaf
		)
		*/
	)
	
	on ckbtnBlurWeightsOptimize changed state do
	(--build octree for blur optimization
		if state and sMyObject != undefined then
		(
			fnBlurOptimize iOptimizeMaxNodesPerLeaf
		)
		else
		(
			fnBlurUnOptimize()
		)
	)
	
	on btnBlurWeightsExecute pressed do
	(--blur weights
		sMySkin = undefined
		if sMyObject != undefined then
		(
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			iBlurTime = timeStamp()
			aSelectedVerts = fnGetSelectedVerts sMySkin
			
			if aSelectedVerts.count > 0 then
			(--if vertex selection is not empty proceed
				iFindVertsTimeSum = 0
				iGetWeightsTimeSum = 0
				iApplyWeightsTimeSum = 0
				aOldVertWeights = #() --2D array storing old weights for undo function
				aOldVertBones = #() --2D array storing old bones for undo function
				fBlurRadius = spnBlurWeightsRadius.value --blur radius
				aNewVertWeights = #() --2D array storing new weights for all target verts
				aNewVertBones = #() --2D array storing new bones for all target verts
				
				btnAverageWeightsRestore.enabled = false
				btnBlurWeightsRestore.enabled = false
				btnGrabWeightsRestore.enabled = false
				btnReplaceBoneRestore.enabled = false
				for iVert in aSelectedVerts do
				(--stores old weights for undo function
					aOldWeights = #()
					aOldBones = #()
					for i = 1 to (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin iVert do
					(
						append aOldBones ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
						append aOldWeights ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
					)
					append aOldVertBones aOldBones
					append aOldVertWeights aOldWeights
				)
				
				if bBlurOptimize then
				(--if optimized then blur in base pose
					iMyRefFrame = sMySkin.ref_frame
				)
				
				for iTargetVert in aSelectedVerts do
				(--unnormalize vertices and creates aNewVertWeights and aNewVertBones
					aSourceVerts = #() --list of vertices within a certain proximity to selected vertex
					aSourceVertMultipliers = #() --list of multiplier for each source vertex based on proximity to target
					fSourceVertMultiplierSum = 0
					if bBlurOptimize and not chkBlurWeightsAlongEdges.state then
					(--if optimization active then go find vertices in octree
						at time iMyRefFrame
						(--find verts in reference frame
							iFindVertsTime = timeStamp()
							p3TargetPos = aMyVerts[iTargetVert].pos --position of target vertex
							oMyVertsOctreeRoot.fnFindVertsInSphere p3TargetPos fBlurRadius aSourceVerts aSourceVertMultipliers
							for fElement in aSourceVertMultipliers do
							(
								fSourceVertMultiplierSum+= fElement
							)
							iFindVertsTimeSum += timeStamp() - iFindVertsTime
						)
					)
					else if chkBlurWeightsAlongEdges.state then
					(--if edge distance
						at time iMyRefFrame
						(
							iFindVertsTime = timeStamp()
							p3TargetPos = aMyVerts[iTargetVert].pos --position of target vertex
							
							local aResult = fnGetVertsByEdgeDistance $ iTargetVert fBlurRadius
							aSourceVerts = aResult[1]
							aSourceVertMultipliers = aResult[2]
							
							for fElement in aSourceVertMultipliers do
							(
								fSourceVertMultiplierSum+= fElement
							)
							iFindVertsTimeSum += timeStamp() - iFindVertsTime
						)
					)
					else
					(--if not optimized go through all vertices in mesh
						iFindVertsTime = timeStamp()
						p3TargetPos = aMyVerts[iTargetVert].pos --position of target vertex
						for iVert = 1 to aMyVerts.count do( --creates aSourceVerts array
							fCurDist = distance p3TargetPos (sMyObject.verts[iVert].pos)
							if fCurDist < fBlurRadius then(
								append aSourceVerts iVert
								append aSourceVertMultipliers (fBlurRadius - fCurDist)
								fSourceVertMultiplierSum += (fBlurRadius - fCurDist)
							)
						)
						iFindVertsTimeSum += timeStamp() - iFindVertsTime
					)
					
					if fSourceVertMultiplierSum == 0 then
					(
						print ("ERROR: Vertex " + iTargetVert as string + " has no influences!")
						--return()
						fSourceVertMultiplierSum = 1
					)
					
					fSourceVertMultiplierSum = 1/fSourceVertMultiplierSum
					for i = 1 to aSourceVertMultipliers.count do
					(--normalizes aSourceVertMultipliers
						aSourceVertMultipliers[i] *= fSourceVertMultiplierSum
					)
					
					iGetWeightsTime = timeStamp()
					aNewBones = #() --new bones that target vertex will be skinned to
					aNewWeights = #() -- new weights for these bones
					for iSourceVertID = 1 to aSourceVerts.count do
					(--per vertex creates element for aNewVertWeights and aNewVertBones
						iWeightCount = (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin aSourceVerts[iSourceVertID]
						for iWeightIndex = 1 to iWeightCount do
						(
							iBoneID = (getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin aSourceVerts[iSourceVertID] iWeightIndex
							if (appendIfUnique aNewBones iBoneID) == true then
							(--if bone not in list: append bone and weight, else add weight of bone to existing weights
								append aNewWeights (((getSkinOps sel:sMySkin).GetVertexWeight sMySkin aSourceVerts[iSourceVertID] iWeightIndex)*aSourceVertMultipliers[iSourceVertID])
							)
							else
							(
								aNewWeights[findItem aNewBones iBoneID] += ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin aSourceVerts[iSourceVertID] iWeightIndex)*aSourceVertMultipliers[iSourceVertID]
							)
						)
					)
					append aNewVertWeights aNewWeights
					append aNewVertBones aNewBones
					iGetWeightsTimeSum += timeStamp() - iGetWeightsTime
				)
				for i = 1 to aSelectedVerts.count do(
					iApplyWeightsTime = timeStamp()
					(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aNewVertBones[i] aNewVertWeights[i]
					iApplyWeightsTimeSum += timeStamp() - iApplyWeightsTime
				)
				
				print ("### Blurtimes - Find Verts: " + formattedPrint (iFindVertsTimeSum/1000.0) format:".3f" + " - Get Weights: " + formattedPrint (iGetWeightsTimeSum/1000.0) format:".3f"  + " - Apply Weights: " + formattedPrint (iApplyWeightsTimeSum/1000.0) format:".3f"  + " - Total: " + formattedPrint ((timeStamp() - iBlurTime)/1000.0) format:".3f" + " ###")
				btnBlurWeightsRestore.enabled = true
			)
		)
		--else(messageBox "Please select the CrySkin modifier of an editable mesh/poly!")
	)
	
	on btnBlurWeightsRestore pressed do
	(--undo blur weights
		for i = 1 to aSelectedVerts.count do
		(
			(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
		)
		aSelectedVerts = #()
		aOldVertWeights = #()
		aOldVertBones = #()
		btnBlurWeightsRestore.enabled = false
	)
	
	
--##########################################################################################
--BACKUP SKIN 
--##########################################################################################

fn fnBackupSkinData = 
(
	local aPreSel = selection as array 				
	local aPreSubLevel = subobjectlevel
	local pObjects = objects as array
	local pSkinnedObjects = #()
	local pSkinDataObjects = #()
	
	with redraw off
	(
		with undo on
		(
			-- Find Skinned Objects
			for i = 1 to pObjects.count do
			(
				if classOf pObjects[i].modifiers[1] == Skin then
				(
					append pSkinnedObjects pObjects[i]
				)
			)

			try(SkinDataLayer = layermanager.newLayerFromName "skindata")catch()
			try(SkinDataLayer = layermanager.getLayerFromName "skindata")catch()
			-- Delete the objects in the layer
			tempNodes = #()
			SkinDataLayer.nodes &tempNodes
			for i = 1 to tempNodes.count do
			(
				delete tempNodes[i]
			)

			--Backup SkinData
			for i = 1 to pSkinnedObjects.count do
			(
				skinUtils.ExtractSkinData pSkinnedObjects[i]	
				try(append pSkinDataObjects (getNodeByName ("SkinData_" + pSkinnedObjects[i].name)))catch()
			)

			for obj = 1 to pSkinDataObjects.count do
			(
				SkinDataLayer.addnode pSkinDataObjects[obj]
			)
			
			try
			(
				for i = 1 to aPreSel.count do
				(
					selectMore aPreSel[i]
				)
				subobjectlevel = aPreSubLevel
			)catch()
		)
	)
)
on btnBackupSkinData pressed do
(
	fnBackupSkinData()
)
	

on btnRestoreSkinData pressed do
(		
	fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\skinrestore.ms")
)
on btnRemoveLODBones pressed do
(		
	fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\removeBonesFromLOD.ms")
)


on btnFixOpenEdgeSkinning pressed do
(
	fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\skinning_FixOpenEdges.ms")
)


--##########################################################################################
--GRAB WEIGHTS FROM SURFACE
--##########################################################################################
	on btnGrabWeightsExecute pressed do
	(--grab weights from backfacing
		sMyObject = $
		sMySkin = undefined
		if sMyObject != undefined then
		(
			try
			(
				--meshop.flipNormals sMyObject #(1)
				--meshop.flipNormals sMyObject #(1)
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			aMyVerts = sMyObject.verts
			aSelectedVerts = fnGetSelectedVerts sMySkin
			
			if aSelectedVerts.count > 0 then
			(--if vertex selection is not empty proceed
				aNewVertWeights = #() --2D array storing new weights for all target verts
				aNewVertBones = #() --2D array storing new bones for all target verts
				
				btnAverageWeightsRestore.enabled = false
				btnBlurWeightsRestore.enabled = false
				btnGrabWeightsRestore.enabled = false
				btnReplaceBoneRestore.enabled = false
				for iVert in aSelectedVerts do
				(--stores old weights for undo function
					aOldWeights = #()
					aOldBones = #()
					for i = 1 to (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin iVert do
					(
						append aOldBones ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
						append aOldWeights ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin iVert i)
					)
					append aOldVertBones aOldBones
					append aOldVertWeights aOldWeights
				)
				
				iMyRefFrame = sMySkin.ref_frame
				at time iMyRefFrame
				(
					sMySnapShot = snapshot sMyObject
					if chkGrabWeightsFlipMesh.state then	(meshop.flipNormals sMySnapShot #{1..sMySnapShot.numfaces})
					
					for iTargetVert in aSelectedVerts do
					(--creates aNewVertWeights and aNewVertBones
						p3TargetPos = aMyVerts[iTargetVert].pos --position of target vertex
						p3TargetNormal = getNormal sMySnapShot iTargetVert --normal of target vertex
						if chkGrabWeightsFlipMesh.state then	(p3TargetNormal = -p3TargetNormal) --'unflip' normal if mesh has been flipped - we want to use normal of the original mesh
						aNewBones = #() --new bones that target vertex will be skinned to
						aNewWeights = #() --new weights for these bones
						aSourceVerts = #() --source vertices
						aSourceVertMultipliers = #() --multiplier for source vertices
						
						case rdoGrabWeightsDirection.state of
						(--change direction of ray
							1: p3TargetNormal = [p3TargetNormal.X,0,0]
							2: p3TargetNormal = [0,p3TargetNormal.Y,0]
							3: p3TargetNormal = [0,0,p3TargetNormal.Z]
						)
						if chkGrabWeightsFlipRay.state then	(p3TargetNormal = -p3TargetNormal) --flip normal if turned on by user
						p3TargetPos += p3TargetNormal*spnGrabWeightsBias.value --apply ray bias to reduce errors with very close surfaces 
						
						aHit = intersectRayEx sMySnapShot (ray p3TargetPos p3TargetNormal) --works only with Editable_mesh
						if aHit != undefined then
						(
							p3HitFaceVerts = getFace sMySnapShot aHit[2]
							
							--debug draw
							/*
							local aOldDebugDraw = getNodeByName "cryGrabWeightsRays" all:true
							for each in aOldDebugDraw do delete each
							local nNewLine = splineShape()
							nNewLine.name = "cryGrabWeightsRays"
							addNewSpline nNewLine
							addKnot nNewLine 1 #corner #line p3TargetPos
							addKnot nNewLine 1 #corner #line aHit[1].pos
							*/
						)
						else
						(
							aHit = #(undefined,undefined,[1,0,0])
							p3HitFaceVerts = [iTargetVert, iTargetVert, iTargetVert]
						)
						
						for i = 1 to 3 do
						(
							append aSourceVerts (p3HitFaceVerts[i] as integer)
							append aSourceVertMultipliers aHit[3][i]
						)
						
						for iSourceVertID = 1 to 3 do
						(
							iWeightCount = (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin aSourceVerts[iSourceVertID]
							for iWeightIndex = 1 to iWeightCount do
							(
								iBoneID = (getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin aSourceVerts[iSourceVertID] iWeightIndex
								if (appendIfUnique aNewBones iBoneID) == true then
								(--if bone not in list: append bone and weight, else add weight of bone to existing weights
									append aNewWeights (((getSkinOps sel:sMySkin).GetVertexWeight sMySkin aSourceVerts[iSourceVertID] iWeightIndex)*aSourceVertMultipliers[iSourceVertID])
								)
								else
								(
									aNewWeights[findItem aNewBones iBoneID] += ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin aSourceVerts[iSourceVertID] iWeightIndex)*aSourceVertMultipliers[iSourceVertID]
								)
							)
						)
						append aNewVertWeights aNewWeights
						append aNewVertBones aNewBones
					)
				)
				--meshop.flipNormals sMySnapShot #{1..sMySnapShot.numfaces}
				
				for i = 1 to aSelectedVerts.count do(
					(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aNewVertBones[i] aNewVertWeights[i]
				)
				
				delete sMySnapShot
				
				btnGrabWeightsRestore.enabled = true
			)
		)
		else(messageBox "Please select the CrySkin modifier of an editable mesh!")
	)
	
	on btnGrabWeightsRestore pressed do
	(--undo grab weights
		for i = 1 to aSelectedVerts.count do(
			(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
		)
		aSelectedVerts = #()
		aOldVertWeights = #()
		aOldVertBones = #()
		btnGrabWeightsRestore.enabled = false
	)
	
--########################################################################################
--REPLACE BONE
--########################################################################################
	on btnReplaceBoneGetOld pressed do
	(--use currently selected bone as old bone
		sMySkin = undefined
		if sMyObject != undefined then
		(
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			ddlReplaceBoneOldBone.selection = (getSkinOps sel:sMySkin).GetSelectedBone sMySkin
		)
	)
	
	on btnReplaceBoneGetNew pressed do
	(--use currently selected bone as old bone
		sMySkin = undefined
		if sMyObject != undefined then
		(
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			ddlReplaceBoneNewBone.selection = (getSkinOps sel:sMySkin).GetSelectedBone sMySkin
		)
	)
	
	on btnReplaceBoneExecute pressed do
	(--replace bone
		sMySkin = undefined
		if sMyObject != undefined then
		(
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
		)
		if sMySkin != undefined then
		(
			aSelectedVerts = fnGetSelectedVerts sMySkin
			if aSelectedVerts.count > 0 then
			(--if vertex selection is not empty proceed
				btnAverageWeightsRestore.enabled = false
				btnBlurWeightsRestore.enabled = false
				btnGrabWeightsRestore.enabled = false
				btnReplaceBoneRestore.enabled = false
				for iVert in aSelectedVerts do
				(--stores old weights for undo function
					aOldWeights = #()
					aOldBones = #()
					for i = 1 to (getSkinOps sel:sMySkin).GetVertexWeightCount sMySkin iVert do
					(
						append aOldBones ((getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iVert i)
						append aOldWeights ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin iVert i)
					)
					append aOldVertBones aOldBones
					append aOldVertWeights aOldWeights
				)
				
				iOldBone = ddlReplaceBoneOldBone.selection
				iNewBone = ddlReplaceBoneNewBone.selection
				
				--creates aNewVertWeights and aNewVertBones
				for iTargetVert in aSelectedVerts do
				(
					--per vertex creates element for aNewVertWeights and aNewVertBones
					aNewBones = #() --new bones that target vertex will be skinned to
					aNewWeights = #() --new weights for these bones
					iWeightCount = (getSkinOps sel:sMySkin).GetVertexWeightCount sMyskin iTargetVert
					iReplaceID = fnFindBoneIDInWeightList sMyskin iOldBone iTargetVert
					for i = 1 to iWeightCount do
					(
						if i == iReplaceID then
						(
							append aNewBones iNewBone
						)
						else
						(
							iBoneID = (getSkinOps sel:sMySkin).GetVertexWeightBoneID sMySkin iTargetVert i
							append aNewBones iBoneID
						)
						append aNewWeights ((getSkinOps sel:sMySkin).GetVertexWeight sMySkin iTargetVert i)
					)
					(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin iTargetVert aNewBones aNewWeights
				)
				btnReplaceBoneRestore.enabled = true
			)
		)
	)
	
	on btnReplaceBoneRestore pressed do
	(--undo replace bone
		for i = 1 to aSelectedVerts.count do
		(
			(getSkinOps sel:sMySkin).ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
		)
		aSelectedVerts = #()
		aOldVertWeights = #()
		aOldVertBones = #()
		btnReplaceBoneRestore.enabled = false
	)
	
--########################################################################################
--SELECT BY WEIGHT
--########################################################################################
	on spnSelectByWeightLowerThreshold changed value do
	(
		fnSelectVerticesByWeight ddlSelectByWeightBone.selection value spnSelectByWeightUpperThreshold.value
	)
	
	on spnSelectByWeightUpperThreshold changed value do
	(
		fnSelectVerticesByWeight ddlSelectByWeightBone.selection spnSelectByWeightLowerThreshold.value value
	)
--#########################################################################################
--BAKE TO BONES
--#########################################################################################
	on btnBake2Bones pressed do
	(
		if $selection.count > 0 then
		(
			if selectedVerts == undefined or selectedVerts.count < 1 then
			(
				messagebox "No vertices selected"
				return undefined
			)
			
			for i = 1 to selectedVerts.count do
			(
				box name:("bakeBone_" + (selectedVerts[i] as string)) position:((obj1.modifiers[1].getVertex selectedVerts[i]) + [0,0,-.05]) length:1 width:1 height:1
			)
			
			for i = (animationrange.start as integer) to (animationrange.end as integer) do
			(
				with animate on
				(
					slidertime = i
					for m=1 to selectedVerts.count do
					(
						(execute ("$bakeBone_" + selectedVerts[m] as string)).position = ((obj1.modifiers[1].getVertex selectedVerts[m]) + [0,0,-.05])
					)
				)
			)
		)
		else
			messageBox "Select Node before." title:"Error"
	)
	
	on btnBlendAllBones pressed do
	(
		try
		(
			for obj in selection do
			(
				undo "Blend Weights" on
				(
					modPanel.setCurrentObject obj.modifiers[#Skin]
					for i=1 to ((getSkinOps sel:obj.skin).getNumberBones obj.skin) do
					(
						(getSkinOps sel:obj.modifiers[#Skin]).SelectBone obj.modifiers[#Skin] i
						(getSkinOps sel:obj.modifiers[#Skin]).blendSelected obj.modifiers[#Skin]
						print i
					)
				)
			)
		)
		catch()
	)
)

--add skinning tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCrySkinning" rltCrySkinning #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCrySkinning")
callbacks.addScript #modPanelObjPostChange "(cryMaxTools.basic.ROMan.get \"rltCrySkinning\").fnUpdateSelection()" id:#skinningUpdateSelection