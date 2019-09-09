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
import re
import os
import json
import shutil
import stat
import hashlib
import fnmatch
import datetime
import subprocess

from waflib import TaskGen, Utils, Logs
from waflib.Configure import conf
from gems import Gem, GemManager

def fast_copyfile(src, dst, buffer_size=1024*1024):
    """
    Copy data from src to dst - reimplemented with a buffer size
    Note that this function is simply a copy of the function from the official python shutils.py file, but
    with an increased (configurable) buffer size fed into copyfileobj instead of the original 16kb one
    """
    if shutil._samefile(src, dst):
        raise shutil.Error("`%s` and `%s` are the same file" % (src, dst))

    for fn in [src, dst]:
        try:
            st = os.stat(fn)
        except OSError:
            # File most likely does not exist
            pass
        else:
            # XXX What about other special files? (sockets, devices...)
            if stat.S_ISFIFO(st.st_mode):
                raise shutil.SpecialFileError("`%s` is a named pipe" % fn)

    with open(src, 'rb') as fsrc:
        with open(dst, 'wb') as fdst:
            shutil.copyfileobj(fsrc, fdst, buffer_size)

def fast_copy2(src, dst, check_file_hash=False, buffer_size=1024*1024):
    """
    Copy data and all stat info ("cp -p src dst").
    The destination may be a directory.
    Note that this is just a copy of the copy2 function from shutil.py that calls the above fast copyfile
    """

    if os.path.isdir(dst):
        dst = os.path.join(dst, os.path.basename(src))

    # If the destination file exists, and we want to check the fingerprints, perform the check
    # to see if we need to actually copy the file
    if os.path.exists(dst) and check_file_hash:
        src_hash = calculate_file_hash(src)
        dst_hash = calculate_file_hash(dst)
        if src_hash == dst_hash:
            # If the hashes match, no need to try to copy
            return False

    fast_copyfile(src, dst, buffer_size)
    shutil.copystat(src, dst)
    return True


MAX_TIMESTAMP_DELTA_MILS = datetime.timedelta(milliseconds=1)


def should_overwrite_file(source_path, dest_path):
    """
    Generalized decision to perform a copy/overwrite of an existing file.

    For improved performance only a comparison of size and mod times of the file is checked.

    :param source_path:     The source file path to copy from
    :param dest_path:     The target file path to copy to
    :return:  True if the target file does not exist or the target file is different from the source based on a specific criteria
    """

    copy = False
    if os.path.exists(source_path):
        if not os.path.exists(dest_path):
            # File does not exist, so always perform a copy
            copy = True
        elif os.path.isfile(source_path) and os.path.isfile(dest_path):
            # Only overwrite for files
            source_stat = os.stat(source_path)
            dest_stat = os.stat(dest_path)
            # Right now, ony compare the file sizes and mod times to detect if an overwrite is necessary
            if source_stat.st_size != dest_stat.st_size:
                copy = True
            else:
                # If the sizes are the same, then check the timestamps for the last mod time
                source_mod_time = datetime.datetime.fromtimestamp(source_stat.st_mtime)
                dest_mod_time = datetime.datetime.fromtimestamp(dest_stat.st_mtime)
                if abs(source_mod_time-dest_mod_time) > MAX_TIMESTAMP_DELTA_MILS:
                    copy = True

    return copy


def copy_and_modify(src_file, dst_file, pattern_and_replace_tuples):
    """
    Read a source file, apply replacements, and save to a target file

    :param src_file:    The source file to read and do the replacements
    :param dst_file:    The target file
    :param pattern_and_replace_tuples:  Pattern+Replacement tuples
    """
    if not os.path.exists(src_file):
        raise ValueError('Source file ({}) does not exist.'.format(src_file))

    with open(src_file,'r') as src_file_file:
        src_file_content = src_file_file.read()

    for pattern_str, replacement_str in pattern_and_replace_tuples:
        src_file_content = re.sub(pattern_str, replacement_str, src_file_content)

    with open(dst_file,'w') as dst_file:
        dst_file.write(src_file_content)


def calculate_file_hash(file_path):
    """
    Quickly compute a hash (md5) of a file
    :param file_path:   The file to hash
    :return:    The md5 hash result (hex string)
    """
    with open(file_path) as file_to_hash:
        file_content = file_to_hash.read()
    digest = hashlib.md5()
    digest.update(file_content)
    hash_result = digest.hexdigest()
    return hash_result


