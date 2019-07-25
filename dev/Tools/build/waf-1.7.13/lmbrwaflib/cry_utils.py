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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#
import sys, subprocess
from waflib import Configure, Logs, Utils, Options, ConfigSet
from waflib.Configure import conf, deprecated
from waflib import Logs, Node, Errors
from waflib.Scripting import _is_option_true
from waflib.TaskGen import after_method, before_method, feature, extension, taskgen_method
from waflib.Task import Task, RUN_ME, SKIP_ME
from waf_branch_spec import WAF_FILE_GLOB_WARNING_THRESHOLD

import utils
import json
import os
import re

winreg_available = True
try:
    import _winreg
except ImportError:
    winreg_available = False
    pass

###############################################################################
WAF_EXECUTABLE = 'lmbr_waf.bat'

REGEX_PATH_ALIAS = re.compile('@(.*)@')

#############################################################################
# Helper method for getting the host platform's command line character limit
def get_command_line_limit():
    arg_max = 16384 # windows command line length limit

    if Utils.unversioned_sys_platform() != "win32":
        arg_max = int(subprocess.check_output(["getconf", "ARG_MAX"]).strip())
        env_size = len(subprocess.check_output(['env']))
        # on platforms such as mac, ARG_MAX encodes the amount of room you have for args, but in BYTES.
        # as well as for environment itself.
        arg_max = arg_max - (env_size * 2) # env has lines which are null terminated, reserve * 2 for safety
        # finally, assume we're using some sort of unicode encoding here, with each character costing us at least two bytes
        arg_max = arg_max / 2

    return arg_max

def append_kw_entry(kw,key,value):
    """
    Helper method to add a kw entry

    :param kw:      The kw dictionary to apply to
    :param key:     The kw entry to add/set
    :param value:   The value (or values if its a list) to set
    """
    if not key in kw:
        kw[key] = []
    if isinstance(value,list):
        kw[key] += value
    else:
        kw[key] += [value]


def append_unique_kw_entry(kw,key,value):
    """
    Helper method to add a unique kw entry

    :param kw:      The kw dictionary to apply to
    :param key:     The kw entry to add/set
    :param value:   The value (or values if its a list) to set
    """
    if not key in kw:
        kw[key] = []
    if isinstance(value,list):
        for item in value:
            append_unique_kw_entry(kw,key,item)
    else:
        if value not in kw[key]:
            kw[key].append(value)


#############################################################################
# Helper method to add a value to a keyword and create the keyword if it is not there
# This method will also make sure that a list of items is appended to the kw key
@conf
def prepend_kw_entry(kw,key,value):
    if not key in kw:
        kw[key] = []
    if isinstance(value,list):
        kw[key] = value + kw[key]
    else:
        kw[key] = [value] + kw[key]

#############################################################################
#  Utility function to add an item to a list uniquely.  This will not reorder the list, rather it will discard items
#  that already exist in the list.  This will also flatten the input into a single list if there are any embedded
#  lists in the input
@conf
def append_to_unique_list(unique_list,x):

    if isinstance(x,list):
        for y in x:
            append_to_unique_list(unique_list, y)
    else:
        if not x in unique_list:
            unique_list.append(x)


#############################################################################
# Utility method to perform remove duplicates from a list while preserving the order (ie not using sets)
@conf
def clean_duplicates_in_list(input_list,debug_list_name):
    clean_list = []
    distinct_set = set()
    for item in input_list:
        try:
            if item not in distinct_set:
                clean_list.append(item)
                distinct_set.add(item)
            else:
                Logs.debug('lumberyard: Duplicate item {} detected in \'{}\''.format(item,debug_list_name))
        except TypeError as type_error:
            Logs.error("Invalid item '{}' in '{}'. May be caused by incorrect trailing comma in wscript.".format(item, debug_list_name))
    return clean_list


#############################################################################
# Utility function to make sure that kw inputs are 'lists'
def sanitize_kw_input_lists(list_keywords, kw):
    for key in list_keywords:
        if key in kw:
            if not isinstance(kw[key],list):
                kw[key] = [ kw[key] ]


def flatten_list(input):
    """
    Given a list or list of lists, this method will flatten any list structure into a single list
    :param input:   Artibtrary list to flatten.  If not a list, then the input will be returned as a list of that single item
    :return: Flattened list
    """
    if isinstance(input,list):
        result = []
        for item in input:
            result += flatten_list(item)
        return result
    else:
        return [input]


