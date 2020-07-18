built_modules = list(name for name in
    "Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;WinExtras;Xml;XmlPatterns;Help;Multimedia;MultimediaWidgets;OpenGL;OpenGLFunctions;Positioning;Qml;Quick;QuickWidgets;Sensors;Svg;UiTools;WebChannel;WebEngineCore;WebEngine;WebEngineWidgets;WebSockets"
    .split(";"))

shiboken_library_soversion = str(5.12)
pyside_library_soversion = str(5.12)

version = "5.12.4"
version_info = (5, 12, 4, "", "")

__build_date__ = '2020-03-30T17:10:10+00:00'





