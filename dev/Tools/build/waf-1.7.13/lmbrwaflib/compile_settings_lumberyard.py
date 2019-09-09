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
    
from waflib.Configure import conf
from cry_utils import append_kw_entry, prepend_kw_entry, append_to_unique_list, append_unique_kw_entry


@conf
def init_compiler_settings(conf):
    v = conf.env
    # Create empty env values to ensure appending always works
    v['DEFINES'] = []
    v['INCLUDES'] = []
    v['CXXFLAGS'] = []
    v['LIB'] = []
    v['LIBPATH'] = []
    v['LINKFLAGS'] = []
    v['BINDIR'] = ''
    v['LIBDIR'] = ''
    v['PREFIX'] = ''

    
@conf
def set_editor_flags(self, kw):

    if 'platforms' not in kw:
        append_kw_entry(kw, 'platforms', ['win', 'darwin'])
    if 'configurations' not in kw:
        append_kw_entry(kw, 'configurations', ['all'])
    if 'exclude_monolithic' not in kw:
        kw['exclude_monolithic'] = True

    kw['client_only'] = True

    prepend_kw_entry(kw,'includes',['.',
                                    self.CreateRootRelativePath('Code/Sandbox/Editor'),
                                    self.CreateRootRelativePath('Code/Sandbox/Editor/Include'),
                                    self.CreateRootRelativePath('Code/Sandbox/Plugins/EditorCommon'),
                                    self.CreateRootRelativePath('Code/CryEngine/CryCommon'),
                                    self.ThirdPartyPath('boost')])

    if 'priority_includes' in kw:
        prepend_kw_entry(kw,'includes',kw['priority_includes'])

    append_kw_entry(kw,'defines',['CRY_ENABLE_RC_HELPER',
                                  '_AFXDLL',
                                  '_CRT_SECURE_NO_DEPRECATE=1',
                                  '_CRT_NONSTDC_NO_DEPRECATE=1',
        ])

    append_kw_entry(kw,'win_defines',['WIN32'])


@conf
def set_rc_flags(self, kw, ctx):

    prepend_kw_entry(kw,'includes',['.',
                                    self.CreateRootRelativePath('Code/CryEngine/CryCommon'),
                                    self.ThirdPartyPath('boost'),
                                    self.CreateRootRelativePath('Code/Sandbox/Plugins/EditorCommon')])
    rc_defines = ['RESOURCE_COMPILER',
                  'FORCE_STANDARD_ASSERT',
                  '_CRT_SECURE_NO_DEPRECATE=1',
                  '_CRT_NONSTDC_NO_DEPRECATE=1']

    append_unique_kw_entry(kw, 'defines', rc_defines)
    append_unique_kw_entry(kw, 'win_defines', 'WIN32')



###############################################################################
@conf
def Settings(self, *k, **kw):
    if not kw.get('files', None) and not kw.get('file_list', None) and not kw.get('regex', None):
        self.fatal("A Settings container must provide a list of verbatim file names, a waf_files file list or a regex")

    return kw


COMPILE_SETTINGS = 'lumberyard'


def initialize_lumberyard(ctx):
    """
    Setup all platform, compiler and configuration agnostic settings
    """
    v = ctx.env

    if conf.is_option_true('enable_memory_tracking'):
        append_to_unique_list(v['DEFINES'], 'AZCORE_ENABLE_MEMORY_TRACKING')

    # BEGIN JAVELIN MOD: https://jira.agscollab.com/browse/JAV-18779 Allows for an AZ Allocator to be used for memory management
    # removed below if check because of issues related to https://jira.agscollab.com/browse/LYAJAV-126
    # if conf.is_option_true('use_az_allocator_for_cry_memory_manager'):
    #    append_to_unique_list(v['DEFINES'], 'USE_AZ_ALLOCATOR_FOR_CRY_MEMORY_MANAGER')
    v['DEFINES'] += ['USE_AZ_ALLOCATOR_FOR_CRY_MEMORY_MANAGER']
    # END JAVELIN MOD

    # To allow pragma comment (lib, 'SDKs/...) uniformly, pass Code to the libpath
    append_to_unique_list(v['LIBPATH'], conf.CreateRootRelativePath('Code'))
    return True


@conf
def load_lumberyard_settings(ctx, config, is_test, is_server):

    if not initialize_lumberyard(ctx):
        return False

    # Apply compile settings from the config
    ctx.apply_compile_settings(COMPILE_SETTINGS, config)

    # Apply optional 'test' and 'server' settings
    if is_test:
        ctx.load_test_settings()
    if is_server:
        ctx.load_server_settings()
    return True


@conf
def apply_restricted_platform_environments(ctx):
    """
    # Place holder conf function so that restricted platforms can have post events trigger if they are registered
    :param ctx: Context
    """
    pass
