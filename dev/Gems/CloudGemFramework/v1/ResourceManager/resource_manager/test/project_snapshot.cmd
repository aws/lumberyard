@ECHO OFF 

REM $Revision: #2 $

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

SET ROOT_DIR=%CMD_DIR%\..\..\..\..\..\..

FOR /F "delims=" %%F IN ("%ROOT_DIR%") DO SET "ROOT_DIR=%%~fF"

SET PYTHON_DIR=%ROOT_DIR%\Tools\Python
IF EXIST %PYTHON_DIR% GOTO PYTHON_READY
ECHO Missing: %PYTHON_DIR%
GOTO :EOF
:PYTHON_READY
SET PYTHON=%PYTHON_DIR%\python3.cmd

SET PYTHONPATH="%PYTHONPATH%;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\\ResourceManager;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\\AWS\\common-code\\ResourceManagerCommon;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\\AWS\\common-code\\Utils;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\\AWS\\common-code\\lib;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\ResourceManager\\lib;%ROOT_DIR%\\Gems\\CloudGemFramework\\v1\\ResourceManager\\resource_manager\\test"

%PYTHON% %CMD_DIR%\project_snapshot.py %*

