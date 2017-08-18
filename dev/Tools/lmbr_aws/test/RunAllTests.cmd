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

SET TEST_RUNNER=%PYTHON% %ROOT_DIR%\Tools\lmbr_aws\test\test_runner.py %*
ECHO %TEST_RUNNER%
CALL %TEST_RUNNER%
IF ERRORLEVEL 1 GOTO FAILED

EXIT /b 0

:FAILED
EXIT /b 1

