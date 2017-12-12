-------------------------------------------------------------------------------
--	smartObjects.ms
--	Version 1.0 
--	by Marcus Krautwurst
-------------------------------------------------------------------------------

global smartObj_cryGlobal
global smartObjHelpersArray_cryGlobal
/*
if SOWindow != undefined then
(
	closeRolloutFloater SOWindow
)
*/

rollout SmartObjects "Smart Objects"
(
	group "Generator/Export"
	(
		button btnSOGetObj "Set Smart Object" width:150 height:15 
		button btnSOAddLoc "Add Start/Stop Locations" width:150 height:15 
		button btnSOExport "Export Smart Object Data" width:150 height:25
	)
	
	on btnSOGetObj pressed do
	(
		if $ == undefined then
			(
				messagebox "Please select an object to be defined as the Smart Object."
				return undefined
			)
		
		if selection.count != 1 then
		(
			messagebox "You can only export one mesh as a Smart Object"
			return undefined
		)	
			
		t = $.pos
		if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
		(
			messagebox "ERROR: Object's pivot is not at origin (0,0,0)"
			return undefined
		)


		
		if classof $ == PolyMeshObject or classof $ == Editable_mesh or classof $ == Editable_Poly then
		(
			global smartObj_cryGlobal = $
			btnSOGetObj.text = (("Smart Object: ")+($.name))
		)
		else
		(
			messagebox "Smart Object needs to be Geometry"
			return undefined
		)	
		$.wireColor = (color 214 228 153)
	)
	
	on btnSOAddLoc pressed do
	(
		undo on
		(
			if IsValidNode smartObj_cryGlobal == true then
			(
				if $so*.count == 0 then numSO_cryGlobal =  1
				if $so*.count > 0 then numSO_cryGlobal +=1
				Circle radius:100 pos:[0,0,0] wirecolor:[0,255,0] name:("so_start_" + (numSO_cryGlobal as string)) isselected:true
				setUserPropBuffer $ "0"
				$.parent = smartObj_cryGlobal
				Text text:(("Smart\r\nObject #" + (numSO_cryGlobal as string)) + ("\r\nStart") as string)size:40 pos:[0,20,0] wirecolor:[0,255,0] name:("TextStart_" + (numSO_cryGlobal as string)) isselected:true 
				convertToSplineShape (getnodebyname ("TextStart_" + (numSO_cryGlobal as string)))			
				(getnodebyname ("TextStart_" + (numSO_cryGlobal as string))).parent = (getnodebyname ("so_start_" + (numSO_cryGlobal as string)))
				setTransformLockFlags (getnodebyname ("TextStart_" + (numSO_cryGlobal as string))) #{1,2,3}	
		
				Circle radius:100 pos:[-600,0,0] wirecolor:[255,0,0] name:("so_end_" + (numSO_cryGlobal as string)) isselected:true
				setUserPropBuffer $ "0"
				$.parent = smartObj_cryGlobal
				Text text:(("Smart\r\nObject #" + (numSO_cryGlobal as string)) + ("\r\nEnd") as string) size:40 pos:[-600,30,0] wirecolor:[255,0,0] name:("TextEnd_" + (numSO_cryGlobal as string)) isselected:true 
				convertToSplineShape (getnodebyname ("TextEnd_" + (numSO_cryGlobal as string)))			
				(getnodebyname ("TextEnd_" + (numSO_cryGlobal as string))).parent = (getnodebyname ("so_end_" + (numSO_cryGlobal as string)))
				setTransformLockFlags (getnodebyname ("TextEnd_" + (numSO_cryGlobal as string))) #{1,2,3}				
				deselect $
			)
			else
			(
				messagebox "ERROR: Please define a Smart Object first before creating helpers!"
				return undefined
			)
		)
	)		
	on btnSOExport pressed do
	(
		global SOData		
		if IsValidNode smartObj_cryGlobal == true then
		(
			if btnSOGetObj.text != "Set Smart Object" then
			(					
					if $so*.count != 0 then 
						(
						SO_out_file_dialog = getSaveFileName caption:"Export Smart Object Data" \
						filename:((maxFilepath)+(smartObj_cryGlobal.name)+(".xml")) \
						types:"SmartObject Data(*.xml)"
						try
						(
							SO_out_file = createfile (SO_out_file_dialog) -- Save Dialog							
							-- Add the Node to the Output Array
							SOData = #(("<sotemplate>\n<object model=\"")+(smartObj_cryGlobal.name)+(".cgf\"/>"))
							smartObjHelpersArray_cryGlobal = ($so_* as array)
							for obj = 1 to smartObjHelpersArray_cryGlobal.count do 			
							(								
								append soData ("\t<helper name=\"" + smartObjHelpersArray_cryGlobal[obj].name as string + "\" pos=\"" + (filterstring ((smartObjHelpersArray_cryGlobal[obj].pos/100) as string) "[]")[1] + "\" rot=\"" + (smartObjHelpersArray_cryGlobal[obj].rotation.w as string + "," + smartObjHelpersArray_cryGlobal[obj].rotation.x as string + "," + smartObjHelpersArray_cryGlobal[obj].rotation.y as string + "," + smartObjHelpersArray_cryGlobal[obj].rotation.z as string) + "\" radius=\"" + (smartObjHelpersArray_cryGlobal[obj].radius/100) as string + "\" projectOnGround=\"" + (getUserPropBuffer smartObjHelpersArray_cryGlobal[obj]) +"\" />")
							)		
							append soData ("</sotemplate>")
							
							---- Write the File							
							for i = 1 to SOData.count do format (SOData[i] + "\n") to:SO_out_file echo:false
							close SO_out_file
							print (("File saved to ")+(SO_out_file_dialog))
						)
						catch()
					)
					else
					(
						messagebox "ERROR: No Start/Stop Locations placed!"
					)
			)
			else
			(
				messagebox "ERROR: Set a Smart Object first"
			)
		)
		else
		(
			messagebox "ERROR: Smart Object not found"
		)
	)	
)
/*
SOWindow = newRolloutFloater "Smart Objects" 190 140
addRollout SmartObjects SOWindow
*/

--####### Load the Rollout with ROMan ########
cryMaxTools.basic.ROMan.cryAdd "SmartObjects" SmartObjects #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "SmartObjects") rolledup:false