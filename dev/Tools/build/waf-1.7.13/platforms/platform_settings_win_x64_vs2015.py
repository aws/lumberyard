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
from waflib import Errors
from waflib.Configure import conf, Logs
from lumberyard import deprecated

import os


PLATFORM = 'win_x64_vs2015'
COMPILER_ID = 'msvc 14.0'
COMPILER_ARCH = 'x64'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_vs2015_common_settings(ctx):

    env = ctx.env

    restricted_tool_list_macro_header = 'AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS='
    restricted_tool_list_macro = restricted_tool_list_macro_header
    if len(restricted_tool_list_macro) > len(restricted_tool_list_macro_header):
        env['DEFINES'] += [restricted_tool_list_macro]

    # Attempt to detect the C++ compiler for VS 2015 ( msvs version 14.0 )
    windows_kit = ctx.options.win_vs2015_winkit
    try:
        ctx.auto_detect_msvc_compiler(COMPILER_ID, COMPILER_ARCH, windows_kit)
    except Exception as e:
        raise Errors.WafError("Unable to find Visual Studio 2015 C++ compiler and/or Windows Kit {}: {}".format(windows_kit, str(e)))

    # Detect the QT binaries, if the current capabilities selected requires it.
    _, enabled, _, _ = ctx.tp.get_third_party_path(PLATFORM, 'qt')
    if enabled:
        ctx.find_qt5_binaries(PLATFORM)

    if ctx.options.use_uber_files:
        env['CFLAGS'] += ['/bigobj']
        env['CXXFLAGS'] += ['/bigobj']

    if not env['CODE_GENERATOR_PATH']:
        raise Errors.WafError('AZ Code Generator path not set for target platform {}'.format(PLATFORM))

    ctx.find_dx12(windows_kit)

    ctx.load_msvc_common_settings()

    ctx.load_windows_common_settings()

    ctx.load_cryengine_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_vs2015_configuration_settings(ctx, platform_configuration):
    
    if platform_configuration.settings.does_configuration_match('debug'):
        ctx.register_win_x64_external_ly_identity('vs2015', 'Debug')
        ctx.register_win_x64_external_ly_metrics('vs2015', 'Debug')

    elif platform_configuration.settings.does_configuration_match('profile'):
        ctx.register_win_x64_external_ly_identity('vs2015', 'Release')
        ctx.register_win_x64_external_ly_metrics('vs2015', 'Release')


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_vs2015_available(ctx):
    return True
