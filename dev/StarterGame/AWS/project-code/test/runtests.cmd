@ECHO OFF 

REM $Revision: #1 $

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

REM  This cmd file could live in two places. The first is the origional source
REM  location. The second is where it is copied to by initialize-project.
REM 
REM    {root}\Tools\lmbr_aws\aws_resource_manager\default_content\program-code\test
REM    {root}\{game}\AWS\program_code\test
REM
REM  Below we try to find {root} by looking for lmbr_aws.cmd relative to these locations

SET ROOT_DIR=%CMD_DIR%\..\..\..\..\..\..
IF EXIST %ROOT_DIR%\lmbr_aws.cmd GOTO ROOT_DIR_READY
SET ROOT_DIR=%CMD_DIR%\..\..\..\..
IF EXIST %ROOT_DIR%\lmbr_aws.cmd GOTO ROOT_DIR_READY
ECHO Can't find root directory.
GOTO :EOF
:ROOT_DIR_READY

REM Convert to absoulte path
pushd %ROOT_DIR%
set ROOT_DIR=%CD%
popd

SET PYTHON_DIR=%ROOT_DIR%\Tools\Python
IF EXIST %PYTHON_DIR% GOTO PYTHON_READY
ECHO Missing: %PYTHON_DIR%
GOTO :EOF
:PYTHON_READY
SET PYTHON=%PYTHON_DIR%\python.cmd

SET AWS_SDK_DIR=%ROOT_DIR%\Tools\AWSPythonSDK
IF EXIST %AWS_SDK_DIR%\append_PYTHONPATH.cmd GOTO AWS_SDK_READY
ECHO Missing: %AWS_SDK_DIR%
GOTO :EOF
:AWS_SDK_READY

CALL %PYTHON% -c "import mock" 2> nul
IF ERRORLEVEL 1 GOTO NO_MOCK
GOTO MOCK_READY

:NO_MOCK

ECHO.
ECHO ERROR: The Python "mock" module is not installed. It is required to run the tests. Use "%PYTHON_DIR%\pip install mock" to install it."
ECHO.
GOTO :EOF

:MOCK_READY

REM When running in a Lambda function the path includes the directory containing
REM the uploaded code (TARGET_DIR for these tests) and the AWS REM SDK (boto3)

SET TARGET_DIR=%CMD_DIR%\..

REM Convert to absoulte path
pushd %TARGET_DIR%
set TARGET_DIR=%CD%
popd

SET PYTHONPATH=%TARGET_DIR%
CALL %AWS_SDK_DIR%\append_PYTHONPATH.cmd

IF "%*"=="" %PYTHON% -m unittest discover --verbose --failfast 
IF NOT "%*"=="" %PYTHON% -m unittest --verbose %*