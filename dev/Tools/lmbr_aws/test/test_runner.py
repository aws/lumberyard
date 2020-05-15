#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #16 $

from __future__ import print_function

import argparse
import copy
import ctypes
import datetime
import glob
import importlib
import importlib.util
import json
import os
import subprocess
import sys
import threading
import time
import warnings
import platform

from typing import List
from functools import cmp_to_key

from six import iteritems

# Test are divided into to categories: unit tests and integration tests.
#
# Unit tests have no external dependencies (such as AWS or game levels) and are
# expected to execute quickly. Integration tests do have such dependencies and
# may take a while to execute.
#
# All unfiltered unit test are always executed first, followed by integration
# tests.
#
# Test suites are defined using the following properties:
#
#   - group: for sequential test runs, within each category (unit/integration)
#     tests are executed from the lowest number group to the highest. Tests in
#     the same group are sorted by name.
#
#     For parallel test runs, if group is GROUP_EXCLUSIVE, then the test suite
#     is run by itself before any other tests. Then all tests in all other groups
#     are run (TODO: if --fail-fast is specified, it may make sense to run each
#     group individually in order).
#
#   - command: the command executed to run the test suite. This path should be
#     relative to the "dev" directory.
#
#   - arguments: a list of arguments passed to command. Defaults to []. Arguments
#     can use the following substitution values:
#
#        BUILD_DIRECTORY - the value of the --build-directory option.
#
#   - required_libs: a list of python library names that must be installed before
#     running the test suite. This program will verify that these libraries are
#     present before starting any tests. The 'mock' library is always required.
#
#   - disabled: set to True to disable the test. The default is that the test is
#     enabled.
#

import path_utils

TYPE_INTEGRATION_TEST = "integration_test"
TYPE_UNIT_TEST = "unit_test"

# Map of toolchain to Gem containing CPP unit tests
CPP_GEM_MAP = {
    "Bin64vc141.Debug.Test": "Gem.CloudGemFramework.6fc787a982184217a5a553ca24676cfa.v1.1.4.dll",
    "Bin64vc142.Debug.Test": "Gem.CloudGemFramework.6fc787a982184217a5a553ca24676cfa.v1.1.4.dll"
}

# Default to vs2017 version of CloudGemFramework
DEFAULT_CPP_TEST_GEM = CPP_GEM_MAP["Bin64vc141.Debug.Test"]

# What needs to be omitted from the code coverage run
COVERAGE_OMIT = ','.join(
    [
        os.path.join('*', 'temp', '*'),  # any temp files that were created
        os.path.join('*', 'test', '*'),  # any unit test files
        os.path.join('*', '__init__.py'),  # any init files
        os.path.join('*', 'python', '*'),  # any other python libs
    ])

# These gems do not currently have unit tests, so they need to be manually checked during code coverage jobs
MISSING_COVERAGE_LIST = ['CloudGemWebCommunicator',
                         'CloudGemAWSScriptBehaviors',
                         'CloudGemDefectReporter',
                         'CloudGemLeaderboard',
                         'CloudGemMessageOfTheDay',
                         ]


def python_unittest_command(search_start_directory_path, top_level_directory_path=None, pattern='test_*.py'):
    # type: (str, str, str) -> List[str]
    """
    Creates the python command to run unit tests. It constructs a list with default parameters and returns it.
    :param search_start_directory_path: Where the unit test search starts
    :param top_level_directory_path: The top level directory of the project
    :param pattern: The naming pattern to find unit tests to run
    :return: A list of elements that make up the python unittest command to run
    """
    if top_level_directory_path is None:
        top_level_directory_path = os.path.abspath(os.path.join(search_start_directory_path, '..'))

    return [
        path_utils.python_path(),
        '-m', 'unittest', 'discover',
        '--verbose',
        '--failfast',
        '--top-level-directory', top_level_directory_path,
        '--start-directory', search_start_directory_path,
        '--pattern', pattern
    ]


def python_coverage_unittest_command(search_start_directory_path, gem_name, top_level_directory_path=None,
                                     pattern='test_*.py'):
    # type: (str, str, str, str) -> List[str]
    """
    Creates the python command to run code coverage. It constructs a list with default parameters and returns it. It
    targets the gem folder as the top level for code coverage. The command also includes all of the omission items.
    :param search_start_directory_path: Where the unit test search starts
    :param gem_name: Folder name of the gem located at dev/Gems
    :param top_level_directory_path: The top level directory of the project
    :param pattern: The naming pattern to find unit tests to run
    :return: A list of elements that make up the python code coverage command to run
    """
    if top_level_directory_path is None:
        top_level_directory_path = os.path.abspath(os.path.join(search_start_directory_path, '..'))

    sources_list = [path_utils.gem_path(gem_name)]
    for missing_gem in MISSING_COVERAGE_LIST:
        sources_list.append(path_utils.gem_path(missing_gem))
    sources_string = ','.join(sources_list)

    return [
        path_utils.python_win_coverage_path(),
        'run', '--append', '--branch',
        '--source', sources_string,
        '--omit', COVERAGE_OMIT,
        '-m', 'unittest', 'discover',
        '--verbose',
        '--failfast',
        '--top-level-directory', top_level_directory_path,
        '--start-directory', search_start_directory_path,
        '--pattern', pattern
    ]


