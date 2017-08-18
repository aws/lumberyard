-------------------------------------------------------------------------------
--	SetTransformationLimits
-- 	Version 0.8
--	by Marcus Krautwurst
-------------------------------------------------------------------------------

rollout T_Limits "Transformation Limits" 
(
	
	local limitpos	
	local limitrotation	
	local LimitMirrorObjSel = undefined
	local LimitMirrorTargetSel = undefined
	
	group "Position Limits"
	(
		label act_trans_x "" across:3 width:40 height:14
		label act_trans_y "" width:40 height:14
		label act_trans_z "" width:40 height:14
		checkbox CHKactive_Trans_from_x "X" across:3 offset: [5,0] width:40 height:15 triState:0
		checkbox CHKactive_Trans_from_y "Y" offset: [10,0] width:40 height:15 triState:0
		checkbox CHKactive_Trans_from_z "Z" offset: [15,0] width:40 height:15 triState:0
		spinner Trans_from_x "X" across:3 offset:[-15,0]width:55 height:16 range:[-500000,500000,0] type:#integer enabled: false
		spinner Trans_from_y "Y" offset:[-10,0] width:55 height:16 range:[-500000,500000,0]  type:#integer enabled: false
		spinner Trans_from_z "Z" offset:[-5,0] width:55 height:16 range:[-500000,500000,0]  type:#integer enabled: false
		spinner Trans_to_x "X"  across:3 offset:[-15,0] width:55 height:16 range:[-500000,500000,0] type:#integer enabled: false
		spinner Trans_to_y "Y" offset:[-10,0] width:55 height:16 range:[-500000,500000,0] type:#integer enabled: false
		spinner Trans_to_z "Z" offset:[-5,0] width:55 height:16 range:[-500000,500000,0] type:#integer enabled: false
		button resetP ">       Z e r o  L i m i t s       <" width:165 height:15
	)	
	group "Rotation Limits"
	(
		label act_rot_x "" across: 3 width:40 height:14
		label act_rot_y "" width:40 height:14
		label act_rot_z "" width:40 height:14	
		checkbox CHKactive_from_x "X" across: 3 offset: [5,0] width:40 height:15 triState:0
		checkbox CHKactive_from_y "Y"width:40 offset: [10,0] height:15 triState:0
		checkbox CHKactive_from_z "Z" width:40 offset: [15,0] height:15 triState:0
		spinner from_x "X" across: 3  offset: [-15,0] width:55 height:15 range:[0,50000,0] type:#integer enabled: false
		spinner from_y "Y" offset: [-10,0] width:55 height:15 range:[0,50000,0] type:#integer enabled: false
		spinner from_z "Z" offset: [-5,0] width:55 height:15 range:[0,50000,0] type:#integer enabled: false
		spinner to_x "X" across: 3 offset: [-15,0] width:55 height:15 range:[-50000,0,0] type:#integer enabled: false
		spinner to_y "Y" offset: [-10,0] width:55 height:15 range:[-50000,0,0] type:#integer enabled: false
		spinner to_z "Z" offset: [-5,0] width:55 height:15 range:[-50000,0,0] type:#integer enabled: false
		button resetR ">       Z e r o  L i m i t s       <" width:165 height:15		
	)	
	group "Reset Object Controllers"
	(
	button Kill ">  R e s e t   C o n t r o l l e r s  <" width:165 height:25 offset: [1,0]
	)
	group "Mirror Limits"
	(
	pickbutton LimitMirrorObj "Pick Object" width:75 height:25 across: 2 offset:[-5,0]
	pickbutton LimitMirrorTarget "Pick Target" width:75 height:25 offset:[5,0] enabled:false
	label LBLMirrorAxis "Mirror Plane" offset:[-53,0]
	radiobuttons RADMirrorAxis labels:#("XZ", "XY", "YZ") offset:[31,-18]
	checkbox CHKMirrorOffset "Keep Initial Offset" triState:1
	)
	--- Mirror: Select Object
	on LimitMirrorObj picked LimitMirrorObjTemp do
	(
		if LimitMirrorObjTemp != undefined then
		(
			LimitMirrorObjSel = LimitMirrorObjTemp
			LimitMirrorObj.text = LimitMirrorObjSel.name
			LimitMirrorTarget.enabled = true
			select LimitMirrorObjSel
		)
	)
	--- Mirror: Select Target
	on LimitMirrorTarget picked LimitMirrorTargetTemp do
	(	
		LimitMirrorTargetSel = LimitMirrorTargetTemp
		LimitMirrorTarget.text = LimitMirrorTargetSel.name
		LimitMirrorTargetSel.pos.controller.X_Position.controller = Float_Limit()
		--if 
			if CHKMirrorOffset.triState == 1 then
		(
			LimitMirrorTargetSel.pos.controller.X_Position.controller.upper_limit = (LimitMirrorTargetSel.pos[1] + (LimitMirrorObjSel.pos.controller.X_Position.controller.upper_limit-LimitMirrorObjSel.pos[1]))
			LimitMirrorTargetSel.pos.controller.X_Position.controller.lower_limit = (LimitMirrorTargetSel.pos[1] + (LimitMirrorObjSel.pos.controller.X_Position.controller.lower_limit-LimitMirrorObjSel.pos[1]))
			LimitMirrorObj.text = "Pick Object"
			LimitMirrorTarget.text = "Pick Target"
		)
		else
		(
			LimitMirrorTargetSel.pos.controller.X_Position.controller.upper_limit = LimitMirrorObjSel.pos.controller.X_Position.controller.upper_limit
			LimitMirrorTargetSel.pos.controller.X_Position.controller.lower_limit = LimitMirrorObjSel.pos.controller.X_Position.controller.lower_limit
			LimitMirrorObj.text = "Pick Object"
			LimitMirrorTarget.text = "Pick Target"
		)
		select LimitMirrorTargetSel
	)
	/*checkbutton toggle "Global Toggle" width:90 height:18		---- NO FUNCTION AVAILABLE YET
	button undolast "Undo Last" width:60 height:18				---- NO FUNCTION AVAILABLE YET
	*/	
		-- Set Limit Functions
		------------- Spinner X
		on CHKactive_Trans_from_x changed CHKactive_Trans_from_x_state do 
			(
				if CHKactive_Trans_from_x_state == true then
				(
					Trans_from_x.enabled = true
					Trans_to_x.enabled = true
					$.pos.controller.X_Position.controller= FloatLimitCtrl()
					$.pos.controller.X_Position.controller.upper_limit = in coordsys parent $.pos.x
					$.pos.controller.X_Position.controller.lower_limit = in coordsys parent $.pos.x
					)			
				else 
					(
						Trans_from_x.enabled = false
						Trans_to_x.enabled = false
						$.pos.controller.X_Position.controller= Bezier_Float()
					)
				)
		------------- Spinner Y
		on CHKactive_Trans_from_y changed CHKactive_Trans_from_y_state do 
			(
				if CHKactive_Trans_from_y_state == true then	
					(
						Trans_from_y.enabled = true
						Trans_to_y.enabled = true
						$.pos.controller.Y_Position.controller= FloatLimitCtrl()
						$.pos.controller.Y_Position.controller.upper_limit = in coordsys parent $.pos.y
						$.pos.controller.Y_Position.controller.lower_limit = in coordsys parent $.pos.y
					)			
					else
				(
						Trans_from_y.enabled = false
						Trans_to_y.enabled = false
						$.pos.controller.Y_Position.controller= Bezier_Float()
						
						try
						(
							Trans_from_y.value = 0
							Trans_to_y.value = 0
						)
						catch()
				)
			)
		------------- Spinner Z
		on CHKactive_Trans_from_z changed CHKactive_Trans_from_z_state do 
			(
				if CHKactive_Trans_from_z_state == true then	
				(
				Trans_from_z.enabled = true
				Trans_to_z.enabled = true
				$.pos.controller.Z_Position.controller= FloatLimitCtrl()
				$.pos.controller.Z_Position.controller.upper_limit = in coordsys parent $.pos.z
				$.pos.controller.Z_Position.controller.lower_limit = in coordsys parent $.pos.z
				)
				else
				(
				Trans_from_z.enabled = false
				Trans_to_z.enabled = false
				$.pos.controller.Z_Position.controller= Bezier_Float()
				
						try
						(
							Trans_from_z.value = 0
							Trans_to_z.value = 0
						)
						catch()	
				)
			)	
		-- Set the Limits when the Spinners are changed
			on Trans_from_x changed val do $.pos.controller.X_Position.controller.upper_limit = val
			on Trans_to_x changed val do $.pos.controller.X_Position.controller.lower_limit = val
			on Trans_from_y changed val do $.pos.controller.Y_Position.controller.upper_limit = val
			on Trans_to_y changed val do $.pos.controller.Y_Position.controller.lower_limit = val
			on Trans_from_z changed val do $.pos.controller.Z_Position.controller.upper_limit = val
			on Trans_to_z changed val do $.pos.controller.Z_Position.controller.lower_limit = val
		
		fn killAll =	-- Reset Controllers Function (Kill all Controllers and reset to Standard)
		(
				(
				from_x.enabled = false
				from_y.enabled = false
				from_z.enabled = false
				to_x.enabled = false
				to_y.enabled = false
				to_z .enabled = false
				CHKactive_from_x.triState = 0
				CHKactive_from_y.triState = 0
				CHKactive_from_z.triState = 0					
			try
			(			

			)
			catch()		
			try
			(				
				$.rotation.controller[2][2].controller.upper_limit = 0
				$.rotation.controller[2][2].controller.lower_limit = 0
				from_y.value = 0
				to_y.value = 0
			)
			catch()		
			try
			(				
				$.rotation.controller[2][3].controller.upper_limit = 0
				$.rotation.controller[2][3].controller.lower_limit = 0
				from_z.value = 0
				to_z.value = 0
			)
			catch()	
			try			
			(
				$.rotation.controller.delete 2
				$.rotation.controller = Euler_XYZ ()
				$.pos.controller[1].controller = Bezier_Float ()
				$.pos.controller[2].controller = Bezier_Float ()
				$.pos.controller[3].controller = Bezier_Float ()
			)
			catch()		
		)
				Trans_from_x.enabled = false
				Trans_from_y.enabled = false
				Trans_from_z.enabled = false
				Trans_to_x.enabled = false
				Trans_to_y.enabled = false
				Trans_to_z .enabled = false
				CHKactive_Trans_from_x.triState = 0
				CHKactive_Trans_from_y.triState = 0
				CHKactive_Trans_from_z.triState = 0
				try			
			(
				$.pos.controller[1].controller = Bezier_Float ()
				$.pos.controller[2].controller = Bezier_Float ()
				$.pos.controller[3].controller = Bezier_Float ()
			)
			catch()
			
		)		
		-- Reset Function
		on resetP pressed  do
		(
			try
			(
				$.pos.controller.X_Position.controller.upper_limit = 0
				$.pos.controller.X_Position.controller.lower_limit = 0
				$.pos.controller.Y_Position.controller.upper_limit = 0
				$.pos.controller.Y_Position.controller.lower_limit = 0
				$.pos.controller.Z_Position.controller.upper_limit = 0
				$.pos.controller.Z_Position.controller.lower_limit = 0
				Trans_from_x.value = 0
				Trans_from_y.value = 0
				Trans_from_z.value = 0
				Trans_to_x.value = 0
				Trans_to_y.value = 0
				Trans_to_z.value = 0
			)
			catch()
		)		
		fn update_position =
		(
			try
			(
				limitSelection = $
				if $ != undefined or $.count < 1 then
						(
							-- print actual Position above the Limit-Spinners
							act_trans_x.caption = in coordsys world formattedprint ($.pos).x format:".1f" as string
							act_trans_y.caption = in coordsys world formattedprint ($.pos).y format:".1f" as string
							act_trans_z.caption = in coordsys world formattedprint ($.pos).z format:".1f" as string												
							if	($.pos.controller.X_Position.controller as string) == "Controller:Float_Limit" then -- Set the defined Controller for X
								(
									Trans_from_x.enabled = true
									Trans_to_x.enabled = true
									CHKactive_Trans_from_x.triState = 1
									Trans_from_x.value = ($.pos.controller.X_Position.controller.upper_limit) 
									Trans_to_x.value = ($.pos.controller.X_Position.controller.lower_limit)
								)
								else
									(
										Trans_from_x.enabled = false
										Trans_to_x.enabled = false
										CHKactive_Trans_from_x.triState = 0
									)			
							if	($.pos.controller.Y_Position.controller as string) == "Controller:Float_Limit" then  -- Set the defined Controller for Y
								(
										Trans_from_y.enabled = true
										Trans_to_y.enabled = true
										CHKactive_Trans_from_y.triState = 1
										Trans_from_y.value = ($.pos.controller.Y_Position.controller.upper_limit)
										Trans_to_y.value = ($.pos.controller.Y_Position.controller.lower_limit)
								)
								else
								(
										Trans_from_y.enabled = false
										Trans_to_y.enabled = false
										CHKactive_Trans_from_y.triState = 0
								)
							if	($.pos.controller.Z_Position.controller as string) == "Controller:Float_Limit" then  -- Set the defined Controller for Z
								(
										Trans_from_z.enabled = true
										Trans_to_z.enabled = true
										CHKactive_Trans_from_z.triState = 1
										Trans_from_z.value = ($.pos.controller.Z_Position.controller.upper_limit)
										Trans_to_z.value = ($.pos.controller.Z_Position.controller.lower_limit)
								)	
								else 
								(
										Trans_from_z.enabled = false
										Trans_to_z.enabled = false
										CHKactive_Trans_from_z.triState = 0
								)
						)
				else
					(
						print "No Selection"					
					)
						 -- Reset the Spinners when the Checkboxes are deactived
						if CHKactive_Trans_from_x.triState == 0 then
						(
							Trans_from_x.value = 0
							Trans_to_x.value = 0
						)
						if CHKactive_Trans_from_y.triState == 0 then
						(
							Trans_from_y.value = 0
							Trans_to_y.value = 0
						)
						
						if CHKactive_Trans_from_z.triState == 0 then
						(
							Trans_from_z.value = 0
							Trans_to_z.value = 0
						)
			)
			catch()
		)
		fn update_rotation =
		(
			try
			(
				limit2Selection = $
				if $ != undefined or $.count < 1 then 
						(
							act_rot_x.caption = in coordsys parent formattedprint -($.rotation as eulerAngles).x format:".1f" as string
							act_rot_y.caption = in coordsys parent formattedprint -($.rotation as eulerAngles).y format:".1f" as string
							act_rot_z.caption = in coordsys parent formattedprint -($.rotation as eulerAngles).z format:".1f" as string
						
							if $.rotation.controller as string == "Controller:Rotation_List" then -- Check if Limit is active on X
								(	
									if $.rotation.controller[2].controller[1].controller as string == "Controller:Float_Limit" then
									(
										from_x.enabled = true
										to_x.enabled = true
										CHKactive_from_x.triState = 1
										from_x.value = radtoDeg ($.rotation.controller[2].controller[1].controller.upper_limit)
										to_x.value = radtoDeg ($.rotation.controller[2].controller[1].controller.lower_limit)
									)
									else
									(
									from_x.enabled = false
									to_x.enabled = false
									from_x.value = 0
									to_x.value = 0
									CHKactive_from_x.triState = 0
									)
								)
							else
								(
									from_x.enabled = false
									to_x.enabled = false
									from_x.value = 0
									to_x.value = 0
									CHKactive_from_x.triState = 0
								)							
							
								if $.rotation.controller as string == "Controller:Rotation_List" then -- Check if Limit is active on Y
								(	
									if $.rotation.controller[2].controller[2].controller as string == "Controller:Float_Limit" then
									(
										from_y.enabled = true
										to_y.enabled = true
										CHKactive_from_y.triState = 1
										from_y.value = radtoDeg ($.rotation.controller[2].controller[2].controller.upper_limit)
										to_y.value = radtoDeg ($.rotation.controller[2].controller[2].controller.lower_limit)
									)
									else
									(
									from_y.enabled = false
									to_y.enabled = false
									from_y.value = 0
									to_y.value = 0
									CHKactive_from_y.triState = 0
									)
								)
							else
								(
									from_y.enabled = false
									to_y.enabled = false
									from_y.value = 0
									to_y.value = 0
									CHKactive_from_y.triState = 0
								)								
								
								if $.rotation.controller as string == "Controller:Rotation_List" then -- Check if Limit is active on Z
								(	
									if $.rotation.controller[2].controller[3].controller as string == "Controller:Float_Limit" then
									(
										from_z.enabled = true
										to_z.enabled = true
										CHKactive_from_z.triState = 1
										from_z.value = radtoDeg ($.rotation.controller[2].controller[3].controller.upper_limit)
										to_z.value = radtoDeg ($.rotation.controller[2].controller[3].controller.lower_limit)
									)
									else
									(
									from_z.enabled = false
									to_z.enabled = false
									from_z.value = 0
									to_z.value = 0
									CHKactive_from_z.triState = 0
									)
								)
							else
								(
									from_z.enabled = false
									to_z.enabled = false
									from_z.value = 0
									to_z.value = 0
									CHKactive_from_z.triState = 0
								)	
								
						)
			else
			print "No Selection"
			)
			catch()
		)
	-- Set Limits Function on X
		fn setStateX active tm =
		(
			limitFound = false
			subAnimLimitFound = false
			
			try
			(
				if  (execute("$." + tm + ".controller[2].name")) == "Limit" then
				(
					limitFound = true
					
					if (execute("(classOf $." + tm + ".controller[2][1].controller) as String")) == "float_limit" then
						subAnimLimitFound = true
				)
			)
			catch()		
			rotationCon = false
			if tm == "rotation" then
				rotationCon = true
			if active == true then
			(
				with undo on
				(
					if limitFound == false then
					(
						if rotationCon == true then
						(
							(execute("$." + tm + ".controller = rotation_list()"))
							(execute("$." + tm + ".controller[2].controller = Euler_XYZ()"))
						)
						else (execute("$." + tm + ".controller = position_list()"))				
						(execute("$." + tm + ".controller.setactive 2"))
					)
					if subAnimLimitFound == false then
					(
						from_x.enabled = true
						to_x.enabled = true					
						(execute("$." + tm + ".controller[2][1].controller = FloatLimitCtrl()"))					
						(execute("$." + tm + ".controller.setName 2 \"Limit\""))
						(execute("$." + tm + ".controller[2][1].controller.upper_limit = 0"))
						(execute("$." + tm + ".controller[2][1].controller.lower_limit = 0"))
					)				
				)
			)
			else
			(
				if rotationCon == true then
				(
					(execute("$." + tm + ".controller"))[2][1].controller = Bezier_Float()
				)
				(execute("$." + tm + ".controller")).setactive 2
				from_x.enabled = false
				to_x.enabled = false
				from_x.value = 0
				to_x.value = 0
			)
		)
		-- Set Limits Function on Y
		fn setStateY active tm =
		(
			limitFound = false
			subAnimLimitFound = false
			
			try
			(
				if  (execute("$." + tm + ".controller[2].name")) == "Limit" then
				(
					limitFound = true
					
					if (execute("(classOf $." + tm + ".controller[2][2].controller) as String")) == "float_limit" then
						subAnimLimitFound = true
				)
			)
			catch()		
			rotationCon = false
			if tm == "rotation" then
				rotationCon = true
			if active == true then
			(
				with undo on
				(
					if limitFound == false then
					(
						if rotationCon == true then
						(
							(execute("$." + tm + ".controller = rotation_list()"))
							(execute("$." + tm + ".controller[2].controller = Euler_XYZ()"))
						)
						else (execute("$." + tm + ".controller = position_list()"))				
						(execute("$." + tm + ".controller.setactive 2"))
					)
					if subAnimLimitFound == false then
					(
						from_y.enabled = true
						to_y.enabled = true
						(execute("$." + tm + ".controller[2][2].controller = FloatLimitCtrl()"))					
						(execute("$." + tm + ".controller.setName 2 \"Limit\""))
						(execute("$." + tm + ".controller[2][2].controller.upper_limit = 0"))
						(execute("$." + tm + ".controller[2][2].controller.lower_limit = 0"))
					)
				)
			)
			else
			(
				if rotationCon == true then
				(
					(execute("$." + tm + ".controller"))[2][2].controller = Bezier_Float()
				)
				(execute("$." + tm + ".controller")).setactive 2
				from_y.enabled = false
				to_y.enabled = false
				from_y.value = 0
				to_y.value = 0
			)
		)
		-- Set Limits Function on Z
		fn setStateZ active tm =
		(
			limitFound = false
			subAnimLimitFound = false		
			try
			(
				if  (execute("$." + tm + ".controller[2].name")) == "Limit" then
				(
					limitFound = true
					
					if (execute("(classOf $." + tm + ".controller[2][3].controller) as String")) == "float_limit" then
						subAnimLimitFound = true
				)
			)
			catch()		
			rotationCon = false
			if tm == "rotation" then
				rotationCon = true
			if active == true then
			(
				with undo on
				(
					if limitFound == false then
					(
						if rotationCon == true then
						(
							(execute("$." + tm + ".controller = rotation_list()"))
							(execute("$." + tm + ".controller[2].controller = Euler_XYZ()"))
						)
						else (execute("$." + tm + ".controller = position_list()"))				
						(execute("$." + tm + ".controller.setactive 2"))
					)
					if subAnimLimitFound == false then
					(
						from_z.enabled = true
						to_z.enabled = true					
						(execute("$." + tm + ".controller[2][3].controller = FloatLimitCtrl()"))					
						(execute("$." + tm + ".controller.setName 2 \"Limit\""))
						(execute("$." + tm + ".controller[2][3].controller.upper_limit = 0"))
						(execute("$." + tm + ".controller[2][3].controller.lower_limit = 0"))
					)				
				)
			)
			else
			(
				if rotationCon == true then
				(
					(execute("$." + tm + ".controller"))[2][3].controller = Bezier_Float()
				)
				(execute("$." + tm + ".controller")).setactive 2
				from_z.enabled = false
				to_z.enabled = false
				from_z.value = 0
				to_z.value = 0
			)
		)
		on CHKactive_from_x changed CHKactive_from_x_state do setStateX CHKactive_from_x_state "rotation"
		on CHKactive_from_y changed CHKactive_from_y_state do setStateY CHKactive_from_y_state "rotation"
		on CHKactive_from_z changed CHKactive_from_z_state do setStateZ CHKactive_from_z_state "rotation"

		-- Set the Limits when the Spinners are changed
			on from_x changed val do $.rotation.controller[2][1].controller.upper_limit = degtorad val
			on to_x changed val do $.rotation.controller[2][1].controller.lower_limit= degtorad val
			on from_y changed val do $.rotation.controller[2][2].controller.upper_limit = degtorad val
			on to_y changed val do $.rotation.controller[2][2].controller.lower_limit = degtorad val
			on from_z changed val do $.rotation.controller[2][3].controller.upper_limit = degtorad val
			on to_z changed val do $.rotation.controller[2][3].controller.lower_limit = degtorad val
		-- Reset Function (1 Check per Axies is necessary)
		on resetR pressed  do
		(
			try
			(
				$.rotation.controller[2][1].controller.upper_limit = 0
				$.rotation.controller[2][1].controller.lower_limit= 0
				from_x.value = 0
				to_x.value = 0
			)
			catch()		
			try
			(				
				$.rotation.controller[2][2].controller.upper_limit = 0
				$.rotation.controller[2][2].controller.lower_limit = 0
				from_y.value = 0
				to_y.value = 0
			)
			catch()		
			try
			(				
				$.rotation.controller[2][3].controller.upper_limit = 0
				$.rotation.controller[2][3].controller.lower_limit = 0
				from_z.value = 0
				to_z.value = 0
			)
			catch()		
		)	
		-- Onload Commands
		on T_Limits open do
		(
			try
			(
				if selection == undefined or selection.count != 1 then return undefined else
				(		
					LimitMirrorPointA = undefined
					LimitMirrorPointB = undefined
					limitSelection = $
					limitRotation = $.rotation as eulerangles					
				)
			)
			catch ()
		)
		-- Close Window
		on toggle changed do (macros.run 26011)
		on T_limits close do
		(
			callbacks.removeScripts id:#updateLimit
			deleteAllChangeHandlers ()
		)
		on Kill pressed do 
		(	
			if  $ == undefined then messagebox "No object selected"
			try
			(		
				if $.rotation.controller as string == "Controller:Rotation_List" or $.pos.controller[1].controller as string == "Controller:Float_Limit" or $.pos.controller[2].controller as string == "Controller:Float_Limit" or $.pos.controller[3].controller as string == "Controller:Float_Limit" then
				(
					deletewarning = querybox "WARNING! \nAll existing controllers on the object will be reseted to standard. \nThis deletes ALL keyframes on the selected object! \nDo you want to proceed?"
					if deletewarning == true then
					(
						LimitWindow.rollouts[1].KillAll()
					)
					else
					(
						print "Operation canceled"
					)	
				)
				else
				(
				messagebox "No changes made to this object"	
				)
			)
			catch()
		)
)
--LimitWindow = newRolloutFloater "Transformation Limits" 190 440
--####### Load the Rollout with ROMan ########--
cryMaxTools.basic.ROMan.cryAdd "T_Limits" T_Limits #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "T_Limits")
callbacks.addScript #selectionSetChanged "(cryMaxTools.basic.ROMan.get \"T_Limits\").update_rotation()" id:#updateLimit
callbacks.addScript #selectionSetChanged "(cryMaxTools.basic.ROMan.get \"T_Limits\").update_position()" id:#updateLimit