@conf
def is_option_true(ctx, option_name):
    """ Util function to better intrepret all flavors of true/false """
    return _is_option_true(ctx.options, option_name)


@conf
def is_boolean_option(ctx, option_name):
    try:
        _is_option_true(ctx.options, option_name)
        return True
    except Errors.WafError:
        return False

    
#############################################################################
#############################################################################
# Helper functions to handle error and warning output
@conf
def cry_error(conf, msg):
    conf.fatal("error: %s" % msg) 
    
@conf
def cry_file_error(conf, msg, filePath, lineNum = 0 ):
    if isinstance(filePath, Node.Node):
        filePath = filePath.abspath()
    if not os.path.isabs(filePath):
        filePath = conf.path.make_node(filePath).abspath()
    conf.fatal('%s(%s): error: %s' % (filePath, lineNum, msg))

@conf
def cry_warning(conf, msg):
    Logs.warn("warning: %s" % msg)

@conf
def cry_file_warning(conf, msg, filePath, lineNum = 0 ):
    Logs.warn('%s(%s): warning: %s.' % (filePath, lineNum, msg))

#############################################################################
#############################################################################
# Helper functions to json file parsing and validation

def _decode_list(data):
    rv = []
    for item in data:
        if isinstance(item, unicode):
            item = item.encode('utf-8')
        elif isinstance(item, list):
            item = _decode_list(item)
        elif isinstance(item, dict):
            item = _decode_dict(item)
        rv.append(item)
    return rv

def _decode_dict(data):
    rv = {}
    for key, value in data.iteritems():
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

@conf
def parse_json_file(conf, file_node):
    try:
        file_content_raw = file_node.read()
        file_content_parsed = json.loads(file_content_raw, object_hook=_decode_dict)
        return file_content_parsed
    except Exception as e:
        line_num = 0
        exception_str = str(e)

        # Handle invalid last entry in list error
        if "No JSON object could be decoded" in exception_str:
            cur_line = ""
            prev_line = ""
            file_content_by_line = file_content_raw.split('\n')
            for lineIndex, line in enumerate(file_content_by_line):

                # Sanitize string
                cur_line = ''.join(line.split())

                # Handle empty line
                if not cur_line:
                    continue

                # Check for invalid JSON schema
                if any(substring in (prev_line + cur_line) for substring in [",]", ",}"]):
                    line_num = lineIndex
                    exception_str = 'Invalid JSON, last list/dictionary entry should not end with a ",". [Original exception: "%s"]' % exception_str
                    break;

                prev_line = cur_line

        # If exception has not been handled yet
        if not line_num:
            # Search for 'line' in exception and output pure string
            exception_str_list = exception_str.split(" ")
            for index, elem in enumerate(exception_str_list):
                if elem == 'line':
                    line_num = exception_str_list[index+1]
                    break

        # Raise fatal error
        conf.cry_file_error(exception_str, file_node.abspath(), line_num)

###############################################################################

###############################################################################
# Function to generate the EXE_VERSION_INFO defines
@before_method('process_source')
@feature('c', 'cxx')
def apply_version_info(self):

    if getattr(self, 'is_launcher', False) or getattr(self, 'is_dedicated_server', False):
        version = self.bld.get_game_project_version()
    else:
        version = self.bld.get_lumberyard_version()

    # At this point the version number format should be vetted so no need to check again, assume we have numbers

    parsed_version_parts = version.split('.')
    if len(parsed_version_parts) <= 1:
        Logs.warn('Invalid build version (%s), falling back to (1.0.0.1)' % version )
        version_parts = ['1', '0', '0', '1']

    version_parts = [
        parsed_version_parts[0] if len(parsed_version_parts) > 0 else '1',
        parsed_version_parts[1] if len(parsed_version_parts) > 1 else '0',
        parsed_version_parts[2] if len(parsed_version_parts) > 2 else '0',
        parsed_version_parts[3] if len(parsed_version_parts) > 3 else '0'
    ]

    self.env.append_value('DEFINES', 'EXE_VERSION_INFO_0=' + version_parts[0])
    self.env.append_value('DEFINES', 'EXE_VERSION_INFO_1=' + version_parts[1])
    self.env.append_value('DEFINES', 'EXE_VERSION_INFO_2=' + version_parts[2])
    self.env.append_value('DEFINES', 'EXE_VERSION_INFO_3=' + version_parts[3])

lmbr_override_target_map = {'SetupAssistant', 'SetupAssistantBatch'}