def report_coverage(args):
    # type: (argparse.Namespace) -> None
    """
    Runs the coverage reporting command. Is able to produce html, json, and xml reports based off of the .coverage file
    that is produced during a coverage run.
    :param args: The args from the arg parser
    :return: None
    """
    if args.html_report:
        command = [path_utils.python_win_coverage_path(), 'html']
        subprocess.call(command, shell=True)
    if args.json_report:
        command = [path_utils.python_win_coverage_path(), 'json']
        subprocess.call(command, shell=True)
    if args.xml_report:
        command = [path_utils.python_win_coverage_path(), 'xml']
        subprocess.call(command, shell=True)


def combine_coverage(args):
    # type: (argparse.ArgumentParser) -> None
    """
    Combines multiple .coverage files into one if running parallel tests.
    :param args: The args from the arg parser
    :return:
    """
    command = [path_utils.python_win_coverage_path(), 'combine']
    print(subprocess.check_output(command, shell=True))


def lmbr_test_gem_command(gem_dll_name):
    # type: (str) -> List[str]
    """
    Constructs the lmbr_test command to run unit tests for the specified gem
    :param gem_dll_name: The name of the gem dll to run
    :return:
    """
    test_cmd = 'lmbr_test.cmd' if platform.system() == 'Windows' else 'lmbr_test.sh'
    return [
        path_utils.dev_path(test_cmd),
        'scan',
        '--include-gems',
        '--only', gem_dll_name,
        '--dir', '{BUILD_DIRECTORY}'
    ]


def resource_manager_v1_test_python_path(*args):
    # type: (List[str]) -> List[str]
    """
    Returns all of the necessary python paths in order to have correct imports for the test to run. These will be used
    for the PYTHONPATH environment variable when running tests.
    :param args: Any extra paths to be added to the list
    :return: A list of paths to be added to the PYTHONPATH environment variable
    """
    path = [path_utils.resource_manager_v1_path()]
    path.extend(
        path_utils.resolve_imports(
            path_utils.gem_common_code_path('CloudGemFramework', gem_version_directory='v1'),
            path_utils.resource_manager_v1_path()
        )
    )
    path.append(path_utils.resource_manager_v1_lib_path())
    path.append(path_utils.resource_manager_v1_test_path())
    path.extend(args)
    return path


def resource_manager_v1_common_code_test_python_path(target_directory_path, *args):
    # type: (str, List[str]) -> List[str]
    """
    Returns all of the necessary python paths in order to have correct imports for the test to run. These will be used
    for the PYTHONPATH environment variable when running tests.
    :param target_directory_path: The path to the gem's common-code directory
    :param args: Any extra paths to be added to the list
    :return: A list of paths to be added to the PYTHONPATH environment variable
    """
    print(target_directory_path)
    path = []
    path.extend(
        path_utils.resolve_imports(
            path_utils.gem_common_code_path('CloudGemFramework', gem_version_directory='v1'),
            target_directory_path
        )
    )
    path.extend(args)
    return path


def resource_manager_v1_lambda_code_test_python_path(target_directory_path, *args):
    # type: (str, List[str]) -> List[str]
    """
    Returns all of the necessary python paths in order to have correct imports for the test to run. These will be used
    for the PYTHONPATH environment variable when running tests.
    :param target_directory_path: The path to the gem's lambda-code directory
    :param args: Any extra paths to be added to the list
    :return: A list of paths to be added to the PYTHONPATH environment variable
    """
    path = []
    path.extend(
        path_utils.resolve_imports(
            path_utils.gem_common_code_path('CloudGemFramework', gem_version_directory='v1'),
            target_directory_path
        )
    )
    path.extend(args)
    return path


GROUP_EXCLUSIVE = -1

