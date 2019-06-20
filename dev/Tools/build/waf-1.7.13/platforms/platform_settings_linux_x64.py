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
from waflib import Errors, Logs
from waflib.Configure import conf
from lumberyard import deprecated


PLATFORM = 'linux_x64'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_linux_x64_common_settings(ctx):

    env = ctx.env
    
    # Setup Tools for CLang Toolchain (simply used system installed version)
    env['AR'] = 'ar'
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = env['LINK_CC'] = env['LINK_CXX'] = 'clang++'
    
    # Pattern to transform outputs
    env['cprogram_PATTERN']   = '%s'
    env['cxxprogram_PATTERN'] = '%s'
    env['cshlib_PATTERN']     = 'lib%s.so'
    env['cxxshlib_PATTERN']   = 'lib%s.so'
    env['cstlib_PATTERN']     = 'lib%s.a'
    env['cxxstlib_PATTERN']   = 'lib%s.a'

    # ASAN and ASLR
    # once we can use clang 4, add , '-fsanitize-address-use-after-scope'
    # disabled until the linker requirements are worked out on linux
    env['LINKFLAGS_ASLR'] = [] #['-fsanitize=memory']
    env['ASAN_cflags'] = [] # ['-fsanitize=address']
    env['ASAN_cxxflags'] = [] # ['-fsanitize=address']

    ctx.load_cryengine_common_settings()

    ctx.load_clang_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_linux_x64_configuration_settings(ctx, platform_configuration):
    # No special configuration-specific setup needed
    pass


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_linux_x64_available(ctx):
    return True


