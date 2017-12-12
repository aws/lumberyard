------------------------
-- CryMaxTools v2.0
-- Toolbox v1.0
-- by Sascha Herfort
------------------------

sToolPath = cryMaxTools.basic.vars.toolsPath

if cryMaxTools.toolBox != undefined then
(--destroy toolbox if already present
	try
	(
		cui.FloatDialogBar cryMaxTools.toolBox
		cui.UnRegisterDialogBar cryMaxTools.toolBox
	)
	catch()
	destroyDialog cryMaxTools.toolBox
)

rollout rltCryMaxToolBox "CryToolBox"
(
	local sCryToolboxIcons = sToolPath + "Icons\\cryToolbox_32i.bmp"
	local sCryToolboxIconMasks = sToolPath + "Icons\\cryToolbox_32a.bmp"
	local sCryToolboxDock = sToolPath + "Icons\\cryToolboxDock_32i.bmp"
	local sCryToolboxDockMasks = sToolPath + "Icons\\cryToolboxDock_32a.bmp"
	
	button btnDockLeft "<---" height:16 width:50 pos:[0,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,1,1,1,1)
	button btnUndock "- - - -" height:16 width:90 pos:[50,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,2,2,2,2)
	button btnDockRight "--->" height:16 width:50 pos:[140,0] images:#(sCryToolboxDock,sCryToolboxDockMasks,3,3,3,3,3)
	checkbutton ckbtnAnimTools "Anim" height:32 width:32 pos:[0,16] images:#(sCryToolboxIcons,sCryToolboxIconMasks,4,1,1,1,1)
	checkbutton ckbtnCharacterTools "Char" height:32 width:32 pos:[32,16] images:#(sCryToolboxIcons,sCryToolboxIconMasks,4,2,2,2,2)
	checkbutton ckbtnModellingTools "Model" height:32 width:32 pos:[64,16] images:#(sCryToolboxIcons,sCryToolboxIconMasks,4,3,3,3,3)
	checkbutton ckbtnExporter "Export" height:32 width:32 pos:[96,16] images:#(sCryToolboxIcons,sCryToolboxIconMasks,4,4,4,4,4)
	--checkbutton ckbtnSettings "Settings" height:32 width:32 pos:[128,16] images:#(sCryToolboxIcons,sCryToolboxIconMasks,4,4,4,4,4)
	subrollout rltToolHolder height:480 width:190 pos:[0,48]
	
--toolbox global variables
	local aRolloutFiles = #( --stores files that contains rollouts per tab -- !!! ADD NEW ROLLOUTS HERE !!! -- changing sequence, will screw up rolledup states
		"Anim",#(
			"Animation\\AnimToolBox.ms",
			"Animation\\techAnimTools.ms"
		),
		"Char",#(
			"Character\\riggingTools.ms",
			"Character\\diagnosticTools.ms",
			"Character\\setLimits.ms",
			"Character\\physicsTools.ms",
			"Character\\skinningTools.ms",
			"Character\\techArtTools.ms",
			"Character\\exporterTools.ms",
			"Character\\facialToolsRO.ms"
		),
		"Model",#(
			"Model\\polyTools.ms",
			"Model\\vertexColorTools.ms",
			"Model\\materialTools.ms",
			"Model\\exportToSandbox.ms",
			"Model\\physProxyTools.ms",
			"Model\\coverHelperTools.ms",
			"Model\\ARMS_Max_Tool_Rollout.ms",
			"Model\\fp16QuantizationTools.ms",
			"Model\\pickableObjectSetup.ms",
			"Model\\cryBreakableTool_Rollout.ms"			
		),
		"Export",#(
			--"Exporter\\ObjectExporter.ms",
			"Exporter\\materialExporter.ms",
			"Exporter\\animationExporter.ms",
			"Exporter\\splitLODExporter.ms",
			"Exporter\\exporterSetup.ms",
			"Exporter\\exportToTrackview.ms"
		),
		"Settings",#(
			--"Exporter\\exporterSetup.ms"
		)
	)
	
	local sCurrentTab = undefined --stores currently opened tab
	local aRolloutStates = #( --stores the rolledup states of the rollouts per tab
		"Anim",#{},
		"Char",#{},
		"Model",#{},
		"Export",#{},
		"Settings",#{}
	)
	
	
