------------------------------------
-- CryMaxTools v2.0
-- Basic Character Module v1.0
-- by Mathias Lindner
------------------------------------



struct cryMaxToolsCharStruct
(--character tools struct
	alignNode
)

cryMaxTools.char = cryMaxToolsCharStruct()





cryMaxTools.char.alignNode = function alignNode obj alignTM useScale:false =
(
	if obj != undefined and alignTM != undefined then
	(
		case classOf alignTM of
		(
			Point3:				alignTM = transMatrix alignTM
			Matrix3:				alignTM = alignTM
			default:
			(
				case superClassOf alignTM of
				(
					GeometryClass:	alignTM = alignTM.transform
					helper:				alignTM = alignTM.transform
					default:				return false
				)
			)
		)
		
		local newScale = [1,1,1]
		
		
		if obj.parent != undefined then
		(
			if useScale == true then
			(
				local newParentTM = obj.parent.transform
				local curParentTM = obj.parent.transform
				local endPos = obj.transform.pos
				
				
				local posOffset = alignTM.pos - curParentTM.pos
				local scaleMul = (((distance curParentTM.pos alignTM.pos) * 100.0) / (distance curParentTM.pos endPos)) / 100.0
				
				local scaleAxis = endPos * inverse curParentTM
				local maxDist = 0.0
				local scaleAxisID = 0
				
				for i = 1 to 3 do
				(
					if scaleAxis[i] > maxDist then
					(
						maxDist = scaleAxis[i]
						scaleAxisID = i
					)
				)

				
				if scaleAxisID > 0 then
					newScale[scaleAxisID] = scaleMul
				
					--// generate new Matrix3 with the position at the offset and afterwards rotated to the origin to get the origin vector
				newParentTM.pos = posOffset
				newParentTM = rotate newParentTM (inverse newParentTM.rotation)
				
				
					
				local upVec = [0,0,1]								--// up vector = z
				local dirVec = normalize newParentTM.pos			--// direction vector
				local xVec = dirVec									--// x = direction vector
				local yVec = normalize (cross upVec xVec)	--// y = cross product of upVec and x
				local zVec = (cross xVec yVec)					--// z = cross product of x and y

					--// apply values to identity Matrix
				local rotateTM = matrix3 1
				rotateTM.row1 = xVec
				rotateTM.row2 = yVec
				rotateTM.row3 = zVec

					--// apply scale, rotation and position back on
				newParentTM.scale = curParentTM.scale * newScale
				newParentTM.rotation = rotateTM.rotation * curParentTM.rotation
				newParentTM.pos = curParentTM.pos
			)
		)
		
		local childArray = #()
		
		undo "Align Node" on
		(
		
			if obj.classID[1] != 37157 then
			(
				for i = 1 to obj.children.count do
					append childArray obj.children[i]
				
				for i = 1 to childArray.count do
					childArray[i].parent = undefined
			)
			
			if useScale == true then
			(
					--// biped of obj' parent
				if obj.parent != undefined then
				(
					if obj.parent.classID[1] == 37157 then
					(
							--// get local scale
						biped.setTransform obj.parent #scale ((biped.getTransform obj.parent #scale) * newScale) false
						biped.setTransform obj.parent #rotation newParentTM.rotation false
						biped.setTransform obj.parent #pos newParentTM.pos false
					)
					else
					(
						local tempNode = obj.parent
						obj.parent = undefined
						tempNode.transform = newParentTM
						obj.parent = tempNode
					)
				)
			)
			
				--// biped of obj
			if obj.classID[1] == 37157 then
			(
				biped.setTransform obj #rotation alignTM.rotation false
				biped.setTransform obj #pos alignTM.pos false
			)
			else
			(
				obj.transform = alignTM
				
				for i = 1 to childArray.count do
					childArray[i].parent = obj
			)
		)
		
		return true
	)
	
	return false
)
alignNode = undefined







macroScript CTAlignNodeNoScale category:"CryMaxTools Character" tooltip:"Align Sel 1 -> Sel 2 No-Scale"
(--macroscript for keyboard shortcuts
	on isVisible do
	(
		if cryMaxTools != undefined then
			if selection.count > 1 then
				return true
		
		
		
		return false
	)
	
	on execute do
	(
		cryMaxTools.char.alignNode selection[1] selection[2]
	)
)




macroScript CTAlignNodeScale category:"CryMaxTools Character" tooltip:"Align Sel 1 -> Sel 2 Scale"
(--macroscript for keyboard shortcuts
	on isVisible do
	(
		if cryMaxTools != undefined then
			if selection.count > 1 then
				return true
		
		return false
	)
	
	on execute do
	(
		cryMaxTools.char.alignNode selection[1] selection[2] useScale:true
	)
)

