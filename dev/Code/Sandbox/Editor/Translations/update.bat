@IF "%QTDIR%" == "" (
    echo QTDIR environment variable is not set!
    EXIT /B 1
)

%QTDIR%\lupdate -noobsolete ..\AssetBrowser -ts .\assetbrowser_en-us.ts
%QTDIR%\lupdate -noobsolete ..\HyperGraph -ts .\flowgraph_en-us.ts