@conf
def get_output_folders(self, platform, configuration, ctx=None, target=None):
    output_paths = []

    # if target is provided, see if it's in the override list, if it is then override with specific logic
    if target is not None:
        # override for Tools/LmbrSetup folder
        if target in lmbr_override_target_map:
            output_paths = [self.get_lmbr_setup_tools_output_folder(platform, configuration)]

    if ctx is None:
        ctx = self

    # if no overwrite provided
    if len(output_paths) == 0:
        # check if output_folder is defined
        output_paths = getattr(ctx, 'output_folder', None)
        if output_paths:
            if not isinstance(output_paths, list):
                output_paths = [output_paths]
        # otherwise use default path generation rule
        else:
            output_paths = [self.get_output_target_folder(platform, configuration)]

    output_nodes = []
    for path in output_paths:
        # Correct handling for absolute paths
        if os.path.isabs(path):
            output_nodes.append(self.root.make_node(path))
        else:
            # For relative path, prefix binary output folder with game project folder
            output_nodes.append(self.launch_node().make_node(path))

    return output_nodes

@conf
def get_standard_host_output_folders(self):
    '''
    Get a list of the standard output folders for the native host platform
    '''
    host = Utils.unversioned_sys_platform()
    if host == 'win32':
        host = 'win'

    output_nodes = []
    launch_node = self.launch_node()

    host_targets = self.get_platform_aliases(host)
    current_platform = self.env['PLATFORM']

    # prefer the current target if it is a host
    if current_platform in host_targets:
        host_targets = [current_platform]

    # filter down to like visual studio versions
    elif 'vs20' in current_platform:
        vs_version = current_platform.split('_')[-1]
        host_targets = [host for host in host_targets if vs_version in host]

    for target in host_targets:
        if not self.is_target_platform_enabled(target):
            continue

        output_nodes.extend(self.get_output_folders(target, 'profile'))

    return output_nodes


