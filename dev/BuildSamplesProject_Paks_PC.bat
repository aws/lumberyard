@echo off

setlocal enabledelayedexpansion 

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

REM Parse command line arguments for OPTIONAL parameters. Looking for --recompress or --use_fastest.
set RECOMPRESS_NAME=--recompress
set /A RECOMPRESS_VALUE=0
set USE_FASTEST_NAME=--use_fastest
set /A USE_FASTEST_VALUE=0
:CmdLineArgumentsParseLoop
if "%1"=="" goto CmdLineArgumentsParsingDone
    if "%1"=="%RECOMPRESS_NAME%" (
        set /A RECOMPRESS_VALUE=1
        goto GotValidArgument
    )
    if "%1"=="%USE_FASTEST_NAME%" (
        set /A USE_FASTEST_VALUE=1
        goto GotValidArgument
    )
    REM If we are here, the user gave us an unexpected argument. Let them know and quit.
    echo "%1" is an invalid argument, optional arguments are "%RECOMPRESS_NAME%" or "%USE_FASTEST_NAME%"
    echo --recompress: If present, the ResourceCompiler (RC.exe) will decompress and compress back each
    echo               PAK file found as they are transferred from the cache folder to the game_pc_pak folder.
    echo --use_fastest: As each file is being added to its PAK file, they will be compressed across all
    echo                available codecs (ZLIB, ZSTD and LZ4) and the one with the fastest decompression time
    echo                will be chosen. The default is to always use ZLIB.
    exit /b 1
:GotValidArgument
    shift
goto CmdLineArgumentsParseLoop
:CmdLineArgumentsParsingDone

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT

REM If a bin folder was registered, validate the presence of the binfolder/rc/rc.exe
IF ERRORLEVEL 1 (
    ECHO unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%

echo ----- Processing Assets Using Asset Processor Batch ----
.\%BINFOLDER%\AssetProcessorBatch.exe /gamefolder=SamplesProject /platforms=pc
IF ERRORLEVEL 1 GOTO AssetProcessingFailed

echo ----- Creating Packages ----
rem lowercase is intentional, since cache folders are lowercase on some platforms
.\%BINFOLDER%\rc\rc.exe /job=%BINFOLDER%\rc\RCJob_Generic_MakePaks.xml /p=pc /game=samplesproject /recompress=%RECOMPRESS_VALUE% /use_fastest=%USE_FASTEST_VALUE%
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




