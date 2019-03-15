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
import subprocess
import logging

from aztest.common import subprocess_with_timeout, SubprocessTimeoutException, clean_timestamp
from datetime import datetime

log = logging.getLogger(__name__)

PYTEST_RESULT_FILENAME = "pytest_results"
XML_EXTENSION = ".xml"
PYTEST_ARTIFACT_FOLDER_NAME = "Test_Results"


def run_pytest(known_args, extra_args):
    """
    Triggers test discovery and execution through pytest
    :param known_args: Arguments recognized by the parser and handled here
        - output_path: the location to save XML results
    :param extra_args: Additional arguments, passed directly to pytest
    :raises: CalledProcessError if pytest detects failures (returning a non-zero return code)
    :raises: SubprocessTimeoutException if pytest times out
    """
    timeout_sec = 1200  # 20 minutes
    if known_args.module_timeout:
        timeout_sec = int(known_args.module_timeout)
        print 'Setting module timeout to {0}'.format(known_args.module_timeout)
    else:
        print 'No timeout set, using default of 1200 seconds (20 minutes).'

    xunit_command = _get_xunit_command(known_args.output_path)
    argument_call = [sys.executable, "-B", "-m", "pytest", "--cache-clear", "-c",
                     "lmbr_test_pytest.ini", xunit_command]
    argument_call.extend(extra_args)
    log.info("Invoking pytest")
    log.info(argument_call)

    try:
        # raise on failure
        return_code = subprocess_with_timeout(argument_call, timeout_sec)
        if return_code != 0:
            raise subprocess.CalledProcessError(return_code, argument_call)
    except SubprocessTimeoutException as ste:
        log.error("Pytest timed out after {} seconds.".format(timeout_sec))
        raise ste

def _get_xunit_command(output_path):
    timestamp = clean_timestamp(datetime.now().isoformat())
    full_filename = "{}_{}{}".format(PYTEST_RESULT_FILENAME, timestamp, XML_EXTENSION)
    folder_name = "{}_{}".format(PYTEST_ARTIFACT_FOLDER_NAME, timestamp)
    output_file = os.path.join(output_path, full_filename)
    output_folder = os.path.join(output_path, folder_name)
    return "--junitxml={} --logs_path={}".format(output_file, output_folder)
