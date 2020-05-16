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
REM

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%
SET TOOLS_DIR=%CMD_DIR%\..\..\

SET PYTHON_DIR=%TOOLS_DIR%\Python
IF EXIST "%PYTHON_DIR%" GOTO PYTHON_DIR_EXISTS

ECHO Could not find Python in %TOOLS_DIR%
GOTO :EOF

:PYTHON_DIR_EXISTS

SET PYTHON=%PYTHON_DIR%\python3.cmd
IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python.cmd in %PYTHON_DIR%
GOTO :EOF

:PYTHON_EXISTS
SET AWS_SDK_DIR=%TOOLS_DIR%\AWSPythonSDK
IF EXIST "%AWS_SDK_DIR%\append_PYTHONPATH.cmd" GOTO AWS_SDK_READY
ECHO Missing: %AWS_SDK_DIR%\append_PYTHONPATH.cmd
GOTO :EOF
:AWS_SDK_READY

SET LMBR_AWS_DIR=%TOOLS_DIR%\lmbr_aws

SET PYTHONPATH=%LMBR_AWS_DIR%;%LMBR_AWS_DIR%\lib;%LMBR_AWS_DIR%\lib\win32;%LMBR_AWS_DIR%\lib\win32\lib
CALL "%AWS_SDK_DIR%\append_PYTHONPATH.cmd"


"%PYTHON%" "%CMD_DIR%\Upgrade.py" %*

