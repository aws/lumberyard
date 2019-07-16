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

from waflib import Errors
from waflib.Configure import conf, Logs
from lumberyard import deprecated


PLATFORM = 'win_x64_vs2017'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_vs2017_common_settings(ctx):

    env = ctx.env
    
    restricted_tool_list_macro_header = 'AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS='
    restricted_tool_list_macro = restricted_tool_list_macro_header
    if len(restricted_tool_list_macro) > len(restricted_tool_list_macro_header):
        env['DEFINES'] += [restricted_tool_list_macro]

    # Attempt to detect the C++ compiler for VS 2017 ( msvs version 15.0 )
    windows_kit = ctx.options.win_vs2017_winkit
    vcvarsall_args = windows_kit + ' ' + ctx.options.win_vs2017_vcvarsall_args
    try:
        ctx.auto_detect_msvc_compiler('msvc 15', 'x64', vcvarsall_args)
    except Exception as err:
        raise Errors.WafError("Unable to find Visual Studio 2017 ({}) C++ compiler and/or Windows Kit {}: {}".format(ctx.options.win_vs2017_vswhere_args, vcvarsall_args, str(err)))

    # Detect the QT binaries, if the current capabilities selected requires it.
    _, enabled, _, _ = ctx.tp.get_third_party_path(PLATFORM, 'qt')
    if enabled:
        ctx.find_qt5_binaries(PLATFORM)

    compiler_flags = []
    if ctx.options.use_uber_files:
        compiler_flags.append('/bigobj')

    env['CFLAGS'] += compiler_flags
    env['CXXFLAGS'] += compiler_flags

    if not env['CODE_GENERATOR_PATH']:
        raise Errors.WafError('[Error] AZ Code Generator path not set for target platform {}'.format(PLATFORM))
    
    ctx.find_dx12(windows_kit)

    ctx.load_cryengine_common_settings()
    
    ctx.load_msvc_common_settings()

    ctx.load_windows_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_vs2017_configuration_settings(ctx, platform_configuration):
    
    if platform_configuration.settings.does_configuration_match('debug'):
        ctx.register_win_x64_external_ly_identity('vs2017', 'Debug')
        ctx.register_win_x64_external_ly_metrics('vs2017', 'Debug')

    elif platform_configuration.settings.does_configuration_match('profile'):
        ctx.register_win_x64_external_ly_identity('vs2017', 'Release')
        ctx.register_win_x64_external_ly_metrics('vs2017', 'Release')


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_vs2017_available(ctx):
    return True
