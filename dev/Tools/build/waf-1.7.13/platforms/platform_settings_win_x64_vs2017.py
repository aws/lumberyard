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

# waflib imports
from waflib.Configure import conf, conf_event

# local imports
from platforms import platform_settings_win_x64


VS_VERSION = '2017'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_vs2017_common_settings(ctx):
    ctx.load_win_x64_common_settings(VS_VERSION)


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_vs2017_configuration_settings(ctx, platform_configuration):
    ctx.load_win_x64_configuration_settings(platform_configuration, VS_VERSION)


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_vs2017_available(ctx):
    return ctx.is_win_x64_available(VS_VERSION)


@conf_event(after_methods=['load_compile_rules_for_enabled_platforms'],
            after_events=['inject_generate_uber_command', 'inject_generate_module_def_command'])
def inject_msvs_2017_command(conf):
    """
    Make sure the msvs command is injected into the configuration context
    """
    platform_settings_win_x64.inject_msvs_command(conf, VS_VERSION)