@conf
def read_file_list(bld, file):
    """
    Read and process a file list file (.waf_file) and manage duplicate files and possible globbing patterns to prepare
    the list for injestion by the project

    :param bld:     The build context
    :param file:    The .waf_file file list to process
    :return:        The processed list file
    """

    if not os.path.isfile(os.path.join(bld.path.abspath(), file)):
        raise Errors.WafError("Invalid waf file list file: {}.  File not found.".format(file))

    # Manage duplicate files and glob hits
    dup_set = set()
    glob_hits = 0

    waf_file_node = bld.path.make_node(file)
    waf_file_node_abs = waf_file_node.abspath()
    base_path_abs = waf_file_node.parent.abspath()

    if not os.path.exists(waf_file_node_abs):
        raise Errors.WafError('Invalid WAF file list: {}'.format(waf_file_node_abs))

    def _invalid_alias_callback(alias_key):
        error_message = "Invalid alias '{}' specified in {}".format(alias_key, file)
        raise Errors.WafError(error_message)

    def _alias_not_enabled_callback(alias_key, roles):
        required_checks = utils.convert_roles_to_setup_assistant_description(roles)
        error_message = "3rd Party alias '{}' specified in {} is not enabled. Make sure that at least one of the " \
                        "following items are checked in SetupAssistant: [{}]".format(alias_key, file, ', '.join(required_checks))
        raise Errors.WafError(error_message)

    def _determine_vs_filter(input_rel_folder_path, input_filter_name, input_filter_pattern):
        """
        Calculate the vvs filter based on the resulting relative path, the input filter name,
        and the pattern used to derive the input relative path
        """
        vs_filter = input_filter_name

        if len(input_rel_folder_path) > 0:
            # If the resulting relative path has a subfolder, the base the filter on the following conditions

            if input_filter_name.lower()=='root':
                # This is the root folder, use the relative folder subpath as the filter
                vs_filter = input_rel_folder_path
            else:
                # This is a named filter, the filter will place all results under this filter
                pattern_dirname = os.path.dirname(input_filter_pattern)
                if len(pattern_dirname) > 0:
                    if input_rel_folder_path != pattern_dirname:
                        # Strip out the base of the filter name
                        vs_filter = input_filter_name + '/' + input_rel_folder_path.replace(pattern_dirname, '')
                    else:
                        vs_filter = input_filter_name
                else:
                    vs_filter = input_filter_name + '/' + input_rel_folder_path

        return vs_filter

    def _process_glob_entry(glob_content, filter_name, current_uber_dict):
        """
        Process a glob content from the input file list
        """
        if 'pattern' not in glob_content:
            raise Errors.WafError('Missing keyword "pattern" from the glob entry"')

        original_pattern = glob_content.pop('pattern').replace('\\', '/')
        if original_pattern.startswith('@'):

            ALIAS_PATTERN = re.compile('@.*@')
            alias_match = ALIAS_PATTERN.search(original_pattern)
            if alias_match:
                alias = alias_match.group(0)[1:-1]
                pattern = original_pattern[len(alias)+2:]
                if alias=='ENGINE':
                    search_node = bld.path
                else:
                    search_node = bld.root.make_node(bld.ThirdPartyPath(alias))
            else:
                pattern = original_pattern
                search_node = waf_file_node.parent
        else:
            pattern = original_pattern
            search_node = waf_file_node.parent

        while pattern.startswith('../'):
            pattern = pattern[3:]
            search_node = search_node.parent

        glob_results = search_node.ant_glob(pattern, **glob_content)

        for globbed_file in glob_results:

            rel_path = globbed_file.path_from(waf_file_node.parent).replace('\\', '/')
            abs_path = globbed_file.abspath().replace('\\', '/')
            rel_folder_path = os.path.dirname(rel_path)

            vs_filter = _determine_vs_filter(rel_folder_path, filter_name, original_pattern)

            if vs_filter not in current_uber_dict:
                current_uber_dict[vs_filter] = []
            if abs_path in dup_set:
                Logs.warn("[WARN] File '{}' specified by the pattern '{}' in waf file '{}' is a duplicate.  It will be ignored"
                          .format(abs_path, original_pattern, waf_file_node_abs))
            else:
                current_uber_dict[vs_filter].append(rel_path)
                dup_set.add(abs_path)

    def _clear_empty_uber_dict(current_uber_dict):
        """
        Perform house clean in case glob pattern overrides move all files out of a 'root' group.
        """
        empty_filters = []
        for filter_name, filter_contents in current_uber_dict.items():
            if len(filter_contents)==0:
                empty_filters.append(filter_name)
        for empty_filter in empty_filters:
            current_uber_dict.pop(empty_filter)
        return current_uber_dict

    def _process_uber_dict(uber_section, uber_dict):
        """
        Process each uber dictionary value
        """
        processed_uber_dict = {}

        for filter_name, filter_contents in uber_dict.items():
            for filter_content in filter_contents:

                if isinstance(filter_content, str):

                    if '*' in filter_content or '?' in filter_content:
                        # If this is a raw glob pattern, stuff it into the expected glob dictionary
                        _process_glob_entry(dict(pattern=filter_content), filter_name, processed_uber_dict)
                    else:
                        # This is a straight up file reference.
                        # Do any processing on an aliased reference
                        if filter_content.startswith('@'):
                            processed_path = bld.PreprocessFilePath(filter_content, _invalid_alias_callback,
                                                                    _alias_not_enabled_callback)
                        else:
                            processed_path = os.path.normpath(os.path.join(base_path_abs, filter_content))

                        if not os.path.exists(processed_path):
                            Logs.warn("[WARN] File '{}' specified in '{}' does not exist.  It will be ignored"
                                      .format(processed_path, waf_file_node_abs))
                        elif not os.path.isfile(processed_path):
                            Logs.warn("[WARN] Path '{}' specified in '{}' is a folder, only files or glob patterns are "
                                      "allowed.  It will be ignored"
                                      .format(processed_path, waf_file_node_abs))
                        elif processed_path in dup_set:
                            Logs.warn("[WARN] File '{}' specified in '{}' is a duplicate.  It will be ignored"
                                      .format(processed_path, waf_file_node_abs))
                        else:
                            if filter_name not in processed_uber_dict:
                                processed_uber_dict[filter_name] = []
                            processed_uber_dict[filter_name].append(processed_path)
                            dup_set.add(processed_path)

                elif isinstance(filter_content, dict):
                    # Dictionaries automatically go through the glob pattern working
                    _process_glob_entry(filter_content, filter_name, processed_uber_dict)
                else:
                    raise Errors.WafError("Invalid entry '{}' in file '{}', section '{}/{}'"
                                          .format(filter_content, file, uber_section, filter_name))

        return _clear_empty_uber_dict(processed_uber_dict)

    def _get_cached_file_list():
        """
        Calculate the location of the cached waf_files path
        """
        bld_node = file_node.get_bld()
        return bld_node.abspath()

    file_node = bld.path.make_node(file)

    if not bld.is_option_true('enable_dynamic_file_globbing'):
        # Unless this is a configuration context (where we want to always calculate any potential glob patterns in the
        # waf_file list) check if the file list exists from any previous waf configure.  If the waf_files had changed
        # in between builds, auto-configure will pick up that change and force a re-write of the waf_files list
        processed_waf_files_path = _get_cached_file_list()
        if os.path.exists(processed_waf_files_path) and not isinstance(bld, Configure.ConfigurationContext):
            processed_file_list = utils.parse_json_file(processed_waf_files_path)
            return processed_file_list

    # Read the source waf_file list
    source_file_list = bld.parse_json_file(file_node)

    # Prepare a processed waf_file list
    processed_file_list = {}

    for uber_file_entry, uber_file_dict in source_file_list.items():
        processed_file_list[uber_file_entry] = _process_uber_dict(uber_file_entry, uber_file_dict)
        pass

    if glob_hits > WAF_FILE_GLOB_WARNING_THRESHOLD:
        Logs.warn('[WARN] Source file globbing for waf file {} resulted in over {} files.  If this is expected, '
                  'consider increasing the warning limit value WAF_FILE_GLOB_WARNING_THRESHOLD in waf_branch_spec.py'
                  .format(file_node.abspath(), WAF_FILE_GLOB_WARNING_THRESHOLD))

    if not bld.is_option_true('enable_dynamic_file_globbing') and isinstance(bld, Configure.ConfigurationContext):
        # If dynamic file globbing is off, then store the cached file list during every configure command
        processed_waf_files_path = _get_cached_file_list()
        processed_waf_files_dir = os.path.dirname(processed_waf_files_path)
        if not os.path.exists(processed_waf_files_dir):
            os.makedirs(processed_waf_files_dir)
        utils.write_json_file(processed_file_list, processed_waf_files_path)

    return processed_file_list


