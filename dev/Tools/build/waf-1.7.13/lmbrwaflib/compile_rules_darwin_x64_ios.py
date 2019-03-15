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
from waflib.Configure import conf

@conf
def load_darwin_x64_ios_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_ios configurations
    """
    pass

@conf
def load_debug_darwin_x64_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_ios configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_darwin_x64_ios_common_settings()

    # Load additional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_clang_settings()
    conf.load_debug_ios_settings()

@conf
def load_profile_darwin_x64_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_ios configurations for
    the 'profile' configuration
    """
    v = conf.env
    conf.load_darwin_x64_ios_common_settings()

    # Load additional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_clang_settings()
    conf.load_profile_ios_settings()

@conf
def load_performance_darwin_x64_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_ios configurations for
    the 'performance' configuration
    """
    v = conf.env
    conf.load_darwin_x64_ios_common_settings()

    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_clang_settings()
    conf.load_performance_ios_settings()

@conf
def load_release_darwin_x64_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin_x64_ios configurations for
    the 'release' configuration
    """
    v = conf.env
    conf.load_darwin_x64_ios_common_settings()

    # Load additional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_clang_settings()
    conf.load_release_ios_settings()
