/*#####################################################
Author: 		Harald Zlattinger
Date:			2010.03.01
Script:		FacialExpressions_Tool
Description:	Helps working with the facial rig and automatically exports FXL

Requirements:
	- needs a CRY-Skinned mesh with facial bones all aligned to world (prefix = "FCL_")
	- facial bones need to be in a flat hierarchy having 1 root bone
	- all facial bones have to be linked to this root bone
*/
FCL_sVersionNumber = "1.5"
/*
Version History:
	1.5 - 	made script more open
			Define variables for:
				prefix of bones 		- default = "FCL_"
				prefix for visimes 	- default = "Visimes_"
				prefix for expr.		- default = "Exp_"
			added another identifier for bones which are used for the mouth area: _M_ <- also changeable in script
				this is important for split up expressions to be able to create curves for face and mouth expressions independently.
	1.4 -	added additional bones for jelena
	1.3	-	updates Path to export FXL File to, when changing the max file
	1.2	- 	added Phonemes k and TH
	1.1		- added eye bone controllers
	1.0		- added mirror script
	0.9		- changed timeTags
	0.8		- updated Phoneme Mapping - KW suggestions
			- reduced default values maxRot, maxTrans in UI
	0.7		- workaround for FacialEditor RotationOrder bug
	0.6		- split Expressions into _FACE and _MOUTH portions
	0.5		- implemented to CryMaxTools
	0.4		- No hardcoded paths
			- takes path of scene file
	0.3		- Added other tools
			- added window
	0.2		- Refined export settings
	0.1		- Created rough version
#####################################################*/

-- initial to be changed strings
FCL_sMeshName = "none"
FCL_sFXLFileName = ("faceExpr.fxl")
FCL_sFXLFilePath = (maxFilePath)
fMaxTrans 	= 0.05 	-- meters
fMaxRot 	= 120		-- degrees
FCL_bShowFile = false
FCL_bSHowFinishMessage = false
FCL_bEyeCTLs = true
fDifferenceThreshold = 0.01
FCL_sBaseName = "BASE"
FCL_sExpName = "Exp_"
FCL_sVisName = "Visime_"
FCL_sMouthToken_Identifier = "_M_"
FCL_sExpMouth_Suffix = "_MOUTH"
FCL_sExpFace_Suffix = "_FACE"
FCL_sBonePrefix = "FCL_"
-- default visime and expressions array for creating time tags.
-- u can still add more here - especially expressions
-- but if u add visimes, please take a look at the visime mapping
FCL_aVisimesAndExprNames = 
#(	
	FCL_sVisName+"AA",
	FCL_sVisName+"CH",
	FCL_sVisName+"EE",
	FCL_sVisName+"FF",
	FCL_sVisName+"II",
	FCL_sVisName+"LL",
	FCL_sVisName+"MP",
	FCL_sVisName+"NN",
	FCL_sVisName+"OO",
	FCL_sVisName+"RR",
	FCL_sVisName+"XX",
	"Blink" -- hardcoded for eye-blinking
)
-- phoneme mapping
FCL_aPhonemesArray = 
#(
	#("AA","AA"),
	#("AH","AA"),
	#("h","II"),
	#("b","MP"),
	#("p","MP"),
	#("EH","II"),
	#("AE","AA"),
	#("IH","EE"),
	#("AY","AA"),
	#("f","FF"),
	#("y","EE"),
	#("IY","EE"),
	#("EY","II"),
	#("AO","OO"),
	#("AW","OO"),
	#("OW","OO"),
	#("OY","OO"),
	#("UH","OO"),
	#("UW","OO"),
	#("r","RR"),
	#("ER","RR"),
	#("l","LL"),
	#("w","OO"),
	#("CH","CH"),
	#("j","CH"),
	#("SH","CH"),
	#("ZH","CH"),
	#("n","NN"),
	#("NG","NN"),
	#("DH","NN"),
	#("d","NN"),
	#("g","NN"),
	#("t","NN"),
	#("z","NN"),
	#("s","NN"),
	#("x","XX"),
	#("v","FF"),
	#("k","NN"),
	#("TH","NN"),
	#("m","MP")
)