def calculate_string_hash(string_to_hash):
    """
    Quickly compute a hash (md5) of a string
    :param string_to_hash:  The string to hash
    :return:    The md5 hash result (hex string)
    """
    digest = hashlib.md5()
    digest.update(string_to_hash)
    hash_result = digest.hexdigest()
    return hash_result

def calculate_string_hash(string_to_hash):
    """
    Quickly compute a hash (md5) of a string
    :param file_path:
    :return:
    """
    digest = hashlib.md5()
    digest.update(string_to_hash)
    hash_result = digest.hexdigest()
    return hash_result



def copy_file_if_needed(source_path, dest_path):
    """
    Determine if we need to overwrite a file based on:
        1. File size
        2. File Hash
    :param source_path: source file path
    :param dest_path:   destination file path
    :return: True if the files are different and a copy is needed, otherwise False
    """
    copy = False
    if os.path.exists(source_path):
        if not os.path.exists(dest_path):
            # File does not exist, so always perform a copy
            copy = True
        elif os.path.isfile(source_path) and os.path.isfile(dest_path):
            # Only overwrite for files
            source_stat = os.stat(source_path)
            dest_stat = os.stat(dest_path)
            # Right now, ony compare the file sizes and mod times to detect if an overwrite is necessary
            if source_stat.st_size != dest_stat.st_size:
                copy = True
            else:
                source_hash = calculate_file_hash(source_path)
                dest_hash = calculate_file_hash(dest_path)
                copy = source_hash != dest_hash

    if copy:
        fast_copy2(source_path, dest_path)

    return copy


def copy_folder(src_folders, dst_folder, ignore_paths, status_format):
    """
    Copy a folder, overwriting existing files only if needed
    :param src_folders:  List of source folders to copy
    :param dst_folder:  The target folder destination
    :param ignore_paths:    list of items inside the folders to ignore
    :return:
    """

    def _copy_folder(src, dst, ignore):
        if not os.path.exists(src):
            return
        if not os.path.isdir(src):
            return
        if not os.path.exists(dst):
            os.makedirs(dst)

        for item in os.listdir(src):
            src_item = os.path.join(src,item)

            ignore_item = False
            for ignore_pattern in ignore:
                if fnmatch.fnmatch(src_item,ignore_pattern):
                    ignore_item = True
                    break
            if ignore_item:
                continue

            dst_item = os.path.join(dst,item)
            if os.path.isdir(src_item):
                _copy_folder(src_item,dst_item, ignore_paths)
            else:
                copy_file_if_needed(src_item, dst_item)
        return

    ignore_paths_normalized = [ os.path.normpath(os.path.abspath(ignore_path)) for ignore_path in ignore_paths]
    src_folders_normalized = [ os.path.normpath(src_folder) for src_folder in src_folders]
    for src_folder_normalized in src_folders_normalized:

        source_folder = os.path.abspath(src_folder_normalized)
        if source_folder in ignore_paths_normalized:
            continue
        target_folder = os.path.abspath(os.path.normpath(os.path.join(dst_folder, src_folder_normalized)))
        if status_format:
            print(status_format.format(src_folder_normalized))
        _copy_folder(source_folder, target_folder, ignore_paths)


def copy_files_to_folder(required_src_files, optional_src_files, dst_folder, status_format=None):
    """
    Copy files to a destination folder

    :param required_src_files:  List of required files relative to the current path
    :param optional_src_files:  List of optional files relative to the current path
    :param dst_folder:          Target folder
    :param status_format:       Optional format string to print the status of each copy
    """
    if not os.path.exists(dst_folder):
        os.makedirs(dst_folder)

    for src_file in required_src_files+optional_src_files:
        if not os.path.isfile(src_file):
            raise ValueError('"{}" is not a file.'.format(src_file))
        if not os.path.exists(src_file):
            if src_file in required_src_files:
                raise ValueError('Required file ({}) does not exist.'.format(src_file))
            else:
                continue
        source_file_abs_path = os.path.abspath(src_file)
        dest_file_abs_path = os.path.join(os.path.abspath(dst_folder), os.path.basename(src_file))
        if copy_file_if_needed(source_file_abs_path, dest_file_abs_path):
            if status_format:
                print(status_format.format(src_file))


# Special token used as a temp replacement to handle escapement of the '#' for non-standard '#'s during json parsing
PRESERVE_LITERAL_HASH_REPLACE_TOKEN = '_&%%&_'


