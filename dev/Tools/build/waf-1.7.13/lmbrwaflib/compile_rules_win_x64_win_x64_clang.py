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
import os
from waflib.Configure import conf, Logs
from waflib.TaskGen import feature, after_method
from compile_rules_win_x64_win_x64_vs2015 import load_win_x64_win_x64_vs2015_common_settings

PLATFORM = 'win_x64_clang'

@conf
def load_win_x64_win_x64_clang_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64 configurations
    """
    v = conf.env

    global PLATFORM

    # Load MSVC settings for non-build stuff (AzCG, CrcFix, etc)
    load_win_x64_win_x64_vs2015_common_settings(conf)

    windows_kit = conf.options.win_vs2015_winkit
    try:
        _, _, _, system_includes, _, _ = conf.detect_msvc(windows_kit, True)
    except:
        Logs.warn('Unable to find Windows Kit {}, removing build target'.format(windows_kit))
        conf.mark_supported_platform_for_removal(PLATFORM)
        return

    restricted_tool_list_macro_header = 'AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS='
    restricted_tool_list_macro = restricted_tool_list_macro_header

    # Start with a blank platform slate
    conf.undefine('AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS')
    if len(restricted_tool_list_macro) > len(restricted_tool_list_macro_header):
        v['DEFINES'] += [ restricted_tool_list_macro ]

    # Remove MSVC/clang specific settings
    v['CFLAGS'] = []
    v['CXXFLAGS'] = []
    v['LINKFLAGS'] = []

    # Linker
    v['CCLNK_SRC_F'] = v['CXXLNK_SRC_F'] = []
    v['CCLNK_TGT_F'] = v['CXXLNK_TGT_F'] = '/OUT:'

    v['LIB_ST']         = '%s.lib'
    v['LIBPATH_ST']     = '/LIBPATH:%s'
    v['STLIB_ST']       = '%s.lib'
    v['STLIBPATH_ST']   = '/LIBPATH:%s'

    v['cprogram_PATTERN']   = '%s.exe'
    v['cxxprogram_PATTERN'] = '%s.exe'

    v['cstlib_PATTERN']      = '%s.lib'
    v['cxxstlib_PATTERN']    = '%s.lib'

    v['cshlib_PATTERN']     = '%s.dll'
    v['cxxshlib_PATTERN']   = '%s.dll'

    v['LINKFLAGS_cshlib']   = ['/DLL']
    v['LINKFLAGS_cxxshlib'] = ['/DLL']

    # AR Tools
    v['ARFLAGS'] = ['/NOLOGO']
    v['AR_TGT_F'] = '/OUT:'

    # Delete the env variables so that they can be replaced with the clang versions
    del v['AR']
    del v['CC']
    del v['CXX']
    del v['LINK']
    conf.find_program('clang',    var='CC',   mandatory=False, silent_output=True)

    if not v['CC']:
        conf.mark_supported_platform_for_removal(PLATFORM)
        Logs.warn('clang executable not found, removing platform {}'.format(PLATFORM))
        return

    conf.find_program('clang++',  var='CXX',  silent_output=True)
    conf.find_program('llvm-lib', var='AR',   silent_output=True)
    conf.find_program('lld-link', var='LINK', silent_output=True)

    v['LINK_CC'] = v['LINK_CXX'] = v['LINK']

    clang_FLAGS = [
        '-mcx16',
        '-msse3',

        '-Wno-macro-redefined',
        '-Wno-nonportable-include-path',
        '-Wno-microsoft-cast',
        '-Wno-ignored-pragma-intrinsic', # Clang doens't need #pragma intrinsic anyway, so don't whine when one isn't recognized
    ]

    # Path to clang.exe is [clang]/bin/clang.exe, but the include path is [clang]/lib/clang/6.0.0/include
    clang_include_path = os.path.join(
        os.path.dirname(os.path.dirname(v['CXX'])),
        'lib', 'clang', '6.0.0', 'include')

    system_includes = [clang_include_path] + system_includes

    # Treat all MSVC include paths as system headers
    for include in system_includes:
        clang_FLAGS += ['-isystem', include]

    v['CFLAGS'] += clang_FLAGS
    v['CXXFLAGS'] += clang_FLAGS
    v['DEFINES'] += [
        '_CRT_SECURE_NO_WARNINGS',
        '_CRT_NONSTDC_NO_WARNINGS',
    ]
    v['LINKFLAGS'] += [
        '/MACHINE:x64',
        '/MANIFEST',            # Create a manifest file
        '/OPT:REF', '/OPT:ICF', # Always optimize for size, there's no reason not to
        '/LARGEADDRESSAWARE',   # tell the linker that the application can handle addresses larger than 2 gigabytes.
    ]

@conf
def load_debug_win_x64_win_x64_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_clang configurations for
    the 'debug' configuration
    """
    conf.load_debug_windows_settings()
    conf.load_win_x64_win_x64_clang_common_settings()

    conf.load_debug_clang_settings()

    conf.env['CXXFLAGS'] += [
        '-O0',
        '-gcodeview',
        '-gno-column-info',
        '-mllvm', '-emit-codeview-ghash-section',
    ]
    conf.env['LINKFLAGS'] += [
        '/debug:GHASH',
    ]

    # Load additional shared settings
    conf.load_debug_cryengine_settings()


@conf
def load_profile_win_x64_win_x64_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_clang configurations for
    the 'profile' configuration
    """
    conf.load_profile_windows_settings()
    conf.load_win_x64_win_x64_clang_common_settings()

    conf.load_profile_clang_settings()

    conf.env['CXXFLAGS'] += [
        '-O3',
        '-gcodeview',
        '-gno-column-info',
        '-mllvm', '-emit-codeview-ghash-section',
    ]
    conf.env['LINKFLAGS'] += [
        '/debug:GHASH',
    ]

    # Load additional shared settings
    conf.load_profile_cryengine_settings()


@conf
def load_performance_win_x64_win_x64_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_clang configurations for
    the 'performance' configuration
    """
    conf.load_performance_windows_settings()
    conf.load_win_x64_win_x64_clang_common_settings()

    conf.load_performance_clang_settings()

    conf.env['CXXFLAGS'] += [
        '-O3',
        '-gcodeview',
        '-gno-column-info',
        '-mllvm', '-emit-codeview-ghash-section',
    ]
    conf.env['LINKFLAGS'] += [
        '/debug:GHASH',
    ]

    # Load additional shared settings
    conf.load_performance_cryengine_settings()


@conf
def load_release_win_x64_win_x64_clang_settings(conf):
    """
    Setup all compiler and linker settings shared over all win_x64_win_x64_clang configurations for
    the 'release' configuration
    """
    conf.load_release_windows_settings()
    conf.load_win_x64_win_x64_clang_common_settings()

    conf.load_release_clang_settings()

    conf.env['CXXFLAGS'] += [
        '-O3',
        '-gline-tables-only'
    ]

    # Load additional shared settings
    conf.load_release_cryengine_settings()

@feature('c', 'cxx')
@after_method('apply_link')
@after_method('add_pch_to_dependencies')
def place_wall_first(self):
    for t in getattr(self, 'compiled_tasks', []):
        for flags_type in ('CFLAGS', 'CXXFLAGS'):
            flags = t.env[flags_type]
            try:
                wall_idx = flags.index('-Wall')
                flags[0], flags[wall_idx] = flags[wall_idx], flags[0]
            except ValueError:
                continue