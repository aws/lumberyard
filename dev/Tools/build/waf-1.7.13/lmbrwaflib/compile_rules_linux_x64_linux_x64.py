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
from waflib.Configure import conf


@conf
def load_linux_x64_linux_x64_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64_linux_x64 configurations
    """
    v = conf.env
    
    # Setup Tools for CLang Toolchain (simply used system installed version)
    v['AR'] = 'ar'
    v['CC'] = 'clang'
    v['CXX'] = 'clang++'
    v['LINK'] = v['LINK_CC'] = v['LINK_CXX'] = 'clang++'
    
    v['CFLAGS'] += [ '-m64', '-fPIC' ]
    v['CXXFLAGS'] += [ '-m64', '-fPIC', '-stdlib=libc++' ]


@conf
def load_debug_linux_x64_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64_linux_x64 configurations for
    the 'debug' configuration
    """
    conf.load_linux_x64_linux_x64_common_settings()
    
    # Load additional shared settings
    conf.load_debug_cryengine_settings()
    conf.load_debug_clang_settings()
    conf.load_debug_linux_settings()
    conf.load_debug_linux_x64_settings()


@conf
def load_profile_linux_x64_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64_linux_x64 configurations for
    the 'profile' configuration
    """
    conf.load_linux_x64_linux_x64_common_settings()
    
    # Load additional shared settings
    conf.load_profile_cryengine_settings()
    conf.load_profile_clang_settings()
    conf.load_profile_linux_settings()
    conf.load_profile_linux_x64_settings()


@conf
def load_performance_linux_x64_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64_linux_x64 configurations for
    the 'performance' configuration
    """
    conf.load_linux_x64_linux_x64_common_settings()
    
    # Load additional shared settings
    conf.load_performance_cryengine_settings()
    conf.load_performance_clang_settings()
    conf.load_performance_linux_settings()
    conf.load_performance_linux_x64_settings()


@conf
def load_release_linux_x64_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64_linux_x64 configurations for
    the 'release' configuration
    """
    conf.load_linux_x64_linux_x64_common_settings()
    
    # Load additional shared settings
    conf.load_release_cryengine_settings()
    conf.load_release_clang_settings()
    conf.load_release_linux_settings()
    conf.load_release_linux_x64_settings()

