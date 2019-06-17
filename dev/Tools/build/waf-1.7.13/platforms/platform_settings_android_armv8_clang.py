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
from waflib import Errors, Logs, Utils
from waflib.Configure import conf
from lumberyard import deprecated


PLATFORM = 'android_armv8_clang'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_android_armv8_clang_common_settings(ctx):
    """
    Setup all compiler and linker settings shared over all android_armv7_clang configurations
    """
    env = ctx.env
    
    # ANDROID_ARCH needs to be set for the android_<toolchain> settings to init correctly
    env['ANDROID_ARCH'] = 'arm64-v8a'
    
    host_platform = Utils.unversioned_sys_platform()
    host_platform_to_toolchain_platform_subdir = {
        'darwin': 'darwin-x86_64',
        'win32': 'windows-x86_64',
    }
    ctx.load_android_clang_toolchains('armv8', host_platform_to_toolchain_platform_subdir[host_platform])

    ndk_root = env['ANDROID_NDK_HOME']
    is_ndk_19_plus = env['ANDROID_IS_NDK_19_PLUS']
    
    if not is_ndk_19_plus:
        platform_root_compile = os.path.join(ndk_root, 'sysroot')
        platform_root_link = os.path.join(ndk_root, 'platforms', env['ANDROID_NDK_PLATFORM'], 'arch-arm64')
        
        env['INCLUDES'] += [
            os.path.join(platform_root_compile, 'usr', 'include'),
        ]
        
        common_flags = [
            '--sysroot={}'.format(platform_root_compile),
            '-isystem', os.path.join(platform_root_compile, 'usr', 'include', 'aarch64-linux-android'),
        ]
        
        env['CFLAGS'] += common_flags[:]
        env['CXXFLAGS'] += common_flags[:]
        
        env['LIBPATH'] += [
            os.path.join(platform_root_link, 'usr', 'lib')
        ]
        
        env['LINKFLAGS'] += [
            '--sysroot={}'.format(platform_root_link),
        ]

    # Set the path to the android-arm gdbserver
    env['EXT_LIB_ANDROID_ARM64_GDBSERVER_PATH'] = ctx.add_to_android_cache(os.path.join(ctx.env['ANDROID_NDK_HOME'], 'prebuilt', 'android-arm64', 'gdbserver', 'gdbserver'))

    ctx.load_cryengine_common_settings()

    ctx.load_android_common_settings()

    ctx.load_clang_common_settings()

    ctx.load_android_clang_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_android_armv8_clang_configuration_settings(ctx, platform_configuration):
    # No special configuration-specific setup needed
    pass


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_android_armv8_clang_available(ctx):
    # This platform is only support if the Armv8 API is valid
    if not ctx.is_android_armv8_api_valid():
        ctx.warn_once('[WARN] Unable to configure Android ARMv8 with an API that is lower than the min spec: API 21')
        return False
    else:
        return True


@conf
def get_android_armv8_clang_target_abi(ctx):
    return 'arm64-v8a'