unit_test_suites = {

    'CloudGemFramework ResourceManager': {
        'group': GROUP_EXCLUSIVE,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(),
            'LYMETRICS': 'TEST'
        },
        'command': python_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_lmbr_aws.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_lmbr_aws.py',
            gem_name='CloudGemFramework'
        ),
        'platforms': ['Windows']
    },

    # Unit tests for all other 'common' components aside from the test_unit_common_code_import.py
    # Note: Using _rm_<file_name_under_test>.py so they can be matched with a pattern because unittest has no mechanism to exclude
    # test_unit_lmbr_aws.py and test_unit_common_code_import.py which are special case tests
    'CloudGemFramework ResourceManagerCode': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(),
            'LYMETRICS': 'TEST'
        },
        'command': python_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_rm_*.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_rm_*.py',
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework ResourceManager CommonCodeImport': {
        'group': 1,
        'environment': {
            'PYTHONPATH': [
                path_utils.resource_manager_v1_resource_manager_common_path(),
                path_utils.gem_common_code_path('CloudGemFramework', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', 'test', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'lib', gem_version_directory='v1')
            ]
        },
        'command': python_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_common_code_import.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            top_level_directory_path=path_utils.resource_manager_v1_path(),
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_unit_common_code_import.py',
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework common-code ResourceManagerCommon': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_common_code_test_python_path(
                path_utils.resource_manager_v1_resource_manager_common_path()
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_resource_manager_common_path('resource_manager_common', 'test'),
            top_level_directory_path=path_utils.resource_manager_v1_resource_manager_common_path()
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_resource_manager_common_path('resource_manager_common', 'test'),
            top_level_directory_path=path_utils.resource_manager_v1_resource_manager_common_path(),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework common-code LambdaSettings': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_common_code_test_python_path(
                path_utils.gem_common_code_path('CloudGemFramework', 'LambdaSettings', gem_version_directory='v1')
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaSettings', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaSettings', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaSettings', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaSettings', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework common-code LambdaService': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_common_code_test_python_path(
                path_utils.gem_common_code_path('CloudGemFramework', 'LambdaService', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ResourceManagerCommon', gem_version_directory='v1'),
                path_utils.gem_lambda_code_path('CloudGemFramework', 'ProjectResourceHandler', gem_version_directory='v1'),
                path_utils.gem_lambda_code_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1')
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaService', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaService', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaService', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'LambdaService', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework common-code ServiceClient_Python': {
        'group': 1,
        'environment': {
            'PYTHONPATH': [
                path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', 'test', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'lib', gem_version_directory='v1')
            ]
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework common-code Utils': {
        'group': 1,
        'environment': {
            'PYTHONPATH': [
                path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ResourceManagerCommon', gem_version_directory='v1')
            ]
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'Utils', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'Utils', 'test', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework lambda-code ProjectResourceHandler': {
        'group': 1,
        'environment': {
            'PYTHONPATH': [
                path_utils.gem_lambda_code_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'Utils', gem_version_directory='v1'),
                path_utils.gem_common_code_path('CloudGemFramework', 'ServiceClient_Python', gem_version_directory='v1'),
                path_utils.resource_manager_v1_resource_manager_common_path()
            ]
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemFramework', 'ProjectResourceHandler', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_lambda_code_path('CloudGemFramework', 'ProjectResourceHandler', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemFramework', 'ProjectResourceHandler', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_lambda_code_path('CloudGemFramework', 'ProjectResourceHandler', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework lambda-code ServiceLambda': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_lambda_code_test_python_path(
                path_utils.gem_lambda_code_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1')
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_lambda_code_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_lambda_code_path('CloudGemFramework', 'ServiceLambda', gem_version_directory='v1'),
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework resource-manager-code': {
        'group': 2,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(
                path_utils.gem_resource_manager_code_lib_path('CloudGemFramework', gem_version_directory='v1')
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_resource_manager_code_test_path('CloudGemFramework', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_resource_manager_code_path('CloudGemFramework', gem_version_directory='v1'),
            pattern='test_unit*.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_resource_manager_code_test_path('CloudGemFramework', gem_version_directory='v1'),
            top_level_directory_path=path_utils.gem_resource_manager_code_path('CloudGemFramework', gem_version_directory='v1'),
            pattern='test_unit*.py',
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemFramework CPP': {
        'group': 2,
        'command': lmbr_test_gem_command(
            gem_dll_name=DEFAULT_CPP_TEST_GEM
        ),
        'coverage_command': 'echo Skipping C++ tests during python code coverage run.'
    },

    'CloudGemPlayerAccount lambda-code ServiceLambda': {
        'group': 1,
        'environment': {
            'PYTHONPATH': [
            ]
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemPlayerAccount', 'ServiceLambda')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_lambda_code_test_path('CloudGemPlayerAccount', 'ServiceLambda'),
            gem_name='CloudGemPlayerAccount'
        )
    }
}

integration_test_suites = {

    'StackOperations': {
        'group': 1,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration.py',
            gem_name='CloudGemFramework'
        )
    },

    'UpdateHooks': {
        'group': 2,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_update_hooks.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_update_hooks.py',
            gem_name='CloudGemFramework'
        )
    },

    'ProjectResourceHooks': {
        'group': 2,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_project_resource_hooks.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_project_resource_hooks.py',
            gem_name='CloudGemFramework'
        )
    },

    'CustomResourceHandlerPlugins': {
        'group': 2,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integraton_custom_resource_handler_plugins.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integraton_custom_resource_handler_plugins.py',
            gem_name='CloudGemFramework'
        )
    },

    'CustomResourceHandlers': {
        'group': 2,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_custom_resource.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_custom_resource.py',
            gem_name='CloudGemFramework'
        )
    },

    'Security': {
        'group': 3,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_security.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_security.py',
            gem_name='CloudGemFramework'
        )
    },

    'ServiceApi': {
        'group': 3,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(
                path_utils.gem_common_code_path('CloudGemFramework', 'lib', gem_version_directory='v1'),
                path_utils.gem_resource_manager_code_path('CloudGemFramework', gem_version_directory='v1'),
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_service_api.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_service_api.py',
            gem_name='CloudGemFramework'
        ),
        'required_libs': []
    },

    'CGP': {
        'group': 3,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(
                path_utils.gem_resource_manager_code_path('CloudGemFramework', gem_version_directory='v1')
            )
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_cgp.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_cgp.py',
            gem_name='CloudGemFramework'
        )
    },

    'CognitoResourceHandlers': {
        'group': 3,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integraton_cognito_resource_handlers.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integraton_cognito_resource_handlers.py',
            gem_name='CloudGemFramework'
        )
    },

    'ExternalResource': {
        'group': 3,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_external_resource.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_external_resource.py',
            gem_name='CloudGemFramework'
        )
    },

    'CloudGemDynamicContent': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemDynamicContent')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemDynamicContent'),
            gem_name='CloudGemDynamicContent'
        )
    },

    'AWSLambdaLanguageDemo': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path(
                'AWSLambdaLanguageDemo', gem_version_directory='v1')
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path(
                'AWSLambdaLanguageDemo', gem_version_directory='v1'),
            gem_name='AWSLambdaLanguageDemo'
        )
    },

    'CloudGemPlayerAccount': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemPlayerAccount'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemPlayerAccount'),
            pattern='test_integration.py',
            gem_name='CloudGemPlayerAccount'
        ),
        'required_libs': [],
        'disable': [
            TYPE_INTEGRATION_TEST
        ]
    },

    'CloudGemInGameSurvey': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemInGameSurvey'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemInGameSurvey'),
            pattern='test_integration.py',
            gem_name='CloudGemInGameSurvey'
        ),
        'required_libs': []
    },

    'CloudGemMetric': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemMetric', gem_version_directory='v1'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemMetric', gem_version_directory='v1'),
            pattern='test_integration.py',
            gem_name='CloudGemMetric'
        ),
        'required_libs': []
    },

    'CloudGemDefectReporter': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemDefectReporter',
                                                                     gem_version_directory='v1'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemDefectReporter',
                                                                     gem_version_directory='v1'),
            pattern='test_integration.py',
            gem_name='CloudGemDefectReporter'
        ),
        'required_libs': [],
        'disable': [
            TYPE_INTEGRATION_TEST
        ]
    },

    'CloudGemTextToSpeech': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path(
                path_utils.gem_common_code_path('CloudGemFramework', 'lib', gem_version_directory='v1'))
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemTextToSpeech'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemTextToSpeech'),
            pattern='test_integration.py',
            gem_name='CloudGemTextToSpeech'
        )
    },

    'CloudGemSpeechRecognition': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemSpeechRecognition'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemSpeechRecognition'),
            pattern='test_integration.py',
            gem_name='CloudGemSpeechRecognition'
        ),
        'required_libs': []
    },

    'CloudGemComputeFarm': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemComputeFarm', gem_version_directory='v1'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemComputeFarm', gem_version_directory='v1'),
            pattern='test_integration.py',
            gem_name='CloudGemComputeFarm'
        ),
        'required_libs': []
    },

    'CloudGemWebCommunicator': {
        'group': 4,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemWebCommunicator'),
            pattern='test_integration.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.gem_aws_test_path('CloudGemWebCommunicator'),
            pattern='test_integration.py',
            gem_name='CloudGemWebCommunicator'
        ),
        'required_libs': []
    },

    'FrameworkVersionUpdate v1.0.0 to latest': {
        'group': 5,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update.py',
            gem_name='CloudGemFramework'
        )
    },

    'FrameworkVersionUpdate v1.1.1 to latest': {
        'group': 5,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update_from_1_1_1_to_latest.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update_from_1_1_1_to_latest.py',
            gem_name='CloudGemFramework'
        )
    },

    'FrameworkVersionUpdate v1.1.2 to latest': {
        'group': 5,
        'environment': {
            'PYTHONPATH': resource_manager_v1_test_python_path()
        },
        'command': python_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update_from_1_1_2_to_latest.py'
        ),
        'coverage_command': python_coverage_unittest_command(
            search_start_directory_path=path_utils.resource_manager_v1_test_path(),
            pattern='test_integration_version_update_from_1_1_2_to_latest.py',
            gem_name='CloudGemFramework'
        )
    }
}


