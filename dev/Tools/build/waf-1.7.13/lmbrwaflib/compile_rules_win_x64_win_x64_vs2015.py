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

PLATFORM = 'win_x64_vs2015'

DX12_INCLUDE_DIR = None
DX12_LIB_DIR = None
def find_dx12(conf):
    global DX12_INCLUDE_DIR
    global DX12_LIB_DIR
    if (DX12_INCLUDE_DIR is None) or (DX12_LIB_DIR is None):
        DX12_INCLUDE_DIR = []   # set to empty, will prevent this part of the function from running more than once even if nothing is found
        DX12_LIB_DIR = []
        v = conf.env
        includes = v['INCLUDES']
        libs = v['LIBPATH']

        for path in includes:
            for root, dirs, files in os.walk(path):
                if 'd3d12.h' in files:
                    DX12_INCLUDE_DIR = root
                    break
            if DX12_INCLUDE_DIR:
                break

        # this is expensive, so only do it if the include was also found
        if DX12_INCLUDE_DIR:
            for path in libs:
                for root, dirs, files in os.walk(path):
                    if 'd3d12.lib' in files:
                        Logs.warn('[DX12] libraries found, can compile for DX12')
                        DX12_LIB_DIR = root
                        break
                if DX12_LIB_DIR:
                    break
    conf.env['DX12_INCLUDES'] = DX12_INCLUDE_DIR
    conf.env['DX12_LIBPATH'] = DX12_LIB_DIR

@conf
def has_dx12(conf):
    if conf.env['DX12_INCLUDES'] and conf.env['DX12_LIBPATH']:
        return True
    return False

@conf
def load_win_x64_win_x64_vs2015_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64 configurations
    """
    v = conf.env

    global PLATFORM

    # Add defines to indicate a win64 build
    v['DEFINES'] += ['_WIN32', '_WIN64', 'NOMINMAX']

    # Make sure this is a supported platform
    if PLATFORM not in conf.get_supported_platforms():
        return

    # Attempt to detect the C++ compiler for VS 2015 ( msvs version 14.0 )
    try:
        conf.auto_detect_msvc_compiler('msvc 14.0', 'x64')
    except:
        Logs.warn('Unable to detect find the C++ compiler for Visual Studio 2015, removing build target')
        conf.mark_supported_platform_for_removal(PLATFORM)
        return

    # Detect the QT binaries
    conf.find_qt5_binaries(PLATFORM)

    # Introduce the linker to generate 64 bit code
    v['LINKFLAGS'] += ['/MACHINE:X64']
    v['ARFLAGS'] += ['/MACHINE:X64']

    VS2015_FLAGS = [
        '/FS',           # Fix for issue writing to pdb files
        '/Wv:18'         # Stick with 2013 warnings for the time being...
    ]

    v['CFLAGS'] += VS2015_FLAGS
    v['CXXFLAGS'] += VS2015_FLAGS

    if conf.options.use_uber_files:
        v['CFLAGS'] += ['/bigobj']
        v['CXXFLAGS'] += ['/bigobj']

    azcg_dir = conf.srcnode.make_node('Tools/AzCodeGenerator/bin/vc140').abspath()
    if not os.path.exists(azcg_dir):
        conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that you have VS2015 AzCodeGenerator binaries available')
    v['CODE_GENERATOR_PATH'] = [azcg_dir]

    crcfix_dir = conf.srcnode.make_node('Tools/crcfix/bin/vc140').abspath()
    if not os.path.exists(crcfix_dir):
        Logs.warn('Unable to locate the crcfix subfolder.  Make sure that you have VS2015 crcfix binaries available')
    v['CRCFIX_PATH'] = [crcfix_dir]

    find_dx12(conf)

@conf
def load_debug_win_x64_win_x64_vs2015_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2015_common_settings()

    # Load additional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_msvc_settings()
    conf.load_debug_windows_settings()


@conf
def load_profile_win_x64_win_x64_vs2015_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'profile' configuration
    """
    conf.load_win_x64_win_x64_vs2015_common_settings()

    # Load additional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_msvc_settings()
    conf.load_profile_windows_settings()


@conf
def load_performance_win_x64_win_x64_vs2015_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'performance' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2015_common_settings()

    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_msvc_settings()
    conf.load_performance_windows_settings()


@conf
def load_release_win_x64_win_x64_vs2015_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'release' configuration
    """
    v = conf.env
    conf.load_win_x64_win_x64_vs2015_common_settings()

    # Load additional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_msvc_settings()
    conf.load_release_windows_settings()

