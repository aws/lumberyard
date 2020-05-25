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

# System Imports
import os

# waflib imports
from waflib import Logs, Utils, Errors, Build
from waflib.Context import BOTH


warned_about_missing_doxygen = False
        
class BuildDoxygen(Build.BuildContext):
    cmd = 'doxygen'
    fun = 'build'
    
    def prepare(self):
        # Skip during project generation
        if self.env['PLATFORM'] == 'project_generator':
            return False

        # don't bother to try this on non-windows
        host = Utils.unversioned_sys_platform()
        if host not in ('win_x64', 'win32'):
            return False

        # if not self.is_option_true('use_doxygen'):
        #     return False

        global warned_about_missing_doxygen
        (output, errors) = self.cmd_and_log('where doxygen', output=BOTH, quiet=BOTH)
        binary = output.strip()
        if not os.path.exists(binary):
            if not warned_about_missing_doxygen:
                Logs.debug('doxygen: No doxygen binary found, skipping doxygen doc creation')
                warned_about_missing_doxygen = True
            return False

        self.binary = binary
        self.doxyfile = self.root.find_node('Docs/doxygen/doxyfile-ly-api')
        if not self.doxyfile:
            Logs.debug('doxygen: No doxyfile found, skipping doxygen doc creation')
            return False
        self.doxyfile = self.doxyfile.abspath()
        return True

    def execute(self):
        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()

        self.load_user_settings()
        if not self.prepare():
            return
        self.exec_doxygen()
        
    def exec_doxygen(self):
        """
        Execute doxygen with argument string
        :return: True on success, False on failure
        """
        cwd = os.path.dirname(self.doxyfile)
        command = '"{}" "{}"'.format(self.binary, os.path.basename(self.doxyfile))
        Logs.info("[WAF] Executing 'doxygen': {} in {}".format(command, cwd))

        try:
            (output, error_output) = self.cmd_and_log(command, output=BOTH, quiet=BOTH, cwd=cwd)
            if error_output and not error_output.isspace():
                self.fatal('doxygen: doxygen output to stderr even though it indicated success. Output:\n{}\nErrors:\n{}'.format(output, error_output))
        except Errors.WafError as e:
            Logs.warn('doxygen: doxygen execution failed! Output:\n{}'.format(e))