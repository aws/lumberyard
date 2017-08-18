------------------------------------
-- CryMaxTools v2.0
-- Basic Modeling Module v1.0
-- by Sascha Herfort
------------------------------------


--##########################################################################################
--### POLY STRUCT ##############################################################################
--### POLY STRUCT ##############################################################################
--### POLY STRUCT ##############################################################################
--##########################################################################################
struct cryMaxToolsModelPolyStruct
(--poly tools struct
--##########################################################################################
--DEBUG VARIABLES
--##########################################################################################
	bPrintFunctionNames = false,
	bPrintDebugInfo = false,
	bPrintUserInfo = true,
	
--##########################################################################################
--GLOBAL VARIABLES
--##########################################################################################
	bLoopMode = false,
	iLoopLevel = 0,
	bDoUpdateLoop = true,
	aLoopOldSelection = #{},
	iLoopStart = 0,
	iLoopEnd = 0,
	sLoopPoly = undefined,
	sLoopHelper = undefined,
	sLoopLayer = undefined,
	sLoopOperation = false,
	fLoopHelperSize = 0.05,
	
	mpiSnapSurface = undefined, --MeshProjIntersect() surface to snap to
	sSnapSurfaceSnapshot = undefined, --contains temporary snapshot for snap surface
	sSnapSurfaceWorkNode = undefined, --node the user is currently working on
	bDoAutoSnap = true, --to prevent auto snap change handler
	iMPIOperations = 0, --counts MPI operations
	
--##########################################################################################
--SHARED FUNCTIONS
--##########################################################################################
	fn fnGetSelectedPolyMod =
	(--returns the current selected poly modifier or baseobject or false
		if bPrintFunctionNames then (print "fnGetSelectedPolyMod")
		
		local sCurrentMod = modPanel.getCurrentObject()
		if classOf sCurrentMod == Edit_Poly then
		(
			return sCurrentMod
		)
		else if classOf sCurrentMod == Editable_Poly then
		(
			return $
		)
		else
		(
			return false
		)
	),
	
	fn fnSelectVertices sPoly aVerts bClearSelection:false =
	(--helper function - works with edit_poly and editable poly
		if bPrintFunctionNames then (print "fnSelectVertices")
		
		if bClearSelection then
		(
			sPoly.setSelection #Vertex #{}
		)
		if classOf sPoly == Edit_Poly then
		(
			sPoly.Select #Vertex aVerts
		)
		else
		(
			polyOp.setVertSelection sPoly (aVerts + (polyOp.getVertSelection sPoly))
		)
	),
	
	fn fnSelectEdges sPoly aEdges bClearSelection:false =
	(--helper function - works with edit_poly and editable poly
		if bPrintFunctionNames then (print "fnSelectEdges")
		
		if bClearSelection then
		(
			sPoly.setSelection #Edge #{}
		)
		if classOf sPoly == Edit_Poly then
		(
			sPoly.Select #Edge aEdges
		)
		else
		(
			polyOp.setEdgeSelection sPoly (aEdges + (polyOp.getEdgeSelection sPoly))
		)
	),
	
	fn fnSelectFaces sPoly aFaces bClearSelection:false =
	(--helper function - works with edit_poly and editable poly
		if bPrintFunctionNames then (print "fnSelectFaces")
		
		if bClearSelection then
		(
			sPoly.setSelection #Face #{}
		)
		if classOf sPoly == Edit_Poly then
		(
			sPoly.Select #Face aFaces
		)
		else
		(
			polyOp.setFaceSelection sPoly (aFaces + (polyOp.getFaceSelection sPoly))
		)
	),
	
	fn fnAreElementsNeighbors sPoly sMode iIndex1 iIndex2 =
	(--returns true if 2 elements are neighbors
		if bPrintFunctionNames then (print "fnAreElementsNeighbors")
		
		case sMode of
		(
			"vertex":
			(
				local bResult = false
				
				for i = 1 to sPoly.GetVertexEdgeCount iIndex1 while (not bResult) do
				(--try to find second vertex in first vertex's edges
					local iEdge = sPoly.GetVertexEdge iIndex1 i
					if (sPoly.GetEdgeVertex iEdge 1) == iIndex2 then
					(
						bResult = true
					)
					else if (sPoly.GetEdgeVertex iEdge 2) == iIndex2 then
					(
						bResult = true
					)
				)
				
				return bResult
			)
			"edge":
			(
				local bResultLoop = false
				
				for i = 1 to 2 while (not bResultLoop) do
				(--try to find shared vertex in edges
					local iVertex = sPoly.GetEdgeVertex iIndex1 i
					if (sPoly.GetEdgeVertex iIndex2 1) == iVertex then
					(
						bResultLoop = true
					)
					else if (sPoly.GetEdgeVertex iIndex2 2) == iVertex then
					(
						bResultLoop = true
					)
				)
				
				local bResultRing = false
			
				for i = 1 to 2 while (not bResultRing) do
				(--try to find shared face in edges
					local iFace = sPoly.GetEdgeFace iIndex1 i
					if (sPoly.GetEdgeFace iIndex2 1) == iFace then
					(
						bResultRing = true
					)
					else if (sPoly.GetEdgeFace iIndex2 2) == iFace then
					(
						bResultRing = true
					)
				)
				
				if bResultLoop and (not bResultRing) then
				(
					return "loop"
				)
				else if (not bResultLoop) and bResultRing then
				(
					return "ring"
				)
				else
				(
					return false
				)
			)
			"face":
			(
				local bResult = false
				
				for i = 1 to sPoly.GetFaceDegree iIndex1 while (not bResult) do
				(--try to find second face in first faces's edges
					local iEdge = sPoly.GetFaceEdge iIndex1 i
					if (sPoly.GetEdgeFace iEdge 1) == iIndex2 then
					(
						bResult = true
					)
					else if (sPoly.GetEdgeFace iEdge 2) == iIndex2 then
					(
						bResult = true
					)
				)
				
				return bResult
			)
		)
	),
	
	fn fnGetVertexListFromSubObjects nNode iSubObjectLevel =
	(--returns a list of vertices generated from the currently selected subobjects - requires an editable_poly node
		case iSubObjectLevel of
		(--get vertices from selected subobjects
			1: (polyOp.getVertsByFlag nNode 1)
			2: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
			3: (polyOp.getVertsUsingEdge nNode (polyOp.getEdgesByFlag nNode 1))
			4: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
			5: (polyOp.getVertsUsingFace nNode (polyOp.getFacesByFlag nNode 1))
			default: (nNode.verts as bitArray)
		)
	),
	
--##########################################################################################
--VERTEX SELECTION FUNCTIONS
--##########################################################################################
	fn fnGetContinuousVertexLoop sPoly iVertex1 iVertex2 bResetVertexSelection:true=
	(--returns a bitarray containing a continuous vertex loop from 2 neighboring vertices
		if bPrintFunctionNames then (print "fnGetContinuousVertexLoop")
		
		with redraw off 
		(
			local aOldEdgeSel = sPoly.GetSelection #Edge
			if bResetVertexSelection then (local aOldVertexSel = sPoly.GetSelection #Vertex)
			
			fnSelectVertices sPoly #{iVertex1,iVertex2} bClearSelection:true
			sPoly.ConvertSelection #Vertex #Edge requireAll:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeLoop
			sPoly.ConvertSelection #Edge #Vertex
			local aNewSel = sPoly.GetSelection #Vertex
			
			fnSelectEdges sPoly aOldEdgeSel bClearSelection:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 1
			if bResetVertexSelection then
			(
				fnSelectVertices sPoly aOldVertexSel bClearSelection:true
			)
			
			return aNewSel
		)
	),
	
	fn fnGetLoopSharedByVertices sPoly iVertex1 iVertex2 bResetVertexSelection:true =
	(--return edge loop that contains both verts or false -- needs to work with edit_poly mod, so no polyOps here
		if bPrintFunctionNames then (print "fnGetLoopSharedByVertices")
		
		with redraw off
		(
			local aOldEdgeSel = sPoly.GetSelection #Edge
			if bResetVertexSelection then (local aOldVertexSel = sPoly.GetSelection #Vertex)
			
			fnSelectVertices sPoly #{iVertex1} bClearSelection:true
			sPoly.ConvertSelection #Vertex #Edge
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeLoop
			local aEdges1 = sPoly.GetSelection #Edge
			
			if classOf sPoly == Edit_Poly then subObjectLevel = 1
			fnSelectVertices sPoly #{iVertex2} bClearSelection:true
			sPoly.ConvertSelection #Vertex #Edge
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeLoop
			local aEdges2 = sPoly.GetSelection #Edge
			
			fnSelectEdges sPoly aOldEdgeSel bClearSelection:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 1
			if bResetVertexSelection then
			(
				fnSelectVertices sPoly aOldVertexSel bClearSelection:true
			)
			
			local aShared = aEdges1*aEdges2
		)
		
		if aShared.numberSet > 0 then
		(
			return aShared
		)
		else
		(
			return false
		)
	),
	
	fn fnGetNextVertOnLoop iCurrentVert iLastVert aEdgeLoop sPoly =
	(--return the next vert on the loop or 0 -- lastVert is used to define direction
		if bPrintFunctionNames then (print "fnGetNextVertOnLoop")
		
		local i = sPoly.GetVertexEdgeCount iCurrentVert
		local aNextEdge = #{}
		local aVert = #{}
		while aVert.isEmpty == true and i > 0 do
		(--loop through all edges of vert
			aNextEdge = #{sPoly.GetVertexEdge iCurrentVert i}
			aNextEdge = aNextEdge*aEdgeLoop --in loop?
			if aNextEdge.isEmpty == false then
			(
				local aEdgeVerts = #{(sPoly.GetEdgeVertex aNextEdge.count 1), (sPoly.GetEdgeVertex aNextEdge.count 2)}
				aVert = aVert + (aEdgeVerts - #{iCurrentVert,iLastVert})
			)
			i = i-1
		)
		
		if aVert.isEmpty == false then
		(
			return (aVert as array)[1]
		)
		else
		(
			return 0
		)
	),
	
	fn fnGetVerticesInBetween sPoly iVert1 iVert2 bResetVertexSelection:true =
	(--returns a bitArray containing vertices that lie in-between 2 vertices on a loop or nothing
		if bPrintFunctionNames then (print "fnGetVerticesInBetween")
		
		with redraw off 
		(
			local aLoop = fnGetLoopSharedByVertices sPoly iVert1 iVert2 bResetVertexSelection:bResetVertexSelection
			if aLoop != false then
			(
				--get neighborverts of first selected vert
				local aEdges = #{}
				aEdges = aEdges + ((for i = 1 to (sPoly.GetVertexEdgeCount iVert1) collect (sPoly.GetVertexEdge iVert1 i)) as bitArray)
				aEdges = (aEdges*aLoop) as array
				if aEdges.count == 1 then append aEdges aEdges[1]
				local aVerts = #{(sPoly.GetEdgeVertex aEdges[1] 1), (sPoly.GetEdgeVertex aEdges[1] 2), (sPoly.GetEdgeVertex aEdges[2] 1), (sPoly.GetEdgeVertex aEdges[2] 2)}
				aVerts = (aVerts-(#(iVert1) as bitArray)) as array
				
				local iLastVert1 = iVert1
				local iCurrentVert1 = aVerts[1]
				local aNewSelection1 = #{aVerts[1]}
				
				if aVerts[2] != undefined then
				(
					local iLastVert2 = iVert1
					local iCurrentVert2 = aVerts[2]
					local aNewSelection2 = #{aVerts[2]}
				)
				local bAddedSomething = true
				
				while iCurrentvert1 != iVert2 and iCurrentvert2 != iVert2 and bAddedSomething do
				(--travel along loop until second vert is found
					local iNewVert1 = fnGetNextVertOnLoop iCurrentVert1 iLastVert1 aLoop sPoly
					if iNewVert1 != 0 then
					(
						if iNewVert1 != iVert2 then
						(--we only vertices inbetween
							append aNewSelection1 iNewVert1
						)
						iLastVert1 = iCurrentvert1
						iCurrentvert1 = iNewVert1
					)
					bAddedSomething = iNewVert1 != 0
					
					if aVerts[2] != undefined then
					(
						local iNewVert2 = fnGetNextVertOnLoop iCurrentVert2 iLastVert2 aLoop sPoly
						if iNewVert2 != 0 then
						(
							if iNewVert2 != iVert2 then
							(--we only vertices inbetween
								append aNewSelection2 iNewVert2
							)
							iLastVert2 = iCurrentvert2
							iCurrentvert2 = iNewVert2
						)
						bAddedSomething = (iNewVert2 != 0) or bAddedSomething
					)
				)
				
				local aNewSelection = #{}
				if iCurrentvert1 == iVert2 then
				(
					aNewSelection = aNewSelection1
				)
				if iCurrentvert2 == iVert2 then
				(
					aNewSelection = aNewSelection + aNewSelection2
				)
				
				return aNewSelection
			)
			else
			(
				return #{}
			)
		)
	),
	
	fn fnGetVertexLoop sPoly iVertex1 iVertex2 bResetVertexSelection:true =
	(--returns a bitArray containing a limited loop OR a continuous loop if vertices are neighbors OR nothing if loop impossible
		if bPrintFunctionNames then (print "fnGetVertexLoop")
		
		local aResult = #{}
		
		if fnAreElementsNeighbors sPoly "vertex" iVertex1 iVertex2 then
		(--return continuous face loop
			aResult = fnGetContinuousVertexLoop sPoly iVertex1 iVertex2 bResetVertexSelection:bResetVertexSelection
		)
		else
		(--return limited face loop or nothing
			aResult = fnGetVerticesInBetween sPoly iVertex1 iVertex2 bResetVertexSelection:bResetVertexSelection
		)
		
		return aResult
	),
	
	fn fnVertexMultiLoopModeUndoRedo =
	(--updates aLoopOldSelection on undo/redo
		if bPrintFunctionNames then (print "fnVertexMultiLoopModeUndoRedo")
		
		if bPrintDebugInfo then
		(
			print ("  Vertex selection difference: "  + ((polyOp.getVertSelection $).numberset - cryMaxTools.model.poly.aLoopOldSelection.numberSet) as string)
		)
		cryMaxTools.model.poly.aLoopOldSelection = polyOp.getVertSelection $
	),
	
	fn fnVertexMultiLoopMode =
	(--multi loop mode for vertex selection
		if bPrintFunctionNames then (print "fnVertexMultiLoopMode")
		
		deleteAllChangeHandlers id:#polyLoopModeSelectionUpdate
		callbacks.removeScripts id:#polyLoopModeSelectionUpdate
		
		undo off
		(--create helper object
			local aLoopHelpers
			cryMaxTools.model.poly.sLoopLayer.nodes &aLoopHelpers
			for each in aLoopHelpers do
			(
				delete each
			)
			
			cryMaxTools.model.poly.sLoopHelper = sphere radius:(fLoopHelperSize*2) segs:8 smooth:true
			cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
			cryMaxTools.model.poly.sLoopHelper.displayByLayer =true
			cryMaxTools.model.poly.sLoopHelper.name = "cryMultiLoopHelper"
			cryMaxTools.model.poly.sLoopLayer.addNode cryMaxTools.model.poly.sLoopHelper
		)
		
		cryMaxTools.model.poly.iLoopStart = 0
		cryMaxTools.model.poly.iLoopEnd = 0
		cryMaxTools.model.poly.aLoopOldSelection = cryMaxTools.model.poly.sLoopPoly.GetSelection #Vertex
		
		callbacks.addScript #sceneUndo "cryMaxTools.model.poly.fnVertexMultiLoopModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		callbacks.addScript #sceneRedo "cryMaxTools.model.poly.fnVertexMultiLoopModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		
		when select cryMaxTools.model.poly.sLoopPoly changes id:#polyLoopModeSelectionUpdate do
		(
			if cryMaxTools.model.poly.bDoUpdateLoop then
			(
				if cryMaxTools.model.poly.bPrintUserInfo then (print "Loop Mode Active!")
				
				local sPoly = cryMaxTools.model.poly.sLoopPoly
				local aLoopNewSelection = sPoly.GetSelection #Vertex
				local aAddedToSelection = aLoopNewSelection - cryMaxTools.model.poly.aLoopOldSelection
				local aRemovedFromSelection = cryMaxTools.model.poly.aLoopOldSelection - aLoopNewSelection
				cryMaxTools.model.poly.aLoopOldSelection = aLoopNewSelection
				
				if aAddedToSelection.numberSet == 1 and keyboard.controlPressed then
				(--if a single element was added to selection with ctrl pressed - enter add mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "add" then
					(
						cryMaxTools.model.poly.sLoopOperation = "add"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aAddedToSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aAddedToSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 1 and (not keyboard.altPressed) then
				(--if a single element was selected without alt pressed - enter add mode
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = (aLoopNewSelection as array)[1]
				)
				else if aRemovedFromSelection.numberSet == 1 and keyboard.altPressed then
				(--if a single element was deselected with alt pressed - enter remove mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "remove" then
					(
						cryMaxTools.model.poly.sLoopOperation = "remove"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aRemovedFromSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aRemovedFromSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 0 and (not keyboard.altPressed) then
				(--if selection cleared - reset
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = 0
				)
				
				undo off
				(--update helper object
					if cryMaxTools.model.poly.iLoopStart > 0 then
					(
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
						)
						else
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 0 100 255
						)
						
						cryMaxTools.model.poly.sLoopHelper.position = $.verts[cryMaxTools.model.poly.iLoopStart].pos
					)
					else
					(
						cryMaxTools.model.poly.sLoopHelper.position = [0,0,0]
					)
				)
				
				if cryMaxTools.model.poly.bPrintDebugInfo then
				(--print debug information
					print ("  aAddedToSelection: " + aAddedToSelection as string)
					print ("  aRemovedFromSelection: " + aRemovedFromSelection as string)
					print ("  iLoopStart: " + cryMaxTools.model.poly.iLoopStart as string)
					print ("  iLoopEnd: " + cryMaxTools.model.poly.iLoopEnd as string)
					print ("  sLoopOperation: " + cryMaxTools.model.poly.sLoopOperation as string)
					print ("  sLoopHelper position: " + cryMaxTools.model.poly.sLoopHelper.position as string)
				)
				
				if cryMaxTools.model.poly.iLoopStart > 0 and cryMaxTools.model.poly.iLoopEnd > 0 then
				(--if two valid elements then loop
					cryMaxTools.model.poly.bDoUpdateLoop = false --avoid self calls during loop operations
					
					undo "Select Vertex Loop" on
					(
						local aNewSelection = cryMaxTools.model.poly.fnGetVertexLoop sPoly cryMaxTools.model.poly.iLoopStart cryMaxTools.model.poly.iLoopEnd
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.fnSelectVertices sPoly (aLoopNewSelection + aNewSelection) bClearSelection:true
						)
						else
						(
							cryMaxTools.model.poly.fnSelectVertices sPoly (aLoopNewSelection - aNewSelection) bClearSelection:true
						)
					)
					
					cryMaxTools.model.poly.iLoopStart = 0
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.sLoopOperation = false
					undo off
					(
						cryMaxTools.model.poly.sLoopHelper.position = [0,0,0]
					)
					
					cryMaxTools.model.poly.bDoUpdateLoop = true
				)
				
				cryMaxTools.model.poly.aLoopOldSelection = sPoly.GetSelection #Vertex
			)
		)
	),
	
	fn fnSelectStars =
	(--adds all star-vertices to the selection
		if bPrintFunctionNames then (print "fnSelectStars")
		
		local sPoly = undefined
		if (sPoly = fnGetSelectedPolyMod()) != false then
		(
			undo "Select Stars" on
			(
				local aNewSelection = #()
				subObjectLevel = 1
				
				for i = 1 to sPoly.getNumVertices() do
				(
					if sPoly.GetVertexEdgeCount i != 4 and sPoly.GetVertexEdgeCount i == sPoly.GetVertexFaceCount i then
					(
						append aNewSelection i
					)
				)
				
				fnSelectVertices sPoly (aNewSelection as bitArray) bClearSelection:(not keyboard.controlPressed)
			)
		)
		else print "No poly selected."
	),
	
--##########################################################################################
--EDGE SELECTION FUNCTIONS
--##########################################################################################
	fn fnGetContinuousEdgeLoop sPoly iEdge bResetEdgeSelection:true =
	(--returns a bitArray containing an edgeloop of an edge
		if bPrintFunctionNames then (print "fnGetContinuousEdgeLoop")
		
		with redraw off
		(
			if bResetEdgeSelection then (local aOldEdgeSel = sPoly.GetSelection #Edge)
			
			fnSelectEdges sPoly #{iEdge} bClearSelection:true
			--if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeLoop
			local aEdges = sPoly.GetSelection #Edge
			
			if bResetEdgeSelection then (fnSelectEdges sPoly aOldEdgeSel bClearSelection:true)
			
			return aEdges
		)
	),
	
	fn fnGetContinuousEdgeRing sPoly iEdge bResetEdgeSelection:true =
	(--returns a bitArray containing an edgering of an edge
		if bPrintFunctionNames then (print "fnGetContinuousEdgeRing")
		
		with redraw off
		(
			if bResetEdgeSelection then (local aOldEdgeSel = sPoly.GetSelection #Edge)
			
			fnSelectEdges sPoly #{iEdge} bClearSelection:true
			--if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeRing
			local aEdges = sPoly.GetSelection #Edge
			
			if bResetEdgeSelection then (fnSelectEdges sPoly aOldEdgeSel bClearSelection:true)
			
			return aEdges
		)
	),
	
	fn fnGetLoopSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--return edge loop that contains both edges or false -- needs to work with edit_poly mod, so no polyOps here
		if bPrintFunctionNames then (print "fnGetLoopSharedByEdges")
		
		with redraw off
		(
			if bResetEdgeSelection then (local aOldSel = sPoly.GetSelection #Edge)
			
			fnSelectEdges sPoly #{iEdge1} bClearSelection:true
			--if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeLoop
			local aEdges1 = sPoly.GetSelection #Edge
			
			fnSelectEdges sPoly #{iEdge2} bClearSelection:true
			sPoly.ButtonOp #SelectEdgeLoop
			local aEdges2 = sPoly.GetSelection #Edge
			
			if bResetEdgeSelection then (fnSelectEdges sPoly aOldSel bClearSelection:true)
			
			local aShared = aEdges1*aEdges2
		)
		
		if aShared.numberSet > 0 then
		(
			aShared
		)
		else
		(
			false
		)
	),
	
	fn fnGetRingSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--return edge ring that contains both edges or false -- needs to work with edit_poly mod, so no polyOps here
		if bPrintFunctionNames then (print "fnGetRingSharedByEdges")
		
		with redraw off
		(
			if bResetEdgeSelection then (local aOldSel = sPoly.GetSelection #Edge)
			
			fnSelectEdges sPoly #{iEdge1} bClearSelection:true
			--if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeRing
			local aEdges1 = sPoly.GetSelection #Edge
			
			fnSelectEdges sPoly #{iEdge2} bClearSelection:true
			sPoly.ButtonOp #SelectEdgeRing
			local aEdges2 = sPoly.GetSelection #Edge
			
			if bResetEdgeSelection then (fnSelectEdges sPoly aOldSel bClearSelection:true)
			
			local aShared = aEdges1*aEdges2
		)
		
		if aShared.numberSet > 0 then
		(
			aShared
		)
		else
		(
			false
		)
	),
	
	fn fnGetNextEdgeOnLoop iCurrentEdge iLastEdge aEdgeLoop sPoly =
	(--return the next edge on the loop or 0 -- lastEdge is used to define direction
		if bPrintFunctionNames then (print "fnGetNextEdgeOnLoop")
		
		local iVert = sPoly.GetEdgeVertex iCurrentEdge 1 --vert between current edge and new edge
		if iVert == (sPoly.GetEdgeVertex iLastEdge 1) or iVert == (sPoly.GetEdgeVertex iLastEdge 2) then
		(
			iVert = sPoly.GetEdgeVertex iCurrentEdge 2
		)
		
		local i = sPoly.GetVertexEdgeCount iVert
		local aEdge = #{}
		while aEdge.isEmpty == true and i > 0 do
		(--loop through all edges of vert
			aEdge = #{sPoly.GetVertexEdge iVert i}
			aEdge = aEdge*aEdgeLoop --in loop?
			aEdge = aEdge-#{iCurrentEdge} --is current edge?
			i = i-1
		)
		
		if aEdge.isEmpty == false then
		(
			(aEdge as array)[1]
		)
		else
		(
			0
		)
	),
	
	fn fnGetNextEdgeOnRing iCurrentEdge iLastEdge aEdgeRing sPoly =
	(--return the next edge on the ring or 0 -- lastEdge is used to define direction
		if bPrintFunctionNames then (print "fnGetNextEdgeOnRing")
		
		local iFace = sPoly.GetEdgeFace iCurrentEdge 1 --face between current edge and new edge
		if iFace == (sPoly.GetEdgeFace iLastEdge 1) or iFace == (sPoly.GetEdgeFace iLastEdge 2) then
		(
			iFace = sPoly.GetEdgeFace iCurrentEdge 2
		)
		
		local i = sPoly.GetFaceDegree iFace
		local aEdge = #{}
		while aEdge.isEmpty == true and i > 0 do
		(--loop through all edges of vert
			aEdge = #{sPoly.GetFaceEdge iFace i}
			aEdge = aEdge*aEdgeRing --in ring?
			aEdge = aEdge-#{iCurrentEdge} --is current edge?
			i = i-1
		)
		
		if aEdge.isEmpty == false then
		(
			(aEdge as array)[1]
		)
		else
		(
			0
		)
	),
	
	fn fnGetEdgesInBetweenOnRing sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--returns a bitArray containing edges that lie in-between 2 edges on a ring or nothing
		if bPrintFunctionNames then (print "fnGetEdgesInBetweenOnRing")
		
		with redraw off 
		(
			local aRing = fnGetRingSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
			if aRing != false then
			(
				--get neighboredges of first selected edge
				local aEdges = #{}
				local aFaces = #(sPoly.GetEdgeFace iEdge1 1, sPoly.GetEdgeFace iEdge1 2)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetFaceDegree aFaces[1]) collect (sPoly.GetFaceEdge aFaces[1] i)) as bitArray)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetFaceDegree aFaces[2]) collect (sPoly.GetFaceEdge aFaces[2] i)) as bitArray)
				aEdges[iEdge1] = false
				aEdges = (aEdges*aRing) as array
				
				local iLastEdge1 = iEdge1
				local iCurrentEdge1 = aEdges[1]
				local aNewSelection1 = #{aEdges[1]}
				if aEdges[2] != undefined then
				(
					local iLastEdge2 = iEdge1
					local iCurrentEdge2 = aEdges[2]
					local aNewSelection2 = #{aEdges[2]}
				)
				local bAddedSomething = true
				
				while iCurrentEdge1 != iEdge2 and iCurrentEdge2 != iEdge2 and bAddedSomething do
				(--travel along ring until second edge is found
					local iNewEdge1 = fnGetNextEdgeOnRing iCurrentEdge1 iLastEdge1 aRing sPoly
					if iNewEdge1 != 0 then
					(
						if iNewEdge1 != iEdge2 then
						(--we only want edges inbetween
							append aNewSelection1 iNewEdge1
						)
						iLastEdge1 = iCurrentEdge1
						iCurrentEdge1 = iNewEdge1
					)
					bAddedSomething = iNewEdge1 != 0
					
					if aEdges[2] != undefined then
					(
						local iNewEdge2 = fnGetNextEdgeOnRing iCurrentEdge2 iLastEdge2 aRing sPoly
						if iNewEdge2 != 0 then
						(
							if iNewEdge2 != iEdge2 then
							(--we only want edges inbetween
								append aNewSelection2 iNewEdge2
							)
							iLastEdge2 = iCurrentEdge2
							iCurrentEdge2 = iNewEdge2
						)
						bAddedSomething = (iNewEdge2 != 0) or bAddedSomething
					)
				)
				
				local aNewSelection = #{}
				if iCurrentEdge1 == iEdge2 then
				(
					aNewSelection = aNewSelection1
				)
				if iCurrentEdge2 == iEdge2 then
				(
					aNewSelection = aNewSelection + aNewSelection2
				)
				
				return aNewSelection
			)
			else
			(
				return #{}
			)
		)
	),
	
	fn fnGetEdgesInBetweenOnLoop sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--returns a bitArray containing edges that lie in-between 2 edges on a loop/ring or nothing
		if bPrintFunctionNames then (print "fnGetEdgesInBetweenOnLoop")
		
		with redraw off 
		(
			local aLoop = fnGetLoopSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
			if aLoop != false then
			(
				--get neighboredges of first selected edge
				local aEdges = #{}
				local aVerts = #(sPoly.GetEdgeVertex iEdge1 1, sPoly.GetEdgeVertex iEdge1 2)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetVertexEdgeCount aVerts[1]) collect (sPoly.GetVertexEdge aVerts[1] i)) as bitArray)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetVertexEdgeCount aVerts[2]) collect (sPoly.GetVertexEdge aVerts[2] i)) as bitArray)
				aEdges[iEdge1] = false
				aEdges = (aEdges*aLoop) as array
				
				local iLastEdge1 = iEdge1
				local iCurrentEdge1 = aEdges[1]
				local aNewSelection1 = #{aEdges[1]}
				if aEdges[2] != undefined then
				(
					local iLastEdge2 = iEdge1
					local iCurrentEdge2 = aEdges[2]
					local aNewSelection2 = #{aEdges[2]}
				)
				local bAddedSomething = true
				
				while iCurrentEdge1 != iEdge2 and iCurrentEdge2 != iEdge2 and bAddedSomething do
				(--travel along loop until second edge is found
					local iNewEdge1 = fnGetNextEdgeOnLoop iCurrentEdge1 iLastEdge1 aLoop sPoly
					if iNewEdge1 != 0 then
					(
						if iNewEdge1 != iEdge2 then
						(--we only want edges inbetween
							append aNewSelection1 iNewEdge1
						)
						iLastEdge1 = iCurrentEdge1
						iCurrentEdge1 = iNewEdge1
					)
					bAddedSomething = iNewEdge1 != 0
					
					if aEdges[2] != undefined then
					(
						local iNewEdge2 = fnGetNextEdgeOnLoop iCurrentEdge2 iLastEdge2 aLoop sPoly
						if iNewEdge2 != 0 then
						(
							if iNewEdge2 != iEdge2 then
							(--we only want edges inbetween
								append aNewSelection2 iNewEdge2
							)
							iLastEdge2 = iCurrentEdge2
							iCurrentEdge2 = iNewEdge2
						)
						bAddedSomething = (iNewEdge2 != 0) or bAddedSomething
					)
				)
				
				local aNewSelection = #{}
				if iCurrentEdge1 == iEdge2 then
				(
					aNewSelection = aNewSelection1
				)
				if iCurrentEdge2 == iEdge2 then
				(
					aNewSelection = aNewSelection + aNewSelection2
				)
				
				return aNewSelection
			)
			else
			(
				return #{}
			)
		)
	),
	
	fn fnGetEdgesInBetween sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--returns a bitArray containing edges that lie in-between 2 edges on a loop/ring or nothing
		if bPrintFunctionNames then (print "fnGetEdgesInBetween")
		
		with redraw off 
		(
			local aLoop = fnGetLoopSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
			if aLoop != false then
			(
				--get neighboredges of first selected edge
				local aEdges = #{}
				local aVerts = #(sPoly.GetEdgeVertex iEdge1 1, sPoly.GetEdgeVertex iEdge1 2)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetVertexEdgeCount aVerts[1]) collect (sPoly.GetVertexEdge aVerts[1] i)) as bitArray)
				aEdges = aEdges + ((for i = 1 to (sPoly.GetVertexEdgeCount aVerts[2]) collect (sPoly.GetVertexEdge aVerts[2] i)) as bitArray)
				aEdges[iEdge1] = false
				aEdges = (aEdges*aLoop) as array
				
				local iLastEdge1 = iEdge1
				local iCurrentEdge1 = aEdges[1]
				local aNewSelection1 = #{aEdges[1]}
				if aEdges[2] != undefined then
				(
					local iLastEdge2 = iEdge1
					local iCurrentEdge2 = aEdges[2]
					local aNewSelection2 = #{aEdges[2]}
				)
				local bAddedSomething = true
				
				while iCurrentEdge1 != iEdge2 and iCurrentEdge2 != iEdge2 and bAddedSomething do
				(--travel along loop until second edge is found
					local iNewEdge1 = fnGetNextEdgeOnLoop iCurrentEdge1 iLastEdge1 aLoop sPoly
					if iNewEdge1 != 0 then
					(
						if iNewEdge1 != iEdge2 then
						(--we only want edges inbetween
							append aNewSelection1 iNewEdge1
						)
						iLastEdge1 = iCurrentEdge1
						iCurrentEdge1 = iNewEdge1
					)
					bAddedSomething = iNewEdge1 != 0
					
					if aEdges[2] != undefined then
					(
						local iNewEdge2 = fnGetNextEdgeOnLoop iCurrentEdge2 iLastEdge2 aLoop sPoly
						if iNewEdge2 != 0 then
						(
							if iNewEdge2 != iEdge2 then
							(--we only want edges inbetween
								append aNewSelection2 iNewEdge2
							)
							iLastEdge2 = iCurrentEdge2
							iCurrentEdge2 = iNewEdge2
						)
						bAddedSomething = (iNewEdge2 != 0) or bAddedSomething
					)
				)
				
				local aNewSelection = #{}
				if iCurrentEdge1 == iEdge2 then
				(
					aNewSelection = aNewSelection1
				)
				if iCurrentEdge2 == iEdge2 then
				(
					aNewSelection = aNewSelection + aNewSelection2
				)
				
				return aNewSelection
			)
			else
			(
				local aRing = fnGetRingSharedByEdges sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
				if aRing != false then
				(
					--get neighboredges of first selected edge
					local aEdges = #{}
					local aFaces = #(sPoly.GetEdgeFace iEdge1 1, sPoly.GetEdgeFace iEdge1 2)
					aEdges = aEdges + ((for i = 1 to (sPoly.GetFaceDegree aFaces[1]) collect (sPoly.GetFaceEdge aFaces[1] i)) as bitArray)
					aEdges = aEdges + ((for i = 1 to (sPoly.GetFaceDegree aFaces[2]) collect (sPoly.GetFaceEdge aFaces[2] i)) as bitArray)
					aEdges[iEdge1] = false
					aEdges = (aEdges*aRing) as array
					
					local iLastEdge1 = iEdge1
					local iCurrentEdge1 = aEdges[1]
					local aNewSelection1 = #{aEdges[1]}
					if aEdges[2] != undefined then
					(
						local iLastEdge2 = iEdge1
						local iCurrentEdge2 = aEdges[2]
						local aNewSelection2 = #{aEdges[2]}
					)
					local bAddedSomething = true
					
					while iCurrentEdge1 != iEdge2 and iCurrentEdge2 != iEdge2 and bAddedSomething do
					(--travel along ring until second edge is found
						local iNewEdge1 = fnGetNextEdgeOnRing iCurrentEdge1 iLastEdge1 aRing sPoly
						if iNewEdge1 != 0 then
						(
							if iNewEdge1 != iEdge2 then
							(--we only want edges inbetween
								append aNewSelection1 iNewEdge1
							)
							iLastEdge1 = iCurrentEdge1
							iCurrentEdge1 = iNewEdge1
						)
						bAddedSomething = iNewEdge1 != 0
						
						if aEdges[2] != undefined then
						(
							local iNewEdge2 = fnGetNextEdgeOnRing iCurrentEdge2 iLastEdge2 aRing sPoly
							if iNewEdge2 != 0 then
							(
								if iNewEdge2 != iEdge2 then
								(--we only want edges inbetween
									append aNewSelection2 iNewEdge2
								)
								iLastEdge2 = iCurrentEdge2
								iCurrentEdge2 = iNewEdge2
							)
							bAddedSomething = (iNewEdge2 != 0) or bAddedSomething
						)
					)
					
					local aNewSelection = #{}
					if iCurrentEdge1 == iEdge2 then
					(
						aNewSelection = aNewSelection1
					)
					if iCurrentEdge2 == iEdge2 then
					(
						aNewSelection = aNewSelection + aNewSelection2
					)
					
					return aNewSelection
				)
				else
				(
					return #{}
				)
			)
		)
	),
	
	fn fnGetEdgeLoopRing sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--returns a bitArray containing a limited loop/ring OR a continuous loop/ring if edges are neighbors OR nothing if loop/ring impossible
		if bPrintFunctionNames then (print "fnGetEdgeLoopRing")
		
		local aResult = #{}
		
		local sNeighborType = fnAreElementsNeighbors sPoly "edge" iEdge1 iEdge2
		if sNeighborType == "loop" then
		(--return continuous edge loop
			aResult = fnGetContinuousEdgeLoop sPoly iEdge1 bResetEdgeSelection:bResetEdgeSelection
		)
		else if sNeighborType == "ring" then
		(--return continuous edge ring
			aResult = fnGetContinuousEdgeRing sPoly iEdge1 bResetEdgeSelection:bResetEdgeSelection
		)
		else
		(--return limited edge loop or nothing
			aResult = fnGetEdgesInBetween sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
		)
		
		return aResult
	),
	
	fn fnGetEdgeLoop sPoly iEdge1 iEdge2 bResetEdgeSelection:true =
	(--returns a bitArray containing a limited loop OR a continuous loop if edges are neighbors OR nothing if loop impossible
		if bPrintFunctionNames then (print "fnGetEdgeLoop")
		
		local aResult = #{}
		
		local sNeighborType = fnAreElementsNeighbors sPoly "edge" iEdge1 iEdge2
		if sNeighborType == "loop" then
		(--return continuous edge loop
			aResult = fnGetContinuousEdgeLoop sPoly iEdge1 bResetEdgeSelection:bResetEdgeSelection
		)
		else
		(--return limited edge loop or nothing
			aResult = fnGetEdgesInBetweenOnLoop sPoly iEdge1 iEdge2 bResetEdgeSelection:bResetEdgeSelection
		)
		
		return aResult
	),
	
	fn fnEdgeMultiLoopRingModeUndoRedo =
	(--updates aLoopOldSelection on undo/redo
		if bPrintFunctionNames then (print "fnEdgeMultiLoopRingModeUndoRedo")
		
		if bPrintDebugInfo then
		(
			print ("  Edge selection difference: "  + ((polyOp.getEdgeSelection $).numberset - cryMaxTools.model.poly.aLoopOldSelection.numberSet) as string)
		)
		cryMaxTools.model.poly.aLoopOldSelection = polyOp.getEdgeSelection $
	),
	
	fn fnEdgeMultiLoopRingMode =
	(--multi loop mode for edge selection
		if bPrintFunctionNames then (print "fnEdgeMultiLoopRingMode")
		
		deleteAllChangeHandlers id:#polyLoopModeSelectionUpdate
		callbacks.removeScripts id:#polyLoopModeSelectionUpdate
		
		undo off
		(--create helper object
			local aLoopHelpers
			cryMaxTools.model.poly.sLoopLayer.nodes &aLoopHelpers
			for each in aLoopHelpers do
			(
				delete each
			)
			
			cryMaxTools.model.poly.sLoopHelper = cylinder radius:cryMaxTools.model.poly.fLoopHelperSize height:cryMaxTools.model.poly.fLoopHelperSize sides:8 smooth:true
			cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
			cryMaxTools.model.poly.sLoopHelper.displayByLayer =true
			cryMaxTools.model.poly.sLoopHelper.name = "cryMultiLoopHelper"
			cryMaxTools.model.poly.sLoopLayer.addNode cryMaxTools.model.poly.sLoopHelper
		)
		
		cryMaxTools.model.poly.iLoopStart = 0
		cryMaxTools.model.poly.iLoopEnd = 0
		cryMaxTools.model.poly.aLoopOldSelection = cryMaxTools.model.poly.sLoopPoly.GetSelection #Edge
		
		callbacks.addScript #sceneUndo "cryMaxTools.model.poly.fnEdgeMultiLoopRingModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		callbacks.addScript #sceneRedo "cryMaxTools.model.poly.fnEdgeMultiLoopRingModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		
		when select cryMaxTools.model.poly.sLoopPoly changes id:#polyLoopModeSelectionUpdate do
		(
			if cryMaxTools.model.poly.bDoUpdateLoop then
			(
				if cryMaxTools.model.poly.bPrintUserInfo then (print "Loop Mode Active!")
				
				local sPoly = cryMaxTools.model.poly.sLoopPoly
				local aLoopNewSelection = sPoly.GetSelection #Edge
				local aAddedToSelection = aLoopNewSelection - cryMaxTools.model.poly.aLoopOldSelection
				local aRemovedFromSelection = cryMaxTools.model.poly.aLoopOldSelection - aLoopNewSelection
				cryMaxTools.model.poly.aLoopOldSelection = aLoopNewSelection
				
				if aAddedToSelection.numberSet == 1 and keyboard.controlPressed then
				(--if a single element was added to selection with ctrl pressed - enter add mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "add" then
					(
						cryMaxTools.model.poly.sLoopOperation = "add"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aAddedToSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aAddedToSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 1 and (not keyboard.altPressed) then
				(--if a single element was selected without alt pressed - enter add mode
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = (aLoopNewSelection as array)[1]
				)
				else if aRemovedFromSelection.numberSet == 1 and keyboard.altPressed then
				(--if a single element was deselected with alt pressed - enter remove mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "remove" then
					(
						cryMaxTools.model.poly.sLoopOperation = "remove"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aRemovedFromSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aRemovedFromSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 0 and (not keyboard.altPressed) then
				(--if selection cleared - reset
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = 0
				)
				
				undo off
				(--update helper object
					if cryMaxTools.model.poly.iLoopStart > 0 then
					(
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
						)
						else
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 0 100 255
						)
						
						local p3Vert1 = $.verts[sPoly.getEdgeVertex cryMaxTools.model.poly.iLoopStart 1].pos
						local p3Vert2 = $.verts[sPoly.getEdgeVertex cryMaxTools.model.poly.iLoopStart 2].pos
						local p3CylinderZ = normalize (p3Vert2 - p3Vert1)
						cryMaxTools.model.poly.sLoopHelper.transform = matrixFromNormal p3CylinderZ
						cryMaxTools.model.poly.sLoopHelper.position = p3Vert1
						cryMaxTools.model.poly.sLoopHelper.height = distance p3Vert1 p3Vert2
						cryMaxTools.model.poly.sLoopHelper.radius = cryMaxTools.model.poly.fLoopHelperSize
					)
					else
					(
						cryMaxTools.model.poly.sLoopHelper.position = [0,0,0]
					)
				)
				
				if cryMaxTools.model.poly.bPrintDebugInfo then
				(--print debug information
					print ("  aAddedToSelection: " + aAddedToSelection as string)
					print ("  aRemovedFromSelection: " + aRemovedFromSelection as string)
					print ("  iLoopStart: " + cryMaxTools.model.poly.iLoopStart as string)
					print ("  iLoopEnd: " + cryMaxTools.model.poly.iLoopEnd as string)
					print ("  sLoopOperation: " + cryMaxTools.model.poly.sLoopOperation as string)
					print ("  sLoopHelper position: " + cryMaxTools.model.poly.sLoopHelper.position as string)
				)
				
				if cryMaxTools.model.poly.iLoopStart > 0 and cryMaxTools.model.poly.iLoopEnd > 0 then
				(--if two valid elements then loop
					cryMaxTools.model.poly.bDoUpdateLoop = false --avoid self calls during loop operations
					
					undo "Select Edge Loop" on
					(
						local aNewSelection = cryMaxTools.model.poly.fnGetEdgeLoopRing sPoly cryMaxTools.model.poly.iLoopStart cryMaxTools.model.poly.iLoopEnd
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.fnSelectEdges sPoly (aLoopNewSelection + aNewSelection) bClearSelection:true
						)
						else
						(
							cryMaxTools.model.poly.fnSelectEdges sPoly (aLoopNewSelection - aNewSelection) bClearSelection:true
						)
					)
					
					cryMaxTools.model.poly.iLoopStart = 0
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.sLoopOperation = false
					undo off
					(
						cryMaxTools.model.poly.sLoopHelper.position = [0,0,0]
					)
					
					cryMaxTools.model.poly.bDoUpdateLoop = true
				)
				
				cryMaxTools.model.poly.aLoopOldSelection = sPoly.GetSelection #Edge
			)
		)
	),
	
--##########################################################################################
--FACE SELECTION FUNCTIONS
--##########################################################################################
	fn fnGetContinuousFaceLoop sPoly iFace1 iFace2 bResetFaceSelection:true =
	(--returns a bitarray containing a continuous face loop from 2 neighboring faces
		if bPrintFunctionNames then (print "fnGetContinuousFaceLoop")
		
		with redraw off
		(
			local aOldEdgeSel = sPoly.GetSelection #Edge
			if bResetFaceSelection then (local aOldFaceSel = sPoly.GetSelection #Face)
			
			fnSelectFaces sPoly #{iFace1,iFace2} bClearSelection:true
			sPoly.ConvertSelection #Face #Edge requireAll:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeRing
			sPoly.ConvertSelection #Edge #Face
			
			local aResult = sPoly.GetSelection #Face
			
			fnSelectEdges sPoly aOldEdgeSel bClearSelection:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 4
			if bResetFaceSelection then
			(
				fnSelectFaces sPoly aOldFaceSel bClearSelection:true
			)
			
			return aResult
		)
	),
	
	fn fnGetRingSharedByFaces sPoly iFace1 iFace2 bResetFaceSelection:true =
	(--return edge ring that contains both faces or false -- needs to work with edit_poly mod, so no polyOps here
		if bPrintFunctionNames then (print "fnGetRingSharedByFaces")
		
		with redraw off
		(
			local aOldEdgeSel = sPoly.GetSelection #Edge
			if bResetFaceSelection then (local aOldFaceSel = sPoly.GetSelection #Face)
			
			fnSelectFaces sPoly #{iFace1} bClearSelection:true
			sPoly.ConvertSelection #Face #Edge
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeRing
			local aEdges1 = sPoly.GetSelection #Edge
			
			if classOf sPoly == Edit_Poly then subObjectLevel = 4
			fnSelectFaces sPoly #{iFace2} bClearSelection:true
			sPoly.ConvertSelection #Face #Edge
			if classOf sPoly == Edit_Poly then subObjectLevel = 2
			sPoly.ButtonOp #SelectEdgeRing
			local aEdges2 = sPoly.GetSelection #Edge
			
			fnSelectEdges sPoly aOldEdgeSel bClearSelection:true
			if classOf sPoly == Edit_Poly then subObjectLevel = 4
			if bResetFaceSelection then
			(
				fnSelectFaces sPoly aOldFaceSel bClearSelection:true
			)
			
			local aShared = aEdges1*aEdges2
		)
		
		if aShared.numberSet > 0 then
		(
			return aShared
		)
		else
		(
			return false
		)
	),
	
	fn fnGetNextFaceOnRing iCurrentFace iLastFace aEdgeRing sPoly =
	(--return the next face on the edge ring or 0 -- lastFace is used to define direction
		if bPrintFunctionNames then (print "fnGetNextFaceOnRing")
		
		local aEdges1 = (for i = 1 to sPoly.GetFaceDegree iCurrentFace collect sPoly.GetFaceEdge iCurrentFace i) as bitArray
		local aEdges2 = (for i = 1 to sPoly.GetFaceDegree iLastFace collect sPoly.GetFaceEdge iLastFace i) as bitArray
		local iEdge = (((aEdges1-aEdges2)*aEdgeRing) as array)[1] --edge between current face and new face
		
		local iFace = sPoly.GetEdgeFace iEdge 1
		if iFace == iCurrentFace then iFace = sPoly.GetEdgeFace iEdge 2
		
		iFace
	),
	
	fn fnGetFacesInBetween sPoly iFace1 iFace2 bResetFaceSelection:true =
	(--returns a bitArray containing faces that lie in-between 2 faces on a loop or nothing
		if bPrintFunctionNames then (print "fnGetFacesInBetween")
		
		local aRing = fnGetRingSharedByFaces sPoly iFace1 iFace2 bResetFaceSelection:bResetFaceSelection
		if aRing != false then
		(
			--get neighbor faces of first face
			local aFaces = #{}
			local aEdges = (for i = 1 to (sPoly.GetFaceDegree iFace1) collect (sPoly.GetFaceEdge iFace1 i)) as bitArray
			aEdges = (aEdges*aRing) as array
			for iEdge in aEdges do
			(
				local currentFace = sPoly.GetEdgeFace iEdge 1
				if currentFace != 0 then aFaces = aFaces + #{currentFace}
				local currentFace = sPoly.GetEdgeFace iEdge 2
				if currentFace != 0 then aFaces = aFaces + #{currentFace}
			)
			aFaces = (aFaces - #{iFace1}) as array
			
			local iLastFace1 = iFace1
			local iCurrentFace1 = aFaces[1]
			local aFacesInBetween1 = #{aFaces[1]}
			if aFaces[2] != undefined then
			(
				local iLastFace2 = iFace1
				local iCurrentFace2 = aFaces[2]
				local aFacesInBetween2 = #{aFaces[2]}
			)
			local bAddedSomething = true
			
			while iCurrentFace1 != iFace2 and iCurrentFace2 != iFace2 and bAddedSomething do
			(--travel along ring until second face is found
				local iNewFace1 = fnGetNextFaceOnRing iCurrentFace1 iLastFace1 aRing sPoly
				if iNewFace1 != 0 then
				(--only proceed if new face was found
					if iNewFace1 != iFace2 then
					(--we only want faces inbetween
						append aFacesInBetween1 iNewFace1
					)
					iLastFace1 = iCurrentFace1
					iCurrentFace1 = iNewFace1
				)
				bAddedSomething = iNewFace1 != 0
				
				if aFaces[2] != undefined then
				(
					local iNewFace2 = fnGetNextFaceOnRing iCurrentFace2 iLastFace2 aRing sPoly
					if iNewFace2 != 0 then
					(--only proceed if new face was found
						if iNewFace2 != iFace2 then
						(--we only want faces inbetween
							append aFacesInBetween2 iNewFace2
						)
						iLastFace2 = iCurrentFace2
						iCurrentFace2 = iNewFace2
					)
					bAddedSomething = (iNewFace2 != 0) or bAddedSomething
				)
			)
			
			local aFacesInBetween = #{}
			if iCurrentFace1 == iFace2 then
			(
				aFacesInBetween = aFacesInBetween1
			)
			if iCurrentFace2 == iFace2 then
			(
				aFacesInBetween = aFacesInBetween + aFacesInBetween2
			)
			
			return aFacesInBetween
		)
		else
		(
			return #{}
		)
	),
	
	fn fnGetFaceLoop sPoly iFace1 iFace2 bResetFaceSelection:true =
	(--returns a bitArray containing a limited loop OR a continuous loop if faces are neighbors OR nothing if loop impossible
		if bPrintFunctionNames then (print "fnGetFaceLoop")
		
		local aResult = #{}
		
		if fnAreElementsNeighbors sPoly "face" iFace1 iFace2 then
		(--return continuous face loop
			aResult = fnGetContinuousFaceLoop sPoly iFace1 iFace2 bResetFaceSelection:bResetFaceSelection
		)
		else
		(--return limited face loop or nothing
			aResult = fnGetFacesInBetween sPoly iFace1 iFace2 bResetFaceSelection:bResetFaceSelection
		)
		
		return aResult
	),
	
	fn fnFaceMultiLoopModeUndoRedo =
	(--updates aLoopOldSelection on undo/redo
		if bPrintFunctionNames then (print "fnFaceMultiLoopModeUndoRedo")
		
		if bPrintDebugInfo then
		(
			print ("  Face selection difference: "  + ((polyOp.getFaceSelection $).numberset - cryMaxTools.model.poly.aLoopOldSelection.numberSet) as string)
		)
		cryMaxTools.model.poly.aLoopOldSelection = polyOp.getFaceSelection $
	),
	
	fn fnFaceMultiLoopMode =
	(--multi loop mode for face selection
		if bPrintFunctionNames then (print "fnFaceMultiLoopMode")
		
		deleteAllChangeHandlers id:#polyLoopModeSelectionUpdate
		callbacks.removeScripts id:#polyLoopModeSelectionUpdate
		
		undo off
		(--create helper object
			local aLoopHelpers
			cryMaxTools.model.poly.sLoopLayer.nodes &aLoopHelpers
			for each in aLoopHelpers do
			(
				delete each
			)
			
			cryMaxTools.model.poly.sLoopHelper = Editable_Mesh name:"cryMultiLoopHelper"
			cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
			cryMaxTools.model.poly.sLoopHelper.displayByLayer =true
			cryMaxTools.model.poly.sLoopLayer.addNode cryMaxTools.model.poly.sLoopHelper
		)
		
		cryMaxTools.model.poly.iLoopStart = 0
		cryMaxTools.model.poly.iLoopEnd = 0
		cryMaxTools.model.poly.aLoopOldSelection = cryMaxTools.model.poly.sLoopPoly.GetSelection #Face
		
		callbacks.addScript #sceneUndo "cryMaxTools.model.poly.fnFaceMultiLoopModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		callbacks.addScript #sceneRedo "cryMaxTools.model.poly.fnFaceMultiLoopModeUndoRedo()" id:#polyLoopModeSelectionUpdate
		
		when select cryMaxTools.model.poly.sLoopPoly changes id:#polyLoopModeSelectionUpdate do
		(
			if cryMaxTools.model.poly.bDoUpdateLoop then
			(
				if cryMaxTools.model.poly.bPrintUserInfo then (print "Loop Mode Active!")
				
				local sPoly = cryMaxTools.model.poly.sLoopPoly
				local aLoopNewSelection = sPoly.GetSelection #Face
				local aAddedToSelection = aLoopNewSelection - cryMaxTools.model.poly.aLoopOldSelection
				local aRemovedFromSelection = cryMaxTools.model.poly.aLoopOldSelection - aLoopNewSelection
				cryMaxTools.model.poly.aLoopOldSelection = aLoopNewSelection
				
				if aAddedToSelection.numberSet == 1 and keyboard.controlPressed then
				(--if a single element was added to selection with ctrl pressed - enter add mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "add" then
					(
						cryMaxTools.model.poly.sLoopOperation = "add"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aAddedToSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aAddedToSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 1 and (not keyboard.altPressed) then
				(--if a single element was selected without alt pressed - enter add mode
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = (aLoopNewSelection as array)[1]
				)
				else if aRemovedFromSelection.numberSet == 1 and keyboard.altPressed then
				(--if a single element was deselected with alt pressed - enter remove mode
					if cryMaxTools.model.poly.iLoopStart == 0 or cryMaxTools.model.poly.sLoopOperation != "remove" then
					(
						cryMaxTools.model.poly.sLoopOperation = "remove"
						cryMaxTools.model.poly.iLoopEnd = 0
						cryMaxTools.model.poly.iLoopStart = (aRemovedFromSelection as array)[1]
					)
					else
					(
						cryMaxTools.model.poly.iLoopEnd = (aRemovedFromSelection as array)[1]
					)
				)
				else if aLoopNewSelection.numberSet == 0 and (not keyboard.altPressed) then
				(--if selection cleared - reset
					cryMaxTools.model.poly.sLoopOperation = "add"
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.iLoopStart = 0
				)
				
				undo off
				(--update helper object
					if cryMaxTools.model.poly.iLoopStart > 0 then
					(
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 100 255 50
						)
						else
						(
							cryMaxTools.model.poly.sLoopHelper.wireColor = color 0 100 255
						)
						
						setNumverts cryMaxTools.model.poly.sLoopHelper 0
						
						local iNewVertexCount = sPoly.getFaceDegree cryMaxTools.model.poly.iLoopStart
						local p3LocalOffset = (polyOp.getFaceNormal $ cryMaxTools.model.poly.iLoopStart)*cryMaxTools.model.poly.fLoopHelperSize
						
						setNumverts cryMaxTools.model.poly.sLoopHelper iNewVertexCount
						for i = 1 to iNewVertexCount do
						(
							cryMaxTools.model.poly.sLoopHelper.verts[i].pos = ($.verts[sPoly.getFaceVertex cryMaxTools.model.poly.iLoopStart i].pos + p3LocalOffset)
						)
						
						meshOp.createPolygon cryMaxTools.model.poly.sLoopHelper (#{1..iNewVertexCount} as array) smGroup:1
						setFaceNormal cryMaxTools.model.poly.sLoopHelper 1 (polyOp.getFaceNormal $ cryMaxTools.model.poly.iLoopStart)
					)
					else
					(
						setNumverts cryMaxTools.model.poly.sLoopHelper 0
					)
				)
				
				if cryMaxTools.model.poly.bPrintDebugInfo then
				(--print debug information
					print ("  aAddedToSelection: " + aAddedToSelection as string)
					print ("  aRemovedFromSelection: " + aRemovedFromSelection as string)
					print ("  iLoopStart: " + cryMaxTools.model.poly.iLoopStart as string)
					print ("  iLoopEnd: " + cryMaxTools.model.poly.iLoopEnd as string)
					print ("  sLoopOperation: " + cryMaxTools.model.poly.sLoopOperation as string)
					print ("  sLoopHelper verts: " + cryMaxTools.model.poly.sLoopHelper.verts.count as string)
				)
				
				if cryMaxTools.model.poly.iLoopStart > 0 and cryMaxTools.model.poly.iLoopEnd > 0 then
				(--if two valid elements then loop
					cryMaxTools.model.poly.bDoUpdateLoop = false --avoid self calls during loop operations
					
					undo "Select Face Loop" on
					(
						local aNewSelection = cryMaxTools.model.poly.fnGetFaceLoop sPoly cryMaxTools.model.poly.iLoopStart cryMaxTools.model.poly.iLoopEnd
						if cryMaxTools.model.poly.sLoopOperation == "add" then
						(
							cryMaxTools.model.poly.fnSelectFaces sPoly (aLoopNewSelection + aNewSelection) bClearSelection:true
						)
						else
						(
							cryMaxTools.model.poly.fnSelectFaces sPoly (aLoopNewSelection - aNewSelection) bClearSelection:true
						)
					)
					
					cryMaxTools.model.poly.iLoopStart = 0
					cryMaxTools.model.poly.iLoopEnd = 0
					cryMaxTools.model.poly.sLoopOperation = false
					undo off
					(
						setNumverts cryMaxTools.model.poly.sLoopHelper 0
					)
					
					cryMaxTools.model.poly.bDoUpdateLoop = true
				)
				
				cryMaxTools.model.poly.aLoopOldSelection = sPoly.GetSelection #Face
			)
		)
	),
	
	fn fnSelectNonQuads =
	(--adds all non-quad faces to the selection
		if bPrintFunctionNames then (print "fnSelectNonQuads")
		
		local sPoly = undefined
		if (sPoly = fnGetSelectedPolyMod()) != false then
		(
			undo "Select Non-Quads" on
			(
				local aNewSelection = #{}
				subObjectLevel = 4
				
				for i = 1 to sPoly.GetNumFaces() do
				(
					if sPoly.GetFaceDegree i != 4 then
					(
						aNewSelection[i] = true
					)
				)
				
				fnSelectFaces sPoly aNewSelection bClearSelection:keyboard.controlPressed
			)
		)
		else print "No poly selected."
	),
	
--##########################################################################################
--SNAP TO SURFACE FUNCTIONS
--##########################################################################################
	fn fnDestroyMPI =
	(--destroys existing MPI and snapshot
		if bPrintFunctionNames then (print "fnDestroyMPI")
		
		if classOf mpiSnapSurface == MeshProjIntersect then
		(--clear old surface
			mpiSnapSurface.free()
			mpiSnapSurface = undefined
		)
		
		if sSnapSurfaceSnapshot != undefined then
		(--clear snapshot
			if isValidNode sSnapSurfaceSnapshot then
			(
				delete sSnapSurfaceSnapshot
			)
			sSnapSurfaceSnapshot = undefined
		)
		
		deleteAllChangeHandlers id:#polyMoveVertsOnSurface
		callbacks.removeScripts id:#polyUpdateSurface
		
		try
		(
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSnapToSurface.enabled = false
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").chkSnapToSurfaceAuto.enabled = false
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").chkSnapToSurfaceAuto.checked = false
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSnapToOriginalPosition.enabled = false
		)
		catch()
	),
	
	fn fnBuildMPIForNode sNode bSnapshot:false =
	(--builds the MPI for fnFindClosestPointOnMPI()
		if bPrintFunctionNames then (print "fnBuildMPIForNode")
		
		fnDestroyMPI()
		
		if bSnapshot then
		(
			sSnapSurfaceSnapshot = snapshot sNode
			sSnapSurfaceSnapshot.xRay = true
			sNode = sSnapSurfaceSnapshot
			try
			(
				(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSnapToOriginalPosition.enabled = true
			)
			catch()
		)
		
		mpiSnapSurface = MeshProjIntersect()
		mpiSnapSurface.setNode sNode
		mpiSnapSurface.build()
		iMPIOperations = 0
		
		try
		(
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSnapToSurface.enabled = true
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").chkSnapToSurfaceAuto.enabled = true
		)
		catch()
	),
	
	fn fnFindClosestPointOnMPI p3Point =
	(--returns the closest point on a surface to a point in space or false --don
		if bPrintFunctionNames then (print "fnFindClosestPointOnMPI")
		
		if (mpiSnapSurface.closestFace p3Point doubleSided:true) then
		(
			iMPIOperations += 1
			mpiSnapSurface.getHitPos()
		)
		else
		(
			false
		)
	),
	
	fn fnUpdateSurfaceMoveNode =
	(--callback for surface work mode - deactivate auto mode when selection changed
		if bPrintFunctionNames then (print "fnUpdateSurfaceMoveNode")
		
		if $ != sSnapSurfaceWorkNode then
		(
			deleteAllChangeHandlers id:#polyMoveVertsOnSurface
			callbacks.removeScripts id:#polyUpdateSurface
			(cryMaxTools.basic.ROMan.get "rltCryPolyTools").chkSnapToSurfaceAuto.state = false
		)
	),
	
	fn fnSnapVerticesToSurface sPoly =
	(--snap selected vertices to MPI surface
		if bPrintFunctionNames then (print "fnSnapVerticesToSurface")
		
		bDoAutoSnap = false
		
		undo "Snap Vertices to Surface" on
		(
			local aVerts = sPoly.getSelection #Vertex
			for each in aVerts do
			(
				local p3NewPos = fnFindClosestPointOnMPI sPoly.verts[each].pos
				
				if p3NewPos != false then
				(
					sPoly.verts[each].pos = p3NewPos
				)
			)
			
			if iMPIOperations > 10000 then
			(--rebuild MPI to avoid memory leaking
			)
		)
		
		bDoAutoSnap = true
	),
	
	fn fnSnapVerticesBackToSnapshot sPoly =
	(--snap vertices back to original position -- from snapshot mesh
		if bPrintFunctionNames then (print "fnSnapVerticesBackToSnapshot")
		
		bDoAutoSnap = false
		
		undo "Snap Vertices to Snapshot" on
		(
			local aVerts = sPoly.getSelection #Vertex
			for each in aVerts do
			(
				sPoly.verts[each].pos = sSnapSurfaceSnapshot.verts[each].pos
			)
		)
		
		bDoAutoSnap = true
	),
	
--##########################################################################################
--TOPOLOGY FUNCTIONS
--##########################################################################################
	fn fnRemoveEdgeLoop sPoly: aEdges: =
	(--removes loops of selected edges or edges in aEdges
		if bPrintFunctionNames then (print "fnRemoveEdgeLoop")
		
		if sPoly == unsupplied then
		(--get poly
			local sModifier = modPanel.getCurrentObject()
			if classOf sModifier == Edit_Poly or classOf sModifier == Editable_Poly then
			(
				sPoly = sModifier
			)
			else
			(
				sPoly = undefined
			)
		)
		
		if aEdges == unsupplied and sPoly != undefined then
		(
			aEdges = sPoly.getSelection #Edge
		)
		
		if sPoly != undefined and aEdges.numberSet != 0 then
		(
			undo "Remove Edge Loop" on
			(
				subObjectLevel = 2
				sPoly.buttonOp #selectEdgeLoop
				sPoly.convertSelection #Edge #Vertex
				local aVerts = sPoly.getSelection #Vertex
				local aStarVerts = #{}
				for each in aVerts do
				(
					local iEdgeCount = sPoly.getVertexEdgeCount each
					if iEdgeCount != 4 then
					(
						if iEdgeCount == (sPoly.getVertexFaceCount each) then
						(
							append aStarVerts each
						)
					)
				)
				sPoly.setSelection #Vertex (aVerts-aStarVerts)
				if classOf sPoly == Editable_Poly then (sPoly.buttonOp #remove) else (sPoly.buttonOp #RemoveEdge)
				subObjectLevel = 1
				if classOf sPoly == Editable_Poly then (sPoly.buttonOp #remove) else (sPoly.buttonOp #RemoveVertex)
				subObjectLevel = 2
			)
		)
	),
	
--##########################################################################################
--GENERAL FUNCTIONS
--##########################################################################################
	fn fnSelectionToLoop =
	(--turn selection to loop/ring - auto select limited/continuous loop/ring
		if bPrintFunctionNames then (print "fnSelectionToLoop")
		
		local sPoly = undefined
		if (sPoly = fnGetSelectedPolyMod()) != false then
		(
			if sPoly.GetEPolySelLevel() == #Vertex then
			(
				local aVertSelection = (sPoly.GetSelection #Vertex) as array
				if aVertSelection.count == 2 then
				(
					cryMaxTools.model.poly.bDoUpdateLoop = false
					undo "Loop Vertex Selection" on
					(
						local aNewSelection = fnGetVertexLoop sPoly aVertSelection[1] aVertSelection[2] bResetVertexSelection:false
						fnSelectVertices sPoly (aNewSelection + (aVertSelection as bitArray)) bClearSelection:true
					)
					cryMaxTools.model.poly.bDoUpdateLoop = true
				)
				else print "Select exactly 2 vertices!"
			)
			else if sPoly.GetEPolySelLevel() == #Edge then
			(
				local aEdgeSelection = (sPoly.GetSelection #Edge) as array
				if aEdgeSelection.count == 2 then
				(
					cryMaxTools.model.poly.bDoUpdateLoop = false
					undo "Loop Edge Selection" on
					(
						local aNewSelection = fnGetEdgeLoop sPoly aEdgeSelection[1] aEdgeSelection[2] bResetVertexSelection:false
						fnSelectEdges sPoly (aNewSelection + (aEdgeSelection as bitArray)) bClearSelection:true
					)
					cryMaxTools.model.poly.bDoUpdateLoop = true
					
					if aNewSelection.numberSet == 0 then
					(
						sPoly.ButtonOp #SelectEdgeLoop
					)
				)
				else
				(
					sPoly.ButtonOp #SelectEdgeLoop
				)
			)
			else if sPoly.GetEPolySelLevel() == #Face then
			(
				local aFaceSelection = (sPoly.GetSelection #Face) as array
				if aFaceSelection.count == 2 then
				(
					cryMaxTools.model.poly.bDoUpdateLoop = false
					undo "Loop Face Selection" on
					(
						local aNewSelection = fnGetFaceLoop sPoly aFaceSelection[1] aFaceSelection[2] bResetVertexSelection:false
						fnSelectFaces sPoly (aNewSelection + (aFaceSelection as bitArray)) bClearSelection:true
					)
					cryMaxTools.model.poly.bDoUpdateLoop = true
				)
				else print "Select exactly 2 faces!"
			)
			else print "Works only in vertex, edge or face selection mode."
		)
		else print "No poly selected."
	),
	
	fn fnMultiLoopRingUpdate bFromCallback:false =
	(--watches subobj level and modifier changes -- updates selection change handlers
		if bPrintFunctionNames then (print "fnMultiLoopRingUpdate")
		if bPrintDebugInfo then
		(--print debug information
			print ("  bFromCallback: " + bFromCallback as string)
			print ("  sLoopPoly: " + cryMaxTools.model.poly.sLoopPoly as string)
			print ("  subObjectLevel:" + subObjectLevel as string)
			print ("  callbacks.notificationParam: " + callbacks.notificationParam() as string)
		)
		
		if cryMaxTools.model.poly.sLoopPoly == false or cryMaxTools.model.poly.fnGetSelectedPolyMod() != cryMaxTools.model.poly.sLoopPoly or subObjectLevel == 0 or subObjectLevel == 3 or subObjectLevel == 5 then
		(--selected mod changes = exit loop mode
			cryMaxTools.model.poly.fnMultiLoopRingMode state:false
		)
		else if bDoUpdateLoop then
		(--subobj level changed -- update loop mode
			iLoopLevel = subObjectLevel
		)
		
		if bDoUpdateLoop and cryMaxTools.model.poly.bLoopMode then
		(
			gc()
			
			cryMaxTools.model.poly.sLoopLayer = LayerManager.getLayerFromName "cryMultiLoopLayer"
			if cryMaxTools.model.poly.sLoopLayer == undefined then
			(
				undo off
				(
					cryMaxTools.model.poly.sLoopLayer = LayerManager.newLayerFromName "cryMultiLoopLayer"
					cryMaxTools.model.poly.sLoopLayer.isFrozen = true
					cryMaxTools.model.poly.sLoopLayer.showFrozenInGray = false
				)
			)
			
			case iLoopLevel of
			(
				1: fnVertexMultiLoopMode()
				2: fnEdgeMultiLoopRingMode()
				4: fnFaceMultiLoopMode()
			)
		)
	),
	
	fn fnMultiLoopRingMode state:"toggle" =
	(--auto-build loop between last 2 selected elements if possible -- state:on,off,toggle to switch between active/inactive
		if bPrintFunctionNames then (print "fnMultiLoopRingMode")
		
		local bStateChanged = true
		
		case state of
		(--set global loop-mode variable
			"toggle":
			(
				bStateChanged = true
				cryMaxTools.model.poly.bLoopMode = not cryMaxTools.model.poly.bLoopMode
			)
			true:
			(
				bStateChanged = (cryMaxTools.model.poly.bLoopMode == false)
				cryMaxTools.model.poly.bLoopMode = true
			)
			false:
			(
				bStateChanged = (cryMaxTools.model.poly.bLoopMode == true)
				cryMaxTools.model.poly.bLoopMode = false
			)
		)
		
		if bStateChanged then
		(--only do stuff on state change
			if cryMaxTools.model.poly.bLoopMode then
			(
				if bPrintUserInfo then (print "Loop Mode On")
				
				cryMaxTools.model.poly.sLoopPoly = undefined
				if (cryMaxTools.model.poly.sLoopPoly = fnGetSelectedPolyMod()) != false then
				(--register change handlers
					callbacks.addScript #modPanelObjPostChange "cryMaxTools.model.poly.fnMultiLoopRingUpdate bFromCallback:true" id:#polyLoopMode
					callbacks.addScript #ModPanelSubObjectLevelChanged "cryMaxTools.model.poly.fnMultiLoopRingUpdate bFromCallback:true" id:#polyLoopMode
				)
				cryMaxTools.model.poly.fnMultiLoopRingUpdate()
				
				try
				(
					(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSelectLoop.enabled = false
					(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSelectRing.enabled = false
				)
				catch()
			)
			else
			(--unregister change handlers
				if bPrintUserInfo then (print "Loop Mode Off")
				
				cryMaxTools.model.poly.sLoopLayer = LayerManager.getLayerFromName "cryMultiLoopLayer"
				if cryMaxTools.model.poly.sLoopLayer != undefined then
				(
					undo off
					(
						local aLoopHelpers
						cryMaxTools.model.poly.sLoopLayer.nodes &aLoopHelpers
						for each in aLoopHelpers do
						(
							delete each
						)
						
						LayerManager.deleteLayerByName "cryMultiLoopLayer"
						cryMaxTools.model.poly.sLoopLayer = undefined
					)
				)
				
				cryMaxTools.model.poly.sLoopHelper = undefined
				
				try
				(
					(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnMultiLoopRing.state = false
					(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSelectLoop.enabled = true
					(cryMaxTools.basic.ROMan.get "rltCryPolyTools").btnSelectRing.enabled = true
				)
				catch()
				callbacks.removeScripts id:#polyLoopModeSelectionUpdate
				callbacks.removeScripts id:#polyLoopMode
				deleteAllChangeHandlers id:#polyLoopModeSelectionUpdate
			)
		)
	),
	
	fn fnSelectionToRing =
	(--turn selection to ring - auto select limited/continuous loop - only edges
		if bPrintFunctionNames then (print "fnSelectionToRing")
		
		local sPoly = undefined
		if (sPoly = fnGetSelectedPolyMod()) != false then
		(
			if sPoly.GetEPolySelLevel() == #Edge then
			(
				local aEdgeSelection = (sPoly.GetSelection #Edge) as array
				if aEdgeSelection.count == 2 then
				(
					undo "Limited Edge Ring" on
					(
						local aNewSelection
						local sNeighborType = fnAreElementsNeighbors sPoly "edge" aEdgeSelection[1] aEdgeSelection[2]
						if sNeighborType == "ring" then
						(--return continuous edge ring
							aNewSelection = fnGetContinuousEdgeRing sPoly aEdgeSelection[1] bResetEdgeSelection:true
						)
						else
						(--return limited edge loop or nothing
							aNewSelection = fnGetEdgesInBetweenOnRing sPoly aEdgeSelection[1] aEdgeSelection[2] bResetEdgeSelection:true
						)
						fnSelectEdges sPoly (aNewSelection + (aEdgeSelection as bitArray)) bClearSelection:true
					)
					if aNewSelection.numberSet == 0 then
					(
						sPoly.ButtonOp #SelectEdgeRing
					)
				)
				else
				(
					sPoly.ButtonOp #SelectEdgeRing
				)
			)
			else print "Works only in edge selection mode."
		)
		else print "No poly selected."
	)
)


--##########################################################################################
--### PROXY STRUCT #############################################################################
--### PROXY STRUCT #############################################################################
--### PROXY STRUCT #############################################################################
--##########################################################################################
struct cryMaxToolsModelProxyStruct
(--phys proxy and primitive alignment tools
	--######################################################################################
	--###  GLOBAL VARIABLES  #####################################################################
	--######################################################################################
	
	--TWEAKABLES MAGICIAN
	iRefinementSearchSteps = 21, --step inbetween local minimum's meighbors to refine angle
	iMaxRefinementIterations = 9, --max repeat refinement
	bEarlyAbort = true, --aborts BB search when error threshold reached
	fMaxError = 1, --stop refining when two successive approximations are less than this close together (centimeters)
	fBBDifferenceExtrapolationFactor = 0.1,
	sProxyNameDefault = "$physics_proxy",
	
	--DEBUG OPTIONS
	bDebugPrint = false,
	bDebugDraw = false,
	bRealtimePreview = false,
	
	--GLOBALS
	fInitialSearchRange = 80.0, --should not be changed
	iInitialSearchSteps = 9, --step along initial arc to find local minimum
	qOrientation = quat 0 0 0 1,
	iSearchSteps = 0,
	iRefinementSteps = 0,
	modMatIDModifier = undefined,
	nPreviewBox = undefined,
	
	
	--######################################################################################
	--###  FUNCTIONS  ##########################################################################
	--######################################################################################
	
	fn fnGetBBVolume aBB =
	(--returns the volume a BB - assumes array of min/max points of BB as input
		local p3Size = aBB[2] - aBB[1]
		
		p3Size.x*p3Size.y*p3Size.z
	),
	
	fn fnGetOrientedBBSize aBB qOrientation =
	(--returns the extents of the supplied BB after applying the supplied orientation
		aBB[2]*qOrientation - aBB[1]*qOrientation
	),
	
	fn fnGetOrientedBBCentroid aBB qOrientation =
	(--returns the centroid of the supplied BB after applying the supplied orientation
		(aBB[1] + aBB[2])*.5*qOrientation
	),
	
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
	),
	
	fn fnGetAABBFromVertices nNode aVertices =
	(--returns the axis-aligned bounding box of the supplied vertices
		local aXCoords = for each in aVertices collect nNode.verts[each].pos.x
		local aYCoords = for each in aVertices collect nNode.verts[each].pos.y
		local aZCoords = for each in aVertices collect nNode.verts[each].pos.z
		
		#([aMin aXCoords, aMin aYCoords, aMin aZCoords], [aMax aXCoords, aMax aYCoords, aMax aZCoords])
	),
	
	fn fnGetBBFromRotatedVertices nNode aVertices qOrientation =
	(--returns bounding box of the supplied vertices after applying supplied rotation
		local aXCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).x
		local aYCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).y
		local aZCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).z
		
		#([aMin aXCoords, aMin aYCoords, aMin aZCoords], [aMax aXCoords, aMax aYCoords, aMax aZCoords])
	),
	
	fn fnGetBBSizeFromRotatedVertices nNode aVertices qOrientation =
	(--returns the bounding box size of the supplied vertices after applying the supplied rotation
		local aXCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).x
		local aYCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).y
		local aZCoords = for each in aVertices collect (nNode.verts[each].pos*qOrientation).z
		
		[aMax aXCoords, aMax aYCoords, aMax aZCoords] - [aMin aXCoords, aMin aYCoords, aMin aZCoords]
	),
	
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
			print ("Smallest extent: #" + iMinExtent as string + "=" + (aExtents[iMinExtent]/1000000) as string + "cubic meters")
			print ("Neighbor 1: #" + aNeighbors[1] as string + "=" + (aExtents[aNeighbors[1]]/1000000) as string + "cubic meters")
			print ("Neighbor 2: #" + aNeighbors[2] as string + "=" + (aExtents[aNeighbors[2]]/1000000) as string + "cubic meters")
		)
		
		--find start/end angle to refine search in between
		--local aAngles = #(aAnglesToTest[findItem aExtents (aMin #(aExtents[aNeighbors[1]],aExtents[aNeighbors[2]]))]) --add neighbor with smallest BB size first
		--append aAngles aAnglesToTest[findItem aExtents (aMax #(aExtents[aNeighbors[1]],aExtents[aNeighbors[2]]))] --add neighbor with biggest BB size second
		local fStepSize = (fAngle2 - fAngle1)/(iSteps - 1.0)
		local aAngles = #(if iMinExtent == 1 then aAnglesToTest[1] - fStepSize else aAnglesToTest[aNeighbors[1]], if iMinExtent == iSteps then aAnglesToTest[iSteps] + fStepSize else aAnglesToTest[aNeighbors[2]])
		
		if bDebugPrint then print ("aAngles: " + aAngles as string)
		
		aAngles
	),
	
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
	),
	
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
	),
	
	fn fnCreateCryPhysBoxPrimitive aBB qOrientation fBias sName mMaterial iMatID sUDPs:"" =
	(--creates a cryEngine physics primitive box - returns the new scene node
		local nMinBB = box length:(aBB[2].y - aBB[1].y + fBias) width:(aBB[2].x - aBB[1].x + fBias) height:(aBB[2].z - aBB[1].z + fBias)
		CenterObject nMinBB
		nMinBB.pos = (aBB[1] + (aBB[2] - aBB[1])*.5)*(inverse qOrientation)
		rotate nMinBB qOrientation
			
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
	),
	
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
	),
	
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
	),
	
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
)

struct cryMaxToolsModelStruct
(--modeling tools struct
	poly = cryMaxToolsModelPolyStruct(),
	proxy = cryMaxToolsModelProxyStruct()
)
cryMaxTools.model = cryMaxToolsModelStruct()

macroScript Poly_Loop category:"CryMaxTools" tooltip:"Loop/Ring Poly Selection"
(--macroscript for keyboard shortcuts
	on isVisible do
	(
		if cryMaxTools != undefined then
			return true
		else
			return false
	)
	
	on execute do
	(
		cryMaxTools.model.poly.fnSelectionToLoop()
	)
)

macroScript Poly_Ring category:"CryMaxTools" tooltip:"Ring Edge Selection"
(--macroscript for keyboard shortcuts
	on isVisible do
	(
		if cryMaxTools != undefined then
			return true
		else
			return false
	)
	
	on execute do
	(
		cryMaxTools.model.poly.fnSelectionToRing()
	)
)

macroScript Poly_MutliLoop category:"CryMaxTools" tooltip:"Multi Loop/Ring Poly Selection"
(--macroscript for keyboard shortcuts
	on isVisible do
	(
		if cryMaxTools != undefined then
			return true
		else
			return false
	)
	
	on execute do
	(
		undo off
		(
			cryMaxTools.model.poly.fnMultiLoopRingMode()
		)
	)
)