@conf
def get_platform_and_configuration(bld):
    # Assume that the configuration is at the end:
    # Keep 'dedicated' as part of the configuration.
    # extreme example: 'win_x64_vs2012_debug_dedicated'

    elements = bld.variant.split('_')
    idx = -1
    if 'dedicated' in elements:
        idx -= 1
    if 'test' in elements:
        idx -= 1

    platform = '_'.join(elements[:idx])
    configuration = '_'.join(elements[idx:])

    return (platform, configuration)


###############################################################################
@conf
def target_clean(self):

    tmp_targets = self.options.targets[:]
    to_delete = []
    # Sort of recursive algorithm, find all outputs of targets
    # Repeat if new targets were added due to use directives
    while len(tmp_targets) > 0:
        new_targets = []

        for tgen in self.get_all_task_gen():
            tgen.post()
            if not tgen.target in tmp_targets:
                continue

            for t in tgen.tasks:
                # Collect outputs
                for n in t.outputs:
                    if n.is_child_of(self.bldnode):
                        to_delete.append(n)
            # Check for use flag
            if hasattr(tgen, 'use'):
                new_targets.append(tgen.use)
        # Copy new targets
        tmp_targets = new_targets[:]

    # Final File list to delete
    to_delete = list(set(to_delete))
    for file_to_delete in to_delete:
        file_to_delete.delete()

###############################################################################
@conf
def modules_clean(self, modules):

    tmp_modules = modules[:]
    to_delete = []
    # Sort of recursive algorithm, find all outputs of supplied modules
    # Repeat if new targets were added due to use directives
    while len(tmp_modules) > 0:
        new_targets = []

        for tgen in self.get_all_task_gen():
            tgen.post()
            if not tgen.target in tmp_modules:
                continue

            for task in tgen.tasks:
                # Collect outputs
                for task_output in task.outputs:
                    if task_output.is_child_of(self.bldnode):
                        to_delete.append(task_output)

            # Check against codegen tasks so that their outputs are accounted for (if known)
            for azcg_task in tgen.bld.get_group('az_code_gen_group'):
                if azcg_task.generator == tgen:
                    outputs = azcg_task.azcg_get('AZCG_OUTPUTS', [])
                    for azcg_output in outputs:
                        if azcg_output.is_child_of(self.bldnode):
                            to_delete.append(azcg_output)

            # Check for use flag
            if hasattr(tgen, 'use'):
                new_targets.append(tgen.use)
        # Copy new targets
        tmp_modules = new_targets[:]

    # Final File list to delete
    to_delete = list(set(to_delete))
    for file_to_delete in to_delete:
            file_to_delete.delete()

