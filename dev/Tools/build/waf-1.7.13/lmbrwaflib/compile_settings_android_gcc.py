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
def load_android_gcc_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android gcc configurations
    !!! But not the actual compiler, since the compiler depends on the host !!!
    """

    # additional gcc build settings for android
    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']

    stl_root = os.path.join(ndk_root, 'sources', 'cxx-stl', 'gnu-libstdc++', '4.9')
    env['INCLUDES'] += [
        os.path.join(stl_root, 'include'),
        os.path.join(stl_root, 'libs', env['ANDROID_ARCH'], 'include'),
    ]

    common_flags = [
        # Enabled Warnings
        '-Wsequence-point',         # undefined semantics warning
        '-Wmissing-braces',         # missing brackets in union initializers
        '-Wreturn-type',            # no return value
        '-Winit-self',              # un-initialized vars used to init themselves
        '-Winvalid-pch',            # cant use found pre-compiled header

        # Disabled Warnings
        '-Wno-error=pragmas',       # Bad pragmas wont cause compile errors, just warnings
        '-Wno-deprecated',          # allow deprecated features of the compiler, not the deprecated attribute

        # Other
        '-fms-extensions',          # allow some non standard constructs
        '-MP',                      # add PHONY targets to each dependency
        '-MMD',                     # use file name as object file name
    ]

    env['CFLAGS'] += common_flags[:]

    env['CXXFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += [
        '-std=gnu++1y',             # set the c++ standard to GNU C++14
        '-fpermissive',             # allow some non-conformant code
    ]

    env['LIB'] += [
        'gnustl_shared',            # shared library of gnu stl
    ]

    env['LIBPATH'] += [
        os.path.join(stl_root, 'libs', env['ANDROID_ARCH']),
    ]

    # required 3rd party libs that need to be included in the apk
    env['EXT_LIBS'] += [
        conf.add_to_android_cache(os.path.join(stl_root, 'libs', env['ANDROID_ARCH'], 'libgnustl_shared.so'))
    ]

    # disable support for the following build options
    env['COMPILER_FLAGS_DisableOptimization'] = [ ] 
    env['COMPILER_FLAGS_DebugSymbols'] = [ ] 


################################################################
@conf
def load_debug_android_gcc_settings(conf):
    """
    Setup all compiler and linker settings shared over all android gcc configurations
    for the "debug" configuration
    """

    conf.load_android_gcc_common_settings()


################################################################
@conf
def load_profile_android_gcc_settings(conf):
    """
    Setup all compiler and linker settings shared over all android gcc configurations
    for the "profile" configuration
    """

    conf.load_android_gcc_common_settings()


################################################################
@conf
def load_performance_android_gcc_settings(conf):
    """
    Setup all compiler and linker settings shared over all android gcc configurations
    for the "performance" configuration
    """

    conf.load_android_gcc_common_settings()


################################################################
@conf
def load_release_android_gcc_settings(conf):
    """
    Setup all compiler and linker settings shared over all android gcc configurations
    for the "release" configuration
    """

    conf.load_android_gcc_common_settings()

