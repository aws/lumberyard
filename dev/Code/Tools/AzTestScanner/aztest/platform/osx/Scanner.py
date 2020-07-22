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

import subprocess
import logging as lg
import os
import time
from aztest.common import SubprocessTimeoutException

logger = lg.getLogger(__name__)


class Executor:
    def __init__(self):
        self.last_exit_code = 0

    def run_cmd_in_subprocess(self, cmd, cwd=None, chain=None, timeout=None):
        """
        Run a command on OSX in a subprocess, optionally chain stdout to a second command
        :type cmd: list
        :type cwd: None or str
        :type timeout: time out on the subprocess in seconds
        :return: generator, output lines
        """
        deadline = None
        if timeout:
            deadline = time.time() + timeout

        logger.info("run cmd=" + ', '.join(cmd) + " cwd={}".format(cwd or "<none>"))

        p0 = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cwd)
        proc = p0

        if chain:
            p1 = subprocess.Popen(chain, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=p0.stdout, cwd=cwd)
            proc = p1

        while True:
            if timeout and time.time() > deadline:
                proc.kill()  # Required in linux, since terminate() is different

                raise SubprocessTimeoutException("Subprocess timed out after {} seconds. If this is too short, "
                                                 "change it in the command line arguments.".format(timeout))

            line = proc.stdout.readline(400).decode('utf8')  # limit to 100 characters to avoid deadlocks
            if line != '':
                yield line.strip()
                # force the loop to read the next line
                continue
            
            proc.poll()

            if proc.returncode is not None:
                self.last_exit_code = proc.returncode
                logger.info("AZTestRunner returned code: {}".format(self.last_exit_code))
                return

            time.sleep(0.001)


class Scanner:
    __runner_exe__ = 'AzTestRunner'

    def __init__(self):
        self.executor = Executor()

    def enumerate_modules(self, dir):
        # find only the .dylibs
        cmd = ['find', dir, '-type', 'f', '-name', '*.dylib']
        for f in self.executor.run_cmd_in_subprocess(cmd):
            yield f

    def enumerate_executables(self, dir):
        # this command will finds all executables including dynamic libraries
        # so filter out the dynamic libraries
        cmd = ['find', dir, '-type', 'f', '-perm', '+1']
        for path in self.executor.run_cmd_in_subprocess(cmd):
            if not path.endswith('.dylib') and not path.endswith('.dll'):
                yield path

    def call(self, filename, method, runner_path, args, timeout):
        try:
            dir, fn = os.path.split(filename)

            # run and log output
            for l in self.executor.run_cmd_in_subprocess([runner_path, fn, method] + args, cwd=dir, timeout=timeout):
                logger.info(l)

            return self.executor.last_exit_code

        except Exception as ex:
            logger.exception()

    def run(self, filename, args, timeout):
        try:
            dir, fn = os.path.split(filename)
            fn = "./" + fn  # fix up the filename so it can be executed on osx

            # run and log output
            for l in self.executor.run_cmd_in_subprocess([fn] + args, cwd=dir, timeout=timeout):
                logger.info(l)

            return self.executor.last_exit_code

        except Exception as ex:
            logger.exception()

    def exports_symbol(self, filename, symbol):
        # C prefix symbol
        prefixed_symbol = "_" + symbol

        cmd = ['nm', '-j', filename]
        chain = ['grep', prefixed_symbol]

        found = False
        for l in self.executor.run_cmd_in_subprocess(cmd, chain=chain):
            if prefixed_symbol == l:
                return True

        return False