--toolbox global functions
	fn fnSetToolAnim =
	(--set anim tools active
		ckbtnAnimTools.state = true
		ckbtnCharacterTools.state = false
		ckbtnModellingTools.state = false
		ckbtnExporter.state = false
		--ckbtnSettings.state = false
		
		if (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count > 0 then
		(
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
			(
				aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1].open -- save rolledup state
				removeSubRollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1]
			)
		)
		
		local aCurrentRollouts = aRolloutFiles[(findItem aRolloutFiles "Anim")+1]
		local aCurrentStates = aRolloutStates[(findItem aRolloutFiles "Anim")+1]
		for i = 1 to aCurrentRollouts.count do
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + aCurrentRollouts[i]) --add rollout
			((cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i]).open = aCurrentStates[i] --restore rolledup state
		)
		
		sCurrentTab = "Anim"
	)
	
	fn fnSetToolChar =
	(--set character tools active
		ckbtnAnimTools.state = false
		ckbtnCharacterTools.state = true
		ckbtnModellingTools.state = false
		ckbtnExporter.state = false
		--ckbtnSettings.state = false
		
		if (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count > 0 then
		(
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
			(
				aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1].open -- save rolledup state
				removeSubRollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1]
			)
		)
		
		local aCurrentRollouts = aRolloutFiles[(findItem aRolloutFiles "Char")+1]
		local aCurrentStates = aRolloutStates[(findItem aRolloutFiles "Char")+1]
		for i = 1 to aCurrentRollouts.count do
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + aCurrentRollouts[i]) --add rollout
			((cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i]).open = aCurrentStates[i] --restore rolledup state
		)
		
		sCurrentTab = "Char"
	)
	
	fn fnSetToolModel =
	(--set modeling tools active
		ckbtnAnimTools.state = false
		ckbtnCharacterTools.state = false
		ckbtnModellingTools.state = true
		ckbtnExporter.state = false
		--ckbtnSettings.state = false
		
		if (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count > 0 then
		(
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
			(
				aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1].open -- save rolledup state
				removeSubRollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1]
			)
		)
		
		local aCurrentRollouts = aRolloutFiles[(findItem aRolloutFiles "Model")+1]
		local aCurrentStates = aRolloutStates[(findItem aRolloutFiles "Model")+1]
		for i = 1 to aCurrentRollouts.count do
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + aCurrentRollouts[i]) --add rollout
			((cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i]).open = aCurrentStates[i] --restore rolledup state
		)
		
		sCurrentTab = "Model"
	)
	
	fn fnSetToolExport =
	(--set exporter tools active
		ckbtnAnimTools.state = false
		ckbtnCharacterTools.state = false
		ckbtnModellingTools.state = false
		ckbtnExporter.state = true
		--ckbtnSettings.state = false
		
		if (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count > 0 then
		(
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
			(
				aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1].open -- save rolledup state
				removeSubRollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1]
			)
		)
		
		local aCurrentRollouts = aRolloutFiles[(findItem aRolloutFiles "Export")+1]
		local aCurrentStates = aRolloutStates[(findItem aRolloutFiles "Export")+1]
		for i = 1 to aCurrentRollouts.count do
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + aCurrentRollouts[i]) --add rollout
			((cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i]).open = aCurrentStates[i] --restore rolledup state
		)
		
		sCurrentTab = "Export"
	)
	
	fn fnSetToolSettings =
	(--set settings tab active
		ckbtnAnimTools.state = false
		ckbtnCharacterTools.state = false
		ckbtnModellingTools.state = false
		ckbtnExporter.state = false
		--ckbtnSettings.state = true
		
		if (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count > 0 then
		(
			for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
			(
				aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1].open -- save rolledup state
				removeSubRollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[1]
			)
		)
		
		local aCurrentRollouts = aRolloutFiles[(findItem aRolloutFiles "Settings")+1]
		local aCurrentStates = aRolloutStates[(findItem aRolloutFiles "Settings")+1]
		for i = 1 to aCurrentRollouts.count do
		(
			fileIn (cryMaxTools.basic.vars.toolsPath + aCurrentRollouts[i]) --add rollout
			((cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i]).open = aCurrentStates[i] --restore rolledup state
		)
		
		sCurrentTab = "Settings"
	)
	
