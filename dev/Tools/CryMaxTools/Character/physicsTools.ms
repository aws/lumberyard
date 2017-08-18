-------------------------------------------
-- CryMaxTools v2.0
-- Physics Tools
-- by Chris Evans/Sascha Herfort/Marcus Krautwurst
---------------------------

rollout rltCryPhysics "Physics Setup Tools"
(	
---------------------
-- Phys Mesh Setup
---------------------
	group "Phys Setup"
	(
		spinner spnPhysDefaultLimit "Limits:" range:[0,90,10] type:#float width:76 offset:[-5,0] across:2 align:#left
		spinner spnPhysDefaultDamping "Damping:" range:[0,1,1] type:#float offset:[-4,0] align:#left
		button createPhys "Create Phys Skeleton" height:16 width:160 offset:[0,-5]
		
		button createParentFrame "Create ParentFrame" height:16 width:160 offset:[0,3]
	)
	
	group "Physproxy Primitives"
	(
		label lblInfo1 "-create proxy and adjust" align:#left
		label lblInfo2 "-select bone+proxy, hit 'merge'" align:#left offset:[0,-2]
		
		checkbox chkAutoMerge "Auto Merge" checked:false offset:[-5,3]
		button btnAddBox "Add Box" height:16 width:80 offset:[-5,-5] align:#left across:2
		button btnAddSphere "Add Sphere" height:16 width:80 align:#right offset:[5,-5]
		button btnAddCylinder "Add Cylinder" height:16 width:80 offset:[-5,-5] align:#left across:2
		button btnAddCapsule "Add Capsule" height:16 width:80 align:#right offset:[5,-5]
		
		button btnMakeEditPoly "Make instanced Edit Poly" height:16 width:160 offset:[1,3]
		button btnMerge "Merge Proxy to Bone" height:16 width:160 offset:[0,-5]
	)
	
	fn fnFindParentInSelection aSelection sNode =
	(--returns next higher item in hierarchy that is also selected (parent of parent ...)
		local sParent = "notfound"
		local sCurNode = sNode
		while sParent == "notfound" do
		(
			sParent = sCurNode.parent
			if (findItem aSelection sParent) == 0 and sParent != undefined then
			(
				sCurNode = sParent
				sParent = "notfound"
			)
		)
		sParent
	)
	
	on createPhys pressed do
	(
		if $ != undefined then
		(--create phys switch for selected bones
			undo "Make Phys Skeleton" on
			(
				aMySelection = getcurrentselection()
				aNewNodes = #()
				for sObj in aMySelection do
				(
					sNewNode = snapshot sObj
					sNewNode.name = (sObj.name + " phys")
					sNewNode.parent = fnFindParentInSelection aMySelection sObj
					append aNewNodes sNewNode
				)
				for sObj in aNewNodes do
				(
					if sObj.parent != undefined then
					(
						sObj.parent = getnodebyname (sObj.parent.name + " phys")
						ik.setAxisActive sObj #rotational #{1,2,3}
					)
					else
					(
						ik.setAxisActive sObj #rotational #{}
					)
					ik.setAxisLimit sObj #rotational #{1,2,3}
					ik.setAxisMin sObj #rotational [-spnPhysDefaultLimit.value,-spnPhysDefaultLimit.value,-spnPhysDefaultLimit.value]
					ik.setAxisMax sObj #rotational [spnPhysDefaultLimit.value,spnPhysDefaultLimit.value,spnPhysDefaultLimit.value]
					ik.setAxisDamping sObj #rotational [spnPhysDefaultDamping.value,spnPhysDefaultDamping.value,spnPhysDefaultDamping.value]
				)
				select aNewNodes
			)
		)
	)
	
	on createParentFrame pressed do
	(
		if selection != undefined then
		(
			undo "createParentFrame" on
			(
				for obj in selection do
				(
					frame = snapshot obj name:((obj.name as string) + " parentFrame")
					frame.parent = obj.parent
					obj.parent = frame
					--frame.ishidden = true
					--frame.isfrozen = true
				)
			)
		)
	)
	
	fn fnGetBoundingBoxStats sNode =
	(--returns local size and pivot of boundingbox ... dirty hack!
		p3CurPivot = sNode.transform.position
		mInverseNodeTransforms = inverse sNode.transform
		aCurBoundingBox = (nodeLocalBoundingBox sNode)
		p3Min = aCurBoundingBox[1]*mInverseNodeTransforms
		p3Max = aCurBoundingBox[2]*mInverseNodeTransforms
		p3Size = [p3Max.x - p3Min.x,p3Max.y - p3Min.y,p3Max.z - p3Min.z]
		p3RelativePivot = -[-(p3Max.z + p3Min.z)*.5,(p3Max.y + p3Min.y)*.5,p3Min.x] --pivot relative to new primitive pivot ... note:rotated around Y 90degrees
		#(p3Size,p3RelativePivot) --returns size, relative pivot and min/max corners
	)
	
	fn fnMergeProxyToBone sBone sProxy =
	(--merges a physproxy into a bone, using and edit poly modifier
		max modify mode
		if sBone.modifiers["Edit Poly"] == undefined then
		(--get edit poly modifier or create it
			sMyModifier = Edit_Poly()
			addModifier sBone sMyModifier
		)
		sMyModifier = sBone.modifiers["Edit Poly"]
		modPanel.setCurrentObject sMyModifier ui:false node:sBone --important: node: must be defined when working with instanced modifiers
		sMyModifier.setEPolySelLevel  #Face
		sMyModifier.setSelection #Face #{1..getNumFaces sBone}
		sMyModifier.currentOperation = 11 --deletes selected faces
		sMyModifier.attach sProxy
		sMyModifier.SetEPolySelLevel #Object
	)
	
	on btnAddBox pressed do
	(
		if $ != undefined then
		(
			undo "Add Box Physproxy" on
			(
				aSelectedBones = #()
				aNewNodes = #()
				for sObj in $selection do append aSelectedBones sObj
				for sObj in aSelectedBones do
				(
					print sObj
					aBoundingBoxStats = fnGetBoundingBoxStats sObj
					sBox = box length:(aBoundingBoxStats[1].y) width:(aBoundingBoxStats[1].z) height:(aBoundingBoxStats[1].x) --this is dirty ... bones look at X, new primitives at Z
					sBox.pivot = aBoundingBoxStats[2]
					sBox.parent = sObj
					AlignToParent sBox
					sBox.position = sObj.transform.position
					in coordsys local rotate sBox (eulerangles 0 90 0)
					sBox.name = (sObj.name as string + " physproxy")
					if chkAutoMerge.checked then
					(
						fnMergeProxyToBone sObj sBox
					)
					else
					(
						append aNewNodes sBox
					)
				)
				if chkAutoMerge.checked == false then select aNewNodes
				else select aSelectedBones
			)
		)
	)
	
	on btnAddSphere pressed do
	(
		if $ != undefined then
		(
			undo "Add Sphere Physproxy" on
			(
				aSelectedBones = #()
				aNewNodes = #()
				for sObj in $selection do append aSelectedBones sObj
				for sObj in aSelectedBones do
				(
					aBoundingBoxStats = fnGetBoundingBoxStats sObj
					sSphere = sphere radius:(aBoundingBoxStats[1].x*.5) recenter:true --this is dirty ... bones look at X, new primitives at Z
					sSphere.pivot = aBoundingBoxStats[2]
					sSphere.parent = sObj
					AlignToParent sSphere
					sSphere.position = sObj.transform.position
					in coordsys local rotate sSphere (eulerangles 0 90 0)
					sSphere.name = (sObj.name as string + " physproxy")
					if chkAutoMerge.checked then
					(
						fnMergeProxyToBone sObj sSphere
					)
					else
					(
						append aNewNodes sSphere
					)
				)
				if chkAutoMerge.checked == false then select aNewNodes
				else select aSelectedBones
			)
		)
	)
	
	on btnAddCylinder pressed do
	(
		if $ != undefined then
		(
			undo "Add Cylinder Physproxy" on
			(
				aSelectedBones = #()
				aNewNodes = #()
				for sObj in $selection do append aSelectedBones sObj
				for sObj in aSelectedBones do
				(
					aBoundingBoxStats = fnGetBoundingBoxStats sObj
					sCylinder = cylinder height:(aBoundingBoxStats[1].x) radius:((aBoundingBoxStats[1].y+aBoundingBoxStats[1].z)*.25) --this is dirty ... bones look at X, new primitives at Z
					sCylinder.pivot = aBoundingBoxStats[2]
					sCylinder.parent = sObj
					AlignToParent sCylinder
					sCylinder.position = sObj.transform.position
					in coordsys local rotate sCylinder (eulerangles 0 90 0)
					sCylinder.name = (sObj.name as string + " physproxy")
					if chkAutoMerge.checked then
					(
						fnMergeProxyToBone sObj sCylinder
					)
					else
					(
						append aNewNodes sCylinder
					)
				)
				if chkAutoMerge.checked == false then select aNewNodes
				else select aSelectedBones
			)
		)
	)
	
	on btnAddCapsule pressed do
	(
		if $ != undefined then
		(
			undo "Add Capsule Physproxy" on
			(
				aSelectedBones = #()
				aNewNodes = #()
				for sObj in $selection do append aSelectedBones sObj
				for sObj in aSelectedBones do
				(
					aBoundingBoxStats = fnGetBoundingBoxStats sObj
					sCapsule = capsule height:(aBoundingBoxStats[1].x) radius:((aBoundingBoxStats[1].y+aBoundingBoxStats[1].z)*.25) heighttype:0 --this is dirty ... bones look at X, new primitives at Z
					sCapsule.pivot = aBoundingBoxStats[2]
					sCapsule.parent = sObj
					AlignToParent sCapsule
					sCapsule.position = sObj.transform.position
					in coordsys local rotate sCapsule (eulerangles 0 90 0)
					sCapsule.name = (sObj.name as string + " physproxy")
					if chkAutoMerge.checked then
					(
						fnMergeProxyToBone sObj sCapsule
					)
					else
					(
						append aNewNodes sCapsule
					)
				)
				if chkAutoMerge.checked == false then select aNewNodes
				else select aSelectedBones
			)
		)
	)
	
	on btnMakeEditPoly pressed do
	(--create edit poly instanced over all selected nodes
		if $ != undefined then
		(
			undo "Make instanced Edit Poly" on
			(
				aSelectedObjs = #()
				for sObj in $selection do append aSelectedObjs sObj
				sMyModifier = Edit_Poly()
				for sObj in aSelectedObjs do
				(
					addModifier sObj sMyModifier
				)
			)
		)
	)
	
	on btnMerge pressed do
	(--merge physproxy to bone
		if $ != undefined then
		(
			undo "Merge Physproxy to Bone" on
			(
				if $selection.count == 2 then
				(--match bone and proxy by hierarchy
					if $selection[1].parent == $selection[2] then
					(--find bone and physproxy by hierarchy
						sBone = $selection[2]
						fnMergeProxyToBone $selection[2] $selection[1]
						select sBone
					)
					else if $selection[2].parent == $selection[1] then
					(
						sBone = $selection[1]
						fnMergeProxyToBone $selection[1] $selection[2]
						select sBone
					)
					else
					(--if physproxy is not parented to bone, abort
						messagebox "Physproxy is not parented to according bone!"
					)
				)
				else
				(--match bone and proxy by name
					aSelectedObjs = #()
					aBones = #()
					aProxies = #()
					for sObj in $selection do append aSelectedObjs sObj
					for sObj in aSelectedObjs do
					(
						iBaseNameEnd = findString sObj.name "physproxy"
						if iBaseNameEnd != undefined then
						(
							sBone = undefined
							iBoneInSelection = findItem aSelectedObjs (getNodeByName (substring sObj.name 1 (iBaseNameEnd-2)))
							if iBoneInSelection > 0 then
							(
								sBone = aSelectedObjs[iBoneInSelection]
							)
							sProxy = sObj
							if sBone != undefined then
							(
								append aBones sBone
								append aProxies sProxy
							)
						)
					)
					for i = 1 to aBones.count do
					(
						fnMergeProxyToBone aBones[i] aProxies[i]
					)
				)
			)
		)
	)
	
----------------------
-- Rope Setup Begin
----------------------
	group "Rope Setup"
	(
		----- INTERFACE----
		label lblCreateSegDescr1"select a number and define rope"
		label lblCreateSegDescr2"creates rope from selected to end"
		button btnCreateSeg "Define Rope Segments" width:150 height:16 offset:[0,-2] 
		checkbutton btnCreateSeg1 "1" width:16 height:16 offset:[-3,-5] across:6
		checkbutton btnCreateSeg2 "2" width:16  height:16 offset:[-12,-5]
		checkbutton btnCreateSeg3 "3" width:16  height:16 offset:[-21,-5]
		checkbutton btnCreateSeg4 "4" width:16  height:16 offset:[-30,-5]
		checkbutton btnCreateSeg5 "5" width:16 height:16 offset:[-39,-5]
		checkbutton btnCreateSeg6 "6" width:16  height:16 offset:[-48,-5]
		--slider sld_ "Rope Number" pos:[57,104] width:150 height:44 offset:[-100,0] range:[0,50,0] type:#integer ticks:1 
		checkbox chkRopeAutoPhys "Auto Create Phys Mesh" offset:[2,-2] triState:1
		button btnLine1 "" height:1 width:168 offset:[1,0]		
		spinner spnMaxTimeStep width:40 range:[-100,100,0.1] align:#left across:2 offset:[0,5]
		label lblmaxtimestep "Time Step" offset:[-45,7]
		spinner spnSegLimit width:40 range:[-100,100,50] align:#left across:2
		label lblSegLimit "Seg. Limit" offset:[-45,2]
		spinner spnRopeDamping width:40 range:[-100,100,1] align:#left across:2
		label lblDamping "Damping" offset:[-47,2]
		spinner spnFriction width:40 range:[-100,100,0.6] align:#left across:2
		label lblFriction "Friction" offset:[-51,2]
		spinner spnStiffness width:40 range:[-100,100,15] align:#left across:2
		label lblStiffness "Stiffness" offset:[-48,2]
		spinner spnStiffnessDecay width:40 range:[-100,100,0.2] align:#left across:2
		label lblStiffnessDecay "Decay" offset:[-52,2]
		button btnRopeSet "S e t" width:60 height:62 offset:[50,-130]
		button btnRopereset "R e s e t" width:60 height:62 offset:[50,-5]		
		label lblRopeColl "Collision" align:#left across:3
		checkbox RopeCollideCharacter "self" offset:[10,-2] triState:1
		checkbox RopeCollideEnvironment "world" offset:[10,-2]triState:1
	)	
	
	fn buttonStates state = --Function for the Rope Number Button States
	(
		for i = 1 to 6 do
			execute ("(cryMaxTools.basic.ROMan.get \"rltCryMaxToolBox\").rltToolHolder.rltCryTechArt.btnCreateSeg" + i as String + ".checked = " + state as String)
	)
	
	fn resetRopeValues = -- Function for the Reset Button (predefined values for the ropes)
	(
		spnMaxTimeStep.value = 0.1
		spnSegLimit.value = 50
		spnRopeDamping.value = 1
		spnFriction.value = 0.6
		spnStiffness.value = 15
		spnStiffnessDecay.value = 0.2
		RopeList = selection as array
		
		for i = 1 to RopeList.count do  -- Set the values to the according properties
		(
			ik.setAxisMax RopeList[i] #rotational [spnStiffness.value,spnStiffnessDecay.value,spnRopeDamping.value]
			ik.setAxisSpringTension RopeList[i] #rotational [(spnSegLimit.value)/50.0,(spnMaxTimeStep.value)/50.0,(spnFriction.value)/50.0]			
		)
	)
	
	local RopeActiveNumber = undefined		
	-- Button States for the Rope Numbers
	on btnCreateSeg1 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg1.checked = true
		RopeActiveNumber = 1 )
	on btnCreateSeg2 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg2.checked = true
		RopeActiveNumber = 2)
	on btnCreateSeg3 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg3.checked = true
		RopeActiveNumber = 3)
	on btnCreateSeg4 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg4.checked = true
		RopeActiveNumber = 4)
	on btnCreateSeg5 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg5.checked = true
		RopeActiveNumber = 5)
	on btnCreateSeg6 changed val do	(
	  if val == true then buttonStates false
		btnCreateSeg6.checked = true
		RopeActiveNumber = 6)
			
	on btnCreateSeg pressed do
	(			
		if RopeActiveNumber != undefined then
			(
				childArray = cryMaxTools.getChildren $selection[1]
				FirstRopeName = (("Rope") + RopeActiveNumber as String + (" Seg01"))
				$selection[1].name = FirstRopeName as string					
				try
				(
					for i = 1 to (childArray.count + 1) do
					(
						OtherRopeName = (("Rope") + RopeActiveNumber as String + (" Seg0") + (i+1) as string)						
						ChildArray[i].name = OtherRopeName as string
					)
				)
				catch()
			)
			else
				(
					messagebox "Select a rope number first" title:"No rope number defined"
				)
			if chkRopeAutoPhys.checked = true then
			(
				RopePhysCopy = snapShot $
				RopePhysCopy.parent = undefined
				RopePhysCopy.name = (FirstRopeName + (" phys"))
				RopePhysCopy.wirecolor = red
				select RopePhysCopy					
			)
	)
	--- Variable Definitions
	on btnRopeSet pressed do 
	(
		RopeList = selection as array
		for i = 1 to RopeList.count do 
			(
					if RopeCollideCharacter.triState == 1 then 
					(
						ik.setAxisLimit RopeList[i] #rotational #{1,2,3}
					)						
					if RopeCollideEnvironment.triState == 1 then 
					(
						ik.setAxisLimit RopeList[i] #rotational #{1,2,3}
					)					
				ik.setAxisMax RopeList[i] #rotational [spnStiffness.value,spnStiffnessDecay.value,spnRopeDamping.value]
				ik.setAxisSpringTension RopeList[i] #rotational [(spnSegLimit.value)/50.0,(spnMaxTimeStep.value)/50.0,(spnFriction.value)/50.0]
				setUserPropBuffer RopeList[i] "active_phys gravity"
			)
	)
--- Reset all Values
	on btnRopeReset pressed do resetRopeValues ()
)
--add physics tools rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryPhysics" rltCryPhysics #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryPhysics")