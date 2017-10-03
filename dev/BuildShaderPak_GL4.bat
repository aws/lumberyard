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

REM Detect the version of VS and determine the bin64 folder
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

set SHADERPLATFORM=GL4
rem other available platforms are GL4 GLES3 METAL and the Console platforms
rem if changing the above platform, also change the below folder name to match
set SHADERFLAVOR=GL4

xcopy /y %SOURCESHADERLIST% Cache\%GAMENAME%\PC\user\Cache\Shaders\shaderlist.txt*
%BINFOLDER%\ShaderCacheGen.exe /BuildGlobalCache /ShadersPlatform=%SHADERPLATFORM%
call .\lmbr_pak_shaders.bat %SHADERFLAVOR% %GAMENAME%

