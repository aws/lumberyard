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

PLATFORM = 'win_x64_vs2013'

@conf
def load_win_x64_win_x64_vs2013_common_settings(conf):
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

    # Attempt to detect the C++ compiler for VS 2013 ( msvs version 12.0 )
    try:
        conf.auto_detect_msvc_compiler('msvc 12.0', 'x64', '')
    except:
        Logs.warn('Unable to find Visual Studio 2013 C++ compiler, removing build target')
        conf.mark_supported_platform_for_removal(PLATFORM)
        return

    # Detect the QT binaries
    conf.find_qt5_binaries(PLATFORM)

    # Introduce the linker to generate 64 bit code
    v['LINKFLAGS'] += ['/MACHINE:X64']
    v['ARFLAGS'] += ['/MACHINE:X64']

    VS2013_FLAGS = [ 
        '/FS',           # Fix for issue writing to pdb files
        '/Zo'            # Enhanced optimized debugging (increases pdb size but has no effect on code size and improves debugging) - this is enabled by default for vs2015.
    ]

    v['CFLAGS'] += VS2013_FLAGS 
    v['CXXFLAGS'] += VS2013_FLAGS

    if conf.options.use_uber_files:
        v['CFLAGS'] += ['/bigobj']
        v['CXXFLAGS'] += ['/bigobj']

    azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc120')
    if not os.path.exists(azcg_dir):
        conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that you have VS2013 AzCodeGenerator binaries available')
    v['CODE_GENERATOR_PATH'] = [azcg_dir]

    crcfix_dir = conf.Path('Tools/crcfix/bin/vc120')
    if not os.path.exists(crcfix_dir):
        Logs.warn('Unable to locate the crcfix subfolder.  Make sure that you have VS2013 crcfix binaries available')
    v['CRCFIX_PATH'] = [crcfix_dir]


@conf
def load_debug_win_x64_win_x64_vs2013_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'debug' configuration
    """
    conf.load_debug_msvc_settings()
    conf.load_debug_windows_settings()  
    conf.load_win_x64_win_x64_vs2013_common_settings()
    
    # Load additional shared settings
    conf.load_debug_cryengine_settings()

    conf.register_win_x64_external_ly_identity('vs2013', 'Debug')
    conf.register_win_x64_external_ly_metrics('vs2013', 'Debug')

@conf
def load_profile_win_x64_win_x64_vs2013_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'profile' configuration
    """
    conf.load_profile_msvc_settings()
    conf.load_profile_windows_settings()
    conf.load_win_x64_win_x64_vs2013_common_settings()
    
    # Load additional shared settings
    conf.load_profile_cryengine_settings()

    conf.register_win_x64_external_ly_identity('vs2015', 'Release')
    conf.register_win_x64_external_ly_metrics('vs2015', 'Release')


@conf
def load_performance_win_x64_win_x64_vs2013_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'performance' configuration
    """
    conf.load_performance_msvc_settings()
    conf.load_performance_windows_settings()
    conf.load_win_x64_win_x64_vs2013_common_settings()
    
    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    

@conf
def load_release_win_x64_win_x64_vs2013_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_v140 configurations for
    the 'release' configuration
    """
    conf.load_release_msvc_settings()
    conf.load_release_windows_settings()
    conf.load_win_x64_win_x64_vs2013_common_settings()
    
    # Load additional shared settings
    conf.load_release_cryengine_settings()

