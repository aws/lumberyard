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

def gem_common_code_path(gem_name, subdirectory_name, *args, **kwargs):
    return gem_aws_path(gem_name, 'common-code', subdirectory_name, *args, **kwargs)

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

class SysPathContext(object):

    def __init__(self, inserted_paths, added_modules):
        self.__inserted_paths = inserted_paths
        self.__added_modules = added_modules

    @property
    def inserted_paths(self):
        return self.__inserted_paths

    @property
    def added_modules(self):
        return self.__added_modules


@contextlib.contextmanager
def sys_path_context(*inserted_paths):

    print('path_utils -- initializing sys path context')

    # insert paths into sys.path

    __add_paths(inserted_paths)

    # we will track names of modules added in this context, the empty
    # added_modules dict is added to the SysPathContext object below, 
    # then populated later.

    original_module_names = set(sys.modules.keys())
    added_modules = {}

    # execute the body of the with statement

    try:

        yield SysPathContext(inserted_paths, added_modules)

    except Exception as e:

        # include a lot of debugging info if something goes wrong

        details = '\n'
        details += '*********************************************************************************************************\n'

        # system path

        details += '\n'
        details += 'System path when {} "{}" occured:\n'.format(type(e), str(e))
        details += '\n'
        for path in sys.path:
            details += '    {}\n'.format(path if path else '(empty)')

        # loaded modules

        details += '\n'
        details += 'Modules loaded when {} "{}" occured:\n'.format(type(e), str(e))
        details += '\n'
        module_names = sys.modules.keys()
        module_names.sort()
        for module_name in module_names:
            module = sys.modules[module_name]
            details += '     {} from {}\n'.format(module_name, module.__file__ if hasattr(module, '__file__') else '(unkown)')

        # stack trace of origional error

        details += '\n'
        details += traceback.format_exc()

        details += '\n'
        details += '*********************************************************************************************************\n'
        details += '\n'

        raise RuntimeError(details)

    finally:

        # body of with statement has returned...

        # determine which modules were added (these are saved in the SysPathContext object
        # created above) and remove them from the sys.modules dictionary.

        for module_name, module in sys.modules.iteritems():
            if module_name not in original_module_names:
                added_modules[module_name] = module

        print('path_utils -- {} modules were added to the context.'.format(len(added_modules)))

        __remove_modules(added_modules)

        # remove paths from sys.path

        __remove_paths(inserted_paths)


def setup_sys_path_context(sys_path_context):
    print('path_utils -- setup sys path context, restoring {} modules.'.format(len(sys_path_context.added_modules)))
    __add_paths(sys_path_context.inserted_paths)
    __add_modules(sys_path_context.added_modules)


def teardown_sys_path_context(sys_path_context):
    print('path_utils -- teardown sys path context, removing {} modules.'.format(len(sys_path_context.added_modules)))
    __remove_modules(sys_path_context.added_modules)
    __remove_paths(sys_path_context.inserted_paths)


def __add_paths(paths):
    for path in reversed(paths):
        print('           -- adding path {}'.format(path))
        sys.path.insert(0, path)


def __remove_paths(paths):
    for path in paths:
        print('           -- removing path {}'.format(path))
        sys.path.remove(path)


def __remove_modules(modules):
    for module_name in modules.keys():
        if module_name in sys.modules:
            # for debugging: print('           -- removing module {} {}'.format(module_name, sys.modules[module_name]))
            del sys.modules[module_name]


def __add_modules(modules):
    for module_name, module in modules.iteritems():
        # for debugging: print('           -- adding module {} {}'.format(module_name, module))
        sys.modules[module_name] = module
