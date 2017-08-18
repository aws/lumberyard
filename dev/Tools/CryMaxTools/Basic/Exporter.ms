------------------------------------
-- CryMaxTools v2.0
-- Basic Exporter Module v1.0
-- by Sascha Herfort
------------------------------------


--###########################################################################################
--OBJECT EXPORTER STRUCT
--OBJECT EXPORTER STRUCT   #########################################################################
--OBJECT EXPORTER STRUCT
--###########################################################################################
struct cryMaxToolsObjectExportStruct
(--object exporter struct
--#########################################################
--GLOBAL VARIABLES
--#########################################################
	sExportNodePrefix = "cryExportNode_",
	aExportNodes = #(), --contains all export nodes
	sExportNodeLayer = undefined, --references cry export layer
	bMakeExportNodeNamesUnique = true, --make names of cryExportNodes unqiue incase of duplicate names
	fMorphThresholdDefault = 0.01, --default value for morph threshold in centimeters
	
	bCheckMaterials = true, --set by user - warn if no crytek shader applied
	bCheckMorphs = true, --set by user - warn if morpher mod has weights > 0
	bCheckBoneScale = true, --set by user - checks for scaled bones
	bMarkDegenerateFaces = true, --set by user - places helpers on degenerate face positions returned by RC
	bMarkCollapsedVertices = true, --set by user - places helpers on collapsed triangle positions returned by RC
	
	
--#########################################################
--CHECK FUNCTIONS
--#########################################################
	
--### MORPH SETUP CHECK BEGIN ###
	fn fnCheckNonZeroMorphs aExportNodes =
	(--return nodes that have morph modifier with non-zero-weights
		local aResult = #()
		
		for sExportNode in aExportNodes do
		(
			for sObject in sExportNode.children do
			(
				local sMorpher = undefined
				local bHasMorpher = true
				try
				(
					sMorpher = sObject.morpher
				)
				catch
				(
					bHasMorpher = false
				)
				
				if bHasMorpher then
				(
					local bLastMorphUsed = true
					local iIndex = 1
					local aObjectsNonZeroMorphs = #()
					
					while bLastMorphUsed do
					(
						if sMorpher[iIndex] != undefined and sMorpher[iIndex].name != "- empty -" then
						(
							if (WM3_MC_GetValue sMorpher iIndex) != 0.0 then
							(
								append aObjectsNonZeroMorphs (WM3_MC_GetName sMorpher iIndex)
							)
						)
						else
						(
							bLastMorphUsed = false
						)
						iIndex += 1
					)
					
					if aObjectsNonZeroMorphs.count > 0 then
					(
						append aResult #(sObject,aObjectsNonZeroMorphs)
					)
				)
			)
		)
		
		aResult
	),
--### MORPH SETUP CHECK END ###
	
--### BONE SCALE CHECK BEGIN ###
	fn fnGetBonesWithScaleFromHierarchies aExportNodes =
	(--returns bones that have a scale != 100% applied to them
		local aChildren = #()
		local aBones = #()
		local aResult = #()
		for each in aExportNodes do
		(--get children on export nodes
			aChildren += cryMaxTools.getChildren each
		)
		for each in aChildren do
		(--get skinned bones from children
			aBones += cryMaxTools.export.fnGetSkinnedHierarchiesFromNode each
		)
		
		makeUniqueArray aBones
		
		for each in aBones do
		(
			local p3ScaleOffset = (if each.parent == undefined then each.transform.scale else (each.transform.scale * (inverse each.parent.transform).scale)) - [1,1,1] --get scale relative to if present, else world
			if (p3ScaleOffset.x*p3ScaleOffset.x + p3ScaleOffset.y*p3ScaleOffset.y + p3ScaleOffset.z*p3ScaleOffset.z) > 0.0001 then
			(
				append aResult each
			)
		)
		
		aResult
	),
--### BONE SCALE CHECK BEGIN ###

	
--#########################################################
--CRYEXPORTNODE FUNCTIONS
--#########################################################
	
--### FILE NAME FUNCTIONS BEGIN ###
	fn fnGetExportFileName iIndex =
	(--returns the resulting filename of the cryExportNode
		substring cryMaxTools.export.object.aExportNodes[iIndex].name (cryMaxTools.export.object.sExportNodePrefix.count + 1) (cryMaxTools.export.object.aExportNodes[iIndex].name.count - cryMaxTools.export.object.sExportNodePrefix.count)
	),
	
	fn fnSetExportFileName iIndex sName =
	(--changes name of exportnode - triggers auto rename function to ensure unqiue naming of nodes
		cryMaxTools.export.object.aExportNodes[iIndex].name = cryMaxTools.export.object.sExportNodePrefix + sName
		cryMaxTools.export.object.fnMakeExportNodeNamesUnique sNode:cryMaxTools.export.object.aExportNodes[iIndex]
	),
	
	fn fnMakeExportNodeNamesUnique sNode: =
	(--makes names of all cryExportNodes unique - if sNode contains a valid cryExportNode, only this node will be renamed
		local aExportNodeNames = for nNode in cryMaxTools.export.object.aExportNodes collect nNode.name --collect names into an array
		aExportNodeNames = makeUniqueArray aExportNodeNames --find names that must be checked
		
		if sNode != unsupplied then
		(--node was supplied
			if cryMaxTools.export.object.fnIsExportNode sNode then
			(--supplied node is valid
				local aExportNodesWithCurrentName = getNodeByName sNode.name all:true --find all nodes with name of supplied node
				
				if aExportNodesWithCurrentName.count > 1 then
				(--found more than 1 --> then rename
					local sOldName = sNode.name
					
					sNode.name = uniqueName sNode.name
					
					--print ("renamed " + sOldName + " to " + sNode.name)
					cryMaxTools.export.log.fnUserNotification ("Duplicate names in cryExportNodes. Renamed " + sOldName + " to " + sNode.name + ".") iSeverity:2
				)
			)
		)
		else
		(--no valid node supplied, check all
			local aWarningMessages = #() --to tell user which nodes have been renamed
			local aSeverityLevels = #()
			
			for sName in aExportNodeNames do
			(--check all existing names
				local aExportNodesWithCurrentName = getNodeByName sName all:true --find all nodes with the current name
				
				if aExportNodesWithCurrentName.count > 1 then
				(--found more than 1 --> then rename
					
					for i = 2 to aExportNodesWithCurrentName.count do
					(
						aExportNodesWithCurrentName[i].name = uniqueName sName
						
						append aWarningMessages ("Renamed " + sName + " to " + aExportNodesWithCurrentName[i].name + ".")
						append aSeverityLevels 2
						--cryMaxTools.export.log.fnUserNotification ("Duplicate names in cryExportNodes. Renamed " + sName + " to " + aExportNodesWithCurrentName[i].name + ".") iSeverity:2
					)
				)
			)
			
			cryMaxTools.export.log.fnMultiUserNotification aWarningMessages aSeverityLevels "The following cryExportNodes have been renamed, due to duplicate names:"
		)
	),
--### FILE NAME FUNCTIONS END ###
	
	
--### FILE TYPE FUNCTIONS BEGIN ###
	fn fnGetExportFileType iIndex =
	(--returns filetype of exportnode or empty string
		local sFiletype = getUserProp cryMaxTools.export.object.aExportNodes[iIndex] "fileType"
		if sFiletype == undefined then sFiletype = ""
		sFiletype
	),
	
	fn fnSetExportFileType iIndex sType =
	(--sets the filetype of the exportnode or empty if unknown filetype
		sKeyValue = case sType of
		(
			"cgf": "cgf"
			"chr": "chr"
			"cga": "cga"
			"anm": "anm"
			default: ""
		)
		setUserProp cryMaxTools.export.object.aExportNodes[iIndex] "fileType" sKeyValue
	),
--### FILE TYPE FUNCTIONS END ###
	
	
--### DO NOT MERGE FLAG FUNCTIONS BEGIN ###
	fn fnGetDoNotMergeAllowed iIndex =
	(--returns true if supplied cryExportNode is allowed to have doNotMerge flag
		cryMaxTools.export.object.aExportNodes[iIndex].children.count < 2
	),
	
	fn fnGetDoNotMergeFlag iIndex =
	(--returns true if doNotMerge flag/string is present, false otherwise - assumes integer index of cryExportNode in aExportNodes as input
		cryMaxTools.export.fnGetUPDFlag cryMaxTools.export.object.aExportNodes[iIndex] "doNotMerge" --reuse general UDP function
	),
	
	fn fnSetDoNotMergeFlag iIndex bState =
	(--adds/removes doNotMerge flag/string of cryExportNode - assumes integer index of cryExportNode in aExportNodes, boolean as input
		if cryMaxTools.export.object.fnGetDoNotMergeAllowed iIndex then
		(
			cryMaxTools.export.fnSetUPDFlag cryMaxTools.export.object.aExportNodes[iIndex] "doNotMerge" bState --reuse general UDP function
		)
		else
		(
			cryMaxTools.export.fnSetUPDFlag cryMaxTools.export.object.aExportNodes[iIndex] "doNotMerge" false --not allowed with multiple children - would result in CGF with multiple roots
		)
	),
--### DO NOT MERGE FLAG FUNCTIONS END ###
	
	
--### STORE VERTEX COLORS FLAG FUNCTIONS BEGIN ###
	fn fnGetStoreVertexColorsFlag iIndex =
	(--returns true if doNotExport flag/string is present, false otherwise - assumes integer index of cryExportNode in aExportNodes as input
		cryMaxTools.export.fnGetUPDFlag cryMaxTools.export.object.aExportNodes[iIndex] "storeVertexColors" --reuse general UDP function
	),
	
	fn fnSetStoreVertexColorsFlag iIndex bState =
	(--adds/removes doNotExport flag/string of cryExportNode - assumes integer index of cryExportNode in aExportNodes, boolean as input
		cryMaxTools.export.fnSetUPDFlag cryMaxTools.export.object.aExportNodes[iIndex] "storeVertexColors" bState --reuse general UDP function
	),
--### STORE VERTEX COLORS FLAG FUNCTIONS END ###
	
	
--### MORPH THRESHOLD FUNCTIONS BEGIN ###
	fn fnDoesNodeHaveMorphs iIndex =
	(--returns true if any of the cryExportNodes children has a morph modifier, false otherwise
		local aChildren = cryMaxTools.export.object.aExportNodes[iIndex].children --collect all children of cryExportNode
		
		for each in aChildren do
		(--check all children for morphers
			if each.modifiers["morpher"] != undefined then
			(--morpher found - abort and return true
				return true
			)
		)
		
		false --if loop didn't find morpher, return false
	),
	
	fn fnGetMorphThreshold iIndex =
	(--gets morph threshold property of cryExportNode - if morph mod present and value not set, set default and then return it
		if cryMaxTools.export.object.fnDoesNodeHaveMorphs iIndex then
		(--morph modifier found, get property
			local fMorphThreshold = getUserProp cryMaxTools.export.object.aExportNodes[iIndex] "morphThreshold"
			
			if fMorphThreshold == undefined then
			(--if morph threshold not set, then set default value
				setUserProp cryMaxTools.export.object.aExportNodes[iIndex] "morphThreshold" cryMaxTools.export.object.fMorphThresholdDefault
				fMorphThreshold = cryMaxTools.export.object.fMorphThresholdDefault
			)
			
			fMorphThreshold
		)
		else
		(--no morph modifier found, delete property, return 0
			cryMaxTools.export.fnDeleteUserDefinedProperty cryMaxTools.export.object.aExportNodes[iIndex] "morphThreshold"
			
			0
		)
	),
	
	fn fnSetMorphThreshold iIndex fValue =
	(--sets morph threshold property of cryExportNode - delete property if no morph mods present
		if cryMaxTools.export.object.fnDoesNodeHaveMorphs iIndex then
		(--morph modifier found, set property
			setUserProp cryMaxTools.export.object.aExportNodes[iIndex] "morphThreshold" fValue
		)
		else
		(--no morph modifier found, delete property
			cryMaxTools.export.fnDeleteUserDefinedProperty cryMaxTools.export.object.aExportNodes[iIndex] "morphThreshold"
		)
	),
--### MORPH THRESHOLD FUNCTIONS END ###
	
	
--### EXPORT NODE LAYER FUNCTIONS BEGIN ###
	fn fnGetExportNodeLayer =
	(--returns export node layer, creates it if not present
		local sExportNodeLayer = LayerManager.getLayerFromName "cryExportLayer"
		
		if sExportNodeLayer == undefined then
		(
			sExportNodeLayer = LayerManager.newLayerFromName "cryExportLayer"
			sExportNodeLayer.isfrozen = true
			sExportNodeLayer.ishidden = true
		)
		
		sExportNodeLayer
	),
--### EXPORT NODE LAYER FUNCTIONS END ###
	
--### EXPORT NODE CREATION/DESTRUCTION FUNCTIONS BEGIN ###
	fn fnIsExportNode sNode =
	(--returns true if node is an exportnode
		matchPattern sNode.name pattern:(cryMaxTools.export.object.sExportNodePrefix + "*") ignoreCase:true
	),
	
	fn fnRemoveExportNode iIndex =
	(--removes the exportnode - assumes integer as input
		delete cryMaxTools.export.object.aExportNodes[iIndex]
		
		deleteItem cryMaxTools.export.object.aExportNodes iIndex
	),
	
	fn fnRemoveMultipleExportNodes aNodeSelection =
	(--removes multiple exportNodes - assumes bitArray as input
		local aExportNodesToRemove = for each in (aNodeSelection as array) where each <= cryMaxTools.export.object.aExportNodes.count collect cryMaxTools.export.object.aExportNodes[each] --collect actual references of nodes to be removed
		
		for each in aExportNodesToRemove do
		(
			deleteItem cryMaxTools.export.object.aExportNodes (findItem cryMaxTools.export.object.aExportNodes each)
			
			delete each
		)
	),
	
	fn fnSetExportNode sNode iIndex: =
	(--adds exportnode to node unless already present, returns node or undefined if node is parented to non-cryExportNode - if iIndex is specified, the node will be added to that cryExportNode instead of creating a new one
		if sNode.parent == undefined then
		(
			if iIndex == unsupplied then
			(--if no index supplied, make new cryExportNode
				sNewNode = dummy pos:sNode.transform.position name:(cryMaxTools.export.object.sExportNodePrefix + sNode.name)
				appendIfUnique aExportNodes sNewNode
				cryMaxTools.export.object.fnSetExportFileType (findItem cryMaxTools.export.object.aExportNodes sNewNode) "cgf"
				
				sNode.parent = sNewNode
				
				sExportNodeLayer = cryMaxTools.export.object.fnGetExportNodeLayer()
				sExportNodeLayer.addnode sNewNode
				
			)
			else
			(--if index specified, add node to that cryExportNode
				sNode.parent = cryMaxTools.export.object.aExportNodes[iIndex]
			)
			
			sNode.parent
		)
		else if (matchPattern sNode.parent.name pattern:(cryMaxTools.export.object.sExportNodePrefix + "*") ignoreCase:true) then
		(
			cryMaxTools.export.log.fnUserNotification (sNode.name + " has a cryExportNode already. Cannot reparent to cryExportNode.") iSeverity:2
			
			sNode.parent
		)
		else
		(
			cryMaxTools.export.log.fnUserNotification (sNode.name + " has a parent already. Cannot reparent to cryExportNode.") iSeverity:2
			
			undefined
		)
	),
	
	fn fnSetExportNodeForMultipleNodes aNodes iIndex: =
	(--adds all supplied nodes to a single cryExportNode using fnSetExportNode() function
		aValidNodes = for sNode in aNodes where not (cryMaxTools.export.object.fnIsExportNode sNode) collect sNode --collect nodes that are not cryExportNodes
		
		aNodesWithParent = #()
		aNodesWithOutParent = #()
		for sNode in aValidNodes do
		(--divide selection in objects that have a parent and those that don't
			if sNode.parent == undefined then append aNodesWithOutParent sNode
			else append aNodesWithParent sNode
		)
		
		for sNode in aNodesWithParent do
		(--process all nodes that have a parent already - will print error per obj that object cannot be reparented
			cryMaxTools.export.object.fnSetExportNode sNode
		)
		
		local sNewNode = undefined
		for i = 1 to aNodesWithOutParent.count do
		(--add all nodes without a parent to one exportnode
			if iIndex == unsupplied then
			(--if no index supplied, make new cryExportNode
				if i == 1 then
				(--if no index supplied then for first object create cryExportNode
					sNewNode = cryMaxTools.export.object.fnSetExportNode aNodesWithOutParent[1]
					
					if aNodesWithOutParent.count > 1 then
					(--if multiple objects were selected, rename to reflect count in name
						cryMaxTools.export.object.fnSetExportFileName (findItem cryMaxTools.export.object.aExportNodes sNewNode) (aNodesWithOutParent.count as string + "_objects")
					)
				)
				else
				(--additional objects get parented to same object
					cryMaxTools.export.object.fnSetExportNode aNodesWithOutParent[i] iIndex:(findItem cryMaxTools.export.object.aExportNodes sNewNode)
				)
			)
			else
			(--if index specified, add node to that cryExportNode
				cryMaxTools.export.object.fnSetExportNode aNodesWithOutParent[i] iIndex:iIndex
			)
		)
		
		sNewNode
	),
	
	fn fnDetachNodesFromExportNode iIndex aNodes =
	(--detaches the specified nodes from the cryExportNode, if possible
		local aExportNodesChildren = cryMaxTools.export.object.aExportNodes[iIndex].children
		local iNodesDetachedCount = 0
		
		for each in aNodes do
		(
			if (findItem aExportNodesChildren each) != 0 then
			(
				each.parent = undefined
				
				iNodesDetachedCount += 1
			)
		)
		
		cryMaxTools.export.log.fnUserNotification (iNodesDetachedCount as string + " nodes detached from " + cryMaxTools.export.object.aExportNodes[iIndex].name + "." + (fnGetExportFileType iIndex) + ".")
	),
--### EXPORT NODE CREATION/DESTRUCTION FUNCTIONS END ###
	

--#########################################################
--GENERAL FUNCTIONS
--#########################################################
	fn fnPrintStats =
	(--print some stats for debugging
		print ("aExportNodes: " + aExportNodes as string)
	),
	
	fn fnExport =
	(--performs object export - including pre- and post-export functions
		cryMaxTools.export.log.fnDebugMessage "OBJECT EXPORTER: Called to export all cryExportNodes."
		
		local aWarningMessages = #() --each element will be a line in the warning message display
		local aSeverityLevels = #()
		
		--### PRE EXPORT CHECKS BEGIN ###
		if bCheckMorphs then
		(--preform morph check if enabled
			local aMorphMessages = cryMaxTools.export.object.fnCheckNonZeroMorphs cryMaxTools.export.object.aExportNodes --performs non-zero morph check for all cryExportNode children and returns nodes where test failed
			
			if aMorphMessages.count > 0 then
			(--if morph check found problems - generate warning messages
				for i = 1 to aMorphMessages.count do
				(
					local sNonZeroMorphTargets = ""
					
					for j = 1 to aMorphMessages[i][2].count do
					(--generate non zero morph target string
						sNonZeroMorphTargets += ("\"" + aMorphMessages[i][2][j] + "\"")
						
						if j != aMorphMessages[i][2].count then
						(
							sNonZeroMorphTargets += ", "
						)
					)
						
					append aWarningMessages ("Object \"" + aMorphMessages[i][1].name + "\" has non zero morphs: " + sNonZeroMorphTargets + ". Export result may differ from 3dsmax state.")
					append aSeverityLevels 2
				)
			)
		)
		
		if bCheckMaterials then
		(--perform material check if enabled
			local aObjectsWithInvalidMaterials = cryMaxTools.export.material.fnGetObjectsWithInvalidMaterials cryMaxTools.export.object.aExportNodes
			
			if aObjectsWithInvalidMaterials.count > 0 then
			(--if material check found problems - generate warning messages
				append aWarningMessages "" --insert linebreak in warning messages
				append aSeverityLevels 0
				
				for i = 1 to aObjectsWithInvalidMaterials.count do
				(
					append aWarningMessages ("Object \"" + aObjectsWithInvalidMaterials[i].name + "\" has no standard material with a crytek shader applied. Export may fail.")
					append aSeverityLevels 2
				)
			)
		)
		
		if bCheckBoneScale then
		(--perform bone scale check if enabled
			local aBonesWithScale = cryMaxTools.export.object.fnGetBonesWithScaleFromHierarchies cryMaxTools.export.object.aExportNodes
			
			if aBonesWithScale.count > 0 then
			(--if bone scale check found problems - generate warning messages
				append aWarningMessages "" --insert linebreak in warning messages
				append aSeverityLevels 0
				
				for i = 1 to aBonesWithScale.count do
				(
					append aWarningMessages ("Bone \"" + aBonesWithScale[i].name + "\" has scale applied. Export result may change.")
					append aSeverityLevels 2
				)
			)
		)
		--### PRE EXPORT CHECKS END ###
		
		if aWarningMessages.count > 0 then
		(--if warnings occured, ask user if export shall be continued
			local sWarningsDisplayResult = cryMaxTools.export.log.fnMultiUserNotification aWarningMessages aSeverityLevels "The following Warnings have occured. Continue Export?" bYesNoBox:true
			
			if sWarningsDisplayResult == undefined or sWarningsDisplayResult == "yes" then
			(--if no message has been displayed or user chose to continue export, export
				if cryMaxTools.export.bReparentTwistBones then
				(
					local aReparentedTwistBones = cryMaxTools.export.fnReparentTwistBonesInHierarchies cryMaxTools.export.object.aExportNodes
					
					if aReparentedTwistBones.count != 0 then
					(
						local sTwistBonesNames = ""
						
						for i = 1 to aReparentedTwistBones.count do
						(--generate namestring
							append sTwistBonesNames ("\"" + aReparentedTwistBones[i].name + "\"")
							if i != aReparentedTwistBones.count then
							(
								append sTwistBonesNames ", "
							)
						)
						
						cryMaxTools.export.log.fnUserNotification ("The following biped twist bones have been reparented: " + sTwistBonesNames + ".")
					)
				)
				
				try
				(--catch plug-in crashes
					cryengine2.export.export()
				)
				catch
				(
					cryMaxTools.export.log.fnUserNotification ("Potential export plug-in crash. Please refer to \"" + getFilenameFile maxFileName + ".exportlog\" for more information.") iSeverity:3
				)
			)
			else
			(
				cryMaxTools.export.log.fnUserNotification "Export aborted by user."
			)
		)
		else
		(
			try
			(--catch plug-in crashes
				cryengine2.export.export()
			)
			catch
			(
				cryMaxTools.export.log.fnUserNotification ("Potential export plug-in crash. Please refer to \"" + getFilenameFile maxFileName + ".exportlog\" for more information.") iSeverity:3
			)
		)
	),
	
	fn fnExportSelection aSelectedExportNodes =
	(--performs test object export of only supplied export nodes
		cryMaxTools.export.log.fnDebugMessage "OBJECT EXPORTER: Called to test export selected cryExportNodes."
		
		aSelectedExportNodes *= #{1..cryMaxTools.export.object.aExportNodes.count} --cut off bits that do not correspond to a node in the exportNodeList
		if aSelectedExportNodes.numberSet != 0 then
		(--at least one node selected
			cryengine2.export.export aSelectedExportNodes
		)
		else
		(
			--cryMaxTools.export.object.fnUserNotification "Error! No valid selection of cryExportNodes. Aborting export."
			cryMaxTools.export.log.fnUserNotification "No valid selection of cryExportNodes. Nothing to export. Aborting." iSeverity:3
		)
	),
	
	fn fnUpdate =
	(--finds all exportnodes in the scene, make layer if undefined ... will be called in a postfileopen callback!
		cryMaxTools.export.object.sExportNodeLayer = fnGetExportNodeLayer() --get/create cryExportLayer
		
		cryMaxTools.export.object.aExportNodes = for nNode in $objects where (matchPattern nNode.name pattern:(sExportNodePrefix + "*") ignoreCase:true) collect nNode --collect all cryExportNodes from the scene into an array
		
		for nNode in aExportNodes do
		(--move all cryExportNodes to cryExportNode layer
			cryMaxTools.export.object.sExportNodeLayer.addnode nNode
		)
		
		if bMakeExportNodeNamesUnique then
		(--make names unique when feature turned on
			fnMakeExportNodeNamesUnique()
		)
	)
)


