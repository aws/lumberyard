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
from waflib import Errors, Logs, Node
from waflib.Configure import conf, ConfigurationContext

from cry_utils import append_kw_entry, append_to_unique_list
from cryengine_modules import build_stlib, \
                              build_shlib,\
                              build_program, \
                              BuildTaskGenerator, \
                              RunTaskGenerator,\
                              ApplyConfigOverwrite,\
                              ApplyBuildOptionSettings,\
                              LoadFileLists,\
                              LoadAdditionalFileSettings,\
                              is_third_party_uselib_configured,\
                              clean_duplicates_in_list
from settings_manager import LUMBERYARD_SETTINGS

import utils

import re
import os
import glob

"""
The following 'KEYWORD_TYPES' represents reserved keywords for the build system and their expected type. Any keywords 
that are not listed below will be treated as lists. 
"""
KEYWORD_TYPES = {
    "str": [    # Keyword types that can only be a single string value
        'target',
        'vs_filter',
        'pch',
        'langname',
        'copyright_org'
    ],
    "bool": [   # Keyword types that can only be a single boolean value
        'disable_pch',
        'use_required_gems',
        'exclude_monolithic',
        'client_only',
        'server_only',
        'test_only',
        'exclude_test',
        'remove_release_define',
        'enable_rtti'
    ],
    "additional_settings": [
        'additional_settings'
    ],
    "path": [   # Keyword types that represent a list of string paths (aliases will be applied if present)
        'includes',
        'project_settings',
        'meta_includes',
        'file_list',
        'includes',
        'export_includes',
        'libpath',  # shared
        'stlibpath',  # static
        'frameworkpath',
        'rpath',
        'mirror_artifacts_to_include',
        'mirror_artifacts_to_exclude',
        'output_folder',
        'source_artifacts_include',
        'source_artifacts_exclude',
        'mirror_artifacts_to_include',
        'mirror_artifacts_to_exclude',
        'additional_manifests'
    ],
    'ALIASABLE': [
        'includes',
        'defines'
    ]
}

"""
The following list represents keywords that will automatically be set to either a blank list or if its a single item, it will be converted to a list of that single item
"""
SANITIZE_TO_LIST_KEYWORDS = {
    'additional_settings',
    'export_definitions',
    'meta_includes',
    'file_list',
    'use',
    'defines',
    'export_defines',
    'includes',
    'export_includes',
    'cxxflags',
    'cflags',
    'lib',
    'libpath',
    'stlib',
    'stlibpath',
    'linkflags',
    'framework',
    'frameworkpath',
    'rpath',
    'features',
    'uselib',
    'mirror_artifacts_to_include',
    'mirror_artifacts_to_exclude',
    'source_artifacts_include',
    'source_artifacts_exclude',
    'mirror_artifacts_to_include',
    'mirror_artifacts_to_exclude',
    'copy_external',
    'copy_dependent_files',
    'additional_manifests',
    'files',
    'winres_includes',
    'winres_defines',
    'remove_cxxflags',
    'remove_defines'
}


def _collect_legacy_keywords():

    """
    Helper function to build a set of all permutations of platform/configuration prefixed keywords so we can flag them for deprecated
    wscript definitions
    :return:  The list of all possible deprecated keywords
    """
    # Build a set of all permutations so we can detect any legacy platform/config keyword name modifiers
    legacy_modifiers = set()
    for platform_details in LUMBERYARD_SETTINGS.get_all_platform_settings():
        platform_and_alias = [platform_details.platform] + list(platform_details.aliases)
        for platform in platform_and_alias:
            legacy_modifiers.add(platform)
            for configuration_details in LUMBERYARD_SETTINGS.get_build_configuration_settings():
                legacy_modifiers.add(
                    '{}_{}'.format(platform, configuration_details.build_config_name(is_test=False, is_server=False)))
                legacy_modifiers.add(
                    '{}_{}'.format(platform, configuration_details.build_config_name(is_test=False, is_server=True)))
                legacy_modifiers.add(
                    '{}_{}'.format(platform, configuration_details.build_config_name(is_test=True, is_server=False)))
                legacy_modifiers.add(
                    '{}_{}'.format(platform, configuration_details.build_config_name(is_test=True, is_server=True)))
            legacy_modifiers.add('{}_ndebug'.format(platform))

    legacy_keywords = set()
    for legacy_modifier in legacy_modifiers:
        for values in KEYWORD_TYPES.values():
            for value in values:
                legacy_keywords.add('{}_{}'.format(legacy_modifier, value))
        for value in SANITIZE_TO_LIST_KEYWORDS:
            legacy_keywords.add('{}_{}'.format(legacy_modifier, value))
    return legacy_keywords


