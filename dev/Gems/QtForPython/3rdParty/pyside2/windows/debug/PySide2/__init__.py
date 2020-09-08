__all__ = list("Qt" + body for body in
    "Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;WinExtras;Xml;XmlPatterns;Help;Multimedia;MultimediaWidgets;OpenGL;OpenGLFunctions;Positioning;Qml;Quick;QuickWidgets;Sensors;Svg;UiTools;WebChannel;WebEngineCore;WebEngine;WebEngineWidgets;WebSockets"
    .split(";"))
__version__ = "5.12.4"
__version_info__ = (5, 12, 4, "", "")

def _setupQtDirectories():
    import sys
    import os

    # On Windows we need to explicitly import the shiboken2 module so
    # that the libshiboken.dll dependency is loaded by the time a
    # Qt module is imported. Otherwise due to PATH not containing
    # the shiboken2 module path, the Qt module import would fail
    # due to the missing libshiboken dll.
    # We need to do the same on Linux and macOS, because we do not
    # embed rpaths into the PySide2 libraries that would point to
    # the libshiboken library location. Importing the module
    # loads the libraries into the process memory beforehand, and
    # thus takes care of it for us.
    import shiboken2
    #   Trigger signature initialization.
    type.__signature__

    pyside_package_dir =  os.path.abspath(os.path.dirname(__file__))

    if sys.platform == 'win32':
        # PATH has to contain the package directory, otherwise plugins
        # won't be able to find their required Qt libraries (e.g. the
        # svg image plugin won't find Qt5Svg.dll).
        os.environ['PATH'] = pyside_package_dir + os.pathsep + os.environ['PATH']

        # On Windows add the PySide2\openssl folder (if it exists) to
        # the PATH so that the SSL DLLs can be found when Qt tries to
        # dynamically load them. Tell Qt to load them and then reset
        # the PATH.
        openssl_dir = os.path.join(pyside_package_dir, 'openssl')
        if os.path.exists(openssl_dir):
            path = os.environ['PATH']
            try:
                os.environ['PATH'] = openssl_dir + os.pathsep + path
                try:
                    from . import QtNetwork
                except ImportError:
                    pass
                else:
                    QtNetwork.QSslSocket.supportsSsl()
            finally:
                os.environ['PATH'] = path

_setupQtDirectories()
