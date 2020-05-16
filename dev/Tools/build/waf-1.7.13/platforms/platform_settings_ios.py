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

# System Imports
import os
import subprocess
import sys

# waflib imports
from waflib.Configure import conf


PLATFORM = 'ios'
def apple_clang_supports_option(option):
    '''
    Test that a compiler switch/option is supported by the apple clang that is in the PATH
    '''
    with open(os.devnull, "w") as DEV_NULL:
        clang_subprocess = subprocess.Popen(['clang', option, '-o-', '-x', 'c++', '-'], 
            stdin=subprocess.PIPE, stdout=DEV_NULL, stderr=subprocess.STDOUT)
        clang_subprocess.stdin.write(b'int main(){}\n')
        clang_subprocess.communicate()
        clang_subprocess.stdin.close()
        return clang_subprocess.returncode == 0
    return false

# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_ios_common_settings(ctx):

    env = ctx.env

    # Set Minimum ios version and the path to the current sdk
    settings = ctx.get_ios_xcode_settings()
    min_iphoneos_version = '-miphoneos-version-min={}'.format(settings['IPHONEOS_DEPLOYMENT_TARGET'])

    sdk_path = subprocess.check_output(['xcrun', '--sdk', 'iphoneos', '--show-sdk-path']).decode(sys.stdout.encoding or 'iso8859-1', 'replace').strip()
    sysroot = '-isysroot{}'.format(sdk_path)

    common_flags = [
        sysroot,
        min_iphoneos_version,
        '-Wno-shorten-64-to-32'
    ]

    env['CFLAGS'] += common_flags
    env['CXXFLAGS'] += common_flags
    env['MMFLAGS'] = env['CXXFLAGS'] + ['-x', 'objective-c++']

    env['LINKFLAGS'] += ['-Wl,-dead_strip', sysroot, min_iphoneos_version]

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

    # Since we only support a single ios target (clang-64bit), we specify all tools directly here    
    env['AR'] = 'ar'
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = env['LINK_CC'] = env['LINK_CXX'] = 'clang++'

    # Add the C++ -fno-aligned-allocation switch for "real" clang versions 7.0.0(AppleClang version 10.0.1) and above for Mac
    # iOS only supports C++17 aligned new/delete when targeting iOS 11 and above
    if apple_clang_supports_option('-fno-aligned-allocation'):
        env['CXXFLAGS'] += ['-fno-aligned-allocation']
    ctx.load_cryengine_common_settings()
    
    ctx.load_clang_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_ios_configuration_settings(ctx, platform_configuration):
    # No special configuration-specific setup needed
    pass


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_ios_available(ctx):
    return True


@conf
def load_apple_arm_settings(ctx, settings_name):
    """ Helper function for loading the global apple settings """

    if hasattr(ctx, settings_name):
        return

    settings_file_names = [
        'common_arm_settings.json',
        '{}.json'.format(settings_name)
    ]

    settings_files = list()
    for root_node in ctx._get_settings_search_roots():
        for file_name in settings_file_names:
            settings_node = root_node.make_node(['_WAF_', 'apple', file_name])
            if os.path.exists(settings_node.abspath()):
                settings_files.append(settings_node)

    settings = dict()
    for settings_file in settings_files:
        try:
            settings.update(ctx.parse_json_file(settings_file))
        except Exception as e:
            ctx.cry_file_error(str(e), settings_file.abspath())

    # For symroot need to have the full path so change the value from
    # relative path to a full path
    if 'SYMROOT' in settings:
        settings['SYMROOT'] = os.path.join(Context.launch_dir, settings['SYMROOT'])

    setattr(ctx, settings_name, settings)


@conf
def get_ios_xcode_settings(self):
    """" Helper function for getting ios settings """
    self.load_apple_arm_settings('ios_settings')
    return self.ios_settings


@conf
def get_ios_project_name(self):
    return '{}/{}'.format(self.options.ios_project_folder, self.options.ios_project_name)