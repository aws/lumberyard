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


REM search for the engine root from the engine.json if possible
IF NOT EXIST engine.json GOTO noSetupConfig

FOR /F "tokens=1,2*" %%A in ('findstr /I /N "ExternalEnginePath" engine.json') do SET ENGINE_ROOT=%%C

REM Clear the trailing comma if any
SET ENGINE_ROOT=%ENGINE_ROOT:,=%

REM Trim the double quotes
SET ENGINE_ROOT=%ENGINE_ROOT:"=%

IF "%ENGINE_ROOT%"=="" GOTO noSetupConfig

IF NOT EXIST "%ENGINE_ROOT%" GOTO noSetupConfig

REM Set the base path to the value
SET BASE_PATH=%ENGINE_ROOT%\
ECHO [WAF] Engine Root: %BASE_PATH%
GOTO pythonPathSet

:noSetupConfig
SET BASE_PATH=%~dp0
ECHO [WAF] Engine Root: %BASE_PATH%

:pythonPathSet

SET WAF_SCRIPT="%BASE_PATH%Tools\build\waf-1.7.13\lmbr_waf"

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
SET COMMAND_ID="%COMPUTERNAME%-%TIME%"
IF DEFINED BUILD_TAG (
    IF DEFINED P4_CHANGELIST (
        SET COMMAND_ID="%BUILD_TAG%.%P4_CHANGELIST%-%TIME%"
    )
)
pushd %BASE_PATH%
REM call is required here otherwise control won't be returned to lmbr_waf.bat and the rest of the file doesn't execute
CALL "%PYTHON_EXECUTABLE%" %WAF_SCRIPT% %*

IF ERRORLEVEL 1 (
    IF EXIST "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\build_metrics_overrides.py" (
        call "%PYTHON_EXECUTABLE%" "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\write_build_metric.py" WafBuildResult 0 Unitless %*
        
        if EXIST "%BASE_PATH%build_metrics.txt" (
            call powershell -Command "(gc '%BASE_PATH%build_metrics.txt') -replace '#BUILD_RESULT#', 'False' | Set-Content '%BASE_PATH%build_metrics.txt'"
        )
    ) ELSE (
        ECHO Metrics were enabled but build_metrics_overrides file does not exist, cannot write WafBuildResult.
    )
    GOTO fail
) ELSE (
    IF EXIST "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\build_metrics_overrides.py" (
        call "%PYTHON_EXECUTABLE%" "%BASE_PATH%Tools\build\waf-1.7.13\build_metrics\write_build_metric.py" WafBuildResult 1 Unitless %*
        if EXIST "%BASE_PATH%build_metrics.txt" (
            call powershell -Command "(gc '%BASE_PATH%build_metrics.txt') -replace '#BUILD_RESULT#', 'True' | Set-Content '%BASE_PATH%build_metrics.txt'"
        )
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