-- processing phonemes array for functionality
for x = 1 to FCL_aPhonemesArray.count do
(
	FCL_aPhonemesArray[x][2] = FCL_sVisName + FCL_aPhonemesArray[x][2]
)
-- create Entry on top of the list for the Base-Bone
insertItem #("_",FCL_sBaseName) FCL_aPhonemesArray 1


/*#################################################################
#################################################################
#################################################################
#################################################################
START FXL EXPORT
#################################################################
#################################################################
#################################################################
#################################################################*/

FCL_tools_window
FCL_tools_rollout
try(destroyDialog FCL_tools_window)catch()

dotnet.loadAssembly "system.xml.dll"
xml = undefined

-- static variables - don't change
FCL_theReferenceMatrix = matrix3 [1,0,0] [0,1,0] [0,0,1] [0,0,0]
FCL_aBoneCtlsToGroup = #()
FCL_aExpressionsToGroup = #()
FCL_aVisimesToGroup = #()
FCL_aDeformingBones = #()
FCL_aMouthBonesArray = #()
FCL_aFaceBonesArray = #()
FCL_transformArray = #("1_tX","2_tY","3_tZ","6_rX","5_rY","4_rZ")
FCL_nodeObjToExport

struct FCL_strucTimeTag (theName, theTime, theID)
FCL_aTimeTags = #()

