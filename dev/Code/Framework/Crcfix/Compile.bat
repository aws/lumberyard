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


setlocal enabledelayedexpansion

REM Set the working folder to the containing folder
SET COMPILE_WORK_FOLDER=%~dp0

REM Set the compiler to build Unity Builds
SET CL="/DAZ_UNITY_BUILD"

echo =================
echo =================
echo   Windows win32
echo =================
echo =================

echo =================
echo Release
echo =================
"%VS120COMNTOOLS%\..\IDE\devenv" "crcfix.sln" /Rebuild "Release|Win32"
IF ERRORLEVEL 1 GOTO BuildFailed

REM If we got here we have a successful build
GOTO BuildSuccess

:BuildFailed
echo ***********************************
echo ********** BUILD FAILURE **********
echo ***********************************
exit /b 1

:BuildSuccess
echo ***********************************
echo ********** BUILD SUCCESS **********
echo ***********************************