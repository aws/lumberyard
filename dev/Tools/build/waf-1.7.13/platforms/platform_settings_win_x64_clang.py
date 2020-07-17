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

# waflib imports
from waflib.Configure import conf, Logs
from waflib import Errors


PLATFORM = 'win_x64_clang'

CLANG_VERSION = '7.0.0'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64 configurations
    """
    v = conf.env

    if not conf.find_program('clang', mandatory=False, silent_output=True):
        raise Errors.WafError("Unable to detect Clang for windows")
    
    v['PLATFORM'] = PLATFORM

    # Load MSVC settings for non-build stuff (AzCG, CrcFix, etc)
    # load_win_x64_win_x64_vs2017_common_settings(conf)
    conf.load_windows_common_settings()

    conf.load_win_x64_vs2017_common_settings()
    
    windows_kit = conf.options.win_vs2017_winkit
    try:
        # Attempt to detect the C++ compiler for VS 2015 ( msvs version 14.0 )
        ctx.detect_visual_studio_2015(windows_kit)
        system_includes = []
    except:
        Logs.warn('Unable to find Windows Kit {}, removing build target'.format(windows_kit))
        conf.disable_target_platform(PLATFORM)
        return

    
    # Remove MSVC/clang specific settings
    v['CFLAGS'] = []
    v['CXXFLAGS'] = []
    v['LINKFLAGS'] = []
    
    # Linker
    v['CCLNK_SRC_F'] = v['CXXLNK_SRC_F'] = []
    v['CCLNK_TGT_F'] = v['CXXLNK_TGT_F'] = '/OUT:'
    
    v['LIB_ST'] = '%s.lib'
    v['LIBPATH_ST'] = '/LIBPATH:%s'
    v['STLIB_ST'] = '%s.lib'
    v['STLIBPATH_ST'] = '/LIBPATH:%s'
    
    v['cprogram_PATTERN'] = '%s.exe'
    v['cxxprogram_PATTERN'] = '%s.exe'
    
    v['cstlib_PATTERN'] = '%s.lib'
    v['cxxstlib_PATTERN'] = '%s.lib'
    
    v['cshlib_PATTERN'] = '%s.dll'
    v['cxxshlib_PATTERN'] = '%s.dll'
    
    v['LINKFLAGS_cshlib'] = ['/DLL']
    v['LINKFLAGS_cxxshlib'] = ['/DLL']
    
    # AR Tools
    v['ARFLAGS'] = ['/NOLOGO']
    v['AR_TGT_F'] = '/OUT:'
    
    # Delete the env variables so that they can be replaced with the clang versions
    del v['AR']
    del v['CC']
    del v['CXX']
    del v['LINK']
    
    conf.find_program('clang', var='CC', silent_output=True)
    conf.find_program('clang++', var='CXX', silent_output=True)
    conf.find_program('llvm-lib', var='AR', silent_output=True)
    conf.find_program('lld-link', var='LINK', silent_output=True)
    
    v['LINK_CC'] = v['LINK_CXX'] = v['LINK']
    
    # Moved to platform.win_x64_clang.json
    """
    clang_FLAGS = [
        '-mcx16',
        '-msse3',
        
        '-Wno-macro-redefined',
        '-Wno-microsoft-cast',
        '-Wno-ignored-pragma-intrinsic',
        # Clang doens't need #pragma intrinsic anyway, so don't whine when one isn't recognized
    ]
    """
    clang_FLAGS = []
    
    # Path to clang.exe is [clang]/bin/clang.exe, but the include path is [clang]/lib/clang/7.0.0/include
    clang_include_path = os.path.join(
        os.path.dirname(os.path.dirname(v['CXX'])),
        'lib', 'clang', CLANG_VERSION, 'include')
    
    system_includes = [clang_include_path] + system_includes
    
    # Treat all MSVC include paths as system headers
    for include in system_includes:
        clang_FLAGS += ['-isystem', include]
    
    v['CFLAGS'] += clang_FLAGS
    v['CXXFLAGS'] += clang_FLAGS
    # Moved to platform.win_x64_clang.json
    """
    v['DEFINES'] += [
        '_CRT_SECURE_NO_WARNINGS',
        '_CRT_NONSTDC_NO_WARNINGS',
    ]
    """
    
    # Moved to platform.win_x64_clang.json
    """
    v['LINKFLAGS'] += [
        '/MACHINE:x64',
        '/MANIFEST',  # Create a manifest file
        '/OPT:REF', '/OPT:ICF',  # Always optimize for size, there's no reason not to
        '/LARGEADDRESSAWARE',  # tell the linker that the application can handle addresses larger than 2 gigabytes.
    ]
    """
    v['WINDOWS_CLANG_SUPPORTED'] = True

    conf.load_clang_common_settings()

    conf.load_cryengine_common_settings()

    conf.register_win_x64_external_optional_cuda(PLATFORM)


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_clang_configuration_settings(ctx, platform_configuration):
    # No special configuration-specific setup needed
    pass


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_clang_available(ctx):
    return ctx.is_option_true('win_enable_clang_for_windows')