def parse_json_file(json_file_path, allow_non_standard_comments=False):
    """
    Parse a json file into a utf-8 encoded python dictionary
    :param json_file_path:              The json file to parse
    :param allow_non_standard_comments: Allow non-standard comment ('#') tags in the file
    :return: Dictionary representation of the json file
    """

    def _decode_list(list_data):
        rv = []
        for item in list_data:
            if isinstance(item, unicode):
                item = item.encode('utf-8')
            elif isinstance(item, list):
                item = _decode_list(item)
            elif isinstance(item, dict):
                item = _decode_dict(item)
            rv.append(item)
        return rv

    def _decode_dict(dict_data):
        rv = {}
        for key, value in dict_data.iteritems():
            if isinstance(key, unicode):
                key = key.encode('utf-8')
            if isinstance(value, unicode):
                value = value.encode('utf-8')
            elif isinstance(value, list):
                value = _decode_list(value)
            elif isinstance(value, dict):
                value = _decode_dict(value)
            rv[key] = value
        return rv

    try:
        if allow_non_standard_comments:
            # If we are reading non-standard json files where we are accepting '#' as comment tokens, then the
            # file must have CR/LF characters and will be read in line by line.
            with open(json_file_path) as json_file:
                json_lines = json_file.readlines()
                json_file_content = ""
                for json_line in json_lines:
                    comment_index = json_line.find('#')
                    literal_pound_index = json_line.find('##')
                    if comment_index>=0 and comment_index != literal_pound_index:
                        processed_line = json_line.split('#')[0].strip()
                    else:
                        if literal_pound_index>=0:
                            processed_line = json_line.replace('##','#').strip()
                        else:
                            processed_line = json_line.strip()
                        
                    json_file_content += processed_line
        else:
            with open(json_file_path) as json_file:
                json_file_content = json_file.read()

        json_file_data = json.loads(json_file_content, object_hook=_decode_dict)
        return json_file_data
    except Exception as e:
        raise ValueError('Error reading {}: {}'.format(json_file_path, e.message))


def write_json_file(json_file_data, json_file_path):
    """

    :param json_file_data:
    :param json_file_path:
    :return:
    """
    try:
        with open(json_file_path,"w") as json_file:
            json_file_content = json.dumps(json_file_data,sort_keys=True,separators=(',',': '),indent=4)
            json_file.write(json_file_content)
    except Exception as e:
        raise ValueError('Error writing {}: {}'.format(json_file_path, e.message))


def get_dependencies_recursively_for_task_gen(ctx, task_generator):
    """
    Gets the explicit dependencies for a given task_generator and recursivesly
    gets the explicit dependencies for those task_generators. Explicit
    dependencies are looked for in the use, uselib, and libs attributes of the
    task_generator.

    :param ctx: Current context that is being executed
    :param task_generator: The task_generator to get the dependencies for
    :return: list of all the dependencies that the task_generator and its dependents have
    :rtype: set
    """
    if task_generator == None:
        return set()

    Logs.debug('utils: Processing task %s' % task_generator.name)

    dependencies = getattr(task_generator, 'dependencies', None)
    if not dependencies:
        dependencies = []
        uses = getattr(task_generator, 'use', [])
        uses_libs = getattr(task_generator, 'uselib', [])
        libs = getattr(task_generator, 'libs', [])

        for use_depend in uses:
            if use_depend not in dependencies:
                Logs.debug('utils: %s - has uses dependency of %s' % (task_generator.name, use_depend))
                dependencies += [use_depend]
                try:
                    depend_task_gen = ctx.get_tgen_by_name(use_depend)
                    dependencies.extend(get_dependencies_recursively_for_task_gen(ctx, depend_task_gen))
                except:
                    pass

        for use_depend in uses_libs:
            if use_depend not in dependencies:
                Logs.debug('utils: %s - has use_lib dependency of %s' % (task_generator.name, use_depend))
                dependencies += [use_depend]
                try:
                    depend_task_gen = ctx.get_tgen_by_name(use_depend)
                    dependencies.extend(get_dependencies_recursively_for_task_gen(ctx, depend_task_gen))
                except:
                    pass

        for lib in libs:
            if lib not in dependencies:
                Logs.debug('utils: %s - has lib dependency of %s' % (task_generator.name, use_depend))
                dependencies += [lib]

        task_generator.dependencies = set(dependencies)

    return task_generator.dependencies


"""
Parts of the build/package process require the ability to create directory junctions/symlinks to make sure some assets are properly
included in the build while maintaining as small of a memory footprint as possible (don't want to make copies).  Since we only care
about directories, we don't need the full power of os.symlink (doesn't work on windows anyway and writing one would require either
admin privileges or running something such as a VB script to create a shortcut to bypass the admin issue; neither of those options
are desirable).  The following functions are to make it explicitly clear we only care about directory links.
"""
def junction_directory(source, link_name):
    if not os.path.isdir(source):
        Logs.error("[ERROR] Attempting to make a junction to a file, which is not supported.  Unexpected behaviour may result.")
        return

    if Utils.unversioned_sys_platform() == "win32":
        cleaned_source_name = '"' + os.path.normpath(source) + '"'
        cleaned_link_name = '"' + os.path.normpath(link_name) + '"'

        # mklink generally requires admin privileges, however directory junctions don't.
        # subprocess.check_call will auto raise.
        subprocess.check_call('mklink /D /J %s %s' % (cleaned_link_name, cleaned_source_name), shell=True)
    else:
        os.symlink(source, link_name)


def remove_junction(junction_path):
    """
    Wrapper for correctly deleting a symlink/junction regardless of host platform
    """
    if Utils.unversioned_sys_platform() == "win32":
        os.rmdir(junction_path)
    else:
        os.unlink(junction_path)


def get_spec_dependencies(ctx, modules_spec, valid_gem_types):
    """
    Goes through all the modules defined in a spec and determines if they would
    be a dependency of the current game project set in the bootstrap.cfg file.

    :param ctx: Context that is being executed
    :type ctx: Context
    :param modules_spec: The name of the spec to get the modules from
    :type modules_spec: String
    :param valid_gem_types: The valid gem module types to include as dependencies
    :type gem_types: list of Gem.Module.Type
    """

    modules_in_spec = ctx.loaded_specs_dict[modules_spec]['modules']

    gem_name_list = [gem.name for gem in ctx.get_game_gems(ctx.project)]

    game_engine_module_dependencies = set()
    for module in modules_in_spec:
        Logs.debug('utils: Processing dependency {}'.format(module))

        try:
            module_task_gen = ctx.get_tgen_by_name(module)
        except:
            module_task_gen = None

        if module_task_gen is None:
            Logs.debug('utils: -> could not find a task gen for {} so ignoring it'.format(module))
        else:
            # If the ../Resources path exists then the module name must match
            # our project name otherwise the module is for a different game
            # project. 
            path = module_task_gen.path.make_node("../Resources")
            if not os.path.exists(path.abspath()) or module_task_gen.name == ctx.project:
                # Need to check if the module is a gem, and if so part of the game gems...
                if ctx.is_gem(module_task_gen.name):
                    if module_task_gen.name in gem_name_list:
                        Logs.debug('utils: -> adding module becaue it is a used gem') 
                        a_list = [gem_module for gem in GemManager.GetInstance(ctx).gems if gem.name == module for gem_module in gem.modules if gem_module.type in valid_gem_types or 'all' in valid_gem_types]
                        if len(a_list) > 0:
                            game_engine_module_dependencies.add(a_list[0].file_name)
                            module_dependencies = get_dependencies_recursively_for_task_gen(ctx, module_task_gen)
                            for module_dep in module_dependencies:
                                if module_dep not in game_engine_module_dependencies:
                                    Logs.debug('utils: -> adding module dependency {}'.format(module_dep))
                                    game_engine_module_dependencies.add(module_dep)
                    else:
                        Logs.debug('utils: -> skipping gem {} because it is not in the project gem list'.format(module))

                else:
                    if getattr(module_task_gen, 'is_editor_plugin', False) and modules_spec not in ['all', 'engine_and_editor']:
                        Logs.debug('utils: -> skipping module because it is an editor plugin and we are not including them.')
                        continue

                    Logs.debug('utils: -> adding the module and its dependencies because it is a module for the game project.')
                    task_project_name = getattr(module_task_gen, 'project_name', None)
                    game_engine_module_dependencies.add(module)
                    module_dependencies = get_dependencies_recursively_for_task_gen(ctx, module_task_gen)
                    for module_dep in module_dependencies:
                        if module_dep not in game_engine_module_dependencies:
                            Logs.debug('utils: -> adding module dependency {}'.format(module_dep))
                            game_engine_module_dependencies.add(module_dep)

    return game_engine_module_dependencies
	
