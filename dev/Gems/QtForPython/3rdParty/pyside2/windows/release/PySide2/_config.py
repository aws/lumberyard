built_modules = list(name for name in
    "Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;WinExtras;Xml;XmlPatterns;Help;OpenGL;OpenGLFunctions;Qml;Quick;QuickWidgets;Sensors;Svg;UiTools;WebChannel;WebEngineCore;WebEngine;WebEngineWidgets;WebSockets"
    .split(";"))

shiboken_library_soversion = str(5.14)
pyside_library_soversion = str(5.14)

version = "5.14.2.3"
version_info = (5, 14, 2.3, "", "")

__build_date__ = '2020-09-11T01:15:13+00:00'
__build_commit_date__ = '2020-09-10T23:25:41+00:00'
__build_commit_hash__ = '251473bb714b5ebb2c44797a5312264fb0470b12'
__build_commit_hash_described__ = 'v5.14.2.3-1-g251473bb'


