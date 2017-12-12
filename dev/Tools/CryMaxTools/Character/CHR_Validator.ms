-------------------------------------------
-- CHR-Validator
-- Exporter Tools
-- by Marcus Krautwurst
---------------------------

max modify mode
try(destroyDialog rltCHRValidator)catch()

pValidationNode = $
NoExportNodes = #()
aSkinnedBones = undefined
aSkinMeshRoot = undefined 
ExportHierarchy = undefined
aZeroWeights = #()
intProblems = 0
aDuplicates = false
aDegenerateFaces = #()
aVertexProximity = #()
aVertexProximityCount = undefined
aVertexColorBackup = #()


rollout rltCHRValidator ""
(
	fn fnBackupVertexColor pTarget =
	(
		try
		(
			local aVertColors = #()
			for v = 1 to pTarget.verts.count do
			(
				append aVertColors (polyOp.getMapVert pTarget 0 v)
			)	
		)catch()
		return aVertColors	
	)
	
	fn fnRestoreVertexColor pTarget aVertexColors=
	(
		for i = 1 to aVertexColors.count do
		(
			polyop.setvertcolor pTarget -0 (pTarget.verts[i]) aVertexColors[i]
			polyop.setvertcolor pTarget -1 (pTarget.verts[i]) aVertexColors[i]
		)
	)
	
	fn fnCheckForZeroSkinWeights pTarget =
	(	
		gc()
		max modify mode
		
		local pSkin = pTarget.modifiers[#CrySkin]		
		local aPoints= #()

		for vert = 1 to pTarget.verts.count do
		(
			intInfluence = skinOps.GetVertexWeightCount pSkin vert
			if intInfluence == 0 then append aZeroWeights vert
		)
		if aZeroWeights.count > 0 then
		return aZeroWeights else return undefined
	)
	
	fn fnFixZeroWeights pTarget =
	(
		local aPoints = #()
		if aZeroWeights.count > 0 then 
		(
			for v = 1 to aZeroWeights.count do
			(
				tmpHelper = point pos:pTarget.verts[aZeroWeights[v]].pos wirecolor:yellow size:20 box:false cross:false axistripod:true		
				append aPoints tmpHelper
			)			
		)	
		return aPoints
	)

	fn fnGetBones skinMesh = 
	(
		max modify mode
		select skinMesh
		if skinMesh.modifiers["CrySkin"] != undefined then
			SkinMod = skinMesh.modifiers["CrySkin"]
		else
			if skinMesh.modifiers["Skin"] != undefined then
				SkinMod = skinMesh.modifiers["Skin"]
			
		local aBones = #()
		local boneCount = skinOps.getNumberBones SkinMod

		for i = 1 to boneCount do
		(
			append aBones (skinOps.GetBoneName SkinMod i 1)	
		)		
		return aBones
	)

	fn fnGetRootNode pNode = 
	(	
		local pParent = pNode.parent
		if pParent != undefined then
		(
			while pParent != undefined do
			(
				pParent2 = pParent.parent
				if pParent2 == undefined then return pParent else pParent = pParent2
			)
		)
		else return pNode	
	)

	fn fnGetHierarchy obj &arr:#() =
	(
		for child in obj.children do
		(
			if child.name[1] != "_" then
			(
				append arr child
				fnGetHierarchy child arr:arr
			)
		)
		return arr
	)
	
	fn fnCheckDegenerateFaces =
	(
		gc light:true
		local fThreshold = 0.001 --face area threshold - RC uses 0.0
		local aPoints = #()
		local aDegenerateFaces = #{}	
		
		local aObjectSet = for each in $selection where classOf each == editable_poly or classOf each == polyMeshObject collect each
		
		for nMesh in aObjectSet do
		(
			local nTriangulatedCopy = snapshot nMesh
			convertTo nTriangulatedCopy Editable_Poly
			nTriangulatedCopy.selectedVerts = nTriangulatedCopy.verts --select all vertices
			nTriangulatedCopy.connectVertices()				
			
			for each in nTriangulatedCopy.faces do
			(
				if polyOp.getFaceArea nTriangulatedCopy each.index <= fThreshold then
				(				
					append aDegenerateFaces each.index
				)
			)
			
			local aVertsOnDegradedFaces = polyop.getVertsUsingFace nTriangulatedCopy aDegenerateFaces	
			
			
			if (aDegenerateFaces as array).count > 0 then
			(
				polyop.setvertcolor nMesh -0 (nMesh.verts) (color 0 255 0)
				polyop.setvertcolor nMesh -1 (nMesh.verts) (color 0 255 0)
				
				polyop.setvertcolor nMesh -0 aVertsOnDegradedFaces (color 255 0 0)		
				polyop.setvertcolor nMesh -1 aVertsOnDegradedFaces (color 255 0 0)					
			)
			
			delete nTriangulatedCopy
			return aVertsOnDegradedFaces as array
		)
		return aDegenerateFaces as array
	)

	fn fnCheckForDuplicates theArray =
	(
		local aDuplicates = #()
		for each = 1 to theArray.count do
		(
			for i = 1 to theArray.count do
			(
				if theArray[each].name == theArray[i].name and theArray[each] != theArray[i] then append aDuplicates theArray[each] 
			)			
		)
		if aDuplicates.count > 0 then
			return aDuplicates 
		else return false
	)	
	
	fn fnCheckVertexDistance pTarget=
	(--warn if vertices too close to each other
		local fThreshold = 0.2 --in centimeters - RC uses 0.2
		local aVerticesTooClose = #()
		
		local sClass = classOf pTarget
		if sClass == Editable_Poly or sClass == Editable_Mesh or sClass == PolyMeshObject then
		(
			local aObjectVerticesTooClose = #{}
			local aVertices = for each in pTarget.verts collect each
			
			fn fnSortByZ v1 v2 =
			(
				z1 = v1.pos.z
				z2 = v2.pos.z
				case of
				(
					(z1 > z2): 1
					(z1 < z2): -1
					default:0
				)
			)
			
			qsort aVertices fnSortByZ --sort vertices by z coordinate
			
			while aVertices.count > 0 do
			(
				local sCurrentVertex = aVertices[1]
				deleteItem aVertices 1
				
				local fDistanceZ = 0.0
				local iIndex = 1
				while fDistanceZ < fThreshold and iIndex <= aVertices.count do
				(
					if (distance sCurrentVertex.pos aVertices[iIndex].pos) < fThreshold then
					(
						append aObjectVerticesTooClose sCurrentVertex.index
						append aObjectVerticesTooClose aVertices[iIndex].index
					)
					
					if iIndex < aVertices.count then
					(
						fDistanceZ = aVertices[iIndex+1].pos.z - sCurrentVertex.pos.z
					)
					else
					(
						fDistanceZ = 1000.0
					)
					
					iIndex += 1
				)
			)
		)
		
		if aObjectVerticesTooClose.numberSet > 0 then
		(
			return aObjectVerticesTooClose
		)
		else
		(
			return undefined
		)
	)
	
	fn InitUI dnControl = 
	(
		local BoldFont = dotNetObject "System.Drawing.Font" "Arial" 12 (dotNetClass "System.Drawing.FontStyle").Bold (dotNetClass "System.Drawing.GraphicsUnit").Pixel		
		dnControl.forecolor = (dotnetClass "System.Drawing.Color").fromARGB 0 0 0
		dnControl.backColor = BGColor
		dnControl.font = BoldFont
	)
	
	fn InitTitle dnControl = 
	(
		local BoldFont = dotNetObject "System.Drawing.Font" "Arial" 14 (dotNetClass "System.Drawing.FontStyle").Bold (dotNetClass "System.Drawing.GraphicsUnit").Pixel		
		dnControl.forecolor = (dotnetClass "System.Drawing.Color").fromARGB 255 255 255
		local getUICol = (colorman.getcolor #background)*255
		dnControl.backColor = (dotnetClass "System.Drawing.Color").fromARGB getUICol[1]  getUICol[2] getUICol[3]
		dnControl.font = BoldFont
	)
	
	fn fnGenImage width height col = 
	(
		local img = bitmap width height color:col
		try
		(
			for y = 1 to height do
			(
				iGetPixels = getPixels img [0,y-1] width
				for x = width to 1 by -1 do
				(
					iGetPixels[x] *= (x+y)/(width as float)+1.2
				)
				setPixels img [0,y-1] iGetPixels
			)
		)catch()
		
		setclipboardBitmap img
		clipboardClass = dotNetClass "System.Windows.Forms.Clipboard"		
		theImage = clipboardClass.getImage()
		return theImage
	)
	
	local BGColor_R = (dotnetClass "System.Drawing.Color").fromARGB 218 74 74
	local BGColor_R_mxs = (color 218 74 74)
	
	local BGColor_G = (dotnetClass "System.Drawing.Color").fromARGB 85 216 59	
	local BGColor_G_mxs = (color 85 216 59)	
	
	local Horizontal_Spacing = [0,5]
	
	
	dotNetControl dnLBL_Title "System.Windows.Forms.Label" text:"CHR Validation - "  height:20 offset:Horizontal_Spacing
	dotnetControl dnLBL_Hierarchy "System.Windows.Forms.Label" text:"Hierarchy Validation"  offset:Horizontal_Spacing
	dropdownlist  BoneExportList items:#() fieldWidth:220 height:25
	dotnetControl dnLBL_VertexProximity "System.Windows.Forms.Label" text:"Vertices Validation"
	label lblVertProximityStatus "" align:#left across:2
	checkbutton btnVertexProximityShow "show" height:15 width:30 visible:false offset:[40,0]
	dotnetControl dnLBL_Faces "System.Windows.Forms.Label" text:"Faces Validation"
	label lblVerticesStatus "" align:#left across:2
	checkbutton btnDegenerateFacesShow "show" height:15 width:30 visible:false offset:[40,0]
	dotnetControl dnLBL_Skinweights "System.Windows.Forms.Label" text:"Skinweights Validation"  offset:Horizontal_Spacing
	label lblZeroWeightsStatus  align:#left across:2
	button btnZeroWeightsFix "show" height:15 offset:[40,0] width:30 visible:false
	dotnetControl dnLBL_Duplicates "System.Windows.Forms.Label" text:"Joint Names Validation"  offset:Horizontal_Spacing
	label lblDuplicates align:#left across:2
	button btnDuplicatesFix "show" height:15 offset:[40,0] width:30 visible:false	
	
	
	
	button btnOk "OK" width:110 offset:Horizontal_Spacing across:2
	button btnReCheck "RE-CHECK" width:110 offset:Horizontal_Spacing
	
	
	
	on rltCHRValidator open do
	(
		if $ != undefined and selection.count == 1 then
		(			
			aSkinnedBones = rltCHRValidator.fnGetBones pValidationNode
			aSkinMeshRoot = rltCHRValidator.fnGetRootNode (getNodeByName aSkinnedBones[1])
			ExportHierarchy = rltCHRValidator.fnGetHierarchy aSkinMeshRoot arr:#(aSkinMeshRoot)	
			
			InitUI dnLBL_Faces
			InitUI dnLBL_Hierarchy
			InitUI dnLBL_Skinweights
			InitUI dnLBL_Duplicates
			InitUI dnLBL_VertexProximity
			
			aVertexColorBackup =  fnBackupVertexColor pValidationNode			
			InitTitle dnLBL_Title			 
			rltCHRValidator.title = (pValidationNode.name + ".chr")
			
			--***************** CHECK: Vertex Proximity ********************
			aVertexProximity = fnCheckVertexDistance pValidationNode
			if aVertexProximity != undefined and aVertexProximity.count > 0 then 
			(
				intProblems += 1
				dnLBL_VertexProximity.text += " - FAIL!"				
				dnLBL_VertexProximity.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_R_mxs)
				lblVertProximityStatus.text = (aVertexProximity.numberSet as string + " vertices are too close to each other!")
				btnVertexProximityShow.visible = true
			)
			else
			(				
				dnLBL_VertexProximity.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_G_mxs)
				dnLBL_VertexProximity.text += " - OK!"
			)							
			--***************** CHECK: Degenerate Faces ********************
			aDegenerateFaces = fnCheckDegenerateFaces()
			if aDegenerateFaces.count > 0 then 
			(
				intProblems += 1
				dnLBL_Faces.text += " - FAIL!"				
				dnLBL_Faces.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_R_mxs)
				lblVerticesStatus.text = (aDegenerateFaces.count as string + " vertices cause degenerate faces!")
				btnDegenerateFacesShow.visible = true
			)
			else
			(
				dnLBL_Faces.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_G_mxs)
				dnLBL_Faces.text += " - OK!"			
			)		
			--***************** CHECK: Hierarchy ********************			
			local aBoneExportList = #()
			if ExportHierarchy.count > 255 then
			(
				aBoneExportList = #("-----[    Bone-Limit of 255 exceeded     ]-----")
			)			
			
			append aBoneExportList ("---------[     " + ExportHierarchy.count as string + " joints in hierarchy    ]---------")
			
			for i = 1 to ExportHierarchy.count do
			(
				append aBoneExportList ExportHierarchy[i].name
			)
-- 			sort aBoneExportList
			BoneExportList.items = aBoneExportList	
			
			if ExportHierarchy.count > 255 then
			(
				intProblems += 1
				dnLBL_Hierarchy.text += " - FAIL!"
				dnLBL_Hierarchy.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_R_mxs)
			)
			else
			(
				dnLBL_Hierarchy.text += " - OK!"							
				dnLBL_Hierarchy.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_G_mxs)

			)		
			--***************** CHECK: Valid Skinweights ********************
			
			aCheckForZeroWeights = fnCheckForZeroSkinWeights $		
			if aCheckForZeroWeights == undefined then
			(
				dnLBL_Skinweights.text += " - OK!"		
				dnLBL_Skinweights.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_G_mxs)
			)		
			else
			(
				intProblems += 1
				dnLBL_Skinweights.text += " - FAIL!"
				dnLBL_Skinweights.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_R_mxs)
				lblZeroWeightsStatus.text = (aCheckForZeroWeights.count as string+ " vertices have invalid skinning")				
				btnZeroWeightsFix.visible = true
			)
			
			--********************* CHECK: Joints  *************************
			aDuplicates = fnCheckForDuplicates ExportHierarchy
			if aDuplicates != false then
			(
				intProblems += 1
				dnLBL_Duplicates.text += " - FAIL!"
				dnLBL_Duplicates.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_R_mxs)
				lblDuplicates.text = ("Some joints have duplicate names!")
				btnDuplicatesFix.visible = true
			)
			else
			(
				dnLBL_Duplicates.backGroundImage = (fnGenImage dnLBL_VertexProximity.width dnLBL_VertexProximity.height BGColor_G_mxs)
				dnLBL_Duplicates.text += " - OK!"			
			)			
			
			
			
			if intProblems == 0 then
			(
				dnLBL_Title.text += "Awesome!"
			)
			if intProblems == 1 then
			(
				dnLBL_Title.text += "1 problem"
			)
			if intProblems > 1 then
			(			
				dnLBL_Title.text += (intProblems as string + " problems")	
			)
		)
		else
		(			
			destroyDialog rltCHRValidator
			messagebox "Please select a skinned mesh" title:"Skinned Bones have not been found!"
		)
	)
	
	
	on btnVertexProximityShow changed state do
	(
		if state == true then
		(
			btnDegenerateFacesShow.state = false
			polyop.setvertcolor pValidationNode -0 pValidationNode.verts BGColor_G_mxs
			polyop.setvertcolor pValidationNode -0 aVertexProximity BGColor_R_mxs	
			polyop.setvertcolor pValidationNode -1 aVertexProximity BGColor_R_mxs
			setCVertMode pValidationNode state
			pValidationNode.xray = state
		)
		else
		(
			pValidationNode.xray = state
			setCVertMode pValidationNode state
			polyop.setvertcolor pValidationNode -0 pValidationNode.verts (color 255 255 255)				
		)
		CompleteRedraw()
		CompleteRedraw()
	)
	
	
	on btnDegenerateFacesShow changed state do
	(
		if state == true then
		(
			btnVertexProximityShow.state = false
			polyop.setvertcolor pValidationNode -0 pValidationNode.verts BGColor_G_mxs
			polyop.setvertcolor pValidationNode -0 aDegenerateFaces BGColor_R_mxs	
			polyop.setvertcolor pValidationNode -1 aDegenerateFaces BGColor_R_mxs
			setCVertMode pValidationNode state
			pValidationNode.xray = state			
		)
		else
		(
			pValidationNode.xray = state
			setCVertMode pValidationNode state
			polyop.setvertcolor pValidationNode -0 pValidationNode.verts (color 255 255 255)
		)
		CompleteRedraw()
		CompleteRedraw()
	)
	
	
	on BoneExportList selected item do
	(
		try
		(
			select (getNodeByName BoneExportList.items[item])
		)catch()
	)
	
	
	on btnZeroWeightsFix pressed do
	(
		local aMarkerPoints = fnFixZeroWeights pValidationNode
		select aMarkerPoints ; max zoomext sel all
	)
	
	on btnDuplicatesFix pressed do
	(
		select aDuplicates
	)
	
	on btnReCheck pressed do
	(		
		select pValidationNode
		aZeroWeights = #()
		aDegenerateFaces = #()
		aVertexProximity = #()
		intProblems = 0
		destroyDialog rltCHRValidator
		createDialog rltCHRValidator 260 290 style:#(#style_titlebar)  pos:[220,400]	
	)
	
	on btnOK pressed do
	(
-- 		fnRestoreVertexColor pValidationNode aVertexColorBackup
		destroyDialog rltCHRValidator
	)
	
	on rltCHRValidator close do
	(
		try
		(
			setCVertMode pValidationNode false
			pValidationNode.xray = false			
		)catch()
	)
)


createDialog rltCHRValidator 260 290 style:#(#style_titlebar)  pos:[220,400]	

