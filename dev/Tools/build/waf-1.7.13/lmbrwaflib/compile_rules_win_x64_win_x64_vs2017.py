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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#
import os
from waflib.Configure import conf, Logs

PLATFORM = 'win_x64_vs2017'


@conf
def load_win_x64_win_x64_vs2017_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64 configurations
    """
    v = conf.env

    global PLATFORM

    # Add defines to indicate a win64 build
    v['DEFINES'] += ['_WIN32', '_WIN64', 'NOMINMAX', '_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING']

    restricted_tool_list_macro_header = 'AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS='
    restricted_tool_list_macro = restricted_tool_list_macro_header
    if len(restricted_tool_list_macro) > len(restricted_tool_list_macro_header):
        v['DEFINES'] += [ restricted_tool_list_macro ]

    # Make sure this is a supported platform
    if PLATFORM not in conf.get_supported_platforms():
        return

    # Attempt to detect the C++ compiler for VS 2017 ( msvs version 15.0 )
    windows_kit = conf.options.win_vs2017_winkit
    vcvarsall_args = windows_kit + ' ' + conf.options.win_vs2017_vcvarsall_args
    try:
        conf.auto_detect_msvc_compiler('msvc 15', 'x64', vcvarsall_args)
    except:
        Logs.warn('Unable to find Visual Studio 2017 ({}) C++ compiler and/or Windows Kit {}, removing build target'.format(conf.options.win_vs2017_vswhere_args, vcvarsall_args))
        conf.mark_supported_platform_for_removal(PLATFORM)
        return

    # Detect the QT binaries, if the current capabilities selected requires it.
    _, enabled, _, _ = conf.tp.get_third_party_path(PLATFORM, 'qt')
    if enabled:
        conf.find_qt5_binaries(PLATFORM)

    # Introduce the linker to generate 64 bit code
    v['LINKFLAGS'] += ['/MACHINE:X64']
    v['ARFLAGS'] += ['/MACHINE:X64']

    VS2017_FLAGS = [
        '/FS'  # Fix for issue writing to pdb files
    ]

    v['CFLAGS'] += VS2017_FLAGS
    v['CXXFLAGS'] += VS2017_FLAGS

    if conf.options.use_uber_files:
        v['CFLAGS'] += ['/bigobj']
        v['CXXFLAGS'] += ['/bigobj']

    azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc141')
    if not os.path.exists(azcg_dir):
        azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc140')
        if not os.path.exists(azcg_dir):
            conf.fatal(
                'Unable to locate the AzCodeGenerator subfolder.  Make sure that you have VS2017 AzCodeGenerator binaries available')
    v['CODE_GENERATOR_PATH'] = [azcg_dir]

    crcfix_dir = conf.Path('Tools/crcfix/bin/vc141')
    if not os.path.exists(crcfix_dir):
        crcfix_dir = conf.Path('Tools/crcfix/bin/vc140')
        if not os.path.exists(crcfix_dir):
            Logs.warn('Unable to locate the crcfix subfolder.  Make sure that you have VS2017 crcfix binaries available')
    v['CRCFIX_PATH'] = [crcfix_dir]

    conf.find_dx12(windows_kit)

@conf
def load_debug_win_x64_win_x64_vs2017_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2017_common_settings()

    # Load additional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_msvc_settings()
    conf.load_debug_windows_settings()

    conf.register_win_x64_external_ly_identity('vs2017', 'Debug')
    conf.register_win_x64_external_ly_metrics('vs2017', 'Debug')

@conf
def load_profile_win_x64_win_x64_vs2017_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'profile' configuration
    """
    conf.load_win_x64_win_x64_vs2017_common_settings()

    # Load additional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_msvc_settings()
    conf.load_profile_windows_settings()

    conf.register_win_x64_external_ly_identity('vs2017', 'Release')
    conf.register_win_x64_external_ly_metrics('vs2017', 'Release')

@conf
def load_performance_win_x64_win_x64_vs2017_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'performance' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2017_common_settings()

    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_msvc_settings()
    conf.load_performance_windows_settings()


@conf
def load_release_win_x64_win_x64_vs2017_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'release' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2017_common_settings()

    # Load additional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_msvc_settings()
    conf.load_release_windows_settings()

