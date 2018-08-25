@IF "%QTDIR%" == "" (
    echo QTDIR environment variable is not set!
    EXIT /B 1
)

%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Editor 										-ts 	.\Editor_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\ComponentEntityEditorPlugin 			-ts 	.\ComponentEntityEditorPlugin_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\CryDesigner 							-ts 	.\CryDesigner_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\DeploymentTool 						-ts 	.\DeploymentTool_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\EditorAssetImporter 					-ts 	.\EditorAssetImporter_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\EditorAudioControlsEditor				-ts 	.\EditorAudioControlsEditor_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\EditorCommon 							-ts 	.\EditorCommon_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\EditorUI_QT 							-ts 	.\EditorUI_QT_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\FBXPlugin 								-ts 	.\FBXPlugin_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\MaglevControlPanel 					-ts 	.\MaglevControlPanel_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\ParticleEditorPlugin 					-ts 	.\ParticleEditorPlugin_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\PerforcePlugin 						-ts 	.\PerforcePlugin_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Sandbox\Plugins\UiCanvasEditor 						-ts 	.\UiCanvasEditor_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\..\Gems												-ts 	.\Gems_zh_cn.ts
%QTDIR%\lupdate -noobsolete ..\..\..\Framework												-ts 	.\AzToolsFramework_zh_cn.ts