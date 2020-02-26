



python -V
IF %ERRORLEVEL% NEQ 0 exit /B 1
2to3 -h
IF %ERRORLEVEL% NEQ 0 exit /B 1
pydoc -h
IF %ERRORLEVEL% NEQ 0 exit /B 1
python -m venv %%TEMP%%\venv
IF %ERRORLEVEL% NEQ 0 exit /B 1
exit /B 0
