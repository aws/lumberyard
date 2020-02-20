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
REM Auto builds Lumberyard on Windows ready for GameLift hosting
REM Example: BuildGameLiftServer.bat MultiplayerSample all VS2017
REM Script can accept arguments to skip interactive mode
REM %~1 = Project Name
REM %~2 = Compile option to perform, one of : "all", "server", "assets"
REM %~3 = VisualStudio re-distributable can include ["VS2017"]. Defaults to VS2017


SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

ECHO This batch script will attempt to prepare and package a server build ready to be uploaded to GameLift.

REM Push root directory, assuming we are located under Gems\GameLift\Code\Scripts
PUSHD "%~dp0"\..\..\..\..\

REM CLI MODE BELOW

SET PROJECT_NAME_PARAM=%~1
SET COMPILE_OPTION_PARAM=%~2
SET VS_VERSION=%~3

REM Work out what version of visual studio should we use
IF "%VS_VERSION%" == "" (
    SET VS_VERSION="VS2017"
)
call :SetUpVisualStudio %VS_VERSION%


IF "%PROJECT_NAME_PARAM%" NEQ "" (
    IF NOT EXIST "%PROJECT_NAME_PARAM%\" (
        GOTO :InvalidProjectName
    )
    
    IF "%COMPILE_OPTION_PARAM%" NEQ "" (
        SET PROJECT_NAME=%PROJECT_NAME_PARAM%
    
        IF /i "%COMPILE_OPTION_PARAM%" EQU "all" (
            CALL :CompileServer
            CALL :CompileAssets
        ) ELSE IF /i "%COMPILE_OPTION_PARAM%" EQU "server" (
            CALL :CompileServer
        ) ELSE IF /i "%COMPILE_OPTION_PARAM%" EQU "assets" (
            CALL :CompileAssets
        )
        
        ECHO ----- DONE -----
        POPD
        EXIT /B 0
    )
)

REM INTERACTIVE MODE BELOW

SET /p PROJECT_NAME="Enter Project Name (eg. MultiplayerProject): "
IF "%PROJECT_NAME%" EQU "" (
    GOTO :InvalidProjectName
)
IF NOT EXIST "%PROJECT_NAME%\" (
    GOTO :InvalidProjectName
)

SET /p ConfirmCompileOptionInput="Compile options [all, server, assets]: "
IF "%ConfirmCompileOptionInput%" EQU "" (
    GOTO :BuildFailed
)

SET /p ConfirmVisualStudioVersionInput="VisualStudio version [VS2017]: "
IF "%ConfirmVisualStudioVersionInput%" EQU "" (
    GOTO :UnsupportVisualStudio
) ELSE (
    call :SetUpVisualStudio %ConfirmVisualStudioVersionInput%
)

IF /i "%ConfirmCompileOptionInput%" EQU "all" (
    CALL :CompileServer
    CALL :CompileAssets
) ELSE IF /i "%ConfirmCompileOptionInput%" EQU "server" (
    CALL :CompileServer
) ELSE IF /i "%ConfirmCompileOptionInput%" EQU "assets" (
    CALL :CompileAssets
) ELSE (
    GOTO :InvalidCompileOption
)

ECHO ----- DONE -----
POPD
EXIT /B 0

:SetUpVisualStudio 
    SET VS_VERSION=%~1
    
    IF /i "%VS_VERSION%"=="VS2017" (
        Echo Set build environment for VS2017
        SET VS_VERSION=VS2017
        SET VS_VERSION_LOWER=vs2017
        SET BINFOLDER=Bin64vc141
    ) ELSE (
        ECHO Unknown visual studio version: %VS_VERSION%
        GOTO :UnsupportVisualStudio
    )
    EXIT /B 1

:CompileServer
    ECHO ----- COMPILING DEDICATED SERVER -----
    CALL lmbr_waf build_win_x64_%VS_VERSION_LOWER%_release_dedicated -p game_and_engine --progress --enabled-game-projects %PROJECT_NAME% || GOTO :BuildFailed 
    GOTO :EOF

:CompileAssets
    SET DESTINATION_PROJECT_FOLDER_NAME=%CD%\%PROJECT_NAME%_pc_Paks_Dedicated\
    
    ECHO ----- PROCESSING ASSETS USING ASSET PROCESSOR BATCH ----
    CALL %BINFOLDER%\AssetProcessorBatch.exe /gamefolder=%PROJECT_NAME% /platforms=pc || GOTO :AssetProcessingFailed

    ECHO ----- CREATING PACKAGES ----
    CALL %BINFOLDER%\rc\rc.exe /job=%BINFOLDER%\rc\RCJob_Generic_MakePaks.xml /p=pc /game=%PROJECT_NAME% /trg=%PROJECT_NAME%_pc_Paks_Dedicated || GOTO :RCFailed

    PUSHD %DESTINATION_PROJECT_FOLDER_NAME%
    
    ECHO ----- COPYING BINARIES FROM %BINFOLDER%.Release.Dedicated TO %DESTINATION_PROJECT_FOLDER_NAME% -----
    ECHO .pdb > exclusion_list.txt
    XCOPY %DESTINATION_PROJECT_FOLDER_NAME%\..\%BINFOLDER%.Release.Dedicated\* %CD%\%BINFOLDER%.Release.Dedicated\ /Y /I /F /R /EXCLUDE:exclusion_list.txt || (DEL exclusion_list.txt && GOTO :CopyFailed)
    DEL exclusion_list.txt
    
    ECHO ----- COPYING ADDITIONAL FILES -----
    IF "%VS_VERSION%" EQU "VS2017" (
        XCOPY "%DESTINATION_PROJECT_FOLDER_NAME%\..\Tools\Redistributables\Visual Studio 2017\VC_redist.x64.exe" %CD% /Y /I /F /R || GOTO :CopyFailed
        ECHO VC_redist.x64.exe /q > install.bat || GOTO :CopyFailed
    ) ELSE (
        ECHO "Unsupported VS version %VS_VERSION%. Please add mapping above"
        GOTO :CopyFailed
    )
    POPD
    GOTO :EOF
    
:BuildFailed
    ECHO ----- BUILD FAILED -----
    POPD
    EXIT /B 1

:AssetProcessingFailed
    ECHO ----- ASSET PROCESSING FAILED -----
    POPD
    EXIT /B 1

:RCFailed
    ECHO ----- RC PAK FAILED -----
    POPD
    EXIT /B 1

:CopyFailed
    ECHO ----- COPY FAILED -----
    POPD
    EXIT /B 1

:InvalidProjectName
    ECHO ----- INVALID PROJECT NAME -----
    POPD
    ECHO Make sure the project folder exists.
    CALL :PrintUsage %~0
    EXIT /B 1

:InvalidCompileOption
    ECHO ----- INVALID COMPILE OPTION -----
    POPD
    CALL :PrintUsage %~0
    EXIT /B 1
    
:UnsupportVisualStudio
    ECHO ----- INVALID VISUAL STUDIO OPTION ----
    POPD
    ECHO Update visual studio versions at top of script
    CALL :PrintUsage %~0
    EXIT /B 1

:PrintUsage
    ECHO ----- Usage: %~nx1 [ProjectName] [all, server, assets] [VS2017] -----
    ECHO ProjectName: The name of your project. eg: MultiplayerProject
    ECHO all: This option will compile the dedicated server and compile the server assets.
    ECHO server: This option will compile the dedicated server.
    ECHO assets: This option will compile the server assets.
    ECHO vs: This option provides the version of VisualStudio to target [VS2017^]
    GOTO :EOF
 


