#!/usr/bin/env python
# encoding: utf-8

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

from waflib.Configure import conf
from waflib.TaskGen import feature, after_method, before_method
from waflib import Task, Logs, Utils, Errors
from waflib.Context import BOTH
import os

warned_about_missing_crcfix = False

@feature('crcfix')
@after_method('apply_incpaths')
def create_crcfix_tasks(self):
    # Skip during project generation
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    # don't bother to try this on non-windows
    host = Utils.unversioned_sys_platform()
    if host not in ('win_x64', 'win32'):
        return

    if not self.bld.is_option_true('use_crcfix'):
        return

    global warned_about_missing_crcfix
    binary = os.path.join(self.bld.env['CRCFIX_PATH'][0], self.bld.env['CRCFIX_EXECUTABLE'])
    if not os.path.exists(binary):
        if not warned_about_missing_crcfix:
            Logs.debug('crcfix: No crcfix binary found, skipping crcfix task creation')
            warned_about_missing_crcfix = True
        return

    src_node = self.path.get_src()
    paths = []
    inputs = []
    for ext in ('h', 'cpp', 'hxx', 'hpp', 'cxx', 'c', 'inl'):
        paths.append('"' + os.path.join(src_node.abspath(), '*', '*.' + ext) + '"')

    inputs = self.to_nodes(self.source) + self.to_nodes(self.header_files)

    task = self.create_task('crcfix', inputs)
    task.prepare(binary=binary, input_paths=paths)

class crcfix(Task.Task):
    color = 'CYAN'

    def __init__(self, *k, **kw):
        super(crcfix, self).__init__(*k, **kw)

    def prepare(self, *k, **kw):
        self.binary = kw['binary']
        self.input_paths = kw['input_paths']

    def run(self):
        args = ' '.join(self.input_paths)
        success = self.exec_crcfix(args)
        return 0 if success else 1

    def exec_crcfix(self, args):
        """
        Execute the code generator with argument string
        :return: True on success, False on failure
        """
        command = '\"' + self.binary + '\" ' + args
        Logs.debug('crcfix: Invoking crcfix with command: {}'.format(command))

        try:
            (output, error_output) = self.generator.bld.cmd_and_log(command, output=BOTH, quiet=BOTH)
            if error_output and not error_output.isspace():
                Logs.error('crcfix: crcfix output to stderr even though it indicated success. Output:\n{}\nErrors:\n{}'.format(output, error_output))
                return False
        except Errors.WafError as e:
            Logs.warn('crcfix: crcfix execution failed! Output:\n{}'.format(e))
            return True # do not fail the build, this is a warning

        return True
        