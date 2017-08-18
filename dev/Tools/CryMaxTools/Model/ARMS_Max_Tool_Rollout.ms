---------------------------
-- CryMaxTools v2.0
-- ARMS v1.0
-- by Harald Zlattinger
---------------------------

rollout rltCryARMS_Max_Tool "ARMS Max Tool" width:190 --height:496
(
	button btn_ARMSMaxToolStart "ARMS Max Tool"
	
	on btn_ARMSMaxToolStart pressed do
	(
		fileIn (cryMaxTools.basic.vars.toolsPath + "Model\\ARMS_Max_Tool.ms") --add rollout
	)
)

--add exporter rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryARMS_Max_Tool" rltCryARMS_Max_Tool #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryARMS_Max_Tool")

