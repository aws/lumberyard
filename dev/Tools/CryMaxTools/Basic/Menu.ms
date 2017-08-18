


struct cryMaxToolsMenuStruct
(
	create,
	remove,
	update
)



cryMaxTools.basic.menu = cryMaxToolsMenuStruct()




cryMaxTools.basic.menu.create = function createMenuVoid =
(
	
	
	
	macroScript ATCallToolBox category:"CryMaxTools" toolTip:"open toolBox"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					return true
				else
					return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
					fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
			)catch()
		)
	)
	
	
	
	
	macroScript ATCallCharacter category:"CryMaxTools" toolTip:"open toolBox (character)"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.char != undefined then
						return true
				
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if rltCryMaxToolBox != undefined then
					(
						if rltCryMaxToolBox.open == false then
							fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					)
					else
						fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					
					rltCryMaxToolBox.fnSetToolChar()
				)
			)catch()
		)
	)
	
	
	
	
	
	
	macroScript ATCallModel category:"CryMaxTools" toolTip:"open toolBox (model)"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.model != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if rltCryMaxToolBox != undefined then
					(
						if rltCryMaxToolBox.open == false then
							fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					)
					else
						fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					
					rltCryMaxToolBox.fnSetToolModel()
				)
			)catch()
		)
	)
	
	
	
	
	
	macroScript ATCallExport category:"CryMaxTools" toolTip:"open toolBox (export)"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.model != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
				(
					if rltCryMaxToolBox != undefined then
					(
						if rltCryMaxToolBox.open == false then
							fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					)
					else
						fileIn (cryMaxTools.basic.vars.toolsPath + "Basic\\ToolBox.ms")
					
					rltCryMaxToolBox.fnSetToolExport()
				)
			)catch()
		)
	)
	
	
	
	
	
	
	
	
	
	
	macroScript ATCallAnimMan category:"CryMaxTools" toolTip:"open animation manager"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.anim != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
					fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AnimManager.ms")
			)catch()
		)
	)
	
	
	
	
	
	macroScript ATCallAlignLocator category:"CryMaxTools" toolTip:"open align / pivot / locator"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.anim != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
					fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AlignLocator.ms")
			)catch()
		)
	)
	
	
	
	
	macroScript ATCallAimPosesCreator category:"CryMaxTools" toolTip:"open directional blend creator"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.anim != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
					fileIn (cryMaxTools.basic.vars.toolsPath + "Animation\\AimPoseCreator.ms")
			)catch()
		)
	)
	
	
	
	macroScript ATCallCryBreakableTool category:"CryMaxTools" toolTip:"Open CryBreakable Tool"
	(	
		on isVisible do
		(
			try
			(
				if cryMaxTools != undefined then
					if cryMaxTools.model != undefined then
						return true
					
				return false
				
			)catch( return false )
		)			
		on execute do
		(
			try
			(
				if cryMaxTools != undefined then
					fileIn (cryMaxTools.basic.vars.toolsPath + "Model\\CryBreakableTool.ms")
			)catch()
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	MacroScript SkinLoopSelection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Loop Selection"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Loop Selection (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).loopSelection (modPanel.GetcurrentObject())
			else
				skinOps.loopSelection (modPanel.GetcurrentObject())
		)
	)

	MacroScript SkinRingSelection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Ring Selection"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Ring Selection (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ringSelection (modPanel.GetcurrentObject())
			else
				skinOps.ringSelection (modPanel.GetcurrentObject())
		)
	)

	MacroScript SkinGrowSelection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Grow Selection"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Grow Selection (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).growSelection (modPanel.GetcurrentObject())
			else
				skinOps.growSelection (modPanel.GetcurrentObject())
		)
	)

	MacroScript SkinShrinkSelection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Shrink Selection"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Shrink Selection (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).shrinkSelection (modPanel.GetcurrentObject())
			else
				skinOps.shrinkSelection (modPanel.GetcurrentObject())
		)
	)

	MacroScript SkinSelectVerticesByBone
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Vertices By Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Vertices By Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).selectVerticesByBone (modPanel.GetcurrentObject())
			else
				skinOps.selectVerticesByBone (modPanel.GetcurrentObject())
		)
	)



	MacroScript WeightTable_Dialog
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Weight Table Dialog"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Weight Table Dialog (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then
			(
				if getSkinOps != undefined then
					(getSkinOps()).isWeightTableOpen (modPanel.GetcurrentObject())
				else
					skinOps.isWeightTableOpen (modPanel.GetcurrentObject())
			)
			else 
			(
				false
			)
		)
		
		on closeDialogs do 
		(
			if getSkinOps != undefined then
				(getSkinOps()).closeWeightTable  (modPanel.GetcurrentObject())
			else
				skinOps.closeWeightTable  (modPanel.GetcurrentObject())
		) 
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).WeightTable  (modPanel.GetcurrentObject())
			else
				skinOps.WeightTable  (modPanel.GetcurrentObject())
		)
	)
	MacroScript BlendWeights
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Blend Weights"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Blend Weights (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).blendSelected (modPanel.GetcurrentObject())
			else
				skinOps.blendSelected (modPanel.GetcurrentObject())
		)
	)

	MacroScript RemoveZeroWeights
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Remove Zero Weights"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Remove Zero Weights (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).RemoveZeroWeights (modPanel.GetcurrentObject())
			else
				skinOps.RemoveZeroWeights (modPanel.GetcurrentObject())
		)
	)

	MacroScript WeightTool_Dialog
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Weight Tool Dialog"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Weight Tool Dialog (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then
			(
				if getSkinOps != undefined then
					(getSkinOps()).isWeightToolOpen (modPanel.GetcurrentObject())
				else
					skinOps.isWeightToolOpen (modPanel.GetcurrentObject())
			)
			else
			(
				false
			)
		)	
		on closeDialogs do 
		(
			if getSkinOps != undefined then
				(getSkinOps()).closeWeightTool  (modPanel.GetcurrentObject())
			else
				skinOps.closeWeightTool  (modPanel.GetcurrentObject())
		) 
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).WeightTool  (modPanel.GetcurrentObject())
			else
				skinOps.WeightTool  (modPanel.GetcurrentObject())
		)
	)

	MacroScript SetWeight_00
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.0"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.0 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.0
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.0
		)
	)

	MacroScript SetWeight_01
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.10"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.10 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.1
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.1
		)
	)

	MacroScript SetWeight_25
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.25"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.25 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.25
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.25
		)
	)

	MacroScript SetWeight_50
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.50"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.5 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.5
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.5
		)
	)

	MacroScript SetWeight_75
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.75"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.75 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.75
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.75
		)
	)

	MacroScript SetWeight_90
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 0.90"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 0.90 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.90
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 0.90
		)
	)

	MacroScript SetWeight_100
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight To 1.0"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight To 1.0 (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 1.0
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) 1.0
		)
	)

	MacroScript SetWeight_Custom
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Set Weight Custom"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Set Weight Custom (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			tmod = modPanel.GetcurrentObject()
			v = tmod.weightTool_weight
			
			if getSkinOps != undefined then
				(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) v
			else
				skinOps.SetWeight  (modPanel.GetcurrentObject()) v
		)
	)

	MacroScript AddWeight
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Add Weight"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Add Weight (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).AddWeight  (modPanel.GetcurrentObject()) 0.05
			else
				skinOps.AddWeight  (modPanel.GetcurrentObject()) 0.05
		)
	)

	MacroScript SubtractWeight
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Subtract Weight"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Subtract Weight (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).AddWeight  (modPanel.GetcurrentObject()) -0.05
			else
				skinOps.AddWeight  (modPanel.GetcurrentObject()) -0.05
		)
	)

	MacroScript ScaleWeight_Custom
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Scale Weight Custom"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Scale Weight Custom (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			tmod = modPanel.GetcurrentObject()
			v = tmod.weightTool_scale
			
			if getSkinOps != undefined then
				(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) v
			else
				skinOps.ScaleWeight  (modPanel.GetcurrentObject()) v
		)
	)

	MacroScript ScaleWeight_Up
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Scale Weight Up"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Scale Weight Up (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) 1.05
			else
				skinOps.ScaleWeight  (modPanel.GetcurrentObject()) 1.05
		)
	)

	MacroScript ScaleWeight_Down
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Scale Weight Down"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Scale Weight Down (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) 0.95
			else
				skinOps.ScaleWeight  (modPanel.GetcurrentObject()) 0.95
		)
	)

	MacroScript CopyWeights
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Copy Weights"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Copy Weights (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).CopyWeights  (modPanel.GetcurrentObject())
			else
				skinOps.CopyWeights  (modPanel.GetcurrentObject())
		)
	)

	MacroScript PasteWeights
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Paste Weights"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Paste Weights (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).PasteWeights  (modPanel.GetcurrentObject())
			else
				skinOps.PasteWeights  (modPanel.GetcurrentObject())
		)
	)


	MacroScript PasteWeightsByPos
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Paste Weights By Pos"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Paste Weights By Pos(Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			tmod = modPanel.GetcurrentObject()
			v = tmod.weightTool_tolerance
			
			if getSkinOps != undefined then
				(getSkinOps()).pasteWeightsByPos (modPanel.GetcurrentObject()) v
			else
				skinOps.pasteWeightsByPos (modPanel.GetcurrentObject()) v
		)
	)


	MacroScript selectParent
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Parent Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Parent Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectParent  (modPanel.GetcurrentObject()) 
			else
				skinOps.SelectParent  (modPanel.GetcurrentObject()) 
		)
	)

	MacroScript selectChild
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Child Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Child Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectChild  (modPanel.GetcurrentObject())
			else
				skinOps.SelectChild  (modPanel.GetcurrentObject())
		)
	)

	MacroScript selectNextSibling
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Sibling Next"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Next Sibling Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectNextSibling  (modPanel.GetcurrentObject())
			else
				skinOps.SelectNextSibling  (modPanel.GetcurrentObject())
		)
	)

	MacroScript selectPreviousSibling
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Sibling Previous"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Previous Sibling Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectPreviousSibling  (modPanel.GetcurrentObject()) 
			else
				skinOps.SelectPreviousSibling  (modPanel.GetcurrentObject()) 
		)
	)




	MacroScript backFaceCullVertices
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Backface Cull Vertices"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Backface Cull Vertices (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then (
				(modPanel.GetcurrentObject()).backfacecull
			)
			else (
				false
			)
		)
		
		on execute do
		(
			if  (modPanel.GetcurrentObject()).backfacecull then
				(modPanel.GetcurrentObject()).backfacecull = false
			else  (modPanel.GetcurrentObject()).backfacecull = true
		)
	)


	MacroScript AddBonesFromView
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Add Bones"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Add Bones (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			pushprompt "-- Click object to add as Bone"
			
			if getSkinOps != undefined then
				(getSkinOps()).AddBoneFromViewStart  (modPanel.GetcurrentObject())
			else
				skinOps.AddBoneFromViewStart  (modPanel.GetcurrentObject())
		)
	)

	MacroScript multiRemove
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Remove Bones"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Remove Multiple Bones (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).MultiRemove  (modPanel.GetcurrentObject())
			else
				skinOps.MultiRemove  (modPanel.GetcurrentObject())
		)
	)

	MacroScript selectPrevious
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Previous Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Previous Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectPreviousBone  (modPanel.GetcurrentObject()) 
			else
				skinOps.SelectPreviousBone  (modPanel.GetcurrentObject()) 
		)
	)

	MacroScript selectNext
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Next Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Next Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectNextBone  (modPanel.GetcurrentObject())
			else
				skinOps.SelectNextBone  (modPanel.GetcurrentObject())
		)
	)

	MacroScript zoomToBone
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Zoom To Bone"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Zoom To Selected Bone (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ZoomToBone  (modPanel.GetcurrentObject()) FALSE
			else
				skinOps.ZoomToBone  (modPanel.GetcurrentObject()) FALSE
		)
	)

	MacroScript zoomToGizmo
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Zoom To Gizmo"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Zoom To Selected Gizmo (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ZoomToGizmo  (modPanel.GetcurrentObject()) FALSE
			else
				skinOps.ZoomToGizmo  (modPanel.GetcurrentObject()) FALSE
		)
	)


	MacroScript selectEndPoint
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select End Point"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select End Point (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on Execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectEndPoint  (modPanel.GetcurrentObject())
			else
				skinOps.SelectEndPoint  (modPanel.GetcurrentObject())
		)
	)

	MacroScript selectStartPoint
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Start Point"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Start Point (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on Execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).SelectStartPoint  (modPanel.GetcurrentObject())
			else
				skinOps.SelectStartPoint  (modPanel.GetcurrentObject())
		)
	)

	MacroScript filterVertices
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Vertices"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Filter Vertices (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then (
				(modPanel.GetcurrentObject()).filter_vertices
			)
			else (
				false
			)
		)
		
		on execute do
		(
			if  (modPanel.GetcurrentObject()).filter_vertices then
			(modPanel.GetcurrentObject()).filter_vertices = FALSE
			else  (modPanel.GetcurrentObject()).filter_vertices = TRUE
		)
	)

	MacroScript filterEnvelopes
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Cross Sections"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Filter Cross Sections (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then (
				(modPanel.GetcurrentObject()).filter_cross_sections
			)
			else (
				false
			)
		)	
		
		on execute do
		(
			if  (modPanel.GetcurrentObject()).filter_cross_sections then
				(modPanel.GetcurrentObject()).filter_cross_sections  = FALSE
			else  (modPanel.GetcurrentObject()).filter_cross_sections  = TRUE
		)
	)

	MacroScript filterCrossSections
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Envelopes"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Filter Envelopes (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then (
				(modPanel.GetcurrentObject()).filter_envelopes
			)
			else ( 
				false
			)
		)	
		
		on execute do
		(
			if  (modPanel.GetcurrentObject()).filter_envelopes then
			(modPanel.GetcurrentObject()).filter_envelopes  = false
			else  (modPanel.GetcurrentObject()).filter_envelopes  = true
		)
	)

	MacroScript excludeVerts
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Exclude Verts"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Exclude Verts (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on Execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ButtonExclude  (modPanel.GetcurrentObject())
			else
				skinOps.ButtonExclude  (modPanel.GetcurrentObject())
		)
	)

	MacroScript includeVerts
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Include Verts"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Include Verts (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on Execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ButtonInclude  (modPanel.GetcurrentObject())
			else
				skinOps.ButtonInclude  (modPanel.GetcurrentObject())
		)
	)

	MacroScript selectIncludeVerts
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Select Excluded Verts"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Select Excluded Verts (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ButtonSelectExcluded  (modPanel.GetcurrentObject())
			else
				skinOps.ButtonSelectExcluded  (modPanel.GetcurrentObject())
		)
	)

	-- Added August 27 2000 Fred Ruff

	MacroScript CopySelectedBone
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Copy Envelope"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Copy Envelope (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).copySelectedBone (modPanel.GetcurrentObject())
			else
				skinOps.copySelectedBone (modPanel.GetcurrentObject())
		)
	)
	MacroScript PasteToSelectedBone
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Paste Envelope"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Paste Envelope (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).PasteToSelectedBone (modPanel.GetcurrentObject())
			else
				skinOps.PasteToSelectedBone (modPanel.GetcurrentObject())
		)
	)
	MacroScript PasteToAllBones
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Paste to All Envelope"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Paste To All Envelopes (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).PasteToAllBones  (modPanel.GetcurrentObject())
			else
				skinOps.PasteToAllBones  (modPanel.GetcurrentObject())
		)
	)
	MacroScript AddCrossSection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Add Cross Section"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Add Cross Section (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin) 
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ButtonAddCrossSection  (modPanel.GetcurrentObject())
			else
				skinOps.ButtonAddCrossSection  (modPanel.GetcurrentObject())
		)
	)
	MacroScript RemoveCrossSection
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Remove Cross Section"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Remove Cross Section (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin) 
		
		on execute do
		(
			if getSkinOps != undefined then
				(getSkinOps()).ButtonRemoveCrossSection  (modPanel.GetcurrentObject())
			else
				skinOps.ButtonRemoveCrossSection  (modPanel.GetcurrentObject())
		)
	)
	MacroScript DrawEnvelopeOnTop
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Envelope On Top"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Draw Envelope On Top (Skin)" 
	-- Needs Icon
	(
		on isVisible return ((classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin or (classof(modPanel.GetcurrentObject())) == CrySkin then (
				(modPanel.GetcurrentObject()).envelopesAlwaysOnTop
			)
			else
			(
				false
			)
		)
		
		on execute do
		(
			if (modPanel.GetcurrentObject()).envelopesAlwaysOnTop then
				(modPanel.GetcurrentObject()).envelopesAlwaysOnTop = FALSE
			else
				(modPanel.GetcurrentObject()).envelopesAlwaysOnTop = TRUE
		)
	)

	MacroScript DrawCrossSectionsOnTop
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"CrossSections On Top"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Draw CrossSections On Top (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
		on isChecked return 
		(
			if (classof(modPanel.GetcurrentObject())) == Skin then 
			(
				(modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop
			)
			else 
			(
				false
			)
		)
		
		on execute do
		(
			if (modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop then
				(modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop = false
			else  (modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop = true
		)
	)

	MacroScript GizmoResetRotationPlane
	enabledIn:#("max") --pfb: 2003.12.12 added product switch
	ButtonText:"Gizmo Reset Reset Rotation Plane"
	Category:"Skin Modifier" 
	internalCategory:"Skin Modifier" 
	Tooltip:"Gizmo Reset Reset Rotation Plane (Skin)" 
	-- Needs Icon
	(
		on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
		on execute do
		(
			if cryMaxTools != undefined then
				(getSkinOps()).GizmoResetRotationPlane (modPanel.GetcurrentObject())
			else
				skinOps.GizmoResetRotationPlane (modPanel.GetcurrentObject())
		)
	)
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	try (menuMan.unRegisterMenu (menuMan.findMenu "CryMaxTools") ) catch()
	try (menuMan.unRegisterMenu (menuMan.findMenu "Animation") ) catch()
	try (menuMan.unRegisterMenu (menuMan.findMenu "Character") ) catch()
	try (menuMan.unRegisterMenu (menuMan.findMenu "Model") ) catch()
	try (menuMan.unRegisterMenu (menuMan.findMenu "ToolBox") ) catch()
	
	local mainMenuBar = menuMan.getMainMenuBar()
	
	local mainMenu = menuMan.findMenu "CryMaxTools"
	if mainMenu == undefined then
		mainMenu = menuMan.createMenu "CryMaxTools"
	
	local seperator = menuMan.createSeparatorItem()
	
	
		--// ANIM MENU
	local animMenu = menuMan.findMenu "Animation"
	if animMenu == undefined then
		animMenu = menuMan.createMenu "Animation"
	
	
	
	local addSubAnim = menuMan.createActionItem "ATCallAnimMan" "CryMaxTools" ; addSubAnim.setTitle "Animation Manager" ; addSubAnim.setUseCustomTitle true ; animMenu.addItem addSubAnim -1
	addSubAnim = menuMan.createActionItem "ATCallAlignLocator" "CryMaxTools" ; addSubAnim.setTitle "Align / Pivot / Locator" ; addSubAnim.setUseCustomTitle true ; animMenu.addItem addSubAnim -1
	addSubAnim = menuMan.createActionItem "ATCallAimPosesCreator" "CryMaxTools" ; addSubAnim.setTitle "Directional Blend Creator" ; addSubAnim.setUseCustomTitle true ; animMenu.addItem addSubAnim -1
	
	local animMenuSubItem = menuMan.createSubMenuItem "Animation" animMenu
	

		--// TOOLBOX
	
	local toolBoxMenu = menuMan.findMenu "ToolBox"
	if toolBoxMenu == undefined then
		toolBoxMenu = menuMan.createActionItem "ATCallToolBox" "CryMaxTools" ; toolBoxMenu.setTitle "ToolBox" ; toolBoxMenu.setUseCustomTitle true
	
	local charMenu = menuMan.findMenu "Character"
	if charMenu == undefined then
		charMenu = menuMan.createActionItem "ATCallCharacter" "CryMaxTools" ; charMenu.setTitle "Character" ; charMenu.setUseCustomTitle true
	
	local modelMenu = menuMan.findMenu "Model"
	if modelMenu == undefined then
		modelMenu = menuMan.createMenu "Model"
	
	addSubAnim = menuMan.createActionItem "ATCallModel" "CryMaxTools" ; addSubAnim.setTitle "Model" ; addSubAnim.setUseCustomTitle true ; modelMenu.addItem addSubAnim -1
	addSubAnim = menuMan.createActionItem "ATCallCryBreakableTool" "CryMaxTools" ; addSubAnim.setTitle "CryBreakable Tool" ; addSubAnim.setUseCustomTitle true ; modelMenu.addItem addSubAnim -1
	local modelMenuSubItem = menuMan.createSubMenuItem "Model" modelMenu
	
	local exportMenu = menuMan.findMenu "Export"
	if exportMenu == undefined then
		exportMenu = menuMan.createActionItem "ATCallExport" "CryMaxTools" ; exportMenu.setTitle "Export" ; exportMenu.setUseCustomTitle true
	
	
		
	mainMenu.addItem animMenuSubItem -1		--// add animMenu
	mainMenu.addItem charMenu -1					--// add charMenu
	mainMenu.addItem modelMenuSubItem -1				--// add modelMenu
	mainMenu.addItem exportMenu -1				--// add exportMenu
	mainMenu.addItem seperator -1
	mainMenu.addItem toolBoxMenu -1				--// add toolBox
	
	
		--// add mainMenu
	local mainMenuSubItem = menuMan.createSubMenuItem "CryMaxTools" mainMenu
	mainMenuBar.addItem mainMenuSubItem (mainMenuBar.numItems() - 1)
	
	menuMan.updateMenuBar()
)
createMenuVoid = undefined



cryMaxTools.basic.menu.create()