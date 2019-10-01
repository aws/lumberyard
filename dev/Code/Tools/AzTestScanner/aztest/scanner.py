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

import colorama
import logging as lg
import ntpath
import os
import sys
import json
import xml.etree.ElementTree as ET
from aztest.common import create_output_directory, create_xml_output_filename, ModuleType, MODULE_UNIT_EXPORT_SYMBOL, \
    MODULE_INTEG_EXPORT_SYMBOL, EXECUTABLE_EXPORT_SYMBOL, ScanResult, SubprocessTimeoutException
from aztest.filters import FileApprover, get_default_blacklist, get_default_whitelist
from aztest.log import setup_logging
from aztest.report import HTMLReporter, XMLGenerator
from aztest.errors import RunnerReturnCodes
from bootstrap import BootstrapConfig
from colorama import Fore
from colorama import Style

logger = lg.getLogger(__name__)

if sys.platform.startswith("darwin"):
    from aztest.platform.osx import Scanner
elif sys.platform.startswith("win32"):
    from aztest.platform.win import Scanner
elif sys.platform.startswith("linux"):
    from aztest.platform.linux import Scanner
else:
    logger.error("Unrecognized platform: {}".format(sys.platform))
    exit()

__no_dll__ = False

TESTS_RUN_KEY = "tests_run"
TESTS_PASSED_KEY = "tests_passed"
TESTS_FAILED_KEY = "tests_failed"
TESTS_ERRORED_KEY = "tests_errored"
TESTS_SKIPPED_KEY = "tests_skipped"
TOTAL_TIME_TAKEN_KEY = "total_time_taken"


def add_dirs_to_path(path_dirs):
    if path_dirs:
        os.environ["PATH"] += os.pathsep + os.pathsep.join([os.path.abspath(path_dir) for path_dir in path_dirs])


def scan_one(args, extra, type_, scanner, runner_path, bootstrap_config, file_name, output_dir):
    """ Scan one module or executable

    :param args: command line arguments
    :param extra: extra parameters
    :param int type_: module or executable
    :param scanner: platform-specific scanner instance
    :param runner_path: path to test runner executable
    :param BootstrapConfig bootstrap_config: configuration object for bootstrapping modules
    :param file_name: filename of module to scan
    :param output_dir: directory for output
    :return: ScannerResult
    """

    logger.info("{}: {}".format(type_, file_name))
    xml_out = create_xml_output_filename(file_name, output_dir)
    if os.path.exists(xml_out):
        return  # module has already been tested

    # for a more exhaustive list of options:
    # https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#running-a-subset-of-the-tests
    # --help                lists command options
    # --gtest_list_tests    just list what tests are in the module
    # --gtest_shuffle       shuffle test ordering
    cmd_args = ["--gtest_output=xml:" + xml_out,
                "--gtest_color=yes"]

    cmd_args += extra

    if args.wait_for_debugger:
        # user has requested to attach a debugger when running
        cmd_args += ["--wait-for-debugger"]

    ret = 0

    if type_ == ModuleType.LIBRARY:
        if args.integ:
            # user wants to run integration tests
            export_symbol = MODULE_INTEG_EXPORT_SYMBOL
            cmd_args += ["--integ"]
        else:
            # just run unit tests
            export_symbol = MODULE_UNIT_EXPORT_SYMBOL

        if args.suite:
            cmd_args += ["--suite"]
            cmd_args += [args.suite]

        # run with bootstrapper
        ran_with_bootstrapper = False
        if bootstrap_config:
            module_name = os.path.split(file_name)[1]
            bootstrapper = bootstrap_config.get_bootstrapper(module_name)
            if bootstrapper:
                ran_with_bootstrapper = True
                try:
                    working_dir = args.dir
                    app = os.path.join(args.dir, bootstrapper.command_line[0])
                    if not os.path.isfile(app):
                        logger.error("bootstrap executable not found {}".format(app))

                    full_command_line = bootstrapper.command_line + tuple(cmd_args)
                    ret = scanner.bootstrap(working_dir, full_command_line)

                except:
                    ret = RunnerReturnCodes.UNEXPECTED_EXCEPTION
                    logger.exception("bootstrap failed")

        # run with "runner_<platform>" as the implicit bootstrapper (no need to specify this
        # in the bootstrapper config file)
        if not ran_with_bootstrapper:
            if scanner.exports_symbol(file_name, export_symbol):
                try:
                    ret = scanner.call(file_name, export_symbol, runner_path, args=cmd_args, timeout=args.module_timeout)
                    logger.debug("Code returned from call: {}".format(ret))

                except KeyboardInterrupt:
                    raise

                except SubprocessTimeoutException as ste:
                    ret = RunnerReturnCodes.MODULE_TIMEOUT
                    logger.exception(ste.message)

                except:
                    ret = RunnerReturnCodes.UNEXPECTED_EXCEPTION
                    logger.exception("module call failed")

            else:
                ret = RunnerReturnCodes.SYMBOL_NOT_FOUND

    elif type_ == ModuleType.EXECUTABLE:
        if scanner.exports_symbol(file_name, EXECUTABLE_EXPORT_SYMBOL):
            try:
                cmd_args = ["--unittest"] + cmd_args
                ret = scanner.run(file_name, args=cmd_args, timeout=args.module_timeout)

            except KeyboardInterrupt:
                raise

            except SubprocessTimeoutException as ste:
                ret = RunnerReturnCodes.MODULE_TIMEOUT
                logger.exception(ste.message)

            except:
                ret = RunnerReturnCodes.UNEXPECTED_EXCEPTION
                logger.exception("executable run failed")

        else:
            logger.error("Executable does not export correct symbol.")
            ret = RunnerReturnCodes.SYMBOL_NOT_FOUND

    else:
        raise NotImplementedError("module type not supported: " + str(type_))

    err = RunnerReturnCodes.to_string(ret)
    return ScanResult(path=file_name, return_code=ret, xml_path=xml_out, error_msg=err)


