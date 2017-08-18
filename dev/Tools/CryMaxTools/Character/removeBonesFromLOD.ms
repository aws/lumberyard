/*#####################################################
Author: 		Harald Zlattinger
Date:			2010.09.20
Script:		remove bones from LOD
Description:	Removes bones from skin modifier depending on chrparam entries

Requirements:
	- needs a CRY-Skinned mesh and a valid chrparams file
	  
Version History:
	0.6		- fixed lowerCase bug where bones were added even though they were already in skinmodifier
	0.5		- don't stop when CrySkin is not present on object
	0.4		- fixed a bug where comparing names isn't lowercase
			- fixed a bug where objects with less than 4 letters would break the script
	0.3		- ignores case of bone names
	0.2		- fixed bip reparent twist bones
	0.1		- Created rough version
#####################################################*/


LBR_sVersionNumber = "0.6"

global aBonesInLOD = #()
global iCurrentLOD = 0
global currentLODProcessing = 1
global aBonesInModifier = #()

LBR_LOD_boneRemove_window
LBR_LOD_boneRemove_rollout
try(destroyDialog LBR_LOD_boneRemove_window)catch()
dotNet.loadAssembly "system.xml"
theXMLDoc = dotNetObject "system.xml.xmlDocument"

fn fnGetValidParent pNode =
(
	nParent = pNode.parent
	iFound = findItem aBonesInLOD[currentLODProcessing] (toLower(nParent.name))
	if iFound == 0 then
	(
		theParent = fnGetValidParent nParent
	)
	else
	(
		return nParent
	)
	return theParent
)

fn fnGetBoneIDbyName boneName =
(
	for x = 1 to aBonesInModifier.count do
	(
		if boneName == aBonesInModifier[x] then
			return x
	)
)

fn fnCheckBoneInfluence pSkin vid boneID =
(
	numInflBones = skinOps.GetVertexWeightCount pSkin vid
	for x = 1 to numInflBones do
	(
		boneInfId = skinOps.GetVertexWeightBoneID pSkin vid x
		if boneInfId == boneID then return true
	)
)

fn fnGetBonesInSkinModifier pSkin =
(
	local aBonesInModifierTMP = #()
	-- get all Bones in SkinModifier
	for num = 1 to (skinOps.GetNumberBones pSkin) do
	(
		sBone = toLower (skinOps.GetBoneName pSkin num 0)
		append aBonesInModifierTMP sBone
	)
	return aBonesInModifierTMP
)

fn fnGetVertexBoneWeightByID pSkin boneID vid =
(
	theWeight = 0.0
	for x = 1 to (skinOps.GetVertexWeightCount pSkin vid) do
	(
		realBoneID = skinOps.GetVertexWeightBoneID pSkin vid x
		if realBoneID == boneID then
		(
			return (skinOps.GetVertexWeight pSkin vid x)
		)
	)
	return theWeight
)

