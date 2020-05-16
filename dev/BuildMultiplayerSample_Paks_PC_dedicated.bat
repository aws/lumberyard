@echo off

setlocal enabledelayedexpansion 

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

set "BINFOLDER_HINT_NAME=--binfolder-hint"
set "BINFOLDER_HINT="
:CmdLineArgumentsParseLoop
if "%1"=="" goto CmdLineArgumentsParsingDone
    if "%1"=="%BINFOLDER_HINT_NAME%" (
        REM The next parameter is the hint directory
        set BINFOLDER_HINT=%2%
        shift
        goto GotValidArgument
    )
    REM If we are here, the user gave us an unexpected argument. Let them know and quit.
    echo "%1" is an invalid argument, optional arguments are "%BINFOLDER_HINT_NAME%"
    echo %BINFOLDER_HINT_NAME%^=^<folder_name^>: A hint to indicate the folder name to use for finding the windows binaries i.e Bin64vc142
    exit /b 1
:GotValidArgument
    shift
goto CmdLineArgumentsParseLoop
:CmdLineArgumentsParsingDone

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT %BINFOLDER_HINT%

REM If a bin folder was registered, validate the presence of the binfolder/rc/rc.exe
IF ERRORLEVEL 1 (
    ECHO unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%

echo ----- Processing Assets Using Asset Processor Batch ----
.\%BINFOLDER%\AssetProcessorBatch.exe /gamefolder=MultiplayerSample /platforms=pc
IF ERRORLEVEL 1 GOTO AssetProcessingFailed

echo ----- Creating Packages ----
rem lowercase is intentional, since cache folders are lowercase on some platforms
.\%BINFOLDER%\rc\rc.exe /job=%BINFOLDER%\rc\RCJob_Generic_MakePaks.xml /p=pc /game=multiplayersample /trg=MultiplayerSample_pc_Paks_Dedicated
IF ERRORLEVEL 1 GOTO RCFailed

echo ----- Done -----
cd "%ORIGINALDIRECTORY%"
exit /b 0

:RCFailed
echo ---- RC PAK failed ----
cd "%ORIGINALDIRECTORY%"
exit /b 1

:AssetProcessingFailed
echo ---- ASSET PROCESSING FAILED ----
cd "%ORIGINALDIRECTORY%"
exit /b 1





