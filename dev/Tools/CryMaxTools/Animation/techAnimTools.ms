------------------------------------
-- CryMaxTools v2.0
-- Technical Animator Tools v1.0
-- by Sascha Herfort
------------------------------------

rollout rltCryTechAnimTools "Tech Anim Tools" width:190
(
	group "Copy with Keys"
	(
		button btnCopyWithKeys "Copy with keys" width:80 height:16 across:2 align:#center offset:[-1,-2] tooltip:"Copy selected node with a key per frame."
		checkBox chkCopyWithKeys "Reparent to:" align:#center offset:[6,-2] checked:false
		dropDownList ddlCopyWithKeys width:160 offset:[-3,-5] height:30
	)
	
	group "Keyframe Tools"
	(
		button btnSetKeyPerFrame "Set key per frame" width:160 height:16 align:#center offset:[-1,-2] tooltip:"Sets a transform key on each frame."
		button btnReduceKeys "Reduce Keys" width:80 height:16 align:#center across:2 offset:[-3,-5] toolTip:"Reduce keys of selected nodes."
		spinner spnReduceKeysThreshold width:80 height:16 align:#center offset:[2,-5]
		button btnBlurRotation "Blur Rotation" width:80 height:16 align:#center across:2 offset:[-3,-5]
		button btnBlurPosition "Blur Position" width:80 height:16 align:#center offset:[2,-5]
	)

---------------------	
--global variables--
---------------------
	
----------------------
--global functions--
----------------------
	fn fnCopyWithKeys =
	(--copy a node with a key per frame
		undo "Copy with keys" on
		(
			aNodes = $selection as array
			clearSelection()
			
			for sNode in aNodes do
			(
				local aKeys = #()
				local iIndex = 1
				for i = (animationRange.start.frame as integer) to (animationRange.end.frame as integer) do
				(
					at time i
					(
						append aKeys sNode.transform
					)
				)
				
				local sNewNode = snapshot sNode
				if chkCopyWithKeys.state then
				(
					sNewNode.parent = getnodebyname ddlCopyWithKeys.selected
				)
				
				for i = (animationRange.start.frame as integer) to (animationRange.end.frame as integer) do
				(
					with animate on
					(
						at time i
						(
							sNewNode.transform = aKeys[iIndex]
						)
					)
					iIndex += 1
				)
				
				selectMore sNewNode
			)
		)
	)
	
	fn fnSetKeyPerFrame  =
	(--sets a key on each frame
		--if bPrintFunctionNames then (print "fnSetKeyPerFrame")
		
		if $selection != undefined then
		(
			undo "Set key per frame" on
			(
				for each in $selection do
				(
					animate on
					(
						nKeyHelper = point()
						for i = (animationrange.start.frame as integer) to (animationrange.end.frame as integer) do
						(
							at time i
							(
								nKeyHelper.pos = [0,0,0]
								nKeyHelper.rotation = each.rotation
								nKeyHelper.pos = each.pos
							)
						)
						for i = (animationrange.start.frame as integer) to (animationrange.end.frame as integer) do
						(
							at time i
							(
								each.pos = [0,0,0]
								each.rotation = nKeyHelper.rotation
								each.pos = nKeyHelper.pos
							)
						)
						delete nKeyHelper
					)
				)
			)
		)
	)
	
	fn fnReduceKeys fThreshold bPos:false bRot:false bScale:false bBiped:false =
	(--deletes keys from a node - more threshold = more keys to remove
		--if bPrintFunctionNames then (print "fnReduceKeys")
		
		if $selection != undefined then
		(
			undo "Remove keys" on
			(
				for each in $selection do
				(
					for i = (animationrange.start.frame as integer) to (animationrange.end.frame as integer) do
					(
						at time i
						(
							if each.transform.controller.keys.count > 0 then
							(
								local iKey = getKeyIndex each.transform.controller i
								if iKey != 0 then
								(--has a key
									local m3CurrentTransform = each.transform
									
									if bBiped then
									(--if reduce rotation keys
										deleteKey each.transform.controller iKey
										if abs(((inverse each.transform.rotationPart)*(m3CurrentTransform.rotationPart)).angle) > fThreshold then
										(--if rotation difference larger than threshold, set new key
											animate on
											(
												each.transform = m3CurrentTransform
											)
										)
									)
								)
							)
							else
							(
								local iKey = getKeyIndex each.rotation.controller i
								if iKey != 0 then
								(--has a key
									local m3CurrentTransform = each.rotation
									
									if bBiped then
									(--if reduce rotation keys
										deleteKey each.transform.controller iKey
										if abs(((inverse each.transform.rotationPart)*(m3CurrentTransform)).angle) > fThreshold then
										(--if rotation difference larger than threshold, set new key
											animate on
											(
												each.rotation = m3CurrentTransform
											)
										)
									)
								)
							)
						)
					)
				)
			)
		)
	)
	
	fn fnBlurRotation iWidth =
	(--blur rotation
		--if bPrintFunctionNames then (print "fnBlurRotation")
		
		if $selection != undefined then
		(
			undo "Blur Rotation" on
			(
				for each in $selection do
				(
					local aTransforms = #()
					for i = (animationrange.start.frame as integer - iWidth) to (animationrange.end.frame as integer + iWidth) do
					(
						if i > 0 then
						(
							at time i
							(
								aTransforms[i] = each.transform
							)
						)
					)
					
					for i = (animationrange.start.frame as integer) to (animationrange.end.frame as integer) do
					(
						local iKey = getKeyIndex each.transform.controller i
						--if iKey != 0 and isKeySelected each.transform.controller iKey then
						(
							local quatNewRotation = undefined
							local fWeight = 1/((iWidth*2+1) as float)
							local iIteration = 1
							for j = (i - iWidth) to (i + iWidth) do
							(
								if j > 0 then
								(
									if aTransforms[j] == undefined then
									(
										local qCurrentRot = aTransforms[i].rotationPart
									)
									else
									(
										local qCurrentRot = aTransforms[j].rotationPart
									)
								)
								else
								(
									local qCurrentRot = aTransforms[1].rotationPart
								)
								
								if iIteration == 1 then
								(
									quatNewRotation = qCurrentRot*fWeight
								)
								else
								(
									quatNewRotation += qCurrentRot*fWeight
								)
								iIteration += 1
							)
							
							--print (aTransforms[i].rotationPart as string + " " + (at time i(each.transform.rotationPart)) as string)
							
							
							animate on
							(
								at time i
								(
									local m3NewTransform = (normalize quatNewRotation) as matrix3
									m3NewTransform.translation = aTransforms[i].translationPart
									each.transform = m3NewTransform
								)
							)
							
						)
					)
				)
			)
		)
	)
	
	fn fnBlurPosition iWidth =
	(--blur position
		--if bPrintFunctionNames then (print "fnBlurRotation")
		
		if $selection != undefined then
		(
			undo "Blur Position" on
			(
				for each in $selection do
				(
					local aTransforms = #()
					for i = (animationrange.start.frame as integer - iWidth) to (animationrange.end.frame as integer + iWidth) do
					(
						if i > 0 then
						(
							at time i
							(
								aTransforms[i] = each.transform
							)
						)
					)
					
					for i = (animationrange.start.frame as integer) to (animationrange.end.frame as integer) do
					(
						local iKey = getKeyIndex each.transform.controller i
						--if iKey != 0 and isKeySelected each.transform.controller iKey then
						(
							local p3NewPosition = undefined
							local fWeight = 1/((iWidth*2+1) as float)
							local iIteration = 1
							for j = (i - iWidth) to (i + iWidth) do
							(
								local p3CurrentPos = undefined
								if j > 0 then
								(
									if aTransforms[j] == undefined then
									(
										p3CurrentPos = aTransforms[i].translationpart 
									)
									else
									(
										p3CurrentPos = aTransforms[j].translationpart 
									)
								)
								else
								(
									p3CurrentPos = aTransforms[1].translationpart 
								)
								
								if iIteration == 1 then
								(
									p3NewPosition = p3CurrentPos*fWeight
								)
								else
								(
									p3NewPosition += p3CurrentPos*fWeight
								)
								iIteration += 1
							)
							
							--print (aTransforms[i].rotationPart as string + " " + (at time i(each.transform.rotationPart)) as string)
							
							animate on
							(
								at time i
								(
									local m3NewTransform = aTransforms[i]
									m3NewTransform.translation = p3NewPosition
									each.transform = m3NewTransform
								)
							)
						)
					)
				)
			)
		)
	)
------------------------
--UI event handlers--
------------------------
	on btnCopyWithKeys pressed do
	(--copy selected node with keys
		fnCopyWithKeys()
	)
	
	on chkCopyWithKeys changed state do
	(--update list when checkbox true
		if state then
		(
			local aItems = for each in $objects collect (each.name as name)
			fn fnSort n1 n2 =
			(
				case of
				(
					(n1 > n2): 1
					(n1 < n2): -1
					default: 0
				)
			)
			qsort aItems fnSort
			ddlCopyWithKeys.items = aItems
		)
	)
	
	on btnSetKeyPerFrame pressed do
	(--set a key per frame
		fnSetKeyPerFrame()
	)
	
	on btnReduceKeys pressed do
	(--reduce keys
		fnReduceKeys spnReduceKeysThreshold.value bBiped:true
	)
	
	on btnBlurRotation pressed do
	(--blur rotation
		fnBlurRotation 2
	)
	
	on btnBlurPosition pressed do
	(--blur position
		fnBlurPosition 2
	)
)

--add rollouts to crytoolbox
cryMaxTools.basic.ROMan.cryAdd "rltCryTechAnimTools" rltCryTechAnimTools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryTechAnimTools")