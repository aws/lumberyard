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
from waflib.TaskGen import feature, after_method
from waflib import Logs


LAUNCHER_SCRIPT='''#!/bin/bash
PATTERN="%e.%t.coredump"
CURRENT_VALUE=`cat /proc/sys/kernel/core_pattern`

if [ "$CURRENT_VALUE" != "%e.%t.coredump" ]; then
    xterm -T "Update Core Dump FileName" -e sudo sh -c "echo %e.%t.coredump > /proc/sys/kernel/core_pattern"
fi

ulimit -c unlimited
xterm -T "${project.to_launch_executable} Launcher" -hold -e './${project.to_launch_executable}'
'''


# Function to generate the copy tasks for build outputs
@feature('cprogram', 'cxxprogram')
@after_method('apply_flags_msvc')
@after_method('apply_link')
def add_linux_launcher_script(self):
    if not getattr(self, 'create_linux_launcher', False):
        return

    if self.env['PLATFORM'] == 'project_generator':
        return

    if not getattr(self, 'link_task', None):
        self.bld.fatal('Linux Launcher is only supported for Executable Targets')

    # Write to rc file if content is different
    for node in self.bld.get_output_folders(self.bld.env['PLATFORM'],self.bld.env['CONFIGURATION']):

        node.mkdir()

        for project in self.bld.get_enabled_game_project_list():
            # Set up values for linux launcher script template
            linux_launcher_script_file = node.make_node('Launch_'+self.bld.get_executable_name(project)+'.sh')
            self.to_launch_executable = self.bld.get_executable_name(project)


            linux_launcher_script_content = LAUNCHER_SCRIPT.replace('${project.to_launch_executable}', self.to_launch_executable)

            if not os.path.exists(linux_launcher_script_file.abspath()) or linux_launcher_script_file.read() != linux_launcher_script_content:
                Logs.info('Updating Linux Launcher Script (%s)' % linux_launcher_script_file.abspath() )
                linux_launcher_script_file.write(linux_launcher_script_content)
