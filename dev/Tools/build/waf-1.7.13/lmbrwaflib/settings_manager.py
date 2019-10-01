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

from waflib import Errors, Logs, Utils
from waflib.Configure import conf, ConfigurationContext
from waflib.Build import BuildContext
from waflib.Utils import unversioned_sys_platform

from cry_utils import append_to_unique_list, read_game_name_from_bootstrap
from mscv_helper import find_valid_wsdk_version

import utils
import copy
import glob
import os
import re
import sys
import hashlib
import ConfigParser


# Map of default callbacks
AUTO_DEFAULT_VALUE_CALLBACKS = {}

SETTING_KEY_ATTRIBUTE = 'attribute'
SETTING_KEY_LONG_FORM = 'long_form'
SETTING_KEY_SHORT_FORM = 'short_form'
SETTING_KEY_DESCRIPTION = 'description'
SETTING_KEY_DEFAULT_VALUE = 'default_value'


def _stringify(value):
    """
    Local helper function to ensure a value is a string
    :param value: Value to verify
    :return:  The string value
    """
    if not isinstance(value, str):
        return str(value)
    else:
        return value


def register_auto_default_callback(callback_name):
    """
    Decorator to declare functions that act as default value initializers that can be attributed to the default_value.
    The function must return a string value, and accept a single parameter which is the name of the 'attribute'.
    The function is accessible in the default settings file under the 'default_value' key using a '@' prefix.
    If you need to use the literal '@', then use a double '@@' instead.

    The following example uses the registered function 'get_wsdk_version' to get the default value
    Example:
        ..
        {
            "long_form"         : "--win-vs2015-winkit",
            "attribute"         : "win_vs2015_winkit",
            "default_value"     : "@get_wsdk_version",
            "description"       : "The windows kit that vs2015 builds windows targets against"
        }




    :param callback_name:   The name of the callback to register. It will be accessible in the default_settings file
    :return: The function decorator
    """

    def register_auto_default_callback_decorator(func):
        if callback_name in AUTO_DEFAULT_VALUE_CALLBACKS:
            raise Errors.WafError("Duplicate auto default value callback:{}".format(callback_name))
        AUTO_DEFAULT_VALUE_CALLBACKS[callback_name] = func

    return register_auto_default_callback_decorator


def apply_optional_callback(default_value, attribute):
    """
    Given a default value, see if it references an attribute callback or not and apply the callback result if so.
    Otherwise accept the default value as is
    :param default_value: The default value to return or apply the callback for the result
    :param attribute:   The name of the attribute to apply the value to
    :return: The processed value
    """
    if isinstance(default_value, str) and default_value.startswith('@'):
        value = default_value[1:].strip()
        if not value.startswith('@'):
            value_callback = value
            if value_callback not in AUTO_DEFAULT_VALUE_CALLBACKS:
                raise Errors.WafError("Attribute '{}' in the default settings file does not have a registered callback function '{}}'".format(attribute, value_callback))
            value = AUTO_DEFAULT_VALUE_CALLBACKS[value_callback](attribute)
        return value
    else:
        return default_value


@register_auto_default_callback('get_wsdk_version')
def get_wsdk_version(attribute):
    """
    Default attribute handler to get the current wsdk version
    :param attribute:   The name of the attribute that is requesting this value
    :return:    The wsdk version string if found, '' if not
    """
    wsdk_version = ''
    if unversioned_sys_platform() != 'win32':
        return wsdk_version
    try:
        wsdk_version = find_valid_wsdk_version()
    except Exception as e:
        Logs.debug('Error finding valid wsdk version:{}'.format(str(e)))
    if wsdk_version == '':
        Logs.info('Unable to find a valid wsdk version for attribute {}'.format(attribute))
    return wsdk_version


def apply_env_setting_node_to_ctx(ctx, settings_node):
    """
    Apply an env settings subnode (platform-specific or common set of env attributes) to the context env

    :param ctx:             The Context whose env will be updated
    :param settings_node:   The settings node to read the flags from
    """
    if not settings_node:
        return
    env = ctx.env

    # Apply the flags uniquely
    env_apply_flags =  ('DEFINES', 'CFLAGS', 'CXXFLAGS', 'LINKFLAGS')
    for env_apply_flag in env_apply_flags:
        env_values = settings_node.get(env_apply_flag)
        if env_values:
            append_to_unique_list(env[env_apply_flag], env_values)


CACHED_COMMON_CONFIGS = {}
def read_common_config(common_config_path):
    try:
        return CACHED_COMMON_CONFIGS[common_config_path]
    except KeyError:
        platform_setting_json = utils.parse_json_file(common_config_path, True)
        CACHED_COMMON_CONFIGS[common_config_path] = platform_setting_json
        return platform_setting_json
       
    
def process_env_dict_values(source_env_dict, configuration, processed_env_values_dict):
    """
    Read in a raw env dictionary (a dictionary with a combination of 'env', 'env/{configuration') and process each key
    in the source env_dict based on the following:
    
    (?OPTION_CONDITIONAL?|@ENV_CONDITION@):[ENV_KEY1[,ENV_KEYX]*]
    
    :param source_env_dict:             The source dictionary that contains the 'env' sub keys
    :param configuration:               The configuration to match to the env keys
    :param processed_env_values_dict:   The result env key dictionary to update based on the current configuration
    """
    processed_env_target = processed_env_values_dict.setdefault(configuration, {})
    for raw_key, values in source_env_dict.items():
        # Check for any conditional marker in the key
        conditional_index = raw_key.find(':')
        optional_conditional_name = raw_key[:conditional_index] if conditional_index >= 0 else ''
        check_values = (raw_key[conditional_index+1:] if conditional_index >= 0 else raw_key).split(',')
        for check_value in check_values:
            stripped_check_value = check_value.strip()
            working_key = '{}:{}'.format(optional_conditional_name, stripped_check_value) if optional_conditional_name else stripped_check_value
            processed_env_target.setdefault(working_key, [])
            if isinstance(values, list):
                append_to_unique_list(processed_env_target[working_key], values)
            else:
                processed_env_target[working_key] = values
    

def process_env_dict(source_env_dict, processed_env_dict):
    """
    Read a dictionary and process any 'env' or 'env/{configuration}' key node into a env dictionary based on
    configurations ('_' denotes all configurations)
    
    :param source_env_dict:     The source dictionary that contains the 'env*' to evaluate
    :param processed_env_dict:  The target dictionary of configurations -> env dictionary
    """
    for check_key, env_dict in source_env_dict.items():
        if check_key == 'env':
            configuration = '_'
        elif check_key.startswith('env/'):
            configuration = check_key.split('/')[1] or '_'
        else:
            continue
        process_env_dict_values(env_dict, configuration, processed_env_dict)


def merge_attributes_group(settings_include_file, source_attributes_dict, merge_attributes_dict):
    """
    Perform a merge of attributes from a source attributes dictionary to a merged attributes dictionary.
    Any conflicting value will be overwritten
    
    :param settings_include_file:           The source settings include file the source attributes were read from
    :param source_attributes_dict:          The source attributes dictionary to merge from
    :param merge_attributes_dict:           The attributes dictionary to merge into
    """
    
    for source_attr_key, source_attr_value in source_attributes_dict.items():
        
        if source_attr_key not in merge_attributes_dict:
            # New attribute, just set the value in the  merged dictionary and continue
            merge_attributes_dict[source_attr_key] = source_attr_value
        else:
            # There is a conflict
            current_merge_value = merge_attributes_dict[source_attr_key]
            
            if source_attr_value == current_merge_value:
                # If the values are the same, then do nothing
                pass
            else:
                # The values are different. Overwrite and warn
                Logs.warn("[WARN] Attribute value '{}' in settings file '{}' ({}) is different from the existing value ({}). Overwriting with value ({})"
                          .format(source_attr_key,
                                  settings_include_file,
                                  source_attr_value,
                                  current_merge_value,
                                  source_attr_value))
                merge_attributes_dict[source_attr_key] = source_attr_value

    
