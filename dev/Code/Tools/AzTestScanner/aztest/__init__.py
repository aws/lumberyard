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
import logging

import aztest.plugins as plugins

from aztest.common import DEFAULT_OUTPUT_PATH
from aztest.scanner import scan
from aztest.report.HTMLReporter import generate_standalone_report

logger = logging.getLogger(__name__)


def get_parser():
    parser = argparse.ArgumentParser(description="AZ Test Scanner")
    parser.add_argument('--verbosity', '-v', default='INFO', choices=logging._levelNames,
                        help="set the verbosity of logging")
    subparsers = parser.add_subparsers()
    # adding plugins subparsers first allows AzTest to clobber any conflicting options, instead of vice versa
    plugins.subparser_hook(subparsers)

    p_scan = subparsers.add_parser("scan", help="scans a directory for modules to test and executes them",
                                   epilog="Extra parameters are assumed to be for the test framework and will be "
                                          "passed along to the modules/executables under test. Returns 1 if any "
                                          "module has failed (for any reason), or zero if all tests in all modules "
                                          "have passed.")
    p_scan.add_argument('--dir', '-d', required=True, help="root directory to scan")
    p_scan.add_argument('--runner-path', '-r', required=False,
                        help="path to the test runner, default is {dir}/AzTestRunner(.exe)")
    p_scan.add_argument('--add-path', required=False, action="append",
                        help="path(s) to libraries that are required by modules under test")
    p_scan.add_argument('--output-path', required=False, default=DEFAULT_OUTPUT_PATH,
                        help="sets the path for output folders, default is dev/TestResults")
    p_scan.add_argument('--html-report', required=False, action="store_true",
                        help="[DEPRECATED] Enabled by default. html reports will be automatically generated "
                             "(Use --no-html-report to turn html report generation off)")
    p_scan.add_argument('--no-html-report', required=False, action="store_true",
                        help="if set, no HTML report is generated at the end of the scan")
    p_scan.add_argument('--integ', '-i', required=False, action="store_true",
                        help="if set, runs integration tests instead of unit tests")
    p_scan.add_argument('--suite', '-s', required=False,
                        help="Restrict the set of tests run to only be a specific suite")
    p_scan.add_argument('--no-timestamp', required=False, action="store_true",
                        help="if set, removes timestamp from output files")
    p_scan.add_argument('--wait-for-debugger', required=False, action="store_true",
                        help="if set, tells the AzTestRunner to wait for a debugger to be attached before continuing")
    p_scan.add_argument('--bootstrap-config', required=False,
                        help="json configuration file to bootstrap applications for modules which require them")
    p_scan.add_argument('--module-timeout', type=int, required=False, default=300,
                        help='Duration in seconds before aborting and marking a test module as a failure')

    g_filters = p_scan.add_argument_group('Filters', "Arguments for filtering what is scanned")
    g_filters.add_argument('--limit', '-n', required=False, type=int,
                           help="limit number of modules to scan")
    g_filters.add_argument('--only', '-o', required=False,
                           help="scan only the file(s) named in this argument (comma separated), matches using "
                                "'endswith'")
    g_filters.add_argument('--whitelist-file', required=False, action="append", metavar='FILE', dest="whitelist_files",
                           help="path to a generic, newline-delimited file used for whitelisting modules")
    g_filters.add_argument('--blacklist-file', required=False, action="append", metavar='FILE', dest="blacklist_files",
                           help="path to a generic, newline-delimited file used for blacklisting modules")
    g_filters.add_argument('--exe', required=False, action="store_true",
                           help="if set, scans executables as well as shared modules")

    p_scan.set_defaults(func=scan)

    p_report = subparsers.add_parser("report", help="generates an HTML report based on a directory of XML files")
    p_report.add_argument('--dir', '-d', required=False, default=DEFAULT_OUTPUT_PATH,
                          help="test result directory to generate report from (if given directory has list of "
                               "directories instead of XML files, this will search the newest directory for the files),"
                               " default is dev/TestResults/{newest}")
    p_report.add_argument("--output-path", required=False,
                          help="sets the path for the report(s), default is to put them in the same directory as the "
                               "XML files")

    p_report.set_defaults(func=generate_standalone_report)

    return parser


def execute(args=None):
    parser = get_parser()

    known_args, extra_args = parser.parse_known_args(args)

    # execute command
    return known_args.func(known_args, extra_args)