LEGACY_KEYWORDS = _collect_legacy_keywords()


@build_stlib
def LumberyardStaticLibrary(ctx, *k, **kw):
    """
    WAF Task Generator for static c++ libraries
    """
    target = get_target(ctx, kw)
    platform, configuration = get_current_platform_configuration(ctx)

    additional_aliases = initialize_task_generator(ctx, target, platform, configuration, kw)

    configure_task_generator(ctx, target, kw)

    kw['stlib'] = True

    if not BuildTaskGenerator(ctx, kw):
        return None

    return RunTaskGenerator(ctx, *k, **kw)


@build_shlib
def LumberyardSharedLibrary(ctx, *k, **kw):
    """
    WAF Task Generator for shared c++ libraries
    """

    target = get_target(ctx, kw)
    platform, configuration = get_current_platform_configuration(ctx)

    additional_aliases = initialize_task_generator(ctx, target, platform, configuration, kw)

    configure_task_generator(ctx, target, kw)

    if not BuildTaskGenerator(ctx, kw):
        return None

    return RunTaskGenerator(ctx, *k, **kw)


@build_program
def LumberyardApplication(ctx, *k, **kw):
    """
    WAF Task Generator for c++ applications
    """
    target = get_target(ctx, kw)
    platform, configuration = get_current_platform_configuration(ctx)

    additional_aliases = initialize_task_generator(ctx, target, platform, configuration, kw)

    configure_task_generator(ctx, target, kw)

    append_kw_entry(kw, 'features', ['copy_3rd_party_binaries'])

    if not BuildTaskGenerator(ctx, kw):
        return None

    return RunTaskGenerator(ctx, *k, **kw)


########################################################################################################################
def get_target(ctx, kw):
    """
    Get the required target from the input keywords. 'target' is required so if it doesnt exist, an exception will be
    raised.

    :param ctx: Current context
    :param kw:  Keyword dictionary to search for 'target'
    :return: The 'target' keyword value
    """
    # 'target' is required for all modules, validate it is in the dictionary
    target = kw.get('target', None)
    if not target:
        raise Errors.WafError("Missing required 'target' for module definition in file: '{}'".format(ctx.cur_script.abspath()))
    return target


def get_current_platform_configuration(ctx):
    """
    Determine the current configuration, or 'None' if this is a 'project_generator' task or a configure command

    :param ctx: The context to query for the platform and configuration string value
    :return:    The PlatformConfiguration object for the current configuration if this is a build command, otherwise None
    """
    env_platform = ctx.env['PLATFORM']
    env_configuration = ctx.env['CONFIGURATION']
    if isinstance(ctx, ConfigurationContext) or env_platform == 'project_generator':
        configuration = None
        platform = None
    else:
        platform_details = ctx.get_target_platform_detail(env_platform)
        configuration = platform_details.get_configuration(env_configuration)
        platform = platform_details.platform
    return platform, configuration


def initialize_task_generator(ctx, target, platform, configuration, kw):
    """
    Initialize and prepare the kw dictionary for the task generator

    :param ctx:             The current context
    :param target:          The current module target
    :param configuration:   The current build configuration (none if this is a platform_generator or Configure)
    :param kw:              The current kw dictionary for the module
    :return                 A dictionary of additional aliases specific to this target
    """

    # Perform pre-processing of the keywords
    additional_aliases = preprocess_keywords(ctx, target, platform, configuration, kw)

    # Sanitize any left over keywords
    sanitize_kw_input(kw)

    # Verify the required inputs and make sure that any paths that are set are valid
    verify_inputs(ctx, target, kw)

    # Register the project to a vs filter (for msvs generation)
    register_visual_studio_filter(ctx, target, kw)

    # Track all the files from all the file_lists independent of the target platform
    track_file_list_changes(ctx, kw)

    return additional_aliases