def main():
    parser = argparse.ArgumentParser(
        prog='test_runner',
        description='Run Cloud Canvas unit and integration tests.'
    )

    parser.add_argument('--sequential', '-s', action='store_true', required=False,
                        help='Run all the test suites sequentially and write all output to stdout/stderr instead of '
                             'files. The default is to run the test suites in parallel and write output to files.')
    parser.add_argument('--filter', '-f', nargs='+', metavar='STRING', required=False,
                        help='Run only test suites with names contain STRING. The default is to run all test suites.')
    parser.add_argument('--list', '-l', action='store_true', required=False,
                        help='List the available test suites and exit. No tests are run.')
    parser.add_argument('--build-directory', '-b', metavar='PATH', required=False, default='Bin64vc141.Debug.Test',
                        help='The build output directory used for running C++ tests. Default is Bin64vc141.Debug.Test.')
    parser.add_argument('--unit-tests-only', '-u', action='store_true', required=False,
                        help='Run only the unit tests. By default unit tests and integration tests are run.')
    parser.add_argument('--integration-tests-only', '-i', action='store_true', required=False,
                        help='Run only the integration tests. By default unit tests and integration tests are run.')
    parser.add_argument('--fail-fast', '-t', action='store_true', required=False,
                        help='Do not run test suites after a failure. By default all test suites are run. Only applies '
                             'to sequential runs.')
    parser.add_argument('--continue', '-c', action='store_true', required=False, dest='continue_run',
                        help='Continue with previous failed test runs. By default all the test state files are deleted.'
                        )
    parser.add_argument('--coverage', '-co', action='store_true', required=False,
                        help='Run as coverage instead of python.')
    parser.add_argument('--html-report', '-html', action='store_true', required=False,
                        help='Produces an html code coverage report at the --coverage_path directory.')
    parser.add_argument('--json-report', '-json', action='store_true', required=False,
                        help='Produces an json code coverage report at the --coverage_path directory.')
    parser.add_argument('--xml-report', '-xml', action='store_true', required=False,
                        help='Produces an xml code coverage report at the --coverage_path directory.')

    args = parser.parse_args()

    if args.coverage and not args.sequential:
        parser.error("--coverage needs to be set with --sequential, it cannot run in parallel.")

    # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
    warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)

    if args.coverage:
        if not os.path.exists(path_utils.python_win_coverage_path()):
            raise Exception('Python coverage cannot be found at: {} \n Run "pip install coverage" in Tools/Python'.format(
                path_utils.python_win_coverage_path()))

        # Remove any existing coverage files if they exist
        if os.path.exists(os.path.join(path_utils.dev_path(), '.coverage')):
            os.remove(os.path.join(path_utils.dev_path(), '.coverage'))

    # Ensure selection of right gem for CPP unit tests based on build
    selected_gem = CPP_GEM_MAP[args.build_directory]
    if selected_gem:
        global DEFAULT_CPP_TEST_GEM
        DEFAULT_CPP_TEST_GEM = selected_gem

    output_message('')

    cleanup_previous_test_state(args)

    if args.list:
        exit_code = list_test_suites(args)
    elif args.sequential:
        exit_code = run_test_suites_in_sequence(args)
    else:
        exit_code = run_test_suites_in_parallel(args)

    if args.coverage:
        report_coverage(args)

    return exit_code


