@ECHO OFF
REM Bootstraps the Cloud Canvas test runner python script.

SETLOCAL

SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

SET ROOT_DIR=%CMD_DIR%\..\..\..

REM Convert to absoulte path
pushd %ROOT_DIR%
set ROOT_DIR=%CD%
popd

SET PYTHON_DIR=%ROOT_DIR%\Tools\Python
IF EXIST %PYTHON_DIR% GOTO PYTHON_READY
ECHO Missing: %PYTHON_DIR%
GOTO FAILED
:PYTHON_READY
SET PYTHON=%PYTHON_DIR%\python.cmd

SET parsedparams=
SET skipaztests=0

:doParse
if NOT "%1" == "" (
    if "%1" == "--noaztests" (
        SET skipaztests=1
        SHIFT
        GOTO :doParse
    )
    SET parsedparams=%parsedparams% %1
    SHIFT
    GOTO :doParse
)

SET TEST_RUNNER=%PYTHON% %ROOT_DIR%\Tools\lmbr_aws\test\test_runner.py %parsedparams%
ECHO %TEST_RUNNER%
CALL %TEST_RUNNER%
IF ERRORLEVEL 1 GOTO FAILED

if %skipaztests% == 1 (
    ECHO Skipping AzTests
    GOTO :SUCCESS
)

ECHO Initiating AzTestScanner...

SET LMBR_TEST=%ROOT_DIR%\lmbr_test.cmd
SET LMBR_TEST_RUNNER=%LMBR_TEST% scan --dir %ROOT_DIR%\Bin64vc140.Debug.test --html-report
ECHO %LMBR_TEST_RUNNER%
CALL %LMBR_TEST_RUNNER%
IF ERRORLEVEL 1 GOTO FAILED

:SUCCESS
EXIT /b 0

:FAILED
EXIT /b 1

