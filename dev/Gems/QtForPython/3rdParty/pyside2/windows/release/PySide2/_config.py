built_modules = list(name for name in
    "Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;WinExtras;Xml;XmlPatterns;Help;OpenGL;OpenGLFunctions;Qml;Quick;QuickWidgets;Sensors;Svg;UiTools;WebChannel;WebEngineCore;WebEngine;WebEngineWidgets;WebSockets"
    .split(";"))

shiboken_library_soversion = str(5.12)
pyside_library_soversion = str(5.12)

version = "5.12.5"
version_info = (5, 12, 5, "a", "1")

__build_date__ = '2020-07-21T16:48:48+00:00'





