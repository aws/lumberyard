#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

from waflib.Configure import conf

LMBR_SETUP_QT_FILTERS = {
    "win": {
        "Modules": [
            "Qt5Core",
            "Qt5Gui",
            "Qt5Network",
            "Qt5Qml",
            "Qt5Quick",
            "Qt5Svg",
            "Qt5Widgets",
            "Qt5Concurrent",
            "Qt5WinExtras",
            "Qt5Xml"
        ],

        "qtlibs": {
            "profile": {
                "plugins": [
                    "bearer/qgenericbearer.dll",
                    "bearer/qnativewifibearer.dll",
                    "imageformats/qdds.dll",
                    "imageformats/qgif.dll",
                    "imageformats/qicns.dll",
                    "imageformats/qico.dll",
                    "imageformats/qjpeg.dll",
                    "imageformats/qsvg.dll",
                    "imageformats/qtiff.dll",
                    "imageformats/qwbmp.dll",
                    "imageformats/qwebp.dll",
                    "platforms/qwindows.dll",
                ],
                "qml": [
                    "Qt/labs/settings/qmlsettingsplugin.dll",
                    "Qt/labs/settings/plugins.qmltypes",
                    "Qt/labs/settings/qmldir",
                    "QtGraphicalEffects/*",
                    "QtQuick/Window.2/windowplugin.dll",
                    "QtQuick/Window.2/plugins.qmltypes",
                    "QtQuick/Window.2/qmldir",
                    "QtQuick.2/qtquick2plugin.dll",
                    "QtQuick.2/plugins.qmltypes",
                    "QtQuick.2/qmldir",
                ]
            },
            "debug": {
                "plugins": [
                    "bearer/qgenericbearerd.dll",
                    "bearer/qnativewifibearerd.dll",
                    "imageformats/qddsd.dll",
                    "imageformats/qgifd.dll",
                    "imageformats/qicnsd.dll",
                    "imageformats/qicod.dll",
                    "imageformats/qjpegd.dll",
                    "imageformats/qsvgd.dll",
                    "imageformats/qtiffd.dll",
                    "imageformats/qwbmpd.dll",
                    "imageformats/qwebpd.dll",
                    "platforms/qwindowsd.dll",
                ],
                "qml": [
                    "Qt/labs/settings/qmlsettingsplugind.dll",
                    "Qt/labs/settings/plugins.qmltypes",
                    "Qt/labs/settings/qmldir",
                    "QtGraphicalEffects/*",
                    "QtQuick/Window.2/windowplugind.dll",
                    "QtQuick/Window.2/plugins.qmltypes",
                    "QtQuick/Window.2/qmldir",
                    "QtQuick.2/qtquick2plugind.dll",
                    "QtQuick.2/plugins.qmltypes",
                    "QtQuick.2/qmldir",
                ]
            }
        }
    },

    "darwin": {
        "qtlibs": {
            "profile": {
                "lib": [
                    "QtConcurrent.framework",
                    "QtCore.framework",
                    "QtGui.framework",
                    "QtNetwork.framework",
                    "QtPrintSupport.framework",
                    "QtQml.framework",
                    "QtQuick.framework",
                    "QtSvg.framework",
                    "QtWidgets.framework",
                    "QtXml.framework",
					"QtMacExtras.framework"
                ],
                "plugins": [
                    "imageformats/libqdds.dylib",
                    "imageformats/libqgif.dylib",
                    "imageformats/libqicns.dylib",
                    "imageformats/libqico.dylib",
                    "imageformats/libqjpeg.dylib",
                    "imageformats/libqsvg.dylib",
                    "imageformats/libqtga.dylib",
                    "imageformats/libqtiff.dylib",
                    "imageformats/libqwbmp.dylib",
                    "imageformats/libqwebp.dylib",
                    "platforms/libqcocoa.dylib"
                ],
                "qml": [
                    "Qt/labs/settings/libqmlsettingsplugin.dylib",
                    "Qt/labs/settings/plugins.qmltypes",
                    "Qt/labs/settings/qmldir",
                    "QtGraphicalEffects/*",
                    "QtQuick/Window.2/libwindowplugin.dylib",
                    "QtQuick/Window.2/plugins.qmltypes",
                    "QtQuick/Window.2/qmldir",
                    "QtQuick.2/libqtquick2plugin.dylib",
                    "QtQuick.2/plugins.qmltypes",
                    "QtQuick.2/qmldir",
                ]
            },
            "debug": {
                "lib": [
                    "QtConcurrent.framework",
                    "QtCore.framework",
                    "QtGui.framework",
                    "QtNetwork.framework",
                    "QtPrintSupport.framework",
                    "QtQml.framework",
                    "QtQuick.framework",
                    "QtSvg.framework",
                    "QtWidgets.framework",
                    "QtXml.framework",
					"QtMacExtras.framework"
                ],
                "plugins": [
                    "imageformats/libqdds_debug.dylib",
                    "imageformats/libqgif_debug.dylib",
                    "imageformats/libqicns_debug.dylib",
                    "imageformats/libqico_debug.dylib",
                    "imageformats/libqjpeg_debug.dylib",
                    "imageformats/libqsvg_debug.dylib",
                    "imageformats/libqtga_debug.dylib",
                    "imageformats/libqtiff_debug.dylib",
                    "imageformats/libqwbmp_debug.dylib",
                    "imageformats/libqwebp_debug.dylib",
                    "platforms/libqcocoa_debug.dylib"
                ],
                "qml": [
                    "Qt/labs/settings/libqmlsettingsplugin_debug.dylib",
                    "Qt/labs/settings/plugins.qmltypes",
                    "Qt/labs/settings/qmldir",
                    "QtGraphicalEffects/*",
                    "QtQuick/Window.2/libwindowplugin_debug.dylib",
                    "QtQuick/Window.2/plugins.qmltypes",
                    "QtQuick/Window.2/qmldir",
                    "QtQuick.2/libqtquick2plugin_debug.dylib",
                    "QtQuick.2/plugins.qmltypes",
                    "QtQuick.2/qmldir",
                ]
            }
        }
    },

    "linux": {
        "qtlibs": {
            "profile": {
                "lib": [
                    "libicudata.so.56.1",
                    "libicui18n.so.56.1",
                    "libicuuc.so.56.1",
                    "libQt5Concurrent.so.5.6.2",
                    "libQt5Core.so.5.6.2",
                    "libQt5Network.so.5.6.2"
                ]
            },
            "debug": {
                "lib": [
                    "libicudata.so.56.1",
                    "libicui18n.so.56.1",
                    "libicuuc.so.56.1",
                    "libQt5Concurrent.so.5.6.2",
                    "libQt5Core.so.5.6.2",
                    "libQt5Network.so.5.6.2"
                ]
            }
        }
    }
}

