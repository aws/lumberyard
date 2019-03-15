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

from compile_settings_android_clang import load_android_clang_toolchains

from waflib.Configure import conf


################################################################
################################################################
@conf
def get_android_armv8_clang_target_abi(conf):
    return 'arm64-v8a'


################################################################
################################################################
@conf
def load_win_x64_android_armv8_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv8_clang configurations
    """
    load_android_clang_toolchains(conf, 'armv8', 'windows-x86_64')


@conf
def load_debug_win_x64_android_armv8_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv8_clang configurations for
    the 'debug' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv8_clang_common_settings()

    # load platform agnostic common settings
    conf.load_debug_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_debug_android_settings()
    conf.load_debug_android_armv8_settings()

    conf.load_debug_clang_settings()
    conf.load_debug_android_clang_settings()


@conf
def load_profile_win_x64_android_armv8_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv8_clang configurations for
    the 'profile' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv8_clang_common_settings()

    # load platform agnostic common settings
    conf.load_profile_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_profile_android_settings()
    conf.load_profile_android_armv8_settings()

    conf.load_profile_clang_settings()
    conf.load_profile_android_clang_settings()


@conf
def load_performance_win_x64_android_armv8_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv8_clang configurations for
    the 'performance' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv8_clang_common_settings()

    # load platform agnostic common settings
    conf.load_performance_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_performance_android_settings()
    conf.load_performance_android_armv8_settings()

    conf.load_performance_clang_settings()
    conf.load_performance_android_clang_settings()


@conf
def load_release_win_x64_android_armv8_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_android_armv8_clang configurations for
    the 'release' configuration
    """

    # load this first because it finds the compiler
    conf.load_win_x64_android_armv8_clang_common_settings()

    # load platform agnostic common settings
    conf.load_release_cryengine_settings()

    # load the common android and architecture settings before the toolchain specific settings
    # because ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    conf.load_release_android_settings()
    conf.load_release_android_armv8_settings()

    conf.load_release_clang_settings()
    conf.load_release_android_clang_settings()