def configure_task_generator(ctx, target, kw):
    """
    Helper function to apply default configurations and to set platform/configuration dependent settings
    * Fork of ConfigureTaskGenerator
    """

    # Ensure we have a name for lookup purposes
    kw.setdefault('name', target)

    # Lookup the PlatformConfiguration for the current platform/configuration (if this is a build command)
    target_platform = []

    # Special case:  Only non-android launchers can use required gems
    apply_required_gems = kw.get('use_required_gems', False)
    if kw.get('is_launcher', False):
        if apply_required_gems and not ctx.is_android_platform(target_platform):
            ctx.apply_required_gems_to_context(target, kw)
    else:
        if apply_required_gems:
            ctx.apply_required_gems_to_context(target, kw)

    # Apply all settings, based on current platform and configuration
    ApplyConfigOverwrite(ctx, kw)

    ApplyBuildOptionSettings(ctx, kw)

    # Load all file lists (including additional settings)
    file_list = kw['file_list']
    for setting in kw.get('additional_settings',[]):

        for setting_kw in setting:
            if setting_kw in LEGACY_KEYWORDS:
                Logs.warn("[WARN] platform/configuration specific keyword '{}' found in 'additional_settings' keyword for "
                          "target'{}' is not supported with any of the Lumberyard* modules. Use the project_settings "
                          "keyword to specify platform/config specific values instead".format(setting_kw, target))
        file_list += setting.get('file_list', [])

    file_list = kw['file_list']

    LoadFileLists(ctx, kw, file_list)

    kw.setdefault('additional_settings', [])
    
    LoadAdditionalFileSettings(ctx, kw)

    apply_uselibs(ctx, target, kw)

    process_optional_copy_keywords(ctx, target, kw)

    # Clean out some duplicate kw values to reduce the size for the hash calculation
    kw['defines'] = clean_duplicates_in_list(kw['defines'], '{} : defines'.format(target))

    # Optionally remove any c/cxx flag that may have been inherited by a shared settings file
    remove_cxxflags = kw.get('remove_cxxflags', None)
    if remove_cxxflags:
        cflag_keys = ('cflags', 'cxxflags')
        for remove_cxxflag in remove_cxxflags:
            for cflag_key in cflag_keys:
                kw.setdefault(cflag_key,[]).remove(remove_cxxflag)

    # Optionally remove any define that may have been inherited by a shared settings file
    remove_defines = kw.get('remove_defines', None)
    if remove_defines:
        for remove_define in remove_defines:
            kw.setdefault('defines').remove(remove_define)


def preprocess_keywords(ctx, target, platform, configuration, kw):
    """
    Perform pre-processing of the input keywords and return the name of the module target

    :param ctx:             Current Context
    :param target:          The target name of the module
    :param platform:        The current build platform (None if this is a project_generator or configure context)
    :param configuration:   The current build configuration (None if this is a project_generator or configure context)
    :param kw:              The keyword dictionary to pre-process
    :return                 A dictionary of additional aliases specific to this target
    """

    # Apply default values
    apply_default_keywords(ctx, target, kw)
    assert 'output_file_name' in kw, "kw value 'output_file_name' expected to be set at this point (from apply_default_keywords)"
    output_filename = kw['output_file_name']

    additional_aliases = {
        'TARGET': target,
        'OUTPUT_FILENAME':  output_filename
    }
    
    settings_files = kw.get('project_settings_includes', [])
    for settings_file in settings_files:
        ctx.apply_settings_file(target, kw, settings_file, additional_aliases, platform, configuration)
        
    # Also check for any restricted platform settings file that follows the expected pattern
    for p0, p1, _, _ in ctx.env['RESTRICTED_PLATFORMS']:
        restricted_platform_settings = os.path.normpath(os.path.join(ctx.path.abspath(), '{}/{}_{}_settings.json'.format(p0, target.lower(), p1)))
        if os.path.isfile(restricted_platform_settings):
            ctx.apply_settings_file(target, kw, restricted_platform_settings, additional_aliases, platform, configuration)

    # Apply additional aliases
    preprocess_config_aliases_keywords(ctx, kw, additional_aliases)
    
    return additional_aliases


