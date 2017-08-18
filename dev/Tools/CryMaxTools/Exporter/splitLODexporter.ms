fn fnC2ExportLODs projectRoot =
(
	local objs = objects
	local indexes = #()
	local lodObjs = #()

	for i=1 to objs.count do
	(
		--print objs[i].name
		local idx = findString objs[i].name "_LOD"
		if idx != undefined then
		(
			append indexes idx
			append lodObjs objs[i]
		)
	)

	local origMaxFilePath = maxFilePath
	local origMaxFileName = maxFileName

	if (origMaxFileName != ".max" or origMaxFileName != "") then
	(
		messageBox "You must Load or Save a valid Max scene first."
		return false
	)
	
	UtilityPanel.OpenUtility csExport
	
	csexport.export.set_node_list(#())
	local rootFilenames = #()
	local lod0Filenames = #()

	-- save current max scene to a temp folder, so we can export out of the view of the Editor (which may be open)
	try(makedir (origMaxFilePath + "_LODtemp")) catch()
	saveMaxFile (origMaxFilePath + "_LODtemp/" + origMaxFileName) clearNeedSaveFlag:false quiet:true
	-- make a copy of the original max scene file so we can restore it later
	copyFile (origMaxFilePath + origMaxFileName) (origMaxFilePath + origMaxFileName + "1")

	for i=1 to lodObjs.count do
	(
		local obj = lodObjs[i]
		local oldName = obj.name
		local lodNum = substring oldName (indexes[i]+4) 1
		local noLODName = substring oldName 1 (indexes[i]-1)

		local noLODFilename = (noLODName + ".cgf")
		local lodFilename = (noLODName + "_LOD" + (lodNum as string) + ".cgf")

		-- now we are exporting to the temp folder
		obj.name = noLODName
		csexport.export.set_node_list (#(obj))
		csexport.export.export_nodes()
		obj.name = oldName

		deleteFile (maxFilePath + lodFilename)
		renameFile (maxFilePath + noLODFilename) (maxFilePath + lodFilename)

		rccmd = (projectRoot + "/bin32/rc/rc " + maxFilePath + lodFilename + " /refresh")
		--print rccmd

		try
		(HiddenDOSCommand ( rccmd ))
		catch
		(DOSCommand ( rccmd ))

		-- now copy the compiled export back to the original folder so Editor can see it
		deleteFile (origMaxFilePath + lodFilename)
		copyFile (maxFilePath + lodFilename) (origMaxFilePath + lodFilename)

		-- if this is LOD0, then store the name so we can remove the _LOD0 later
		if ((lodNum as integer) == 0) then
		(
			append rootFilenames (origMaxFilePath + noLODFilename)
			append lod0Filenames (origMaxFilePath + lodFilename)
		)

		print ("Exported "+obj.name+" as "+lodFilename)
	)

	-- list all the temporary files in the temp folder
	tempfiles = getFiles (origMaxFilePath + "_LODtemp/*.*")

	-- save the current scene back to the original location, restore the back-up one, and delete the backup file
	saveMaxFile (origMaxFilePath + origMaxFileName)
	deleteFile (origMaxFilePath + origMaxFileName)
	renameFile (origMaxFilePath + origMaxFileName + "1") (origMaxFilePath + origMaxFileName)

	-- delete all the temporary files
	for f in tempfiles do
	(h
		deleteFile f
	)

	-- and remove the temporary folder
	DOSCommand ("rmdir /Q " + origMaxFilePath + "\\" + "_LODtemp")

	-- for all the LOD0 objects, remove the _LOD0 suffix
	for i=1 to rootFilenames.count do
	(
		deleteFile rootFilenames[i]
		renameFile lod0Filenames[i] rootFilenames[i]
	)
	
	gc()
)

	struct stCryExportNode
	(
		projectRoot = "",
		exportNode = undefined,		-- export node assigned to the export list
		aTempLODNodes = #(),		-- list of temporary LOD nodes that we create (to delete after export)
		aTempExportNodes = #(),
		aChildNodes = #(),				-- all the direct children of the export node (so we can restore original hierarchy later)
		glassNode = undefined,
		baseMesh = undefined,			-- original base mesh of this export node (should be only one)
		sOrigBaseName = "",					-- original base mesh name
		aSubMeshes = #(),				-- list of submeshes under the base mesh
		aPhysProxies = #(),				-- list of phys proxies
		aLODs = #(),						-- LOD nodes belonging to this object
		aSnapshots = #(),				-- list of temporary snapshot meshes to delete later
		
		-- return a unique name that is not used by any other object in the scene
		fn renameUnique sName =
		(
			i = 0
			local test = (sName + (i as string))
			
			if (test != sName) then
			(
				while getNodeByName(test) != undefined do
				(
					i += 1
					test = (sName + (i as string))
				)
			)
			return test
		),
		
		-- check UDP of obj, searching for any ocurrence of any string in array aStrings
		fn checkUDP obj aStrings =
		(
			local bFound = false
			local udp = getUserPropBuffer obj
			for str in aStrings do
			(
				if (findString (toLower(udp)) str) != undefined then
					bFound = true
			)
			return bFound
		),
		
		fn checkMat obj =
		(
			local snap = snapshotAsMesh obj
			local aMatPhys = #()
			local matIDList = #()
			-- first find out which submats are physicalized
			if (getNumSubMtls obj.material) > 0 then
			(
				matIDList = obj.material.materialIDList
				for i=1 to matIDList.count do
				(
					local phys = false
					try (phys = obj.material.materialList[i].physicalizeMaterial) catch()
					aMatPhys[matIDList[i]] = phys
				)
			)
			
			iPhysOrNot = 0	-- 0 = no phys, 1 = all phys, 2 = mixed
			for f=1 to snap.numfaces do
			(
				local matid = getFaceMatID snap f
				if aMatPhys[matid] then
				(
					if iPhysOrNot == 0 then
						iPhysOrNot = 1
				) else (
					if iPhysOrNot == 1 then
						iPhysOrNOt = 2
				)
			)
			
			if iPhysOrNot == 1 then
				return "phys"
			else
				return ""
		),
		
		fn getObjectType obj = 
		(
			type = ""
			
			if superClassOf obj == geometryClass then	-- should be a mesh
			(
				if (substring obj.name 1 1) == "_" then
					type = "skip"
				
				if (substring obj.name 1 1) == "$" then
				(
					if ((findString (toLower(obj.name)) "$physics_proxy") == 1) then		-- if it's a physproxy, collect it  ** TODO: check other names & check UDP properties
					(
						type = "proxy"
					) else (
						-- check for existence of 'proxy' anywhere in the name -- rename to $physics_proxy...
						if ((findString (toLower(obj.name)) "proxy") != undefined) then
						(
							obj.name = renameUnique "$physics_proxy"
							type = "proxy"
						)
				
						-- check for old style $collision naming convention -- rename to $physics_proxy...
						if (((findString (toLower(obj.name)) "$collision") == 1) or ((findString (toLower(obj.name)) "$colision") == 1)) then
						(
							obj.name = renameUnique "$physics_proxy"
							type = "proxy"
						)
						
						if ((findString (toLower(obj.name)) "joint") == undefined) then
						(
							type = "joint"
						)

						if ((findString (toLower(obj.name)) "$lod") == 1) then
						(
							type = "lod"
						)
						
						-- no matches so far...
						if (type == "") then
						(
							-- check the UDP for key words
							if (checkUDP obj #("other_rendermesh", "box", "sphere", "capsule", "cylinder")) == true then
							(
								obj.name = renameUnique "$physics_proxy"
								type = "proxy"
							)
						)
					)
				) else (  -- no $ prefix..
					if type == "" then
					(
						if ((findString (toLower(obj.name)) "breakGlass") == 1) then
						(
							type = "glass"
						) else (
							if (checkUDP obj #("other_rendermesh", "box", "sphere", "capsule", "cylinder")) == true then -- or (checkMat obj) == "phys" then
							(
								obj.name = renameUnique "$physics_proxy"
								type = "proxy"
							) else (
								-- everything known fails, so must be a mesh
								--print ("OBJ "+obj.name+" = mesh")
								type = "mesh"
							)
						)
					)
				)
			)

			return type
		),
		
		-- get all child nodes of the exportNode, so that when we change the hierarchy temporarily for exporting, we can restore it all later
		fn getExportNodeChildren = 
		(
			aChildNodes = exportNode.children
			
			if (aChildNodes.count == 0) then
			(
				--messageBox "ERROR: export node contains no child objects."
				if (superClassOf exportNode == GeometryClass) then
				(
					baseMesh = exportNode
					return true
				) else
					return false
			) else (
				for childNode in aChildNodes do
				(
					--print ("Checking childnode '" + childNode.name+"'...")
					local type = getObjectType childNode
					--print ("childNode = "+childNode.name+", type='"+type+"'")
					case type of
					(
						"proxy" 	: append aPhysProxies childNode
						"lod" 		: append aLODs childNode
						--"joint" 	: 
						--"skip"	:
						"glass" 	: (	if glassNode == undefined then  (
											glassNode = childNode
										) else (
											messageBox ("ERROR: found more than one 'breakGlass' object under export node "+exportNode.name)
											return false
										)
									)
						"mesh"	: 
									(	if baseMesh == undefined then (
											sOrigBaseName = childNode.name
											childNode.name = renameUnique childNode.name
											baseMesh = childNode
										) else (
											messageBox ("ERROR: found more than one non-glass mesh object ("+baseMesh.name+", "+childNode.name+") under export node "+exportNode.name)
											return false
										)
									)
					)
				)
				if (glassNode != undefined) then
				(
					if (checkUDP exportNode #("no_runtime_merge")) == false then
					(
						local udp = getUserPropBuffer exportNode
						if (udp.count > 0) then
							udp = udp + "\r\n"
						
						udp = (udp + "no_runtime_merge")
						setUserPropBuffer exportNode udp
					)
				)
				
				if (baseMesh == undefined) then
				(
					if (superClassOf exportNode == GeometryClass) then
					(
						baseMesh = exportNode
						return true
					) else
						return false
				)
			)
			-- only return true if we found a base mesh
			return true
		),

		fn getSubmeshes = 
		(
			for childNode in baseMesh.children do
			(
				local type = getObjectType childNode
				--print ("submesh Node = "+childNode.name+", type='"+type+"'")
				case type of
				(
					"proxy" 	: append aPhysProxies childNode
					"lod" 		: append aLODs childNode
					--"joint"	: 
					--"skip"	:
					"glass" 	: 
										(	
											messageBox ("ERROR: found a 'breakGlass' object under mesh node "+childNode.name)
											return false
										)
					"mesh"	: append aSubMeshes childNode
				)
			)
		),
		
		fn recursiveSnapAttach obj att =
		(
			attach obj (snapshotAsMesh att)								-- attach any sub-meshes of the LOD to our new object
			for child in att.children do
				recursiveSnapshotAttach obj child
			
			return obj
		),
		
		fn prepareExportObjs =
		(
			-- We will not export the actual original mesh, but instead make a duplicate of it. This way we don't need to destroy and re-create the user's complex hierarchy.
			local objSnap = mesh name:baseMesh.name		-- make an empty mesh
			objSnap.pivot = baseMesh.pivot							-- match the pivot to the original mesh
			attach objSnap (snapshotAsMesh baseMesh)		-- make a snapshot of original mesh and attach it to our new mesh
			
			for submesh in aSubMeshes do							-- if there are any sub-mesh geometries to merge..
			(
				objSnap = recursiveSnapAttach objSnap submesh		-- recursively attach each sub-mesh and any children
			)
			objSnap.material = baseMesh.material					-- assign original object's material to our new mesh
			objSnap.parent = exportNode								-- parent it to the export node
			baseMesh.parent = undefined							-- remove the original mesh (and its entire hierarchy) from the export node temporarily - will be restored after export

			append aSnapshots objSnap								-- keep track of all the snapshot meshes we make so we can delete them later
			
			for proxy in aPhysProxies do								-- for each proxy we found
			(
				proxy.parent = objSnap								-- parent it to our new temporary mesh
			)

			aTempExportNodes[1] = exportNode
			for i=1 to aLODs.count do
			(
				local lodSnap = mesh name:aLODs[i].name										-- create an empty mesh
				lodSnap.pivot = aLODs[i].pivot														-- match the pivot to the main lod mesh
				attach lodSnap (snapshotAsMesh aLODs[i])										-- attach the main lod mesh to our new object
				
				append aSnapshots lodSnap															-- keep track of our snapshot meshes to delete later
				
				for childObj in aLODs[i].children do
				(
					lodSnap = recursiveSnapAttach lodSnap childObj
				)
				lodSnap.material = aLODs[i].material												-- apply the original material
				
				local newLodDummy = copy exportNode												-- make a copy of the original top-node dummy, to use as an export dummy for this LOD
				newLodDummy.name = (exportNode.name + "_LOD" + (i as string))	-- add the _LODx suffix to the top-node name
				append aTempLODNodes newLodDummy											-- keep track of it, to delete later
				lodSnap.parent = newLodDummy												-- parent our new temp LOD mesh to this new dummy
				lodSnap.name = baseMesh.name													-- rename the mesh to match the original object mesh name
				append aTempExportNodes newLodDummy										-- keep track of which export nodes we need to export
					
				aLODs[i].parent = undefined
			)
		),
		
		fn export =
		(
			local origMaxFilePath = maxFilePath
			local origMaxFileName = maxFileName

			-- save current max scene to a temp folder, so we can export out of the view of the Editor (which may be open)
			try(makedir (origMaxFilePath + "_LODtemp")) catch()
			saveMaxFile (origMaxFilePath + "_LODtemp/" + origMaxFileName) clearNeedSaveFlag:false quiet:true
			
			local tempMaxFilePath = maxFilePath	
			
			-- make a copy of the original max scene file so we can restore it later
			copyFile (origMaxFilePath + origMaxFileName) (origMaxFilePath + origMaxFileName + "_cgfexport")

			try (csexport.export.set_node_list (aTempExportNodes)) 
				catch (
					msg = ""
					for a in aTempExportNodes do
						msg = (msg + a.name + "\n")
					messageBox ("UNKOWN ERROR: Cannot add these objects to the export list:\n"+msg)
					return false
				)
			try (csexport.export.export_nodes())
				catch (
					messageBox ("UNKNOWN ERROR calling csexport.export.export_nodes()")
					return false
				)
			
			for expNode in aTempExportNodes do
			(
				cgf = expNode.name
				cgfFilename = tempMaxFilePath + cgf + ".cgf"
				if doesFileExist(cgfFilename) then
				(
					rccmd = (projectRoot + "/bin32/rc/rc " + cgfFilename + " /refresh")
					--print rccmd

					try
					(HiddenDOSCommand ( rccmd ))	-- Max 2010
					catch
					(DOSCommand ( rccmd ))				-- Max 2008

					-- now copy the compiled export back to the original folder so Editor can see it
					try(deleteFile (origMaxFilePath + cgf + ".cgf")) 
						catch()
					--print ("copyFile "+cgfFilename+" "+(origMaxFilePath + cgf + ".cgf"))
					try(copyFile cgfFilename (origMaxFilePath + cgf + ".cgf")) 
						catch(print "ERROR! Could not copy file from " + cgfFilename + " TO " + (origMaxFilePath + cgf + ".cgf"))
					
					print ("EXPORTED: "+cgf+".cgf")
				) else (
					messageBox ("Script RC Fail - Cannot find exported file: "+cgfFilename)
					return false
				)
			)
			
			saveMaxFile (origMaxFilePath + origMaxFileName)
			deleteFile (origMaxFilePath + origMaxFileName)
			renameFile (origMaxFilePath + origMaxFileName + "_cgfexport") (origMaxFilePath + origMaxFileName)
			
			gc()
			
			return true
		),
		
		fn fnCleanup = 
		(
			-- now restore the original hierarchy, and delete any temporary snapshot meshes we made before
			baseMesh.name = sOrigBaseName
			for i=1 to aSnapshots.count do	-- delete any snapshot meshes we created
			(
				delete aSnapshots[i]
			)
			for i=1 to aLODs.count do
			(
				aLODs[i].parent = baseMesh		-- parent the original LOD meshes back to the original mesh
				delete aTempLodNodes[i]			-- delete our temporary LOD mesh
			)
			for proxy in aPhysProxies do
			(
				proxy.parent = baseMesh		-- parent the proxies back to the original mesh
			)
			baseMesh.parent = exportNode
			for obj in aChildNodes do			-- restore original hierarchy of objects parented to the exportNode
			(
				--print ("Restoring "+obj.name+" to exportNode")
				obj.parent = exportNode
			)
		)

	)
		
fn fnC2ExportCGFs projectRoot mode = 
(
	local origMaxFileName = maxFileName
	
	local exportNodes = #()
	local cleanExportNodes = #()
	local tempExportNodes = #()
	
	-- If Max file is not saved yet, then we don't have a valid export location
	if (origMaxFileName != ".max" or origMaxFileName != "") then
	(
		--messageBox "You must Load or Save a valid Max scene first."
		--return false
	)

	-- open the Exporter panel, or we can't access the export node list
	try(
		UtilityPanel.OpenUtility csExport
	) catch (
		messageBox "ERROR: CryExporter Panel cannot be accessed. - Try opening the Exporter panel manually and re-run script."
		return false
	)
	
	local cleanBackupExportList = #()
	local backupExportList = csexport.export.get_node_list()
	for i=1 to backupExportList do
	(
		if backupExportList[i] != undefined then
			append cleanBackupExportList backupExportList[i]
	)
	
	if mode == "all" then
		exportNodes = csexport.export.get_node_list()
	else
	(
		local expNodeList = csexport.export.get_node_list()
		for thisObj in selection do
		(
			local useThisObj = thisObj
			while useThisObj.parent != undefined do
			(
				useThisObj = useThisObj.parent
			)
			if (findItem expNodeList useThisObj) > 0 then
				appendIfUnique exportNodes useThisObj
		)
	)

	-- sometimes the export list gets messed up, with "deleted items" mixed in. So, check each item and get rid of these undefined ones
	for i=1 to exportNodes.count do
	(
		if exportNodes[i] != undefined then
		(
			append cleanExportNodes exportNodes[i]
		)
	)
	if (exportNodes.count != cleanExportNodes.count) then
	(
		csexport.export.set_node_list(#())
		csexport.export.set_node_list(cleanExportNodes)
	)
	-- now we have a clean export list with only exportable nodes
	exportNodes = cleanExportNodes
	
	if exportNodes.count == 0 then				-- if export list is empty, error out.
	(
		messageBox "Nothing in Export list."
		return false
	)

	for exportNode in exportNodes do			-- loop through each export node
	(
		print "----------------------------------------------------"
		print ("EXPORT NODE: "+exportNode.name)
		thisCryExportNode = stCryExportNode exportNode:exportNode projectRoot:projectRoot
		
		if (thisCryExportNode.getExportNodeChildren() != false) then
		(
			if (thisCryExportNode.getSubmeshes() != false) then
			(
				thisCryExportNode.prepareExportObjs()
				thisCryExportNode.export()									-- export all of the export nodes
				thisCryExportNode.fnCleanup()
			)
		)
	)

	csexport.export.set_node_list(#())						-- clear out the export list (we added LODs to it temporarily)
	csexport.export.set_node_list(cleanBackupExportList)	-- restore it back to original export list before we started
	
	gc()
)

fn fnC2CleanupOldLODs projectRoot mode = 
(
	local topObjs = #()
	local selectedTops = #()
	local myObjects = #()
	
	fn getLods topObjs objName = 
	(
		local foundLODs = #()
		
		lodObj = undefined
		suffixPos = findstring objName "_LOD"
		rootname = substring objName 1 (suffixPos-1)
		print ("Searching objects for LOD "+(lvl as string)+" matching rootname: "+rootname)
		
		for obj in topObjs do
		(
			--print ("Checking "+obj.name+"...")
			if isValidNode obj then
			(
				if (obj.name == rootname) then
				(
					print ("Found LOD0: " + obj.name)
					foundLODs[1] = obj
				) else (
					--print ("Checking "+obj.name+"...")
					local thisSufPos = findstring obj.name "_LOD"
					if thisSufPos != undefined then
					(
						thisRoot = substring obj.name 1 (thisSufPos-1)
						if (thisRoot == rootname) then
						(
							lodnum = (substring obj.name obj.name.count 1) as number
							--print ("lodnum = "+(lodnum as string))
							if lodnum != undefined then
							(
								print ("Found LOD["+((lodnum+1) as string)+"] : "+obj.name)
								foundLODs[lodnum+1] = obj
							)
						)
					)
				)
			)
		)
		
		return foundLODs
	)
	
	for obj in objects do
	(
		if obj.parent == undefined then
			append topObjs obj
	)

	if mode=="all" then
	(
		selectedTops = topObjs
	) else (
		local selectedObjs = selection
		for thisObj in selectedObjs do
		(
			local useThisObj = thisObj
			while useThisObj.parent != undefined do
			(
				useThisObj = useThisObj.parent
			)
			appendIfUnique selectedTops useThisObj
		)
	)
	
	local found = #()
	i = 0
	for topObj in selectedTops do
	(
		i+=1
		if isValidNode topObj then
		(
			if (findString topObj.name "_LOD") != undefined then
			(
				if found[i] != true then
				(
					local lods = getLODs topObjs topObj.name
					
					for x = 1 to lods.count do
					(
						for y = 1 to topObjs.count do
						(
							if (isValidNode lods[x] and isValidNode topObjs[y]) then
							(
								if lods[x].name == topObjs[y].name then
								(
									found[y] = true
								)
							)
						)
					)
							
					--print "Found LODs: "
					cryExportNodeLOD0 = undefined
					for x=1 to lods.count do
					(
						if (isValidNode lods[x]) == false then
							continue
						
						print (((x-1) as string)+": "+lods[x].name)
						if x==1 then
						(
							strPos = findString lods[x].name "_LOD"
							if strPos != undefined then
							(
								lods[x].name = substring lods[x].name 1 (strPos-1)
							)
						)

						thisCryExportNode = stCryExportNode exportNode:lods[x] projectRoot:projectRoot
						if (thisCryExportNode.getExportNodeChildren() != false) then
							thisCryExportNode.getSubmeshes()
						
						if x == 1 then (
							cryExportNodeLOD0 = thisCryExportNode
						) else (
							if cryExportNodeLOD0 != undefined then
							(
								if cryExportNodeLOD0.baseMesh != undefined then
								(
									thisCryExportNode.baseMesh.parent = cryExportNodeLOD0.baseMesh
									thisCryExportNode.baseMesh.name = ("$"+"lod"+((x-1) as string))
									thisCryExportNode.baseMesh.transform = cryExportNodeLOD0.baseMesh.transform
									if (thisCryExportNode.baseMesh != lods[x]) then
										delete lods[x]
								)
							)
						)
					)
				)
			)
		)
	)
)

rollout rltCryLODExporter_rollout "CGF Split-LOD Combiner" width:190
(

	local projectRoot = csexport.get_root_path()
	
	--button btn_c2ExportLODs "EXPORT LODs" width:180
	button btn_c2CleanAllOldLODs "Combine Split LODs - ALL" width:170 align:#center tooltip:"Convert all separate LODs into combined LOD structure"
	button btn_c2CleanSelOldLODs "Combine Split LODs - SELECTED" width:170 align:#center tooltip:"Convert selected separate LODs into combined LOD structure"
	button btn_c2ExportCGFs "EXPORT ALL" enabled:false width:170 align:#center tooltip:"Export compound objects with embedded LODs into separate LOD files - from Export List"
	button btn_c2ExportSelCGFs "EXPORT SELECTED" enabled:false width:170 align:#center tooltip:"Export compound objects with embedded LODs into separate LOD files - from Export List"
	
	on btn_c2CleanAllOldLODs pressed do
	(
		fnC2CleanupOldLODs projectRoot "all"
	)
	
	on btn_c2CleanSelOldLODs pressed do
	(
		fnC2CleanupOldLODs projectRoot "selected"
	)
	
	on btn_c2ExportCGFs pressed do
	(
		fnC2ExportCGFs projectRoot "all"
	)
	on btn_c2ExportSelCGFs pressed do
	(
		fnC2ExportCGFs projectRoot "selected"
	)

)

try(removeSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryLODExporter_rollout")) catch()
try(cryMaxTools.basic.ROMan.cryAdd "rltCryLODExporter_rollout" rltCryLODExporter_rollout #main) catch()
try(addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryLODExporter_rollout")) catch()
	