###############################################################################
@conf
def clean_output_targets(self):

    to_delete = []
    is_msvc = False
    for base_output_folder_node in self.get_output_folders(self.env['PLATFORM'],self.env['CONFIGURATION']):

        # Go through the task generators
        for tgen in self.get_all_task_gen():

            is_msvc = tgen.env['CXX_NAME'] == 'msvc'

            # collect only shlibs and programs
            if not hasattr(tgen,'_type') or not hasattr(tgen,'env'):
                continue

            # determine the proper target extension pattern
            if tgen._type=='shlib' and tgen.env['cxxshlib_PATTERN']!='':
                target_ext_PATTERN=tgen.env['cxxshlib_PATTERN']
            elif tgen._type=='program' and tgen.env['cxxprogram_PATTERN']!='':
                target_ext_PATTERN=tgen.env['cxxprogram_PATTERN']
            else:
                continue

            target_output_folder_nodes = []

            # Determine if there is a sub folder
            if hasattr(tgen, 'output_sub_folder'):
                target_output_folder_nodes.append(base_output_folder_node.make_node(tgen.output_sub_folder))
            else:
                target_output_folder_nodes.append(base_output_folder_node)

            if hasattr(tgen, 'output_folder'):
                target_output_folder_nodes.append(tgen.bld.root.make_node(tgen.output_folder))

            # Determine if there are copy sub folders
            target_output_copy_folder_items = []
            target_output_copy_folder_attr = getattr(tgen, 'output_sub_folder_copy', None)
            if isinstance(target_output_copy_folder_attr, str):
                target_output_copy_folder_items.append(base_output_folder_node.make_node(target_output_copy_folder_attr))
            elif isinstance(target_output_copy_folder_attr, list):
                for target_output_copy_folder_attr_item in target_output_copy_folder_attr:
                    if isinstance(target_output_copy_folder_attr_item, str):
                        target_output_copy_folder_items.append(base_output_folder_node.make_node(target_output_copy_folder_attr_item))

            for target_output_folder_node in target_output_folder_nodes:

                target_name = getattr(tgen, 'output_file_name', tgen.get_name())
                delete_target = target_output_folder_node.make_node(target_ext_PATTERN % str(target_name))
                to_delete.append(delete_target)
                for target_output_copy_folder_item in target_output_copy_folder_items:
                    delete_target_copy = target_output_copy_folder_item.make_node(target_ext_PATTERN % str(target_name))
                    to_delete.append(delete_target_copy)

                # If this is an MSVC build, add pdb cleaning just in case
                if is_msvc:
                    delete_target_pdb = target_output_folder_node.make_node('%s.pdb' % str(target_name))
                    to_delete.append(delete_target_pdb)
                    for target_output_copy_folder_item in target_output_copy_folder_items:
                        delete_target_copy = target_output_copy_folder_item.make_node('%s.pdb' % str(target_name))
                        to_delete.append(delete_target_copy)

        # Go through GEMS and add possible gems components
        gems_output_names = set()
        if self.options.project_spec in self.loaded_specs_dict:
            spec_dict = self.loaded_specs_dict[self.options.project_spec]
            if 'projects' in spec_dict:
                for project in spec_dict['projects']:
                    for gem in self.get_game_gems(project):
                        gems_output_names.update(module.file_name for module in gem.modules)

        gems_target_ext_PATTERN=self.env['cxxshlib_PATTERN']

        for gems_output_name in gems_output_names:
            gems_delete_target = base_output_folder_node.make_node(gems_target_ext_PATTERN % gems_output_name)
            to_delete.append(gems_delete_target)
            if is_msvc:
                gems_delete_target_pdb = base_output_folder_node.make_node('%s.pdb' % str(gems_output_name))
                to_delete.append(gems_delete_target_pdb)

    for file in to_delete:
        if os.path.exists(file.abspath()):
            try:
                if self.options.verbose >= 1:
                    Logs.info('Deleting {0}'.format(file.abspath()))
                file.delete()
            except:
                Logs.warn("Unable to delete {0}".format(file.abspath()))


@feature('link_to_output_folder')
@after_method('process_source')
def link_to_output_folder(self):
    """
    Task Generator for tasks which generate symbolic links from the source to the dest folder
    """
    return # Disabled for now

    if self.bld.env['PLATFORM'] == 'project_generator':
        return # Dont create links during project generation

    if sys.getwindowsversion()[0] < 6:
        self.bld.fatal('not supported')

    # Compute base relative path (from <Root> to taskgen wscript
    relative_base_path = self.path.path_from(self.bld.path)

    # TODO: Need to handle absolute path here correctly
    spec_name = self.bld.options.project_spec
    for project in self.bld.active_projects(spec_name):
        project_folder = self.bld.project_output_folder(project)
        for file in self.source:
            # Build output folder
            relativ_file_path = file.path_from(self.path)

            output_node = self.bld.path.make_node(project_folder)
            output_node = output_node.make_node(relative_base_path)
            output_node = output_node.make_node(relativ_file_path)

            path = os.path.dirname( output_node.abspath() )
            if not os.path.exists( path ):
                os.makedirs( path )

            self.create_task('create_symbol_link', file, output_node)

