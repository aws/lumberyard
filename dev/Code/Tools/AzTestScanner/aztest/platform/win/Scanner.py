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

import fnmatch
import logging as lg
import os
import subprocess
logger = lg.getLogger(__name__)


def run(cmd, cwd=None):
    """
    Run a command on Windows in a subprocess
    :type cmd: list of arguments to run, first should be the executable
    :type cwd: directory to execute from, or null
    :return: return code from process
    """

    # https://docs.python.org/2/library/subprocess.html?highlight=subprocess#subprocess.Popen
    # "If cwd is not None, the child's current directory will be changed to cwd before it
    # is executed. Note that this directory is not considered when searching the executable,
    # so you can't specify the program's path relative to cwd."
    if not os.path.isabs(cmd[0]):
        cmd = (os.path.join(cwd, cmd[0]),) + cmd[1:]

    logger.info("run cmd=[" + ', '.join(cmd) + "], cwd={}".format(cwd or "<none>"))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cwd)

    line = proc.stdout.readline()
    while not line == '':
        logger.info(line.strip())
        line = proc.stdout.readline()

    proc.wait()
    return proc.returncode


class Scanner:
    __runner_exe__ = 'AzTestRunner.exe'

    def __init__(self):
        pass

    def enumerate_by_extension(self, starting_dir, filter):
        for root, dirnames, filenames in os.walk(starting_dir):
            for filename in fnmatch.filter(filenames, filter):
                p = os.path.abspath(os.path.join(root, filename))
                yield p

    def enumerate_modules(self, starting_dir):
        return self.enumerate_by_extension(starting_dir, "*.dll")

    def enumerate_executables(self, starting_dir):
        return self.enumerate_by_extension(starting_dir, "*.exe")

    def call(self, filename, method, runner_path, args):
        working_dir, fn = os.path.split(filename)

        # run tests in the DLL using a special launcher program
        return run([runner_path, fn, method] + args, cwd=working_dir)

    def run(self, filename, args):
        # run tests in an executable
        working_dir, _ = os.path.split(filename)

        # run tests in an executable by directly invoking
        # it with '--unittest' argument as the first parameter
        return run([filename, '--unittest'] + args, cwd=working_dir)

    def bootstrap(self, working_dir, commandline):
        """
        :param commandline: tuple, full commandline to execute
        :return:
        """
        if not working_dir:  # use cwd if not specified in app
            working_dir = os.getcwd()
        elif not os.path.isabs(working_dir):
            working_dir = os.path.join(os.getcwd(), working_dir)

        return run(commandline, cwd=working_dir)

    def exports_symbol(self, filename, symbol):
        return True
