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


################################################################
@conf
def load_android_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    !!! But not the actual compiler, since the compiler depends on the host !!!
    """

    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']

    # in ndk r13, they removed the 'libcxx' sub-dir, leaving 'include' in the root of the folder
    stl_root = os.path.join(ndk_root, 'sources', 'cxx-stl', 'llvm-libc++')
    if env['ANDROID_NDK_REV_MAJOR'] >= 13:
        stl_includes = os.path.join(stl_root, 'include')
    else:
        stl_includes = os.path.join(stl_root, 'libcxx', 'include')

    env['INCLUDES'] += [
        stl_includes,
        os.path.join(ndk_root, 'sources', 'android', 'support', 'include') # the support includes must be after the stl ones
    ]

    common_flags = [
        '-femulated-tls',       # All accesses to TLS variables are converted to calls to __emutls_get_address in the runtime library
        '-Wno-unused-lambda-capture',

        # This was originally disabled only when building Android Clang on MacOS, but it seems the updated toolchain in NDK r15 has  
        # become even more aggressive in "nonportable" include paths, meaning it's nearly impossible to fix the casing the compiler 
        # thinks the path should be.
        # ORIGINAL COMMENT: Unless specified, OSX is generally case-preserving but case-insensitive.  Windows is the same way, however
        # OSX seems to behave differently when it comes to casing at the OS level where a file can be showing as upper-case in Finder 
        # and Terminal, the OS can see it as lower-case.
        '-Wno-nonportable-include-path',
    ]

    env['CFLAGS'] += common_flags[:]

    env['CXXFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += [
        '-fms-extensions',      # Allow MSVC language extensions
    ]

    env['LIB'] += [
        'c++_shared',           # shared library of llvm stl
    ]

    env['LIBPATH'] += [
        os.path.join(stl_root, 'libs', env['ANDROID_ARCH']),
    ]

    env['LINKFLAGS'] += [
        '-Wl,--gc-sections,--icf=safe', # --gc-sections will discard unused sections. --icf=safe will remove duplicate code
    ]

    # these aren't defined in the common clang settings
    env['SHLIB_MARKER'] = '-Wl,-Bdynamic'
    env['STLIB_MARKER'] = '-Wl,-Bstatic'

    # required 3rd party libs that need to be included in the apk
    env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(stl_root, 'libs', env['ANDROID_ARCH'], 'libc++_shared.so'))
    ]

    # not used on android
    env['ARCH_ST'] = []

    # disable support for the following build options
    env['COMPILER_FLAGS_DisableOptimization'] = [ ] 
    env['COMPILER_FLAGS_DebugSymbols'] = [ ]

################################################################
@conf
def load_debug_android_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    for the "debug" configuration
    """

    conf.load_android_clang_common_settings()


################################################################
@conf
def load_profile_android_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    for the "profile" configuration
    """

    conf.load_android_clang_common_settings()


################################################################
@conf
def load_performance_android_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    for the "performance" configuration
    """

    conf.load_android_clang_common_settings()


################################################################
@conf
def load_release_android_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all android clang configurations
    for the "release" configuration
    """

    conf.load_android_clang_common_settings()

