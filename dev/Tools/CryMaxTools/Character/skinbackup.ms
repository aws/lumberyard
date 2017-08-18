with redraw off
(
	with undo on
	(
		aPreSel = selection as array 				
		aPreSubLevel = subobjectlevel
		pObjects = objects as array
		pSkinnedObjects = #()
		pSkinDataObjects = #()
		
	
		-- Find Skinned Objects
		max modify mode
		for i = 1 to pObjects.count do
		(
			if classOf pObjects[i].modifiers[1] == Skin then
			(
				append pSkinnedObjects pObjects[i]
			)
		)

		try(SkinDataLayer = layermanager.newLayerFromName "skindata")catch()
		try(SkinDataLayer = layermanager.getLayerFromName "skindata")catch()
		-- Delete the objects in the layer
		tempNodes = #()
		SkinDataLayer.nodes &tempNodes
		for i = 1 to tempNodes.count do
		(
			delete tempNodes[i]
		)

		--Backup SkinData
		for i = 1 to pSkinnedObjects.count do
		(
			skinUtils.ExtractSkinData pSkinnedObjects[i]	
			try(append pSkinDataObjects (getNodeByName ("SkinData_" + pSkinnedObjects[i].name)))catch()
		)

		for obj = 1 to pSkinDataObjects.count do
		(
			SkinDataLayer.addnode pSkinDataObjects[obj]
		)
		
		for i = 1 to aPreSel.count do
		(
			selectMore aPreSel[i]
		)
-- 		subobjectlevel = aPreSubLevel
	)
)