def register_visual_studio_filter(ctx, target, kw):
    """
    Util-function to register each provided visual studio filter parameter in a central lookup map
    * Forked version of RegisterVisualStudioFilter
    """
    vs_filter = kw.get('vs_filter', None)
    if not vs_filter:
        raise Errors.WafError("Missing required 'vs_filter' keyword for module definition in file: '{}'".format(ctx.cur_script.abspath()))

    if not hasattr(ctx, 'vs_project_filters'):
        ctx.vs_project_filters = {}

    ctx.vs_project_filters[target] = vs_filter


def track_file_list_changes(ctx, kw):
    """
    Util function to ensure file lists are correctly tracked regardless of current target platform
    * Fork of TrackFileListChanges
    """
    def _to_list(value):
        """ Helper function to ensure a value is always a list """
        if isinstance(value, list):
            return value
        return [value]

    files_to_track = []
    kw['waf_file_entries'] = []

    # Collect all file list entries
    for (key,value) in kw.items():
        if 'file_list' in key:
            files_to_track += _to_list(value)
        # Collect potential file lists from additional options
        if 'additional_settings' in key:
            for settings_container in kw['additional_settings']:
                for (key2,value2) in settings_container.items():
                    if 'file_list' in key2:
                        files_to_track += _to_list(value2)

    # Remove duplicates
    files_to_track = list(set(files_to_track))

    # Add results to global lists
    for file in files_to_track:
        file_node = ctx.path.make_node(file)
        append_kw_entry(kw, 'waf_file_entries', [file_node])


def verify_inputs(ctx, target, kw):
    """
    Helper function to verify passed input values
    """
    if not kw.get('file_list', None):
        raise Errors.WafError("Missing required 'file_list' kw input for target '{}'".format(target))

    if 'source' in kw:
        raise Errors.WafError("Unsupported keyword parameter 'source' detected for target '{}'. Use 'file_list' instead".format(target))

    for key_name in kw:
        if key_name in LEGACY_KEYWORDS:
            Logs.warn("[WARN] platform/configuration specific keyword '{}' for target'{}' is not supported with any of "
                      "the Lumberyard* modules. Use the project_settings keyword to specify platform/config specific "
                      "values instead".format(key_name, target))

    is_build_platform_cmd = getattr(ctx, 'is_build_cmd', False)
    if is_build_platform_cmd:
        bintemp = os.path.normcase(ctx.bldnode.abspath())
        for kw_check in kw.keys():
            for path_type in KEYWORD_TYPES['path']:
                if not kw_check.endswith(path_type):
                    continue
                # project_settings is a special case, it is evaluated before reaching this point
                if kw_check == 'project_settings':
                    continue
                if isinstance(kw[kw_check], str):
                    list_to_check = [kw[kw_check]]
                else:
                    list_to_check = kw[kw_check]

                for path_check in list_to_check:
                    if isinstance(path_check, str):
                        # If the path input is a string, derive the absolute path for the input path
                        path_to_validate = os.path.normcase(os.path.join(ctx.path.abspath(), path_check))
                    elif isinstance(path_check, Node.Node):
                        # If the path is a Node object, get its absolute path
                        path_to_validate = os.path.normcase(path_check.abspath())
                    else:
                        raise Errors.WafError("Invalid type for keyword '{}' defined for target '{}'. Expecting a path.".format(kw_check,
                                                                                                                                target))

                    # Path validation can be skipped for azcg because it may not have been generated yet
                    is_azcg_path = path_to_validate.startswith(bintemp)

                    if not os.path.exists(path_to_validate) and not is_azcg_path:
                        Logs.warn("[WARN] '{}' value '{}' defined for target '{}' does not exist".format(kw_check,
                                                                                                         path_to_validate,
                                                                                                         target))


def apply_uselibs(ctx, target, kw):
    """
    Perform validation on all the declared uselibs. This will only warn on invalid uselibs, not error
    """
    # If uselib is set, validate them
    uselib_names = kw.get('uselib', None)
    if uselib_names is not None:
        processed_dependencies = []
        for uselib_name in uselib_names:
            if not is_third_party_uselib_configured(ctx, uselib_name):
                Logs.warn("[WARN] Invalid uselib '{}' declared in project '{}'.  This may cause compilation or linker errors".format(uselib_name, target))
            else:
                if uselib_name not in processed_dependencies and uselib_name not in kw.get('no_inherit_config', []):
                    ctx.append_dependency_configuration_settings(uselib_name, kw)
                    processed_dependencies.append(uselib_name)


