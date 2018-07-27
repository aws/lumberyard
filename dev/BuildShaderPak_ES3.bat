@ECHO OFF
REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM
REM Original file Copyright Crytek GMBH or its affiliates, used under license.

REM expected usage:
REM BuildShaderPak_ES3 gameProjectName

if [%1] == [] GOTO MissingRequiredParam
GOTO SetParams

:MissingRequiredParam
echo Missing one or more required arguments. Example: BuildShaderPak_ES3.bat gameProjectName
echo(
echo positional arguments:
echo    gameProjectName     The name of the game.
echo(
GOTO Failed

:SetParams
set PLATFORM=es3
set SHADERFLAVOR="gles3*"
set GAMENAME=%1

call .\lmbr_pak_shaders.bat %SHADERFLAVOR% %GAMENAME% %PLATFORM%

if %ERRORLEVEL% NEQ 0 GOTO Failed
GOTO Success

:Failed
echo ---- process failed ----
exit /b 1

:Success
echo ---- Process succeeded ----
exit /b 0
