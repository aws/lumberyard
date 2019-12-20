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
from waflib import Errors, Logs, TaskGen, Configure

import os
import subprocess


@Configure.conf
def run_unittest_launcher_for_win_x64(ctx, game_project_name):
    """
    Helper context function to execute the unit test launcher for a specific game project

    :param ctx:                 Context
    :param game_project_name:   The current project name (extracted from bootstrap.cfg)

    """
    
    output_folder = ctx.get_output_folders(ctx.platform, ctx.config)[0]
    current_project_launcher = ctx.env['cprogram_PATTERN'] % '{}Launcher'.format(game_project_name)
    current_project_unittest_launcher_fullpath = os.path.join(output_folder.abspath(), current_project_launcher)
    if not os.path.isfile(current_project_unittest_launcher_fullpath):
        raise Errors.WafError("Unable to launch unit tests for project '{}'. Cannot find launcher file '{}'. Make sure the project has been built successfully.".format(game_project_name, current_project_unittest_launcher_fullpath))
    Logs.info('[WAF] Running unit tests for {}'.format(game_project_name))
    
    try:
        call_args = [current_project_unittest_launcher_fullpath]
        
        # Grab any optional arguments
        auto_launch_unit_test_arguments = ctx.get_settings_value('auto_launch_unit_test_arguments')
        if auto_launch_unit_test_arguments:
            call_args.extend(auto_launch_unit_test_arguments.split(' '))

        result_code = subprocess.call(call_args)
    except Exception as e:
        raise Errors.WafError("Error executing unit tests for '{}': {}".format(game_project_name, e))
    if result_code != 0:
        raise Errors.WafError("Unit tests for '{}' failed. Return code {}".format(game_project_name, result_code))
    else:
        Logs.info('[WAF] Running unit tests for {}'.format(game_project_name))
        
