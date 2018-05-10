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
import os, sys
from waflib import Logs
from waflib.Configure import conf


################################################################
@conf
def get_android_armv7_clang_target_abi(conf):
    return 'armeabi-v7a'


################################################################
@conf
def load_win_x64_android_armv7_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv7_clang configurations
    """
    env = conf.env

    # load the toolchains
    ndk_root = env['ANDROID_NDK_HOME']

    gcc_toolchain_root = os.path.join(ndk_root, 'toolchains', 'arm-linux-androideabi-4.9', 'prebuilt', 'windows-x86_64')
    gcc_toolchain_path = os.path.join(gcc_toolchain_root, 'bin')

    clang_toolchain_path = os.path.join(ndk_root, 'toolchains', 'llvm', 'prebuilt', 'windows-x86_64', 'bin')

    ndk_toolchains = {
        'CC'    : 'clang',
        'CXX'   : 'clang++',
        'AR'    : 'arm-linux-androideabi-ar',
        'STRIP' : 'arm-linux-androideabi-strip',
    }

    if not conf.load_android_toolchains([clang_toolchain_path, gcc_toolchain_path], **ndk_toolchains):
        conf.fatal('[ERROR] android_armv7_clang setup failed')

    if not conf.load_android_tools():
        conf.fatal('[ERROR] android_armv7_clang setup failed')

    # common settings
    gcc_toolchain = '--gcc-toolchain={}'.format(gcc_toolchain_root)
    target_arch = '--target=thumbv7-none-linux-androideabi' # <arch><sub>-<vendor>-<sys>-<abi>

    common_flags = [
        gcc_toolchain,
        target_arch,
    ]

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]
    env['LINKFLAGS'] += common_flags[:]

    azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc141')
    if not os.path.exists(azcg_dir):
        azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc140')
    if not os.path.exists(azcg_dir):
        azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/vc120')
    if not os.path.exists(azcg_dir):
        conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that you have either VS2013, VS2015, or VS2017 binaries available')
    env['CODE_GENERATOR_PATH'] = [azcg_dir]


################################################################
@conf
def load_debug_win_x64_android_armv7_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv7_clang configurations for
    the 'debug' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv7_clang_common_settings()

    # load platform agnostic common settings
    conf.load_debug_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_debug_android_settings()
    conf.load_debug_android_armv7_settings()

    conf.load_debug_clang_settings()
    conf.load_debug_android_clang_settings()


################################################################
@conf
def load_profile_win_x64_android_armv7_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv7_clang configurations for
    the 'profile' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv7_clang_common_settings()

    # load platform agnostic common settings
    conf.load_profile_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_profile_android_settings()
    conf.load_profile_android_armv7_settings()

    conf.load_profile_clang_settings()
    conf.load_profile_android_clang_settings()


################################################################
@conf
def load_performance_win_x64_android_armv7_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv7_clang configurations for
    the 'performance' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv7_clang_common_settings()

    # load platform agnostic common settings
    conf.load_performance_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_performance_android_settings()
    conf.load_performance_android_armv7_settings()

    conf.load_performance_clang_settings()
    conf.load_performance_android_clang_settings()


################################################################
@conf
def load_release_win_x64_android_armv7_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv7_clang configurations for
    the 'release' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv7_clang_common_settings()

    # load platform agnostic common settings
    conf.load_release_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_release_android_settings()
    conf.load_release_android_armv7_settings()

    conf.load_release_clang_settings()
    conf.load_release_android_clang_settings()
