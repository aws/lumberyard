---------------------------
-- CryMaxTools v2.0
-- ARMS v1.0
-- by Harald Zlattinger
---------------------------

rollout rltCryBreakable_Tool "CryENGINE Breakable Tool" width:190 --height:496
(
	button btn_CryBreakableToolStart "CryENGINE Breakable Tool"
	
	on btn_CryBreakableToolStart pressed do
	(
		fileIn (cryMaxTools.basic.vars.toolsPath + "Model\\CryBreakableTool.ms") --add rollout
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryBreakable_Tool" rltCryBreakable_Tool #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryBreakable_Tool")