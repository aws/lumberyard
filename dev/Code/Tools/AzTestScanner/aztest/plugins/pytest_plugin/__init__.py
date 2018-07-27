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

import pytest_runner
from aztest.common import DEFAULT_OUTPUT_PATH


def aztest_add_subparsers(subparsers):
    pytest_subparser = subparsers.add_parser(
        "pytest", description="runs only python tests through py.test, forwarding all arguments except --output-path"
                              " to pytest",
        help='runs only python tests through py.test',  # help for parent parser
        add_help=False,  # suppress subparser-targeted help to pass help argument on to pytest
        usage="%(prog)s [--output-path PATH] <pytest-args ...>"  # help for --output-path errors
    )
    pytest_subparser.add_argument(
        '--output-path', required=False, default=DEFAULT_OUTPUT_PATH
    )
    pytest_subparser.set_defaults(func=pytest_runner.run_pytest)
