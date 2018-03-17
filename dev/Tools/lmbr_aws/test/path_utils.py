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
# $Revision: #1 $

from __future__ import print_function
import os
import sys
import contextlib
import traceback
import __builtin__

__path_stack = []
__imports_stack = []

DEV_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))

def make_path(*args, **kwargs):
    return os.path.abspath(os.path.join(*args))

def dev_path(*args, **kwargs):
    return make_path(DEV_PATH, *args, **kwargs)
    
def python_path(*args, **kwargs):
    return dev_path('Tools', 'Python', 'python.cmd')
    
def python_aws_sdk_path(*args, **kwargs):
    return dev_path('Tools', 'AWSPythonSDK', '1.4.4')

def gem_path(gem_name, *args, **kwargs):
    gem_version_directory = kwargs.get('gem_version_directory')
    if gem_version_directory:
        return dev_path('Gems', gem_name, gem_version_directory, *args, **kwargs)
    else:
        return dev_path('Gems', gem_name, *args, **kwargs)

def gem_aws_path(gem_name, *args, **kwargs):
    return gem_path(gem_name, 'AWS', *args, **kwargs)

def gem_aws_test_path(gem_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'test', *args, **kwargs)

def gem_resource_manager_code_path(gem_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'resource-manager-code', *args, **kwargs)

def gem_resource_manager_code_test_path(gem_name, *args, **kwargs):
    return gem_resource_manager_code_path(gem_name, 'test', *args, **kwargs)

def gem_resource_manager_code_lib_path(gem_name, *args, **kwargs):
    return gem_resource_manager_code_path(gem_name, 'lib', *args, **kwargs)

def gem_project_code_path(gem_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'project-code', *args, **kwargs)

def gem_project_code_test_path(gem_name, *args, **kwargs):
    return gem_project_code_path(gem_name, 'test', *args, **kwargs)

def gem_test_path(gem_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'test', *args, **kwargs)

def gem_lambda_code_path(gem_name, function_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'lambda-code', function_name, *args, **kwargs)

def gem_lambda_code_test_path(gem_name, function_name, *args, **kwargs):
    return gem_lambda_code_path(gem_name, function_name, 'test', *args, **kwargs)

def gem_common_code_path(gem_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'common-code', *args, **kwargs)

def resource_manager_v1_path(*args, **kwargs):
    kwargs['gem_version_directory'] = 'v1'
    return gem_path('CloudGemFramework', 'ResourceManager', *args, **kwargs)

def resource_manager_v1_lib_path(*args, **kwargs):
    return resource_manager_v1_path('lib', *args, **kwargs)

def resource_manager_v1_module_path(*args, **kwargs):
    return resource_manager_v1_path('resource_manager', *args, **kwargs)

def resource_manager_v1_test_path(*args, **kwargs):
    return resource_manager_v1_module_path('test', *args, **kwargs)

def resource_manager_v1_project_resource_handler_path(*args, **kwargs):
    kwargs['gem_version_directory'] = 'v1'
    return gem_lambda_code_path('CloudGemFramework', 'ProjectResourceHandler', *args, **kwargs)

def resource_manager_v1_project_resource_handler_test_path(*args, **kwargs):
    return resource_manager_v1_project_resource_handler_path('test')

def resource_manager_v1_resource_manager_common_path(*args, **kwargs):
    kwargs['gem_version_directory'] = 'v1'
    return gem_common_code_path('CloudGemFramework', 'ResourceManagerCommon', *args, **kwargs)

# TODO: resolve_imports is duplicated in bootstrap.py. Currently test and test/.. don't know anything
# about each other. Need to rework how the test PYTHONPATH is constructed and add test\__init__.py
# to fix this.
#
# There is also a simular function in resource_manager\common_code.py, but it works for gems other
# than CloudGemFramework, which isn't easy or needed here.
def resolve_imports(framework_common_code_directory_path, target_directory_path, imported_paths = None):
    '''Determines the paths identified by an .import file in a specified path.

    The .import file should contain one import name per line. An import name has
    the format CloudGemFramework.{common-code-subdirectory-name}.

    Imports are resolved recursivly in an arbitary order. 
    
    Arguments:

        framework_common_code_directory_path: the path to the common-code directory 
        containing the imported directories.

        target_directory_path: the path that may or may not contain an .import file.

        imported_paths (optional): a set of paths that have already been imported.

    Returns a set of full paths to imported directories. All the directories will 
    have been verified to exist.

    Raises a HandledError if the .import file contents are malformed, identify a gem
    that does not exist or is not enabled, or identifies an common-code directory 
    that does not exist.
    '''

    if imported_paths is None:
        imported_paths = set()

    imports_file_path = os.path.join(target_directory_path, '.import')
    if os.path.isfile(imports_file_path):

        with open(imports_file_path, 'r') as imports_file:
            lines = imports_file.readlines()

        for line in lines:

            # parse the line
            line = line.strip()
            parts = line.split('.')
            if len(parts) != 2 or not parts[0] or not parts[1]:
                raise RuntimeError('Invalid import "{}" found in {}. Import should have the format <gem-name>.<common-code-subdirectory-name>.'.format(line, imports_file_path))

            gem_name, import_directory_name = parts

            if gem_name != 'CloudGemFramework':
                raise RuntimeError('The {} gem is not supported for resource manager imports as requested by {}. Only modules from the CloudGemFramework gem can imported.'.format(gem_name, imports_file_path))

            # get the identified common-code subdirectory

            path = os.path.join(framework_common_code_directory_path, import_directory_name)
            if not os.path.isdir(path):
                raise RuntimeError('The {} gem does not provide the {} import as found in {}. The {} directory does not exist.'.format(gem_name, line, imports_file_path, path))

            # add it to the set and recurse

            if path not in imported_paths: # avoid infinte loops and extra work
                imported_paths.add(path)
                resolve_imports(framework_common_code_directory_path, path, imported_paths)

    return imported_paths

