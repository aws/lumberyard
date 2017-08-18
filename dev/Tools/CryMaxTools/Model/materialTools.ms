---------------------------
-- CryMaxTools v2.0
-- Material Tools v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryMaterialTools "Material Tools" width:190 height:496
(
	spinner spnDestination "Working Slot: " fieldWidth:32 height:16 range:[1,24,24] type:#integer align:#right offset:[5,-2]
		
	button btnBase "Show Multi Material Base" offset:[0,3] width:160 height:16
	button btnSub1 "1" offset:[-4,-5] width:16 height:16 align:#left across:10
	button btnSub2 "2" offset:[-2,-5] width:16 height:16 align:#right
	button btnSub3 "3" offset:[-1,-5] width:16 height:16 align:#right
	button btnSub4 "4" offset:[-0,-5] width:16 height:16 align:#right
	button btnSub5 "5" offset:[1,-5] width:16 height:16 align:#right
	button btnSub6 "6" offset:[2,-5] width:16 height:16 align:#right
	button btnSub7 "7" offset:[3,-5] width:16 height:16 align:#right
	button btnSub8 "8" offset:[4,-5] width:16 height:16 align:#right
	button btnSub9 "9" offset:[5,-5] width:16 height:16 align:#right
	button btnSub10 "10" offset:[6,-5] width:16 height:16 align:#right
	button btnSub11 "11" offset:[-4,-5] width:16 height:16 align:#left across:10
	button btnSub12 "12" offset:[-2,-5] width:16 height:16 align:#right
	button btnSub13 "13" offset:[-1,-5] width:16 height:16 align:#right
	button btnSub14 "14" offset:[0,-5] width:16 height:16 align:#right
	button btnSub15 "15" offset:[1,-5] width:16 height:16 align:#right
	button btnSub16 "16" offset:[2,-5] width:16 height:16 align:#right
	button btnSub17 "17" offset:[3,-5] width:16 height:16 align:#right
	button btnSub18 "18" offset:[4,-5] width:16 height:16 align:#right
	button btnSub19 "19" offset:[5,-5] width:16 height:16 align:#right
	button btnSub20 "20" offset:[6,-5] width:16 height:16 align:#right
	
	spinner spnMultiCount "Nr:" offset:[-5,3] height:16 width:56 range:[1,99,2] type:#integer across:2 align:#left
	button btnMultiMake "Make Multi Material" height:16 width:104 offset:[5,3] align:#right
	
	button btnCharMatMake "Make Character Material" height:16 width:160 offset:[0,3]
	
	button btnSetCrytekShader "Set Crytek Shader" height:16 width:160 offset:[0,3]
	
--global variables
	
--global functions
	fn switchMaterial level =
	(
		baseMat = $.material
		numMat = getNumSubMtls baseMat
		mySlot = spnDestination.value
		MatEditor.Open()
		if level == 0 then
		(
			medit.PutMtlToMtlEditor baseMat mySlot
			medit.SetActiveMtlSlot mySlot true
		)
		else if level <= numMat then
		(
			subMat = findItem baseMat.materialIDList level
			medit.PutMtlToMtlEditor baseMat.materialList[subMat] mySlot
			medit.SetActiveMtlSlot mySlot true
		)
	)

--UI event handlers
	on rltCryMaterialTools open do
	(
		local aMaterialToolsSettings = cryMaxTools.basic.settingsMan.load "materialToolsSettings"
		if aMaterialToolsSettings != false then
		(
			spnDestination.value = aMaterialToolsSettings[1].value[1] as integer
			spnMultiCount.value = aMaterialToolsSettings[2].value[1] as integer
		)
	)
	
	on rltCryMaterialTools close do
	(
		local aMaterialToolsSettings = #("materialToolsSettings")
		append aMaterialToolsSettings ("spnDestination*@" + spnDestination.value as string)
		append aMaterialToolsSettings ("spnMultiCount*@" + spnMultiCount.value as string)
		cryMaxTools.basic.settingsMan.save aMaterialToolsSettings
	)
	
	on btnBase pressed do switchMaterial(0)
	on btnSub1 pressed do switchMaterial(1)
	on btnSub2 pressed do switchMaterial(2)
	on btnSub3 pressed do switchMaterial(3)
	on btnSub4 pressed do switchMaterial(4)
	on btnSub5 pressed do switchMaterial(5)
	on btnSub6 pressed do switchMaterial(6)
	on btnSub7 pressed do switchMaterial(7)
	on btnSub8 pressed do switchMaterial(8)
	on btnSub9 pressed do switchMaterial(9)
	on btnSub10 pressed do switchMaterial(10)
	on btnSub11 pressed do switchMaterial(11)
	on btnSub12 pressed do switchMaterial(12)
	on btnSub13 pressed do switchMaterial(13)
	on btnSub14 pressed do switchMaterial(14)
	on btnSub15 pressed do switchMaterial(15)
	on btnSub16 pressed do switchMaterial(16)
	on btnSub17 pressed do switchMaterial(17)
	on btnSub18 pressed do switchMaterial(18)
	on btnSub19 pressed do switchMaterial(19)
	on btnSub20 pressed do switchMaterial(20)
	
	on btnMultiMake pressed do
	(
		myMat = multimaterial numsubs:spnMultiCount.value
		for i = 1 to myMat.materialList.count do
		(
			myMat.materialList[i].name = ("submaterial" + i as string)
			myMat.materialList[i].shaderType = 2
		)
		MatEditor.Open()
		meditMaterials[spnDestination.value] = myMat
		activeMeditSlot = spnDestination.value
	)
	
	on btnCharMatMake pressed do
	(
		myMat = multimaterial numsubs:10
		myMat.name = "characterMaterial"
		for i = 1 to myMat.materialList.count do
		(
			case i of
			(
				1: (myMat.materialList[i].name = "Skel_Torso"; myMat.materialList[i].Diffuse = color 201 203 0)
				2: (myMat.materialList[i].name = "Skel_Head"; myMat.materialList[i].Diffuse = color 250 254 149)
				3: (myMat.materialList[i].name = "Skel_Arm_Left"; myMat.materialList[i].Diffuse = color 150 91 159)
				4: (myMat.materialList[i].name = "Skel_Arm_Right"; myMat.materialList[i].Diffuse = color 121 180 207)
				5: (myMat.materialList[i].name = "Skel_Leg_Left"; myMat.materialList[i].Diffuse = color 253 0 0)
				6: (myMat.materialList[i].name = "Skel_Leg_Right"; myMat.materialList[i].Diffuse = color 37 147 249)
				7: (myMat.materialList[i].name = "Skel_Foot_Left"; myMat.materialList[i].Diffuse = color 211 132 69)
				8: (myMat.materialList[i].name = "Skel_Foot_Right"; myMat.materialList[i].Diffuse = color 150 182 194)
				9: (myMat.materialList[i].name = "Skel_Hand_Left"; myMat.materialList[i].Diffuse = color 51 37 58)
				10: (myMat.materialList[i].name = "Skel_Hand_Right"; myMat.materialList[i].Diffuse = color 39 57 66)
			)
			myMat.materialList[i].shaderType = 2
			myMat.materialList[i].physicalizeMaterial = on
		)
		if $ != undefined then
		(
			$.material = myMat
		)
		MatEditor.Open()
		meditMaterials[spnDestination.value] = myMat
		activeMeditSlot = spnDestination.value
	)
	
	on btnSetCrytekShader pressed do
	(
		macros.run "CryMaxTools Material" "ATSetToCrytekShader"
	)
)

--add rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryMaterialTools" rltCryMaterialTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaterialTools")