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
import os
import subprocess

@conf
def load_ios_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all ios configurations
    """
    environment = conf.env
    
    # Setup common defines for ios
    environment['DEFINES'] += [ 'APPLE', 'IOS', 'MOBILE', 'APPLE_BUNDLE' ]
    
    # Set Minimum ios version and the path to the current sdk
    conf.options.min_iphoneos_version = "9.0"
    sdk_path = subprocess.check_output(["xcrun", "--sdk", "iphoneos", "--show-sdk-path"]).strip()
    environment['CFLAGS'] += [ '-miphoneos-version-min=' + conf.options.min_iphoneos_version, '-isysroot' + sdk_path, '-Wno-shorten-64-to-32' ]
    environment['CXXFLAGS'] += [ '-miphoneos-version-min=' + conf.options.min_iphoneos_version, '-isysroot' + sdk_path, '-Wno-shorten-64-to-32' ]
    environment['MMFLAGS'] = environment['CXXFLAGS'] + ['-x', 'objective-c++']

    environment['LINKFLAGS'] += [ '-Wl,-dead_strip', '-isysroot' + sdk_path, '-miphoneos-version-min='+conf.options.min_iphoneos_version]
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
    environment['FRAMEWORK'] = [ 'Foundation', 'UIKit', 'QuartzCore', 'GameController', 'CoreMotion' ]

    # Setup compiler and linker settings for mac bundles
    environment['CFLAGS_MACBUNDLE'] = environment['CXXFLAGS_MACBUNDLE'] = '-fpic'
    environment['LINKFLAGS_MACBUNDLE'] = [
        '-bundle', 
        '-undefined', 
        'dynamic_lookup'
        ]
    
    # Since we only support a single ios target (clang-64bit), we specify all tools directly here    
    environment['AR'] = 'ar'
    environment['CC'] = 'clang'
    environment['CXX'] = 'clang++'
    environment['LINK'] = environment['LINK_CC'] = environment['LINK_CXX'] = 'clang++'
    
@conf
def load_debug_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all ios configurations for
    the 'debug' configuration
    """
    conf.load_ios_common_settings()

@conf
def load_profile_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all ios configurations for
    the 'debug' configuration
    """
    conf.load_ios_common_settings()

@conf
def load_performance_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all ios configurations for
    the 'debug' configuration
    """
    conf.load_ios_common_settings()

@conf
def load_release_ios_settings(conf):
    """
    Setup all compiler and linker settings shared over all ios configurations for
    the 'debug' configuration
    """
    conf.load_ios_common_settings()
    