fn fnRemoveBonesOnLOD pTarget =
(
	local pSkin = pTarget.modifiers[#CrySkin]
	if pSkin != undefined then
	(
		local aDeprecatedBones = #()
		local aValidParents = #()
		local aBoneMatrix = #()
		
		-- get all Bones in SkinModifier
		aBonesInModifier = fnGetBonesInSkinModifier pSkin
		
		
		for num = 1 to (aBonesInModifier.count) do
		(
			sBone = aBonesInModifier[num]
			if (findItem aBonesInLOD[currentLODProcessing] sBone) == 0 then
			(
				append aDeprecatedBones sBone
			)
		)
		
		-- go through array and get the parent
		
		for x = 1 to aDeprecatedBones.count do
		(
			theNode = getNodeByName aDeprecatedBones[x]
			theParent = fnGetValidParent theNode
			appendIfUnique aValidParents theParent
			entry = #(aDeprecatedBones[x],toLower(theParent.name))
			aBoneMatrix[x] = entry
		)
		
		-- add missing parents to the skinModifier
		for x=1 to aValidParents.count do
		(
			updateInt = 0
			if x == aValidParents.count then updateInt = 1
			found = findItem aBonesInModifier (toLower(aValidParents[x].name))
			if found == 0 then
			(
				skinOps.addBone pSkin aValidParents[x] updateInt
			)
		)
		-- reread list of bones in modifier
		aBonesInModifier = fnGetBonesInSkinModifier pSkin
		-- recreate Matrix
		for x = 1 to aBoneMatrix.count do
		(
			for y = 1 to 2 do
			(
				aBoneMatrix[x][y] = (fnGetBoneIDbyName aBoneMatrix[x][y])
			)
		)
		
		-- trace through vertices
		numVertices = skinOps.getNumberVertices pSkin
		
		for x =  1 to numVertices do
		(
			skinOps.unNormalizeVertex pSkin x true
		)
		
		
		for y = 1 to aBoneMatrix.count do
		(
			--for x = 1 to numVertices do
			for x = 1 to numVertices do
			(
				bProcessBone = fnCheckBoneInfluence pSkin x aBoneMatrix[y][1]
				if bProcessBone == true then
				(
					targetWeight = 0.0+(fnGetVertexBoneWeightByID pSkin aBoneMatrix[y][1] x)
					sourceWeight = 0.0+(fnGetVertexBoneWeightByID pSkin aBoneMatrix[y][2] x)
					
					targetWeight += sourceWeight
					skinOps.SetVertexWeights pSkin x aBoneMatrix[y][1] 0.0
					skinOps.SetVertexWeights pSkin x aBoneMatrix[y][2] targetWeight
				)
			)
		)
		for x =  1 to numVertices do
		(
			skinOps.unNormalizeVertex pSkin x false
		)
		
		for x = aBoneMatrix.count to 1 by -1 do
		(
			skinOps.removeBone pSkin aBoneMatrix[x][1]
		)
	)
	else
	(
		print ("No CRYSKIN on "+pTarget.name)
	)
)

function LBR_readCHRPARAM n =
(
	local continueWithChildren = false
	case n.name of
	(
		"Params": continueWithChildren = true
		"Lod": 
		(
			aBonesInLOD = #()
			aBonesInLODTMP = #()
			continueWithChildren = true
			print (n.childNodes.count as string + " LOD entries found")
		)
		
		"JointList":
		(
			iCurrentLOD = (((n.GetAttributeNode "level").value) as number)
			aBonesInLOD[iCurrentLOD] = #()
			continueWithChildren = true
			aBonesInLODTMP = #()
		)
		"Joint":
		(
			boneName = toLower((n.GetAttributeNode "name").value)
			append aBonesInLOD[iCurrentLOD] boneName
			continueWithChildren = false
		)
		default: continueWithChildren = false
	)
	if n.childNodes != undefined and continueWithChildren == true then
	(
		for i = 0 to n.childNodes.count-1 do
			LBR_readCHRPARAM n.childNodes.itemOf[i]
	)
)


fn fnLBR_select_chrparam_file =
(
	LBR_chrparam_file_TMP = getOpenFileName   \
		caption:"Please select a file" \
		types:"CHRPARAMS|*.chrparams" \
		historyCategory:"CHRPARAM Files" \
		filename:(maxFilePath)

	
	if LBR_chrparam_file_TMP != "" and LBR_chrparam_file_TMP != undefined then
	(
		--aFSQFiles_ALL = #()
		LBR_LOD_boneRemove_rollout.txt_LBR_chrparam.text = (getFilenameFile LBR_chrparam_file_TMP)
		LBR_LOD_boneRemove_rollout.btn_LBR_removeBones.enabled = true
		LBR_chrparam_file = LBR_chrparam_file_TMP
		theXMLDoc.load LBR_chrparam_file_TMP
		theDocElement=theXMLDoc.documentElement
		LBR_readCHRPARAM theDocElement

	)
)

fn LBR_DOIT = 
(
	if (isValidNode $'Bip01') == true then
	cryMaxTools.export.fnReparentTwistBonesInHierarchies $'Bip01'

	gc()
	with undo on
	(
		max modify mode
		theSelection = #()
		for obj in $ do
		(
			append theSelection obj
		)
		print theSelection

		for obj in theSelection do
		(
			print obj.name
			if obj.name.count > 4 then
			(
				if (substring obj.name (obj.name.count-3) 3) == "LOD" then
				(
					currentLODProcessing = (substring obj.name (obj.name.count) 1) as number

					select obj
					fnRemoveBonesOnLOD obj
				)
			)
		)
		select theSelection
	)
)

/*#################################################################
WINDOW AND ROLLOUT
#################################################################*/
rollout LBR_LOD_boneRemove_rollout ("Remove bones from LOD "+LBR_sVersionNumber)
(
			edittext 	txt_LBR_chrparam						"CHRPARAMS"										width:188	align:#left		enabled:true	readOnly:true
			button		btn_LBR_chrparam						"Select"												width:40	align:#right		enabled:true	offset:[0,-24]
			button 		btn_LBR_removeBones 					"Remove bones in selected LODs" 			width:234 	align:#left 		enabled:false

	
	on btn_LBR_chrparam									pressed 	do 	fnLBR_select_chrparam_file()
	on btn_LBR_removeBones								pressed 	do 	LBR_DOIT()
)

createDialog LBR_LOD_boneRemove_rollout 260 60
LBR_LOD_boneRemove_window = LBR_LOD_boneRemove_rollout