def merge_settings_group(settings_include_file, merge_settings_dict, setting_group_name, settings_value_dicts):
    """
    Perform a merge of the 'settings' attribute for platform settings include files

    :param settings_include_file:   The source settings file used for this merge
    :param merge_settings_dict:     The settings dictionary to merge into
    :param setting_group_name:      The settings group name for the current merge
    :param settings_value_dicts:    The settings values in the group to merge into 'merge_settings_dict'
    """
    settings_group = merge_settings_dict.setdefault(setting_group_name, [])

    # First build up a quick map of the incoming settings value dict to track which items are merged and which
    # are new (based on 'attribute' being the primary key)
    attribute_to_settings_value_dict = {}
    for settings_value_dict in settings_value_dicts:
        if not settings_value_dict.get('attribute', ''):
            raise Errors.WafError("Invalid 'settings' in settings include file ''".format(settings_include_file))
        attribute_to_settings_value_dict[settings_value_dict['attribute']] = settings_value_dict

    # Second, iterate through the existing settings group and merge if needed
    for existing_settings_dict in settings_group:
        existing_settings_attribute = existing_settings_dict['attribute']
        merge_match_settings_dict = attribute_to_settings_value_dict.get(existing_settings_attribute, None)
        if merge_match_settings_dict:
            Logs.debug("settings_manager: Merging attribute '{}' in settings include file '{}'".format(
                existing_settings_attribute, settings_include_file))
            existing_settings_dict['description'] = merge_match_settings_dict.get('description', None)
            existing_settings_dict['default_value'] = merge_match_settings_dict.get('default_value', None)
            existing_settings_dict['long_form'] = merge_match_settings_dict.get('long_form', None)
            optional_short_form = merge_match_settings_dict.get('short_form', None)
            if optional_short_form:
                existing_settings_dict['short_form'] = optional_short_form
            del attribute_to_settings_value_dict[existing_settings_attribute]

    # Add the remaining settings from the attribute to settings map
    for remaining_settings_dict in attribute_to_settings_value_dict.values():
        settings_group.append(remaining_settings_dict)


def read_bootstrap_overrides_from_settings_file(engine_user_settings_options_file):
    """
    Get the two possible bootstrap overrides (bootstrap_tool_param, bootstrap_third_party_override) from a user_settings.option file

    :param engine_user_settings_options_file:   The absolute path of the user_settings.options file
    :return: tuple of (bootstrap_tool_param, bootstrap_third_party_override) if set
    """
    
    # For external projects, the attributes set in the engine's user_settings.options based on the configuration set by
    # setup assistant must be always overwritten since we can't run setup assistant for an external game project
    
    if not os.path.exists(engine_user_settings_options_file):
        raise Errors.WafError(
            "Unable to locate the engine's settings file. Make sure that the engine at '{} 'is properly configured first".format(
                engine_user_settings_options_file))
    
    override_bootstrap_tool_param = None
    override_bootstrap_third_party_override = None
    
    engine_user_settings_reader = ConfigParser.ConfigParser()
    engine_user_settings_reader.read(engine_user_settings_options_file)
    try:
        override_bootstrap_tool_param = engine_user_settings_reader.get('Misc Options', 'bootstrap_tool_param')
    except ConfigParser.NoSectionError as e:
        raise Errors.WafError(
            "The engine's user_settings.options as '{}' file is corrupt: {}".format(engine_user_settings_options_file,
                                                                                    e))
    except ConfigParser.NoOptionError:
        pass
    try:
        override_bootstrap_third_party_override = engine_user_settings_reader.get('Misc Options',
                                                                                  'bootstrap_third_party_override')
    except ConfigParser.NoOptionError:
        pass
    return override_bootstrap_tool_param, override_bootstrap_third_party_override


def process_settings_include_file(settings_include_file, processed_env_dict, processed_settings_dict, processed_attributes_dict, processed_files=[]):
    """
    Process an 'include' settings file and extract/update the following attributes from the 'include' settings file:
    
    'attributes'
    'settings'
    'env'
    'env/{configuration}
    
    :param settings_include_file:       The settings file specified in the 'include' section of the platform settings file
    :param processed_env_dict:          The working 'env','env/{configuration}' dictionary thgat will be updated during the processing of the settings include file
    :param processed_settings_dict:     The working 'settings' dictionary thgat will be updated during the processing of the settings include file
    :param processed_attributes_dict:   The working 'attributess' dictionary that will be updated during the processing of the settings include file
    :param processed_files:             List of processed files (visited) to prevent a cyclic loop
    """
    
    if settings_include_file in processed_files:
        # Cycle detected, ignore
        return
    
    # Read in the settings dictionary from the file
    settings_include_dict = read_common_config(settings_include_file)

    # Process the includes first (if any)
    base_directory = os.path.dirname(settings_include_file)
    additional_includes = settings_include_dict.get('includes', [])
    if additional_includes:
        if not isinstance(additional_includes, list):
            raise Errors.WafError("'includes' key in env file '{}' must refer to a list of filenames.".format(settings_include_file))
        for include_file_name in additional_includes:
            include_env_file = os.path.normpath(os.path.join(base_directory, include_file_name))
            process_settings_include_file(settings_include_file=include_env_file,
                                          processed_env_dict=processed_env_dict,
                                          processed_settings_dict=processed_settings_dict,
                                          processed_attributes_dict=processed_attributes_dict,
                                          processed_files=processed_files + [settings_include_file])
            
    # Second, process all the env* keys
    for check_key, check_value in settings_include_dict.items():
        if check_key=='env' or check_key.startswith('env/'):
            process_env_dict(settings_include_dict, processed_env_dict)
            
    # Merge in any 'settings' entry
    settings_dict = settings_include_dict.get('settings', {})
    for additional_setting_group, additional_settings_value in settings_dict.items():
        merge_settings_group(settings_include_file=settings_include_file,
                             merge_settings_dict=processed_settings_dict,
                             setting_group_name=additional_setting_group,
                             settings_value_dicts=additional_settings_value)

    # Merge in any 'attributes' dictionary
    attributes_dict = settings_include_dict.get('attributes', {})
    merge_attributes_group(settings_include_file=settings_include_file,
                           source_attributes_dict=attributes_dict,
                           merge_attributes_dict=processed_attributes_dict)
   

class PlatformSettings(object):
    """
    Class represents a target platform setting based on the platform configurations in the _WAF_/settings/platforms subfolder
    """

    def __init__(self, platform_setting_file):
        """
        Init
        :param platform_settings:   The platform settings json dictionary read in from the platform definition file
        """
        platform_settings = utils.parse_json_file(platform_setting_file, True)
        self.platform = platform_settings["platform"]
        self.display_name = platform_settings.get("display_name", self.platform)
        self.hosts = platform_settings["hosts"]

        self.aliases = set()

        # Apply the aliases if any
        aliases_string = platform_settings.get('aliases', None)
        if aliases_string:
            for alias_key in aliases_string.split(','):
                self.aliases.add(alias_key.strip())

        self.has_server = platform_settings.get("has_server", False)
        self.has_test = platform_settings.get("has_tests", False)
        self.is_monolithic = platform_settings.get("is_monolithic", False)
        self.enabled = platform_settings.get("enabled", True)
        self.additional_modules = platform_settings.get("modules", [])
        self.needs_java = platform_settings.get("needs_java", False)
        
        env_dict = {}
        
        # Check if there are any includes of additional common settings
        platform_settings_base = os.path.dirname(platform_setting_file)
        additional_includes = platform_settings.get("includes", [])
        self.settings = {}
        self.attributes =  {}
        
        for additional_include in additional_includes:
            additional_include_file = os.path.normpath(os.path.join(platform_settings_base, additional_include))
            process_settings_include_file(settings_include_file=additional_include_file,
                                          processed_env_dict=env_dict,
                                          processed_settings_dict=self.settings,
                                          processed_attributes_dict=self.attributes,
                                          processed_files=[platform_setting_file])

        # Process all the env* keys
        process_env_dict(platform_settings, env_dict)

        # Merge in the current 'settings' over any that were processed from the include settings file
        platform_settings_dict = platform_settings.get("settings", {})
        for platform_setting_group, platform_settings_value in platform_settings_dict.items():
            merge_settings_group(settings_include_file=platform_setting_file,
                                 merge_settings_dict=self.settings,
                                 setting_group_name=platform_setting_group,
                                 settings_value_dicts=platform_settings_value)
            
        # Merge in the current 'attributes' over and that were process from the include settings file
        attributes_dict = platform_settings.get("attributes", {})
        merge_attributes_group(settings_include_file=platform_setting_file,
                               source_attributes_dict=attributes_dict,
                               merge_attributes_dict=self.attributes)

        self.env_dict = env_dict


class ConfigurationSettings(object):
    """
    Class represents a configuration setting based on the configurations defined in the _WAF_/settings/build_configurations.json
    """

    def __init__(self, name, is_monolithic, has_test, third_party_config, base_config=None):
        """
        Initialize

        :param name:                The base name for the configuration
        :param is_monolithic:       Flag indicating if the configuration is monolithic
        :param has_test:            Flag indicating if the configuration is a test-able configuration
        :param third_party_config:  Mapping for build type from this configuration to a corresponding 3rd party configuration defined in the _WAF_/3rdPArty definition
                                    files. (except for special cases where the 3rd party library actually maps to LY base configurations)
        :param base_config:         If this is a derived/custom config, then it must have a base configuration (debug, profile, performance, or release) to provide a fallback to
        """
        self.name = name
        self.is_monolithic = is_monolithic
        self.has_test = has_test
        self.third_party_config = third_party_config
        self.base_config = base_config

    def build_config_name(self, is_test, is_server):
        """
        Build the output configuration name for this configuration
        :param is_test:     Build a name for a test variant of this configurationm
        :param is_server:   Build a name for a server variant of this configurationm
        :return: The config name based on optional variants
        """
        config_name = self.name
        if is_test:
            config_name += '_test'
        if is_server:
            config_name += '_dedicated'
        return config_name

    def get_output_folder_ext(self):
        """
        Get the output folder extension to tag to the end of the output folder for the current build target
        :return: The output folder extension for this configuration
        """
        option_key = 'output_folder_ext_{}'.format(self.name)
        return LUMBERYARD_SETTINGS.get_settings_value(option_key)
    
    def does_configuration_match(self, match_name, check_base=True):
        """
        Test this configuration setting if it matches a configuration name.
        :param match_name:  The name to match
        :param check_base:  Flag to search the base config (if this is a derived custom config)
        :return: Match result
        """
        if self.name == match_name:
            return True
        elif self.base_config and check_base:
            if self.base_config.name == match_name:
                return True
        else:
            return False


