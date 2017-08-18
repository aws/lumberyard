--CryENGINE phys proxy tools
--by Sascha Herfort
--
--a collection of tools for phys proxy creation
--requires max file directory to be export directory, export file per node and CGF filetype

rollout rltCryPhysProxyTools "Phys Proxy Tools" width:190 height:492
(
	--######################################################################################
	--###  GUI  ###############################################################################
	--######################################################################################
	group "Create Proxy for Selection"
	(
		editText edtName "Name:" text:(cryMaxTools.basic.ROMan.get "rltCryPhysProxyTools").sProxyNameDefault fieldWidth:128 offset:[5,0] align:#right
		editText edtUDP "UDPs:" text:(cryMaxTools.basic.ROMan.get "rltCryPhysProxyTools").sProxyUDPDefault fieldWidth:128 offset:[5,-5] align:#right
		spinner spnMatID "Material ID:" type:#integer range:[1,99,1] fieldWidth:24 offset:[5,-5] align:#right
		label lblProxyPerElement "Proxy per Element" offset:[64,-5] align:#right across:2
		checkBox chkProxyPerElement "" offset:[18,-5] align:#right checked:false
		
		button btnSphere "Sphere" offset:[1,3] width:160 height:16 align:#center enabled:false
		
		button btnAACapsule "Capsule" offset:[13,3] width:64 height:16 align:#center across:5 enabled:false
		button btnAlignedCapsule "Aligned" offset:[39,3] width:48 height:16 align:#center enabled:false
		button btnAlignedCapsuleX "X" offset:[41,3] width:16 height:16 align:#center enabled:false
		button btnAlignedCapsuleY "Y" offset:[27,3] width:16 height:16 align:#center enabled:false
		button btnAlignedCapsuleZ "Z" offset:[13,3] width:16 height:16 align:#center enabled:false
		
		button btnAABox "Box" offset:[13,3] width:64 height:16 align:#center across:5
		button btnAlignedBox "Aligned" offset:[39,3] width:48 height:16 align:#center
		button btnAlignedBoxX "X" offset:[41,3] width:16 height:16 align:#center
		button btnAlignedBoxY "Y" offset:[27,3] width:16 height:16 align:#center
		button btnAlignedBoxZ "Z" offset:[13,3] width:16 height:16 align:#center
		
		button btnAACylinder "Cylinder" offset:[13,3] width:64 height:16 align:#center across:5 enabled:false
		button btnAlignedCylinder "Aligned" offset:[39,3] width:48 height:16 align:#center enabled:false
		button btnAlignedCylinderX "X" offset:[41,3] width:16 height:16 align:#center enabled:false
		button btnAlignedCylinderY "Y" offset:[27,3] width:16 height:16 align:#center enabled:false
		button btnAlignedCylinderZ "Z" offset:[13,3] width:16 height:16 align:#center enabled:false
		
		button btnConvexHull "Convex Hull" offset:[1,3] width:160 height:16 align:#center enabled:false
		
		spinner spnBias "Size Bias" range:[0,10,1] fieldWidth:32 offset:[5,3] align:#right
		spinner spnMaxError "Error Threshold" range:[0,100,1] fieldWidth:32 offset:[5,-5] align:#right
		label lblRealtimePreview "Realtime Preview" offset:[64,-5] align:#right across:2
		checkBox chkRealtimePreview "" offset:[18,-5] align:#right checked:false
	)
	
	
	--######################################################################################
	--###  GLOBAL VARIABLES  #####################################################################
	--######################################################################################
	
	--TWEAKABLES MAGICIAN
	local iRefinementSearchSteps = 21 --step inbetween local minimum's meighbors to refine angle
	local iMaxRefinementIterations = 9 --max repeat refinement
	local bEarlyAbort = true --aborts BB search when error threshold reached
	local fMaxError = 1 --stop refining when two successive approximations are less than this close together (centimeters)
	local fBBDifferenceExtrapolationFactor = 0.1
	local sProxyNameDefault = "$physics_proxy"
	local sProxyUDPDefault = ""
	
	--DEBUG OPTIONS
	local bDebugPrint = false
	local bDebugDraw = false
	local bRealtimePreview = false
	
	--GLOBALS
	local fInitialSearchRange = 80.0 --should not be changed
	local iInitialSearchSteps = 9 --step along initial arc to find local minimum
	local qOrientation = quat 0 0 0 1
	local iSearchSteps = 0
	local iRefinementSteps = 0
	local modMatIDModifier = undefined
	local nPreviewBox = undefined
	
	
	--######################################################################################
	--###  FUNCTIONS  ##########################################################################
	--######################################################################################
	
	fn fnGetBBVolume aBB =
	(--returns the volume a BB - assumes array of min/max points of BB as input
		local p3Size = aBB[2] - aBB[1]
		
		p3Size.x*p3Size.y*p3Size.z
	)
	
	fn fnGetBBSizeDifference aBB1 aBB2 =
	(--returns the difference between the body diagonals of the supplied BBs
		local p3Size1 = aBB1[2] - aBB1[1]
		local p3Size2 = aBB2[2] - aBB2[1]
		
		if bDebugDraw then
		(
			local nTempPoint = point name:"cryAlignPhysPrimitiveBBDifference" size:50
			nTempPoint.wireColor = color 255 100 50
			nTempPoint.pos = p3Size1
			local nTempPoint = point name:"cryAlignPhysPrimitiveBBDifference" size:50
			nTempPoint.wireColor = color 50 255 100
			nTempPoint.pos = p3Size2
		)
		
		distance p3Size1 p3Size2
	)
	
	fn fnGetAABBFromVertices nNode aVertices =
	(--returns the axis-aligned bounding box of the supplied vertices
		local aXCoords = for each in aVertices collect nNode.verts[each].pos.x
		local aYCoords = for each in aVertices collect nNode.verts[each].pos.y
		local aZCoords = for each in aVertices collect nNode.verts[each].pos.z
		
		#([aMin aXCoords, aMin aYCoords, aMin aZCoords], [aMax aXCoords, aMax aYCoords, aMax aZCoords])
	)
	
	fn fnGetBBFromRotatedVertices nNode aVertices qOrientation =
	(--returns bounding box of the supplied vertices after applying supplied rotation
		local aXCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).x
		local aYCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).y
		local aZCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).z
		
		#([aMin aXCoords, aMin aYCoords, aMin aZCoords], [aMax aXCoords, aMax aYCoords, aMax aZCoords])
	)
	
	fn fnGetBBSizeFromRotatedVertices nNode aVertices qOrientation =
	(--returns the bounding box size of the supplied vertices after applying the supplied rotation
		local aXCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).x
		local aYCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).y
		local aZCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).z
		
		[aMax aXCoords, aMax aYCoords, aMax aZCoords] - [aMin aXCoords, aMin aYCoords, aMin aZCoords]
	)
	
	fn fnMinimumBBVolumeLinearSearch nNode aVertices qOrientation p3Axis fAngle1 fAngle2 iSteps bUpdatePreview:true bDebugPrint:bDebugPrint =
	(--performs a linear search for the minimum BB volume inbetween the supplied angles along the supplied axis with the supplied amount of steps - returns and array of two angles (neighbors of local minimum), unsorted
		if bDebugPrint then print "fnMinimumBBVolumeLinearSearch"
		
		iSearchSteps += iSteps
		
		--sort angles, so smallest comes first
		local aTempAngles = #(fAngle1, fAngle2)
		sort aTempAngles
		fAngle1 = aTempAngles[1]
		fAngle2 = aTempAngles[2]
		
		local aExtents = #()
		local aAnglesToTest = for i = 1 to iSteps collect (fAngle1 + (fAngle2 - fAngle1)*(i-1)/(iSteps - 1.0))
		if bDebugPrint then print ("aAnglesToTest: " + aAnglesToTest as string)
		for i = 1 to iSteps do
		(--step through arc from fAngle1 to fAngle2 and store area of face to minimize
			local fAngle = aAnglesToTest[i]
			local qOrientationToTest = qOrientation*(quat fAngle p3Axis) --apply initial rotation offset
			
			local p3BBSize = fnGetBBSizeFromRotatedVertices nNode aVertices qOrientationToTest
			append aExtents (p3BBSize.x*p3BBSize.y*p3BBSize.z)
			
			if bRealtimePreview and bUpdatePreview then
			(--debug draw
				with animate on
				(
					nPreviewBox.scale = p3BBSize
					local aBB = fnGetBBFromRotatedVertices nNode aVertices qOrientationToTest
					nPreviewBox.rotation = qOrientationToTest
					nPreviewBox.pos = (aBB[1] + (aBB[2] - aBB[1])*.5)*(inverse qOrientationToTest)
				)
				if animationRange.end == sliderTime then
				(
					animationRange = interval animationRange.start (sliderTime+1)
				)
				sliderTime += 1
			)
		)
		
		if bDebugDraw then
		(--debug draw
			local aAABB = fnGetAABBFromVertices nNode aVertices
			local fScale = 1000000/((aAABB[2].x-aAABB[1].x)*(aAABB[2].y-aAABB[1].y)*(aAABB[2].z-aAABB[1].z))
			local p3Centroid = aAABB[2]
			p3Centroid += [0,0,10]
			local fOffset = 50*(sqrt ((if qOrientation.axis.x != 0 then 1 else 0) + (if qOrientation.axis.y != 0 then 1 else 0) + (if qOrientation.axis.z != 0 then 1 else 0)))
			for i = 1 to iSteps do point pos:(p3Centroid+[aAnglesToTest[i],fOffset,fScale*aExtents[i]/100000]) size:((aAnglesToTest[2] - aAnglesToTest[1])*2) name:"cryAlignPhysPrimitiveSearchSamples" wireColor:(color (i*255.0/iSteps) (i*255.0/iSteps*.5 + fOffset*1.4) (i*255.0/iSteps*.2 + fOffset*2.3)) box:true
		)
		
		--find smallest extent and neighbors
		local iMinExtent = (findItem aExtents (aMin aExtents))
		local aNeighbors = #(if iMinExtent == 1 then iSteps else (iMinExtent-1), if iMinExtent == iSteps then 1 else (iMinExtent+1))
		
		if bDebugPrint then
		(
			print ("aExtents: " + aExtents as string)
			print ("Smallest extent: #" + iMinExtent as string + "=" + (aExtents[iMinExtent]/1000000) as string + "m³")
			print ("Neighbor 1: #" + aNeighbors[1] as string + "=" + (aExtents[aNeighbors[1]]/1000000) as string + "m³")
			print ("Neighbor 2: #" + aNeighbors[2] as string + "=" + (aExtents[aNeighbors[2]]/1000000) as string + "m³")
		)
		
		--find start/end angle to refine search in between
		--local aAngles = #(aAnglesToTest[findItem aExtents (aMin #(aExtents[aNeighbors[1]],aExtents[aNeighbors[2]]))]) --add neighbor with smallest BB size first
		--append aAngles aAnglesToTest[findItem aExtents (aMax #(aExtents[aNeighbors[1]],aExtents[aNeighbors[2]]))] --add neighbor with biggest BB size second
		local fStepSize = (fAngle2 - fAngle1)/(iSteps - 1.0)
		local aAngles = #(if iMinExtent == 1 then aAnglesToTest[1] - fStepSize else aAnglesToTest[aNeighbors[1]], if iMinExtent == iSteps then aAnglesToTest[iSteps] + fStepSize else aAnglesToTest[aNeighbors[2]])
		
		if bDebugPrint then print ("aAngles: " + aAngles as string)
		
		aAngles
	)
	
	fn fnOptimizeLeastOptimalBBAxis nNode aVertices qOrientation fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{1..3} =
	(--linear searches all axes for the minimum BB volume and optimizes the axis which deviates the most from its optimum - returns an array containing a quaternion representing the orientation of the BB after optimization and the index of the axis that was least optimal
		if bDebugPrint then print ("fnOptimizeLeastOptimalBBAxis " + aAxesToOptimize as string)
		
		local aAxes = #([1,0,0],[0,1,0],[0,0,1])
		local aInitialBB = fnGetBBFromRotatedVertices nNode aVertices qOrientation
		local fInitialBBVolume = fnGetBBVolume aInitialBB
		
		local aOptimalAngles = #(#(0,fInitialSearchRange),#(0,fInitialSearchRange),#(0,fInitialSearchRange)) --stores angles inbetween which to search for optimum
		local aVolumeDeviances = #(0.0,0.0,0.0) --stores volume deviance after optimization
		local aLastStepBBs = #(aInitialBB,aInitialBB,aInitialBB) --stores BBs from last iteration - used to abort when error threshold reached
		--local fLastStepDifference = (pow (1/fBBDifferenceExtrapolationFactor) (iMaxRefinementIterations - 1))*fMaxError --initial value in case first difference is zero - must not go below fMaxError before iMaxRefinementIterations iterations
		local aAABB = fnGetAABBFromVertices nNode aVertices
		local fLastStepDifference = (length (aAABB[2] - aAABB[1]))*(pow 3 0.5)*.5 --initial value cheated - hope that max possible BB is never longer than body diagonal of actual BB
		local aLastStepBBDifferences = #(fLastStepDifference,fLastStepDifference,fLastStepDifference) --stores BBDifference from last iteration - used to extrapolate on zero error
		
		local iRefinementStep = 0
		while aVolumeDeviances[1] == 0 and aVolumeDeviances[2] == 0 and aVolumeDeviances[3] == 0 and iRefinementStep <= iMaxRefinementIterations do
		(--Step 1: find least optimal axis by volume deviance to optimum - refine until nonzero deviance found or error threshold reached
			if bDebugPrint then print ("iRefinementStep: " + iRefinementStep as string)
			
			local iSearchSteps = if iRefinementStep == 0 then iInitialSearchSteps else iRefinementSearchSteps
				
			for i = 1 to 3 where aAxesToOptimize[i] do
			(--test all 3 axes - omit axes that are have reached error threshold
				aMinimumNeighbors = fnMinimumBBVolumeLinearSearch nNode aVertices qOrientation aAxes[i] aOptimalAngles[i][1] aOptimalAngles[i][2] iSearchSteps bUpdatePreview:false bDebugPrint:false
				aOptimalAngles[i] = aMinimumNeighbors
				local qCurrentOrientation = qOrientation*(quat ((aMinimumNeighbors[1] + aMinimumNeighbors[2])*.5) aAxes[i])
				local aBB = fnGetBBFromRotatedVertices nNode aVertices qCurrentOrientation
				aVolumeDeviances[i] = fInitialBBVolume - (fnGetBBVolume aBB)
				
				if bEarlyAbort then
				(--abort if max error threshold reached
					local fBBDifference = fnGetBBSizeDifference aBB aLastStepBBs[i]
					aLastStepBBs[i] = aBB
					
					if fBBDifference == 0 then
					(--extrapolate on zero values
						fBBDifference = aLastStepBBDifferences[i]*fBBDifferenceExtrapolationFactor
						if bDebugPrint then print ("BB Difference: " + fBBDifference as string + "cm extrapolated")
					)
					else if bDebugPrint then print ("BB Difference: " + fBBDifference as string + "cm")
					
					aLastStepBBDifferences[i] = fBBDifference
					
					aAxesToOptimize[i] = fBBDifference > fMaxError --axis needs optimization until precision threshold reached
					if bDebugPrint and not aAxesToOptimize[i] then print ("Omitting axis: " + i as string + " at iteration " + iRefinementStep as string)
				)
			)
			
			iRefinementStep += 1
			
			if bDebugPrint then print ("aVolumeDeviances: " + aVolumeDeviances as string)
		)
		if bDebugPrint then print ("aOptimalAngles: " + aOptimalAngles as string)
		if bDebugPrint then print ("aVolumeDeviances: " + aVolumeDeviances as string)
		if bDebugPrint then print ("iRefinementStep: " + iRefinementStep as string)
		
		if iRefinementStep <= iMaxRefinementIterations then
		(--refine if steps left
			local iLeastOptimalAxis = findItem aVolumeDeviances (aMax aVolumeDeviances)
			if bDebugPrint then print ("iLeastOptimalAxis: " + iLeastOptimalAxis as string)
			
			local aMinimumNeighbors = aOptimalAngles[iLeastOptimalAxis]
			local fAngle = (aMinimumNeighbors[1] + aMinimumNeighbors[2])*.5
			local p3Axis = aAxes[iLeastOptimalAxis]
			local aBB = fnGetBBFromRotatedVertices nNode aVertices (qOrientation*(quat fAngle p3Axis))
			local fLastStepBBDifference = aLastStepBBDifferences[iLeastOptimalAxis]
			local bAxisNeedsOptimization = true
			for i = iRefinementStep to iMaxRefinementIterations where bAxisNeedsOptimization do
			(--Step 2: refine least optimal axis
				aMinimumNeighbors = fnMinimumBBVolumeLinearSearch nNode aVertices qOrientation p3Axis aMinimumNeighbors[1] aMinimumNeighbors[2] iRefinementSearchSteps bDebugPrint:false
				fAngle = (aMinimumNeighbors[1] + aMinimumNeighbors[2])*.5
				
				if bEarlyAbort then
				(--abort if max error threshold reached
					local aNewBB = fnGetBBFromRotatedVertices nNode aVertices (qOrientation*(quat fAngle p3Axis))
					local fBBDifference = fnGetBBSizeDifference aBB aNewBB
					aBB = aNewBB
					
					if fBBDifference == 0 then
					(--extrapolate on zero values
						fBBDifference = fLastStepBBDifference*fBBDifferenceExtrapolationFactor
						if bDebugPrint then print ("BB Difference: " + fBBDifference as string + "cm extrapolated")
					)
					else if bDebugPrint then print ("BB Difference: " + fBBDifference as string + "cm")
					
					fLastStepBBDifference = fBBDifference
					
					bAxisNeedsOptimization = fBBDifference > fMaxError --axis needs optimization until precision threshold reached
					aAxesToOptimize[iLeastOptimalAxis] = bAxisNeedsOptimization
					if bDebugPrint and not bAxisNeedsOptimization then print ("Omitting axis: " + iLeastOptimalAxis as string + " at iteration " + i as string)
				)
			)
			
			if bDebugPrint then print ("aAxesToOptimize: " + aAxesToOptimize as string)
			
			#(qOrientation*(quat fAngle p3Axis), iLeastOptimalAxis)
		)
		else
		(--no optimization occured
			#(qOrientation, 0)
		)
	)
	
	fn fnGetMinimumVolumeBBOrientation nNode aVertices =
	(--returns the orientation of the minimum volume BB of a selection of vertices
		local iAxisIterations = 4
		
		iSearchSteps = 0
		
		local qOrientation = quat 0 0 0 1
		local iOptimizedAxis = 4
		
		--local aLastBBVolume = fnGetBBVolume (fnGetAABBFromVertices nNode aVertices)
		
		for i = 1 to iAxisIterations where iOptimizedAxis != 0 do
		(--keep optimizing until no axis can be optimized any further
			if bDebugPrint then
			(
				print ("-----=====#####   STEP " + i as string + "   #####=====-----")
				print ("-----=====#####   STEP " + i as string + "   #####=====-----")
				print ("-----=====#####   STEP " + i as string + "   #####=====-----")
			)
			
			aOptimization = fnOptimizeLeastOptimalBBAxis nNode aVertices qOrientation fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:(#{1..3} - #{iOptimizedAxis})
			qOrientation = aOptimization[1]
			iOptimizedAxis = aOptimization[2]
			
			if bDebugPrint then
			(
				print ("Step " + i as string + " qOrientation: " + qOrientation.angle as string + " " + qOrientation.axis as string)
				print ("Step " + i as string + " iOptimizedAxis: " + iOptimizedAxis as string)
			)
			
			/*
			--debug bb volume difference
			local fVolume = fnGetBBVolume (fnGetBBFromRotatedVertices nNode aVertices qOrientation)
			if i > 4 then
			(
				print ("Step " + i as string + " BB volume difference: " + ((aLastBBVolume - fVolume)/1000000.0) as string)
				(point pos:(fnGetAABBFromVertices nNode aVertices)[1])
			)
			
			aLastBBVolume = fVolume
			*/
		)
		
		if bDebugPrint then print ("iSearchSteps: " + iSearchSteps as string)
		
		qOrientation
	)
	
	fn fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName mMaterial iMatID sUDPs:"" =
	(--creates a cryEngine physics primitive box - returns the new scene node
		local nMinBB = box length:(aBB[2].y - aBB[1].y + fBias) width:(aBB[2].x - aBB[1].x + fBias) height:(aBB[2].z - aBB[1].z + fBias)
		CenterObject nMinBB
		nMinBB.pos = (aBB[1] + (aBB[2] - aBB[1])*.5)*(inverse qOrientation)
		rotate nMinBB qOrientation
		
		nMinBB.mapcoords = true --if this off, no UVs will be generated and some versions of our RC will silently dismiss the mesh and not export it!!!
		nMinBB.name = uniqueName sName --set name
		nMinBB.material = mMaterial --set material
		cryMaxTools.export.fnSetUPDFlag nMinBB "box" true --set box flag
		
		if sUDPs != "" then
		(--set custom UDP flags
			local aFlags = filterString sUDPs " "
			for each in aFlags do
			(
				cryMaxTools.export.fnSetUPDFlag nMinBB each true
			)
		)
		
		addModifier nMinBB (smoothModifier smoothingBits:1) --add smoothing group modifier
		
		if modMatIDModifier == undefined or modMatIDModifier.materialID != iMatID then
		(--if no matching material ID modifier present, create it
			modMatIDModifier = Materialmodifier materialID:iMatID name:"physMatID"
		)
		addModifier nMinBB modMatIDModifier
		
		gc light:true
		
		nMinBB
	)
	
	fn fnCreateAlignedCryPhysBoxPrimitive nNode fBias sName iMatID bPerElement:false sUDPs:"" =
	(--creates a minimum volume cryEngine physics primitive box from the selected subObjects of the node, applies the appropriate material and links the primitive to the node
		if classOf nNode == Editable_Poly then
		(
			undo off --"Create Aligned Box Primitive" on
			(
				undo off
				(--clear debug draw:
					delete (getNodeByName "cryAlignPhysPrimitiveSearchSamples" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveRotation" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveSteps" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveBBDifference" all:true)
				)
				
				nPreviewBox = undefined
				
				if bRealtimePreview then
				(--debug draw
					undo off
					(
						sliderTime = 0
						
						nPreviewBox = box length:1 width:1 height:1
						nPreviewBox.name = "cryAlignPhysPrimitiveSteps"
						nPreviewBox.xray = true
						CenterObject nPreviewBox
					)
				)
				
				local aVertices = case subObjectLevel of
				(--get vertices from selected subobjects
					1: (polyOp.getVertsByFlag nNode 1)
					2: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					3: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					4: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					5: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					default: (nNode.verts as bitArray)
				)
				
				if bPerElement then
				(--create proxy per element
					local aFaces = nNode.faces as bitArray
					local aElements = #()
					while aFaces.numberSet != 0 do
					(--get element of faces and remove from remaining faces list
						local aNewElement = polyOp.getElementsUsingFace nNode (aFaces as array)[1]
						append aElements aNewElement
						aFaces -= aNewElement
					)
					
					local aVertCollections = #()
					for each in aElements do
					(--find vertex collections that are selected
						local aCurrentVertCollection = (polyOp.getVertsUsingFace nNode each)*aVertices
						if aCurrentVertCollection.numberSet > 1 then
						(--skip collections with 1 vert - would create infinitely small BB
							append aVertCollections aCurrentVertCollection
						)
					)
					
					for each in aVertCollections do
					(
						local qOrientation = fnGetMinimumVolumeBBOrientation nNode each
						
						local aBB = fnGetBBFromRotatedVertices nNode each qOrientation
						local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName nNode.material iMatID sUDPs:sUDPs
						
						nPhysPrimitive.parent = nNode
						
						if not bRealtimePreview then
						(--force redraw to reveal new box
							forceCompleteRedraw()
						)
					)
				)
				else
				(--create single proxy
					local qOrientation = fnGetMinimumVolumeBBOrientation nNode aVertices
					
					local aBB = fnGetBBFromRotatedVertices nNode aVertices qOrientation
					local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName nNode.material iMatID sUDPs:sUDPs
					
					nPhysPrimitive.parent = nNode
				)
				
				if bRealtimePreview then
				(--debug draw
					undo off
					(
						delete nPreviewBox
					)
				)
			)
		)
	)
	
	fn fnCreateAxisAlignedCryPhysBoxPrimitive nNode fBias sName iMatID bPerElement:false sUDPs:"" =
	(--creates an axis aligned cryEngine physics primitive box from the selected subObjects of the node, applies the appropriate material and links the primitive to the node
		if classOf nNode == Editable_Poly then
		(
			undo off --"Create Aligned Box Primitive" on
			(
				undo off
				(--clear debug draw:
					delete (getNodeByName "cryAlignPhysPrimitiveSearchSamples" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveRotation" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveSteps" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveBBDifference" all:true)
				)
				
				nPreviewBox = undefined
				
				local aVertices = case subObjectLevel of
				(--get vertices from selected subobjects
					1: (polyOp.getVertsByFlag nNode 1)
					2: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					3: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					4: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					5: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					default: (nNode.verts as bitArray)
				)
				
				if bPerElement then
				(--create proxy per element
					local aFaces = nNode.faces as bitArray
					local aElements = #()
					while aFaces.numberSet != 0 do
					(--get element of faces and remove from remaining faces list
						local aNewElement = polyOp.getElementsUsingFace nNode (aFaces as array)[1]
						append aElements aNewElement
						aFaces -= aNewElement
					)
					
					local aVertCollections = #()
					for each in aElements do
					(--find vertex collections that are selected
						local aCurrentVertCollection = (polyOp.getVertsUsingFace nNode each)*aVertices
						if aCurrentVertCollection.numberSet > 1 then
						(--skip collections with 1 vert - would create infinitely small BB
							append aVertCollections aCurrentVertCollection
						)
					)
					
					for each in aVertCollections do
					(
						local aBB = fnGetAABBFromVertices nNode each
						local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB (quat 0 0 0 1) fBias sName nNode.material iMatID sUDPs:sUDPs
						
						nPhysPrimitive.parent = nNode
						
						if not bRealtimePreview then
						(--force redraw to reveal new box
							forceCompleteRedraw()
						)
					)
				)
				else
				(--create single proxy
					local aBB = fnGetAABBFromVertices nNode aVertices
					local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB (quat 0 0 0 1) fBias sName nNode.material iMatID sUDPs:sUDPs
					
					nPhysPrimitive.parent = nNode
				)
			)
		)
	)
	
	fn fnCreateSemiAlignedCryPhysBoxPrimitive nNode fBias sName iMatID sAxis bPerElement:false sUDPs:"" =
	(--creates a minimum volume cryEngine physics primitive box from the selected subObjects of the node while only rotating around a single axis, applies the appropriate material and links the primitive to the node
		if classOf nNode == Editable_Poly then
		(
			undo off --"Create Aligned Box Primitive" on
			(
				undo off
				(--clear debug draw:
					delete (getNodeByName "cryAlignPhysPrimitiveSearchSamples" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveRotation" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveSteps" all:true)
					delete (getNodeByName "cryAlignPhysPrimitiveBBDifference" all:true)
				)
				
				nPreviewBox = undefined
				
				if bRealtimePreview then
				(--debug draw
					undo off
					(
						sliderTime = 0
						
						nPreviewBox = box length:1 width:1 height:1
						nPreviewBox.name = "cryAlignPhysPrimitiveSteps"
						nPreviewBox.xray = true
						CenterObject nPreviewBox
					)
				)
				
				local iAxisToOptimize = if sAxis == #X then 1 else if sAxis == #Y then 2 else if sAxis == #Z then 3
				local aVertices = case subObjectLevel of
				(--get vertices from selected subobjects
					1: (polyOp.getVertsByFlag nNode 1)
					2: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					3: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
					4: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					5: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
					default: (nNode.verts as bitArray)
				)
				
				if bPerElement then
				(--create proxy per element
					local aFaces = nNode.faces as bitArray
					local aElements = #()
					while aFaces.numberSet != 0 do
					(--get element of faces and remove from remaining faces list
						local aNewElement = polyOp.getElementsUsingFace nNode (aFaces as array)[1]
						append aElements aNewElement
						aFaces -= aNewElement
					)
					
					local aVertCollections = #()
					for each in aElements do
					(--find vertex collections that are selected
						local aCurrentVertCollection = (polyOp.getVertsUsingFace nNode each)*aVertices
						if aCurrentVertCollection.numberSet > 1 then
						(--skip collections with 1 vert - would create infinitely small BB
							append aVertCollections aCurrentVertCollection
						)
					)
					
					for each in aVertCollections do
					(
						print ("iAxisToOptimize: " + iAxisToOptimize as string)
						local qOrientation = (fnOptimizeLeastOptimalBBAxis nNode each (quat 0 0 0 1) fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{iAxisToOptimize})[1]
						
						local aBB = fnGetBBFromRotatedVertices nNode each qOrientation
						local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName nNode.material iMatID sUDPs:sUDPs
						
						nPhysPrimitive.parent = nNode
						
						if not bRealtimePreview then
						(--force redraw to reveal new box
							forceCompleteRedraw()
						)
					)
				)
				else
				(--create single proxy
					local qOrientation = (fnOptimizeLeastOptimalBBAxis nNode aVertices (quat 0 0 0 1) fInitialSearchRange iInitialSearchSteps iRefinementSearchSteps iMaxRefinementIterations aAxesToOptimize:#{iAxisToOptimize})[1]
					
					local aBB = fnGetBBFromRotatedVertices nNode aVertices qOrientation
					local nPhysPrimitive = fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName nNode.material iMatID sUDPs:sUDPs
					
					nPhysPrimitive.parent = nNode
				)
				
				if bRealtimePreview then
				(--debug draw
					undo off
					(
						delete nPreviewBox
					)
				)
			)
		)
	)
	
	--######################################################################################
	--###  MAGIC  ######################################e#######################################
	--######################################################################################
	
	on edtName entered sNewName do
	(--catch empty names
		if sNewName == "" then
		(
			edtName.text = sProxyNameDefault
		)
	)
	
	on btnAABox pressed do
	(--create axis aligned box proxy
		local iOldSubObjectLevel = subObjectLevel
		
		if $selection.count > 0 then
		(
			for each in $selection do
			(
				local nNode = each
				
				fnCreateAxisAlignedCryPhysBoxPrimitive nNode spnBias.value edtName.text spnMatID.value bPerElement:chkProxyPerElement.state sUDPs:(edtUDP.text)
			)
		)
		
		if iOldSubObjectLevel != undefined then
		(
			subObjectLevel = iOldSubObjectLevel
		)
	)
	
	on btnAlignedBox pressed do
	(--create minimum volume box proxy
		local iOldSubObjectLevel = subObjectLevel
		
		if $selection.count > 0 then
		(
			for each in $selection do
			(
				local nNode = each
				
				fnCreateAlignedCryPhysBoxPrimitive nNode spnBias.value edtName.text spnMatID.value bPerElement:chkProxyPerElement.state sUDPs:(edtUDP.text)
			)
		)
		
		if iOldSubObjectLevel != undefined then
		(
			subObjectLevel = iOldSubObjectLevel
		)
	)
	
	on btnAlignedBoxX pressed do
	(--create minimum volume box proxy, allow only rotation round X axis
		local iOldSubObjectLevel = subObjectLevel
		
		if $selection.count > 0 then
		(
			for each in $selection do
			(
				local nNode = each
				
				fnCreateSemiAlignedCryPhysBoxPrimitive nNode spnBias.value edtName.text spnMatID.value #X bPerElement:chkProxyPerElement.state sUDPs:(edtUDP.text)
			)
		)
		
		if iOldSubObjectLevel != undefined then
		(
			subObjectLevel = iOldSubObjectLevel
		)
	)
	
	on btnAlignedBoxY pressed do
	(--create minimum volume box proxy, allow only rotation round Y axis
		local iOldSubObjectLevel = subObjectLevel
		
		if $selection.count > 0 then
		(
			for each in $selection do
			(
				local nNode = each
				
				fnCreateSemiAlignedCryPhysBoxPrimitive nNode spnBias.value edtName.text spnMatID.value #Y bPerElement:chkProxyPerElement.state sUDPs:(edtUDP.text)
			)
		)
		
		if iOldSubObjectLevel != undefined then
		(
			subObjectLevel = iOldSubObjectLevel
		)
	)
	
		on btnAlignedBoxZ pressed do
	(--create minimum volume box proxy, allow only rotation round Z axis
		local iOldSubObjectLevel = subObjectLevel
		
		if $selection.count > 0 then
		(
			for each in $selection do
			(
				local nNode = each
				
				fnCreateSemiAlignedCryPhysBoxPrimitive nNode spnBias.value edtName.text spnMatID.value #Z bPerElement:chkProxyPerElement.state sUDPs:(edtUDP.text)
			)
		)
		
		if iOldSubObjectLevel != undefined then
		(
			subObjectLevel = iOldSubObjectLevel
		)
	)
	
	on chkRealtimePreview changed state do
	(
		bRealtimePreview = state
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryPhysProxyTools" rltCryPhysProxyTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryPhysProxyTools")