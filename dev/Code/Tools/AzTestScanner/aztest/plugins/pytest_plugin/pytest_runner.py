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

import os
import sys
import logging
import aztest.log as lg

from aztest.common import subprocess_with_timeout, SubprocessTimeoutException, clean_timestamp
from datetime import datetime

log = logging.getLogger(__name__)

RESULT_XML_FILENAME = "pytest_results.xml"
ARTIFACT_FOLDER = "pytest_results"

def run_pytest(known_args, extra_args):
    """
    Triggers test discovery and execution through pytest
    :param known_args: Arguments recognized by the parser and handled here
    :param extra_args: Additional arguments, passed directly to pytest
    :raises: CalledProcessError if pytest detects failures (returning a non-zero return code)
    :raises: SubprocessTimeoutException if pytest fails to return within timeout
    """
    lg.setup_logging(level=known_args.verbosity)
    timeout_sec = known_args.module_timeout
    xunit_flags = _get_xunit_flags(known_args.output_path)
    argument_call = [sys.executable, "-B", "-m", "pytest", "--cache-clear", "-c",
                     "lmbr_test_pytest.ini"]
    argument_call.extend(xunit_flags)
    argument_call.extend(extra_args)
    log.info("Invoking pytest with a timeout of {} seconds".format(timeout_sec))
    log.info(argument_call)

    try:
        return_code = subprocess_with_timeout(argument_call, timeout_sec)
    except SubprocessTimeoutException:
        print("")  # force a newline after pytest output
        log.error("Pytest execution timed out after {} seconds".format(timeout_sec))
        sys.exit(1)  # exit on failure instead of raising, to avoid creating a confusing stacktrace
    else:
        if return_code != 0:
            log.error("Pytest reported failure with exit code: {}".format(return_code))
            sys.exit(return_code)  # exit on failure instead of raising, to avoid creating a confusing stacktrace


def _get_xunit_flags(output_path):
    timestamp = clean_timestamp(datetime.now().isoformat())
    current_folder = os.path.abspath(output_path)

    output_folder = os.path.join(current_folder,  # Absolute path where lmbr_test command is invoked OR user defined path if using --output-path arg in cmd.
                                 timestamp,  # Timestamp folder name in YYYY_MM_DDTHH_MM_SSSSS format.
                                 ARTIFACT_FOLDER)
    results_file = os.path.join(output_folder, RESULT_XML_FILENAME)

    log.info("Setting results folder to {}".format(output_folder))

    #Linux
    if sys.platform == "linux" or sys.platform == "linux2":
        lmbr_test_path = os.path.join(os.getcwd(), "lmbr_test.sh")
    #OS X
    elif sys.platform == "darwin":
        lmbr_test_path = os.path.join(os.getcwd(), "lmbr_test.sh")
    #Windows
    elif sys.platform == "win32":
        lmbr_test_path = os.path.join(os.getcwd(), "lmbr_test.cmd")

    #replacing double backslash with single forward slash.
    #python subprocess has a bug where it can't recognize file if path contains double backslash.
    lmbr_test_path = lmbr_test_path.replace(os.sep, '/')
    argument_call = [lmbr_test_path, "pysetup", "check", "ly_test_tools"]
    process = subprocess_with_timeout(argument_call, 120)

    #LyTestTools is installed if process returns 0.
    if process == 0:
        return ["--junitxml={}".format(results_file),
                "--output-path={}".format(output_folder)]
    else:
        return ["--junitxml={}".format(results_file),
                "--logs_path={}".format(output_folder)]