def process_optional_copy_keywords(ctx, target, kw):
    """
    Process any optional copy* keywords
    """
    is_build_platform_cmd = getattr(ctx, 'is_build_cmd', False)
    if is_build_platform_cmd:
        # Check if we are applying external file copies
        if 'copy_external' in kw and len(kw['copy_external']) > 0:
            for copy_external_key in kw['copy_external']:
                copy_external_env_key = 'COPY_EXTERNAL_FILES_{}'.format(copy_external_key)
                if 'COPY_EXTERNAL_FILES' not in ctx.env:
                    ctx.env['COPY_EXTERNAL_FILES'] = []
                append_kw_entry(kw, 'features', 'copy_external_files')
                if copy_external_env_key in ctx.env:
                    for copy_external_value in ctx.env[copy_external_env_key]:
                        ctx.env['COPY_EXTERNAL_FILES'].append(copy_external_value)

        # Check if we are applying external file copies to specific files
        copy_dependent_files = kw.get('copy_dependent_files', [])
        if len(copy_dependent_files) > 0:
            append_kw_entry(kw, 'features', 'copy_module_dependent_files')
            copy_dependent_env_key = 'COPY_DEPENDENT_FILES_{}'.format(target.upper())
            if copy_dependent_env_key not in ctx.env:
                ctx.env[copy_dependent_env_key] = []
            for copy_dependent_file in copy_dependent_files:
                ctx.env[copy_dependent_env_key].append(copy_dependent_file)

        # Check if we are applying external file copies to specific files
        copy_dependent_files = kw.get('copy_dependent_files_keep_folder_tree', [])
        if len(copy_dependent_files) > 0:
            append_kw_entry(kw, 'features', 'copy_module_dependent_files_keep_folder_tree')
            copy_dependent_env_key = 'COPY_DEPENDENT_FILES_KEEP_FOLDER_TREE_{}'.format(target.upper())
            if copy_dependent_env_key not in ctx.env:
                ctx.env[copy_dependent_env_key] = []
            for copy_dependent_file in copy_dependent_files:
                ctx.env[copy_dependent_env_key].append(copy_dependent_file)


FILE_ALIAS_3P_PREFIX = "3P:"
FILE_ALIAS_AZCG_PREFIX = "AZCG:"
REGEX_PATH_ALIAS = re.compile('@([\\w\\.\\:]+)@')


class AliasProcessing(object):
    """
    Alias Processing class to manage and evaluation any '@' aliases in config files or wscripts
    """

    def __init__(self, ctx):
        self.alias_dict = {
            'ROOT': ctx.srcnode.abspath(),
            'SETTINGS': ctx.srcnode.make_node('_WAF_/settings').abspath()
        }

    def register_script_alias(self, alias, value):
        """
        Register an alias value
        :param alias:   The alias to register
        :param value:   The value of the alias
        """
        if alias in self.alias_dict:
            Logs.warn("[WARN] Overriding existing script alias '{}' value '{}' with '{}'".format(alias, self.alias_dict[alias], value))
        self.alias_dict[alias] = value

    def preprocess_value(self, ctx, value, additional_aliases):
        """
        Process a value and evaluate any aliases
        :param ctx:                 Context
        :param value:               The value to process the alias
        :param additional_aliases:  Any additional aliases to consider
        :return:
        """
        if isinstance(value, Node.Node):
            return value
        if '@' not in value:
            return value
        alias_match_groups = REGEX_PATH_ALIAS.search(value)
        if alias_match_groups:

            # Alias pattern detected, evaluate the alias
            alias_key = alias_match_groups.group(1)

            if alias_key.startswith(FILE_ALIAS_3P_PREFIX):
                # Apply a third party alias reference
                third_party_identifier = alias_key.replace(FILE_ALIAS_3P_PREFIX, "").strip()
                resolved_path, enabled, roles, optional = ctx.tp.get_third_party_path(None, third_party_identifier)

                # If the path was not resolved, it could be an invalid alias (missing from the SetupAssistantConfig.json
                if not resolved_path:
                    raise Errors.WafError("Invalid 3rd party alias: '{}'".format(alias_key))

                # If the path was resolved, we still need to make sure the 3rd party is enabled based on the roles
                if not enabled and not optional:
                    raise Errors.WafError("3rd party alias '{}' is not enabled. Make sure it is enabled in SetupAssistant".format(alias_key))

                processed_value = os.path.normpath(value.replace('@{}@'.format(alias_key), resolved_path))
            elif alias_key.startswith(FILE_ALIAS_AZCG_PREFIX):
                azcg_identifier = alias_key.replace(FILE_ALIAS_AZCG_PREFIX, "").strip()
                resolved_path = ctx.azcg_output_dir_node(azcg_identifier).abspath()
                processed_value = os.path.normpath(value.replace('@{}@'.format(alias_key), resolved_path))
            elif alias_key == 'PATH':
                processed_value = os.path.normpath(value.replace('@{}@'.format(alias_key), ctx.path.abspath()))
            elif additional_aliases and alias_key in additional_aliases:
                processed_value = value.replace('@{}@'.format(alias_key), str(additional_aliases[alias_key]))
            elif alias_key in self.alias_dict:
                processed_value = os.path.normpath(value.replace('@{}@'.format(alias_key), str(self.alias_dict[alias_key])))
            else:
                processed_value = value
            return processed_value
        else:
            return value