class Settings(object):
    """
    Class to manage user settings based on the default settings and any override value in user_settings.options
    """

    def __init__(self):
        """
        Initialize
        """

        # Map of build configuration names to their ConfigurationSettings instance
        self.build_configuration_map = {}

        # Map of build configuration aliases to their concrete build configuration names
        self.build_configuration_alias_map = {}

        # Map of all concrete build configuration names
        self.all_build_configuration_names = {}

        # Set of any runtime override attributes (from the command line) that will override the current settings
        self.runtime_override_attributes = set()

        # Map of settings attributes to their final value (from default, user_settings, or command line)
        self.settings_map = {}

        # Map of target platform names to their PlatformSettings instance
        self.platform_settings_map = {}

        # Map of platform aliases to their concrete platform names
        self.platform_alias_map = {}

        # The base env node that is build configuration independent
        self.base_env = {}

        # Map of the settings attribute to their default values
        self.default_settings_map = {}
        
        # Check the engine.json file to determine where the engine root is so we can look for the default settings file
        engine_json_path = os.path.join(os.getcwd(), 'engine.json')
        if not os.path.isfile(engine_json_path):
            raise Errors.WafError("Missing required engine configuration file: {}".format(engine_json_path))
        
        engine_json = utils.parse_json_file(engine_json_path)
        if 'ExternalEnginePath' in engine_json:
            external_engine_path = engine_json['ExternalEnginePath']
            if os.path.isabs(external_engine_path):
                engine_root_abs = external_engine_path
            else:
                engine_root_abs = os.path.normpath(os.path.join(os.getcwd(), external_engine_path))
        else:
            engine_root_abs = os.getcwd()

        is_external_engine = os.path.normcase(os.getcwd()) != os.path.normcase(engine_root_abs)
        
        base_settings_path = os.path.join(engine_root_abs, '_WAF_')
        self.default_settings_path = os.path.join(base_settings_path, 'default_settings.json')
        
        # Load the required default_settings file
        if not os.path.exists(self.default_settings_path):
            raise Errors.WafError("Missing required default settings file: {}".format(self.default_settings_path))
        self.default_settings = utils.parse_json_file(self.default_settings_path)

        # Load the platform settings from the _WAF_/settings/platforms folder
        self.platform_settings_map.clear()
        extra_settings_path = os.path.join(base_settings_path, 'settings')
        platform_settings_path = os.path.join(extra_settings_path, 'platforms')
        self.load_platform_settings(platform_settings_path)
        
        # Optionally load the platform settings from the restricted platform folder if any
        restricted_platforms_path = os.path.join(platform_settings_path, 'restricted')
        if os.path.isdir(restricted_platforms_path):
            self.load_platform_settings(restricted_platforms_path)

        # Initialize and load the user_settings (if it exists). It will always exist in the current root's _WAF_
        # folder
        self.override_settings = ConfigParser.ConfigParser()
        self.user_settings_path = os.path.join(os.getcwd(), '_WAF_', 'user_settings.options')
        if os.path.exists(self.user_settings_path):
            self.override_settings.read(self.user_settings_path)
            
        if is_external_engine:
            # For external projects, the 'enabled_game_projects' can only be the same as the value in bootstrap.cfg
            override_enabled_game = read_game_name_from_bootstrap(os.path.join(os.getcwd(), 'bootstrap.cfg'))

            # For external projects, the attributes set in the engine's user_settings.options based on the configuration set by
            # setup assistant must be always overwritten since we can't run setup assistant for an external game project
            engine_user_settings_options_file = os.path.join(engine_root_abs, '_WAF_', 'user_settings.options')
            override_bootstrap_tool_param, override_bootstrap_third_party_override = read_bootstrap_overrides_from_settings_file(engine_user_settings_options_file)
            if not override_bootstrap_tool_param:
                raise Errors.WafError("Unable to determine this project's capabilities. Make sure that the engine at '{} 'is properly configured first".format(engine_root_abs))
        else:
            override_bootstrap_tool_param = None
            override_enabled_game = None
            override_bootstrap_third_party_override = None

        # Initialize the settings maps and update the settings override (comments) with the current default values
        self.update_settings_map(override_enabled_projects=override_enabled_game,
                                 override_bootstrap_tool_param=override_bootstrap_tool_param,
                                 override_bootstrap_third_party_override=override_bootstrap_third_party_override)

        # Load the configuration settings from _WAF_/settings/build_configurations.json
        build_settings_file = os.path.join(extra_settings_path, 'build_configurations.json')
        self.initialize_configurations(build_settings_file)

        # Update the settings map again with any value found in the configuration
        self.update_settings_map(override_enabled_projects=override_enabled_game,
                                 override_bootstrap_tool_param=override_bootstrap_tool_param,
                                 override_bootstrap_third_party_override=override_bootstrap_third_party_override)
        
        self.update_settings_file(apply_from_command_line=False,
                                  override_enabled_projects=override_enabled_game,
                                  override_bootstrap_tool_param=override_bootstrap_tool_param,
                                  override_bootstrap_third_party_override=override_bootstrap_third_party_override)
        

    def create_config(self):
        """
        Create a ConfigParser object for legacy operations such as gui_tasks
        :return: Config parser object set to the current settings values
        """

        config = ConfigParser.ConfigParser()

        for section, section_values in self.default_settings.items():
            if not config.has_section(section):
                config.add_section(section)

            for section_value_dict in section_values:
                attribute = section_value_dict.get(SETTING_KEY_ATTRIBUTE)
                value = self.get_settings_value(attribute)
                config.set(section, attribute, value)

        return config

    def load_platform_settings(self, platform_settings_path):
        """
        Scan through the platform settings folder and add the platform settings to the platform settings dictionary.
        For each platform settings, also apply any platform 'settings' in the definitions to the overall settings
        file. The attribute for the platform-specific setting will be prefixed by the platform name itself.
        For example:

            'output_folder' for 'win_x64_vs2015' will be evaluated to 'win_x64_vs2015_output_folder' in the options

        :param platform_settings_path:  the path to scan for the platform settings file

        """
        if not os.path.exists(platform_settings_path):
            return
        glob_search_pattern = os.path.join(platform_settings_path, 'platform.*.json')
        platform_setting_files = glob.glob(glob_search_pattern)
        host_platform = Utils.unversioned_sys_platform()

        for platform_setting_file in platform_setting_files:
            try:
                # Parse and evaluate each 'platform.XXX.json' configuration file found in the glob
                platform_setting = PlatformSettings(platform_setting_file)

                supported_host_platforms = [host.strip() for host in platform_setting.hosts.split(',')]
                if host_platform not in supported_host_platforms:
                    # Skip platforms not supported on the current host
                    continue

                self.apply_platform_settings_to_default_options(platform_setting)
                
                self.apply_platform_folder_settings(platform_setting)

                self.platform_settings_map[platform_setting.platform] = platform_setting
                
                # Apply the platform name to any aliases specified by the platform
                for platform_alias in platform_setting.aliases:
                    append_to_unique_list(self.platform_alias_map.setdefault(platform_alias, []), platform_setting.platform)

            except (KeyError, ValueError) as err:
                raise Errors.WafError("Invalid platform settings file '{}': {}".format(platform_setting_file, str(err)))

    def add_configuration(self, has_test_configs, has_server_configs, default_folder_ext, configuration):
        """
        Add a configuration instance to this settings object and apply the relevant aliases as well.

        :param has_test_configs:        Does the configuration have a test variant
        :param has_server_configs:      Does the configuration have a server variant
        :param default_folder_ext:      The default build configuration folder extension
        :param configuration:           The configuration instance to add
        """

        def _apply_alias(alias_name, config_names):
            append_to_unique_list(self.build_configuration_alias_map.setdefault(alias_name, list()), config_names)

        def _apply_configuration_folder_ext_settings(config_name):
            build_config_group = 'Output Folder'
            if build_config_group not in self.default_settings:
                self.default_settings[build_config_group] = []

            config_folder_ext_settings = {
                SETTING_KEY_LONG_FORM: '--output-folder-ext-{}'.format(config_name),
                SETTING_KEY_ATTRIBUTE: 'output_folder_ext_{}'.format(config_name),
                SETTING_KEY_DEFAULT_VALUE: default_folder_ext,
                SETTING_KEY_DESCRIPTION: 'The output folder name extension for {} builds'.format(config_name)
            }
            self.default_settings[build_config_group].append(config_folder_ext_settings)

        config_key = configuration.name
        self.build_configuration_map[config_key] = configuration

        base_config_name = configuration.build_config_name(False, False)
        self.all_build_configuration_names[base_config_name] = configuration

        _apply_configuration_folder_ext_settings(config_key)

        # Apply the base configuration name to the 'all' list
        _apply_alias('all', base_config_name)

        # Add to the monolithic type and monolithic_all alias
        monolithic_alias = 'monolithic' if configuration.is_monolithic else 'non_monolithic'
        _apply_alias(monolithic_alias, base_config_name)
        _apply_alias('{}_all'.format(monolithic_alias), base_config_name)

        # If the configuration has a base config, then apply the it the base config's _all alias as well
        if configuration.base_config:
            base_config_all = '{}_all'.format(configuration.base_config.build_config_name(False, False))
            _apply_alias(base_config_all, base_config_name)

        # Apply the rules for optional test configs
        if has_test_configs:
            # Add the base to a 'non_test' alias if we have test configurations enabled
            _apply_alias('non_test', base_config_name)

            if configuration.has_test:
                # Add test configurations to 'test_only'
                test_config_name = configuration.build_config_name(True, False)
                self.all_build_configuration_names[test_config_name] = configuration

                _apply_alias('test_all', test_config_name)
                # Create a derived alias with an '_all' to represent the configuration and its test counterpart
                _apply_alias('{}_all'.format(base_config_name), [base_config_name, test_config_name])
                # If the configuration has a base config, then apply the it the base config's _all alias as well
                if configuration.base_config:
                    base_config_all = '{}_all'.format(configuration.base_config.build_config_name(False, False))
                    _apply_alias(base_config_all, test_config_name)
                # Add the _test configuration to 'all'
                _apply_alias('all', test_config_name)
                # Add to the monolithic type _all
                _apply_alias('{}_all'.format(monolithic_alias), [base_config_name, test_config_name])

        if has_server_configs:
            # Add the base to a 'non-server' configuration if we have server configurations
            _apply_alias('non_dedicated', base_config_name)

            # Add the server configuration to a 'server' alias
            server_config_name = configuration.build_config_name(False, True)
            self.all_build_configuration_names[server_config_name] = configuration

            _apply_alias('dedicated', server_config_name)
            # Add to the monolithic type alias
            _apply_alias(monolithic_alias, server_config_name)
            # Add the _server configuration to 'all'
            _apply_alias('all', server_config_name)

            if has_test_configs and configuration.has_test:
                test_server_config_name = configuration.build_config_name(True, True)
                self.all_build_configuration_names[test_server_config_name] = configuration

                # Add the _test_server configuration to 'server' alias
                _apply_alias('server', test_server_config_name)
                # Add the _test_server configuration to 'test' alias
                _apply_alias('test', test_server_config_name)
                # Add the _test_server configuration to 'all'
                _apply_alias('all', test_server_config_name)
                # Add the _test_server configuration to 'test_all'
                _apply_alias('test_all', test_server_config_name)
                # Add to the monolithic type _all
                _apply_alias('{}_all'.format(monolithic_alias), [base_config_name, test_server_config_name])

    def process_env_node(self, env_node):
        """
        Process the base 'env' node
        :param env_node:    The base env node
        """
        if not env_node:
            return None
        processed_subnodes = {}
        for node_name, node_dict in env_node.items():

            if not isinstance(node_dict, dict):
                raise Errors.WafError("Invalid type found in the base 'env' node for the build configuration file for item '{}'. It must be a dictionary.".format(node_name))

            concrete_attributes = {}
            pending_attributes = {}

            for env_attribute_name, env_attribute_list in node_dict.items():

                if not isinstance(env_attribute_list, list):
                    raise Errors.WafError(
                        "Invalid type found in the 'env/{}' node for the build configuration file for item '{}'. It must be a list of strings.".format(node_name,env_attribute_name))

                has_alias = False
                for env_attribute_item in env_attribute_list:
                    if env_attribute_item.startswith('@'):
                        has_alias = True
                        break
                if has_alias:
                    pending_attributes[env_attribute_name] = env_attribute_list[:]
                else:
                    concrete_attributes[env_attribute_name] = env_attribute_list[:]

            while True:
                if not pending_attributes:
                    # No more pending attributes to update
                    break

                pending_attribute_key = pending_attributes.keys()[0]
                pending_attribute_values = pending_attributes.pop(pending_attribute_key)

                processed_list = []
                has_alias = False
                alias_expanded = False
                for pending_attribute_value in pending_attribute_values:
                    if pending_attribute_value.startswith('@'):
                        alias_key = pending_attribute_value[1:]
                        if alias_key == pending_attribute_key:
                            raise Errors.WafError("Cyclic alias dependency detected in the env configuration for alias '{}'".format(alias_key))
                        elif alias_key in concrete_attributes:
                            alias_expanded = True
                            for aliased_value in concrete_attributes[alias_key]:
                                processed_list.append(aliased_value)
                        elif alias_key not in pending_attributes:
                            raise Errors.WafError("Invalid alias detected in the env configuration:'{}'".format(alias_key))
                        else:
                            processed_list.append(pending_attribute_value)
                            has_alias = True
                    else:
                        processed_list.append(pending_attribute_value)
                if has_alias:
                    pending_attributes[pending_attribute_key] = processed_list
                else:
                    concrete_attributes[pending_attribute_key] = processed_list

                if not alias_expanded:
                    raise Errors.WafError("Invalid alias(es) detected in the env definition. (Possible cycle)")

            processed_subnodes[node_name] = dict(concrete_attributes)
        return processed_subnodes

    def initialize_configurations(self, configuration_settings_file):
        """
        Load the settings for the build configurations

        :param configuration_settings_file: The absolute path to the build_configurations.json file
        :return: The map of build configurations to their ConfigurationSettings
        """

        has_test_configs = utils.is_value_true(self.get_settings_value('has_test_configs'))
        has_server_configs = utils.is_value_true(self.get_settings_value('has_server_configs'))

        self.build_configuration_map.clear()
        self.build_configuration_alias_map.clear()
        self.all_build_configuration_names.clear()

        if not os.path.exists(configuration_settings_file):
            raise Errors.WafError('Invalid/missing build configuration settings file:{}'.format(configuration_settings_file))

        build_configurations_json = utils.parse_json_file(configuration_settings_file, allow_non_standard_comments=True)

        env_node = build_configurations_json.get('env', None)
        self.base_env = self.process_env_node(env_node)

        configurations_node = build_configurations_json.get('configurations', None)
        if not configurations_node:
            raise Errors.WafError("Invalid build configuration settings file {}. Missing 'configurations' node".format(
                configuration_settings_file))

        base_configuration_map = {}

        # First pass build up the base configuration
        base_configurations = set()
        for key, value_dict in configurations_node.items():

            base_configuration = value_dict.get('base', None)
            if not base_configuration:

                default_folder_ext = value_dict['default_output_ext']

                try:
                    base_configurations.add(key)
                    config = ConfigurationSettings(name=key,
                                                   is_monolithic=value_dict['is_monolithic'],
                                                   has_test=value_dict['has_test'],
                                                   third_party_config=value_dict['third_party_config'],
                                                   base_config=None)
                    base_configuration_map[key] = config
                    self.add_configuration(has_test_configs=has_test_configs,
                                           has_server_configs=has_server_configs,
                                           default_folder_ext=default_folder_ext,
                                           configuration=config)
                except KeyError as e:
                    raise Errors.WafError("Invalid key while looking up Configuration values: {}".format(str(e)))

        for key, value_dict in configurations_node.items():
            if key not in base_configurations:

                # Locate the base configuration
                base_configuration_key = value_dict.get('base', None)
                default_folder_ext = value_dict['default_output_ext']
                assert base_configuration_key, "'base' key for non-base configuration must exist"
                if base_configuration_key not in base_configuration_map:
                    raise Errors.WafError("Invalid base configuration '{}' for configuration '{}' in file '{}'".format(base_configuration_key, key, configuration_settings_file))
                base_configuration = base_configuration_map[base_configuration_key]

                # Create the derived configuration
                config = ConfigurationSettings(name=key,
                                               is_monolithic=value_dict.get('is_monolithic', base_configuration.is_monolithic),
                                               has_test=value_dict.get('has_test', base_configuration.has_test),
                                               third_party_config=value_dict.get('third_party_config', base_configuration.third_party_config),
                                               base_config=base_configuration)

                self.add_configuration(configuration=config,
                                       has_test_configs=has_test_configs,
                                       has_server_configs=has_server_configs,
                                       default_folder_ext=value_dict.get('default_output_ext', default_folder_ext))

        for config_key in self.build_configuration_map.keys():
            Logs.debug('settings: Initialized Build Configuration: {}'.format(config_key))
        for alias_name, alias_values in self.build_configuration_alias_map.items():
            Logs.debug('settings: Build Configuration alias {}: {}'.format(alias_name,','.join(alias_values)))
            
    @staticmethod
    def apply_optional_override(long_form, short_form, arguments):
        
        override_value = None
        
        # The highest priority is from the command line argument
        if not long_form and not short_form:
            return None
        
        arg_count = len(arguments)
        arg_index = 0
        while arg_index < arg_count:
            arg = arguments[arg_index]
            arg_tokens = arg.split('=')
            if arg_tokens[0] == long_form:
                # This matches the long form of an argument, we expect a value after the '=' sign
                if len(arg_tokens) > 1:
                    override_value = arg_tokens[1]
                elif '=' in arg:
                    # An equals was specified but no value was set, assume the desired value is an empty string
                    override_value = ''
                break
            elif short_form and arg_tokens[0] == short_form:
                # This matches the short form of an argument
                if '=' in arg:
                    # If the equal sign was used, accept the value parsed if any
                    raise Errors.WafError("Invalid usage of short form command argument '{}' "
                                          "({}). Use '{} {}' instead.".format(short_form,
                                                                              arg,
                                                                              short_form,
                                                                              arg_tokens[1] if len(arg_tokens) > 0 else '(Value)'))
                else:
                    # If not, then the next arg is expected to be the value
                    if (arg_index - 1) < arg_count:
                        override_value = arguments[arg_index + 1]
                        arg_index += 1
                        break
                    else:
                        break
                break
            arg_index += 1
        return override_value

    def update_settings_map(self, override_enabled_projects=None, override_bootstrap_tool_param=None, override_bootstrap_third_party_override=None):
        """
        Update the attribute -> settings map. There are potentially three sources of the value for each attribute:
            1. The default_value set in default_settings.json (or any default value callback result)
            2. The override value set in user_settings.options
            3. Any passed in argument that caan override the settings
        :return: tuple: The overall attribute->settings value map, attribute->settings value from command line overrides
        """
        # Convert the default settings with the applied override settings into a proper map
        attribute_to_section = {}

        self.settings_map.clear()
        self.runtime_override_attributes.clear()

        for section_key, section_settings in self.default_settings.items():

            for section_setting in section_settings:

                attribute = section_setting.get(SETTING_KEY_ATTRIBUTE)
                # Attribute is required
                if not attribute:
                    raise Errors.WafError('Invalid default_settings.json. Missing attribute in section {}'.format(section_key))

                # Attribute must be unique regardless of which section its in
                if attribute in attribute_to_section:
                    raise Errors.WafError("Duplicate attribute '{}' in section '{}' already defined in section '{}' in file {}"
                                          .format(attribute,
                                                  section_key,
                                                  attribute_to_section[attribute],
                                                  self.default_settings_path))

                attribute_to_section[attribute] = section_key

                # Lowest priority for the value is the default value from default_settings.json
                value = _stringify(section_setting.get(SETTING_KEY_DEFAULT_VALUE, ''))

                # Keep track of the default value of the attribute
                self.default_settings_map[attribute] = value
                
                # Check if the default is a callback function
                value = apply_optional_callback(value, attribute)

                # If there are any of the external project related overrides that are passed in, apply it here
                if override_enabled_projects and attribute == 'enabled_game_projects':
                    value = override_enabled_projects
                elif override_bootstrap_tool_param and attribute == 'bootstrap_tool_param':
                    value = override_bootstrap_tool_param
                elif override_bootstrap_third_party_override and attribute == 'bootstrap_third_party_override':
                    value = override_bootstrap_third_party_override

                # Next Highest Priority is the override value in user_settings.json
                if self.override_settings.has_option(section_key, attribute):
                    value = self.override_settings.get(section_key, attribute)

                # The highest priority is from the command line argument
                long_form = section_setting.get(SETTING_KEY_LONG_FORM, None)
                short_form = section_setting.get(SETTING_KEY_SHORT_FORM, None)
                if long_form or short_form:
    
                    override_value = Settings.apply_optional_override(long_form, short_form, sys.argv)
                    if override_value is not None:
                        value = override_value
                        self.runtime_override_attributes.add(attribute)

                # Make sure the value is a string
                value = _stringify(value)

                self.settings_map[attribute] = value

    def update_settings_file(self, apply_from_command_line, override_enabled_projects=None, override_bootstrap_tool_param=None, override_bootstrap_third_party_override=None):
        """
        Update the user_settings.options if there is a change in any precomputed hash.  If there was any change to the
        default settings or user settings, then the user_settings.options wil be updated
        """
        def _get_override_hash_from_file(settings_override_file):
            """
            Try to read the last hash of the settings override file
            """
            hash_value = None
            if os.path.exists(settings_override_file):
                with open(settings_override_file, 'r') as override_settings_file:
                    override_settings_content = override_settings_file.read()

                match = re.match(r'; hash=(.+)', override_settings_content)
                if match:
                    hash_value = match.group(1)

            return hash_value

        def _build_override_contents_and_hash(last_hash, default_settings, override_settings):
            """
            Build up the user_settings contents based on any current user_settings override and compute the has
            to see if the entire user_settings.options file needs to be udpated
            """
            user_settings_content = "; This file represents the override for waf settings based on '{}'\n".format(self.default_settings_path)
            user_settings_content += "; To override any of the following items, remove the ';' comment and set the appropriate value.\n"
            user_settings_content += "; Otherwise the values will be read from default_settings.json.\n\n"

            # Load default settings
            for settings_group, settings_list in default_settings.items():
                user_settings_content += '\n[{}]\n\n'.format(settings_group)

                # Iterate over all options in this group
                for settings in settings_list:

                    option_name = settings.get(SETTING_KEY_ATTRIBUTE, None)
                    default_value = _stringify(settings.get(SETTING_KEY_DEFAULT_VALUE, ''))

                    if apply_from_command_line and option_name in self.runtime_override_attributes:
                        override_value = self.settings_map[option_name]
                    elif override_settings and override_settings.has_option(settings_group, option_name):
                        override_value = override_settings.get(settings_group, option_name)
                    else:
                        override_value = None
                        
                    if override_enabled_projects and option_name == 'enabled_game_projects':
                        override_value = override_enabled_projects
                    elif override_bootstrap_tool_param and option_name == 'bootstrap_tool_param':
                        override_value = override_bootstrap_tool_param
                    elif override_bootstrap_third_party_override and option_name == 'bootstrap_third_party_override':
                        override_value = override_bootstrap_third_party_override

                    has_existing_override = (override_value != default_value and not last_hash)
                    if override_value and (has_existing_override or last_hash):
                        user_settings_content += '{} = {}\n'.format(option_name, override_value)
                    else:
                        user_settings_content += ';{} = {}\n'.format(option_name, default_value)

            digest = hashlib.md5()
            digest.update(user_settings_content)
            hash_result = digest.hexdigest()

            return user_settings_content, hash_result

        assert self.user_settings_path, 'self.user_settings_path must be initialized first'
        assert self.default_settings, 'self.default_settings must be initialized first'
        assert self.override_settings, 'self.override_settings must be initialized first'

        # Read the current settings
        last_hash = _get_override_hash_from_file(self.user_settings_path)
        content, current_hash = _build_override_contents_and_hash(last_hash, self.default_settings, self.override_settings)
        if current_hash != last_hash:
            update_contents = '; hash={}\n{}'.format(current_hash, content)
            with open(self.user_settings_path, 'w') as user_settings_file:
                user_settings_file.write(update_contents)

    def get_settings_value(self, attribute_name):
        """
        Get a settings value by its attribute name
        :param attribute_name:  The name of the attribute to lookup
        :return: The value for the attribute
        """
        if attribute_name not in self.settings_map:
            raise Errors.WafError("Invalid attribute '{}' when looking for its settings value.".format(attribute_name))
        return self.settings_map[attribute_name]

    def set_settings_value(self, attribute_name, value):
        """
        Set the value for a settings attribute
        :param attribute_name:  The attribute name to set the value for
        :param value:           The value to set to
        """
        if attribute_name not in self.settings_map:
            raise Errors.WafError("Invalid attribute '{}' when looking for its settings value.".format(attribute_name))
        self.runtime_override_attributes.add(attribute_name)
        self.settings_map[attribute_name] = value

    def apply_to_options(self, options):
        """
        Apply the settings that this object manages to a context's options
        :param options: The options to apply to
        """
        for attribute, value in self.settings_map.items():
            setattr(options, attribute, value)

    def apply_to_settings_context(self, ctx):
        """
        Apply all the possible values from the default_settings as arguments to the options context
        :param ctx:     The options context to apply to
        """
        for settings_group, settings_list in self.default_settings.items():
            group = ctx.add_option_group(settings_group)

            # Iterate over all options in this group
            for settings in settings_list:
                
                attribute = settings.get(SETTING_KEY_ATTRIBUTE, None)
                assert attribute, "Option in group {} is missing mandatory setting '{}'".format(settings_group, SETTING_KEY_ATTRIBUTE)
                
                long_form = settings.get(SETTING_KEY_LONG_FORM, None)
                assert long_form, "Option in group {} is missing mandatory setting '{}'".format(settings_group, SETTING_KEY_LONG_FORM)

                short_form = settings.get(SETTING_KEY_SHORT_FORM, None)
                description = settings.get(SETTING_KEY_DESCRIPTION, '')
                default_value = self.settings_map.get(attribute, '') # Use the evaluated default here to reflect any overridden value in user_settings.options

                if short_form:
                    group.add_option(short_form, long_form,
                                     dest=attribute,
                                     action='store',
                                     default=str(default_value),
                                     help=description)
                else:
                    group.add_option(long_form,
                                     dest=attribute,
                                     action='store',
                                     default=str(default_value),
                                     help=description)

    def get_default_sections(self):
        """
        Function to support legacy operations that need the list of sections in the default_settings file
        :return: Copy of the list of sections
        """
        default_sections = self.default_settings.keys()[:]
        return default_sections

    def get_default_options(self):
        """
        Function to support legacy operations that need the json object of the default settings. This returns a deep
        copy of the settings
        :return: Deep copy of the default settings json object
        """
        return copy.deepcopy(self.default_settings)

    def get_default_value_and_description(self, section_name, setting_name):
        """
        Function to support legacy operations such as the show_option_dialog command to get the default value and
        description of the attribute
        :param section_name: The section name of the setting to lookup
        :param setting_name: The name of the setting to lookup
        :return:
        """

        for settings_group, settings_list in self.default_settings.items():
            if settings_group == section_name:
                for settings in settings_list:
                    if settings[SETTING_KEY_ATTRIBUTE] == setting_name:
                        default_value = settings.get(SETTING_KEY_DEFAULT_VALUE, '')
                        default_description = settings.get(SETTING_KEY_DESCRIPTION, '')
                        return apply_optional_callback(default_value, setting_name), default_description
        raise Errors.WafError("Invalid default section/setting:  {}/{}".format(section_name,setting_name))

    def get_platform_settings(self, platform_name):
        """
        Get the platform settings instance based on its name
        :param platform_name: The platform name to lookup
        """
        try:
            return self.platform_settings_map[platform_name]
        except KeyError:
            raise Errors.WafError("Invalid platform name '{}' for platform settings.".format(platform_name))

    def get_all_platform_settings(self):
        """
        Get the list of all the platform settings instances
        """
        return self.platform_settings_map.values()

    def get_all_platform_names(self):
        """
        Get the list of all the platform settings instances
        """
        return self.platform_settings_map.keys()

    def get_build_configuration_settings(self):
        """
        Get the list of all ConfigurationDetails settings instances
        """
        return self.build_configuration_map.values()

    def get_build_configuration_setting(self, configuration):
        """
        Get a ConfigurationDetail settings instance based on a root configuration name
        :param configuration:   The root configuration name
        """
        try:
            return self.build_configuration_map[configuration]
        except KeyError:
            raise Errors.WafError("Invalid build configuration '{}'".format(configuration))

    def get_configurations_for_alias(self, alias):
        """
        Get all the concrete configuration names for an alias
        :param alias: The alias to lookup the configuration names
        """
        try:
            # If the alias is actually a valid configuration name, then return it
            if alias in self.all_build_configuration_names.keys():
                return [alias]
            # If the alias is in the alias list, return it
            return self.build_configuration_alias_map[alias]
        except KeyError:
            raise Errors.WafError("Invalid configuration alias '{}'".format(alias))

    def get_platforms_for_alias(self, alias):
        """
        Get all of the concrete platform names for an alias
        :param alias: The alias to lookup the configuration names
        :return: The concrete platform names for valid aliases.
        """
        try:
            if alias in self.platform_settings_map:
                return [alias]
            return self.platform_alias_map[alias]
        except KeyError:
            raise Errors.WafError("Invalid platform alias '{}'".format(alias))

    def is_valid_configuration(self, config_name):
        """
        Check if a configuration name is a valid configuration
        :param config_name: The name to check
        :return: True if its valid, False if not
        """
        return config_name in self.all_build_configuration_names
    
    def get_root_configuration_name(self, config_name):
        """
        Given a configuration name, return the root configuration. In other words, <config>, <config>_dedicated, <config>_test, etc
        all map to the same <config>
        
        :param config_name: The (derived) configuration name to lookup
        :return: The root configuration
        """
        try:
            return self.all_build_configuration_names[config_name]
        except KeyError:
            raise Errors.WafError("Invalid configuration '{}'".format(config_name))

    def is_valid_configuration_alias(self, config_alias):
        """
        Check if an alias is a valid configuration alias
        :param config_alias: The alias to check
        :return: True if its valid, False if not
        """
        return config_alias in self.build_configuration_alias_map

    def get_configuration_names(self):
        """
        Return all of the configuration names
        """
        return self.all_build_configuration_names.keys()

    def get_configuration_aliases(self):
        """
        Return all of the configuration aliases
        """
        return self.build_configuration_alias_map.keys()

    def apply_platform_settings_to_default_options(self, platform):
        """
        Apply any platform settings in a platform configuration file to the current options.
        
        :param platform:    The deserialized platform configuration to process anyt optional settings
        """
        
        # This call is only applicable for platforms that have custom platform-specific settings
        if not platform.settings:
            return
        
        try:
            # Iterate through each definition in the settings dictionary
            for section_name, section_dict_list in platform.settings.items():
                
                if not isinstance(section_dict_list, list):
                    raise Errors.WafError("The settings dictionary must be a dictionary of section names (string) to list of dictionaries for the attributes.")
            
                platform_group = section_name
                applied_platform_settings = []
                for platform_settings in section_dict_list:
                    
                    # Process each dictionary of attributes
                    if not isinstance(platform_settings, dict):
                        raise Errors.WafError("Entry must be a dictionary of values for the setting.")

                    # 'attribute' is required
                    setting_attribute = platform_settings.get(SETTING_KEY_ATTRIBUTE, '').strip()
                    if not setting_attribute:
                        raise Errors.WafError("Entry missing required '{}' key".format(SETTING_KEY_ATTRIBUTE))
                    
                    # 'long_form' is required
                    setting_long_form = platform_settings.get(SETTING_KEY_LONG_FORM, '').strip()
                    if not setting_long_form:
                        raise Errors.WafError("Entry '{}' missing required '{}' key".format(setting_attribute, SETTING_KEY_LONG_FORM))

                    description = platform_settings.get(SETTING_KEY_DESCRIPTION, '')
                    if isinstance(description, list):
                        description = '\n'.join(description)

                    applied_platform_setting = {
                        SETTING_KEY_LONG_FORM: setting_long_form,
                        SETTING_KEY_ATTRIBUTE: setting_attribute,
                        SETTING_KEY_DEFAULT_VALUE: platform_settings.get(SETTING_KEY_DEFAULT_VALUE, '').strip(),
                        SETTING_KEY_DESCRIPTION: description.strip()
                    }
                    # 'short_form' is optional and only applied if provided
                    setting_short_form = platform_settings.get(SETTING_KEY_SHORT_FORM, '').strip()
                    if setting_short_form:
                        applied_platform_setting[SETTING_KEY_SHORT_FORM] = applied_platform_setting
                        
                    applied_platform_settings.append(applied_platform_setting)
                    
                self.add_platform_settings(platform_name=platform.platform,
                                           platform_group=platform_group,
                                           platform_settings=applied_platform_settings)
                
        except Errors.WafError as waf_err:
            raise Errors.WafError("Invalid platform settings file (platform '{}'). {}".format(platform.platform, waf_err))

    def add_platform_settings(self, platform_name, platform_group, platform_settings):

        if platform_group not in self.default_settings:
            # If the platform group doesnt exist, then just set it to current list
            self.default_settings[platform_group] = platform_settings

        else:
            # If the platform group exists, we need to merge it to the existing group
    
            # Build of a map of the current settings, using 'attributes' as the primary key
            previous_platform_map = {}
            for previous_platform_settings in self.default_settings[platform_group]:
                previous_attribute_key = previous_platform_settings[SETTING_KEY_ATTRIBUTE]
                previous_platform_map[previous_attribute_key] = previous_platform_settings
    
            # Iterate through the platform settings that are being set
            for update_platform_settings in platform_settings:
                update_attribute_key = update_platform_settings[SETTING_KEY_ATTRIBUTE]
                if update_attribute_key in previous_platform_map:
            
                    # Update/Validate the results (with warnings)
            
                    # Long form cannot change, raise an error if its different
                    current_long_form = previous_platform_map[update_attribute_key][SETTING_KEY_LONG_FORM]
                    update_long_form = update_platform_settings[SETTING_KEY_LONG_FORM]
                    if update_long_form != current_long_form:
                        raise Errors.WafError("Conflicting '{}' attribute for platform '{}' detected for "
                                              "attribute '{}': ({}!={})".format(SETTING_KEY_LONG_FORM,
                                                                                platform_name,
                                                                                update_attribute_key,
                                                                                update_long_form,
                                                                                current_long_form))
                    # Warn on any changes to default values
                    current_default_value = previous_platform_map[update_attribute_key][SETTING_KEY_DEFAULT_VALUE]
                    update_default_value = update_platform_settings[SETTING_KEY_DEFAULT_VALUE]
                    if update_default_value != current_default_value:
                        previous_platform_map[update_attribute_key][SETTING_KEY_DEFAULT_VALUE] = update_default_value
                        Logs.warn("[WARN] '{}' for settings attribute '{}' for platform '{}' "
                                  "overwritten. ({}->{}) '".format(SETTING_KEY_DEFAULT_VALUE,
                                                                   update_attribute_key,
                                                                   platform_name,
                                                                   current_default_value,
                                                                   update_default_value))
            
                    # Warn on any changes to the description
                    current_description = previous_platform_map[update_attribute_key][SETTING_KEY_DESCRIPTION]
                    update_description = update_platform_settings[SETTING_KEY_DESCRIPTION]
                    if update_description != current_description:
                        previous_platform_map[update_attribute_key][SETTING_KEY_DESCRIPTION] = update_description
                        Logs.warn("[WARN] '{}' for settings attribute '{}' for platform '{}' "
                                  "overwritten. ({}->{}) '".format(SETTING_KEY_DESCRIPTION,
                                                                   update_attribute_key,
                                                                   platform_name,
                                                                   current_description,
                                                                   update_description))
                else:
                    # New attribute, add to the attributes for the current platform group
                    self.default_settings[platform_group].append(update_platform_settings)
                    
    def apply_platform_folder_settings(self, platform):
        
        build_config_group = 'Output Folder'
        if build_config_group not in self.default_settings:
            self.default_settings[build_config_group] = []

        platform_name = platform.platform

        long_form_platform_name = platform.attributes.get('legacy_platform_name',platform_name).replace('_', '-')

        try:
            default_value = platform.attributes['default_folder_name']
        except KeyError:
            raise Errors.WafError("Missing attribute 'default_folder_name' for platform settings for platform {}".format(platform.name))

        platform_folder_settings = {
            SETTING_KEY_LONG_FORM: '--output-folder-{}'.format(long_form_platform_name),
            SETTING_KEY_ATTRIBUTE: 'out_folder_{}'.format(platform_name),
            SETTING_KEY_DEFAULT_VALUE: default_value,
            SETTING_KEY_DESCRIPTION: 'The base output folder name for the {} platform.'.format(platform_name)
        }
        self.default_settings[build_config_group].append(platform_folder_settings)



