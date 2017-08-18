--############################################################################################
--SCRIPT TO EXPORT CAMERA MOVEMENT FROM CRYENGINE EDITOR TO 3DSMAX - REQUIRES ENCLOSED FLOW GRAPH TO GENERATE DATA
--############################################################################################

sFile = maxFilePath + "Editor.log"
fUnitScale = 100.0

--############################################################################################
--FUNCTIONS BELOW
--############################################################################################
fn fnGetFileStreamLineFlowLogData sFileStream =
(--RETURNS FLOW LOG OUTPUT OR UNDEFINED
	local sNextLine = readLine sFileStream
	if substring sNextLine 1 10 == "[flow-log]" then
	(
		substring sNextLine 12 -1
	)
	else
	(
		undefined
	)
)

fn fnGetNextFileStreamFlowLogData sFileStream =
(--RETURNS NEXT FLOW LOG OUTPUT OR UNDEFINED
	local sFlowLogData = undefined
	
	while not eof sFileStream and sFlowLogData == undefined do
	(
		sFlowLogData = fnGetFileStreamLineFlowLogData sFileStream
	)
	
	sFlowLogData
)

fn fnGetFloatFromString sString =
(--CONVERTS A STRING FROM FLOW LOG DATA TO A FLOAT VALUE OR UNDEFINED
	sString as float
)

fn fnGetPoint3FromString sString =
(--CONVERTS A STRING FROM FLOW LOG DATA TO A POINT3 VALUE OR UNDEFINED
	local p3Result = undefined
	local aStringElements = filterString sString ","
	
	if aStringElements.count == 3 then
	(
		local bContainsNaN = false
		
		for i = 1 to 3 do
		(
			aStringElements[i] = aStringElements[i] as float
			bContainsNaN = not (classOf aStringElements[i] == float)
		)
		
		if not bContainsNaN then
		(
			p3Result = point3 aStringElements[1] aStringElements[2] aStringElements[3]
		)
	)
	
	p3Result
)

fn fnVectorIsNormalized p3Vector =
(--RETURNS TRUE IF LENGTH OF VECTOR IS NEAR 1.0
	close_enough  (length p3Vector) 1.0 10
)

fn fnGetNextFlowLogPoint3 sFileStream =
(--RETURNS NEXT FLOW LOG VECTOR OR UNDEFINED
	local sString = fnGetNextFileStreamFlowLogData sFileStream
	if sString != undefined then
	(
		local p3Result = fnGetPoint3FromString sString
	)
	
	while not eof sFileStream and p3Result == undefined do
	(
		sString = fnGetNextFileStreamFlowLogData sFileStream
		if sString != undefined then
		(
			p3Result = fnGetPoint3FromString sString
		)
	)
	
	p3Result
)

fn fnGetNextFlowLogFloat sFileStream =
(--RETURNS NEXT FLOW LOG FLOAT OR UNDEFINED
	local sString = fnGetNextFileStreamFlowLogData sFileStream
	if sString != undefined then
	(
		local fResult = fnGetFloatFromString sString
	)
	
	while not eof sFileStream and fResult == undefined do
	(
		sString = fnGetNextFileStreamFlowLogData sFileStream
		if sString != undefined then
		(
			fResult = fnGetFloatFromString sString
		)
	)
	
	fResult
)

--############################################################################################
--MAGIC BELOW
--############################################################################################
if doesFileExist sFile then
(
	local sLogStream = openFile sFile
	
	local iCameraLogStart = 0
	while (skipToString sLogStream "[flow-log] Start Camera output") != undefined do
	(--FIND THE LATEST CAMERA MOVEMENT LOG IN THE EDITOR.LOG
		iCameraLogStart = filePos sLogStream
	)
	
	seek sLogStream iCameraLogStart --SKIP TO POSITION OF LAST CAMERA MOVEMENT LOG START
	
	local fTimeStep = fnGetFloatFromString (fnGetNextFileStreamFlowLogData sLogStream)
	local fFOV = fnGetFloatFromString (fnGetNextFileStreamFlowLogData sLogStream)
	
	if fTimeStep != undefined and fFOV != undefined then
	(--EDITOR LOG CONTAINS TIME STEP AND CAMERA FOV DATA - ASSUMING LATEST EXPORT FLOW GRAPH VERSION - PROCEEDING
		local aPositions = #()
		local aRotations = #()
		local aRolls = #()
		
		local p3NextVector = fnGetNextFlowLogPoint3 sLogStream
		if p3NextVector != undefined then
		(
			/*
			local bPositionFirst = not (fnVectorIsNormalized p3NextVector)
			
			if bPositionFirst then
			(
				append aPositions p3NextVector
			)
			else
			(
				append aRotations p3NextVector
			)
			*/
			append aPositions p3NextVector
			p3NextVector = fnGetNextFlowLogPoint3 sLogStream
			local iElement = 2
			
			while p3NextVector != undefined do
			(
				case iElement of
				(
					1: append aPositions p3NextVector
					2: append aRotations p3NextVector
					3: append aRolls p3NextVector
				)
				
				if iElement == 2 then
				(--GRAB FLOAT IF NEXT IS ROLL
					p3NextVector = fnGetNextFlowLogFloat sLogStream
				)
				else
				(
					p3NextVector = fnGetNextFlowLogPoint3 sLogStream
				)
				--DIDNT WANT TO BOTHER WITH MODULO
				iElement += 1
				if iElement == 4 then iElement = 1
			)
		)
		
		print ("Positions:" + aPositions as string)
		print ("Rotations:" + aRotations as string)
		print ("Rolls:" + aRolls as string)
		
		if aPositions.count > 0 and aRotations.count > 0 then
		(
			local nCamera = freecamera()
			nCamera.name = "cameraFromEditor"
			nCamera.fovType = 2
			nCamera.curFov = fFOV
			animationRange = interval 1 (aMin #(aPositions.count,aRotations.count,aRolls.count))
			--frameRate = 1/fTimeStep
			
			with animate on
			(
				for i = 1 to (aMin #(aPositions.count,aRotations.count,aRolls.count)) do
				(
					at time i
					(
						local p3YAxis = aRotations[i]
						local qRollQuat = quat -(radToDeg aRolls[i]) p3YAxis
						local p3XAxis = normalize ((cross p3YAxis [0,0,1])*qRollQuat)
						local p3ZAxis = (cross p3XAxis p3YAxis)
						nCamera.transform = (matrix3 (p3XAxis) (p3ZAxis) (-p3YAxis) (aPositions[i]*fUnitScale))
						--nCamera.pos = aPositions[i]
					)
				)
			)
		)
	)
	else
	(
		print "Error!\nCouldn't find fixed_time_step and fov data.\nPlease delete editor.log and re-export camera movement with enclosed flow-graph script!"
	)
)
else 
(
	print "Editor.log not found.\nPlease save your max file and place the Editor.log in the same directory!"
)