@conf
def register_script_env_var(ctx, env_var, value):
    try:
        ctx.alias_processor.register_script_alias(env_var, value)
    except AttributeError:
        ctx.alias_processor = AliasProcessing(ctx)
        ctx.alias_processor.register_script_alias(env_var, value)


def process_string(ctx, input_string, additional_aliases):
    """
    Process a string through the alias processor and perform any necessary substitutions

    :param ctx:                 Context
    :param input_string:        The input string to process
    :param additional_aliases:  Any additional aliases (dict) to use for processing
    :return:    The processed string
    """
    try:
        return ctx.alias_processor.preprocess_value(ctx, input_string, additional_aliases)
    except AttributeError:
        ctx.alias_processor = AliasProcessing(ctx)
        return ctx.alias_processor.preprocess_value(ctx, input_string, additional_aliases)


def preprocess_config_aliases_keywords(ctx, kw, additional_aliases):
    """
    Perform some additional preprocessing on keywords that may have an alias, including apply ALIASABLE rules such as
    'export_includes', etc
    :param ctx:                 Context
    :param kw:                  The keyword dictionary to process
    :param additional_aliases:  Any additional aliases (dict) to use for processing
    :return:
    """
    keys = kw.keys()
    for key in keys:
        if key not in KEYWORD_TYPES['ALIASABLE']:
            continue
        if isinstance(kw[key], str):
            if '@' in kw[key]:
                kw[key] = process_string(ctx, kw[key], additional_aliases)
        elif isinstance(kw[key], list):
            processed = []
            for value in kw[key]:
                processed.append(process_string(ctx, value, additional_aliases))
            kw[key] = processed


def get_project_settings(ctx, project_setting_file):
    """
    Read, parse, and cache project settings files (by their absolute file path). The parsing is relaxed and will accept
    '#' as comment tags within the file.
    :param ctx:                     Context
    :param project_setting_file:    The absolute file path to the settings file
    :return: The parsed dictionary from the settings file
    """
    try:
        return ctx.project_settings[project_setting_file]
    except AttributeError:
        ctx.project_settings = {
            project_setting_file: utils.parse_json_file(project_setting_file, True)
        }
    except KeyError:
        ctx.project_settings[project_setting_file] = utils.parse_json_file(project_setting_file,True)
    return ctx.project_settings[project_setting_file]


# The following _apply* functions are helper functions to apply default values for missing required keywords for
# the dictionary DEFAULT_KEYWORDS.


def _apply_default_all(ctx, target, kw, key):
    if key not in kw:
        kw[key] = ['all']


def _apply_empty_list(ctx, target, kw, key):
    if key not in kw:
        kw[key] = []


def _apply_output_filename(ctx, target, kw, key):
    if key not in kw:
        output_filename = target
    elif isinstance(kw[key], list):
        output_filename = kw[key][0]  # Change list into a single string
    else:
        output_filename = kw[key]
    kw[key] = output_filename


def _apply_copyright_org(ctx, target, kw, key):
    if key not in kw:
        kw[key] = 'Amazon'


def _apply_base_project_settings_path(ctx, target, kw, key):
    kw[key] = ctx.path.abspath()