--toolbox UI event handlers
	on rltCryMaxToolBox open do
	(
		local aToolboxSettings = cryMaxTools.basic.settingsMan.load "toolboxSettings"
		
		if aToolboxSettings != false then
		(
			local p2TempPos = filterString aToolboxSettings[1].value[1] "[,]"
			p2TempPos = point2 (p2TempPos[1] as float) (p2TempPos[2] as float)
			local fTempHeight = aToolboxSettings[2].value[1] as float
			
			try(SetDialogPos rltCryMaxToolBox p2TempPos)
			catch(print "Couldn't load Toolbox position")
			try(rltCryMaxToolBox.height = fTempHeight)
			catch(print "Couldn't load Toolbox size")
			
			for i = 1 to aRolloutStates.count do
			(--load rolledup states
				if classOf aRolloutStates[i] == bitArray then
				(
					local sString = aToolboxSettings[6].value[i]
					for j = 1 to sString.count do
					(
						aRolloutStates[i][j] = (sString[j] == "1")
					)
				)
			)
			
			try(case aToolboxSettings[3].value[1] of
				(
					"Anim": fnSetToolAnim()
					"Char": fnSetToolChar()
					"Model": fnSetToolModel()
					"Export": fnSetToolExport()
					"Settings": fnSetToolSettings()
				)
			)
			catch(
				sCurrentTab = "Export"
				fnSetToolExport()
				print "Couldn't load Toolbox tab"
			)
			
			local sTempScrollPos = aToolboxSettings[4].value[1] as integer
			if rltCryMaxToolBox.rltToolHolder.rollouts.count > 0 then rltCryMaxToolBox.rltToolHolder.rollouts[1].scrollPos = sTempScrollPos
				
			local sTempDockState = aToolboxSettings[5].value[1]
			if sTempDockState == "cui_dock_left" then
			(
				cui.RegisterDialogBar rltCryMaxToolBox style:#()
				cui.DockDialogBar rltCryMaxToolBox #cui_dock_left
			)
			if sTempDockState == "cui_dock_right" then
			(
				cui.RegisterDialogBar rltCryMaxToolBox style:#()
				cui.DockDialogBar rltCryMaxToolBox #cui_dock_right
			)
		)
	)
	
	
	on rltCryMaxToolBox moved value do
	(
		try rltCryExportAnimation.updateRangeUI() catch()
	)
	
	
	
	on rltCryMaxToolBox close do
	(
		local aToolboxSettings = #("toolboxSettings")
		append aToolboxSettings ("position*@" + (GetDialogPos rltCryMaxToolBox) as string) --store toolbox position
		append aToolboxSettings ("height*@" + (GetDialogSize rltCryMaxToolBox)[2] as string) --store toolbox size
		append aToolboxSettings ("currentTab*@" + sCurrentTab as string) --store currently opened tab
		
		try(sTempScrollPos = rltCryMaxToolBox.rltToolHolder.rollouts[1].scrollPos as string)catch(sTempScrollPos = "0")
		append aToolboxSettings ("scrollPosition*@" + sTempScrollPos) --store rollout scrollposition
		
		try(sTempDockState = cui.getDockState rltCryMaxToolBox)catch(sTempDockState = "")
		append aToolboxSettings ("dockState*@" + sTempDockState) --store dockstate
		
		for i = 1 to (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts.count do
		(--update rollouts states of current tab
			aRolloutStates[(findItem aRolloutStates sCurrentTab)+1][i] = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder.rollouts[i].open -- save rolledup state
		)
		local sStatesToSave = ""
		for i = 1 to aRolloutStates.count do
		(--save rolledup states
			if classOf aRolloutStates[i] == string then
			(
				local sStates = ""
				for j = 1 to aRolloutStates[i+1].count do
				(
					if aRolloutStates[i+1][j] == true then
					(
						append sStates "1"
					)
					else
					(
						append sStates "0"
					)
				)
				append sStatesToSave (aRolloutStates[i] + "|@" + sStates + "|@")
			)
		)
		append aToolboxSettings ("rolledUpStates*@" + sStatesToSave)
		
		cryMaxTools.basic.settingsMan.save aToolboxSettings
	)
	
	on btnDockLeft pressed do
	(
		try
		(
			local sDockState = cui.getDockState rltCryMaxToolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState == undefined then
		(--register if not yet done
			cui.RegisterDialogBar rltCryMaxToolBox style:#() maxsize:[190,1200]
		)
		cui.DockDialogBar rltCryMaxToolBox #cui_dock_left
		
		rltCryMaxToolBox.rltToolHolder.height = rltCryMaxToolBox.height - rltCryMaxToolBox.rltToolHolder.pos[2]
	)
	
	on btnDockRight pressed do
	(
		try
		(
			local sDockState = cui.getDockState rltCryMaxToolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState == undefined then
		(--register if not yet done
			cui.RegisterDialogBar rltCryMaxToolBox style:#() maxsize:[190,1200]
		)
		cui.DockDialogBar rltCryMaxToolBox #cui_dock_right
		
		rltCryMaxToolBox.rltToolHolder.height = rltCryMaxToolBox.height - rltCryMaxToolBox.rltToolHolder.pos[2]
	)
	
	on btnUndock pressed do
	(
		try
		(
			local sDockState = cui.getDockState rltCryMaxToolBox
		)
		catch
		(--not a registered dialog bar
			local sDockState = undefined
		)
		
		if sDockState != undefined then
		(--float and undock if registerd
			cui.FloatDialogBar rltCryMaxToolBox
			cui.UnRegisterDialogBar rltCryMaxToolBox
		)
		
		rltCryMaxToolBox.rltToolHolder.height = rltCryMaxToolBox.height - rltCryMaxToolBox.rltToolHolder.pos[2]
	)
	
	on ckbtnAnimTools changed state do
	(
		if state then
		(
			fnSetToolAnim()
		)
		else
		(
			fnSetToolExport()
		)
	)
	
	on ckbtnCharacterTools changed state do
	(
		if state then
		(
			fnSetToolChar()
		)
		else
		(
			fnSetToolExport()
		)
	)
	
	on ckbtnModellingTools changed state do
	(
		if state then
		(
			fnSetToolModel()
		)
		else
		(
			fnSetToolExport()
		)
	)
	
	on ckbtnExporter changed state do
	(
		fnSetToolExport()
	)
	
	/*
	on ckbtnSettings changed state do
	(
		if state then
		(
			fnSetToolSettings()
		)
		else
		(
			fnSetToolExport()
		)
	)
	*/
	
	on rltCryMaxToolBox resized size do
	(
		rltCryMaxToolBox.rltToolHolder.height = size[2] - rltCryMaxToolBox.rltToolHolder.pos[2]
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryMaxToolBox" rltCryMaxToolBox #main
createDialog (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox") height:512 width:190 style:#(#style_titlebar, #style_resizing, #style_sysmenu) lockWidth:true

cryMaxTools.toolBox = (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox")