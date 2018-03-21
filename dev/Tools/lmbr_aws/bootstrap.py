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
# $Revision: #20 $

import ConfigParser
import importlib
import json
import os
import sys

from StringIO import StringIO

CLOUD_GEM_FRAMEWORK_UUID = '6fc787a982184217a5a553ca24676cfa'

AWS_PYTHON_SDK_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'AWSPythonSDK', '1.4.4'))


def load_resource_manager_module(framework_directory_path, module_name):
    '''Loads a module from the ResourceManager subdirectory of the specified CloudGemFramework directory.'''
    
    framework_common_code_directory_path = get_framework_common_code_directory_path(framework_directory_path)
    resource_manager_directory_path = get_resource_manager_directory_path(framework_directory_path)
    imported_paths = resolve_imports(framework_common_code_directory_path, resource_manager_directory_path)
    lib_directory_path = os.path.join(resource_manager_directory_path, 'lib')
    
    new_sys_path = [resource_manager_directory_path]
    new_sys_path.extend(imported_paths)
    new_sys_path.append(lib_directory_path)
    new_sys_path.append(AWS_PYTHON_SDK_PATH)
    new_sys_path.extend(sys.path)
    sys.path = new_sys_path

    return importlib.import_module(module_name)
    
    
def get_game_directory_path(root_directory_path):

    bootstrap_file_path = os.path.abspath(os.path.join(root_directory_path, "bootstrap.cfg"))

    if not os.path.exists(bootstrap_file_path):
        raise RuntimeError('Cannot determine game directory name. The {} file does not exist.'.format(bootstrap_file_path))

    # If we add a section header and change the comment prefix, then
    # bootstrap.cfg looks like an ini file and we can use ConfigParser.
    ini_str = '[default]\n' + open(bootstrap_file_path, 'r').read()
    ini_str = ini_str.replace('\n--', '\n#')
    ini_fp = StringIO(ini_str)
    config = ConfigParser.RawConfigParser()
    config.readfp(ini_fp)

    game_directory_name = config.get('default', 'sys_game_folder')
    game_directory_path = os.path.join(root_directory_path, game_directory_name)
    
    if not os.path.exists(game_directory_path):
        raise RuntimeError('The game directory identified by the bootstrap.cfg file does not exist: {}.'.format(game_directory_path))
        
    return game_directory_path

    
def get_framework_directory_path(root_directory_path, game_directory_path):

    gems_file_path = os.path.abspath(os.path.join(game_directory_path, 'gems.json'))
    if not os.path.isfile(gems_file_path):
        raise RuntimeError('Cannot determine enabled CloudGemFramework version because the gems.json file does not exist: {}.'.format(gems_file_path))
    try:
        with open(gems_file_path, 'r') as gems_file:
            gems_file_object = json.load(gems_file)
    except Exception as e:
        raise RuntimeError('Cannot determine enabled CloudGemFramework version because the gems.json file at {} cloud not be read: {}'.format(gems_file_path, e.message))
        
    format_version = gems_file_object.get('GemListFormatVersion', 'UNKNOWN')
    if format_version != 2:
        raise RuntimeError('Cannot determine enabled CloudGemFramework version because the gems.json file at {} does not contain a supported format version. Supported version: 2, Actual version: {}'.format(gems_file_path, format_version))

    for entry in gems_file_object.get('Gems', []):
        uuid = entry.get('Uuid')
        if uuid == CLOUD_GEM_FRAMEWORK_UUID:
            relative_gem_path = entry.get('Path')
            if not relative_gem_path:
                raise RuntimeError('Cannot determine enabled CloudGemFramework version because the gems.json file at {} does not specify a Path property for the CloudGemFramework.'.format(gems_file_path))
            framework_directory_path = os.path.abspath(os.path.join(root_directory_path, relative_gem_path.replace('/', os.sep)))
            return framework_directory_path
            
    return None

def get_resource_manager_directory_path(framework_directory_path):

    resource_manager_directory_path = os.path.join(framework_directory_path, 'ResourceManager')
    if not os.path.isdir(resource_manager_directory_path):
        raise RuntimeError('The enabled CloudGemFramework version does not contain a ResourceManager directory: {}'.format(resource_manager_directory_path))
                
    return resource_manager_directory_path

            
def get_framework_common_code_directory_path(framework_directory_path):

    path = os.path.join(framework_directory_path, 'AWS', 'common-code')
    if not os.path.isdir(path):
        raise RuntimeError('The enabled CloudGemFramework version does not contain a common-code directory: {}'.format(path))
                
    return path

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

    
            