--###########################################################################################
--ANIMATION EXPORTER STRUCT
--ANIMATION EXPORTER STRUCT   ######################################################################
--ANIMATION EXPORTER STRUCT
--###########################################################################################

struct rangeStruct (animName = "", start = 0.0, end = 0.0, rootNodes = (#()), type = "caf")

struct cryMaxToolsAnimationExportStruct
(--animation exporter struct
--#########################################################
--GLOBAL VARIABLES
--#########################################################
	--sAnimExportNode = undefined, --references node that contains export data
	animRanges = #(),
	exportType = 1,	--// 1 = CAF; 		2 = ANM
	aRangeNames = #(), --contains range names
	aRangeBegins = #(), --contains range begins
	aRangeEnds = #(), --contains range ends
	aSubHierarchies = #(), --contains root nodes of hierarchies to export
	sAnimNamePrefix = "", --contains custom filename prefix
	sAnimExportPath = "", --contains custom export path

	
--#########################################################
--SHARED FUNCTIONS
--#########################################################
	fn fnGetExportNode =
	(
		local sAnimExportNode = getNodeByName "cryAnimationExportNode"
		if sAnimExportNode == undefined then
		(-- create node if doesn't exist
			sAnimExportNode = dummy pos:[0,0,0] name:"cryAnimationExportNode"
			
			local sExportNodeLayer = cryMaxTools.export.object.fnGetExportNodeLayer()
			
			sExportNodeLayer.addnode sAnimExportNode
		)
		
		return sAnimExportNode
	),
	
	
	fn fnCheckBoneScale sBone =
	(--checks bone's hierarchy for scaled bones
		local aScaledBones = #()
		local aChildren = cryMaxTools.getChildren sBone
		for each in aChildren do
		(
			try
			(
				local p3Scale = each.transform.scale - [1,1,1]
				if ((abs p3Scale.x) + (abs p3Scale.y) + (abs p3Scale.z)) > 0.0001 then
				(
					append aScaledBones each.name
				)
			)
			catch()
			try
			(
				if (numKeys each.scale.controller) > 0 then
				(
					for i = 1 to (numKeys each.scale.controller) do
					(
						local p3Scale = each.scale.controller.keys[i].value - [1,1,1]
						if ((abs p3Scale.x) + (abs p3Scale.y) + (abs p3Scale.z)) > 0.0001 then
						(
							appendIfUnique aScaledBones each.name
						)
					)
				)
			)
			catch()
		)
		
		if aScaledBones.count > 0 then
		(
			return aScaledBones
		)
		else
		(
			return true
		)
	),
	
	fn fnGetRootFromSkin sNode =
	(--returns the root bone of the hierarchy that node is skinned to or false
		try
		(
			local sSkin = sNode.crySkin
		)
		catch
		(
			local sSkin = undefined
		)
		
		if sSKin != undefined and (skinOps.getNumberBones sSKin) > 0 then
		(
			local sCurrentMode = getCommandPanelTaskMode()
			setCommandPanelTaskMode #modify
			modPanel.setCurrentObject sSkin
			
			local sRoot = cryMaxTools.findRoot (getNodeByName (skinOps.getBoneName sSKin 1 0))
			
			setCommandPanelTaskMode sCurrentMode
			
			return sRoot
		)
		else
		(
			return undefined
		)
	),
	

--#########################################################
--RANGE FUNCTIONS
--#########################################################
	
	fn fnSetRanges rangeArray =
	(
		local sAnimExportNode = fnGetExportNode()
		local nameString = "", startString = "", endString = "", rootNodeString = "", typeString = ""
		
		if rangeArray.count > 0 then
		(	
			for i = 1 to rangeArray.count do
			(
				nameString += rangeArray[i].animName + ";"
				startString += rangeArray[i].start as String + ";"
				endString += rangeArray[i].end as String + ";"
				
				local tempRootNodeString = "|"
				for d = 1 to rangeArray[i].rootNodes.count do
					tempRootNodeString += rangeArray[i].rootNodes[d] + "|"
				
				rootNodeString += tempRootNodeString + ";"
				typeString += rangeArray[i].type + ";"
			)
		)
		
		setUserProp sAnimExportNode "names" nameString
		setUserProp sAnimExportNode "begins" startString
		setUserProp sAnimExportNode "ends" endString
		setUserProp sAnimExportNode "rootNodes" rootNodeString
		setUserProp sAnimExportNode "type" typeString
		
		animRanges = rangeArray
	),
	

	
	
	fn fnGetRanges =
	(
		animRanges = #()
		
		local sAnimExportNode = fnGetExportNode()
		
		if (getUserProp sAnimExportNode "names") != undefined then
		(
			try
			(
				local animNameFilter = filterString (getUserProp sAnimExportNode "names") ";"
				local startFilter = filterString (getUserProp sAnimExportNode "begins") ";"
				local endFilter = filterString (getUserProp sAnimExportNode "ends") ";"
				local rootNodeFilter = filterString (getUserProp sAnimExportNode "rootNodes") ";"
				local typeFilter = filterString (getUserProp sAnimExportNode "type") ";"
			)
			catch
			(
				print "Inconsistency in cryAnimationExportNode! Check correct saving of File!"
				return false
			)
			
			
			if not (animNameFilter.count == startFilter.count and animNameFilter.count == endFilter.count) then
			(
				print "Inconsistency in cryAnimationExportNode! Check correct saving of File!"
				return false
			)
			
			for i = 1 to animNameFilter.count do
			(
				local tempStart = execute startFilter[i]
				local tempEnd = execute endFilter[i]
				local tempRootNodes = #()
				local tempRootNodeFilter = filterString rootNodeFilter[i] "|"
				local type = typeFilter[i]
				
				if tempStart == undefined then
					tempStart = "start"
				
				if tempEnd == undefined then
					tempEnd = "end"
				
				for d = 1 to tempRootNodeFilter.count do
				(
					local tempRootNode = getNodeByName tempRootNodeFilter[d]
					if tempRootNode != undefined then
						append tempRootNodes tempRootNode.name
				)
				
				append animRanges (rangeStruct animName:animNameFilter[i] start:tempStart end:tempEnd rootNodes:tempRootNodes type:type)
			)
		)
	),

	
--#########################################################
--EXPORT PATH FUNCTIONS
--#########################################################
	fn fnSetExportSettings setVar dialog:false = 
	(--set the custom export path
		local sAnimExportNode = fnGetExportNode()
		
		if classOf setVar == String then
		(
			if dialog == true then
			(
				if sAnimExportPath != "" then
				(
					local sInitialDir = sAnimExportPath
				)
				else
				(
					local sInitialDir = crymaxtools.basic.vars.buildpath + "\\Game\\Animations"
				)
				local sPath = getSavePath caption:"Please select the animation export path!" initialDir:sInitialDir
				if sPath != undefined then
					sAnimExportPath = sPath
			)
			else
			(--sets path to paramter of function
				sAnimExportPath = setVar
			)
			
			if sAnimExportPath != "" then
				if sAnimExportPath[sAnimExportPath.count] != "\\" then
					sAnimExportPath += "\\"
				
			setUserProp sAnimExportNode "path" sAnimExportPath
		)
		
		if classOf setVar == Integer then
		(
			exportType = setVar
			
			setUserProp sAnimExportNode "exportType" (exportType as String)
		)
	),
	
	fn fnGetExportSettings =
	(--get export path from node
		local sAnimExportNode = fnGetExportNode()
		
		sAnimExportPath = ""
		
		if (local tempPath = (getUserProp sAnimExportNode "path")) != undefined then
			sAnimExportPath = tempPath
			
		if (local tempType = (getUserProp sAnimExportNode "exportType")) != undefined then
			exportType = tempType
	),
	
	
--#########################################################
--GENERAL FUNCTIONS
--#########################################################
	fn fnUpdateExporterData =
	(--get data from anim export node
		cryMaxTools.export.anim.fnGetRanges()
		--cryMaxTools.export.anim.fnGetSubHierarchies()
		--cryMaxTools.export.anim.fnGetExportNamePrefix()
		cryMaxTools.export.anim.fnGetExportSettings()
	),
	
	fnUpdate = fnUpdateExporterData, --for consistency, update function has same name in obj and anim exporter
	
	
	
	
	fn fnExport bAbortOnError: useSelection:undefined customOpenPath:"" =
	(--performs animation export
		local aErrors = #("Error: The following bones have a scale applied:\n","")
		local fileExportPath = ""
		
		local safeAnimRange = animationRange
		local exportError = #()
		
		local startIndex = 1
		if useSelection == undefined and animRanges.count > 1 then
			startIndex = 2
		
		
	
		
		
		
		for i = startIndex to animRanges.count do
		(
			if useSelection != undefined then
				if findItem useSelection i == 0 then
					continue
				
			if animRanges[i].rootNodes.count == 0 then
			(
				append exportError (#(i, "No Bones to Export!"))
				continue
			)
			
			local rangeBones = #()
			
			for d = 1 to animRanges[i].rootNodes.count do
			(
				if (local tempBone = getNodeByName animRanges[i].rootNodes[d]) != undefined then
					append rangeBones tempBone
				else
					append exportError (#(i, (animRanges[i].rootNodes[d] + " Not found!")))
			)
			
			
			local range = animationRange
			local abort = false
				
			if i == 1 then
			(
				local initialDir = crymaxtools.basic.vars.buildpath + "\\Game\\Animations"
				if customOpenPath != "" then
					initialDir = customOpenPath
				
				local types = "Character Animation File (*.caf)|*.caf|Geometry Animation File (*.anm)|*.anm"
				
				local newPath = getSaveFileName caption:"Please select the animation export path!" filename:initialDir types:types
				if newPath != undefined then
					fileExportPath = newPath
				else
					abort = true
			)
			else
			(
				local extension = "." + animRanges[i].type
				
				
				fileExportPath = sAnimExportPath + animRanges[i].animName + extension
				range = interval animRanges[i].start animRanges[i].end
			)
			
			if abort == false then
			(
				animationRange = range
				print range
				
				
				if cryMaxTools.export.bReparentTwistBones then
				(
					local aReparentedTwistBones = cryMaxTools.export.fnReparentTwistBonesInHierarchies rangeBones
				 
					if aReparentedTwistBones.count != 0 then
					(
						local sTwistBonesNames = ""
					  
						for i = 1 to aReparentedTwistBones.count do
						(--generate namestring
							append sTwistBonesNames ("\"" + aReparentedTwistBones[i].name + "\"")
							if i != aReparentedTwistBones.count then
							(
								append sTwistBonesNames ", "
							)
						)
				  
						cryMaxTools.export.log.fnUserNotification ("The following biped twist bones have been reparented: " + sTwistBonesNames + ".")
					)
				)
				
				
				
				if extension == ".caf" or extension == ".anm" then
				(
					if cryMaxTools.anim.vars.useColladaExporter == true then
					(
						try
							cryengine2.export.export_anim fileExportPath rangeBones
						catch
						(
							print "No Collada Exporter found, using normal cryExport"
							useCollada = false
						)
					)
					else
					(
						try
						(
							UtilityPanel.OpenUtility CryEngine2_Exporter
							
							if extension == ".caf" then
							(
								try
									csexport.export.set_bone_list rangeBones
								catch()
								
								csexport.export.export_anim fileExportPath
							)
							else if extension == ".anm" then
							(
								saveMaxFile (sAnimExportPath + animRanges[i].animName + ".max")
								
								try
									csexport.export.set_node_list rangeBones
								catch()
								
								csexport.export.export_nodes()
							)
						)
						catch(print ("Failed to export: " + fileExportPath))
					)
				)
				else
				(
					local saveSel = getCurrentSelection()
					select rangeBones
					
					cryMaxTools.anim.functions.UI.saveAnimCheck fileExportPath
					
					clearSelection()
					select saveSel
				)
			)
		)
		
		
		animationRange = safeAnimRange
		
		return fileExportPath
	)
)


--###########################################################################################
--MATERIAL EXPORTER STRUCT
--MATERIAL EXPORTER STRUCT   ########################################################################
--MATERIAL EXPORTER STRUCT
--###########################################################################################
struct cryMaxToolsMaterialExportStruct
(--material exporter/importer struct
--#########################################################
--GLOBAL VARIABLES
--#########################################################
	sMaterialExportFailMessage = ("Material % is not a standard material with a crytek shader. Export failed."), --format with material's name to stringStream
	sMaterialExportSuccessMessage = ("Material file \"%.mtl\" successfully written."), --format with material's name to stringStream
	
--#########################################################
--MATERIAL EXPORT FUNCTIONS
--#########################################################
	fn fnGetMaterialsFromObjects aNodes =
	(--returns an array of materials, used in all children of the provided nodes - assumes array of nodes as input
		local aMaterials = #()
		
		for each in aNodes do
		(--collect materials from selected nodes
			local aExportNodeChildren = cryMaxTools.getChildren each
			
			for each in aExportNodeChildren do
			(--export materials of all children
				if each.material != undefined then
				(
					appendIfUnique aMaterials each.material
				)
			)
		)
		
		aMaterials
	),
	
	fn fnExportMaterialsFromExportNodes aNodeSelection =
	(--exports all materials used by the supplied cryExportNodes -- assumes bitArray of exportnode selection as input
		cryMaxTools.export.log.fnDebugMessage ("MATERIAL EXPORTER: Called to export materials of selected cryExportNodes.")
		
		if aNodeSelection.numberSet != 0 then
		(
			local aNodes = for each in (aNodeSelection as array) where each <= cryMaxTools.export.object.aExportNodes.count collect cryMaxTools.export.object.aExportNodes[each] --convert selection to array of nodes
			local aMaterialsFromObjects = cryMaxTools.export.material.fnGetMaterialsFromObjects aNodes --collect materials from objects
		
			local aUserMessages = #(aMaterialsFromObjects.count as string + " materials found in selected export nodes.")
			local aSeverityLevels = #(1)
			
			for each in aMaterialsFromObjects do
			(--export valid materials
				if cryMaxTools.export.material.fnExportMaterial each bSilent:true then
				(--if material exported successfully, append positive user feedback
					local ssUserMessage = "" as stringStream
					format cryMaxTools.export.material.sMaterialExportSuccessMessage each.name to:ssUserMessage
					append aUserMessages (ssUserMessage as string)
					append aSeverityLevels 1
				)
				else
				(--append negative user feedback
					local ssUserMessage = "" as stringStream
					format cryMaxTools.export.material.sMaterialExportFailMessage ("\"" + each.name + "\"") to:ssUserMessage
					append aUserMessages (ssUserMessage as string)
					append aSeverityLevels 3
				)
			)
			
			cryMaxTools.export.log.fnMultiUserNotification aUserMessages aSeverityLevels "Material Exporter Messages:"
		)
		else
		(
			cryMaxTools.export.log.fnUserNotification "No object exporter selection to export materials from."
		)
	),

	fn fnExportMaterial sMaterial bSilent:false =
	(--exports material to .mtl file - non-cryShader materials will cause error - returns true on success, false otherwise
		local sMaterialName = try ("\"" + sMaterial.name + "\"") catch "undefined"
		cryMaxTools.export.log.fnDebugMessage ("MATERIAL EXPORTER: Called to export material: " + sMaterialName + ".")
		
		local aInvalidMaterials = cryMaxTools.export.material.fnGetInvalidMaterialsFromMaterial sMaterial
		
		if aInvalidMaterials.count == 0 then 
		(--proceed if material is valid
			dotNet.loadAssembly "system.xml" --load dotNet XML assembly 
			
			local xmlMaterialFile = dotNetObject "system.xml.xmlDocument" --create XML document
			
			if classOf sMaterial == multiMaterial then
			(--create multi material
				--create multi material root
				local sXMLRoot = xmlMaterialFile.createElement "Material"
				sXMLRoot.setAttribute "Name" sMaterial.name
				sXMLRoot.setAttribute "MtlFlags" "524544"
				xmlMaterialFile.appendChild sXMLRoot
				
				--create submaterials element
				local sSubMaterials = xmlMaterialFile.createElement "SubMaterials"
				
				for i = 1 to sMaterial.count do
				(--get colors and textures from all submaterials
					local sSubMaterial = cryMaxTools.export.material.fnGenerateXMLMaterialBlock sMaterial[i] xmlMaterialFile
					
					sSubMaterials.appendChild sSubMaterial
				)
				
				sXMLRoot.appendChild sSubMaterials
				
				xmlMaterialFile.save (maxFilePath + (sMaterial.name) + ".mtl")
				
				if not bSilent then
				(
					local ssUserMessage = "" as stringStream
					format cryMaxTools.export.material.sMaterialExportSuccessMessage sMaterial.name to:ssUserMessage
					cryMaxTools.export.log.fnUserNotification (ssUserMessage as string) --user notification
				)
			)
			else
			(--create simple material
				--create single material root
				local sXMLRoot = cryMaxTools.export.material.fnGenerateXMLMaterialBlock sMaterial xmlMaterialFile
				
				xmlMaterialFile.appendChild sXMLRoot
				
				xmlMaterialFile.save (maxFilePath + (sMaterial.name) + ".mtl")
				
				if not bSilent then
				(
					local ssUserMessage = "" as stringStream
					format cryMaxTools.export.material.sMaterialExportSuccessMessage sMaterial.name to:ssUserMessage
					cryMaxTools.export.log.fnUserNotification (ssUserMessage as string) --user notification
				)
			)
			
			true
		)
		else
		(--material not valid, notify user
			if not bSilent then
			(
				local ssUserMessage = "" as stringStream
				
				for each in aInvalidMaterials do
				(
					format cryMaxTools.export.material.sMaterialExportFailMessage ("\"" + each.name + "\"") to:ssUserMessage
				)
				
				cryMaxTools.export.log.fnUserNotification (ssUserMessage as string) iSeverity:3 --user notification
			)
			
			false
		)
	),
	
	fn fnGenerateXMLMaterialBlock sMaterial xmlMaterialFile bWriteTexturePaths:true =
	(--generates a block for the MTL containing one material - assumes 3dsmax Standard material, dotNet xmlDocument instance as input
		local sNewMaterial = xmlMaterialFile.createElement "Material"
		sNewMaterial.setAttribute "Name" sMaterial.name
		if sMaterial.surfaceName == "Physical Proxy (NoDraw)" then
		(--set properties for nodraw material
			sNewMaterial.setAttribute "MtlFlags" "1152"
			sNewMaterial.setAttribute "Shader" "Nodraw"
			sNewMaterial.setAttribute "GenMask" "0"
		)
		else
		(--set properties for standard illum shader
			sNewMaterial.setAttribute "MtlFlags" "524416"
			sNewMaterial.setAttribute "Shader" "Illum"
			sNewMaterial.setAttribute "GenMask" "100000000"
		)
		--sNewMaterial.setAttribute "SurfaceType" ""
		--sNewMaterial.setAttribute "MatTemplate" ""
		sNewMaterial.setAttribute "Diffuse" ((sMaterial.diffuse.red/255) as string + "," + (sMaterial.diffuse.green/255) as string + "," + (sMaterial.diffuse.blue/255) as string)
		sNewMaterial.setAttribute "Specular" ((sMaterial.specular.red/255) as string + "," + (sMaterial.specular.green/255) as string + "," + (sMaterial.specular.blue/255) as string)
		sNewMaterial.setAttribute "Emissive" ((sMaterial.selfIllumAmount*.01) as string + "," + (sMaterial.selfIllumAmount*.01) as string + "," + (sMaterial.selfIllumAmount*.01) as string)
		sNewMaterial.setAttribute "Shininess" (sMaterial.glossiness as string)
		sNewMaterial.setAttribute "Opacity" ((sMaterial.opacity*.01) as string)
		
		if bWriteTexturePaths then
		(--write texture paths to MTL file
			local sTextures = xmlMaterialFile.createElement "Textures"
			
			if classOf sMaterial.diffuseMap == bitmapTexture then
			(--if map is a texture
				local sDiffuseTexturePath = undefined
				
				try
				(
					sDiffuseTexturePath = sMaterial.diffuseMap.fileName
				)
				catch
				(
					sDiffuseTexturePath = ""
				)
				
				if matchPattern sDiffuseTexturePath pattern:(cryMaxTools.basic.vars.buildPath + "\\game*") then
				(--texture is in game folder structure already - extract path relative to build path
					sDiffuseTexturePath = substring sDiffuseTexturePath (cryMaxTools.basic.vars.buildPath + "\\game\\").count (sDiffuseTexturePath.count - cryMaxTools.basic.vars.buildPath.count - 5 )
				)
				else if matchPattern sDiffuseTexturePath pattern:(maxFilePath + "*") then
				(--texture is in a subfolder of object - extract relative path
					sDiffuseTexturePath = substring sDiffuseTexturePath (maxFilePath.count + 1) (sDiffuseTexturePath.count - maxFilePath.count)
				)
				else
				(
					sDiffuseTexturePath = undefined
				)
				if sDiffuseTexturePath != undefined then
				(
					local sDiffuseTexture = xmlMaterialFile.createElement "Texture"
					sDiffuseTexture.setAttribute "Map" "Diffuse"
					sDiffuseTexture.setAttribute "File" sDiffuseTexturePath
					sTextures.appendChild sDiffuseTexture
				)
			)
			
			if classOf sMaterial.specularMap == bitmapTexture then
			(--if map is a texture
				local sSpecularTexturePath = sMaterial.specularMap.fileName
				if matchPattern sSpecularTexturePath pattern:(cryMaxTools.basic.vars.buildPath + "\\game*") then
				(--texture is in game folder structure already - extract path relative to build path
					sSpecularTexturePath = substring sSpecularTexturePath (cryMaxTools.basic.vars.buildPath + "\\game\\").count (sSpecularTexturePath.count - cryMaxTools.basic.vars.buildPath.count - 5 )
				)
				else if matchPattern sSpecularTexturePath pattern:(maxFilePath + "*") then
				(--texture is in a subfolder of object - extract relative path
					sSpecularTexturePath = substring sSpecularTexturePath (maxFilePath.count + 1) (sSpecularTexturePath.count - maxFilePath.count)
				)
				else
				(
					sSpecularTexturePath = undefined
				)
				if sSpecularTexturePath != undefined then
				(
					local sSpecularTexture = xmlMaterialFile.createElement "Texture"
					sSpecularTexture.setAttribute "Map" "Specular"
					sSpecularTexture.setAttribute "File" sSpecularTexturePath
					sTextures.appendChild sSpecularTexture
					--sSpecularTexture.appendChild (xmlMaterialFile.createElement "TexMod")
				)
			)
			
			if classOf sMaterial.bumpMap == normal_bump and classOf sMaterial.bumpMap.normal_map == bitmapTexture then
			(--if normalbump is used
				local sBumpTexturePath = sMaterial.bumpMap.normal_map.fileName
			)
			else if classOf sMaterial.bumpMap == bitmapTexture then
			(
				local sBumpTexturePath = sMaterial.bumpMap.fileName
			)
			if sBumpTexturePath != undefined then
			(
				if matchPattern sBumpTexturePath pattern:(cryMaxTools.basic.vars.buildPath + "\\game*") then
				(--texture is in game folder structure already - extract path relative to build path
					sBumpTexturePath = substring sBumpTexturePath (cryMaxTools.basic.vars.buildPath + "\\game\\").count (sBumpTexturePath.count - cryMaxTools.basic.vars.buildPath.count - 5 )
				)
				else if matchPattern sBumpTexturePath pattern:(maxFilePath + "*") then
				(--texture is in a subfolder of object - extract relative path
					sBumpTexturePath = substring sBumpTexturePath (maxFilePath.count + 1) (sBumpTexturePath.count - maxFilePath.count)
				)
				else
				(
					sBumpTexturePath = undefined
				)
				if sBumpTexturePath != undefined then
				(
					local sBumpTexture = xmlMaterialFile.createElement "Texture"
					sBumpTexture.setAttribute "Map" "Bumpmap"
					sBumpTexture.setAttribute "File" sBumpTexturePath
					sTextures.appendChild sBumpTexture
				)
			)
			
			sNewMaterial.appendChild sTextures
		)
		
		local sPublicParams = xmlMaterialFile.createElement "PublicParams"
		sPublicParams.setAttribute "AmbientMultiplier" "1"
		sNewMaterial.appendChild sPublicParams
		
		sNewMaterial --return result
	),
	
	
--#########################################################
--MATERIAL IMPORT FUNCTIONS
--#########################################################
	fn fnImportMaterial sFile: =
	(--import .mtl file to material
		dotNet.loadAssembly "system.xml"
		
		if sFile == unsupplied then
		(
			sFile = getOpenFileName caption:"Please specify material file!" fileName:maxFilePath types:"Crysis Material File (*.mtl)|*.mtl"
		)
		
		if sFile != undefined then
		(--load material file
			local xmlMaterialFile = dotNetObject "system.xml.xmlDocument"
			xmlMaterialFile.load sFile
			
			--load root
			local sXMLRoot = xmlMaterialFile.documentElement
			if sXMLRoot != undefined then
			(
				max mtledit
				local sMaterial = medit.GetCurMtl()
				sMaterial.name = getFilenameFile sFile
				
				if sXMLRoot.ChildNodes.itemOf[0] != undefined and sXMLRoot.ChildNodes.itemOf[0].name == "SubMaterials" then
				(
					sMaterial = multimaterial numSubs:sXMLRoot.ChildNodes.itemOf[0].ChildNodes.count
					sMaterial.name = getFilenameFile sFile
					setMeditMaterial (medit.GetActiveMtlSlot()) sMaterial
					
					for i = 0 to sXMLRoot.ChildNodes.itemOf[0].ChildNodes.count - 1 do
					(
						local sSubMaterial = sXMLRoot.ChildNodes.itemOf[0].ChildNodes.itemOf[i]
						
						sMaterial.materialList[i+1].name = (sSubMaterial.getAttributeNode "Name").value
						sMaterial.materialList[i+1].shaderType = 2
						
						local aTempColor = filterString (sSubMaterial.getAttributeNode "Diffuse").value ","
						sMaterial.materialList[i+1].diffuse = color (aTempColor[1] as float *255) (aTempColor[2] as float *255) (aTempColor[3] as float *255)
						
						local aTempColor = filterString (sSubMaterial.getAttributeNode "Specular").value ","
						sMaterial.materialList[i+1].specular = color (aTempColor[1] as float *255) (aTempColor[2] as float *255) (aTempColor[3] as float *255)
						sMaterial.materialList[i+1].specularLevel= 100
						
						sMaterial.materialList[i+1].opacity = ((sSubMaterial.getAttributeNode "Opacity").value as float)*100
						sMaterial.materialList[i+1].alphaBlend = (sMaterial.materialList[i+1].opacity != 100)
						
						for j = 0 to sSubMaterial.childNodes.itemOf[0].childNodes.count-1 do
						(--load texture filenames
							local sTexture = sSubMaterial.childNodes.itemOf[0].childNodes.itemOf[j]
							print (sTexture.getAttributeNode "Map").value
							case (sTexture.getAttributeNode "Map").value of
							(
								"Diffuse":
								(
									sMaterial.materialList[i+1].diffuseMapEnable = on
									sMaterial.materialList[i+1].diffuseMap = Bitmaptexture fileName:(cryMaxTools.basic.vars.buildPath + "\\Game\\" + (sTexture.getAttributeNode "File").value)
								)
								"Bumpmap":
								(
									sMaterial.materialList[i+1].normalMapEnable = on
									sMaterial.materialList[i+1].normalMap = Bitmaptexture fileName:(cryMaxTools.basic.vars.buildPath + "\\Game\\" + (sTexture.getAttributeNode "File").value)
								)
								"Specular":
								(
									sMaterial.materialList[i+1].specularMapEnable = on
									sMaterial.materialList[i+1].specularMap = Bitmaptexture fileName:(cryMaxTools.basic.vars.buildPath + "\\Game\\" + (sTexture.getAttributeNode "File").value)
								)
								"SubSurface":
								(
									sMaterial.materialList[i+1].subsurfaceMapEnable = on
									sMaterial.materialList[i+1].subsurfaceMap = Bitmaptexture fileName:(cryMaxTools.basic.vars.buildPath + "\\Game\\" + (sTexture.getAttributeNode "File").value)
								)
							)
						)
					)
				)
			)
		)
	),
	
--#########################################################
--MATERIAL SETUP CHECK FUNCTIONS
--#########################################################
	fn fnIsMaterialValid sMaterial =
	(--returns true if material uses standard material crytek shader, false otherwise
		classOf sMaterial == Standardmaterial and sMaterial.shadername == "Crytek Shader"
	),
	
	fn fnCheckMaterial sMaterial =
	(--returns true if material and submaterials are valid, false otherwise - assumes 3dsmax material as input
		if classOf sMaterial == multiMaterial then
		(--multi material
			local aValidMaterials = #{}
			
			for each in sMaterial do
			(--check each sub material
				aValidMaterials[aValidMaterials.count + 1] = cryMaxTools.export.material.fnIsMaterialValid each
			)
			
			aValidMaterials.count == aValidMaterials.numberSet --is true if all bits set
		)
		else
		(--single material
			local bMaterialIsValid = cryMaxTools.export.material.fnIsMaterialValid sMaterial
			
			bMaterialIsValid --return result
		)
	),
	
	fn fnGetInvalidMaterialsFromMaterial sMaterial =
	(--returns array containing all invalid submaterials or the supplied material if invalid
		local aInvalidMaterials = #()
		
		if classOf sMaterial == multiMaterial then
		(--multi material
			for each in sMaterial do
			(--check each sub material
				if not (cryMaxTools.export.material.fnIsMaterialValid each) then
				(
					append aInvalidMaterials each
				)
			)
		)
		else if not (cryMaxTools.export.material.fnIsMaterialValid sMaterial) then
		(--single material
			append aInvalidMaterials sMaterial
		)
		
		aInvalidMaterials
	),
	
	fn fnGetInvalidMaterialsFromHierarchy aNodes =
	(--returns array containing all invalid materials of children of supplied exportnodes
		local aInvalidMaterials = #()
		local aMaterialsToCheck = cryMaxTools.export.material.fnGetMaterialsFromObjects aNodes
		
		
		for each in aMaterialsToCheck do
		(--collect materials from selected nodes
			if not (cryMaxTools.export.material.fnCheckMaterial each) then
			(--set bit in aResult if material is valid
				appendIfUnique aInvalidMaterials each
			)
		)
		
		aInvalidMaterials
	),
	
	fn fnGetObjectsWithInvalidMaterials aNodes bIncludeSkinnedBones:true =
	(--returns array containing all children of supplied exportnodes with invalid materials
		local aObjectsWithInvalidMaterials = #()
		
		for each in aNodes do
		(--collect materials from selected nodes
			local aExportNodeChildren = cryMaxTools.getChildren each
			
			if bIncludeSkinnedBones then
			(--include bones in material check
				local aBones = #()
				for each in aExportNodeChildren do 
				(
					aBones += cryMaxTools.export.fnGetSkinnedBonesFromNode each
				)
				
				aExportNodeChildren += aBones
				
				makeUniqueArray aExportNodeChildren
			)
			
			for i = 1 to aExportNodeChildren.count do
			(--export materials of all children
				if not (cryMaxTools.export.material.fnCheckMaterial aExportNodeChildren[i].material) then
				(--set bit in aObjectsWithInvalidMaterials if material is invalid
					append aObjectsWithInvalidMaterials aExportNodeChildren[i]
				)
			)
		)
		
		aObjectsWithInvalidMaterials
	)
)

--###########################################################################################
--EXPORTER LOG MESSAGE STRUCT
--EXPORTER LOG MESSAGE STRUCT   #####################################################################
--EXPORTER LOG MESSAGE STRUCT
--###########################################################################################
struct cryMaxToolsExporterLogMessageStruct
(--exporter logging and messaging struct - severity levels: 1 = simple user notification, 2 = warnings when behaviour of command changes, 3 = critical error that prevents successful action
	sLogFileNameFileType = "scriptlog", --filetype for log file
	aSeverityPrefixes = #("D","N","W","E"), --used to mark different levels of severity in the log file - first element is for debag messages that are never shown to the user - IMPORTANT: individual prefixes must be unqiue!!! - no ":" allowed, will be added automatically
	aSeverityNames = #("Notice.","Warning!","Error!"), --names used when displaying severity level to the user
	aSeverityIcons = #( --icons used when displaying severity level to the user
								openBitmap (cryMaxTools.basic.vars.toolsPath + "Icons\\cryExportLogSeverityLevel1_32.bmp"),
								openBitmap (cryMaxTools.basic.vars.toolsPath + "Icons\\cryExportLogSeverityLevel2_32.bmp"),
								openBitmap (cryMaxTools.basic.vars.toolsPath + "Icons\\cryExportLogSeverityLevel3_32.bmp")
							),
	cSeverityIconTransparentColor = color 0 255 255,
	
	--modern popup stuff
	p2PopupDialogSizeLimit = [600,400], --maximum size that any popup window generated by this module, may have
	iCloseOnCursorDistanceThreshold = 16, --distance of cursor to message window before it closes in pixels - only if auto-close enabled
	p2DialogSizeOffsetBorder = [6,6], --pixels added to dialog by style_border setting
	p2DialogSizeOffsetTitle = [0,0], --pixels added to dialog by style_titlebar setting
	rltCurrentPopup = undefined, --stores the rollout of the popup to be displayed
	iCurrentSeverity = 1,
	sCurrentUserMessage = "",
	
	--lod display dialog stuff
	aLogDialogSizeLimits = #([192,64],[1024,1024]), --min/max size of log display dialog - not including border and title bar
	p2LogDialogSize = [512,128], --stores current size of log display dialog - will be saved in INI file
	p2LogDialogPos = undefined, --stores current position of dialog - will be saved in INI file - if not undefined, dialog will be opened on exporter start
	rltLogDialog = undefined, --stores the rollout of the log display dialog
	aLogSubRollouts = #(), --stores subrollouts contained by log display dialog
	
--###########################################################################################
--LOG FILE HANDLING FUNCTIONS
--###########################################################################################
	fn fnGetLogFilePath =
	(--returns the full filepath to the log-file for the currently opened maxfile
		cryMaxTools.basic.vars.toolsPath + "Logs\\" + (getFilenameFile maxFileName) + "." + sLogFileNameFileType --generate full file path to log file
	),
	
	fn fnGetLogFileStream bReadOnly:true =
	(--returns the log file stream for the opened 3dsmax file, or creates it if not present
		local sFilePath = fnGetLogFilePath()
		
		if doesFileExist sFilePath then
		(
			return (openFile sFilePath mode:"a")
		)
		
		makeDir (getFilenamePath sFilePath) all:true
		return (createFile sFilePath)
	),
	
	fn fnClearLogFile =
	(--erases the content of the log file - this should only happen directly after pressing the export button - returns true on success, false if file not existing
		local sFilePath = fnGetLogFilePath()
		
		if doesFileExist sFilePath then
		(--file found, erase content
			deleteFile sFilePath
			true
		)
		else
		(--file not present
			false
		)
	),

--###########################################################################################
--LOG FILE WRITING FUNCTIONS
--###########################################################################################	
	fn fnDebugMessage sString =
	(--prints a debug message to the log file - no user feedback
		cryMaxTools.export.log.fnPrintToLog (sString as string) iSeverity:0
	),
	
	fn fnPrintToLog sString iSeverity:1 =
	(--adds the supplied parameter as a new line at the end of the log file
		local sLogFileStream = fnGetLogFileStream bReadOnly:false  --open log file for read/write
		
		seek sLogFileStream #eof --go to end of file
		
		local sNewLine = "[" + (filterstring localtime " ")[2] + "] " + aSeverityPrefixes[iSeverity+1] + ": " + sString --add time&date to beginning of line
		if sNewLine[sNewLine.count] != "\n" then
		(--if last character is not a line-break, then append line-break
			sNewLine += "\n"			
		)
		
		format sNewLine to:sLogFileStream
		
		close sLogFileStream
		
		true
	),
	
--###########################################################################################
--LOG FILE READING FUNCTIONS
--###########################################################################################
	fn fnRemoveQuotationMarksFromString sString =
	(--removes all " inside a string
		sResult = ""
		
		for each in (filterString sString "\"" splitEmptyTokens:true) do
		(
			sResult += each
		)
		
		sResult
	),
	
	fn fnCompareTimeStrings sTime1 sTime2 =
	(--compares two times in string format and returns according result: "greater", "less", "equal"
		local aTime1 = filterString sTime1 ":"
		local aTime2 = filterString sTime2 ":"
		local sResult = "equal"
		
		for i = 1 to 3 do
		(--compare each component
			if sResult == "equal" then
			(
				case of
				(
					(aTime1[i] > aTime2[i]): sResult = "greater"
					(aTime1[i] < aTime2[i]): sResult = "less"
				)
			)
		)
		
		sResult
	),
	
	fn fnSplitLogFileLine sString =
	(--reads in a line from the log file and returns an array containing time, level of severity, remaining string
		local sStringStream = sString as stringStream
		local aResult = #()
		
		aResult[1] = trimLeft (readDelimitedString sStringStream "]") "[" --read time from line, cut off brackets
		aResult[2] = (findItem cryMaxTools.export.log.aSeverityPrefixes (trimLeft (readDelimitedString sStringStream ":") " ")) - 1 --read severity prefix and convert to integer level of severity
		seek sStringStream ((filePos sStringStream) + 1) --move stringStream offset + 1
		aResult[3] = readLine sStringStream --rest of line is actual message
		
		aResult
	),
	
	fn fnGetLogMessagesByPattern sPattern =
	(--returns an array containing all the log lines matching the provided pattern - assumes string as input
		local sLogFileStream = fnGetLogFileStream() --open log file for read only
		
		if not eof sLogFileStream then
		(--if file is not empty
			local sNextLine = (fnSplitLogFileLine (readLine sLogFileStream))[3] --cut off time and severity prefix from line
			local aMatches = #()
			
			while not eof sLogFileStream do
			(--test all lines inside file until reached file end
				if matchPattern sNextLine pattern:sPattern then
				(
					append aMatches sNextLine
				)
				
				sNextLine = (fnSplitLogFileLine (readLine sLogFileStream))[3] --cut off time and severity prefix from line
			)
			
			close sLogFileStream
			aMatches --return matches
		)
		else
		(--if file is empty, return empty array
			close sLogFileStream
			#()
		)
	),
	
	fn fnGetLogMessagesBySeverity aSeverities =
	(--returns a 2D-array containing all log messages with the specified severity and the according severity - assumes bitArray as input
		local sLogFileStream = fnGetLogFileStream() --open log file for read only
		
		if not eof sLogFileStream then
		(--if file is not empty
			local aNextLine = (fnSplitLogFileLine (readLine sLogFileStream)) --get first line and split
			local aMatches = #(#(),#())
			
			while not eof sLogFileStream do
			(--test all lines inside file until reached file end
				if aSeverities[aNextLine[2]] then
				(--check if severity level of messages has been requested
					append aMatches[2] aNextLine[2] --put severity level to resulting array
					append aMatches[1] aNextLine[3] --put message to resulting array
				)
				
				aNextLine = (fnSplitLogFileLine (readLine sLogFileStream)) --get next line and split
			)
			
			close sLogFileStream
			aMatches --return matches
		)
		else
		(--if file is empty, return empty 2D-array
			close sLogFileStream
			#(#(),#())
		)
	),
	
	fn fnGetLogMessagesByTime =
	(--returns a 3D-array containing one array per day - each day array contains an array of messages and one array of severity levels
		local sLogFileStream = fnGetLogFileStream() --open log file for read only
		local aResult = #(#(#(),#()))
		
		if not eof sLogFileStream then
		(--if file is not empty
			local aCurrentLine = (fnSplitLogFileLine (readLine sLogFileStream)) --get first line and split
			local aNextLine = (fnSplitLogFileLine (readLine sLogFileStream)) --get second line and split
			local iDayIndex = 1
			
			while not eof sLogFileStream do
			(--test all lines inside file until reached file end - assign to array per day
				if aCurrentLine[2] != 0 then
				(--if message is not a debug message (severity = 0)
					append aResult[iDayIndex][1] aCurrentLine[3] --append current message
					append aResult[iDayIndex][2] aCurrentLine[2] --append current severity
					
					if (cryMaxTools.export.log.fnCompareTimeStrings aNextLine[1] aCurrentLine[1]) == "less" then
					(--if next time is smaller than current, start new day
						iDayIndex += 1
						append aResult #(#(),#())
					)
				)
				
				aCurrentLine = for each in aNextLine collect each --copy next line into current line
				aNextLine = (fnSplitLogFileLine (readLine sLogFileStream)) --get next line and split
			)
			
			aMatches --return matches
		)
		
		close sLogFileStream

		aResult
	),
	
	
--###########################################################################################
--USER NOTIFICATION FUNCTIONS
--###########################################################################################
	fn fnCutStringToPixelWidth sString iMaxWidth =
	(--returns a string that is no longer than the supplied pixel amount
		local fAverageCharacterWidth = 5.5 --estimated - tweak for faster search
		local iOriginalTextWidth = (getTextExtent sString).x
		
		if iOriginalTextWidth > iMaxWidth then
		(--approximate cut off string that is smaller than iMaxWidth
			local iLastCharacter = (iOriginalTextWidth/fAverageCharacterWidth)
		)
		else
		(
			sString
		)
	),
	
	fn fnUserNotification sString iSeverity:1 =
	(--passes the supplied message to the log file and displays it to the user, if severity level matches user settings for display - returns true if message was displayed, false otherwise
		iSeverity = aMin #(3, aMax #(1, iSeverity)) --clamp severity level to remain within range
		sString = sString as string --make sure that message is a string
		
		cryMaxTools.export.log.fnPrintToLog sString iSeverity:iSeverity
		
		if (4 - iSeverity) <= cryMaxTools.export.iPopupVerbosity then
		(--popup only if message within user defined verbosity range
			messageBox sString title:cryMaxTools.export.log.aSeverityNames[iSeverity] beep:(iSeverity == 3) --display message, beep only when severity level = 3
			
			--modern message display dialog code below - WIP!
			/*
			cryMaxTools.export.log.iCurrentSeverity = iSeverity
			cryMaxTools.export.log.sCurrentUserMessage = sString
			local p2WindowSize = [(getTextExtent sString)[1] + 16,16]
			
			rollout rltSingleUserNotification cryMaxTools.export.log.aSeverityNames[cryMaxTools.export.log.iCurrentSeverity]
			(
				imgTag itgSeverityIcon "" pos:[2,2] bitMap:cryMaxTools.export.log.aSeverityIcons[cryMaxTools.export.log.iCurrentSeverity] transparent:cryMaxTools.export.log.cSeverityIconTransparentColor toolTip:cryMaxTools.export.log.aSeverityNames[cryMaxTools.export.log.iCurrentSeverity]
				label lblMessage "" pos:[16,2] align:#left
				timer tmrMouseDistanceCheckInterval interval:50
				
				on rltSingleUserNotification open do
				(
					lblMessage.text = cryMaxTools.export.log.sCurrentUserMessage
				)
				
				
				on tmrMouseDistanceCheckInterval tick do
				(--test mousedistance in timer tick
					local b2DialogBox = box2 (GetDialogPos cryMaxTools.export.log.rltCurrentPopup).x (GetDialogPos cryMaxTools.export.log.rltCurrentPopup).y (GetDialogSize cryMaxTools.export.log.rltCurrentPopup).x (GetDialogSize cryMaxTools.export.log.rltCurrentPopup).y
					local b2ToleranceBox = box2 [b2DialogBox.left - cryMaxTools.export.log.iCloseOnCursorDistanceThreshold, b2DialogBox.top - cryMaxTools.export.log.iCloseOnCursorDistanceThreshold] [b2DialogBox.right + cryMaxTools.export.log.iCloseOnCursorDistanceThreshold + cryMaxTools.export.log.p2DialogSizeOffsetBorder.x, b2DialogBox.bottom + cryMaxTools.export.log.iCloseOnCursorDistanceThreshold + cryMaxTools.export.log.p2DialogSizeOffsetBorder.y]
					local p2CursorPos = mouse.screenPos
					
					if not (contains b2ToleranceBox p2CursorPos) then
					(
						DestroyDialog rltSingleUserNotification
						--local p2Distance = ((GetDialogPos rltSingleUserNotification) - p2CursorPos)
						--print ((abs p2Distance.x) as string + " " + (abs p2Distance.y) as string)
					)
				)
			)
			
			if cryMaxTools.export.log.rltCurrentPopup != undefined then
			(--destroy popup if already present
				destroyDialog cryMaxTools.export.log.rltCurrentPopup
			)
			cryMaxTools.export.log.rltCurrentPopup = rltSingleUserNotification
			
			createDialog cryMaxTools.export.log.rltCurrentPopup width:p2WindowSize.x height:p2WindowSize.y pos:(mouse.screenPos - p2WindowSize/2 - [0,12]) style:#(#style_border) lockWidth:true lockHeight:true
			*/
			
			true
		)
		else
		(--return false when message was not displayed
			false
		)
	),
	
	fn fnMultiUserNotification aStrings aSeverityLevels sTitle bYesNoBox:false =
	(--same as fnUserNotification but with multiple lines - returns true if message was printed to log else false - if bYesNoBox is true, the user will can choose to answer yes/no - the choice will be returned as a string - if messages has not been displayed due to low verbosity level, undefined will be returned - sTitle should be a question in this case - empty message strings with severity 0 will not be added to the log but will create an empty line in the popup message - nonempty message string with severity 0 will be ignored
		if aStrings.count > 0 and aStrings.count == aSeverityLevels.count then
		(--both supplied arrays need same member count
			local iTextExtentX = 0 --width of text to be displayed in pixels
			local iTextExtentY = 0 --height of text to be displayed in pixels
			local sDisplayMessage = ""
			
			for i = 1 to aStrings.count do
			(--validate incoming data, print to log file and generate display data
				aSeverityLevels[i] = aMin #(3, aMax #(0, aSeverityLevels[i])) --clamp severity level to remain within range
				aStrings[i] = aStrings[i] as string --make sure that message is a string
				
				if aSeverityLevels[i] > 0 then
				(
					cryMaxTools.export.log.fnPrintToLog aStrings[i] iSeverity:aSeverityLevels[i]
				)
				
				iTextExtentX = aMax #(iTextExtentX, (getTextExtent aStrings[i]).x)
				iTextExtentY += (getTextExtent aStrings[i]).y
				
				if aSeverityLevels[i] > 0 and ((4 - aSeverityLevels[i]) <= cryMaxTools.export.iPopupVerbosity) then
				(--popup only if message within user defined verbosity range
					sDisplayMessage += (aSeverityNames[aSeverityLevels[i]] + " " + aStrings[i] + "\n")
				)
				else if i != aSeverityLevels.count and aSeverityLevels[i] == 0 and aStrings[i] == "" and sDisplayMessage!= "" then
				(--insert empty line unless no more lines after this one or current display string empty
					sDisplayMessage += "\n"
				)
			)
			
			if bYesNoBox then
			(--display yes/no box
				if sDisplayMessage != "" then
				(
					local sResult = queryBox sDisplayMessage title:(sTitle as string) beep:true
					
					if sResult then sResult = "yes" else sResult = "no"
					
					sResult
				)
				else
				(
					undefined
				)
			)
			else
			(--display simple messagebox
				if sDisplayMessage != "" then
				(
					messageBox sDisplayMessage title:(sTitle as string) beep:true
				)
				
				true
			)
		)
		else
		(
			false
		)
	),
	
	fn fnGenerateLogDisplaySubRollouts =
	(--dynamic rollout creation - one per day in the log file - containing all messages of that day - remove rollouts from dialog first!!!
		local aMessages = cryMaxTools.export.log.fnGetLogMessagesByTime()
		
		if aMessages.count > 3 then
		(--combine days if more than 3
			local aMessagesCopy = copy aMessages #noMap
			
			local aMessages = #(#(#(),#()),aMessagesCopy[aMessagesCopy.count - 1],aMessagesCopy[aMessagesCopy.count])
			for i = 1 to (aMessagesCopy.count - 2) do
			(--merge day older than yesterday
				aMessages[1][1] += aMessagesCopy[i][1]
				aMessages[1][2] += aMessagesCopy[i][2]
			)
		)
		
		cryMaxTools.export.log.aLogSubRollouts = #() --clear current rollouts
		
		for i = 1 to aMessages.count do
		(--generate rollout per day
			local sTitle = #("Older","Yesterday","Today")[i] --generate rollout title
			
			rltNewDynamicRollout = rolloutCreator ("rltLogDay" + i as string) sTitle
			rltNewDynamicRollout.begin()
			
			for j = 1 to aMessages[i][1].count do
			(--add one icon and message per aTodayMessages element
				local sBitMapVariableName = "cryMaxTools.export.log.aSeverityIcons[" + aMessages[i][2][j] as string + "]"
				local sToolTip = case aMessages[i][2][j] of
				(
					1: "\"Notice.\""
					2: "\"Warning!\""
					3: "\"Error!\""
				)
				
				rltNewDynamicRollout.addControl #imgTag ("itg" + j as string) "" paramStr:("pos:[2," + (j*16 - 12) as string + "] bitMap:" + sBitMapVariableName + " transparent:cryMaxTools.export.log.cSeverityIconTransparentColor toolTip:" + sToolTip)
				rltNewDynamicRollout.addControl #label ("lbl" + j as string) (cryMaxTools.export.log.fnRemoveQuotationMarksFromString aMessages[i][1][j]) paramStr:("pos:[16," + (j*16 - 12) as string + "]")
			)
			
			rltNewDynamicRollout = rltNewDynamicRollout.end()
			
			append cryMaxTools.export.log.aLogSubRollouts rltNewDynamicRollout
		)
	),
	
	fn fnDisplayLogFile =
	(--opens a new dialog displaying all messages from the log file - can be filtered with matchPattern using sFilterString
		if cryMaxTools.export.log.rltLogDialog != undefined then
		(--destroy log display dialog if already present
			destroyDialog cryMaxTools.export.log.rltLogDialog
		)
		
		cryMaxTools.export.log.fnGenerateLogDisplaySubRollouts() --update subrollouts
		
		-- ### LOD DISPLAY DIALOG CREATION BEGIN ###
		rollout rltLogDisplayContainer "CryENGINE 3 Exporter Script Log"
		(--log display container dialog
			subRollout rltMessageHolder width:cryMaxTools.export.log.p2LogDialogSize.x height:(cryMaxTools.export.log.p2LogDialogSize.y - 16) pos:[0,0]
			
			button btnClear "Clear Log File" pos:[0,cryMaxTools.export.log.p2LogDialogSize.y - 16] width:80 height:16
			button btnRefresh "Refresh" pos:[80,cryMaxTools.export.log.p2LogDialogSize.y - 16] width:64 height:16
			button btnClose "Close" pos:[464,cryMaxTools.export.log.p2LogDialogSize.y - 16] width:48 height:16
			
			on rltLogDisplayContainer open do
			(--on open add all subrolloouts
				for each in cryMaxTools.export.log.aLogSubRollouts do
				(
					addSubRollout rltMessageHolder each
				)
				for i = 1 to rltMessageHolder.rollouts.count do
				(
					rltMessageHolder.rollouts[i].open = i == rltMessageHolder.rollouts.count
					if i == rltMessageHolder.rollouts.count then
					(
						rltMessageHolder.rollouts[i].scrollPos = rltMessageHolder.rollouts[i].height - 16*i
					)
				) 
			)
			
			on btnClose pressed do
			(--close log display dialog
				DestroyDialog cryMaxTools.export.log.rltLogDialog
			)
			
			on rltLogDisplayContainer resized size do
			(--enforce dialog size to stay within limits
				if size.x < cryMaxTools.export.log.aLogDialogSizeLimits[1].x then
				(
					cryMaxTools.export.log.rltLogDialog.width = cryMaxTools.export.log.aLogDialogSizeLimits[1].x
					size.x = cryMaxTools.export.log.aLogDialogSizeLimits[1].x
				)
				if size.y < cryMaxTools.export.log.aLogDialogSizeLimits[1].y then
				(
					cryMaxTools.export.log.rltLogDialog.height = cryMaxTools.export.log.aLogDialogSizeLimits[1].y
					size.y = cryMaxTools.export.log.aLogDialogSizeLimits[1].y
				)
				if size.x > cryMaxTools.export.log.aLogDialogSizeLimits[2].x then
				(
					cryMaxTools.export.log.rltLogDialog.width = cryMaxTools.export.log.aLogDialogSizeLimits[2].x
					size.x = cryMaxTools.export.log.aLogDialogSizeLimits[2].x
				)
				if size.y > cryMaxTools.export.log.aLogDialogSizeLimits[2].y then
				(
					cryMaxTools.export.log.rltLogDialog.height = cryMaxTools.export.log.aLogDialogSizeLimits[2].y
					size.y = cryMaxTools.export.log.aLogDialogSizeLimits[2].y
				)
				
				rltMessageHolder.height = size.y - 16
				rltMessageHolder.width = size.x
				btnClear.pos = [0,size.y - 16]
				btnRefresh.pos = [80,size.y - 16]
				btnClose.pos = size - [48,16]
				
				--remove/re-add subrollouts to update size
				for i = 1 to rltMessageHolder.rollouts.count do
				(
					removeSubRollout rltMessageHolder rltMessageHolder.rollouts[1]
				)
				for each in cryMaxTools.export.log.aLogSubRollouts do
				(
					addSubRollout rltMessageHolder each
				)
				for i = 1 to rltMessageHolder.rollouts.count do
				(
					rltMessageHolder.rollouts[i].open = i == rltMessageHolder.rollouts.count
					if i == rltMessageHolder.rollouts.count then
					(
						rltMessageHolder.rollouts[i].scrollPos = rltMessageHolder.rollouts[i].height - 16*i
					)
				) 
			)
		)
		
		cryMaxTools.export.log.rltLogDialog = rltLogDisplayContainer
			
		local p2Position = cryMaxTools.export.log.p2LogDialogPos
		if p2Position == undefined then
		(
			p2Position = mouse.screenPos - cryMaxTools.export.log.p2LogDialogSize*.5
		)
		createDialog cryMaxTools.export.log.rltLogDialog width:cryMaxTools.export.log.p2LogDialogSize.x height:cryMaxTools.export.log.p2LogDialogSize.y pos:p2Position style:#(#style_titlebar, #style_resizing, #style_sysmenu, #style_toolwindow)
		
		-- ### LOD DISPLAY DIALOG CREATION END ###
	)
)

--###########################################################################################
--GENERAL EXPORTER STRUCT
--GENERAL EXPORTER STRUCT #########################################################################
--GENERAL EXPORTER STRUCT
--###########################################################################################
struct cryMaxToolsExportStruct
(
--#########################################################
--GLOBAL VARIABLES
--#########################################################
	object = cryMaxToolsObjectExportStruct(),
	anim = cryMaxToolsAnimationExportStruct(),
	material = cryMaxToolsMaterialExportStruct(),
	log = cryMaxToolsExporterLogMessageStruct(),
	
	iPopupVerbosity = 2, --set by user - verbosity for popup messages - 1 = only errors, 2 = errors and warnings, 3 = errors, warnings and notifications
	bReparentTwistBones = true, --set by user - reparents biped twist bones
	
--#########################################################
--GENERAL FUNCTIONS
--#########################################################
	fn fnGetSkinnedBonesFromNode sNode =
	(--returns all bones skinned to a node - can handle crySkin only - assumes single node as input
		local aSkinModifiers = for each in sNode.modifiers where (classOf each == Skin) collect each --collect all skin modifiers from node
		local aSkinnedBones = #()
		
		if aSkinModifiers.count != 0 then
		(
			--local sOldCommandPanelMode = getCommandPanelTaskMode()
			--setCommandPanelTaskMode #modify
			
			for each in aSkinModifiers do
			(
				--modPanel.setCurrentObject each
				
				try
				(--doing this on a normal skin modifier would crash max script due to a bug in the exporter plugin
					skinOps.GetSelectedBone each --this will crash on normal skin mod
					
					for i = 1 to skinOps.getNumberBones each do
					(
						append aSkinnedBones (getNodeByName (skinOps.getBoneName each i 1))
					)
				)
				catch
				(
					cryMaxTools.export.log.fnDebugMessage ("fnGetSkinnedBonesFromNode caught a maxScript runtime exception. Probably due to use of Skin modifier instead of CrySkin.")
				)
			)
			
			--setCommandPanelTaskMode sOldCommandPanelMode
			
			makeUniqueArray aSkinnedBones
		)
		
		aSkinnedBones
	),
	
	fn fnGetSkinnedHierarchiesFromNode sNode =
	(--returns all bones skinned to a node, including all nodes above them in the hierarchy - can handle crySkin only - assumes single node as input
		local aSkinModifiers = for each in sNode.modifiers where (classOf each == Skin) collect each --collect all skin modifiers from node
		local aSkinnedBones = #()
		
		if aSkinModifiers.count != 0 then
		(
			--local sOldCommandPanelMode = getCommandPanelTaskMode()
			--setCommandPanelTaskMode #modify
			
			for each in aSkinModifiers do
			(
				--modPanel.setCurrentObject each
				
				try
				(--doing this on a normal skin modifier would crash max script due to a bug in the exporter plugin
					skinOps.GetSelectedBone each --this will crash on normal skin mod
					
					for i = 1 to skinOps.getNumberBones each do
					(--collect all bones from skin mod, plus hierarchies
						local sCurrentBone = getNodeByName (skinOps.getBoneName each i 1)
						if appendIfUnique aSkinnedBones sCurrentBone then
						(--if bone is new, get parents
							local sCurrentNode = sCurrentBone
							
							while sCurrentNode.parent != undefined and appendIfUnique aSkinnedBones sCurrentNode.parent do
							(--collect parent until no new nodes left
								sCurrentNode = sCurrentNode.parent
							)
						)
					)
				)
				catch
				(
					cryMaxTools.export.log.fnDebugMessage ("fnGetSkinnedHierarchiesFromNode caught a maxScript runtime exception. Probably due to use of Skin modifier instead of CrySkin.")
				)
			)
			
			--setCommandPanelTaskMode sOldCommandPanelMode
			
			makeUniqueArray aSkinnedBones
		)
		
		aSkinnedBones
	),
	
	fn fnGetBipedRootsFromHierarchy sHierarchyRoot bIncludeSkinnedBones:true =
	(
		local aChildren= cryMaxTools.getChildren sHierarchyRoot
		local aBipedRoots = #()
			
		if bIncludeSkinnedBones then
		(--search skin modifiers for biped aswell
			local aBones = #()
			for each in aChildren do 
			(
				aBones += cryMaxTools.export.fnGetSkinnedBonesFromNode each
			)
			
			aChildren += aBones
			
			makeUniqueArray aChildren
		)
		
		local aBipedObjects = for each in aChildren where each.classID[1] == 37157 collect each
		
		for each in aBipedObjects do
		(--collect biped roots from biped objects
			appendIfUnique aBipedRoots each.controller.rootNode
		)
		
		aBipedRoots
	),
	
	fn fnReparentTwistBones sRoot =
	(--reparents biped arm twist bones - returns reparented nodes - assumes biped rootnode as input
		
		local aReparented = #()
		local aTwistBones = #(biped.getNode sRoot #LFArmTwist,biped.getNode sRoot #RFArmTwist,biped.getNode sRoot #LUpArmTwist,biped.getNode sRoot #RUpArmTwist)
		local aNewParentBones = #(biped.getNode sRoot #LArm link: 3,biped.getNode sRoot #RArm link: 3,biped.getNode sRoot #LArm link: 2,biped.getNode sRoot #RArm link: 2)
		
		for i = 1 to 4 do
		(
			if aTwistBones[i] != undefined and aTwistBones[i].parent != aNewParentBones[i] then
			(
				aTwistBones[i].parent = aNewParentBones[i]
				append aReparented aTwistBones[i]
			)
		)
		
		aReparented
	),
	
	fn fnReparentTwistBonesInHierarchies aNodes =
	(--finds all the bipeds in the hierarchy of the node and reparents their twist bones - returns an array containing reparented twist bones
		local aBipedRoots = #()
		local aReparented = #()
		
		for each in aNodes do
		(--collect biped roots
			aBipedRoots += cryMaxTools.export.fnGetBipedRootsFromHierarchy each
		)
		
		for each in aBipedRoots do
		(--reparent twist bones for current biped and collect names of reparented nodes
			aReparented += cryMaxTools.export.fnReparentTwistBones each
		)
		
		cryMaxTools.export.log.fnDebugMessage ("fnReparentTwistBonesInHierarchies called and found the following bipeds: " + ((for each in aBipedRoots collect each.name) as string) + ".")
		
		aReparented
	),
	
	fn fnSaveSettings =
	(--save exporter user settings - strings containing ";" will confuse it
		local aCryExporterSettings = #() --temporary array to store user settings
		
		--append general export settings
		append aCryExporterSettings #("export.iPopupVerbosity", cryMaxTools.export.iPopupVerbosity as string)
		
		--append object export settings
		append aCryExporterSettings #("export.object.bCheckMaterials", cryMaxTools.export.object.bCheckMaterials as string)
		append aCryExporterSettings #("export.object.bCheckMorphs", cryMaxTools.export.object.bCheckMorphs as string)
		append aCryExporterSettings #("export.object.bCheckBoneScale", cryMaxTools.export.object.bCheckBoneScale as string)
		append aCryExporterSettings #("export.object.bMarkDegenerateFaces", cryMaxTools.export.object.bMarkDegenerateFaces as string)
		append aCryExporterSettings #("export.object.bMarkCollapsedVertices", cryMaxTools.export.object.bMarkCollapsedVertices as string)
		
		local ssCryExportSettings = stringStream ""  --string to be saved to INI file
		for each in aCryExporterSettings do
		(--compose string stream from settings array
			format (each[1] + ";" + each[2] + ";") to:ssCryExportSettings
		)
		
		csExport.set_value "cryExportSettings" (ssCryExportSettings as string) --save settings to INI file
	),
	
	fn fnLoadSettings =
	(--load exporter settings into global vars
		local sCryExporterSettings = csExport.get_value "cryExportSettings" --load settings from INI file
		
		if sCryExporterSettings != "" then
		(--if settings present, attempt to assign values to varaibles
			local aCryExporterSettings = filterString sCryExporterSettings ";" splitEmptyTokens:true
			
			--export struct vars
			local iIndex = findItem aCryExporterSettings "export.iPopupVerbosity"
			if iIndex != 0 then
			(--load iPopupVerbosity if present
				case aCryExporterSettings[iIndex+1] of
				(
					"1": (cryMaxTools.export.iPopupVerbosity = 1)
					"2": (cryMaxTools.export.iPopupVerbosity = 2)
					"3": (cryMaxTools.export.iPopupVerbosity = 3)
					default:	(print "cryExporterSettings: Failed to load export.iPopupVerbosity value.")
				)
			)
			
			--export.object struct vars
			local iIndex = findItem aCryExporterSettings "export.object.bCheckMaterials"
			if iIndex != 0 then
			(--load bCheckMaterials if present
				case aCryExporterSettings[iIndex+1] of
				(
					"true": (cryMaxTools.export.object.bCheckMaterials = true)
					"false": (cryMaxTools.export.object.bCheckMaterials = false)
					default:	(print "cryExporterSettings: Failed to load export.object.bCheckMaterials value.")
				)
			)
			
			local iIndex = findItem aCryExporterSettings "export.object.bCheckMorphs"
			if iIndex != 0 then
			(--load bCheckMorphs if present
				case aCryExporterSettings[iIndex+1] of
				(
					"true": (cryMaxTools.export.object.bCheckMorphs = true)
					"false": (cryMaxTools.export.object.bCheckMorphs = false)
					default:	(print "cryExporterSettings: Failed to load export.object.bCheckMorphs value.")
				)
			)
			
			local iIndex = findItem aCryExporterSettings "export.object.bCheckBoneScale"
			if iIndex != 0 then
			(--load bCheckBoneScale if present
				case aCryExporterSettings[iIndex+1] of
				(
					"true": (cryMaxTools.export.object.bCheckBoneScale = true)
					"false": (cryMaxTools.export.object.bCheckBoneScale = false)
					default:	(print "cryExporterSettings: Failed to load export.object.bCheckBoneScale value.")
				)
			)
			
			local iIndex = findItem aCryExporterSettings "export.object.bMarkDegenerateFaces"
			if iIndex != 0 then
			(--load bMarkDegenerateFaces if present
				case aCryExporterSettings[iIndex+1] of
				(
					"true": (cryMaxTools.export.object.bMarkDegenerateFaces = true)
					"false": (cryMaxTools.export.object.bMarkDegenerateFaces = false)
					default:	(print "cryExporterSettings: Failed to load export.object.bMarkDegenerateFaces value.")
				)
			)
			
			local iIndex = findItem aCryExporterSettings "export.object.bMarkCollapsedVertices"
			if iIndex != 0 then
			(--load bMarkCollapsedVertices if present
				case aCryExporterSettings[iIndex+1] of
				(
					"true": (cryMaxTools.export.object.bMarkCollapsedVertices = true)
					"false": (cryMaxTools.export.object.bMarkCollapsedVertices = false)
					default:	(print "cryExporterSettings: Failed to load export.object.bMarkCollapsedVertices value.")
				)
			)
		)
		else
		(
			print "cryExporterSettings: Empty or not found."
		)
	),
	
	fn fnGetExporterSettingsNode =
	(--returns exporter settings node, creates it if not present
		local sExporterSettingsNode = getNodeByName "cryExporterSettingsNode"
		
		if sExporterSettingsNode == undefined then
		(-- create node if doesn't exist
			sExporterSettingsNode = dummy pos:[0,0,0] name:"cryExporterSettingsNode"
			setUserProp sExporterSettingsNode "bReparentTwistBones" "true"
			
			local sExportNodeLayer = cryMaxTools.export.object.fnGetExportNodeLayer() --get cryExportLayer
			sExportNodeLayer.addnode sExporterSettingsNode --move node to layer
		)
		
		sExporterSettingsNode --return node
	),
	
	fn fnStorePerFileSettings =
	(--stores exporter settings that must be set per max file into dummy node
		local sExporterSettingsNode = cryMaxTools.export.fnGetExporterSettingsNode()
		
		setUserProp sExporterSettingsNode "bReparentTwistBones" (cryMaxTools.export.bReparentTwistBones as string) --store bReparentTwistBones flag
	),
	
	fn fnLoadPerFileSettings =
	(--loads exporter settings that must be set per max file into dummy node
		local sExporterSettingsNode = cryMaxTools.export.fnGetExporterSettingsNode()
		
		cryMaxTools.export.bReparentTwistBones = ((getUserProp sExporterSettingsNode "bReparentTwistBones") as string == "true") --load bReparentTwistBones flag
	),
	
	fn fnUpdate =
	(--sync working data with file ... will be called in a postfileopen callback!
		cryMaxTools.export.fnLoadPerFileSettings()
		cryMaxTools.export.object.fnUpdate()
		cryMaxTools.export.anim.fnUpdate()
	),
	
--#########################################################
--USER DEFINED PROPERTIES FUNCTIONS
--#########################################################
	fn fnGetUPDFlag sNode sFlag =
	(--returns true if string was found in node's user defined properties, false otherwise - assumes node, string as input
		local sUserProperties = getUserPropBuffer sNode
		
		matchPattern sUserProperties pattern:("*" + sFlag + "*") --return true if found, false otherwise
	),
	
	fn fnSetUPDFlag sNode sFlag bState =
	(--adds/removes string to node's user defined properties - must be enclosed by line-breaks - assumes node, string, boolean as input
		local sUserProperties = getUserPropBuffer sNode --get userprops from node
		local aUserProperties = filterString sUserProperties "\r\n" --one element per line - next line character in userprops is "\r\n"
		local aNewUserProperties = for each in aUserProperties where (toLower each) != (toLower sFlag) collect each --generate new lines, without flag - removes potential double occurences
		
		if bState then
		(--flag should be set
			append aNewUserProperties sFlag
		)
		
		local sNewUserProperties = ""
		for each in aNewUserProperties do
		(--compose new userprop string
			sNewUserProperties += each + "\r\n"
		)
		
		setUserPropBuffer sNode sNewUserProperties
	),
	
	fn fnDeleteUserDefinedProperty sNode sProperty =
	(--removes a user defined property key/property pair from the UDP string of the specified node - must be delimited by \r\n
		local sUserProperties = getUserPropBuffer sNode --get userprops from node
		local aUserProperties = filterString sUserProperties "\r\n" --one element per line - next line character in userprops is "\r\n"
		local aNewUserProperties = for each in aUserProperties where not (matchPattern each pattern:(sProperty + " = *")) collect each --generate new lines, without property - removes potential double occurences
		
		local sNewUserProperties = ""
		for each in aNewUserProperties do
		(--compose new userprop string
			sNewUserProperties += each + "\r\n"
		)
		
		setUserPropBuffer sNode sNewUserProperties
	),
	
	fnCheckSceneForOldExport
)

cryMaxTools.export = cryMaxToolsExportStruct()

--callbacks to update and global exporter data when file opened/merged/imported/created or max reseted
callbacks.addScript #filePostOpen "cryMaxTools.export.fnUpdate()" id:#updateExporterData
callbacks.addScript #filePostMerge "cryMaxTools.export.fnUpdate()" id:#updateExporterData
callbacks.addScript #postImport "cryMaxTools.export.fnUpdate()" id:#updateExporterData
callbacks.addScript #systemPostNew "cryMaxTools.export.fnUpdate()" id:#updateExporterData
callbacks.addScript #systemPostReset "cryMaxTools.export.fnUpdate()" id:#updateExporterData

--load settings from last session
cryMaxTools.export.fnLoadSettings()








cryMaxTools.export.fnCheckSceneForOldExport = fn fnCheckSceneForOldExport =
(
	cryMaxTools.export.log.fnClearLogFile()

	cryMaxTools.export.log.fnDebugMessage ("Checking zero morphs")
	cryMaxTools.export.object.fnCheckNonZeroMorphs (csexport.export.get_node_list())
	
	cryMaxTools.export.log.fnDebugMessage ("Getting bones")
	local boneScaleArray = cryMaxTools.export.object.fnGetBonesWithScaleFromHierarchies (csexport.export.get_bone_list())
	
	cryMaxTools.export.log.fnDebugMessage ("Reparenting bones (if needed)")
	cryMaxTools.export.fnReparentTwistBonesInHierarchies (csexport.export.get_bone_list())

	cryMaxTools.export.log.fnDebugMessage ("Done")
		
	return true
)

csexport.export.register_export_callback cryMaxTools.export.fnCheckSceneForOldExport
