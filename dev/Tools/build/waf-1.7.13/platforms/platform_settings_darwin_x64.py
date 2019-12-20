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
import os

PLATFORM = 'darwin_x64'

def apple_clang_supports_option(option):
    '''
    Test that a compiler switch/option is supported by the apple clang that is in the PATH
    '''
    with open(os.devnull, "w") as DEV_NULL:
        clang_subprocess = subprocess.Popen(['clang', option, '-o-', '-x', 'c++', '-'], 
            stdin=subprocess.PIPE, stdout=DEV_NULL, stderr=subprocess.STDOUT)
        clang_subprocess.stdin.write('int main(){}\n')
        clang_subprocess.communicate()
        clang_subprocess.stdin.close()
        return clang_subprocess.returncode == 0
    return false

# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_darwin_x64_common_settings(ctx):
    """
    Setup all compiler and linker settings shared over all darwin configurations
    """
    env = ctx.env
    
    # For now, only support 64 bit MacOs Applications
    env['ARCH'] = ['x86_64']
    
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
    env['RPATH_ST'] = '-Wl,-rpath,%s'
    env['ENABLE_STRICT_OBJC_MSGSEND'] = "NO"
    
    # Setup compiler and linker settings  for mac bundles
    env['CFLAGS_MACBUNDLE'] = env['CXXFLAGS_MACBUNDLE'] = '-fpic'
    env['LINKFLAGS_MACBUNDLE'] = [
        '-dynamiclib',
        '-undefined',
        'dynamic_lookup'
    ]
    
    # Set the path to the current sdk
    sdk_path = subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"]).strip()
    env['CFLAGS'] += ['-isysroot' + sdk_path, ]
    env['CXXFLAGS'] += ['-isysroot' + sdk_path, ]
    env['LINKFLAGS'] += ['-isysroot' + sdk_path, ]
    env['ISYSROOT'] = sdk_path

    # Since we only support a single darwin target (clang-64bit), we specify all tools directly here
    env['AR'] = 'ar'
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = env['LINK_CC'] = env['LINK_CXX'] = 'clang++'
    
    # Add the C++ -fno-aligned-allocation switch for "real" clang versions 7.0.0(AppleClang version 10.0.1) and above for Mac
    # The MacOS only supports C++17 aligned new/delete when targeting MacOS 10.14 and above
    if apple_clang_supports_option('-fno-aligned-allocation'):
        env['CXXFLAGS'] += ['-fno-aligned-allocation']

    ctx.load_cryengine_common_settings()
    
    ctx.load_clang_common_settings()


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_darwin_x64_configuration_settings(ctx, platform_configuration):
    
    if platform_configuration.settings.does_configuration_match('debug'):
        ctx.register_darwin_external_ly_identity('Debug')
        ctx.register_darwin_external_ly_metrics('Debug')
        
    elif platform_configuration.settings.does_configuration_match('profile'):
        ctx.register_darwin_external_ly_identity('Release')
        ctx.register_darwin_external_ly_metrics('Release')


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_darwin_x64_available(ctx):
    return True

