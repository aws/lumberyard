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
import argparse
import sys

import pytest_runner
from aztest.common import DEFAULT_OUTPUT_PATH

DEFAULT_TIMEOUT = 3600  # one hour of seconds


def aztest_add_subparsers(subparsers):
    pytest_subparser = subparsers.add_parser(
        "pytest", description="Run only python tests through py.test, forwarding any additional arguments to pytest",
        help='run only python tests through py.test',  # help for parent parser
        add_help=False,  # suppress subparser-targeted help to pass help argument on to pytest
        usage="%(prog)s [-h] [--module-timeout TIMEOUT] [--output-path PATH] <pytest-args ...>"
    )

    class CustomHelpCallback(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            pytest_subparser.print_help()
            sys.exit(0)
    pytest_subparser.add_argument(
        '-h', nargs=0, action=CustomHelpCallback,
        help="Show this show this help message and exit. Note that argument '--help' is passed on to pytest, "
             "and will show its internal help"
    )
    pytest_subparser.add_argument(
        '--output-path', default=DEFAULT_OUTPUT_PATH,
        help="sets the path for result output"
    )
    pytest_subparser.add_argument(
        '--module-timeout', type=int, default=DEFAULT_TIMEOUT,
        help='Duration in seconds before aborting and marking the entire test run as a failure'
    )
    pytest_subparser.set_defaults(func=pytest_runner.run_pytest)
