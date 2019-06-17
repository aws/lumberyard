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
from waflib import Errors, Logs
from waflib.Configure import conf
from lumberyard import deprecated
import subprocess


PLATFORM = 'appletv'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_appletv_common_settings(ctx):
    """
    Setup all compiler and linker settings shared over all appletv configurations
    """
    env = ctx.env
    
    # Set Minimum appletv version and the path to the current sdk
    settings_file = ctx.path.make_node(['_WAF_', 'apple', 'appletv_settings.json'])
    try:
        settings = ctx.parse_json_file(settings_file)
    except Exception as e:
        ctx.cry_file_error(str(e), settings_file.abspath())

    min_appletvos_version = '-mappletvos-version-min={}'.format(settings['TVOS_DEPLOYMENT_TARGET'])

    sdk_path = subprocess.check_output(['xcrun', '--sdk', 'appletvos', '--show-sdk-path']).strip()
    sysroot = '-isysroot{}'.format(sdk_path)

    common_flags = [
        sysroot,
        min_appletvos_version,
        '-Wno-shorten-64-to-32'
    ]

    env['CFLAGS'] += common_flags
    env['CXXFLAGS'] += common_flags
    env['MMFLAGS'] = env['CXXFLAGS'] + ['-x', 'objective-c++']

    env['LINKFLAGS'] += ['-Wl,-dead_strip', sysroot, min_appletvos_version]

    # For now, only support arm64. May need to add armv7 and armv7s
    env['ARCH'] = ['arm64']
    
    # Pattern to transform outputs
    env['cprogram_PATTERN'] = '%s'
    env['cxxprogram_PATTERN'] = '%s'
    env['cshlib_PATTERN'] = 'lib%s.dylib'
    env['cxxshlib_PATTERN'] = 'lib%s.dylib'
    env['cstlib_PATTERN'] = 'lib%s.a'
    env['cxxstlib_PATTERN'] = 'lib%s.a'
    env['macbundle_PATTERN'] = 'lib%s.dylib'
    
    # Specify how to translate some common operations for a specific compiler
    env['FRAMEWORK_ST'] = ['-framework']
    env['FRAMEWORKPATH_ST'] = '-F%s'
    
    # Setup compiler and linker settings for mac bundles
    env['CFLAGS_MACBUNDLE'] = env['CXXFLAGS_MACBUNDLE'] = '-fpic'
    env['LINKFLAGS_MACBUNDLE'] = [
        '-bundle',
        '-undefined',
        'dynamic_lookup'
    ]
    
    # Since we only support a single appletv target (clang-64bit), we specify all tools directly here
    env['AR'] = 'ar'
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = env['LINK_CC'] = env['LINK_CXX'] = 'clang++'

    ctx.load_cryengine_common_settings()

    ctx.load_clang_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_appletv_configuration_settings(ctx, platform_configuration):
    # No special configuration-specific setup needed
    pass


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_appletv_available(ctx):
    return True
