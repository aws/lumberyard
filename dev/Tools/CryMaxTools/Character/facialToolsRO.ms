---------------------------
-- CryMaxTools v2.0
-- by Harald Zlattinger
---------------------------

rollout rltCryFCLExpression_Tool "Facial Expressions Tool" width:190 --height:496
(
	button btn_FCLExpressionToolStart "Facial Expressions Tool"
	
	on btn_FCLExpressionToolStart pressed do
	(
		fileIn (cryMaxTools.basic.vars.toolsPath + "Character\\facialTools.ms") --add rollout
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryFCLExpression_Tool" rltCryFCLExpression_Tool #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryFCLExpression_Tool")