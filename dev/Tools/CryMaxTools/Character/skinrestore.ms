	if selection.count != 0 then
	(
		with undo on 
		(
			--#### Find Skindata Mesh and Skin it
			--#### by Marcus Krautwurst
			
			BipNode = $Bip01
-- 			BipNode.controller.figuremode = true
			
			fObjectsProcessed = 0
			aGetSkinnedBones = #()

			fn MaxRedraw arg =
			(
				WM_SETREDRAW=0xB
				commandHWND = (windows.getChildHWND #max "Command Panel")
				windows.sendmessage (commandHWND[1]) WM_SETREDRAW arg 0
				if arg == 1 then (cui.commandPanelOpen=true)
			)

			fn resetObject pTarget =
			(
				ResetXForm pTarget
				for i = 1 to pTarget.modifiers.count do
				(
					if classOf pTarget.modifiers[i] == XForm then
						maxOps.CollapseNodeTo pTarget i off	
				)
			)


			fn getSkinDataBones SkinDataMesh =
			(	
				aChannels = #()	
				sUserPropBuffer = getUserPropBuffer SkinDataMesh
				aGetChannel  = (filterstring sUserPropBuffer "\r\n")
				
				for i = 1 to aGetChannel.count do
				(
					if findString aGetChannel[i] "MapChannel" != undefined then
					(
						local tempBone = (filterString aGetChannel[i] ":")[3]
						append aGetSkinnedBones tempBone
					)
				)
			)



			fn skinfromSkinData pTarget =
			(	
				SkinData = (getNodeByName ("SkinData_" + pTarget.name))	
				if SkinData != undefined then
				(			
					getSkinDataBones SkinData
					addModifier pTarget (CrySkin())
					SkinMod = pTarget.modifiers[1]
					-- Add Bones
					for i = 1 to aGetSkinnedBones.count do
					(
						-- Update the SkinModifier only at last bone-add
						try((getSkinOps sel:SkinMod).addbone SkinMod (getNodeByName (aGetSkinnedBones[i])) 0)catch()
					)
					-- Load in Skin from SkinData Mesh
					select pTarget
					selectMore SkinData
					skinUtils.ImportSkinDataNoDialog true false false false false 1 0
					select pTarget
					aGetSkinnedBones = #()
				)
			)


			pTargets = selection as array
			max modify mode
			MaxRedraw 0
			
			try(destroyDialog skinFromSkinDataWindow)catch()
			rollout skinFromSkinDataWindow ""
			(
				progressBar ProgBar value:0 height:10 width:300 color:(color 88 143 225) offset:[-14,-5]
				label lblProg "0%"  offset:[0,-17]
			)
			createDialog skinFromSkinDataWindow width:300 height:10 style:#()
			
			for i = 1 to pTargets.count do
			(		
				gc light:true
				try
				(
					select pTargets[i]
					resetObject pTargets[i]
					skinfromSkinData pTargets[i]			
				)catch()
				gc light:true
				skinFromSkinDataWindow.ProgBar.value = ((100/pTargets.count)*i)		
				skinFromSkinDataWindow.lblProg.caption = (((100/pTargets.count)*i)as string + "%")
				
				fObjectsProcessed += 1
			)	

			for i = 1 to pTargets.count do
			(
				SkinMod = pTargets[i].modifiers[#CrySkin]
				SkinMod.bone_limit = 4
			)
			select pTargets
			MaxRedraw 1
			try(destroyDialog skinFromSkinDataWindow)catch()
		)
-- 		BipNode.controller.figuremode = false
	)
	else messagebox "Select something"