# Default keywords and their apply* functions. Keywords defined here will be pre-processed at the code level so it will
# be applied to all projects regardless of their definitions. If any of these keywords are overridden specifically for
# a project, then the override warning will bd suppressed
DEFAULT_KEYWORDS = {
    'use': _apply_empty_list,
    'uselist': _apply_empty_list,
    'cflags': _apply_empty_list,
    'cxxflags': _apply_empty_list,
    'linkflags': _apply_empty_list,
    'defines': _apply_empty_list,
    'platforms': _apply_default_all,
    'configurations': _apply_default_all,
    'output_file_name': _apply_output_filename,
    'copyright_org': _apply_copyright_org,
    'base_project_settings_path': _apply_base_project_settings_path
}


def apply_default_keywords(ctx, target, kw):
    """
    Apply default keywords if they are not provided in the keyword dictionary

    :param ctx:     Context
    :param target:  The name of the current target
    :param kw:      Keyword Dictionary
    """
    kw.setdefault('use', [])
    kw.setdefault('uselib', [])

    for key, func in DEFAULT_KEYWORDS.items():
        func(ctx, target, kw, key)


def sanitize_kw_input(kw):
    """
    Sanitize the kw dictionary and apply as a list
    :param kw: The kw dict to sanitize
    """

    for key in kw:
        if key in SANITIZE_TO_LIST_KEYWORDS:
            if not isinstance(kw[key], list):
                kw[key] = [kw[key]]

    # Recurse for additional settings
    if 'additional_settings' in kw:
        for setting in kw['additional_settings']:
            sanitize_kw_input(setting)


# Possible permutations of the env key for test+dedicated configurations
ENV_CONFIG_DEDICATED_TEST_PERMUTATIONS = ['dedicated,test', 'test,dedicated']

# Based on if are processing a test, dedicated, or both type of configuration, maintain a list of env keys that need to be looked up for merging
ENV_CONFIG_PERMUTATIONS_MAP =  {
                                'test/':            ['test'] + ENV_CONFIG_DEDICATED_TEST_PERMUTATIONS,
                                'dedicated/':       ['dedicated'] + ENV_CONFIG_DEDICATED_TEST_PERMUTATIONS,
                                'test/dedicated/':  ENV_CONFIG_DEDICATED_TEST_PERMUTATIONS
                               }


