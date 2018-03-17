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
from waflib.Configure import conf, Logs


@conf
def load_darwin_x64_darwin_x64_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_darwin_x64 configurations
    """

    common_flags = [
        # Unless specified, OSX is generally case-preserving but case-insensitive.  Windows is the same way, however
        # OSX seems to behave differently when it comes to casing at the OS level where a file can be showing as
        # upper-case in Finder and Terminal, the OS can see it as lower-case.
        '-Wno-nonportable-include-path',
    ]
    conf.env['CFLAGS'] += common_flags[:]
    conf.env['CXXFLAGS'] += common_flags[:]

@conf
def load_debug_darwin_x64_darwin_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_darwin_x64 configurations for
    the 'debug' configuration
    """
    conf.load_darwin_x64_darwin_x64_common_settings()

    # Load additional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_clang_settings()
    conf.load_debug_darwin_settings()

    conf.register_darwin_external_ly_identity('Debug')
    conf.register_darwin_external_ly_metrics('Debug')

@conf
def load_profile_darwin_x64_darwin_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_darwin_x64 configurations for
    the 'profile' configuration
    """
    conf.load_darwin_x64_darwin_x64_common_settings()

    # Load additional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_clang_settings()
    conf.load_profile_darwin_settings()

    conf.register_darwin_external_ly_identity('Release')
    conf.register_darwin_external_ly_metrics('Release')

@conf
def load_performance_darwin_x64_darwin_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_darwin_x64 configurations for
    the 'performance' configuration
    """
    conf.load_darwin_x64_darwin_x64_common_settings()

    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_clang_settings()
    conf.load_performance_darwin_settings()


@conf
def load_release_darwin_x64_darwin_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_darwin_x64 configurations for
    the 'release' configuration
    """
    conf.load_darwin_x64_darwin_x64_common_settings()

    # Load additional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_clang_settings()
    conf.load_release_darwin_settings()
