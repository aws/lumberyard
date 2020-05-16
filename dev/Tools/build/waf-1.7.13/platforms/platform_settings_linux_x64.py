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

# waflib imports
from waflib.Configure import conf

# lmbrwaflib imports
from lmbrwaflib import lumberyard


PLATFORM = 'linux_x64'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_linux_x64_common_settings(ctx):

    env = ctx.env

    platform_settings = ctx.get_platform_settings(PLATFORM)

    default_bin_search_paths = ['/usr/bin']

    cc_compiler = platform_settings.attributes.get('cc_compiler', 'clang')
    cxx_compiler = platform_settings.attributes.get('cxx_compiler', 'clang++')
    compiler_bin_search_paths = platform_settings.attributes.get('compiler_search_paths', default_bin_search_paths)

    error_message_fmt = "Unable to detect '%s' in search paths: %s. Make sure it is installed on this system"

    ctx.find_program('ar',
                     path_list=default_bin_search_paths,
                     var='AR',
                     errmsg=error_message_fmt % ('ar', ','.join(default_bin_search_paths)),
                     silent_output=False)

    ctx.find_program(cc_compiler,
                     path_list=compiler_bin_search_paths,
                     var='CC',
                     errmsg=error_message_fmt % (cc_compiler, ','.join(compiler_bin_search_paths)),
                     silent_output=False)

    ctx.find_program(cxx_compiler,
                     path_list=compiler_bin_search_paths,
                     var='CXX',
                     errmsg=error_message_fmt % (cxx_compiler, ','.join(compiler_bin_search_paths)),
                     silent_output=False)

    env['LINK'] = env['LINK_CC'] = env['LINK_CXX'] = env['CXX']

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


@lumberyard.multi_conf
def generate_ib_profile_tool_elements(ctx):
    linux_tool_elements = [
        '<Tool Filename="x86_64-linux-gnu-gcc" AllowRemote="true" AllowIntercept="false" DeriveCaptionFrom="lastparam" AllowRestartOnLocal="false"/>',
        '<Tool Filename="x86_64-linux-gnu-g++" AllowRemote="true" AllowIntercept="false" DeriveCaptionFrom="lastparam" AllowRestartOnLocal="false"/>'
    ]
    return linux_tool_elements