def list_test_suites(args):
    suites = get_all_filtered_test_suites_in_order(args)

    for suite in suites:
        suite['full_command_line'] = ' '.join(suite.get('command', []))

    def group_formatter(group):
        if group == GROUP_EXCLUSIVE:
            return 'exclusive'
        else:
            return str(group).rjust(len('exclusive'), ' ')

    output_table(suites,
                 [
                     {'Field': 'name', 'Heading': 'Test Name'},
                     {'Field': 'type', 'Heading': 'Type'},
                     {'Field': 'group', 'Heading': 'Group', 'Formatter': group_formatter},
                     {'Field': 'full_command_line', 'Heading': 'Command'}
                 ],
                 sort_column_count=0  # list is already sorted by order run
                 )

    return 0


def run_test_suites_in_parallel(args):
    global_start_time = time.time()

    results_directory_path = get_test_results_directory_path(args)

    filtered_unit_test_suites = filter_and_sort_test_suites(args, unit_test_suites, 'Unit') if not \
        args.integration_tests_only else []

    combined_test_suites = []
    combined_test_suites.extend(filtered_unit_test_suites)

    if not args.unit_tests_only:
        filtered_integration_test_suites = filter_and_sort_test_suites(args, integration_test_suites, 'Integration')
        combined_test_suites.extend(filtered_integration_test_suites)
    else:
        filtered_integration_test_suites = []

    if not combined_test_suites:
        raise RuntimeError('No test suites were selected by the filter.')

    verify_required_libs(combined_test_suites)

    output_message('Hit Ctrl+C to abort...')
    output_message('')

    failed_suites = []

    if filtered_unit_test_suites:
        failed_suites.extend(do_run_test_suites_in_parallel(args, filtered_unit_test_suites, results_directory_path, 'Unit'))

    if failed_suites:
        for suite in filtered_integration_test_suites:
            suite['failure_reason'] = 'Test skipped'
        failed_suites.extend(filtered_integration_test_suites)
    else:
        if not args.unit_tests_only and filtered_integration_test_suites:
            failed_suites.extend(do_run_test_suites_in_parallel(args, filtered_integration_test_suites, results_directory_path, 'Integration'))

    if failed_suites:
        output_message('The following tests failed:')
        output_message('')
        for suite in failed_suites:
            file_path = suite.get('output_file_path', '')
            output_message('    ' + suite['title'] + ' - ' + suite['failure_reason'] + ' ' + file_path)
            # tail the log and show the last 50 lines of the log.
            if file_path:
                with open(file_path, 'r') as file:
                    print("\tLast 50 lines of the log.")
                    lines = tail(file, 50)
                    for line in lines:
                        print("\t\t{}".format(line.rstrip()))

        exit_code = 1
    else:
        output_message('All tests passed.')
        exit_code = 0

    output_message('')

    output_message('Elapsed time for all tests: {}'.format(format_elapsed_time(global_start_time)))

    output_message('')

    return exit_code


def tail(f, lines=1, _buffer=4098):
    """Tail a file and get X lines from the end"""
    # place holder for the lines found
    lines_found = []

    # block counter will be multiplied by buffer
    # to get the block size from the end
    block_counter = -1

    # loop until we find X lines
    while len(lines_found) < lines:
        try:
            f.seek(block_counter * _buffer, os.SEEK_END)
        except IOError:  # either file is too small, or too many lines requested
            f.seek(0)
            lines_found = f.readlines()
            break

        lines_found = f.readlines()

        # we found enough lines, get out
        # Removed this line because it was redundant the while will catch
        # it, I left it for history
        # if len(lines_found) > lines:
        #    break

        # decrement the block counter to get the
        # next X bytes
        block_counter -= 1

    return lines_found[-lines:]


def do_run_test_suites_in_parallel(args, suites, results_directory_path, type):
    failed_suites = []

    exclusive_test_suites = [suite for suite in suites if suite['group'] == GROUP_EXCLUSIVE]
    other_test_suites = [suite for suite in suites if suite['group'] != GROUP_EXCLUSIVE]

    for suite in exclusive_test_suites:
        failed_suites.extend(run_test_suite_list_in_parallel(args, [suite], results_directory_path, type))

    failed_suites.extend(run_test_suite_list_in_parallel(args, other_test_suites, results_directory_path, type))

    return failed_suites


def display_wait_count(wait_count, type):
    if wait_count == 1:
        ctypes.windll.kernel32.SetConsoleTitleA('Waiting for {} {} Test'.format(wait_count, type))
    else:
        ctypes.windll.kernel32.SetConsoleTitleA('Waiting for {} {} Tests'.format(wait_count, type))


def run_test_suite_list_in_parallel(args, suites, results_directory_path, type):
    threads = []

    output_lock = threading.Lock()

    wait_count = [0]  # using an array to avoid unbound variables in the worker function
    failed_suites = []

    output_line_separator('=', 80)

    for suite in suites:
        output_file_path = os.path.normpath(os.path.join(results_directory_path, suite['title'].replace(' ', '_') + ".txt"))

        output_message('  STARTING: {}'.format(suite['title']))
        output_message('   command: {}'.format(format_suite_command('            ', suite)))
        output_message('    output: {}'.format(output_file_path))
        output_message('      time: {}'.format(datetime.datetime.now().strftime('%c')))
        output_line_separator('-', 80)

        def worker(suite, output_file_path):

            with output_lock:
                wait_count[0] = wait_count[0] + 1
                display_wait_count(wait_count[0], type)

            start_time = time.time()

            def execute(command):
                popen = subprocess.Popen(
                    command,
                    stdout=subprocess.PIPE,  # Pipe stdout to the loop below
                    stderr=subprocess.STDOUT,  # Redirect stderr to stdout
                    stdin=subprocess.PIPE,  # See below.
                    universal_newlines=True,  # Convert CR/LF to LF
                    env=process_suite_environment(suite)
                )

                # Piping stdin and closing it causes the process to exit on Ctrl+C instead of prompting
                # to terminate the batch job (at least I think that is what is happening... it made
                # everything work as desired).
                popen.stdin.close()

                # Make the this function return an iterator over lines read from the process output.
                # A bit of python iterator magic here. Basically each the next function of the iterator
                # return by this function is called, it will call readline from the process's stdout
                # pipe. Everything stops when readline returns a zero length string, which happens when
                # the process terminates.

                for stdout_line in iter(popen.stdout.readline, ""):
                    yield stdout_line

                # After the iterator finishes, close the pipe and get the process exit code (no waiting
                # is done here because the process has already terminated, causing the iterator loop
                # above to terminate).
                popen.stdout.close()
                exit_code = popen.wait()

                # If the process failed, raise an exception to signal that to the loop below.
                if exit_code:
                    raise subprocess.CalledProcessError(exit_code, command)

            # Open a file to receive the output which is read from the process using an iterator
            # returned by the execute function. The loop will exit when the process terminates.
            # If the process exited with an error, an exception is raised by the execute function.
            try:
                with open(output_file_path, mode='w') as output_file:
                    suite_command = suite['command']
                    for output_line in execute(suite_command):
                        output_file.write(output_line)
                        output_file.flush()
                exception = None
            except Exception as e:
                exception = e

            with output_lock:
                output_line_separator('-', 80)

                if exception:
                    output_message('    FAILED: {}'.format(suite['title']))
                    suite['output_file_path'] = output_file_path
                    suite['failure_reason'] = 'Test run failed: '
                    failed_suites.append(suite)
                else:
                    output_message('    PASSED: {}'.format(suite['title']))
                    record_successful_suite(suite)

                output_message('   command: {}'.format(format_suite_command('            ', suite)))
                output_message('    output: {}'.format(output_file_path))
                output_message('      time: {}'.format(datetime.datetime.now().strftime('%c')))
                output_message('   elapsed: {}'.format(format_elapsed_time(start_time)))

                wait_count[0] = wait_count[0] - 1
                display_wait_count(wait_count[0], type)

        thread = threading.Thread(target=worker, args=[suite, output_file_path], name=suite['title'])
        thread.start()
        threads.append(thread)

    output_message('Waiting...')

    for thread in threads:
        try:
            thread.join()
        except KeyboardInterrupt:
            # Ctrl+C will cause the sub-processes to exit with a non-zero code, which will cause the
            # tests to show as failed.
            pass

    output_line_separator('=', 80)
    output_message('')

    return failed_suites


