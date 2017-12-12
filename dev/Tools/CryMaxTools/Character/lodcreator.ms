	--############# Create LODS ############# 
	--###  Marcus Krautwurst

	intLODStepsQuality = #(50,25,12.5)

	tempOldName = undefined
	pTarget  = $
	tempFolder = sysInfo.tempdir
	DialogMonitorOPS.RegisterNotification ANoon_EnvelopeCallbackFunction ID:#ANoon_Envelopes
	DialogMonitorOPS.Enabled = true
	
	fn MaxRedraw arg =
	(
		WM_SETREDRAW=0xB
		commandHWND = (windows.getChildHWND #max "Command Panel")
		windows.sendmessage (commandHWND[1]) WM_SETREDRAW arg 0
	)
	
	fn ANoon_EnvelopeCallbackFunction =
	(
		WindowHandle = DialogMonitorOPS.GetWindowHandle()
		theDialogName = UIAccessor.GetWindowText WindowHandle
		if theDialogName != undefined and matchpattern theDialogName pattern:"*Load Envelopes*" do
		UIAccessor.PressButtonByName WindowHandle "OK"
		true
	)
	
	fn createLODMeshes pTarget =
	(
		select pTarget
		skinops.saveEnvelope pTarget.modifiers[#CrySkin] (tempFolder + pTarget.name + ".env")
		tempOldName = pTarget.name
		select pTarget		

		for i = 1 to 3 do
		(
			tempLODSnapshot = snapshot pTarget
			tempLODSnapShot.name = (tempOldName + "_LOD" + (i as string))
		)		
	)



	fn TransferSkin pRef pTarget =
	(
		local treshold = 1	
		with redraw off
		(
			if pRef != undefined or pTarget != undefined then
			(
				local oldBoneNodes = #()			
				skinUtils.ExtractSkinData pTarget
				select pTarget

				if pTarget.modifiers[#CrySkin] != undefined then 
					local OldSkinMod = pTarget.modifiers[#CrySkin]
				
				if pTarget.modifiers[#Skin] != undefined then 
					local OldSkinMod = pTarget.modifiers[#Skin]
				
				if OldSkinMod != undefined then			
				(
					OldSkinModCount = skinOps.GetNumberBones OldSkinMod
					
					for i = 1 to OldSkinModCount do
						append oldBoneNodes (skinOps.GetBoneName OldSkinMod i 1)	

					local tempNewSkinMod = Cryskin()
					
					addModifier pRef tempNewSkinMod

					local NewSkinMod = pRef.modifiers[#CrySkin]
					NewSkinMod.bone_Limit = 4
					for i = 1 to oldBoneNodes.count do
					(
						skinOps.addbone NewSkinMod (getNodebyName oldBoneNodes[i]) 1
					)
					
					local SkinDataMesh = (getNodebyName ("SkinData_" + pTarget.name))
					
					select SkinDataMesh
					selectMore pRef
					skinUtils.ImportSkinDataNoDialog true false false false false treshold 0
					delete SkinDataMesh 
				)
			)
		)
	)
	
	fn LoadEnv pTarget pRef= 
	(
		select pTarget
		skinops.loadEnvelope pTarget.modifiers[#CrySkin] (tempFolder + pRef.name + ".env")
	)
	
	
	fn addMultiResMod pTarget amount =
	(	
		select pTarget
		modPanel.setCurrentObject pTarget.baseObject
		MultiResMod = addModifier pTarget (MultiRes ())
		modPanel.setCurrentObject pTarget.modifiers[#MultiRes]	
		pTarget.modifiers[#MultiRes].vertexPercent = amount
	)
	
	
	fn reorderModifiers pTarget =
	(
		select pTarget
		MultiResMod = pTarget.modifiers[#MultiRes]
		SkinMod = pTarget.modifiers[#CrySkin]
		
		addModifier pTarget MultiResMod
		addModifier pTarget SkinMod		
		
		deletemodifier pTarget pTarget.modifiers.count
		deletemodifier pTarget pTarget.modifiers.count
		
		LoadEnv pTarget (getNodeByName tempOldName)
		
		MultiResMod.reqGenerate = off
		maxOps.CollapseNodeTo pTarget 2 off
	)
	
	fn LODMaster pTarget =
	(			
		try
		(
			gc()
			max modify mode
			if pTarget.modifiers[#Skin] != undefined  then messagebox "Only CrySkin is supported!"
			else
			(
				MaxRedraw 0
				createLODMeshes pTarget
				max modify mode
				for i = 1 to 3 do
				(				
					gc light:true
					LODMesh = (getNodeByName (tempOldName + "_LOD" + (i as string)))
					TransferSkin LODMesh (getNodeByName tempOldName)
					addMultiResMod LODMesh (intLODStepsQuality[i])
					reorderModifiers LODMesh
				)			
				MaxRedraw 1
				cui.commandPanelOpen = true			
			)
			DialogMonitorOPS.Enabled = false
		)catch(MaxRedraw 1)
	)
		
	LODMaster pTarget


