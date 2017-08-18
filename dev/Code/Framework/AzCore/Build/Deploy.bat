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

echo. Deploying AZCore...

@REM initialize environment variables
setlocal enabledelayedexpansion
@set SRC_PATH=%~dp0..
@set DEST_PATH=%1\AZCore

@REM copy property sheets
robocopy %SRC_PATH%\Build %DEST_PATH%\Build *.props /S /PURGE /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

@REM copy .h and .inl files
robocopy %SRC_PATH%\AZCore %DEST_PATH%\AZCore *.h *.inl /MIR /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

@REM copy lib files
robocopy %SRC_PATH%\Lib %DEST_PATH%\Lib *.* /MIR /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

@REM copy all source
robocopy %SRC_PATH%\Build %DEST_PATH%\Source\Build *.vcxproj *.vcxproj.filters *.sln *.props /S /PURGE /NFL /NDL /R:0
if errorlevel 4 goto copy_failed
robocopy %SRC_PATH%\AZCore %DEST_PATH%\Source\AZCore *.h *.inl *.cpp *.c /MIR /NFL /NDL /R:0
if errorlevel 4 goto copy_failed
robocopy %SRC_PATH%\Tests %DEST_PATH%\Source\Tests *.h *.inl *.cpp *.c /MIR /NFL /NDL /R:0
if errorlevel 4 goto copy_failed

goto end

:COPY_FAILED
exit /b 1

:END
exit /b 0
