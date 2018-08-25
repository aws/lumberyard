@IF "%QTDIR%" == "" (
    echo QTDIR environment variable is not set!
    EXIT /B 1
)

%QTDIR%\lrelease .\Editor_zh_cn.ts
%QTDIR%\lrelease .\ComponentEntityEditorPlugin_zh_cn.ts
%QTDIR%\lrelease .\CryDesigner_zh_cn.ts
%QTDIR%\lrelease .\DeploymentTool_zh_cn.ts
%QTDIR%\lrelease .\EditorAssetImporter_zh_cn.ts
%QTDIR%\lrelease .\EditorAudioControlsEditor_zh_cn.ts
%QTDIR%\lrelease .\EditorCommon_zh_cn.ts
%QTDIR%\lrelease .\EditorUI_QT_zh_cn.ts
%QTDIR%\lrelease .\FBXPlugin_zh_cn.ts
%QTDIR%\lrelease .\MaglevControlPanel_zh_cn.ts
%QTDIR%\lrelease .\ParticleEditorPlugin_zh_cn.ts
%QTDIR%\lrelease .\PerforcePlugin_zh_cn.ts
%QTDIR%\lrelease .\UiCanvasEditor_zh_cn.ts
%QTDIR%\lrelease .\Gems_zh_cn.ts