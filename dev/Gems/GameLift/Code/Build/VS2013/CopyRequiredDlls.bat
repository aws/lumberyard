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


@REM initialize environment variables
setlocal enabledelayedexpansion
@set THIRD_PARTY_PATH=%1
@set DEST_PATH=%2
@set CONFIGURATION=%3

echo Copying GameLift Client DLLs...
robocopy %THIRD_PARTY_PATH%\GameLiftSDK\bin\x64\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-gamelift-client.dll /S /NFL /NDL /R:0
robocopy %THIRD_PARTY_PATH%\GameLiftSDK\bin\x64\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-gamelift-client.pdb /S /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

echo Copying only necessary AWSNativeSDK DLLs for GameLift Client...
robocopy %THIRD_PARTY_PATH%\AWSNativeSDK\bin\windows\intel64\vs2013\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-core.dll /NFL /NDL /R:0
robocopy %THIRD_PARTY_PATH%\AWSNativeSDK\bin\windows\intel64\vs2013\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-gamelift.dll /NFL /NDL /R:0
robocopy %THIRD_PARTY_PATH%\AWSNativeSDK\bin\windows\intel64\vs2013\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-core.pdb /NFL /NDL /R:0
robocopy %THIRD_PARTY_PATH%\AWSNativeSDK\bin\windows\intel64\vs2013\%CONFIGURATION%\ %DEST_PATH% aws-cpp-sdk-gamelift.pdb /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

echo Copying GameLift Server DLLs...
robocopy %THIRD_PARTY_PATH%\GameLiftServerSDK\lib\x64_v120_%CONFIGURATION%_Dll\ %DEST_PATH% aws-cpp-sdk-gamelift-server.dll /S /NFL /NDL /R:0
robocopy %THIRD_PARTY_PATH%\GameLiftServerSDK\lib\x64_v120_%CONFIGURATION%_Dll\ %DEST_PATH% aws-cpp-sdk-gamelift-server.pdb /S /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

goto end

:COPY_FAILED
exit /b 1

:END
exit /b 0
