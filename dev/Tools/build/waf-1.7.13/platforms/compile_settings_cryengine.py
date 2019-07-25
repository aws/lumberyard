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
from cry_utils import append_kw_entry, prepend_kw_entry, append_to_unique_list
from waf_branch_spec import LUMBERYARD_COPYRIGHT_YEAR
from waflib import Options
import os


@conf
def load_cryengine_common_settings(conf):
    """
    Setup all platform, compiler and configuration agnostic settings
    """
    v = conf.env

    if conf.is_option_true('enable_memory_tracking'):
        append_to_unique_list(v['DEFINES'], 'AZCORE_ENABLE_MEMORY_TRACKING')
    
    # To allow pragma comment (lib, 'SDKs/...) uniformly, pass Code to the libpath
    append_to_unique_list(v['LIBPATH'], conf.CreateRootRelativePath('Code'))

    # Set the product sku define, default is 'default'
    v['DEFINES'] += ['PRODUCT_SKU_' + Options.options.product_sku]

#############################################################################
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
                                    self.CreateRootRelativePath('Code/CryEngine/CryCommon') ,
                                    self.ThirdPartyPath('boost')])

    if 'priority_includes' in kw:
        prepend_kw_entry(kw,'includes',kw['priority_includes'])

    append_kw_entry(kw,'defines',['CRY_ENABLE_RC_HELPER',
                                  '_CRT_SECURE_NO_DEPRECATE=1',
                                  '_CRT_NONSTDC_NO_DEPRECATE=1',
                                  'LUMBERYARD_COPYRIGHT_YEAR="{}"'.format(LUMBERYARD_COPYRIGHT_YEAR),
        ])

    append_kw_entry(kw,'win_defines',['WIN32'])

###############################################################################
@conf   
def set_rc_flags(self, kw, ctx):

    prepend_kw_entry(kw,'includes',['.',
                                    self.CreateRootRelativePath('Code/CryEngine/CryCommon'),
                                    self.ThirdPartyPath('boost'),
                                    self.CreateRootRelativePath('Code/Sandbox/Plugins/EditorCommon')])
    compileDefines =  ['RESOURCE_COMPILER',
                   'FORCE_STANDARD_ASSERT',
                   '_CRT_SECURE_NO_DEPRECATE=1',
                   '_CRT_NONSTDC_NO_DEPRECATE=1']

    if ctx.is_windows_platform(ctx.env['PLATFORM']):
        compileDefines.append('WIN32')


    append_kw_entry(kw,'defines', compileDefines)

###############################################################################
@conf
def set_pipeline_flags(self, kw, ctx):

    prepend_kw_entry(kw,'includes',['.',
                                   self.CreateRootRelativePath('Code/CryEngine/CryCommon')])

    append_kw_entry(kw,'defines',['RESOURCE_COMPILER',
                                  'FORCE_STANDARD_ASSERT',
                                  '_CRT_SECURE_NO_DEPRECATE=1',
                                  '_CRT_NONSTDC_NO_DEPRECATE=1'])

    if ctx.is_windows_platform(ctx.env['PLATFORM']):
        append_kw_entry(kw,'defines',['WIN32'])

###############################################################################
@conf
def Settings(self, *k, **kw):
    if not kw.get('files', None) and not kw.get('file_list', None) and not kw.get('regex', None):
        self.fatal("A Settings container must provide a list of verbatim file names, a waf_files file list or an regex")

    return kw

