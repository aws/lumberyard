rollout rltCryPickableSetup_rollout "Pickable Object Setup" width:190
(
	local origViewTM = getViewTM()
	local c2PickableNanoObjects = #()
	local c2PickableOrigObjects = #()
	
	local origPropPos = undefined
	local origPropRot = undefined
	
	local response
	
	group "Pickable Dummies"
	(
		button btn_c2Open2HPose "2H Pose..." offset:[-5,1] width:80 height:16 align:#left across:2 tooltip:""
		button btn_c2Make2HDummies "2H Dummy" offset:[0,1] width:80 height:16 align:#left tooltip:""

		button btn_c2Open1HPose "1H Pose..." offset:[-5,1] width:80 height:16 align:#left across:2 tooltip:""
		button btn_c2Make1HDummies "1H Dummy" offset:[0,1] width:80 height:16 align:#left tooltip:""

		button btn_c2Open1HPolePose "Pole Pose..." offset:[-5,1] width:80 height:16 align:#left across:2 tooltip:""
		button btn_c2Make1HPoleDummies "Pole Dummy" offset:[0,1] width:80 height:16 align:#left tooltip:""
	)
	group "Camera Controls"
	(
		button btn_c2Set1pCam "1P Camera" width:160 height:16 align:#center tooltip:""
		button btn_c2SetPerspCam "Persp Cam" width:160 height:16 align:#center tooltip:""
	)
	group "Pickable Pose"
	(
		button btn_c2PreviewPos "Snap to Grab Pose" width:160 height:16 align:#center tooltip:""
		button btn_c2ResetPos "Reset Object for Export" width:160 height:16 align:#center tooltip:""
	)
	
	button btn_c2CleanupPickableScene "Remove Nanosuit" width:160 height:16 align:#center tooltip:""

	/********************************************************************************/
	
	fn fnSilentCommandWithReturn sCommand =
	(
		cryMaxTools.basic.scmd (sCommand + " > \"" + cryMaxTools.basic.perforceMan.tempFilePath + "\"") true
		
		local tempString = ""
		while tempString == "" do
		(
			escapeEnable = true
			tempString = cryMaxTools.basic.perforceMan.readTempFile()
		)
		
		tempString
	)
	
	fn c2PickP4Cmd cmd filename = 
	(
		local p4cmd = ("p4 " + cmd + " " + filename)
		local strStream = fnSilentCommandWithReturn p4cmd
		
		if strStream == undefined then
		(--assume wrong client setup - try to fix client setup
			local sClientName = undefined
			
			local sUserName = (filterString (fnSilentCommandWithReturn "echo %username%") "\n")[1]
			local aClientsText = filterString (fnSilentCommandWithReturn ("p4 clients -u " + sUserName)) "\n"
			global aClientsTemp = for each in aClientsText collect (filterString each " ")[2] --read in client names from cmd output - must be global to access from inside rollout
			
			if aClientsTemp.count == 1 then
			(--only one client available - use it
				sClientName = aClientsTemp[1]
			)
			else
			(--more than one client available - make user chose
				global sP4ClientResponse = undefined
				
				rollout rltTest_rollout "Pickable Object Setup" width:400 height:80
				(
					label msg "Perforce client name does not exist. Please chose your perforce workspace below:" align:#center offset:[0,10]
					dropDownList ddlClients "" items:#("you", "should", "never", "see", "this") align:#right width:250 pos:[20,40]
					button btnOK "OK" align:#left width:50 pos:[270,40]
					button btnCancel "Cancel" align:#left width:60 pos:[320,40]
					
					on rltTest_rollout open do
					(
						ddlClients.items = aClientsTemp
					)
					
					on btnOK pressed do
					(
						sP4ClientResponse = ddlClients.selected
						try(destroyDialog rltTest_rollout) catch()
					)
					
					on btnCancel pressed do
					(
						sP4ClientResponse = undefined
						try(destroyDialog rltTest_rollout) catch()
					)
				)
				CreateDialog rltTest_rollout modal:true
				destroyDialog rltTest_rollout
				
				sClientName = sP4ClientResponse
				
				sP4ClientResponse = undefined
			)
			
			if sClientName == undefined then
			(--failed to find p4 client - tell user where files are in p4
				rollout rltTest_rollout "Pickable Object Setup" width:512 height:112
				(
					label msg "ERROR! Couldn't find working perforce client spec. Please get Nanosuit manually:" pos:[8,8]
					edittext edtP4Paths height:48 width:504 pos:[2,24] readOnly:true text:"//data/Games/Crysis2/GameCrysis2/_Animations_source/human/male/weapons/pickable/1p/1_handed/stand_tac_idlePose_small_1p_01.max\n//data/Games/Crysis2/GameCrysis2/_Animations_source/human/male/weapons/pickable/1p/2_handed/stand_tac_idlePose_medium_1p_01.max\n//data/Games/Crysis2/GameCrysis2/_Animations_source/human/male/weapons/pickable/1p/pole/stand_tac_idlePose_pole_add_1p_01.max"
					button btnOK "OK" width:80 height:24 pos:[224,80]
					
					on btnOK pressed do
					(
						try(destroyDialog rltTest_rollout) catch()
					)
				)
				CreateDialog rltTest_rollout modal:true
				destroyDialog rltTest_rollout
			)
			else
			(--found a good p4 client - set it
				cryMaxTools.basic.scmd ("p4 set P4CLIENT=" + sClientName) true
				
				strStream = fnSilentCommandWithReturn p4cmd
			)
			
			aClientsTemp = undefined
		)
		
		--print ("cmd = " + p4cmd)
		
		--old code below - used to crash when p4 client has bad value
		/*
		try
			(HiddenDOSCommand ( p4cmd ))	-- Max 2010
		catch
			(DOSCommand ( p4cmd ))				-- Max 2008

		while (fid = openFile tmpFile mode:"a") == undefined do
		(
			--print "still 0"
		)
				
		fid = openFile tmpFile mode:"r"
		if fid == undefined then
		(
			print ("CANNOT FIND hiddencmdout.tmp")
			return undefined
		)
		
		local strStream = ""
		
		while not (eof fid) do
			strStream = strStream + (readLine fid)
		close fid
		
		if strStream == "" then
			strStream = undefined
		
		deleteFile tmpFile
		*/
		
		print strStream
		
		return (filterString strStream "\n")[1]
	)
	
	fn fnC2GetPickPose type =
	(
		p4PoseFile = "//data/Games/Crysis2/GameCrysis2/_Animations_source/human/male/weapons/pickable/1p/"
		case type of
		(
			"1H" 	: p4PoseFile = (p4PoseFile + "1_handed/stand_tac_idlePose_small_1p_01.max")
			"2H" 	: p4PoseFile = (p4PoseFile + "2_handed/stand_tac_idlePose_medium_1p_01.max")
			"pole"  : p4PoseFile = (p4PoseFile + "pole/stand_tac_idlePose_pole_add_1p_01.max")
		)

		print ("Type = " + type)
		print ("p4PoseFile = " + p4PoseFile)
		
		--response = yesNoCancelBox ("Do you wish to Open the nanosuit figure (OPEN = YES), or Merge it with the current scene (MERGE = NO) ?")
		response = "cancel"
		
		rollout rltTest_rollout "Pickable Object Setup" width:400 height:80
		(
			label msg "Do you wish to Open the nanosuit figure, or Merge it with the current scene?" align:#center offset:[0,10]
			button but1_load "LOAD" align:#left width:95 pos:[50,40]
			button but2_merge "MERGE" align:#left width:95 pos:[150,40]
			button but3_cancel "CANCEL" align:#left width:95 pos:[250,40]
			
			on but1_load pressed do
			(
				response= "load"
				try(destroyDialog rltTest_rollout) catch()
			)
			on but2_merge pressed do
			(
				response= "merge"
				try(destroyDialog rltTest_rollout) catch()
			)
			on but3_cancel pressed do
			(
				response= "cancel"
				try(destroyDialog rltTest_rollout) catch()
			)
		)
		CreateDialog rltTest_rollout modal:true
		destroyDialog rltTest_rollout
				
		if response == "cancel" then
			return()

		localPoseFile = ""
		
		strStream = c2PickP4Cmd "sync -f" p4PoseFile
		if strStream == undefined then
			strStream = ""
			
		if (idx = findString strStream "refreshing ") != undefined then
		(
			print ("strStream = " + strStream)
			
			localPoseFile = substring strStream (idx + 11) strStream.count
		) else (
			strStream = c2PickP4Cmd "have" p4PoseFile
			
			if strStream == undefined then
				return()
			
			print ("strStream = " + strStream)
			aStrStream = filterString strStream "-"
			if aStrStream.count == 2 then
			(
				localPoseFile = trimLeft aStrStream[2] " "
			)
		)
		print ("Loading: " + localPoseFile)
		
		if (doesFileExist localPoseFile) then
		(
			c2PickableNanoObjects = getMAXFileObjectNames localPoseFile quiet:true
			
			if response == "load" then
			(
				loadMaxFile localPoseFile quiet:true
			)
			else if response == "merge" then
			(
				c2PickableOrigObjects = objects as array
				mergeMaxFile localPoseFile #skipDups #useSceneMtlDups quiet:true
			)
		) else (
			print ("FILE DOES NOT EXIST: " + localPoseFile)
		)
	)

	fn fnC2MakePickDummies type =
	(
		if (selection.count != 1) then
		(
			messageBox("You must select only the pickable mesh object.")
			return()
		)
		if (superclassof selection[1] != GeometryClass) then
		(
			messageBox("You must select only the pickable mesh object.")
			return()
		)
		
		pickObj = selection[1]
		exportNode = pickObj
		if (isValidNode pickObj.parent) then
			exportNode = pickObj.parent
		
		pickChilds = pickObj.children
		for child in pickChilds do
		(
			if (classOf child == Dummy) then
			(
				if (matchPattern child.name pattern:"player_grab*" ignoreCase:true) == true then
				(
					delete child
				)
			)
		)
		
		rHand = $'_Bip01 weapon_bone'
		if (isValidNode rHand) == false then
		(
			messageBox("Cannot find _Bip01 weapon_bone!")
			return()
		)
		
		rHandPos = biped.getTransform rHand #pos
		rHandRot = biped.getTransform rHand #rotation
		dummy1 = Dummy pos:[0,0,0] isSelected:off
		dummy1.rotation = inverse rHandRot
		dummy1.position = rHandPos
		if (type == "1H") then
			dummy1.name = pickObj.name + "_player_grab_1h"
		if (type == "pole") then
			dummy1.name = pickObj.name + "_player_grab_pole"
		if (type == "2H") then
			dummy1.name = pickObj.name + "_player_grab"
		dummy1.parent = pickObj
		
		pickObj.rotation = origPropRot
		pickObj.position = origPropPos
		
		UtilityPanel.OpenUtility csExport
		csexport.export.set_node_list(#())
		csexport.export.set_node_list(#(exportNode))
			
		select dummy1
	)

	fn fnC2SetPickablePose =
	(
		local pickHelper = undefined
		local rHand = $'_Bip01 weapon_bone'
		local pickableObject = undefined
		
		if (selection.count != 1) then
		(
			messageBox("You must select the pickable helper or object.")
			return()
		)
		if rHand == undefined then
		(
			messageBox("You must import one of the nanosuit pickable Poses first.")
			return()
		)
		
		if (classOf selection[1] != Dummy) then
		(
			pickableObject = selection[1]
			
			for child in pickableObject.children do
			(
				if classOf child == Dummy then
				(
					pickHelper = child
					break
				)
			)
			if pickHelper == undefined then
			(
				-- If we didn't find a helper node, then try to align the selected object to between the nanosuit hands
				
				origPropPos = pickableObject.position
				origPropRot = pickableObject.rotation

				local bb = nodeLocalBoundingBox pickableObject
				local center = (bb[1] + bb[2]) / 2
				local offset = center - pickableObject.position
				local rHand = $'Bip01 R Hand'
				local lHand = $'Bip01 L Hand'
				
				local newpos = (rHand.transform.position + lHand.transform.position)/2 + (point3 0 30 0)
				
				selection[1].position = newpos - offset
				
				return()
			)
		) else (
			pickHelper = selection[1]
			pickableObject = pickHelper.parent

		)
		
		origPropPos = pickableObject.position
		origPropRot = pickableObject.rotation

		print origPropPos
		print origPropRot
		
		pickObj = pickHelper.parent
		pickParent = pickObj.parent
		
		dumTrans = pickHelper.transform
		dummy1 = Dummy pos:[0,0,0] isSelected:off
		dummy1.transform = dumTrans
		pickObj.parent = dummy1

		rHandPos = rHand.objecttransform.position
		rHandRot = rHand.objecttransform.rotation
		dummy1.rotation = inverse rHandRot
		dummy1.position = rHandPos
		if (pickParent != undefined) then
			pickObj.parent = pickParent
		delete dummy1
		select pickHelper
	)

	fn fnC2ResetPickablePos = 
	(
		local pickObj = undefined
		local pickHelper = undefined
		
		if (selection.count != 1) then
		(
			messageBox("You must select the pickable helper or object.")
			return()
		)
		if (classOf selection[1] != Dummy) then
		(
			pickObj = selection[1]
			for child in pickObj.children do
			(
				if classOf child == Dummy then
				(
					pickHelper = child
					break
				)
			)
		) else (
			pickHelper = selection[1]
			pickObj = pickHelper.parent
		)

		pickObj.rotation = origPropRot
		pickObj.position = origPropPos
	)






	
	on btn_c2Open2HPose pressed do
	(
		fnC2GetPickPose "2H"
	)
	on btn_c2Make2HDummies pressed do
	(
		fnC2MakePickDummies "2H"
	)
	
	on btn_c2Open1HPose pressed do
	(
		fnC2GetPickPose "1H"
	)
	on btn_c2Make1HDummies pressed do
	(
		fnC2MakePickDummies "1H"
	)
	
	on btn_c2Open1HPolePose pressed do
	(
		fnC2GetPickPose "pole"
	)
	on btn_c2Make1HPoleDummies pressed do
	(
		fnC2MakePickDummies "pole"
	)
	on btn_c2Set1pCam pressed do
	(
		if isValidNode $_camera_head then
		(
			origViewTM = getViewTM()
			viewport.setCamera $_camera_head
		)
	)
	on btn_c2SetPerspCam pressed do
	(
		viewport.setType #view_persp_user
		viewport.setTM origViewTM
	)
	on btn_c2PreviewPos pressed do
	(
		fnC2SetPickablePose()
	)
	on btn_c2ResetPos pressed do
	(
		fnC2ResetPickablePos()
	)
	
	fn getChildrenRecur obj objCollection = 
	(
		append objCollection obj
		
		for child in obj.children do
		(
			append objCollection child
			getChildrenRecur child objCollection
		)
		return objCollection
	)
	
	on btn_c2CleanupPickableScene pressed do
	(
		local bipRoot = $Bip01
		objCollection = #()
		objCollection = getChildrenRecur $Bip01 objCollection

		select objCollection
		try(delete selection) catch()
		
		for obj in c2PickableNanoObjects do
		(
			try(delete (getNodeByName obj)) catch()
		)
	)
)

try(removeSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryPickableSetup_rollout")) catch()
try(cryMaxTools.basic.ROMan.cryAdd "rltCryPickableSetup_rollout" rltCryPickableSetup_rollout #main) catch()
try(addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryPickableSetup_rollout")) catch()
	