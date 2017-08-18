--CryENGINE cover&lean helper tools
--by Sascha Herfort

rollout rltCryCoverHelperTools "Cover & Lean Helper Tools" width:190 height:492
(
	--######################################################################################
	--###  GUI  ###############################################################################
	--######################################################################################
	button btnCreate "Cover Helper for Selection" offset:[1,3] width:160 height:16 align:#center
	button btnCreateAligned "Auto Aligned Helper" offset:[34,-5] width:112 height:16 align:#center across:4
	button btnCreateAlignedX "X" offset:[61,-5] width:16 height:16 align:#center
	button btnCreateAlignedY "Y" offset:[40,-5] width:16 height:16 align:#center
	button btnCreateAlignedZ "Z" offset:[19,-5] width:16 height:16 align:#center
	
	group "Adjust selected Helper"
	(
		label lblWarning1 "Warning!" align:#left
		label lblWarning2 "Do not scale the dummy helper." align:#left offset:[0,-5]
		label lblWarning3 "Scale is ignored in game." align:#left offset:[0,-5]
		label lblWarning4 "Use the controls below instead:" align:#left offset:[0,-5]
		spinner spnSizeX "Size Adjust X: " align:#right fieldWidth:64 offset:[5,0] range:[-1000.0,1000.0,0]
		spinner spnSizeY "Size Adjust Y: " align:#right fieldWidth:64 offset:[5,-5] range:[-1000.0,1000.0,0]
		spinner spnSizeZ "Size Adjust Z: " align:#right fieldWidth:64 offset:[5,-5] range:[-1000.0,1000.0,0]
	)
	
	
	--######################################################################################
	--###  GLOBAL VARIABLES  #####################################################################
	--######################################################################################
	
	--TWEAKABLES MAGICIAN
	local sNamePrefix = "$cover"
	local iRefinementSearchSteps = 21 --step inbetween local minimum's meighbors to refine angle
	local iMaxRefinementIterations = 9 --max repeat refinement
	local fMaxError = 1 --stop refining when two successive approximations are less than this close together (centimeters)
	local fInitialSearchRange = 80.0 --should not be changed
	local iInitialSearchSteps = 9 --step along initial arc to find local minimum
	
	--DEBUG OPTIONS
	local bDebugPrint = false
	local bDebugDraw = false
	local bRealtimePreview = false
	
	--GLOBALS
	local p3CurrentSize = [0,0,0]
	
	--######################################################################################
	--###  FUNCTIONS  ##########################################################################
	--######################################################################################
	
	fn fnGetAABBFromVertices nNode aVertices =
	(--returns the axis-aligned bounding box of the supplied vertices
		local aXCoords = for each in aVertices collect nNode.verts[each].pos.x
		local aYCoords = for each in aVertices collect nNode.verts[each].pos.y
		local aZCoords = for each in aVertices collect nNode.verts[each].pos.z
		
		#([aMin aXCoords, aMin aYCoords, aMin aZCoords], [aMax aXCoords, aMax aYCoords, aMax aZCoords])
	)
	
	fn fnGetBBSizeFromRotatedVertices nNode aVertices qOrientation =
	(--returns the bounding box size of the supplied vertices after applying the supplied rotation
		local aXCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).x
		local aYCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).y
		local aZCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).z
		
		[aMax aXCoords, aMax aYCoords, aMax aZCoords] - [aMin aXCoords, aMin aYCoords, aMin aZCoords]
	)
	
	fn fnCreateOrientedCoverAndLeanHelper nNode qOrientation p3Pivot p3Size =
	(--create cover and lean helper and link to supplied node
		local nCoverAndLeanHelper = dummy boxSize:p3Size name:(uniqueName sNamePrefix)
		rotate nCoverAndLeanHelper qOrientation
		nCoverAndLeanHelper.pos = p3Pivot
		nCoverAndLeanHelper.parent = nNode
		
		nCoverAndLeanHelper
	)
	
	fn fnCreateAlignedCoverAndLeanHelper nNode sAxes:#all =
	(--create auto aligned (min volume BB) cover and lean helper
		local aVertices = cryMaxTools.model.poly.fnGetVertexListFromSubObjects nNode subObjectLevel
		local qOrientation = quat 0 0 0 1
		case sAxes of
		(
			#all: (qOrientation = cryMaxTools.model.proxy.fnGetMinimumVolumeBBOrientation nNode aVertices)
			#x: (qOrientation = (cryMaxTools.model.proxy.fnOptimizeLeastOptimalBBAxis nNode aVertices (quat 0 0 0 1) fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{1})[1])
			#y: (qOrientation = (cryMaxTools.model.proxy.fnOptimizeLeastOptimalBBAxis nNode aVertices (quat 0 0 0 1) fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{2})[1])
			#z: (qOrientation = (cryMaxTools.model.proxy.fnOptimizeLeastOptimalBBAxis nNode aVertices (quat 0 0 0 1) fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{3})[1])
		)
		
		print aVertices
		print qOrientation
		
		local aBB = cryMaxTools.model.proxy.fnGetBBFromRotatedVertices nNode aVertices qOrientation
		local p3Size = cryMaxTools.model.proxy.fnGetOrientedBBSize aBB (quat 0 0 0 1)
		local p3Pivot = cryMaxTools.model.proxy.fnGetOrientedBBCentroid aBB (inverse qOrientation)
		
		(fnCreateOrientedCoverAndLeanHelper nNode qOrientation p3Pivot p3Size)
	)
	
	
	--######################################################################################
	--###  MAGIC  #############################################################################
	--######################################################################################
	
	on btnCreate pressed do
	(--create aligned dummy helper
		if $selection.count == 1 and classOf $ == editable_poly then
		(
			nNode = $
			
			local aVertices = cryMaxTools.model.poly.fnGetVertexListFromSubObjects nNode subObjectLevel
			
			local qOrientation = inverse nNode.transform.rotation
			local aBB = cryMaxTools.model.proxy.fnGetBBFromRotatedVertices nNode aVertices qOrientation
			local p3Size = cryMaxTools.model.proxy.fnGetOrientedBBSize aBB qOrientation
			local p3Pivot = cryMaxTools.model.proxy.fnGetOrientedBBCentroid aBB qOrientation
			
			(fnCreateOrientedCoverAndLeanHelper nNode qOrientation p3Pivot p3Size)
		)
	)
	
	on btnCreateAligned pressed do
	(--create min volume BB dummy helper
		if $selection.count == 1 and classOf $ == editable_poly then
		(
			nNode = $
			
			(fnCreateAlignedCoverAndLeanHelper nNode)
		)
	)
	
	on btnCreateAlignedX pressed do
	(--create min volume BB dummy helper - rotate on X only
		if $selection.count == 1 and classOf $ == editable_poly then
		(
			nNode = $
			
			(fnCreateAlignedCoverAndLeanHelper nNode sAxes:#x)
		)
	)
	
	on btnCreateAlignedY pressed do
	(--create min volume BB dummy helper - rotate on X only
		if $selection.count == 1 and classOf $ == editable_poly then
		(
			nNode = $
			
			(fnCreateAlignedCoverAndLeanHelper nNode sAxes:#y)
		)
	)
	
	on btnCreateAlignedZ pressed do
	(--create min volume BB dummy helper - rotate on X only
		if $selection.count == 1 and classOf $ == editable_poly then
		(
			nNode = $
			
			(fnCreateAlignedCoverAndLeanHelper nNode sAxes:#z)
		)
	)
	
	on spnSizeX buttondown do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			p3CurrentSize = $.boxSize
		)
	)
	
	on spnSizeX changed fNewValue do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			$.boxSize.x = p3CurrentSize.x + fNewValue
		)
	)
	
	on spnSizeX buttonup bCancelled do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			if bCancelled then $.boxSize = p3CurrentSize
		)
		spnSizeX.value = 0.0
	)
	
	on spnSizeY buttondown do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			p3CurrentSize = $.boxSize
		)
	)
	
	on spnSizeY changed fNewValue do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			$.boxSize.y = p3CurrentSize.y + fNewValue
		)
	)
	
	on spnSizeY buttonup bCancelled do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			if bCancelled then $.boxSize = p3CurrentSize
		)
		spnSizeY.value = 0.0
	)
	
	on spnSizeZ buttondown do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			p3CurrentSize = $.boxSize
		)
	)
	
	on spnSizeZ changed fNewValue do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			$.boxSize.z = p3CurrentSize.z + fNewValue
		)
	)
	
	on spnSizeZ buttonup bCancelled do
	(--adjust dummy helper size
		if $selection.count == 1 and classOf $ == dummy then
		(
			if bCancelled then $.boxSize = p3CurrentSize
		)
		spnSizeZ.value = 0.0
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryCoverHelperTools" rltCryCoverHelperTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryCoverHelperTools")