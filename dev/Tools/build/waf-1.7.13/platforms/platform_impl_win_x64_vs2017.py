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
from waflib import Configure, Logs, Errors, TaskGen

# lmbrwaflib imports
from lmbrwaflib import msvs

MSVS_VERSION = 15
VS_NAME = 'vs2017'
PLATFORM_NAME = 'win_x64_{}'.format(VS_NAME)

@TaskGen.feature('deploy_win_x64_vs2017')
def deploy_win_x64_vs2017(tg):
    # No deployment phase for windows yet
    pass
    
    
@TaskGen.feature('unittest_win_x64_vs2017')
@TaskGen.after_method('deploy_win_x64_vs2017')
def unittest_win_x64_vs2017(tg):

    tg.bld.run_unittest_launcher_for_win_x64(tg.bld.project)


class msvs_2017_generator(msvs.msvs_generator):
    '''
    Command to generate a Visual Studio 2017 solution
    The project spec (-p) option can be be used to generate a solution which targets a specific build spec and name the
    solution with the project spec name
    The vs2017_solution_name(--vs2017-solution-name) option overrides the output name of the solution from
    the user_settings.options file
    The specs_to_include_in_project_generation(--specs-to-include-in-project-generation) override the list of specs to
    include solution from the user_settings.options file
    '''
    cmd = 'msvs_2017'
    fun = 'build'

    def __init__(self):
        super(msvs_2017_generator, self).__init__()
        self.platform_name = PLATFORM_NAME
        self.msvs_version = MSVS_VERSION
        self.vs_solution_name_options_attribute = 'vs2017_solution_name'
        self.vs_name = VS_NAME

    def get_msbuild_toolset_properties_file_path(self, toolset_version, toolset_name):

        ms_build_root = os.path.join(self.vs_installation_path, 'MSBuild', '{}.0'.format(toolset_version))
        platform_props_file = os.path.join(ms_build_root, 'Microsoft.common.props')

        # if the vs_installation_path is within in the C:\Program Files (x86)\Microsoft Visual Studio\2019\ directory
        # due to the v141 build tools being used with VS2019, the MSBuild file is stored in the
        # <VSInstallationPath>/MSBuild/Current/ directory
        if not os.path.isfile(platform_props_file):
            ms_build_root = os.path.join(self.vs_installation_path, 'MSBuild', 'Current')
            platform_props_file = os.path.join(ms_build_root, 'Microsoft.common.props')
        return platform_props_file


@Configure.conf
def detect_visual_studio_2017(ctx, winkit_version, vcvarsall_args, fallback_to_newer_vs_version):

    install_path = ''
    path_vcvars_all= ''
    vs2017_exception_text = ''
    try:
        install_path = ctx.query_visual_studio_install_path(MSVS_VERSION, ctx.options.win_vs2017_vswhere_args)
        path_vcvars_all = os.path.normpath(os.path.join(install_path, 'VC\\Auxiliary\\Build\\vcvarsall.bat'))
        if not os.path.isfile(path_vcvars_all):
            raise Errors.WafError("Unable to detect VS2017. Cannot locate vcvarsall.bat at '{}'".format(path_vcvars_all))
    except Errors.WafError as e:
        vs2017_exception_text = str(e)
        if not fallback_to_newer_vs_version:
            raise Errors.WafError(vs2017_exception_text)

    try:
        if vs2017_exception_text:
            # If VS2017 cannot be found using vswhere, try detecting VS2019 next and using the 14.1x toolset if it is installed
            install_path = ctx.query_visual_studio_install_path(MSVS_VERSION, ctx.options.win_vs2019_vswhere_args)
            path_vcvars_all = os.path.normpath(os.path.join(install_path, 'VC\\Auxiliary\\Build\\vcvarsall.bat'))
            # Append the '-vcvars_ver=14.1' value vcvarsall_args to detect the v141 build tools for VS2019
            # If the vcvarsall_args have a -vcvars_ver entry is already set, this will override that value
            vcvarsall_args = vcvarsall_args + " -vcvars_ver=14.1"
        if not os.path.isfile(path_vcvars_all):
            raise Errors.WafError("Unable to detect VS2019. Cannot locate vcvarsall.bat at '{}'".format(path_vcvars_all))
    except Errors.WafError as e:
        # Re-raise the Errors.WafError exception but with the message from the VS2017 detection exception
        raise Errors.WafError(vs2017_exception_text)

    dev_studio_env = ctx.detect_visual_studio(platform_name=PLATFORM_NAME,
                                              path_visual_studio=install_path,
                                              path_vcvars_all=path_vcvars_all,
                                              winkit_version=winkit_version,
                                              vcvarsall_args=vcvarsall_args)

    ctx.apply_dev_studio_environment(dev_studio_env_map=dev_studio_env,
                                     env_keys_to_apply=['INCLUDES',
                                                        'PATH',
                                                        'LIBPATH',
                                                        'CC',
                                                        'CXX',
                                                        'LINK',
                                                        'LINK_CC',
                                                        'LINK_CXX',
                                                        'WINRC',
                                                        'MT',
                                                        'AR',
                                                        'VS_INSTALLATION_PATH'])

    ctx.env['MSVC_VERSION'] = MSVS_VERSION
    return dev_studio_env


