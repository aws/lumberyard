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
from waflib.Configure import conf, Logs


@conf
def load_win_x64_win_x64_vs2012_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_vs2012 configurations
    """
    try:
        conf.auto_detect_msvc_compiler('msvc 11.0', 'x64')
    except:
        if 'win_x64_vs2012' in conf.get_supported_platforms():
            Logs.warn('Unable to find Visual Studio 2012, removing build target')
            conf.mark_supported_platform_for_removal('win_x64_vs2012')
        return

    v = conf.env

    # Add defines to indicate a win64 build
    v['DEFINES'] += [ '_WIN32', '_WIN64', 'NOMINMAX']

    # Introduce the linker to generate 64 bit code
    v['LINKFLAGS'] += [ '/MACHINE:X64' ]
    v['ARFLAGS'] += [ '/MACHINE:X64' ]


@conf
def load_debug_win_x64_win_x64_vs2012_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_vs2012 configurations for
    the 'debug' configuration
    """
    env = conf.env
    conf.load_win_x64_win_x64_vs2012_common_settings()

    # Load addional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_msvc_settings()
    conf.load_debug_windows_settings()


@conf
def load_profile_win_x64_win_x64_vs2012_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_vs2012 configurations for
    the 'profile' configuration
    """
    env = conf.env
    conf.load_win_x64_win_x64_vs2012_common_settings()

    # Load addional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_msvc_settings()
    conf.load_profile_windows_settings()


@conf
def load_performance_win_x64_win_x64_vs2012_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_vs2012 configurations for
    the 'performance' configuration
    """
    env = conf.env
    conf.load_win_x64_win_x64_vs2012_common_settings()

    # Load addional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_msvc_settings()
    conf.load_performance_windows_settings()


@conf
def load_release_win_x64_win_x64_vs2012_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_vs2012 configurations for
    the 'release' configuration
    """
    conf.load_win_x64_win_x64_vs2012_common_settings()

    # Load addional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_msvc_settings()
    conf.load_release_windows_settings()

