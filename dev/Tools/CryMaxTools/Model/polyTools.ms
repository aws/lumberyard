---------------------------
-- CryMaxTools v2.0
-- Poly Tools v1.0
-- by Sascha Herfort
---------------------------

rollout rltCryPolyTools "Poly Tools" width:190
(
	group "Diagnostics Tools"
	(
		button btnMarkDegenerateTris "Mark Degenerate Triangles" width:160 height:16 align:#center offset:[1,0] tooltip:"Places point helpers at triangles with zero face area."
		button btnDeleteDegenerateTris "Clone without Degenerate Tris" width:160 height:16 align:#center offset:[1,-5] tooltip:"Clones all selected objects and deletes all degenerate triangles in the clones."
		button btnSelectNonQuads "Non-Quad" width:80 height:16 across:2 align:#center offset:[-1,3] tooltip:"Selects faces with more/less than 4 vertices. CTRL adds to selection."
		button btnSelectStars "Stars" width:80 height:16 align:#center offset:[4,3] tooltip:"Select vertices with more/less than 4 edges. CTRL adds to selection."
	)
	
	group "Material ID Tools"
	(
		checkbox chkAutoUpdate "Update (slow!)" align:#left offset:[-4,-2] checked:false
		checkbutton btnMatID1 "1" height:16 width:16 across:10 align:#right offset:[-4,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID2 "2" height:16 width:16 align:#right offset:[-3,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID3 "3" height:16 width:16 align:#right offset:[-2,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID4 "4" height:16 width:16 align:#right offset:[-1,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID5 "5" height:16 width:16 align:#right offset:[0,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID6 "6" height:16 width:16 align:#right offset:[1,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID7 "7" height:16 width:16 align:#right offset:[2,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID8 "8" height:16 width:16 align:#right offset:[3,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID9 "9" height:16 width:16 align:#right offset:[4,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID10 "10" height:16 width:16 align:#right offset:[5,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID11 "11" height:16 width:16 across:10 align:#right offset:[-4,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID12 "12" height:16 width:16 align:#right offset:[-3,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID13 "13" height:16 width:16 align:#right offset:[-2,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID14 "14" height:16 width:16 align:#right offset:[-1,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID15 "15" height:16 width:16 align:#right offset:[0,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID16 "16" height:16 width:16 align:#right offset:[1,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID17 "17" height:16 width:16 align:#right offset:[2,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID18 "18" height:16 width:16 align:#right offset:[3,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID19 "19" height:16 width:16 align:#right offset:[4,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
		checkbutton btnMatID20 "20" height:16 width:16 align:#right offset:[5,-5] toolTip:"Click sets. SHIFT+Click selects. CTRL+Click adds. ALT+Click removes. CTRL+ALT+Click intersects."
	)
	
	group "Selection Tools"
	(
		button btnSelectLoop "Loop Selection" width:80 height:16 align:#center across:2 offset:[-1,-2] tooltip:"Turn Selection to limited/continuous Loop."
		button btnSelectRing "Edge Ring" width:80 height:16 align:#center  offset:[4,-2] tooltip:"Turn Selection to limited/continuous Ring."
		checkButton ckbtnMultiLoopRing "Multi Loop/Ring" width:160 height:16 align:#center offset:[1,-5] tooltip:"Loop/Ring Mode. CTRL adds to selection. ALT removes from selection."
	)
	
	group "Snap to Surface"
	(
		pickbutton pbtnSurfacePick "Pick Surface..." width:100 height:16 align:#center across:2 offset:[9,-2] tooltip:"Pick a reference surface."
		button btnSurfaceSnapshot "Snapshot" width:60 height:16 align:#center across:2 offset:[14,-2] tooltip:"Use snapshot of selection as reference."
		button btnSnapToSurface "Snap Verts to Surface" width:112 height:16 across:2 align:#center offset:[15,-5] tooltip:"Snap vertices to selected surface." enabled:false
		checkBox chkSnapToSurfaceAuto "Auto" width:40 height:16 align:#center offset:[17,-5] enabled:false
		button btnSnapToOriginalPosition "Snap back to original Position" width:160 height:16 align:#center offset:[1,-5] tooltip:"Snap verts back to their original position. Snapshot mode only." enabled:false
	)
	
	group "Topology Tools"
	(
		button btnRemoveEdgeLoop "Remove Edge Loop" width:160 height:16 align:#center offset:[1,-5] tooltip:"Remove Loops of selected Edges from Poly."
		spinner spnMoveEdgeLoop "Move Edge Loop: " width:160 height:16 align:#center offset:[1,-5] tooltip:"Select an edge and drag spinner to move loop proportionally."
	)

---------------------	
--global variables--
---------------------
	global iCryPolyOldFaceSelectionCount = 0 --used for face selection callback -- requires globals
	global bCryPolyDoUpdateFaceSelection = true --used to avoid change handling on matID button pressed
	global mpiSurface = undefined --used for move along surface
	local sSurfaceSnapshot = undefined --used for move along surface
	global sSurfaceMoveNode = undefined --used for move along surface
	
----------------------
--global functions--
----------------------
--shared functions
	fn fnSelectFacesByMatID sPoly iIndex bClearSelection:true =
	(--helper function - works with edit_poly and editable poly
		if classOf sPoly == Edit_Poly then
		(
			sPoly.selectByMaterialClear = bClearSelection
			sPoly.selectByMaterialID = iIndex-1
			sPoly.buttonOp #SelectByMaterial
		)
		else
		(
			sPoly.selectByMaterial iIndex clearCurrentSelection:bClearSelection
		)
	)
	
--general functions
	fn fnPlaceHelperOnDegenerateTriangles =
	(--places a point helper on each degenerate triangle
		local fThreshold = 0.001 --face area threshold - RC uses 0.0
		
		local lHelperLayer = layerManager.getLayerFromName "degenerateTriangleHelpers"
		if lHelperLayer == undefined then lHelperLayer = layerManager.newLayerFromName "degenerateTriangleHelpers"
			
		local aObjectSet = for each in $selection where classOf each == editable_poly or classOf each == polyMeshObject or classOf each == Editable_mesh collect each
		
		for nMesh in aObjectSet do
		(
			local nTriangulatedCopy = snapshot nMesh
			convertTo nTriangulatedCopy Editable_Poly
			nTriangulatedCopy.selectedVerts = nTriangulatedCopy.verts --select all vertices
			nTriangulatedCopy.connectVertices()
			
			local aDegenerateFaces = #{}
			
			for each in nTriangulatedCopy.faces do
			(
				if polyOp.getFaceArea nTriangulatedCopy each.index <= fThreshold then
				(
					append aDegenerateFaces each.index
				)
			)
			
			--print aDegenerateFaces
			for iFace in aDegenerateFaces do
			(--place point helpers on degenerate triangles
				local aFaceVertPositions = for iVert in (polyOp.getFaceVerts nTriangulatedCopy iFace) collect polyOp.getVert nTriangulatedCopy iVert
				local fSize = (aMax #(distance aFaceVertPositions[1] aFaceVertPositions[2], distance aFaceVertPositions[2] aFaceVertPositions[3], distance aFaceVertPositions[3] aFaceVertPositions[1]))*2
				local nHelper = point pos:(polyOp.getFaceCenter nTriangulatedCopy iFace) size:fSize wireColor:[50,255,100] parent:nMesh name:(uniqueName "degenerateTriangleHere") centerMarker:true box:true axisTripod:false
				lHelperLayer.addNode nHelper
			)
			
			local aVertsOnDegradedFaces = polyop.getVertsUsingFace nTriangulatedCopy aDegenerateFaces
			delete nTriangulatedCopy
			
			/*
			if aVertsOnDegradedFaces.numberSet > 0 then
			(
				modVertSelectionEditPoly = edit_poly()
				modVertSelectionEditPoly.name = "degenerate_face_verts"
				addModifier nMesh modVertSelectionEditPoly
				modVertSelectionEditPoly.setSelection #vertex #{} --clear selection - sometimes necessary
				modVertSelectionEditPoly.select #vertex aVertsOnDegradedFaces
			)
			*/
		)
		
		print "Point helpers have been placed at degenerate triangles."
	)
	
	fn fnCloneObjectsAndDeleteDegenerateTriangles =
	(--creates a triangulated copy of each selected object and removes the degenerate triangles in the clones
		local fThreshold = 0.001 --face area threshold - RC uses 0.0
		
		local lHelperLayer = layerManager.getLayerFromName "degenerateTriangleHelpers"
		if lHelperLayer == undefined then lHelperLayer = layerManager.newLayerFromName "degenerateTriangleHelpers"
			
		local aObjectSet = for each in $selection where classOf each == editable_poly or classOf each == polyMeshObject or classOf each == Editable_mesh collect each
		local aNewObjectsSet = #()
		
		for nMesh in aObjectSet do
		(
			local nTriangulatedCopy = snapshot nMesh
			nTriangulatedCopy.parent = nMesh.parent
			nTriangulatedCopy.name = nMesh.name + "_removedDegenerateTriangles"
			convertTo nTriangulatedCopy Editable_Poly
			nTriangulatedCopy.selectedVerts = nTriangulatedCopy.verts --select all vertices
			nTriangulatedCopy.connectVertices()
			
			local aDegenerateFaces = #{}
			
			for each in nTriangulatedCopy.faces do
			(
				if polyOp.getFaceArea nTriangulatedCopy each.index <= fThreshold then
				(
					append aDegenerateFaces each.index
				)
			)
			
			polyop.deleteFaces nTriangulatedCopy aDegenerateFaces
			append aNewObjectsSet nTriangulatedCopy
		)
		
		select aNewObjectsSet
		
		print "Selected Objects have been cloned and degenerate triangles from clones been removed."
	)
	
	fn fnGetMatButtonHighlights =
	(--returns a bitArray containing the states of the matID buttons
		local aIDs = #{}
		local aButtonList = #(btnMatID1,btnMatID2,btnMatID3,btnMatID4,btnMatID5,btnMatID6,btnMatID7,btnMatID8,btnMatID9,btnMatID10,btnMatID11,btnMatID12,btnMatID13,btnMatID14,btnMatID15,btnMatID16,btnMatID17,btnMatID18,btnMatID19,btnMatID20)
		for i = 1 to aButtonList.count do
		(
			aIDs[i] = aButtonList[i].state
		)
		aIDs
	)
	
	fn fnSetMatButtonHighlights aIDs =
	(--sets the states of the matID buttons
		local aButtonList = #(btnMatID1,btnMatID2,btnMatID3,btnMatID4,btnMatID5,btnMatID6,btnMatID7,btnMatID8,btnMatID9,btnMatID10,btnMatID11,btnMatID12,btnMatID13,btnMatID14,btnMatID15,btnMatID16,btnMatID17,btnMatID18,btnMatID19,btnMatID20)
		for i = 1 to aButtonList.count do
		(
			aButtonList[i].state = aIDs[i]
		)
	)
	
	fn fnMatID iIndex =
	(--handles matID button hits
		local sPoly = undefined
		if (sPoly = cryMaxTools.model.poly.fnGetSelectedPolyMod()) != false then
		(
			if keyboard.shiftPressed then
			(--shift+click selects ID
				undo "Select MaterialID" on
				(
					bCryPolyDoUpdateFaceSelection = false --avoid change handler
					
					subObjectLevel = 4
					fnSelectFacesByMatID sPoly iIndex
					
					if (sPoly.getSelection #Face).numberSet > 0 then
					(
						fnSetMatButtonHighlights #{iIndex}
					)
					else
					(
						fnSetMatButtonHighlights #{}
					)
					bCryPolyDoUpdateFaceSelection = true
					iCryPolyOldFaceSelectionCount = sPoly.getSelection #Face
				)
			)
			else if keyboard.controlPressed or keyboard.altPressed then
			(--ctrl/alt handlers
				undo "Select MaterialID" on
				(
					bCryPolyDoUpdateFaceSelection = false --avoid change handler
					
					subObjectLevel = 4
					if keyboard.controlPressed and (keyboard.altPressed == false) then
					(--ctrl+click adds to selection
						local aOldFaceSelectionCount = (sPoly.getSelection #Face).numberSet
						fnSelectFacesByMatID sPoly iIndex bClearSelection:false
						local aNewFaceSelectionCount = (sPoly.getSelection #Face).numberSet
						
						aOldMatButtonHighlights = fnGetMatButtonHighlights()
						if aOldFaceSelectionCount != aNewFaceSelectionCount then
						(
							fnSetMatButtonHighlights (aOldMatButtonHighlights + #{iIndex})
						)
						else
						(
							fnSetMatButtonHighlights (aOldMatButtonHighlights - #{iIndex})
						)
					)
					else if keyboard.altPressed and (keyboard.controlPressed == false) then
					(--alt+click removes from selection
						local aOldFaceSelection = sPoly.getSelection #Face
						fnSelectFacesByMatID sPoly iIndex
						local aNewFaceSelection = sPoly.getSelection #Face
						cryMaxTools.model.poly.fnSelectFaces sPoly (aOldFaceSelection-aNewFaceSelection) bClearSelection:true
						
						fnSetMatButtonHighlights (fnGetMatButtonHighlights() - #{iIndex})
					)
					else if keyboard.controlPressed and keyboard.altPressed then
					(--ctrl+alt+click intersects with selection
						local aOldFaceSelection = sPoly.getSelection #Face
						fnSelectFacesByMatID sPoly iIndex
						local aNewFaceSelection = sPoly.getSelection #Face
						cryMaxTools.model.poly.fnSelectFaces sPoly (aOldFaceSelection*aNewFaceSelection) bClearSelection:true
						
						if aNewFaceSelection.numberSet != 0 then
						(
							fnSetMatButtonHighlights #{iIndex}
						)
						else
						(
							fnSetMatButtonHighlights #{}
						)
					)
					
					bCryPolyDoUpdateFaceSelection = true
					iCryPolyOldFaceSelectionCount = sPoly.getSelection #Face
				)
			)
			else
			(--click sets ID
				undo "Set MaterialID" on
				(
					if (sPoly.getSelection #Face).numberSet > 0 then
					(
						if classOf sPoly == Editable_Poly then
						(--different commands for edit_poly vs. editable_poly
							polyOp.setFaceMatID sPoly #selection iIndex
						)
						else
						(
							sPoly.materialIDToSet = iIndex-1
							sPoly.buttonOp #SetMaterial
						)
						fnSetMatButtonHighlights #{iIndex}
					)
					else
					(
						fnSetMatButtonHighlights #{}
					)
				)
			)
			
			if chkAutoUpdate.state == false then
			(
				fnSetMatButtonHighlights #{}
			)
		)
		else print "No poly selected."
	)
	
	fn fnGetMatIDs sPoly =
	(--returns a bitArray with the matIDs of the current face selection
		local aSelection = (sPoly.getSelection #Face) as array
		local aIDs = #{}
		for each in aSelection do
		(
			aIDs[sPoly.GetFaceMaterial each] = true
		)
		aIDs
	)
	
	fn fnUndoRedoSelection =
	(--updates matID buttons on undo/redo
		local sName = callbacks.notificationParam()
		if (matchPattern sName pattern:"*select*") or (matchPattern sName pattern:"*MaterialID*") then
		(
			if subObjectLevel == 4 or subObjectLevel == 5 then
			(
				local sPoly = cryMaxTools.model.poly.fnGetSelectedPolyMod()
				local iNewFaceSelectionCount = (cryMaxTools.model.poly.fnGetSelectedFaces (sPoly)).numberSet
				if iNewFaceSelectionCount != iCryPolyOldFaceSelectionCount and bCryPolyDoUpdateFaceSelection then
				(
					fnSetMatButtonHighlights (fnGetMatIDs sPoly)
					iCryPolyOldFaceSelectionCount = iNewFaceSelectionCount
				)
			)
		)
	)
	
	fn fnChangeSubSelectionLevel =
	(--highlights buttons according to selected faces' matIDs
		local iLevel = (callbacks.notificationParam())[1]
		if iLevel != 4  and iLevel != 5 then
		(
			fnSetMatButtonHighlights #{}
		)
		else if chkAutoUpdate.state == true then
		(
			fnSetMatButtonHighlights (fnGetMatIDs (cryMaxTools.model.poly.fnGetSelectedPolyMod()))
		)
	)
	
	fn fnUpdateSelection =
	(--install/delete face-selection change-handler for node in mod-panel
		local sPoly = undefined
		if (sPoly = cryMaxTools.model.poly.fnGetSelectedPolyMod()) != false then
		(
			iCryPolyOldFaceSelectionCount = (sPoly.getSelection #Face).numberSet
			fnSetMatButtonHighlights (fnGetMatIDs (cryMaxTools.model.poly.fnGetSelectedPolyMod()))
			when select sPoly changes id:#polySelectionChange do
			(
				if subObjectLevel == 4 or subObjectLevel == 5 then
				(
					local sPoly = cryMaxTools.model.poly.fnGetSelectedPolyMod()
					local iNewFaceSelectionCount = (sPoly.getSelection #Face).numberSet
					if iNewFaceSelectionCount != iCryPolyOldFaceSelectionCount and bCryPolyDoUpdateFaceSelection then
					(
						fnSetMatButtonHighlights (fnGetMatIDs sPoly)
						iCryPolyOldFaceSelectionCount = iNewFaceSelectionCount
					)
				)
			)
			
			callbacks.addScript #sceneUndo "(cryMaxTools.basic.ROMan.get \"rltCryPolyTools\").fnUndoRedoSelection()" id:#polyUpdateSelection
			callbacks.addScript #sceneRedo "(cryMaxTools.basic.ROMan.get \"rltCryPolyTools\").fnUndoRedoSelection()" id:#polyUpdateSelection
			callbacks.addScript #ModPanelSubObjectLevelChanged "(cryMaxTools.basic.ROMan.get \"rltCryPolyTools\").fnChangeSubSelectionLevel()" id:#polyUpdateSelection
		)
		else
		(
			fnSetMatButtonHighlights #{}
			deleteAllChangeHandlers id:#polySelectionChange
		)
	)

------------------------
--UI event handlers--
------------------------
	on rltCryPolyTools open do
	(
		local aPolyToolsSettings = cryMaxTools.basic.settingsMan.load "polyToolsSettings"
		if aPolyToolsSettings != false then
		(
			if aPolyToolsSettings[1].value[1] == "true" then
			(
				chkAutoUpdate.state = true
				callbacks.addScript #modPanelObjPostChange "(cryMaxTools.basic.ROMan.get \"rltCryPolyTools\").fnUpdateSelection()" id:#polyUpdateSelection
				fnUpdateSelection()
			)
			else
			(
				chkAutoUpdate.state = false
			)
		)
	)
	
	on rltCryPolyTools close do
	(
		local aPolyToolsSettings = #("polyToolsSettings")
		append aPolyToolsSettings ("chkAutoUpdate*@" + chkAutoUpdate.state as string)
		cryMaxTools.basic.settingsMan.save aPolyToolsSettings
		
		callbacks.removeScripts id:#polyUpdateSelection
		deleteAllChangeHandlers id:#polySelectionChange
		
		cryMaxTools.model.poly.fnMultiLoopRingMode state:false
		
		cryMaxTools.model.poly.fnDestroyMPI()
	)
	
	on btnMarkDegenerateTris pressed do fnPlaceHelperOnDegenerateTriangles()
	
	on btnDeleteDegenerateTris pressed do fnCloneObjectsAndDeleteDegenerateTriangles()
	
	on btnSelectLoop pressed do
	(--turn selection to loop/ring
		cryMaxTools.model.poly.fnSelectionToLoop()
	)
	
	on btnSelectRing pressed do
	(--turn selection to ring
		cryMaxTools.model.poly.fnSelectionToRing()
	)
	
	on ckbtnMultiLoopRing changed state do
	(--activated multi loop/ring mode
		cryMaxTools.model.poly.fnMultiLoopRingMode state:state
	)
	
	on btnSelectNonQuads pressed do
	(--selects non-quad faces
		cryMaxTools.model.poly.fnSelectNonQuads()
	)
	
	on btnSelectStars pressed do
	(--selects stars
		cryMaxTools.model.poly.fnSelectStars()
	)
	
	on chkAutoUpdate changed state do
	(--de/activate auto-update for mat-ID-selection buttons
		if state == true then
		(
			callbacks.addScript #modPanelObjPostChange "(cryMaxTools.basic.ROMan.get \"rltCryPolyTools\").fnUpdateSelection()" id:#polyUpdateSelection
			fnUpdateSelection()
		)
		else
		(
			fnSetMatButtonHighlights #{}
			callbacks.removeScripts id:#polyUpdateSelection
			deleteAllChangeHandlers id:#polySelectionChange
		)
	)
	
	on btnMatID1 changed state do	fnMatID 1
	on btnMatID2 changed state do	fnMatID 2
	on btnMatID3 changed state do	fnMatID 3
	on btnMatID4 changed state do	fnMatID 4
	on btnMatID5 changed state do	fnMatID 5
	on btnMatID6 changed state do	fnMatID 6
	on btnMatID7 changed state do	fnMatID 7
	on btnMatID8 changed state do	fnMatID 8
	on btnMatID9 changed state do	fnMatID 9
	on btnMatID10 changed state do fnMatID 10
	on btnMatID11 changed state do fnMatID 11
	on btnMatID12 changed state do fnMatID 12
	on btnMatID13 changed state do fnMatID 13
	on btnMatID14 changed state do fnMatID 14
	on btnMatID15 changed state do fnMatID 15
	on btnMatID16 changed state do fnMatID 16
	on btnMatID17 changed state do fnMatID 17
	on btnMatID18 changed state do fnMatID 18
	on btnMatID19 changed state do fnMatID 19
	on btnMatID20 changed state do fnMatID 20
		
	on pbtnSurfacePick picked sObj do
	(--create MPI from reference object
		if classOf sObj == PolyMeshObject or classOf sObj == Editable_Poly or classOf sObj == Editable_Mesh then
		(
			cryMaxTools.model.poly.fnBuildMPIForNode sObj
			
			pbtnSurfacePick.caption = sObj.name
		)
	)
	
	on btnSurfaceSnapshot pressed do
	(--create snapshot and turn to MPI
		if $selection.count == 1 then
		(
			if classOf $ == PolyMeshObject or classOf $ == Editable_Poly or classOf $ == Editable_Mesh then
			(
				cryMaxTools.model.poly.fnBuildMPIForNode $ bSnapshot:true
				
				pbtnSurfacePick.caption = cryMaxTools.model.poly.sSnapSurfaceSnapshot.name
			)
		)
	)
	
	on btnSnapToSurface pressed do
	(--snaps vertices to the surface
		if $selection.count == 1 and classOf $ == Editable_Poly then
		(
			cryMaxTools.model.poly.fnSnapVerticesToSurface $
		)
	)
	
	on chkSnapToSurfaceAuto changed state do
	(--activate move along surface mode
		if $selection.count == 1 and state and classOf $ == Editable_Poly then
		(
			cryMaxTools.model.poly.sSnapSurfaceWorkNode = $
			callbacks.addScript #modPanelObjPostChange "cryMaxTools.model.poly.fnUpdateSurfaceMoveNode()" id:#polyUpdateSurface
			
			when geometry cryMaxTools.model.poly.sSnapSurfaceWorkNode changes id:#polyMoveVertsOnSurface do
			(
				if cryMaxTools.model.poly.bDoAutoSnap and not keyboard.controlPressed then
				(--dirty no execute on undo check ctrl+z = undo
					cryMaxTools.model.poly.fnSnapVerticesToSurface cryMaxTools.model.poly.sSnapSurfaceWorkNode
				)
			)
		)
		else
		(
			cryMaxTools.model.poly.sSnapSurfaceWorkNode = undefined
			cryMaxTools.model.poly.fnUpdateSurfaceMoveNode()
		)
	)
	
	on btnSnapToOriginalPosition pressed do
	(--snap selected verts back to original position - only for snapshot mode
		if $selection.count == 1 and classOf $ == Editable_Poly and cryMaxTools.model.poly.sSnapSurfaceSnapshot.name == ($.name + "01") then
		(
			cryMaxTools.model.poly.fnSnapVerticesBackToSnapshot $
		)
	)
	
	on btnRemoveEdgeLoop pressed do
	(--remove edge loop
		cryMaxTools.model.poly.fnRemoveEdgeLoop()
	)
)

--add rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryPolyTools" rltCryPolyTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryPolyTools")