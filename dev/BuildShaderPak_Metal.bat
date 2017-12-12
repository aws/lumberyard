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
REM BuildShaderPak_Metal MyProject (eg. FeatureTests)

if [%1] == [] GOTO MissingRequiredParam
GOTO SetParams

:MissingRequiredParam
echo Missing one or more required params. Example: BuildShaderPak_Metal.bat gameProjectName
echo(
echo positional arguments:
echo    gameProjectName     The name of the game.
echo(
GOTO Failed

:SetParams
set gamefoldername=%1
set shaderflavors="metal/gmem128 metal/gmem256"
set targetplatform=ios

REM ShaderCacheGen.exe is not currently able to generate the metal shader cache,
REM so for the time being we must copy the compiled shaders from the iOS device.
REM AppData/Documents/shaders/cache/* -> Cache\MyProject\ios\user\cache\shaders\cache\*
REM
REM One ShaderCacheGen.exe can generate the metal shaders (https://issues.labcollab.net/browse/LMBR-18201),
REM we should be able to call this directly (after copying the appropriate shaderlist.txt as is done in BuildShaderPak_DX11.bat)
REM Bin64\ShaderCacheGen.exe /BuildGlobalCache /ShadersPlatform=%targetplatform%

echo Packing Shaders...
call .\lmbr_pak_shaders.bat %shaderflavors% %gamefoldername% %targetplatform%

GOTO Success

:Failed
echo ---- process failed ----
exit /b 1

:Success
echo ---- Process succeeded ----
exit /b 0