import ctypes
###############################################################################
# Class to handle copying of the final outputs into the Bin folder
class create_symbol_link(Task):
    color = 'YELLOW'

    def run(self):
        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # Make output writeable
        try:
            os.chmod(tgt, 493) # 0755
        except:
            pass

        try:
            kdll = ctypes.windll.LoadLibrary("kernel32.dll")
            res = kdll.CreateSymbolicLinkA(tgt, src, 0)
        except:
            self.generator.bld.fatal("File Link Error (%s -> %s( (%s)" % (src, tgt, sys.exc_info()[0]))

        return 0

    def runnable_status(self):
        if super(create_symbol_link, self).runnable_status() == -1:
            return -1

        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # If there any target file is missing, we have to copy
        try:
            stat_tgt = os.stat(tgt)
        except OSError:
            return RUN_ME

        # Now compare both file stats
        try:
            stat_src = os.stat(src)
        except OSError:
            pass
        else:
            # same size and identical timestamps -> make no copy
            if stat_src.st_mtime >= stat_tgt.st_mtime + 2:
                return RUN_ME

        # Everything fine, we can skip this task
        return SKIP_ME


###############################################################################
@feature('cxx', 'c', 'cprogram', 'cxxprogram', 'cshlib', 'cxxshlib', 'cstlib', 'cxxstlib')
@after_method('apply_link')
def add_compiler_dependency(self):
    """ Helper function to ensure each compile task depends on the compiler """
    if self.env['PLATFORM'] == 'project_generator':
        return

    # Create nodes for compiler and linker
    c_compiler_node = self.bld.root.make_node( self.env['CC'] )
    cxx_compiler_node = self.bld.root.make_node( self.env['CXX'] )
    linker_node = self.bld.root.make_node( self.env['LINK'] )

    # Let all compile tasks depend on the compiler
    for t in getattr(self, 'compiled_tasks', []):
        if os.path.isabs( self.env['CC'] ):
            append_to_unique_list(t.dep_nodes, c_compiler_node);
        if os.path.isabs( self.env['CXX'] ):
            append_to_unique_list(t.dep_nodes, cxx_compiler_node);

    # Let all link tasks depend on the linker
    if getattr(self, 'link_task', None):
        if os.path.isabs(  self.env['LINK'] ):
            self.link_task.dep_nodes.append(linker_node)


def get_configuration(ctx, target):
    """
    Get the configuration, but apply the overwrite is specified for a target
    :param ctx:     The context containing the env
    :param target:  The target name to check
    :return:        The configuration value
    """
    if target in ctx.env['CONFIG_OVERWRITES']:
        return ctx.env['CONFIG_OVERWRITES'][target]
    return ctx.env['CONFIGURATION']

def compare_config_sets(left, right, deep_compare = False):
    """
    Helper method to do a basic comparison of different config sets (env)

    :param left:            The left config set to compare
    :param right:           The right config set to compare
    :param deep_compare:    Option to do a deeper value comparison
    :return:    True if the two config sets are equal, False if not
    """

    # If one or the other value is None, return false.  If BOTH are none, return True
    if left is None:
        if right is None:
            return True
        else:
            return False
    elif right is None:
        return False

    # Compare the number of keys
    left_keys = left.keys()
    right_keys = right.keys()
    if len(left_keys) != len(right_keys):
        return False
    key_len = len(left_keys)
    for i in range(0, key_len):

        # Compare each config key entry

        # Compare the key name
        if left_keys[i] != right_keys[i]:
            return False

        # Compare the key value
        left_values = left_keys[i]
        right_values = right_keys[i]

        if isinstance(left_values, list):
            if isinstance(right_values, list):
                # The items is a list
                left_value_count = len(left_values)
                right_value_count = len(right_values)
                if left_value_count != right_value_count:
                    return False
                if deep_compare:
                    for j in range(0,left_value_count):
                        left_value = left_values[j]
                        right_value = right_values[j]
                        if left_value!=right_value:
                            return False
            else:
                # The left and right types mismatch
                return False

        elif isinstance(left_values, bool):
            if isinstance(right_values, bool):
                # Items are a bool
                if left_values != right_values:
                    return False
            else:
                # The left and right types mismatch
                return False

        elif isinstance(left_values, str):
            if isinstance(right_values, str):
                # The items are a string
                if left_values != right_values:
                    return False
            else:
                # The left and right types mismatch
                return False
        else:
            Logs.warn('[WARN] ConfigSet value cannot by compare, unsupported type {} for key '.format(type(left_values),left_keys[i]))
            pass

    return True


