-------------------------------------------
-- CryMaxTools v2.0
-- Diagnostic Tools
-- by Chris Evans/Sascha Herfort/Marcus Krautwurst
---------------------------

rollout rltCryDiagnostics "Diagnostic Tools"
(
	group "Hierarchy Diagnostics"
	(
		button selectRoot "selectRoot" height:16 width:80 offset:[-2,-2] across:2 align:#center
		button selChildren "selectChildren" height:16 width:80 offset:[3,-2] align:#center
		button numChildren "numChildren" height:16 width:80 offset:[-5,3] across:3 align:#left
		label numChildrenLabel "---" offset:[16,3] align:#center
		checkbox printChildren "print" offset:[3,3] align:#right
		label compareLabel "Compare Two Hierarchies:" offset:[-5,3] align:#left
		checkbutton compare01 " Hierarchy 1" height:16 width:80 offset:[-2,-2] across:2 align:#center
		button compare02 " Hierarchy 2" height:16 width:80 offset:[3,-2] align:#center
		checkbox checkConsistency "check consistency" enabled:false offset:[-5,-5] align:#left
	)
	
	on compare01 changed state do
	(
		if $ == undefined then
		(
			messagebox "Please select a hierarchy member!"
			compare01.checked = false
			return undefined
		)
		
		if compare01.checked == true then
		(
			comp1root = cryMaxTools.findRoot $
			print (comp1root.name + " hierarchy loaded.")
			tempArray = cryMaxTools.getChildren comp1root
			global crytools_compareArray1 = #()
			for obj in tempArray do
			(
				append crytools_compareArray1 obj.name
			)
		)
	)
	
	on compare02 pressed do
	(
		if $ == undefined then
		(
			messagebox "Please select a hierarchy member!"
			return undefined
		)
		comp2root = cryMaxTools.findRoot $
		compareArray2 = cryMaxTools.getChildren comp2root
		print (comp2root.name + " heirarchy loaded.")
		isIdentical = "yes"
		for obj in compareArray2 do
		(
			if (findItem crytools_compareArray1 obj.name) == 0 do
			(
				print obj.name
				isIdentical = "no"
			)
		)
		if isIdentical == "yes" then
		(
			print "hierarchy elements are identical"
		)
	)
	
	--	numChildren
	on numChildren pressed do
	(
		if $ == undefined or selection.count > 1 do
		(
			print "Select 1 node."
			numChildrenLabel.text = "--"
			return undefined
		)
		comp2root = cryMaxTools.findRoot $
		childArray = cryMaxTools.getChildren comp2root
		numChildrenLabel.text = (childArray.count as string)
		if printChildren.checked == true then
		(
			for obj in childArray do print obj.name
		)
	)
	
	on selChildren pressed do
	(
		selectMe = selection as array
		for obj in selection do
		(
			join selectMe (cryMaxTools.getChildren obj)
		)
		select selectMe
	)
	
	on selectRoot pressed do
	(
		if selection.count > 1 then
		(
			messagebox "Please select one node of the hierarchy"
			return undefined
		)
		if $ == undefined or $.parent == undefined then
		(
			print "Object has no parent or nothing is selected"
			return undefined
		)
		root_ = cryMaxTools.findRoot $
		select root_
	)
)
--add diagnostics tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryDiagnostics" rltCryDiagnostics #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryDiagnostics")