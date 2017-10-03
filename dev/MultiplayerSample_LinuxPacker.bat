@echo off

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

setlocal enabledelayedexpansion

REM Setup & VARS
SET errorCount=0
SET destination=MultiplayerSample_LinuxPackage

echo Creating Linux Package
SET /p ThirdPartyLocation= "Enter 3rdParty location: "
SET ThirdPartyLocation=%ThirdPartyLocation:"=%


REM Dev Directory list
SET folders=^
_WAF_ ^
Code ^
Engine ^
Gems ^
MultiplayerSample ^
MultiplayerSample_pc_Paks_Dedicated ^
ProjectTemplates ^
Tools

REM Dev File List
SET files=^
.p4ignore ^
AssetProcessorPlatformConfig.ini ^
bootstrap.cfg ^
MultiplayerSample_CreateGameLiftPackage.sh ^
editor.cfg ^
engine.json ^
engineroot.txt ^
lmbr_waf.bat ^
lmbr_waf.sh ^
LyzardConfig.xml ^
SetupAssistantConfig.json ^
SetupAssistantUserPreferences.ini ^
shadercachegen.cfg ^
UserSettings.xml ^
waf_branch_spec.py ^
waf_branch_spec.pyc ^
wscript

REM Complete 3rdParty Folders
SET ThirdPartyFolders=^
alembic ^
AMD ^
AWS\GameLift ^
AWS\AWSNativeSDK\1.1.13\include ^
AWS\AWSNativeSDK\1.1.13\lib\linux ^
boost ^
civetweb ^
Clang\3.7\linux_x64 ^
dyad ^
expat ^
FreeType2 ^
GoogleMock ^
hdf5 ^
ilmbase ^
jansson ^
jinja2 ^
jsmn ^
LibTomCrypt ^
LibTomMath ^
Lua ^
lz4 ^
Lzma ^
LZSS ^
markupsafe ^
md5 ^
mikkelsen ^
OculusSDK ^
OpenSSL ^
OSVR ^
p4api ^
pdcurses ^
rapidjson ^
rapidxml ^
Qt\5.6.2.2-az\gcc_64 ^
SDL2 ^
SQLite ^
squish-ccr ^
Substance ^
szip ^
tiff ^
Wwise\LTX_2016.1.1.5823\SDK\include ^
zlib

REM Setup
cd /d "%~dp0"
mkdir "%destination%\dev"
mkdir "%destination%\3rdParty"


REM Copy 3rdParty Folders over
echo %ThirdPartyLocation%\3rdParty.txt -^> %destination%\3rdParty\3rdParty.txt
copy /y "%ThirdPartyLocation%\3rdParty.txt" "%destination%\3rdParty\3rdParty.txt" > nul
IF ERRORLEVEL 1 (	
	SET errors[!errorCount!]=Failed to copy %ThirdPartyLocation%\3rdParty.txt
	SET /A errorCount+=1
)

(for %%f in (%ThirdPartyFolders%) do (
	echo %ThirdPartyLocation%\%%f -^> %destination%\3rdParty\%%f
	robocopy "%ThirdPartyLocation%\%%f" "%destination%\3rdParty\%%f" /e /copyall /xjd /mt:16 > nul
	
	IF ERRORLEVEL 2 (
		SET errors[!errorCount!]=Failed to copy %ThirdPartyLocation%\%%f
		SET /A errorCount+=1
	)
))

REM Exception: Copy QT relevant version files which sits outside of the platform specific directory
robocopy "%ThirdPartyLocation%\Qt\5.6.2.2-az" "%destination%\3rdParty\Qt\5.6.2.2-az" *.txt /copyall


REM Copy Dev folder files over
(for %%f in (%files%) do (
	echo %%f -^> %destination%\dev\%%f
	copy /y "%%f" "%destination%\dev\%%f" > nul
	
	IF ERRORLEVEL 2 (
		SET errors[!errorCount!]=Failed to copy %%f file
		SET /A errorCount+=1
	)
))


REM Copy Dev folder folders over 
(for %%f in (%folders%) do (
	echo %%f -^> %destination%\dev\%%f	
	robocopy "%%f" "%destination%\dev\%%f" /e /copyall /xjd /mt:16 > nul
	
	IF ERRORLEVEL 2	(
		SET errors[!errorCount!]=Failed to copy %%f folder
		SET /A errorCount+=1
	)
))


REM Check for errors
IF %errorCount% GTR 0 GOTO Fail


REM Create Tarball
"Tools\7za" a -ttar -aoa "%destination%.tar" "%destination%\dev" "%destination%\3rdParty"


REM Clean up after yourself.
echo Cleaning up
echo.
rmdir /s /q "%destination%" || echo Unable to clean up %destination%.  Please delete manually.


:WrapUp
echo %destination%.tar created
echo.
exit /b 0


:Fail
echo There were copy errors.  Please fix the following and try again:
(for /L %%a in (0,1,!errorCount!) do (
	echo(!errors[%%a]!
))
exit /b 1
