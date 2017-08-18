---------------------------
-- CryMaxTools v2.0
-- Vertex Color Tools v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryVertexColorTools "Vertex Color Tools" width:190
(
--######################################################################################
--###  GUI  ###############################################################################
--######################################################################################
	
	group "Invert Vertex Colors"
	(
		button btnInvertVertexAlpha "Invert Vertex Alpha" width:160 height:16 align:#center offset:[-1,-2] tooltip:"Selects faces with more/less than 4 vertices. CTRL adds to selection."
	)

	
--######################################################################################
--###  GLOBAL VARIABLES  #####################################################################
--######################################################################################
	
	
--######################################################################################
--###  FUNCTIONS  ##########################################################################
--######################################################################################
	
	fn fnInvertVertexAlpha nPoly =
	(--invert vertex alpha of supplied editable_poly
		if classOf nPoly == editable_poly and polyop.getMapSupport nPoly -2 then
		(
			for i = 1 to (polyop.getNumMapVerts nPoly -2) do
			(
				local p3CurrentAlpha = polyOp.getMapVert nPoly -2 i
				polyOp.setMapVert nPoly -2 i ([1.0,1.0,1.0] - p3CurrentAlpha)
				update nPoly
			)
			
			true
		)
		else false
	)
	
	
--######################################################################################
--###  USER INPUT EVENT HANDLERS  ##############################################################
--######################################################################################
	
	on btnInvertVertexAlpha pressed do
	(--invert vertex alpha
		for each in $selection do
		(
			fnInvertVertexAlpha each
		)
	)
)

--add rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryVertexColorTools" rltCryVertexColorTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryVertexColorTools")