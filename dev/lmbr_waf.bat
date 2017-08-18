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


SET BASE_PATH=%~dp0

REM Locate Python. We will force the use of our version of Python by setting 
REM PYTHONHOME (done by win\python.cmd) and clearing any PYTHONPATH
REM (https://docs.python.org/2/using/cmdline.html#envvar-PYTHON_DIRECTORY)

SET AZ_TEST_DIR=%BASE_PATH%Code\Tools\AzTestScanner
SET PYTHONPATH=%AZ_TEST_DIR%

SET PYTHON_DIRECTORY=%BASE_PATH%Tools\Python
IF EXIST "%PYTHON_DIRECTORY%" GOTO pythonPathAvailable

GOTO pythonNotFound

:pythonPathAvailable
SET PYTHON_EXECUTABLE=%PYTHON_DIRECTORY%\python.cmd
IF NOT EXIST "%PYTHON_EXECUTABLE%" GOTO pythonNotFound

REM Execute the WAF script
pushd "%BASE_PATH%"

SET BUILD_ID="%COMPUTERNAME%.%TIME%"
call "%PYTHON_EXECUTABLE%" "%BASE_PATH%Tools\build\waf-1.7.13\lmbr_waf" %*

IF ERRORLEVEL 1 (
    IF EXIST "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\build_metrics_overrides.py" (
        call  "%PYTHON_EXECUTABLE%" "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\write_build_metric.py" WafBuildResult 0 Unitless %*
    ) ELSE (
        ECHO Metrics were enabled but build_metrics_overrides file does not exist, cannot write WafBuildResult.
    )
    GOTO fail
) ELSE (
    IF EXIST "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\build_metrics_overrides.py" (
        call "%PYTHON_EXECUTABLE%" "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\write_build_metric.py" WafBuildResult 1 Unitless %*
    ) ELSE (
        ECHO Metrics were enabled but build_metrics_overrides file does not exist, cannot write WafBuildResult.
    )
)

GOTO end

:pythonNotFound
ECHO Unable to locate the local python inside the SDK.  Please contact support.
GOTO fail

:configureFail
ECHO Unable to configure SDK environment.  Please run SetupAssistant.exe in the Bin64 folder first.
GOTO fail

:buildFail
ECHO Error building Lumberyard Setup Assistant
GOTO fail

:fail
popd
EXIT /b 1

:end
popd
