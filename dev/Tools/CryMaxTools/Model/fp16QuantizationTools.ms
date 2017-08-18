--16bit floating point quantization tools
--by Sascha Herfort
--

rollout rltCryFP16Tools "fp16 Quantization Tools" width:190 height:492
(
	--######################################################################################
	--###  GUI  ###############################################################################
	--######################################################################################
	--label lblInfo1 "This Tool" offset:[-8,0] align:#left
	--label lblInfo2 "    Filetype: CGF" offset:[-8,-5] align:#left
	--label lblInfo3 "    Export File per Node: on" offset:[-8,-5] align:#left
	--label lblInfo4 "    Merge All Nodes: off" offset:[-8,-5] align:#left
	
	spinner spnErrorThreshold "Select Warped Vertices: " align:#right fieldWidth:32 offset:[10,0] range:[0.0,100.0,1.0]
	button btnCreateFP16Copy "FP16 Quantization Preview" offset:[0,-5] width:172 height:16 align:#center
	button btnAdjustPivotForSeamlessMAtch "Adjust Pivots For Seamless Match" offset:[0,0] width:172 height:16 align:#center enabled:false
	
	
	--######################################################################################
	--###  GLOBAL VARIABLES  #####################################################################
	--######################################################################################
	
	--TWEAKABLES
	
	--GLOBALS
	
	
	--######################################################################################
	--###  FUNCTIONS  ##########################################################################
	--######################################################################################
	
	fn fnGetBitsFromInt iInt =
	(--returns a string of 0 and 1 showing which bits are set in the incoming int
		local sResult = ""
		for i = 32 to 1 by -1 do
		(
			local bBitSet = bit.get iInt i
			sResult += if bBitSet then "1" else "0"
			if bit.get i 1 and not (bit.get i 2) and not i == 1 then sResult += "-" --add dash for readability
		)
		sResult
	)

	fn fnCryConvertFloatToHalf fValue bDebugPrint:false =
	(
		local iResult = 0
		
		local iValueBits = bit.floatAsInt fValue --get bits of supplied value - do not just convert float to int!
		if bDebugPrint then print ("iValueBits: " + (fnGetBitsFromInt iValueBits) as string)
		local iSign = bit.shift (bit.and iValueBits 0x80000000) -16 --get sign into 16bit format
		if bDebugPrint then print ("iSign: " + (fnGetBitsFromInt iSign) as string)
		local iValueBitsUnsigned = bit.and iValueBits 0x7FFFFFFF --hack off sign
		if bDebugPrint then print ("iValueBitsUnsigned: " + (fnGetBitsFromInt iValueBitsUnsigned) as string)
		
		if iValueBitsUnsigned > 0x47FFEFFF then
		(--The number is too large to be represented as a half.  Saturate to infinity.
			iResult = 0x7FFF
		)
		else
		(
			if iValueBitsUnsigned < 0x38800000 then
			(--The number is too small to be represented as a normalized half. Convert it to a denormalized value.
				local iShift = 113 - (bit.shift iValueBitsUnsigned -23)
				if bDebugPrint then print ("iShift: " + (fnGetBitsFromInt iShift) as string)
				iValueBitsUnsigned = bit.shift (bit.or 0x800000 (bit.and iValueBitsUnsigned 0x7FFFFF)) -iShift
				if bDebugPrint then print ("iValueBitsUnsigned: " + (fnGetBitsFromInt iValueBitsUnsigned) as string)
			)
			else
			(--Rebias the exponent to represent the value as a normalized half.
				iValueBitsUnsigned = (iValueBitsUnsigned + 0xC8000000)
				if bDebugPrint then print ("iValueBitsUnsigned: " + (fnGetBitsFromInt iValueBitsUnsigned) as string)
			)
			
			iResult = bit.and (bit.shift (iValueBitsUnsigned + 0x0FFF + (bit.and (bit.shift iValueBitsUnsigned -13) 1)) -13) 0x7FFF
		)
		
		if bDebugPrint then print ("(bit.or iResult iSign): " + (fnGetBitsFromInt (bit.or iResult iSign)) as string)
		
		bit.or iResult iSign
	)

	fn fnCryConvertHalfToFloat iValue =
	(
		local iMantissa = 0
		local iExponent = 0
		local iResult = 0
		
		iMantissa = bit.and iValue 0x03FF
		
		if (bit.and iValue 0x7C00) != 0  then
		(--The value is normalized
			iExponent = bit.and (bit.shift iValue -10) 0x1F
		)
		else if iMantissa != 0 then
		(--The value is denormalized. Normalize the value in the resulting float
			iExponent = 1
			do
			(
				iExponent -= 1
				iMantissa = bit.shift iMantissa 1
			) while ((bit.and iMantissa 0x0400) == 0)
			
			iMantissa = bit.and iMantissa 0x03FF
		)
		else
		(--The value is zero
			iExponent = -112
		)
		
		iResult = bit.shift (bit.and iValue 0x8000) 16 --Sign
		iResult = bit.or (bit.shift (iExponent + 112) 23) iResult --Exponent
		iResult = bit.or (bit.shift iMantissa 13) iResult --Mantissa
		
		bit.intAsFloat iResult
	)

	fn fnMoveVerticesToFP16Grid nNode bRelativeToPivot:true =
	(
		for each in nNode.verts do
		(
			local p3OldPos = (each.pos - (if bRelativeToPivot then nNode.pos else [0,0,0]))*0.01 --cryEngine stores positions in meters - 3dsmax in centimeters
			local p3NewPos = [0,0,0]
			
			for i = 1 to 3 do
			(
				
				--print  (fnGetFP16ConversionError each.pos[i])
				if p3OldPos[i] != 0 then
				(
					p3NewPos[i] = (fnCryConvertHalfToFloat (fnCryConvertFloatToHalf p3OldPos[i]))*100.0
				)
			)
			
			each.pos = p3NewPos + (if bRelativeToPivot then nNode.pos else [0,0,0])
		)
	)

	fn fnSelectVerticesWithLargeError nNode fErrorThreshold bRelativeToPivot:true =
	(
		local aVertsWithLargeError = #{}
		
		for each in nNode.verts do
		(
			local p3OldPos = (each.pos - (if bRelativeToPivot then nNode.pos else [0,0,0])) --cryEngine stores positions in meters - 3dsmax in centimeters
			--p3OldPos = each.pos
			local p3NewPos = [0,0,0]
			
			for i = 1 to 3 do
			(
				
				--print  (fnGetFP16ConversionError each.pos[i])
				if p3OldPos[i] != 0 then
				(
					p3NewPos[i] = (fnCryConvertHalfToFloat (fnCryConvertFloatToHalf (p3OldPos[i]*0.01)))*100.0
				)
			)
			
			if (distance p3OldPos p3NewPos > fErrorThreshold) then aVertsWithLargeError[each.index] = true
		)
		
		nNode.setSelection #Vertex aVertsWithLargeError
	)

	fn fnGetFP16ConversionError fFloat =
	(
		fnCryConvertHalfToFloat (fnCryConvertFloatToHalf fFloat) - fFloat
	)
	
	--######################################################################################
	--###  USER INPUT EVENT HANDLERS  ##############################################################
	--######################################################################################
	
	on btnCreateFP16Copy pressed do
	(
		if $selection.count == 1 and (classOf $ == editable_poly or classOf $ == editable_mesh or classOf $ == polyMeshObject) then
		(
			nNode = snapshot $
			nNode.name = $.name + "_fp16"
			convertTo nNode editable_poly
			nNode.xray = true
			
			fnSelectVerticesWithLargeError nNode spnErrorThreshold.value
			fnMoveVerticesToFP16Grid nNode
			
			select nNode
			setCommandPanelTaskMode #modify
			subObjectLevel = 1
		)
	)
)

cryMaxTools.basic.ROMan.cryAdd "rltCryFP16Tools" rltCryFP16Tools #main
addSubrollout (cryMaxTools.basic.ROMan.get "rltCryMaxToolBox").rltToolHolder (cryMaxTools.basic.ROMan.get "rltCryFP16Tools")