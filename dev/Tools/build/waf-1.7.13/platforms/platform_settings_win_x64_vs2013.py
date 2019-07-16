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
#

from waflib import Errors
from waflib.Configure import conf, Logs
from lumberyard import deprecated

PLATFORM = 'win_x64_vs2013'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_vs2013_common_settings(ctx):

    env = ctx.env

    global PLATFORM
    # Attempt to detect the C++ compiler for VS 2013 ( msvs version 12.0 )
    try:
        ctx.auto_detect_msvc_compiler('msvc 12.0', 'x64', '')
    except Exception as err:
        raise Errors.WafError("Unable to find Visual Studio 2013 C++ compiler: {}".format(str(err)))

    # Detect the QT binaries
    ctx.find_qt5_binaries(PLATFORM)

    if ctx.options.use_uber_files:
        env['CFLAGS'] += ['/bigobj']
        env['CXXFLAGS'] += ['/bigobj']

    if not env['CODE_GENERATOR_PATH']:
        raise Errors.WafError('[Error] AZ Code Generator path not set for target platform {}'.format(PLATFORM))

    if not env['CRCFIX_PATH']:
        raise Errors.WafError('[Error] CRCFix path not set for target platform {}'.format(PLATFORM))

    conf.load_msvc_common_settings()

    conf.load_windows_common_settings()

    conf.load_cryengine_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_vs2013_configuration_settings(ctx, platform_configuration):
    
    if platform_configuration.settings.does_configuration_match('debug'):
        ctx.register_win_x64_external_ly_identity('vs2013', 'Debug')
        ctx.register_win_x64_external_ly_metrics('vs2013', 'Debug')

    elif platform_configuration.settings.does_configuration_match('profile'):
        ctx.register_win_x64_external_ly_identity('vs2013', 'Release')
        ctx.register_win_x64_external_ly_metrics('vs2013', 'Release')


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_vs2013_available(ctx):
    # Disabled, no longer officially supported
    return False

