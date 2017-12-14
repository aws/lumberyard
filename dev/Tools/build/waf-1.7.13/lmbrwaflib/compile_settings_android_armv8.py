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
import os
from waflib import Logs
from waflib.Configure import conf
from cry_utils import append_to_unique_list


################################################################
@conf
def load_android_armv8_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv8 configurations
    """

    # remove the armv8 android build target if it doens't meet the min API requirment.
    # letting the platform finish configuring is harmless.
    if (not conf.is_android_armv8_api_valid()) and ('android_armv8_clang' in conf.get_supported_platforms()):
        Logs.warn('[WARN] Attempting to configure Android ARMv8 with an API that is lower than the min spec: API 21.  Disabling the Android ARMv8 build target.')
        conf.remove_platform_from_available_platforms('android_armv8_clang')

    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']

    # common settings for all android armv8 builds
    platform_root_compile = os.path.join(ndk_root, 'platforms', env['ANDROID_NDK_PLATFORM'], 'arch-arm64')
    platform_root_link = platform_root_compile

    if conf.is_using_android_unified_headers():
        platform_root_compile = os.path.join(ndk_root, 'sysroot')

    env['INCLUDES'] += [
        os.path.join(platform_root_compile, 'usr', 'include'),
    ]

    system_root = '--sysroot={}'.format(platform_root_compile)

    common_flags = [
        system_root,
    ]

    if conf.is_using_android_unified_headers():
        system_arch = os.path.join(platform_root_compile, 'usr', 'include', 'aarch64-linux-android')
        common_flags += [
            '-isystem', system_arch,
        ]

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]

    defines = [
        'LINUX64',
        '__ARM_NEON__',
    ]
    append_to_unique_list(env['DEFINES'], defines)

    env['LIBPATH'] += [
        os.path.join(platform_root_link, 'usr', 'lib')
    ]

    system_root = '--sysroot={}'.format(platform_root_link)

    env['LINKFLAGS'] += [
        system_root,
    ]

    env['ANDROID_ARCH'] = 'arm64-v8a'


################################################################
@conf
def load_debug_android_armv8_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv8 configurations
    for the "debug" configuration
    """

    conf.load_android_armv8_common_settings()

    # required 3rd party libs that need to be included in the apk
    # Note: gdbserver is only required for debuggable builds
    conf.env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(conf.env['ANDROID_NDK_HOME'], 'prebuilt', 'android-arm', 'gdbserver', 'gdbserver'))
    ]


################################################################
@conf
def load_profile_android_armv8_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv8 configurations
    for the "profile" configuration
    """

    conf.load_android_armv8_common_settings()

    # required 3rd party libs that need to be included in the apk
    # Note: gdbserver is only required for debuggable builds
    conf.env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(conf.env['ANDROID_NDK_HOME'], 'prebuilt', 'android-arm', 'gdbserver', 'gdbserver'))
    ]


################################################################
@conf
def load_performance_android_armv8_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv8 configurations
    for the "performance" configuration
    """

    conf.load_android_armv8_common_settings()


################################################################
@conf
def load_release_android_armv8_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv8 configurations
    for the "release" configuration
    """

    conf.load_android_armv8_common_settings()