LUMBERYARD_SETTINGS = Settings()


@conf
def get_all_platform_names(ctx):
    """
    Lookup the platform configuration object for a platform/configuration
    :param ctx:                 Context
    :param platform_name:       Platform Name to lookup
    :return: The settings for the platform
    """
    return LUMBERYARD_SETTINGS.get_all_platform_names()


@conf
def get_platform_settings(ctx, platform_name):
    """
    Lookup the platform configuration object for a platform/configuration
    :param ctx:                 Context
    :param platform_name:       Platform Name to lookup
    :return: The settings for the platform
    """
    return LUMBERYARD_SETTINGS.get_platform_settings(platform_name)


@conf
def get_all_configuration_names(ctx):
    """
    Get all the possible (platform independent) build configuration names
    :param ctx: Context
    :return: Set of all of the build configuration names
    """
    return LUMBERYARD_SETTINGS.get_configuration_names()


@conf
def get_settings_value(ctx, attribute_name):
    """
    Get a particular settings value based on the attribute name
    :param ctx:             Context
    :param attribute_name:  The name to lookup the value
    """
    return LUMBERYARD_SETTINGS.get_settings_value(attribute_name)


@conf
def get_platform_aliases(_, alias):
    return LUMBERYARD_SETTINGS.get_platforms_for_alias(alias)



