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
from waflib import TaskGen
from waflib.Configure import conf
from waflib.TaskGen import feature, after_method
import os
import subprocess

@conf
def load_appletv_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all appletv configurations
    """
    environment = conf.env
    
    # Setup common defines for appletv
    environment['DEFINES'] += [ 'APPLE', 'APPLETV', 'MOBILE', 'APPLE_BUNDLE' ]
    
    # Set Minimum appletv version and the path to the current sdk
    conf.options.min_appletvos_version = "9.2"
    sdk_path = subprocess.check_output(["xcrun", "--sdk", "appletvos", "--show-sdk-path"]).strip()
    environment['CFLAGS'] += [ '-mappletvos-version-min=' + conf.options.min_appletvos_version, '-isysroot' + sdk_path, '-Wno-shorten-64-to-32' ]
    environment['CXXFLAGS'] += [ '-mappletvos-version-min=' + conf.options.min_appletvos_version, '-isysroot' + sdk_path, '-Wno-shorten-64-to-32' ]
    environment['MMFLAGS'] = environment['CXXFLAGS'] + ['-x', 'objective-c++']

    environment['LINKFLAGS'] += [ '-isysroot' + sdk_path, '-mappletvos-version-min='+conf.options.min_appletvos_version]
    environment['LIB'] += [ 'pthread' ]
    
    # For now, only support arm64. May need to add armv7 and armv7s
    environment['ARCH'] = ['arm64']
    
    # Pattern to transform outputs
    environment['cprogram_PATTERN']   = '%s'
    environment['cxxprogram_PATTERN'] = '%s'
    environment['cshlib_PATTERN']     = 'lib%s.dylib'
    environment['cxxshlib_PATTERN']   = 'lib%s.dylib'
    environment['cstlib_PATTERN']     = 'lib%s.a'
    environment['cxxstlib_PATTERN']   = 'lib%s.a'
    environment['macbundle_PATTERN']  = 'lib%s.dylib'
    
    # Specify how to translate some common operations for a specific compiler   
    environment['FRAMEWORK_ST']     = ['-framework']
    environment['FRAMEWORKPATH_ST'] = '-F%s'
    
    # Default frameworks to always link
    environment['FRAMEWORK'] = [ 'Foundation', 'UIKit', 'QuartzCore', 'GameController' ]

    # Setup compiler and linker settings for mac bundles
    environment['CFLAGS_MACBUNDLE'] = environment['CXXFLAGS_MACBUNDLE'] = '-fpic'
    environment['LINKFLAGS_MACBUNDLE'] = [
        '-bundle', 
        '-undefined', 
        'dynamic_lookup'
        ]
    
    # Since we only support a single appletv target (clang-64bit), we specify all tools directly here    
    environment['AR'] = 'ar'
    environment['CC'] = 'clang'
    environment['CXX'] = 'clang++'
    environment['LINK'] = environment['LINK_CC'] = environment['LINK_CXX'] = 'clang++'
    
@conf
def load_debug_appletv_settings(conf):
    """
    Setup all compiler and linker settings shared over all appletv configurations for
    the 'debug' configuration
    """
    conf.load_appletv_common_settings()

@conf
def load_profile_appletv_settings(conf):
    """
    Setup all compiler and linker settings shared over all appletv configurations for
    the 'debug' configuration
    """
    conf.load_appletv_common_settings()

@conf
def load_performance_appletv_settings(conf):
    """
    Setup all compiler and linker settings shared over all appletv configurations for
    the 'debug' configuration
    """
    conf.load_appletv_common_settings()

@conf
def load_release_appletv_settings(conf):
    """
    Setup all compiler and linker settings shared over all appletv configurations for
    the 'debug' configuration
    """
    conf.load_appletv_common_settings()
    