class ProjectSettingsFile:
    """
    This class will manage the project settings files and any additional includes of other project settings files
    that are declared in them
    """
    
    def __init__(self, ctx, path, additional_aliases):
        
        self.path = path
        self.ctx = ctx
        
        # Parse the json input
        self.dict = utils.parse_json_file(path, True)
        
        # Read in any "includes" section to add a dependency
        self.included_settings = {}
        include_settings_files = self.dict.get('includes', [])
        for include_settings_file in include_settings_files:
            if include_settings_file in self.included_settings:
                continue
            include_settings = ctx.get_project_settings_file(include_settings_file, additional_aliases)
            self.included_settings[include_settings_file] = include_settings
            
    def merge_kw_key_type(self, target, kw_key, kw_key_type, source_section, merge_kw):
        
        assert kw_key_type in (str, bool)
        
        kw_key_type_name = kw_key_type.__name__
        
        if kw_key not in KEYWORD_TYPES[kw_key_type_name]:
            return False

        # This is expected to be a string keyword, validate the source and merge are the same type of string
        if not isinstance(source_section[kw_key], kw_key_type):
            raise Errors.WafError("Invalid type of keyword '{}' in settings file '{}'. Expecting a {}.".format(kw_key,
                                                                                                               self.path,
                                                                                                               kw_key_type_name))
        
        if not isinstance(merge_kw[kw_key], kw_key_type):
            raise Errors.WafError("Invalid type of keyword '{}' in wscript for target '{}'. Expecting a {}.".format(kw_key,
                                                                                                                    target,
                                                                                                                    kw_key_type_name))
        
        Logs.debug("lumberyard: Overwritting kw_key '{}' for target '{}' from settings file '{}' ({}->{})".format(kw_key,
                                                                                                                  target,
                                                                                                                  self.path,
                                                                                                                  source_section[kw_key],
                                                                                                                  merge_kw[kw_key]))
        merge_kw[kw_key] = source_section[kw_key]
        return True

    def merge_kw_key(self, target, kw_key, source_section, merge_kw):
        
        if not kw_key in source_section:
            return
        
        elif not kw_key in merge_kw:
            merge_kw[kw_key] = source_section[kw_key] # TODO: deep copy?
            
        else:
            if not self.merge_kw_key_type(target, kw_key, str, source_section, merge_kw) and not self.merge_kw_key_type(target, kw_key, bool, source_section, merge_kw):
                # Everything else is meant to be a list of string
                target_list = merge_kw.setdefault(kw_key,[])
                append_to_unique_list(target_list, source_section[kw_key])
                
    def merge_kw_section(self, section_key, target, merge_kw):
        section = self.dict.get(section_key, {})
        if section:
            for kw_key in section.keys():
                self.merge_kw_key(target, kw_key, section, merge_kw)

    def merge_kw_section_by_config_permutations(self, config_permutations, target, platform_name, configuration_name, merge_kw):
        for restrictions in config_permutations:
            self.merge_kw_section('*/*/{}'.format(restrictions), target, merge_kw)
            self.merge_kw_section('{}/*/{}'.format(platform_name, restrictions), target, merge_kw)
            self.merge_kw_section('{}/{}/{}'.format(platform_name, configuration_name, restrictions), target, merge_kw)

    def merge_kw_dict(self, target, merge_kw, platform, configuration):
        
        # Process merging any include settings
        for include_setting in self.included_settings.values():
            include_setting.merge_kw_dict(target, merge_kw, platform, configuration)
            
        if platform:
            # If the platform is set, get the platform name and gather the aliases to that platform as a basis to
            # construct the platform/configuration keys
            platform_settings = self.ctx.get_platform_settings(platform)
            platform_names = {platform_settings.platform}
            platform_names |= platform_settings.aliases
            
            # Go through each platform and merge the dictionary based on the platform/platform alias
            for platform_name in platform_names:
                platform_key = '{}/*'.format(platform_name)
                self.merge_kw_section(platform_key, target, merge_kw)
                
                if configuration:
                    # If a configuration is set, we need to narrow down the platform/config keys to attempt to merge
                    
                    if configuration.settings.base_config:
                        # This is a derived configuration, apply base first
                        self.merge_kw_dict(target, merge_kw, platform_name, configuration.settings.base_config)
                        
                    # Perform a straight merge on the platform+configuration key
                    platform_config_key = '{}/{}'.format(platform_name, configuration.settings.name)
                    self.merge_kw_section(platform_config_key, target, merge_kw)
                    
                    config_permutation_key = 'test/' if configuration.is_test else ''
                    config_permutation_key += 'dedicated/' if configuration.is_server else ''
                    
                    config_permutations = ENV_CONFIG_PERMUTATIONS_MAP.get(config_permutation_key, None)
                    if config_permutations:
                        self.merge_kw_section_by_config_permutations(config_permutations=config_permutations,
                                                                     target=target,
                                                                     platform_name=platform_name,
                                                                     configuration_name=configuration.settings.name,
                                                                     merge_kw=merge_kw)
                        
        
@conf
def get_project_settings_file(ctx, path, additional_aliases):
    
    processed_path = process_string(ctx, path, additional_aliases)
    if not os.path.isabs(processed_path):
        processed_path = os.path.join(ctx.path.abspath(), processed_path)
    if not os.path.isfile(processed_path):
        raise Errors.WafError("Invalid project settings file path '{}'", path)
        
    try:
        return ctx.project_settings[processed_path]
    except AttributeError:
        ctx.project_settings = {
            processed_path: ProjectSettingsFile(ctx, processed_path, additional_aliases)
        }
    except KeyError:
        ctx.project_settings[processed_path] = ProjectSettingsFile(ctx, processed_path, additional_aliases)
        
    return ctx.project_settings[processed_path]
    

@conf
def apply_settings_file(ctx, target, merge_kw, settings_file, additional_aliases, platform, configuration):
    
    settings = ctx.get_project_settings_file(settings_file, additional_aliases)
    
    # Always apply the global '*/*' kw section
    settings.merge_kw_section('*/*', target, merge_kw)
    
    settings.merge_kw_dict(target, merge_kw, platform, configuration)
    
    pass


def apply_project_settings_for_input():
    pass