def write_auto_gen_header(file):
    name, ext = os.path.splitext(os.path.basename(file.name))

    comment_str = '//'
    if name in ('wscript', 'CMakeLists') or ext in ('.py', '.properties'):
        comment_str = '#'

    full_line = comment_str * (64 / len(comment_str))

    file.write('{}\n'.format(full_line))
    file.write('{} This file was automatically created by WAF\n'.format(comment_str))
    file.write('{} WARNING! All modifications will be lost!\n'.format(comment_str))
    file.write('{}\n'.format(full_line))


ROLE_TO_DESCRIPTION_MAP = {
    "compileengine":"Compile the engine and asset pipeline",
    "compilegame":"Compile the game code",
    "compilesandbox":"Compile Lumberyard Editor and tools",
    "runeditor":"Run the Lumberyard Editor and tools",
    "rungame":"Run your game project",
    "vc120":"Visual Studio 2013",
    "vc140":"Visual Studio 2015",
    "vc141":"Visual Studio 2017"
}


def convert_roles_to_setup_assistant_description(sdk_roles):
    required_checks = []
    for sdk_role in sdk_roles or []:
        if sdk_role in ROLE_TO_DESCRIPTION_MAP:
            required_checks.append("'{}'".format(ROLE_TO_DESCRIPTION_MAP[sdk_role]))
        else:
            required_checks.append("Option '{}'".format(sdk_role))

    return required_checks


def is_value_true(value):
    """
    Helper function to attempt to evaluate a boolean (true) from a value

    :param value:   The value to inspect
    :return:        True or False
    """

    if isinstance(value, str):
        lower_value = value.lower().strip()
        if lower_value in ('true', 't', 'yes', 'y', '1'):
            return True
        elif lower_value in ('false', 'f', 'no', 'n', '0'):
            return False
        else:
            raise ValueError("Invalid boolean string ('')".format(value))
    elif isinstance(value,bool):
        return value
    elif isinstance(value,int):
        return value != 0
    else:
        raise ValueError("Cannot deduce a boolean result from value {}".format(str(value)))


def read_compile_settings_file(settings_file, configuration):
    """
    Read in a compile settings file and extract the dictionary of known values
    :param settings_file:
    :param configuration:
    :return:
    """

    def _read_config_item(config_settings, key, evaluated_keys={}, pending_keys=[]):

        if key in evaluated_keys:
            return evaluated_keys[key]

        read_values = config_settings[key]
        evaluated_values = []
        for read_value in read_values:
            if read_value.startswith('@'):
                alias_key = read_value[1:]
                if alias_key not in config_settings:
                    raise ValueError("Invalid alias key '{}' in section '{}'".format(read_value, key))
                if alias_key in pending_keys:
                    raise ValueError("Invalid alias key '{}' in section '{}' creates a circular reference.".format(read_value, key))
                elif alias_key not in evaluated_keys:
                    pending_keys.append(key)
                    evaluated_values += _read_config_item(config_settings, alias_key, evaluated_keys, pending_keys)
                    pending_keys.remove(key)
                else:
                    evaluated_values += evaluated_keys[alias_key]
            else:
                evaluated_values.append(read_value)

        evaluated_keys[key] = evaluated_values
        return evaluated_values

    def _merge_config(left_config, right_config):
        if right_config:
            merged_config = {}
            for left_key, left_values in left_config.items():
                merged_config[left_key] = left_values[:]
            for right_key, right_values in right_config.items():
                if right_key in merged_config:
                    merged_config[right_key] += [merge_item for merge_item in right_values if merge_item not in merged_config[right_key]]
                else:
                    merged_config[right_key] = right_values[:]
            return merged_config
        else:
            return left_config

    def _read_config_section(settings, section_name, default_section_name):

        if default_section_name:
            default_dict = _read_config_section(settings, default_section_name, None)
        else:
            default_dict = None

        if section_name not in settings:
            return default_dict

        section_settings = settings.get(section_name)
        result = {}
        for key in section_settings.keys():
            result[key] = _read_config_item(section_settings, key)
        merged_result = _merge_config(result, default_dict)
        return merged_result

    settings_json = parse_json_file(settings_file, allow_non_standard_comments=True)

    result = _read_config_section(settings_json, configuration, 'common')
    return result