@conf
def get_lmbr_setup_tools_output_folder(ctx, platform_override=None, configuration_override=None):
    curr_platform, curr_configuration = ctx.get_platform_and_configuration()

    if platform_override is not None and isinstance(platform_override, basestring):
        curr_platform = platform_override

    if configuration_override is not None and isinstance(configuration_override, basestring):
        curr_configuration = configuration_override

    output_folder_platform      = ""
    output_folder_compiler      = ""
    if curr_platform.startswith("win_"):
        output_folder_platform  = "Win"
        if "vs2013" in curr_platform:
            output_folder_compiler = "vc120"
        elif "vs2015" in curr_platform:
            output_folder_compiler = "vc140"
        elif "vs2017" in curr_platform:
            output_folder_compiler = "vc141"

    elif curr_platform.startswith("darwin_"):
        output_folder_platform  = "Mac"
        output_folder_compiler  = "clang"
    elif curr_platform.startswith("linux_"):
        output_folder_platform  = "Linux"
        output_folder_compiler  = "clang"

    output_folder_configuration = ""
    if curr_configuration.startswith("debug"):
        output_folder_configuration = "Debug"
    elif curr_configuration.startswith("profile"):
        output_folder_configuration = "Profile"

    output_folder_test = ""
    if curr_configuration.endswith("_test"):
        output_folder_test = "Test"

    output_folder = "Tools/LmbrSetup/" + output_folder_platform

    # do not manipulate string if we do not have all the data
    if output_folder_platform != "" and output_folder_compiler != "" and output_folder_configuration != "":
        if output_folder_test != "":
            output_folder += "." + output_folder_compiler
            output_folder += "." + output_folder_configuration
            output_folder += "." + output_folder_test
        elif output_folder_configuration != "Profile":
            output_folder += "." + output_folder_compiler
            output_folder += "." + output_folder_configuration
        elif output_folder_compiler != "vc140" and output_folder_compiler != "clang":
            output_folder += "." + output_folder_compiler

    return output_folder
