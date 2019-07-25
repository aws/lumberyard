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

pushd %~dp0%

SET BASE_PATH=%~dp0


REM Locate Python. We will force the use of our version of Python by setting 
REM PYTHONHOME (done by win\python.cmd) and clearing any PYTHONPATH
REM (https://docs.python.org/2/using/cmdline.html#envvar-PYTHON_DIRECTORY)

SET PYTHONPATH="%BASE_PATH%\Tools\build\waf-1.7.13"

SET PYTHON_DIRECTORY=%BASE_PATH%Tools\Python
IF EXIST "%PYTHON_DIRECTORY%" GOTO pythonPathAvailable

GOTO pythonNotFound

:pythonPathAvailable
SET PYTHON_EXECUTABLE=%PYTHON_DIRECTORY%\python.cmd
IF NOT EXIST "%PYTHON_EXECUTABLE%" GOTO pythonNotFound

REM Execute the WAF script
SET COMMAND_ID="%COMPUTERNAME%-%TIME%"
IF DEFINED BUILD_TAG (
    IF DEFINED P4_CHANGELIST (
        SET COMMAND_ID="%BUILD_TAG%.%P4_CHANGELIST%-%TIME%"
    )
)

SET PARAM=%1

REM call is required here otherwise control won't be returned to lmbr_waf.bat and the rest of the file doesn't execute
CALL "%PYTHON_EXECUTABLE%" -m pytest %PARAM%

GOTO end

:pythonNotFound
ECHO Unable to locate the local python inside the SDK.  Please contact support.
popd
EXIT /b 1

:end
popd