def run_test_suites_in_sequence(args):
    global_start_time = time.time()

    suites = get_all_filtered_test_suites_in_order(args)

    verify_required_libs(suites)

    failed_tests = []

    for suite in suites:

        suite_start_time = time.time()

        output_line_separator('=', 80)
        output_message('  STARTING: {}'.format(suite['title']))
        output_message('        as: {}'.format(format_suite_command('            ', suite, coverage=args.coverage)))
        output_message('')

        if args.coverage:
            suite_command = suite['coverage_command']
        else:
            suite_command = suite['command']
        use_shell = True if platform.system() == 'Windows' else False
        exit_code = subprocess.call(suite_command, shell=use_shell, env=process_suite_environment(suite))

        output_message('')
        if exit_code:
            output_message('   FAILED: {}'.format(suite['title']))
            failed_tests.append(suite)
        else:
            output_message('   PASSED: {}'.format(suite['title']))
            record_successful_suite(suite)

        output_message('  elapsed: {}'.format(format_elapsed_time(suite_start_time)))
        output_line_separator('=', 80)

        if exit_code and args.fail_fast:
            break

    output_message('')
    output_message('')

    if failed_tests:
        output_message('The following tests failed:')
        for suite in failed_tests:
            output_message('    ' + suite['title'])
        exit_code = 1
    else:
        output_message('All tests passed.')
        exit_code = 0

    output_message('')

    output_message('Elapsed time: {}'.format(format_elapsed_time(global_start_time)))

    output_message('')

    return exit_code


def format_elapsed_time(start_time):
    end_time = time.time()

    elapsed_seconds = end_time - start_time
    minutes, seconds = divmod(elapsed_seconds, 60)
    hours, minutes = divmod(minutes, 60)

    return '%d:%02d:%02d' % (hours, minutes, seconds)


def get_all_filtered_test_suites_in_order(args):
    result = []

    result.extend(filter_and_sort_test_suites(args, unit_test_suites, 'Unit'))

    if not args.unit_tests_only:
        result.extend(filter_and_sort_test_suites(args, integration_test_suites, 'Integration'))

    if not result:
        raise RuntimeError('No test suites were selected by the filter.')

    return result


def get_test_run_state_file_path():
    return os.path.join(os.environ.get('TEMP', ''), 'lmbr_aws_test_run_state.json')


def record_successful_suite(suite):
    test_run_state_file_path = get_test_run_state_file_path()

    if os.path.isfile(test_run_state_file_path):
        with open(test_run_state_file_path, 'r') as file:
            record = json.load(file)
    else:
        record = []

    record.append(suite['title'])

    with open(test_run_state_file_path, 'w') as file:
        json.dump(record, file)


def filter_and_sort_test_suites(args, suite_dict, type):
    def compare(x, y):
        x_group = x[1].get('group', 9)
        y_group = y[1].get('group', 9)
        if x_group < y_group:
            return -1
        elif x_group > y_group:
            return 1
        else:
            x_name = x[1].get('name', '')
            y_name = y[1].get('name', '')
            if x_name < y_name:
                return -1
            elif x_name > y_name:
                return 1
            else:
                return 0

    test_run_state_file_path = get_test_run_state_file_path()
    if os.path.isfile(test_run_state_file_path):
        with open(test_run_state_file_path, 'r') as file:
            successful_suite_record = json.load(file)
    else:
        successful_suite_record = []

    were_disabled_suites = True

    suites = []
    for name, suite in sorted(suite_dict.items(), key=cmp_to_key(compare)):
        title = '{} {} Tests'.format(name, type)

        if 'disable' in suite:
            suite_disabled_test_types = suite['disable']
            if TYPE_INTEGRATION_TEST in suite_disabled_test_types:
                continue

        # Filter out any suites for a specific platform(s)
        # Use 'Windows', 'Darwin' etc to filter suites
        if 'platforms' in suite:
            suite_platforms = suite['platforms']
            sys_platform = platform.system()
            if sys_platform not in suite_platforms:
                output_message('WARNING: Suite: {} disabled on platform: {} '.format(title, sys_platform))
                continue

        if filter_includes(args, title):
            if suite.get('disabled', False):
                output_message('WARNING: Skipping disabled suite: {}'.format(title))

            elif title in successful_suite_record:
                output_message('WARNING: Skipping previously successful suite: {}'.format(title))

            else:
                suite = copy.deepcopy(suite)

                suite['name'] = name
                suite['title'] = title
                suite['type'] = type

                new_command = []
                for part in suite['command']:
                    part = part.replace('{BUILD_DIRECTORY}', args.build_directory)
                    new_command.append(part)
                suite['command'] = new_command

                suites.append(suite)

    if were_disabled_suites:
        output_message('')

    return suites


def filter_includes(args, title):
    """
    To filter based on test names with spaces in windows need to quote the string on the command line:
    Tools\\lmbr_aws\\test\\RunAllTests.cmd -u -s -f="\"cloudgemframework resourcemanager unit\"" --noaztests

    This will result in a quoted string, so need to strip below
    """
    if args.filter:
        lower_name = title.lower()
        for _filter in args.filter:
            if _filter.lower().strip('\"') not in lower_name:
                return False
    return True


def verify_required_libs(suites):
    """Note: Temporarily removed most required libs from testing due to differences in site-packages"""
    libs = {'unittest.mock'}

    for suite in suites:
        for lib in suite.get('required_libs', []):
            libs.add(lib)

    all_ok = True
    for lib in libs:
        try:
            spec = importlib.util.find_spec(lib)
            importlib.util.module_from_spec(spec)
        except Exception as e:
            output_message('ERROR: could not import required lib {}: {}'.format(lib, str(e)))
            all_ok = False

    if not all_ok:
        raise RuntimeError('Not all required libraries could be imported.')


