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
from waflib.Configure import conf
from lumberyard import deprecated


@conf
def load_android_clang_toolchains(ctx, architecture, host_platform):
    env = ctx.env
    ndk_root = env['ANDROID_NDK_HOME']
    ndk_rev = env['ANDROID_NDK_REV_MAJOR']
    is_ndk_19_plus = env['ANDROID_IS_NDK_19_PLUS']

    if architecture == 'armv7':
        target_abi = ctx.get_android_armv7_clang_target_abi()
        target_arch = '{}-none-linux-androideabi'.format('armv7a' if is_ndk_19_plus else 'thumbv7') # <arch><sub>-<vendor>-<sys>-<abi>
        target_triple = 'arm-linux-androideabi'

    elif architecture == 'armv8':
        target_abi = ctx.get_android_armv8_clang_target_abi()
        target_arch = 'aarch64-none-linux-android' # <arch><sub>-<vendor>-<sys>-<abi>
        target_triple = 'aarch64-linux-android'

    else:
        ctx.fatal('[ERROR] Unsupported architecture - {} - passed to load_android_clang_toolchains'.format(architecture))

    if is_ndk_19_plus:
        target_arch = '{}{}'.format(target_arch, env['ANDROID_NDK_PLATFORM_NUMBER'])

    ndk_toolchains = {
        'CC'    : 'clang',
        'CXX'   : 'clang++',
        'AR'    : 'llvm-ar',
        'STRIP' : 'llvm-strip'
    }

    llvm_toolchain_root = os.path.join(ndk_root, 'toolchains', 'llvm', 'prebuilt', host_platform)

    ndk_toolchain_search_paths = [
        os.path.join(llvm_toolchain_root, 'bin')
    ]

    common_flags = [
        '--target={}'.format(target_arch)
    ]

    if is_ndk_19_plus:
        # required 3rd party libs that need to be included in the apk
        env['EXT_LIBS'] += [
            ctx.add_to_android_cache(
                os.path.join(llvm_toolchain_root, 'sysroot', 'usr', 'lib', target_triple, 'libc++_shared.so'),
                arch_override = target_abi)
        ]

    else:
        gcc_toolchain_root = os.path.join(ndk_root, 'toolchains', '{}-4.9'.format(target_triple), 'prebuilt', host_platform)

        common_flags += [
            '--gcc-toolchain={}'.format(gcc_toolchain_root)
        ]

        if ndk_rev < 18:
            ndk_toolchains['AR'] = '{}-ar'.format(target_triple)
            ndk_toolchains['STRIP'] = '{}-strip'.format(target_triple)

            ndk_toolchain_search_paths += [
                os.path.join(gcc_toolchain_root, 'bin')
            ]

    if not (ctx.load_android_toolchains(ndk_toolchain_search_paths, **ndk_toolchains) and ctx.load_android_tools()):
        ctx.fatal('[ERROR] android_{}_clang setup failed'.format(architecture))

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]
    env['LINKFLAGS'] += common_flags[:]


@conf
def load_android_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    !!! But not the actual compiler, since the compiler depends on the host !!!
    """

    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']
    is_ndk_19_plus = env['ANDROID_IS_NDK_19_PLUS']

    common_flags = []
    cxx_flags = []

    if not is_ndk_19_plus:
        stl_root = os.path.join(ndk_root, 'sources', 'cxx-stl', 'llvm-libc++')

        env['INCLUDES'] += [
            os.path.join(stl_root, 'include'),
            os.path.join(ndk_root, 'sources', 'android', 'support', 'include') # the support includes must be after the stl ones
        ]

        env['LIBPATH'] += [
            os.path.join(stl_root, 'libs', env['ANDROID_ARCH']),
        ]

        env['LIB'] += [
            'c++_shared',           # shared library of llvm stl
        ]

        # required 3rd party libs that need to be included in the apk
        env['EXT_LIBS'] += [
            conf.add_to_android_cache(os.path.join(stl_root, 'libs', env['ANDROID_ARCH'], 'libc++_shared.so'))
        ]

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:] + cxx_flags[:]

    # these aren't defined in the common clang settings
    env['SHLIB_MARKER'] = '-Wl,-Bdynamic'
    env['STLIB_MARKER'] = '-Wl,-Bstatic'

    # not used on android
    env['ARCH_ST'] = []

    # disable support for the following build options
    env['COMPILER_FLAGS_DisableOptimization'] = [ ]
    env['COMPILER_FLAGS_DebugSymbols'] = [ ]
