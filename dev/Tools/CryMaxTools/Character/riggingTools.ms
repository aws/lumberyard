-------------------------------------------
-- CryMaxTools v2.0
-- Rigging Tools
-- by Chris Evans and Sascha Herfort
---------------------------

rollout rltCryRigging "Rigging Tools"
(
	group "General Tools"
	(
		button matchPivotBtn "matchPivot" height:16 width:80 offset:[-2,-2] across:2 align:#center
		button zeroOut "zeroOut Rots" height:16 width:80 offset:[3,-2] align:#center
		
		button btnAlignNode "Align Sel 1 -> Sel 2" height:16 width:110 offset:[-5,3] across:2 align:#left
		checkbox chkAlignNodeScale "Scale" height:16 width:50 offset:[8,3] align:#right
		
		button clampTimeline "Clamp Timeline at Current" height:16 width:160 offset:[0,3] align:#center
		button clampTimelineKey "Clamp Timeline at Last Key" height:16 width:160 offset:[0,-5] align:#center

		button reduceKeysBTN "Reduce Keys on Sel" height:16 width:160 offset:[0,3] align:#center
	)
	
	group "Biped Tools"
	(
		dropdownlist bipSelect "" items:#("") width:144 offset:[-5,-2] align:#left across:2
		button refreshBipTools "<" offset:[4,-2] width:16 height:20 tooltip:"Refresh" align:#right
		checkbutton figureMode "Figure Mode" height:16 width:80 offset:[-2,3] across:2 align:#center
		checkbutton hideBiped " Hide Biped" height:16 width:80 offset:[3,3] align:#center
		button bipMotion "Bip Motion Menu" height:16 width:160 offset:[0,3] align:#center
		button selectAllBIP "Select Biped Bones" height:16 width:160 offset:[0,3] align:#center
		checkbutton inPlace "Toggle In Place Mode" height:16 width:160 offset:[0,3] align:#center
		button convertBip "Convert Biped to Bones" height:16 width:160 offset:[0,3] align:#center
	)
	
	group "Vertex Tools"
	(
		button channelInfoBTN "channelInfo" height:16 width:80 offset:[-2,-2] across:2 align:#center
		checkbutton vertColors "vertexColors" height:16 width:80 offset:[3,-2] align:#center
		button addChannel "Add cryChannel to Selected" height:16 enabled:false width:160 offset:[0,-5] align:#center
		label cryChannel "There is currently no cryChannel" offset:[0,-2] align:#center
	)

--	on rigging tools open
	on rltCryRigging open do
	(	
		if $bip01 != undefined then
		(
			if (LayerManager.getLayerFromName "Bip") == undefined then
			(
				messageBox "The BIP layer is not named \"Bip\"" title: "Error"
				return undefined
			)
			bip_layer = LayerManager.getLayerFromName "Bip"
			if bip_layer.ishidden == true then
			(
				hideBiped.checked = true
			)
			
			biped_ctrl=$bip01.controller
			if biped_ctrl.figureMode == true then
			(
				figureMode.checked = true
			)
			else
			(
				figureMode.checked = false
			)
			
			figureSet = "if $bip01 != undefined then\n"
			figureSet += "(\n"
			figureSet += "if $bip01.controller.figureMode == true then ((cryMaxTools.basic.ROMan.get \"rltCryRigging\").figureMode.checked = true) else ((cryMaxTools.basic.ROMan.get \"rltCryRigging\").figureMode.checked = false)\n"
			figureSet += ")\n"
			figureSet += "else (print \"no biped found\")"
			callbacks.addscript #filePostOpen figureSet id:#figureMode
			
			BIPhideSet = "bip_layer = LayerManager.getLayerFromName \"Bip\"\n"
			BIPhideSet += "if bip_layer == undefined then\n"
			BIPhideSet += "(\n"
			BIPhideSet += "if bip_layer.ishidden == true then ((cryMaxTools.basic.ROMan.get \"rltCryRigging\").hideBiped.checked = true) else ((cryMaxTools.basic.ROMan.get \"rltCryRigging\").hideBiped.checked = false)\n"
			BIPhideSet += ")\n"
			BIPhideSet += "else (print \"there is no bip layer\")"
			callbacks.addscript #filePostOpen BIPhideSet id:#bipHide
			
			global bips = (cryMaxTools.getBips())
			namearr = #()
			for i=1 to bips.count do
			(
				append namearr bips[i].name
			)
			bipSelect.items = namearr
			global selectedBip = ("$'" + bips[1].name + "'")
			global biped_ctrl = (execute (selectedBip + ".controller"))
		)
	)
		
-- on rigging tools closed
	on rltCryRigging close do
	(
		callbacks.removescripts #filePostOpen id:#figureMode
		callbacks.removescripts #filePostOpen id:#bipHide
	)
		
-- on clampTimeline pressed
	on clampTimeline pressed do
	(
		if animationrange.start == slidertime then
		(
			messagebox ("Timeline cannot be clamped to a single frame (" + slidertime as string + ")")
			return undefined
		)
		animationrange = interval animationrange.start slidertime
	)

-- clampTimelineKey 
	on clampTimelineKey  pressed do
	(
		if $ == undefined then
		(
			messagebox "Nothing is selected"
			return undefined
		)
		
		try
		(
			if $.controller.keys.count == -1 then
			(
				positionK = $.position.controller.keys[$.position.controller.keys.count].time
				rotationK = $.rotation.controller.keys[$.rotation.controller.keys.count].time
				if positionK > rotationK then
				(
					if positionK == animationrange.start then (messagebox "Cannot clamp to timeline start"; return undefined)
					animationrange = interval animationrange.start positionK
					return undefined
				)
				else
				(
					if positionK == animationrange.start then (messagebox "Cannot clamp to timeline start"; return undefined)
					animationrange = interval animationrange.start rotationK
					return undefined
				)
			)
			
			if $ != undefined or $.controller.keys[$.controller.keys.count].time != animationrange.start then
			(
				animationrange = interval animationrange.start $.controller.keys[$.controller.keys.count].time
			)
			else
			(
				messagebox "Object's last keyframe is current key.\nCannot clamp timeline to 1 key"
			)
		)
		catch()
	)
	
--reduce keys
	on reduceKeysBTN pressed do
	(
		try
		(
			undo "reduceKeys" on
			(
				for obj in selection do
				(
					if copyPos.checked == true then
					(
						reduceKeys obj.position.controller 50 1f
					)
					if copyRot.checked == true then
					(
						reduceKeys obj.rotation.controller 50 1f
					)
				)
			)
		)
		catch()
	)

--	crytools.matchPivot button
	on matchPivotBtn pressed do
	(
		if selection.count == undefined or selection.count != 2 then
		(
			messageBox "Please select two objects, the second being the object whos pivot you want to change." title: "Error"
			return undefined
		)
		undo "matchPivot" on
		(
			crytools.matchPivot $[1] $[2]
		)
	)
			
--	zero out button
	on zeroOut pressed do
	(
		objs = getCurrentSelection();
		oc = objs.count;
		undo "ZEROOut" on
		(
			setWaitCursor();
			for i in 1 to oc do 
			(
				pt = Point pos:[0,0,0] isSelected:on centermarker:on axistripod:off cross:off Box:off constantscreensize:off drawontop:off size:20;
				pt.parent = objs[i].parent;
				coordsys world 
				(
					pt.scale = objs[i].scale;
					pt.rotation = objs[i].rotation;
					pt.position = objs[i].position;
					pt.wirecolor = (color 80 10 0);
					pt.name = (objs[i].name+"_ZERO");
				)
				objs[i].parent = pt;
			)
			select objs;
			setArrowCursor();
		)
	)

	
	on btnAlignNode pressed do
	(
		if selection.count > 1 then
			cryMaxTools.char.alignNode selection[1] selection[2] useScale:chkAlignNodeScale.checked
		else
			messageBox "2 Nodes need to be selected" caption:"Align Node"
	)
	
	
	
	on refreshBipTools pressed do
	(
		try
		(
			global bips = (crytools.getBips())
			global namearr = #()
			for i=1 to bips.count do
			(
				append namearr bips[i].name
			)
			bipSelect.items = namearr
			global selectedBip = ("$'" + bips[1].name + "'")
			global biped_ctrl = (execute (selectedBip + ".controller"))
		)
	catch(print "no bips selected/found")
	)

	on bipSelect selected s do
	(
		print (bipSelect.items[s] + " selected.")
		selectedBip = ("$'" + bips[s].name + "'")
		global biped_ctrl = (execute (selectedBip + ".controller"))
	)

-- figure mode button
		on figureMode changed state do
		(
		try
			if (figureMode.checked == true) then 
			(
				biped_ctrl=(execute (selectedBip + ".controller"))
				biped_ctrl.figureMode = true
			)
			else
			(
				biped_ctrl=(execute (selectedBip + ".controller"))
				biped_ctrl.figureMode = false
			)
			catch
			( 
				messageBox "Please load a biped." title:"Error"
				figuremode.Checked = false
				return undefined
			)
		)
		
-- hide biped button
	on hideBiped  changed state do
	(
		try
		(
			if (hideBiped.checked == true) then
			(
				bip_layer = LayerManager.getLayerFromName "Bip"
				bip_layer.ishidden = true
			)
			else
			(
				bip_layer = LayerManager.getLayerFromName "Bip"
				bip_layer.ishidden = false
			)
		)
		catch
		(
			messageBox "Cannot find 'Biped' layer." title:"Error"
			hideBiped.checked = false
			return undefined
		)
	)

--	bip motion menu button
	on bipMotion pressed do
	(
		try
		(
			selectedbipOrig = selectedbip as string
			selectedbip = crytools.cutstring selectedbip "$"
			selectedbip = crytools.cutstring selectedbip "'"
			selectedbip = crytools.cutstring selectedbip "'"
			select (getnodebyname selectedbip)
			max motion mode
			selectedbip = selectedbipOrig 
		)
		catch
		( 
			messageBox "Please load a biped." title:"Error"
			return undefined
		)
	)
		
-- in place mode button
	on inPlace changed state do
	(
		try
			if (inPlace.checked == true) then 
			(
				biped_ctrl.inPlaceMode = true
			)
			else
			(
				biped_ctrl.inPlaceMode = false
			)
		catch
		( 
			messageBox "Please load  a biped, or take the biped out of figure mode." title:"Error"
			inPlace.checked = false
			return undefined
		)
	)
	
	on convertBip pressed do
	(
		--try
		--(
			undo "bip2bones" on
			(
				newbones = #()
				
				if selection == undefined then
				(
					messagebox "nothing is selected"
					return undefined
				)
				
				if selection.count != 1 then
				(
					messagebox "please select only one part of the biped to convert"
					return undefined
				)
				
				objArray = cryMaxTools.anim.functions.OP.sortRootChildren (cryMaxTools.getChildren (cryMaxTools.findRoot $))
				
				for obj in objArray do
				(
					b = snapshot obj name:("bone_" + (obj.name as string))
					b.parent = undefined
					b.transform = obj.transform
					append newbones b
				)

				for i in newbones do
				(
					local boneName = substituteString i.name "bone_" ""
					local b = getnodebyname boneName
					
					if b.parent != undefined then
					(
						i.parent = getnodebyname ("bone_" + b.parent.name)
					)
				)
			)
		--)
		--catch()
	)
		
	on selectAllBIP pressed do
	(
		for obj in objects do
		(
			if obj.classID[1] == 37157 and (not obj.isFrozen) then
			(
				selectMore obj
			)
		)
	)
		
	--	vertex tools
	on channelInfoBTN pressed do
	(
		channelInfo.Dialog ()
	)
		
	on vertColors changed state do
	(
		for obj in selection do
		(
			if obj.showVertexColors == false then
			(
				obj.showVertexColors = true
			)
			else
			(
				obj.showVertexColors = false
			)
		)
	)
		
	on addChannel pressed do
	(
		numChannels = (meshop.getNumVDataChannels $)
		channelinfo.addchannel $
		numChannels += 1
		cryChannel.text = ("cryChannel: channel " + (numchannels as string))
		channelinfo.NameChannel $ 3 numchannels "cry"
		channelinfo.update()
		--s.vertexColorType = #alpha
	)
)

--add rigging tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryRigging" rltCryRigging #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryRigging")