--################################################
-- get all tags in scenes and their name and time
--################################################
fn FCL_fnGet_TimeTags_List = 
(
	allTheTags = #()
	theCount = FrameTagManager.GetTagCount()
	for x = 1 to theCount do
	(
		try
		(
			theID = (FrameTagManager.GetTagID x)
			theName = (FrameTagManager.GetNameByID theID)
			theTime = (FrameTagManager.GetTimeByID theID)
			theTagStruct = FCL_strucTimeTag theName theTime theID
			append allTheTags theTagStruct
		)
		catch()
	)
	return allTheTags
)
--################################################
-- create predefined Tags for Visimes in current timeline starting with frame 0 for BASE
--################################################
fn FCL_fnCreate_PredefinedTags =
(
	FrameTagManager.ResetFrameTags()
	aPredefinedTags = deepCopy FCL_aVisimesAndExprNames
	
	insertItem FCL_sBaseName aPredefinedTags 1
	for x = 0 to aPredefinedTags.count-1 do
	(
		FrameTagManager.CreateNewTag aPredefinedTags[x+1] x
	)
)
--################################################
-- getDifferences of obj transforms between frame x and y
--################################################
fn FCL_fnGetDifferences obj frameNum=
(
	-- obj = Controller
	-- frameNum = frame number of the expression/visime
	aDifferences = #()
	aTemp = matrix3 [0,0,0] [0,0,0] [0,0,0] [0,0,0]
	at time frameNum
	(
		-- get current Transforms of the controller
		aTemp = (obj.transform * inverse obj.parent.transform)
	)
	-- get the difference between transforms of current frame and base frame 0
	aDifferences[1] = (aTemp.pos.X - FCL_theReferenceMatrix.pos.X)
	aDifferences[2] = (aTemp.pos.Y - FCL_theReferenceMatrix.pos.Y)
	aDifferences[3] = (aTemp.pos.Z - FCL_theReferenceMatrix.pos.Z)
	theOrder = 1
	tmpEuler = quatToEuler aTemp.rotation order:theOrder
	refEuler = quatToEuler FCL_theReferenceMatrix.rotation order:theOrder
	aDifferences[4] = (tmpEuler.X - refEuler.X)
	aDifferences[5] = (tmpEuler.Y - refEuler.Y)
	aDifferences[6] = (tmpEuler.Z - refEuler.Z)
	return aDifferences
)
--################################################
-- find bones in skin-modifier of selected object having the defined prefix in their name
--################################################
fn fnGetListOfBones =
(
	aTheBones = #()
	theObj = FCL_nodeObjToExport
	if FCL_nodeObjToExport == undefined then
	(
		theObj = (getNodeByName "faraa")
	)
	select theObj
	if theObj != undefined then
	(
		-- look for the skin modifier
		local theSkinModifier
		for obj in theObj.modifiers do
		(
			if classOf obj == Skin then
			(
				theSkinModifier = obj
				print obj
				print "Found Skin Modifier"
				print (classOf obj)
			)
		)
		max modify mode
		if theSkinModifier != undefined then
		(
			numBones = (getSkinOps sel:theSkinModifier).GetNumberBones theSkinModifier
			for x = 1 to numBones do
			(
				-- process bones with predefined prefix
				theBoneName = (getSkinOps sel:theSkinModifier).GetBoneName theSkinModifier x 0
				if (matchPattern theBoneName pattern:(FCL_sBonePrefix+"*")) then
				(
					append aTheBones theBoneName
					-- distribute bones into split arrays (MOUTH vs. FACE)
					if (matchPattern theBoneName pattern:("*"+FCL_sMouthToken_Identifier+"*")) then
					(
						append FCL_aMouthBonesArray theBoneName
					)
					else
					(
						append FCL_aFaceBonesArray theBoneName
					)
				)
			)
		)
	)
	sort aTheBones
	sort FCL_aMouthBonesArray
	sort FCL_aFaceBonesArray
	
	-- if eye-bone controllers should be created (defined with checkbox in UI) eye bones should be added although they are not in the skin modifier
	-- eye bone names are hardcoded in C2 ... this should be made more dynamic with textfields later
	if FCL_bEyeCTLs then
	(
		append aTheBones "eye_left_bone"
		append aTheBones "eye_right_bone"
	)
	return aTheBones
)
--################################################
-- create Groups for FXL
--################################################
fn fnCreateGroups ele strName=
(
	effector = xml.CreateElement "Effector"
	effector.setAttribute "Name" (strName as string)
	if strName == "Root" then
	(
		effector.setAttribute "Flags" "1"
	)
	effector.setAttribute "Type" "group"
	ele.AppendChild effector
)
--################################################
-- create Expression
--################################################
fn FCL_fnCreateExpressions ele theName =
(
	theExpression = xml.CreateElement "Effector"
	theExpression.setAttribute "Name" theName
	theExpression.setAttribute "Type" "expression"
	ele.AppendChild theExpression
	if (matchPattern theName pattern:(FCL_sExpName+"*")) then
	(
		append FCL_aExpressionsToGroup theName
		theExpression = xml.CreateElement "Effector"
		theExpression.setAttribute "Name" (theName+FCL_sExpFace_Suffix)
		theExpression.setAttribute "Type" "expression"
		ele.AppendChild theExpression
		
		theExpression = xml.CreateElement "Effector"
		theExpression.setAttribute "Name" (theName+FCL_sExpMouth_Suffix)
		theExpression.setAttribute "Type" "expression"
		ele.AppendChild theExpression
	)
	else if (matchPattern theName pattern:(FCL_sVisName+"*")) then
	(
		append FCL_aVisimesToGroup theName
	)
	else
	(
		append FCL_aExpressionsToGroup theName
	)
)
--################################################
-- create Phoneme
--################################################
fn FCL_fnCreatePhoneme ele theName =
(
	thePhoneme = xml.CreateElement "Effector"
	thePhoneme.setAttribute "Name" theName
	thePhoneme.setAttribute "Type" "expression"
	ele.AppendChild thePhoneme
)
--################################################
-- create Bone Controller
--################################################
fn FCL_fnCreateBoneController ele theBone trans=
(
	rotString = "0,0,0"
	posString = "0,0,0"
	case trans of
	(
		1: posString = (fMaxTrans as string + ",0,0")
		2: posString = ("0," + fMaxTrans as string + ",0")
		3: posString = ("0,0," + fMaxTrans as string)
		4: rotString = ((degToRad fMaxRot) as string) + ",0,0"
		5: rotString = "0," + ((degToRad fMaxRot) as string) + ",0"
		6: rotString = "0,0," + ((degToRad fMaxRot) as string)
	)
	boneCtrl = xml.CreateElement "Effector"
	-- add transform name as a suffix due to alphabetical sorting and resulting in wrong rotation order XYZ / ZYX
	boneCtrl.setAttribute "Name" (theBone+"_"+FCL_transformArray[trans])
	boneCtrl.setAttribute "Type" "bone"
	boneCtrl.setAttribute "Attachment" theBone
	boneCtrl.setAttribute "PosOffset" posString
	boneCtrl.setAttribute "RotOffset" rotString
	append FCL_aBoneCtlsToGroup (theBone+"_"+FCL_transformArray[trans])
	ele.AppendChild boneCtrl
)
--################################################
-- create Parent for the Expression
--################################################
fn FCL_fnCreateExpressionParent ele theName = 
(
	-- this function puts the expression in their categories to clean up the FXL
	expression = xml.CreateElement "Effector"
	expression.setAttribute "Name" theName
	ele.AppendChild expression
	return expression
)
--################################################
-- create SubControls
--################################################
fn FCL_fnCreateSubCtrl ele theName theDifference =
(
	for x = 1 to 6 do
	(
		-- only add bone control if transform is bigger than the threshold to keep the fxl short
		if theDifference[x] > fDifferenceThreshold or theDifference[x] < (fDifferenceThreshold*-1) then
		(
			theNewFormattedValue = "";
			subCtrl = xml.CreateElement "SubCtrl"
			if x > 3 then
			(
				theNewFormattedValue = ((theDifference[x]/fMaxRot) as string)
				subCtrl.setAttribute "Weight" theNewFormattedValue
			)
			else
			(
				theNewFormattedValue = (formattedPrint (theDifference[x]/(fMaxTrans*100)) format:"1.8g")
				subCtrl.setAttribute "Weight" theNewFormattedValue
			)
			subCtrl.setAttribute "Keys" ("0:1,1:"+theNewFormattedValue+",")
			subCtrl.setAttribute "Effector" (theName+"_"+FCL_transformArray[x])
			ele.AppendChild subCtrl
		)
	)
)
--################################################
-- collect data and create XML/FXL
--################################################
fn FCL_fnCreateXMLFile = 
(
	xml=dotNetObject "system.xml.xmlDocument"
	FCL_aBoneCtlsToGroup = #()
	FCL_aExpressionsToGroup = #()
	FCL_aVisimesToGroup = #()
	FCL_aDeformingBones = #()
	FCL_aDeformingBones = fnGetListOfBones()
	
	-- checks if there are any facial bones in the object's skin modifier
	if FCL_aDeformingBones.count > 0 then
	(
		root = xml.CreateElement "ExpressionLibrary"
		xml.AppendChild root
		
		allEffectors = xml.CreateElement "AllEffectors"
		root.AppendChild allEffectors 
		
		allControllers = xml.CreateElement "Controllers"
		root.AppendChild allControllers 
		
		--clearListener()
		
		------------------------------------------------------------------------------
		-- create Groups
		------------------------------------------------------------------------------
		fnCreateGroups allEffectors "Root"
		fnCreateGroups allEffectors "_garbage"
		fnCreateGroups allEffectors "BoneControls"
		fnCreateGroups allEffectors "Expressions"
		fnCreateGroups allEffectors "Expressions_Cut"
		fnCreateGroups allEffectors "Visimes"
		fnCreateGroups allEffectors "Phonemes"
		------------------------------------------------------------------------------
		-- create Bone Controllers
		------------------------------------------------------------------------------
		for jnt in FCL_aDeformingBones do
		(
			-- translation
			FCL_fnCreateBoneController allEffectors jnt 1
			FCL_fnCreateBoneController allEffectors jnt 2
			FCL_fnCreateBoneController allEffectors jnt 3
			-- rotation 
			-- due to alphabetical order and wrong rotation order when sorting with rX,rY,rZ (ZYX)
			-- the order is changed manually to rZ,rY,rX to get a rotation order of XYZ
			FCL_fnCreateBoneController allEffectors jnt 6
			FCL_fnCreateBoneController allEffectors jnt 5
			FCL_fnCreateBoneController allEffectors jnt 4
		)
		------------------------------------------------------------------------------
		-- create Phoneme List
		------------------------------------------------------------------------------
		for phon in FCL_aPhonemesArray do
		(
			FCL_fnCreatePhoneme allEffectors phon[1]
		)
		------------------------------------------------------------------------------
		-- get Expressions and Create Them
		------------------------------------------------------------------------------
		FCL_aTimeTags = FCL_fnGet_TimeTags_List()
		for obj in FCL_aTimeTags do
		(
			FCL_fnCreateExpressions allEffectors obj.theName
		)
		------------------------------------------------------------------------------
		-- group to root
		------------------------------------------------------------------------------
		aTmpArray = #("_garbage","BoneControls","Expressions","Visimes","Phonemes")
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "Root"
		allControllers.AppendChild theController
		for obj in aTmpArray do
		(
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" obj
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- group boneCtrls
		------------------------------------------------------------------------------
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "BoneControls"
		allControllers.AppendChild theController
		for obj in FCL_aBoneCtlsToGroup do
		(
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" obj
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- group Phonemes
		------------------------------------------------------------------------------
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "Phonemes"
		allControllers.AppendChild theController
		for phon in FCL_aPhonemesArray do
		(
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" phon[1]
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- group Expressions
		------------------------------------------------------------------------------
		-- Expressions
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "Expressions"
		allControllers.AppendChild theController
		subCtrl = xml.CreateElement "SubCtrl"
		subCtrl.setAttribute "Weight" "1"
		subCtrl.setAttribute "Effector" "Expressions_Cut"
		theController.AppendChild subCtrl
		for obj in FCL_aExpressionsToGroup do
		(
			if (matchPattern obj pattern:(FCL_sExpName+"*")) then
			(
				theArray = #( obj)
				for theObj in theArray do
				(
					subCtrl = xml.CreateElement "SubCtrl"
					subCtrl.setAttribute "Weight" "1"
					subCtrl.setAttribute "Effector" theObj
					theController.AppendChild subCtrl
				)
			)
			else
			(
				subCtrl = xml.CreateElement "SubCtrl"
				subCtrl.setAttribute "Weight" "1"
				subCtrl.setAttribute "Effector" obj
				theController.AppendChild subCtrl
			)
		)
		-- Expressions_Cut
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "Expressions_Cut"
		allControllers.AppendChild theController
		for obj in FCL_aExpressionsToGroup do
		(
			if (matchPattern obj pattern:(FCL_sExpName+"*")) then
			(
				theArray = #(obj+FCL_sExpFace_Suffix, obj+FCL_sExpMouth_Suffix)
				for theObj in theArray do
				(
					subCtrl = xml.CreateElement "SubCtrl"
					subCtrl.setAttribute "Weight" "1"
					subCtrl.setAttribute "Effector" theObj
					theController.AppendChild subCtrl
				)
			)
		)
		------------------------------------------------------------------------------
		-- put subexpressions into expressions
		------------------------------------------------------------------------------
		for obj in FCL_aExpressionsToGroup do
		(
			theController = xml.CreateElement "Effector"
			theController.setAttribute "Name" obj
			allControllers.AppendChild theController
			
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" (obj+FCL_sExpFace_Suffix)
			theController.AppendChild subCtrl
			
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" (obj+FCL_sExpMouth_Suffix)
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- group Visimes
		------------------------------------------------------------------------------
		theController = xml.CreateElement "Effector"
		theController.setAttribute "Name" "Visimes"
		allControllers.AppendChild theController
		for obj in FCL_aVisimesToGroup do
		(
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" obj
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- put visimes into phonemes
		------------------------------------------------------------------------------
		for obj in FCL_aPhonemesArray do
		(
			theController = xml.CreateElement "Effector"
			theController.setAttribute "Name" obj[1]
			allControllers.AppendChild theController
			
			subCtrl = xml.CreateElement "SubCtrl"
			subCtrl.setAttribute "Weight" "1"
			subCtrl.setAttribute "Effector" obj[2]
			theController.AppendChild subCtrl
		)
		------------------------------------------------------------------------------
		-- collect expression data and create subctrls
		------------------------------------------------------------------------------
		for obj in FCL_aTimeTags do
		(
			if (matchPattern obj.theName pattern:(FCL_sExpName+"*")) then
			(
				-- FACE bones
				theExpression = (FCL_fnCreateExpressionParent allControllers (obj.theName+FCL_sExpFace_Suffix))
				for jnt in FCL_aFaceBonesArray do
				(
					theBone = (getNodeByName jnt)
					if theBone != undefined then
					(
						-- get transforms on frame 0 which should be the default pose and also binding pose
						at time 0f
						(
							try
							(
								FCL_theReferenceMatrix = (theBone.transform * inverse theBone.parent.transform)
							)
							catch
							(
								FCL_theReferenceMatrix = (theBone.transform)
							)
						)
						-- getting transform differences from reference frame 0 and current frame
						theDiff = (FCL_fnGetDifferences theBone obj.theTime)
						-- create SubControl for this bone control (if difference is bigger than the threshold)
						FCL_fnCreateSubCtrl theExpression jnt theDiff
					)
					else
					(
						print ("undefined: "+jnt)
					)
				)
				-- MOUTH bones
				theExpression = (FCL_fnCreateExpressionParent allControllers (obj.theName+FCL_sExpMouth_Suffix))
				for jnt in FCL_aMouthBonesArray do
				(
					theBone = (getNodeByName jnt)
					if theBone != undefined then
					(
						-- get transforms on frame 0 which should be the default pose and also binding pose
						at time 0f
						(
							try
							(
								FCL_theReferenceMatrix = (theBone.transform * inverse theBone.parent.transform)
							)
							catch
							(
								FCL_theReferenceMatrix = (theBone.transform)
							)
						)
						-- getting transform differences from reference frame 0 and current frame
						theDiff = (FCL_fnGetDifferences theBone obj.theTime)
						-- create SubControl for this bone control (if difference is bigger than the threshold)
						FCL_fnCreateSubCtrl theExpression jnt theDiff
					)
					else
					(
						print ("undefined: "+jnt)
					)
				)
			)
			else
			(
				theExpression = (FCL_fnCreateExpressionParent allControllers obj.theName)
				for jnt in FCL_aDeformingBones do
				(
					theBone = (getNodeByName jnt)
					if theBone != undefined then
					(
						-- get transforms on frame 0 which should be the default pose and also binding pose
						at time 0f
						(
							try
							(
								FCL_theReferenceMatrix = (theBone.transform * inverse theBone.parent.transform)
							)
							catch
							(
								FCL_theReferenceMatrix = (theBone.transform)
							)
						)
						-- getting transform differences from reference frame 0 and current frame
						theDiff = (FCL_fnGetDifferences theBone obj.theTime)
						-- create SubControl for this bone control (if difference is bigger than the threshold)
						FCL_fnCreateSubCtrl theExpression jnt theDiff
					)
					else
					(
						print ("undefined: "+jnt)
					)
				)
			)
		)
		-- getNodeByName
		try
		(
			xml.save (FCL_sFXLFilePath+FCL_sFXLFileName)
			-- is user activated SHOW FXL AFTER EXPORT
			if FCL_bShowFile == true then
			(
				edit  (FCL_sFXLFilePath+FCL_sFXLFileName)
			)
			-- will show a popup if user wants to be alerted with a popup after finished export
			if FCL_bSHowFinishMessage == true then
			(
				messageBox "DONE" title:"FXL File successfully exported!"
			)
			else
			(
				print ("FXL File successfully exported to: \n\n" + FCL_sFXLFilePath + FCL_sFXLFileName + "\n\nDONE!!!")
			)
			
		)
		catch
		(
			messageBox "The file couldn't be saved!" title:"File Error"
		)
	)
)
/*#################################################################
#################################################################
#################################################################
#################################################################
DONE FXL EXPORT
#################################################################
#################################################################
#################################################################
#################################################################*/



/*#################################################################
RESET CONTROLLERS
#################################################################*/
fn FCL_fnResetAndAddControllers = 
(
	-- sets controllers to zero at current position (doesn't change transforms, only zeroes out the CTLs)
	for Targ in selection do
	(
		Targ.position.controller = position_XYZ()
		Pos = Targ.position.controller = position_list()
		Targ.position.controller[2].controller = Position_XYZ()
		Targ.position.controller.active = 2
		Pos.setName 2 "ANIMATION"
		Targ.rotation.controller = Euler_XYZ()
		Rot = Targ.rotation.controller = rotation_list()
		Targ.rotation.controller[2].controller = Euler_XYZ()
		Targ.rotation.controller.active = 2
		Rot.setName 2 "ANIMATION"
	)
)
/*#################################################################
MIRROR HELPERS/BONES
#################################################################*/
------------------------------------------------------------------------------------------------------------------------------------
-- get Name of the other side
-- checks the name for side tokens _C_ / _L_ / _R_
-- and returns the name of the object with changed side token
------------------------------------------------------------------------------------------------------------------------------------
fn FCL_fnGetNameOfOtherSide theName =
(
	newName = ""
	theSides = #("_C_","_R_","_L_")
	theOccurences = #()
	theOccurences[1] = findString theName theSides[1]
	theOccurences[2] = findString theName theSides[2]
	theOccurences[3] = findString theName theSides[3]
	
	if theOccurences[1] != undefined then
	(
		newName = theName
	)
	else if theOccurences[2] != undefined then
	(
		newName = replace theName theOccurences[2] 3 theSides[3]
	)
	else if theOccurences[3] != undefined then
	(
		newName = replace theName theOccurences[3] 3 theSides[2]
	)
	else
	(
		print (theName + " doesn't fit to the naming conventions. Bones need a token defining the side: _L_ / _R_ / _C_")
	)

	return newName
)
------------------------------------------------------------------------------------------------------------------------------------
-- mirror selected bones / point helpers
-- this should help creating and placing point helpers to automatically mirror positions to the other side
------------------------------------------------------------------------------------------------------------------------------------
fn FCL_fnMirrorSelectedPoints =
(
	for obj in selection do
	(
		newName = (FCL_fnGetNameOfOtherSide obj.name)
		if newName != "" then
		(
			if (getNodeByName newName) == undefined then
			(
				theSize = 2
				theColor = obj.wirecolor
				try ( theSize = obj.size) catch()
				newObj = point pos:obj.transform.pos name:newName size:theSize wirecolor:theColor
				newX = (obj.transform.pos.X * (-1))
				newObj.pos.X = newX
				theParent = (FCL_fnGetNameOfOtherSide obj.parent.name)
				if (getNodeByName theParent) != undefined then
				(
					newObj.parent = (getNodeByName theParent)
				)
				xml = undefined
			)
			else
			(
				print ("Object already exists: " + newName)
			)
		)
	)
)
--################################################
-- select fileName
--################################################
fn FCL_fnselectFileName = 
(
	global FCL_sFXLFilePath = ""
	global FCL_sFXLFileName = ""
	FCL_sFXLFileNameTEMP = getSaveFileName \
		caption:"Please select a filename" \
		filename:(maxFilePath)  \
		types:"Facial Expression Library(*.fxl)|*.fxl" \
		historyCategory:"Facial Expression Libraries"
	
	if FCL_sFXLFileNameTEMP != "" and FCL_sFXLFileNameTEMP != undefined then
	(
		FCL_sFXLFilePath = getFilenamePath FCL_sFXLFileNameTEMP
		FCL_sFXLFileName = filenameFromPath FCL_sFXLFileNameTEMP
		FCL_tools_rollout.txt_FCL_fileName.text = FCL_sFXLFileName
	)
)
--################################################
-- select the object with skin modifier to export FXL from
--################################################
fn FCL_fnPickObject = 
(
	if selection.count > 0 then
	(
		global FCL_nodeObjToExport = selection[1]
		FCL_tools_rollout.txt_FCL_objectName.text = FCL_nodeObjToExport.name
		FCL_sFXLFileName = FCL_nodeObjToExport.name+".fxl"
		FCL_sFXLFilePath = maxFilePath
		FCL_tools_rollout.txt_FCL_fileName.text = FCL_sFXLFileName
	)
	else
	(
		messageBox "Please select an Object" title:"Usage Error"
	)
)
/*#################################################################
STARTUP
#################################################################*/
fn FCL_fncheckSelectedObject =
(
	-- if an object is selected, textfields for mesh and export filename will be filled automatically
	try
	(
		if selection[1] != undefined then
		(
			FCL_nodeObjToExport = selection[1]
		)
		else
		(
			FCL_nodeObjToExport = undefined
		)
	)
	catch()
)
/*#################################################################
WINDOW AND ROLLOUT
#################################################################*/
rollout FCL_tools_rollout ("FacialExpression Tools "+FCL_sVersionNumber)
(
	group "Preparation"
	(
		button 		btn_FCL_createTimeTags 			"Create VisimeTimeTags" 					width:194 	align:#left 		enabled:true
		button 		btn_FCL_controllers		 			"Reset Controllers"				 				width:194 	align:#left 		enabled:true
		button 		btn_FCL_mirrorBones		 		"Mirror Bones"				 					width:194 	align:#left 		enabled:true
	)
	
	group "FXL EXPORT"
	(
		edittext 	txt_FCL_objectName					"Object:"										width:150	align:#left		enabled:false
		button		btn_FCL_selectObject				"Select"											width:40	align:#right		enabled:true	offset:[0,-24]
		edittext 	txt_FCL_fileName						"File:"											width:150	align:#left		enabled:true	text:FCL_sFXLFileName 	readOnly:true
		button		btn_FCL_selectFileName			"Select"											width:40	align:#right		enabled:true	offset:[0,-24]
		edittext 	txt_FCL_maxTranslation			"Max. Translation [cm]:"									align:#right		enabled:true	fieldwidth:40	text:((fMaxTrans*100) as string)
		edittext 	txt_FCL_maxRotation				"Max. Rotation [deg]:"										align:#right		enabled:true	fieldwidth:40	text:(fMaxRot as string)
		edittext 	txt_FCL_diffThresh					"Threshold:"													align:#right		enabled:true	fieldwidth:40	text:(fDifferenceThreshold as string)
		checkbox	cbx_FCL_addEyeCTLs				"Add Controllers for EYE Bones"			width:194	align:#left		enabled:true	checked:FCL_bEyeCTLs
		checkbox	cbx_FCL_showSuccessMsg		"Show message on success"				width:194	align:#left		enabled:true	checked:FCL_bShowFinishMessage
		checkbox	cbx_FCL_showResultFile			"Show FXL after export"					width:194	align:#left		enabled:true	checked:FCL_bShowFile
		button 		btn_FCL_exportFXL 					"Export FXL-File" 								width:194 	align:#left 		enabled:true
	)
	on btn_FCL_selectObject			pressed 	do 	FCL_fnPickObject()
	on btn_FCL_selectFileName		pressed 	do 	FCL_fnselectFileName()
	on cbx_FCL_showResultFile		changed			val 	do	(FCL_bShowFile = val)
	on cbx_FCL_addEyeCTLs			changed			val 	do	(FCL_bEyeCTLs = val)
	on cbx_FCL_showSuccessMsg	changed			val 	do	(FCL_bSHowFinishMessage = val)
	on txt_FCL_maxTranslation		changed			val 	do
																			(
																				try
																				(
																					fMaxTrans = 	(val as float)/100
																				)
																				catch
																				(
																					messageBox "Please only enter numbers" title:"USER ERROR"
																				)
																			)
	on txt_FCL_maxRotation			changed			val 	do
																			(
																				try
																				(
																					fMaxRot = 	val as float
																				)
																				catch
																				(
																					messageBox "Please only enter numbers" title:"USER ERROR"
																				)
																			)
	on txt_FCL_diffThresh				changed			val 	do
																			(
																				try
																				(
																					fDifferenceThreshold = 	val as float
																				)
																				catch
																				(
																					messageBox "Please only enter numbers" title:"USER ERROR"
																				)
																			)
	on btn_FCL_exportFXL				pressed 	do 	FCL_fnCreateXMLFile()
	on btn_FCL_createTimeTags 		pressed 	do 	FCL_fnCreate_PredefinedTags()
	on btn_FCL_controllers 			pressed 	do 	FCL_fnResetAndAddControllers()
	on btn_FCL_mirrorBones 			pressed 	do 	FCL_fnMirrorSelectedPoints()
)

createDialog FCL_tools_rollout 220 333
FCL_tools_window = FCL_tools_rollout
FCL_fncheckSelectedObject()

-- tries to fill UI with information, if a node was selected and the max scene has a path (saved max scene)
try
(
	FCL_tools_rollout.txt_FCL_objectName.text = FCL_nodeObjToExport.name
	FCL_sFXLFileName = FCL_nodeObjToExport.name+".fxl"
	FCL_sFXLFilePath = maxFilePath
	FCL_tools_rollout.txt_FCL_fileName.text = FCL_sFXLFileName
)
catch
(
	FCL_tools_rollout.txt_FCL_objectName.text = "Please select obj"
)
