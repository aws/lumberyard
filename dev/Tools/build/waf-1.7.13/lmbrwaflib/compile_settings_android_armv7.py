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

from cry_utils import append_to_unique_list

from waflib.Configure import conf


################################################################
################################################################
@conf
def load_android_armv7_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv7 configurations
    """

    env = conf.env
    env['ANDROID_ARCH'] = 'armeabi-v7a'

    ndk_root = env['ANDROID_NDK_HOME']
    ndk_rev = env['ANDROID_NDK_REV_MAJOR']

    is_ndk_19_plus = (ndk_rev >= 19)

    defines = [
        'LINUX32',
        '__ARM_NEON__',
    ]
    append_to_unique_list(env['DEFINES'], defines)

    common_flags = [
        '-mfloat-abi=softfp',       # float ABI: hardware code gen, soft calling convention
        '-mfpu=neon',               # enable neon, implies -mfpu=VFPv3-D32
    ]

    link_flags = [
        '-Wl,--fix-cortex-a8',  # required to fix a bug in some Cortex-A8 implementations for neon support
        '-Wl,--icf=safe',       # removes duplicate code
    ]

    if is_ndk_19_plus:
        common_flags += [
            '-mthumb',
            '-fpic'
        ]

    else:
        platform_root_compile = os.path.join(ndk_root, 'sysroot')
        platform_root_link = os.path.join(ndk_root, 'platforms', env['ANDROID_NDK_PLATFORM'], 'arch-arm')

        env['INCLUDES'] += [
            os.path.join(platform_root_compile, 'usr', 'include'),
        ]

        common_flags = [
            '--sysroot={}'.format(platform_root_compile),
            '-isystem', os.path.join(platform_root_compile, 'usr', 'include', 'arm-linux-androideabi'),
        ] + common_flags

        env['LIBPATH'] += [
            os.path.join(platform_root_link, 'usr', 'lib')
        ]

        link_flags = [ '--sysroot={}'.format(platform_root_link) ] + link_flags


    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]

    env['LINKFLAGS'] += link_flags[:]


@conf
def load_debug_android_armv7_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv7 configurations
    for the "debug" configuration
    """
    conf.load_android_armv7_common_settings()

    # required 3rd party libs that need to be included in the apk
    # Note: gdbserver is only required for debuggable builds
    conf.env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(conf.env['ANDROID_NDK_HOME'], 'prebuilt', 'android-arm', 'gdbserver', 'gdbserver'))
    ]


@conf
def load_profile_android_armv7_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv7 configurations
    for the "profile" configuration
    """
    conf.load_android_armv7_common_settings()

    # required 3rd party libs that need to be included in the apk
    # Note: gdbserver is only required for debuggable builds
    conf.env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(conf.env['ANDROID_NDK_HOME'], 'prebuilt', 'android-arm', 'gdbserver', 'gdbserver'))
    ]


@conf
def load_performance_android_armv7_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv7 configurations
    for the "performance" configuration
    """
    conf.load_android_armv7_common_settings()


@conf
def load_release_android_armv7_settings(conf):
    """
    Setup all compiler and linker settings shared over all android armv7 configurations
    for the "release" configuration
    """
    conf.load_android_armv7_common_settings()