PLATFORM_ONLY_ALIASES = {}

@conf
def platform_to_platform_alias(self, platform):
    """
    Return the generic platform alias the concrete platform is associated with
    """
    global PLATFORM_ONLY_ALIASES

    if not PLATFORM_ONLY_ALIASES:
        non_platform_alias_names = {'all', 'msvc', 'clang'}
        PLATFORM_ONLY_ALIASES = {
            alias : platforms for alias, platforms in LUMBERYARD_SETTINGS.platform_alias_map.iteritems() if alias not in non_platform_alias_names
        }

    for alias in PLATFORM_ONLY_ALIASES:
        if platform in PLATFORM_ONLY_ALIASES[alias] or alias == platform:
            return alias

    # the platform has no alias
    return platform


@conf
def get_default_platform_launcher_name(self, platform):
    platform_settings = LUMBERYARD_SETTINGS.get_platform_settings(platform)
    launcher_name = platform_settings.attributes.get('default_launcher_name', None)
    if not launcher_name:
        self.fatal('[ERROR] Unable to determine default launcher name for platform {}'.format(platform))
    return launcher_name


# Keep a blacklist of attributes we want to suppress the reporting of overrides on
SUPPRESS_REPORT_SETTINGS_OVERRIDES = ['bootstrap_tool_param',
                                      'enabled_game_projects']