def scan(args, extra):
    scanner = Scanner()

    output_dir = create_output_directory(args.output_path, args.no_timestamp)

    # setup logging
    log_path = os.path.join(output_dir, "aztest.log")
    setup_logging(log_path, args.verbosity)
    logger.info("AZ Test Scanner")

    if not args.runner_path:
        runner_path = os.path.abspath(os.path.join(args.dir, scanner.__runner_exe__))
    else:
        runner_path = os.path.abspath(args.runner_path)
    if not os.path.exists(runner_path):
        logger.exception("Invalid test runner path: {}".format(runner_path))
        return

    bootstrap_config = None
    if args.bootstrap_config:
        with open(args.bootstrap_config) as json_file:
            bootstrap_config = BootstrapConfig(flatten=True)
            bootstrap_config.load(json.load(json_file))

    add_dirs_to_path(args.add_path)
    scan_results = []  # list of ScanResult()

    # Find default filter files if they exist and add to user-defined lists
    whitelist_files = (args.whitelist_files if args.whitelist_files else []) + [get_default_whitelist()]
    blacklist_files = (args.blacklist_files if args.blacklist_files else []) + [get_default_blacklist()]

    # Create a FileApprover to determine if scanned files can be tested
    file_approver = FileApprover(whitelist_files, blacklist_files)

    module_failures = 0
    library_module_name_list = []
    executable_name_list = []

    # Dynamic Libraries / Modules
    if not __no_dll__:
        logger.info("Scanning for dynamic libraries")
        library_module_name_list = list(scanner.enumerate_modules(args.dir))
        for file_name in library_module_name_list:
            try:
                if args.limit and len(scan_results) >= args.limit:
                    continue  # reached scanning limit

                if args.only and not FileApprover.is_in_list(file_name, args.only.split(',')):
                    continue  # filename does not match any expected pattern

                if not file_approver.is_approved(file_name):
                    continue

                result = scan_one(args, extra, ModuleType.LIBRARY, scanner, runner_path, bootstrap_config,
                                  file_name, output_dir)
                if result:
                    scan_results += [result]
                    if result.return_code != RunnerReturnCodes.TESTS_SUCCEEDED:
                        if result.return_code == RunnerReturnCodes.MODULE_SKIPPED:
                            logger.info("Module SKIPPED: {}".format(file_name))
                        else:
                            logger.error("Module FAILED: {}, with exit code: {} ({})".format(file_name, result.return_code,
                                                                                         RunnerReturnCodes.to_string(
                                                                                             result.return_code)))
                            module_failures += 1
                    if not os.path.exists(result.xml_path):
                        XMLGenerator.create_xml_output_file(result.xml_path, result.return_code, result.error_msg)

            except KeyboardInterrupt:
                logger.exception("Process interrupted by user.")
                break
            except:
                logger.exception("Module scan failed.")

    # Executables
    if args.exe:
        logger.info("Scanning for executables")
        executable_name_list = list(scanner.enumerate_executables(args.dir))
        for file_name in executable_name_list:

            if args.limit and len(scan_results) >= args.limit:
                continue  # reached scanning limit

            if args.only and not FileApprover.is_in_list(file_name, args.only.split(',')):
                continue  # filename does not match any expected pattern

            if not file_approver.is_approved(file_name):
                continue

            result = scan_one(args, extra, ModuleType.EXECUTABLE, scanner, runner_path,
                              bootstrap_config, file_name, output_dir)
            if result:
                scan_results += [result]
                if result.return_code != RunnerReturnCodes.TESTS_SUCCEEDED:
                    logger.error("Module FAILED: {}, with exit code: {} ({})".format(file_name, result.return_code,
                                                                                     RunnerReturnCodes.to_string(
                                                                                         result.return_code)))
                    module_failures += 1
                if not os.path.exists(result.xml_path):
                    XMLGenerator.create_xml_output_file(result.xml_path, result.return_code, result.error_msg)

    # Always save ScanResult data in a JSON file so we have access to it later
    scan_results_json = {'scan_results': []}
    for scan_result in scan_results:
        scan_results_json['scan_results'].append(scan_result._asdict())
    json_path = os.path.join(output_dir, 'scan_results.json')
    with open(json_path, 'w') as f:
        json.dump(scan_results_json, f)

    print "----------------AUTOTEST SUMMARY -----------------"
    print("Log: {}".format(log_path))
    print("JSON results: {}".format(json_path))

    if not args.no_html_report:
        # Convert the set of XML files into an HTML report
        html_report = HTMLReporter.create_html_report(scan_results, output_dir)
        html_failure_report = HTMLReporter.create_html_failure_report(scan_results, output_dir)
        print("HTML report: {}".format(html_report))
        print("HTML failure-only report: {}".format(html_failure_report))

    print("Total modules found: {}".format(len(library_module_name_list)))
    if module_failures:
        colorama.init()
        print(Fore.RED + "**** {} modules had failures or errors ****".format(module_failures))
        print(Style.RESET_ALL)
    else:
        print("No modules had failures nor errors.")

    print("If a module has 1/0 tests failed or errored, this means the library failed to load.")

    test_summary_results = {
      TESTS_RUN_KEY: 0,
      TESTS_PASSED_KEY: 0,
      TESTS_FAILED_KEY: 0,
      TESTS_ERRORED_KEY: 0,
      TESTS_SKIPPED_KEY: 0,
      TOTAL_TIME_TAKEN_KEY: 0,
    }
    for file_name in library_module_name_list:
        _print_summary_for_file_name(test_summary_results, file_name, output_dir)

    for file_name in executable_name_list:
        _print_summary_for_file_name(test_summary_results, file_name, output_dir)

    print "\nTotal tests run: {0}".format(str(test_summary_results[TESTS_RUN_KEY]))
    print "Total tests passed: {0}".format(str(test_summary_results[TESTS_PASSED_KEY]))
    print "Total tests failed: {0}".format(str(test_summary_results[TESTS_FAILED_KEY]))
    print "Total tests errored: {0}".format(str(test_summary_results[TESTS_ERRORED_KEY]))
    print "Total tests skipped: {0}".format(str(test_summary_results[TESTS_SKIPPED_KEY]))
    print "Total test time taken: {0}\n".format(str(test_summary_results[TOTAL_TIME_TAKEN_KEY]))

    print "----------------AUTOTEST SUMMARY -----------------"

    return 1 if module_failures > 0 else 0


