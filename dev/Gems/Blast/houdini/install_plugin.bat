
Xcopy /E /I /r /y %~dp0config %UserProfile%\Documents\houdini18.0\config
Xcopy /E /I /r /y %~dp0otls %UserProfile%\Documents\houdini18.0\otls
Xcopy /E /I /r /y %~dp0python2.7libs %UserProfile%\Documents\houdini18.0\python2.7libs
Xcopy /E /I /r /y %~dp0toolbar %UserProfile%\Documents\houdini18.0\toolbar
		
Xcopy /E /I /r /y %~dp0dso %UserProfile%\Documents\houdini18.0\dso
Xcopy /E /I /r /y %~dp0dependencies %UserProfile%\Documents\houdini18.0\dependencies

powershell -command "Start-Process powershell '-File %~dp0\set_path.ps1' -Verb runas"
powershell -command "Start-Process cmd -ArgumentList '/c %~dp0\install_sqlite.bat' -Verb runas"