@conf
def report_settings_overrides(ctx):
    """
    Iterate and report all of the settings and their default modules to modules that actually support the
    required (conf) decorated function for reporting.
    
    To provide a reporting handler for a setting, define conf a decorated function with the signature of:
    
    @conf
    def report_settings_$ATTRIBUTE NAME(ctx, is_configure_context, is_build, attribute, default_value, settings_value)
    
    where:
        $ATTRIBUTE_NAME: the name of the attribute that is defined in the settings template (either default_settings.json
                         or the platform definition file)
        ctx:             The current Context of the caller to the report_settings_* function
        is_configure:    Is the report running from a ConfigurationContext? (lmbr_waf configure)
        is_build:        Is the report running from a BuildContext?
        attribute:       The name of the attribute
        default_value:   The default value for the attribute as defined in the settings template for the attribute
        settings_value:  The current value for the attribute (either from the default or the override user_settings.options file)
        
    """
    
    is_configure = isinstance(ctx, ConfigurationContext)
    is_build = isinstance(ctx, BuildContext) and getattr(ctx, 'cmd', '').startswith('build_')

    # Only report during configure and build commands
    if not is_configure and not is_build:
        return

    # If we are in an incredibuild recursive loop, dont report to prevent double reporting
    if ctx.is_option_true('internal_dont_check_recursive_execution'):
        return

    for attribute, default_value in LUMBERYARD_SETTINGS.default_settings_map.items():
        
        # Skip any attributes that were blacklisted
        if attribute in SUPPRESS_REPORT_SETTINGS_OVERRIDES:
            continue
            
        # Check for default values that are tagged with '@<attribute_callback>' and process them accordingly
        processed_default_value = apply_optional_callback(default_value, attribute)
        
        settings_value = LUMBERYARD_SETTINGS.settings_map[attribute]
        report_settings_override_func_name = 'report_settings_{}'.format(attribute)
        report_settings_override_func = getattr(ctx, report_settings_override_func_name, None)
        if report_settings_override_func:
            report_settings_override_func(is_configure, is_build, attribute, processed_default_value, settings_value)
        else:
            ctx.default_report_settings_override(attribute, processed_default_value, settings_value)


@conf
def print_settings_override_message(ctx, msg):
    """
    Recommended method to print a settings override. This will perform a consistent tagging of the log and prevent
    duplicate messaging as well

    :param ctx:     Current Context
    :param msg:     The message to report
    """
    try:
        if msg in ctx.reported_messages:
            return
    except AttributeError:
        ctx.reported_messages = set()
        pass
    Logs.pprint('CYAN', '[SETTINGS] {}'.format(msg))
    ctx.reported_messages.add(msg)


@conf
def default_report_settings_override(ctx, attribute, default_value, settings_value):
    if default_value != settings_value:
        ctx.print_settings_override_message('{} = {} (default {})'.format(attribute,
                                                                          settings_value,
                                                                          default_value))


@conf
def report_settings_generate_sig_debug_output(ctx, is_configure, is_build, attribute, default_value, settings_value):
    
    if not is_build:
        return
    
    enable_sig_debug = utils.is_value_true(settings_value)
    if enable_sig_debug:
        ctx.print_settings_override_message('Enabling capturing of task signature data ({}={})'.format(attribute, settings_value))