def _print_summary_for_file_name(test_summary_results, file_name, output_dir):
    try:
        xml_file_name = create_xml_output_filename(file_name, output_dir)
        XMLTree = ET.parse(xml_file_name)

        tests = int(XMLTree.getroot().attrib['tests'])
        failed = int(XMLTree.getroot().attrib['failures'])
        disabled = int(XMLTree.getroot().attrib['disabled'])
        errored = int(XMLTree.getroot().attrib['errors'])
        time = float(XMLTree.getroot().attrib['time'])

        test_summary_results[TESTS_RUN_KEY] = test_summary_results[TESTS_RUN_KEY] + tests
        test_summary_results[TESTS_PASSED_KEY] = test_summary_results[TESTS_PASSED_KEY] + tests - failed - disabled - errored
        test_summary_results[TESTS_FAILED_KEY] = test_summary_results[TESTS_FAILED_KEY] + failed
        test_summary_results[TESTS_ERRORED_KEY] = test_summary_results[TESTS_ERRORED_KEY] + errored
        test_summary_results[TESTS_SKIPPED_KEY] = test_summary_results[TESTS_SKIPPED_KEY] + disabled
        test_summary_results[TOTAL_TIME_TAKEN_KEY] = test_summary_results[TOTAL_TIME_TAKEN_KEY] + time

        bad_tests = int(failed) + int(errored)
        message = "{}: {}/{} tests failed or errored".format(ntpath.basename(file_name),
                                                             str(failed + errored),
                                                             str(tests))
        if bad_tests:
            print(message)

    except IOError:
        # this happens when a module is found but doesn't have tests
        pass
