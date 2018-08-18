@echo off
REM
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM
REM  REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM
REM

echo Installing Visual Studio Visualizers.
SETLOCAL

SET "DOCUMENTS=%USERPROFILE%\Documents"
set FOUND=0

REM Set current folder
cd /d %~dp0

:INSTALL_VS15
SET "FOLDER=%DOCUMENTS%\Visual Studio 2017"
SET "VISUALIZERFOLDER=%FOLDER%\Visualizers"
IF EXIST "%FOLDER%" (
    echo     Visual Studio 2017
    IF NOT EXIST "%VISUALIZERFOLDER%" (
        mkdir "%VISUALIZERFOLDER%"
    )
	
	REM we need to remove azcore.natvis as we had to split it into two versions
	IF EXIST "%VISUALIZERFOLDER%\azcore.natvis" (
		del "%VISUALIZERFOLDER%\azcore.natvis"
	)

    REM rename vs2015 natvis file to vs2017 on copy
    copy azcore_vs2015.natvis "%VISUALIZERFOLDER%\azcore_vs2017.natvis"
    copy azcore.natjmc "%VISUALIZERFOLDER%"
    copy azcore.natstepfilter "%VISUALIZERFOLDER%"
	IF NOT %ERRORLEVEL% == 0 (
		echo "Failed to find Visual Studio 2017 user folder."
	) ELSE (
		SET FOUND=1
	)
)

:INSTALL_VS14
SET "FOLDER=%DOCUMENTS%\Visual Studio 2015"
SET "VISUALIZERFOLDER=%FOLDER%\Visualizers"
IF EXIST "%FOLDER%" (
    echo     Visual Studio 2015
    IF NOT EXIST "%VISUALIZERFOLDER%" (
        mkdir "%VISUALIZERFOLDER%"
    )
	
	REM we need to remove azcore.natvis as we had to split it into two versions
	IF EXIST "%VISUALIZERFOLDER%\azcore.natvis" (
		del "%VISUALIZERFOLDER%\azcore.natvis"
	)
	
    copy azcore_vs2015.natvis "%VISUALIZERFOLDER%"
    copy azcore.natjmc "%VISUALIZERFOLDER%"
    copy azcore.natstepfilter "%VISUALIZERFOLDER%"
	IF NOT %ERRORLEVEL% == 0 (
		echo "Failed to find Visual Studio 2015 user folder."
	) ELSE (
		SET FOUND=1
	)
)

:INSTALL_VS12
SET "FOLDER=%DOCUMENTS%\Visual Studio 2013"
SET "VISUALIZERFOLDER=%FOLDER%\Visualizers"
IF EXIST "%FOLDER%" (
    echo     Visual Studio 2013
    IF NOT EXIST "%VISUALIZERFOLDER%" (
        mkdir "%VISUALIZERFOLDER%"
    )
	
	REM we need to remove azcore.natvis as we had to split it into two versions
	IF EXIST "%VISUALIZERFOLDER%\azcore.natvis" (
		del "%VISUALIZERFOLDER%\azcore.natvis"
	)
	
    copy azcore_vs2013.natvis "%VISUALIZERFOLDER%"
    copy azcore.natjmc "%VISUALIZERFOLDER%"
    copy azcore.natstepfilter "%VISUALIZERFOLDER%"
	IF NOT %ERRORLEVEL% == 0 (
		echo "Failed to find Visual Studio 2013 user folder."
	) ELSE (
		SET FOUND=1
	)
)

IF %FOUND% == 1 goto SUCCESS
echo Failed to find any Visual Studio user folder.

:SUCCESS
echo.
echo Done!
goto END

:FAILED
echo Failed to install visualization file

:END