def get_test_results_directory_path(args):
    path = path_utils.dev_path('CloudCanvasTestResults')
    if not os.path.exists(path):
        os.makedirs(path)
    return path


def output_table(items, specs, sort_column_count=1, indent=False, first_sort_column=0):
    """ Displays a table containing data from items formatted as defined by specs.

    items is an array of dict. The properties shown are determined by specs.

    specs is an array of dict with the following properties:

        Field -- Identifies a property in an item. Required.
        Heading -- The heading that is displayed. Required.
        Default -- A default value for the property. Defaults to ''
        Formatter -- A function that is called to format the property value or the default value.
        Hidden -- If present and True, the column is not displayed.
        HideWhenEmpty -- If present and True, the column is not displayed if there are no values.

    The columns are arranged in the order of the specs. The column widths are automatically determined.

    The items are sorted in ascending order by the formatted value of the first n columns, where n
    is specified by the sort_column_count parameter (which defaults to 1, causing the the table to
    be sorted by the first column only).

    """

    def default_formatter(v):
        return str(v) if v is not None else ''

    def get_formatted_value(item, spec):
        field = spec['Field']
        formatter = spec.get('Formatter', default_formatter)
        default = spec.get('Default', None)
        return formatter(item.get(field, default))

    # For simplicity we generate the formatted value multiple times. If this
    # ends up being used to display large tables this may need to be changed.
    # We sort working up to the first column and python guarantees that a
    # stable sort is used, so things work out how we want.

    for sort_column in range((sort_column_count + first_sort_column) - 1, first_sort_column - 1, -1):
        items = sorted(items, key=lambda item: get_formatted_value(item, specs[sort_column]))

    # determine width of each column

    lengths = {}

    for item in items:
        for spec in specs:
            field = spec['Field']
            lengths[field] = max(lengths.get(field, 0), len(get_formatted_value(item, spec)))

    def is_hidden(spec):
        return spec.get('Hidden', False) or (spec.get('HideWhenEmpty', False) and lengths.get(spec['Field'], 0) == 0)

    specs = [spec for spec in specs if not is_hidden(spec)]

    for spec in specs:
        field = spec['Field']
        lengths[field] = max(lengths.get(field, 0), len(spec['Heading']))

    # determine the prefix for each line

    if indent:
        prefix = '    '
    else:
        prefix = ''

    # show the headings

    heading = '\n'
    heading += prefix
    for spec in specs:
        heading += '{0:{1}}  '.format(spec['Heading'], lengths[spec['Field']])
    output_message(heading)

    # show a dividing line under the headings

    divider = prefix
    for spec in specs:
        divider += ('-' * lengths[spec['Field']]) + '  '
    output_message(divider)

    # show the items

    for item in items:
        line = prefix
        for spec in specs:
            formatted_value = get_formatted_value(item, spec)
            line += '{0:{1}}  '.format(formatted_value, lengths[spec['Field']])
        output_message(line)


def flatten(toFlatten, separator):
    flattened = ""
    for item in toFlatten:
        if isinstance(item, list):
            flattened += flatten(item, separator)
        else:
            flattened += item + separator
    return flattened

def process_suite_environment_value(value):
    if type(value) is list:
        value = flatten(value, os.pathsep)
    value = os.path.expandvars(value)
    return value


def process_suite_environment(suite):
    env = copy.copy(os.environ)

    # prevent interference with the test environments
    env.pop('PYTHONPATH', None)

    if 'environment' in suite:
        for key, value in iteritems(suite.get('environment', {})):
            env[key] = process_suite_environment_value(value)

    return env


def format_suite_command(prefix, suite, coverage=False):
    result = ''

    for key, value in iteritems(suite.get('environment', {})):
        value = process_suite_environment_value(value)
        if result:
            result = result + '\n' + prefix
        if ' ' in key or ' ' in value:
            result = result + 'SET "{}={}"'.format(key, value)
        else:
            result = result + 'SET {}={}'.format(key, value)

    if result:
        result = result + '\n' + prefix

    first_part = True
    if coverage:
        command = suite['coverage_command']
    else:
        command = suite['command']

    for part in command:
        if ' ' in part:
            part = '"' + part + '"'
        if not first_part:
            result = result + ' '
        result = result + part
        first_part = False

    return result


def cleanup_previous_test_state(args):
    if args.continue_run:
        print('Continuing previous test run (not deleting test state).')
        return

    temp_dir = os.environ.get('TEMP', '')
    search_path = os.path.join(temp_dir, 'tmp_last_running_*')

    for file_path in glob.glob(search_path):
        print('Deleting {}'.format(file_path))
        os.remove(file_path)

    test_run_state_file_path = get_test_run_state_file_path()
    if os.path.isfile(test_run_state_file_path):
        print('Deleting {}'.format(test_run_state_file_path))
        os.remove(test_run_state_file_path)


def output_message(msg):
    print(msg)


def output_line_separator(separator, repeats):
    print('{}'.format(separator * repeats))


def debug(*args):
    print(datetime.datetime.now().strftime('%c'), *args)


if __name__ == "__main__":
    sys.exit(main())
