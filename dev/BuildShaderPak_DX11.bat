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
REM BuildShaderPak_DX11 shaderlist.txt

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT

REM If a bin folder was registered, validate the presence of the binfolder/rc/rc.exe
IF ERRORLEVEL 1 (
    ECHO unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%

set SOURCESHADERLIST=%1
set GAMENAME=SamplesProject
set DESTSHADERFOLDER=Cache\%GAMENAME%\PC\user\Cache\Shaders

set SHADERPLATFORM=PC
rem other available platforms are GL4 GLES3 METAL and the Console platforms
rem if changing the above platform, also change the below folder name to match
set SHADERFLAVOR=D3D11

if [%1] == [] GOTO MissingShaderParam
GOTO DoCopy

:MissingShaderParam
echo source Shader List not specified, using %DESTSHADERFOLDER%\shaderlist.txt by default 
GOTO DoCompile

:DoCopy
echo Copying "%SOURCESHADERLIST%" to %DESTSHADERFOLDER%\shaderlist.txt
xcopy /y "%SOURCESHADERLIST%" %DESTSHADERFOLDER%\shaderlist.txt*
IF ERRORLEVEL 1 GOTO CopyFailed
GOTO DoCompile

:CopyFailed
echo copy failed.
GOTO Failed

:DoCompile
echo Compiling shaders...
%BINFOLDER%\ShaderCacheGen.exe /BuildGlobalCache /ShadersPlatform=%SHADERPLATFORM%

echo Packing Shaders...
call .\lmbr_pak_shaders.bat %SHADERFLAVOR% %GAMENAME%

GOTO Success

:Failed
echo ---- process failed ----
exit /b 1

:Success
echo ---- Process succeeded ----
exit /b 0