def split_comma_delimited_string(input_str, enforce_uniqueness=False):
    """
    Utility method to split a comma-delimited list into a list of strings.  Will also handle
    trimming of results and handling empty lists

    :param input_str:           Comma-delimited string to process
    :param enforce_uniqueness:  Enforce uniqueness, will strip out duplicate values if set
    :return:                    List of values parsed from the string.  Will return an empty
                                list [] if no values are set
    """

    result_list = list()
    if input is None:
        return []

    raw_split_values = input_str.split(',')
    for raw_split_value in raw_split_values:
        trimmed_split_value = raw_split_value.strip()

        if len(trimmed_split_value) == 0:
            continue

        if enforce_uniqueness:
            append_to_unique_list(result_list, trimmed_split_value)
        else:
            result_list.append(trimmed_split_value)

    return result_list


warned_messages = set()


@conf
def warn_once(ctx, warning):
    """
    Log a warning only once (prevent duplicated warnings)
    :param warning: Warning message to log
    :return:
    """
    if warning not in warned_messages:
        Logs.warn('[WARN] {}'.format(warning))
        warned_messages.add(warning)


UNVERSIONED_HOST_PLATFORM_TO_WAF_PLATFORM_MAP = {
    'linux':    'linux_x64',
    'win32':    'win_x64',
    'darwin':   'darwin_x64'
}

@conf
def get_waf_host_platform(conf):
    """
    Determine the WAF version of the host platform.

    :param conf:    Configuration context
    :return:        Any of the supported host platforms:
                        linux_64, win_x64, or darwin_x64
                    Otherwise this is a configuration error
    """

    unversioned_platform = Utils.unversioned_sys_platform()
    if unversioned_platform in UNVERSIONED_HOST_PLATFORM_TO_WAF_PLATFORM_MAP:
        return UNVERSIONED_HOST_PLATFORM_TO_WAF_PLATFORM_MAP[unversioned_platform]
    else:
        conf.fatal('[ERROR] Host \'%s\' not supported' % Utils.unversioned_sys_platform())


@conf
def cached_does_path_exist(ctx, path, reset=False):
    """
    Check if a path exists or not, but cache the result to reduce multiple calls to the OS for the same path.  This
    should only be used when we know for certain paths/files wont be created during processing.

    For example:

    if not ctx.cached_does_path_exist(my_file):
        while open(my_file,'w') as file:
            file.write(...)

    The above will cause the result to be out of sync and invalid

    :param ctx:         Context
    :param path:        The path to check for existence
    :return: True if the path exists, false if not
    """
    try:
        path_cache = ctx.cached_path_check
    except AttributeError:
        path_cache = ctx.cached_path_check = {}

    path_to_check = os.path.normcase(path)
    if not path_to_check in path_cache or reset:
        path_exists = os.path.exists(path_to_check)
        path_cache[path_to_check] = path_exists
    else:
        path_exists = path_cache[path_to_check]
    return path_exists


def read_game_name_from_bootstrap(bootstrap_cfg_file_path, game_name_attribute='sys_game_folder'):
    """
    Read the game name from bootstrap.cfg
    
    :param bootstrap_cfg_file_path: The path to bootstrap.cfg
    :param game_name_attribute:     The attribute that holds the game name
    :return: The game name
    """
    
    if not os.path.exists(bootstrap_cfg_file_path):
        raise Errors.WafError("Unable to locate required bootstrap configuration file: {}".format(bootstrap_cfg_file_path))
    
    with open(bootstrap_cfg_file_path, 'r') as bootstrap_file:
        bootstrap_contents = bootstrap_file.read()
    try:
        game_name = re.search('^\s*{}\s*=\s*(\w+)'.format(game_name_attribute), bootstrap_contents, re.MULTILINE).group(1)
    except Exception as e:
        raise Errors.WafError("Error searching for 'sys_game_folder' from '{}': {}".format(bootstrap_cfg_file_path, e))
    return game_name
