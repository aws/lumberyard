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

import aztest.common

from datetime import datetime

log = logging.getLogger(__name__)

PYTEST_RESULT_FILENAME = "pytest_results"
XML_EXTENSION = ".xml"


def run_pytest(known_args, extra_args):
    """
    Triggers test discovery and execution through pytest
    :param known_args: Arguments recognized by the parser and handled here
        - output_path: the location to save XML results
    :param extra_args: Additional arguments, passed directly to pytest
    :raises: CalledProcessError if pytest detects failures (returning a non-zero return code)
    """
    xunit_command = _get_xunit_command(known_args.output_path)
    argument_call = [sys.executable, "-m", "pytest", "-c", "lmbr_test_pytest.ini", xunit_command]
    argument_call.extend(extra_args)
    log.info("Invoking pytest")
    log.info(argument_call)

    # raise on failure
    subprocess.check_call(argument_call)


def _get_xunit_command(output_path):
    timestamp = aztest.common.clean_timestamp(datetime.now().isoformat())
    full_filename = "{}_{}{}".format(PYTEST_RESULT_FILENAME, timestamp, XML_EXTENSION)
    output_file = os.path.join(output_path, full_filename)
    return "--junitxml={}".format(output_file)
