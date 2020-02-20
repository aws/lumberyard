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

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

SET PYTHONHOME=%CMD_DIR%\3.7.5\windows
IF EXIST "%PYTHONHOME%" GOTO PYTHONHOME_EXISTS

ECHO Could not find Python for Windows in %CMD_DIR%\..
GOTO :EOF

:PYTHONHOME_EXISTS

SET CONDA_PREFIX=%PYTHONHOME%
SET PYTHON=%PYTHONHOME%\python.exe
IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python.exe in %PYTHONHOME%
GOTO :EOF

:PYTHON_EXISTS
"%PYTHON%" %*
