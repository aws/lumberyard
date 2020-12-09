from __future__ import print_function
import os
import sys

__all__ = list("Qt" + body for body in
    "Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;WinExtras;Xml;XmlPatterns;Help;OpenGL;OpenGLFunctions;Qml;Quick;QuickWidgets;Sensors;Svg;UiTools;WebChannel;WebEngineCore;WebEngine;WebEngineWidgets;WebSockets"
    .split(";"))
__version__ = "5.14.2.3"
__version_info__ = (5, 14, 2.3, "", "")


def _additional_dll_directories(package_dir):
    # Find shiboken2 relative to the package directory.
    root = os.path.dirname(package_dir)
    # Check for a flat .zip as deployed by cx_free(PYSIDE-1257)
    if root.endswith('.zip'):
        return []
    shiboken2 = os.path.join(root, 'shiboken2')
    if os.path.isdir(shiboken2): # Standard case, only shiboken2 is needed
        return [shiboken2]
    # The below code is for the build process when generate_pyi.py
    # is executed in the build directory. We need libpyside and Qt in addition.
    shiboken2 = os.path.join(os.path.dirname(root), 'shiboken2', 'libshiboken')
    if not os.path.isdir(shiboken2):
        raise ImportError(shiboken2 + ' does not exist')
    result = [shiboken2, os.path.join(root, 'libpyside')]
    for path in os.environ.get('PATH').split(';'):
        if path:
             if os.path.exists(os.path.join(path, 'qmake.exe')):
                 result.append(path)
                 break
    return result


def _setupQtDirectories():
    # On Windows we need to explicitly import the shiboken2 module so
    # that the libshiboken.dll dependency is loaded by the time a
    # Qt module is imported. Otherwise due to PATH not containing
    # the shiboken2 module path, the Qt module import would fail
    # due to the missing libshiboken dll.
    # In addition, as of Python 3.8, the shiboken package directory
    # must be added to the DLL search paths so that shiboken2.dll
    # is found.
    # We need to do the same on Linux and macOS, because we do not
    # embed rpaths into the PySide2 libraries that would point to
    # the libshiboken library location. Importing the module
    # loads the libraries into the process memory beforehand, and
    # thus takes care of it for us.

    pyside_package_dir = os.path.abspath(os.path.dirname(__file__))

    if sys.platform == 'win32' and sys.version_info[0] == 3 and sys.version_info[1] >= 8:
        for dir in _additional_dll_directories(pyside_package_dir):
            os.add_dll_directory(dir)

    try:
        import shiboken2
    except Exception:
        paths = ', '.join(sys.path)
        print('PySide2/__init__.py: Unable to import shiboken2 from {}'.format(paths),
              file=sys.stderr)
        raise

    #   Trigger signature initialization.
    type.__signature__

    if sys.platform == 'win32':
        # PATH has to contain the package directory, otherwise plugins
        # won't be able to find their required Qt libraries (e.g. the
        # svg image plugin won't find Qt5Svg.dll).
        os.environ['PATH'] = pyside_package_dir + os.pathsep + os.environ['PATH']

        # On Windows, add the PySide2\openssl folder (created by setup.py's
        # --openssl option) to the PATH so that the SSL DLLs can be found
        # when Qt tries to dynamically load them. Tell Qt to load them and
        # then reset the PATH.
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
