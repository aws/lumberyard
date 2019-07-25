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

"""
This module manages the build platforms and configurations. It uses the settings_manager module to read the
platform and build configurations under _WAF_/settings and derives all of the supported target platforms and
configurations.
"""

import shutil
import glob
import os
import json
import re

from waflib import Configure, ConfigSet, Context, Options, Utils, Logs, Errors
from waflib.Build import BuildContext, CleanContext, Context, CACHE_DIR
from waflib.Configure import conf, ConfigurationContext, REGISTERED_CONF_FUNCTIONS

from waf_branch_spec import LUMBERYARD_ENGINE_PATH, LMBR_WAF_VERSION_TAG, BINTEMP_CACHE_3RD_PARTY, BINTEMP_CACHE_TOOLS, BINTEMP_MODULE_DEF

import settings_manager
from utils import is_value_true, parse_json_file
from compile_settings_test import load_test_settings
from compile_settings_dedicated import load_dedicated_settings

WAF_TOOL_ROOT_DIR = os.path.join(LUMBERYARD_ENGINE_PATH, "Tools", "build","waf-1.7.13")
LMBR_WAF_TOOL_DIR = os.path.join(WAF_TOOL_ROOT_DIR, "lmbrwaflib")

# some files write a corresponding .timestamp files in BinTemp/ as part of the configuration process
if LUMBERYARD_ENGINE_PATH=='.':
    TIMESTAMP_CHECK_FILES = ['SetupAssistantConfig.json']
else:
    TIMESTAMP_CHECK_FILES = [os.path.join(LUMBERYARD_ENGINE_PATH, "SetupAssistantConfig.json"),'bootstrap.cfg']
ANDROID_TIMESTAMP_CHECK_FILES = ['_WAF_/android/android_settings.json', '_WAF_/environment.json']

TEST_EXTENSION = '.Test'
DEDICATED_EXTENSION = '.Dedicated'

ENABLE_TEST_CONFIGURATIONS = is_value_true(settings_manager.LUMBERYARD_SETTINGS.get_settings_value('has_test_configs'))
ENABLE_SERVER = is_value_true(settings_manager.LUMBERYARD_SETTINGS.get_settings_value('has_server_configs'))

REGEX_ENV_VALUE_ALIAS = re.compile('@([!\w]+)@')
REGEX_CONDITIONAL_ALIAS = re.compile('\\?([!\w]+)\\?')


class PlatformConfiguration(object):
    """
    Representation of a concrete platform configuration instance that is used during the build process. These concrete
    platforms configurations are create based on the parent platform definition and the configuration settings and are
    specific per platform.
    """

    def __init__(self, platform, settings, platform_output_folder, is_test, is_server, is_monolithic):
        """
        Initialize

        :param platform:                The parent Platform settings that owns this configuration
        :param settings:                The ConfigurationSettings object (from settings_manager) that this platform is created from
        :param platform_output_folder:  The output folder base name that is used as the base for each of the platform configurations output folder
        :param is_test:                 Flag: Is this a 'test' configuration
        :param is_server:               Flag: Is this a 'server' configuration
        :param is_monolithic:           Flag: Is this a 'monolithic' configuration
        """
        self.platform = platform
        self.settings = settings
        self.platform_output_folder = platform_output_folder
        self.is_test = is_test
        self.is_server = is_server
        self.is_monolithic = is_monolithic

    def config_name(self):
        """
        Get the concrete configuration name
        :return: the concrete configuration name
        """
        return self.settings.build_config_name(self.is_test, self.is_server)
    
    def platform_name(self):
        """
        Get the platform name
        :return: The concrete platform name
        """
        return self.platform.platform

    def output_folder(self):
        """
        Determine the output folder for this platform configuration instance based on the settings.
        :return: The output folder for this platform configuration instance.
        """
        output = self.platform_output_folder
        output_config_ext = self.settings.get_output_folder_ext()
        if output_config_ext:
            output += '.{}'.format(output_config_ext)
        if self.is_test:
            output += TEST_EXTENSION
        if self.is_server:
            output += DEDICATED_EXTENSION

        return output


APPEND_NONUNIQUE_KEY_LIST = ['LIB']


class PlatformDetail(object):
    """
    Represents a concrete target platform that available as a build target. Each platform detail instance manages
    is own list of available Platform Configurations (PlatformConfiguration) which represents each available individual
    build variant.
    """

    def __init__(self, platform, enabled, has_server, has_tests, is_monolithic_value, output_folder, aliases, attributes, needs_java, platform_env_dict):
        """
        Initialize

        :param platform:            The base name of this platform. (The concrete platform name may be extended with a server suffix if the right options are set)
        :param enabled:             Flag: Is this platform enabled or not
        :param has_server:          Flag: Are server platforms/configurations supported at all
        :param has_tests:           Flag: Does this platform support test configurations?
        :param is_monolithic_value: Flag: Is this a monolithic platform? (or a list of configurations to force monolithic builds on that configuration.)
        :param output_folder:       The output folder base name that is used as the base for each of the platform configurations output folder
        :param aliases:             Optional aliases to add this platform to
        :param attributes:          Optional platform/specific attributes needed for custom platform processing
        :param needs_java:          Flag to indicate if we need java to be loaded (the javaw module)
        :param platform_env_dict:    Optional env map to apply to the env for this platform during configure
        """
        self.platform = platform
        self.enabled = enabled
        self.has_server = has_server
        self.has_tests = has_tests

        # Handle the fact that 'is_monolithic' can be represented in multiple ways
        if not isinstance(is_monolithic_value, bool) and not isinstance(is_monolithic_value, list) and not isinstance(is_monolithic_value, str):
            raise Errors.WafError("Invalid settings value type for 'is_platform' for platform '{}'. Expected a boolean, string, or list of strings")
        if isinstance(is_monolithic_value, str):
            self.is_monolithic = [is_monolithic_value]
        else:
            self.is_monolithic = is_monolithic_value

        self.output_folder = output_folder

        self.aliases = aliases
        self.attributes = attributes
        self.needs_java = needs_java
        self.platform_env_dict = platform_env_dict

        # Initialize all the platform configurations based on the parameters and settings
        self.platform_configs = {}
        for config_settings in settings_manager.LUMBERYARD_SETTINGS.get_build_configuration_settings():

            # Determine if the configuration is monolithic or not
            if isinstance(self.is_monolithic, bool):
                # The 'is_monolithic' flag was set to a boolean value in the platform's configuration file
                if self.is_monolithic:
                    # The 'is_monolithic' flag was set to True, then set (override) all configurations to be monolithic
                    is_config_monolithic = True
                else:
                    # The 'is_monolithic' flag was set to False, then use the default configuration specific setting to
                    # control whether its monolithic or not. We will never override 'performance' or 'release' monolithic
                    # settings
                    is_config_monolithic = config_settings.is_monolithic
            elif isinstance(self.is_monolithic, list):
                # Special case: If the 'is_monolithic' is a list, then evaluate based on the contents of the list
                if len(self.is_monolithic) > 0:
                    # If there is at least one configuration specified in the list, then match the current config to any of the
                    # configurations to determine if its monolithic or not. This mechanism can only enable configurations
                    # that by default are non-monolithic. If the default value is monolithic, it cannot be turned off
                    is_config_monolithic = config_settings.name in self.is_monolithic or config_settings.is_monolithic
                else:
                    # If the list is empty, then that implies we do not override any of the default configuration's
                    # monolithic flag (same behavior as setting 'is_monolithic' to False
                    is_config_monolithic = config_settings.is_monolithic
            else:
                raise Errors.WafError("Invalid type for 'is_monolithic' in platform settings for {}".format(platform))
            
            # Add the base configuration
            base_config = PlatformConfiguration(platform=self,
                                                settings=config_settings,
                                                platform_output_folder=output_folder,
                                                is_test=False,
                                                is_server=False,
                                                is_monolithic=is_config_monolithic)
            self.platform_configs[base_config.config_name()] = base_config

            if has_server and ENABLE_SERVER:
                # Add the optional server configuration if available(has_server) and enabled(enable_server)
                non_test_server_config = PlatformConfiguration(platform=self,
                                                               settings=config_settings,
                                                               platform_output_folder=output_folder,
                                                               is_test=False,
                                                               is_server=True,
                                                               is_monolithic=is_config_monolithic)
                self.platform_configs[non_test_server_config.config_name()] = non_test_server_config

            if self.has_tests and config_settings.has_test and ENABLE_TEST_CONFIGURATIONS:
                # Add the optional test configuration(s) if available (config_detail.has_test) and enabled (enable_test_configurations)
                test_non_server_config = PlatformConfiguration(platform=self,
                                                               settings=config_settings,
                                                               platform_output_folder=output_folder,
                                                               is_test=True,
                                                               is_server=False,
                                                               is_monolithic=is_config_monolithic)
                self.platform_configs[test_non_server_config.config_name()] = test_non_server_config

                if has_server and ENABLE_SERVER:
                    # Add the optional server configuration if available(has_server) and enabled(enable_server)
                    test_server_config = PlatformConfiguration(platform=self,
                                                               settings=config_settings,
                                                               platform_output_folder=output_folder,
                                                               is_test=True,
                                                               is_server=True,
                                                               is_monolithic=is_config_monolithic)
                    self.platform_configs[test_server_config.config_name()] = test_server_config

        # Check if there is a 3rd party platform alias override, otherwise use the platform name
        config_third_party_platform_alias = self.attributes.get('third_party_alias_platform', None)
        if config_third_party_platform_alias:
            try:
                settings_manager.LUMBERYARD_SETTINGS.get_platform_settings(config_third_party_platform_alias)
            except Errors.WafError:
                raise Errors.WafError("Invalid 'third_party_alias_platform' attribute ({}) for platform {}".format(config_third_party_platform_alias, platform))
            self.third_party_platform_key = config_third_party_platform_alias
        else:
            self.third_party_platform_key = platform

    def name(self):
        """
        Get the concrete name for this platform
        """
        platform_name = self.platform
        return platform_name

    def get_configuration_details(self):
        """
        Return the list of all the platform configuration instances that this platform manages
        """
        return self.platform_configs.values()

    def get_configuration_names(self):
        """
        Return the list of all the configuration names that this platform manages
        :return:
        """
        return self.platform_configs.keys()

    def get_configuration(self, configuration_name):
        """
        Lookup a specific platform configuration detail based on a configuration name
        :param configuration_name:  The configuration name to lookup
        :return: The configuration details for a particular configuration name for this platform
        :exception Errors.WafError:
        """
        try:
            return self.platform_configs[configuration_name]
        except KeyError:
            raise Errors.WafError("Invalid configuration '{}' for platform '{}'".format(configuration_name, self.platform))

    def set_enabled(self, enabled=True):
        """
        Set this platform's enable flag
        :param enabled:     The enable value to set to
        """
        self.enabled = enabled

    def is_enabled(self):
        """
        Determines if this platform is enabled or not
        """
        return self.enabled

    def validate_platform_settings_functions(self, ctx):
        """
        Each platform settings file must have 2 required functions and 2 optional functions that are decorated with the @conf decorator:
        
            load_<platform_name>_common_settings(ctx)                       <- Required
            load_<platform_name>_configuration_settings(ctx, configurate)   <- Required
            is_<platform_name>_available(ctx)                               <- Optional (will default to true)
            
        This function will validate that the above functions have been declared and annotated with the conf decorated

        :param      ctx: The configuration context
        :param      default_func: Optional function to return if not part of the configuration context, otherwise throw and exception if the function cannot be found
        :return:    A tuple of the four function listed above
        """
        def _validate_func(func_name, default_func=None):
            if not hasattr(ctx, func_name) and not default_func:
                raise Errors.WafError('Compile rules script for platform {} missing required function {}'.format(self.platform, func_name))
            
            return getattr(ctx, func_name, default_func)
        
        def _default_is_platform_available(ctx):
            return True
        
        def _default_get_output_folder(ctx):
            output_folder_key = '{}_out_folder'.format(self.platform)
            if not hasattr(ctx.options, output_folder_key):
                raise Errors.WafError("Missing configuration value '{}' in settings for platform {}.".format(output_folder_key, self.platform))
            return getattr(ctx.options, output_folder_key)

        load_platform_common_settings_func = _validate_func('load_{}_common_settings'.format(self.platform))
        load_platform_configuration_settings_func = _validate_func('load_{}_configuration_settings'.format(self.platform))
        is_platform_available_func = _validate_func('is_{}_available'.format(self.platform), _default_is_platform_available)
        
        return load_platform_common_settings_func, load_platform_configuration_settings_func, is_platform_available_func

    def apply_env_rules_to_env(self, ctx, configuration='_'):
        
        def _process_key(input_key):
            """
            Process an env key and check for any conditional tag of the form:
            
            @ENV_CONDITION@     - Condition based on a boolean env[] key
            ?OPTION_CONDITION?  - Condition based on a boolean option condition
            
            The name of the condition may be preceded with a '!' character to represent a negation of the value. The
            value that the condition represents must be a boolean
            
            Example:
                "@IS_WINDOWS@:DEFINES": [ "WINDOWS" ],
            
            :param input_key: The key to evaluate
            :return: The processed key value if no condition or the condition exists and is satisfied, otherwise None
            """
            
            # Look for the optional conditional delimiter
            conditional_marker_index = input_key.find(':')
            if conditional_marker_index >= 0:
                
                processed_input_key = input_key[conditional_marker_index+1:]
                conditional_key = input_key[:conditional_marker_index]
                
                match_condition = REGEX_ENV_VALUE_ALIAS.search(conditional_key)
                if match_condition:
                    # The match condition refers to an env value
                    environment_key = match_condition.group(1)
                    
                    # Check the optional negation token
                    negation = environment_key and environment_key[0] == '!'
                    if negation:
                        environment_key = environment_key[1:]

                    # Environment key must exist
                    if environment_key not in ctx.env:
                        Logs.debug("lumberyard_configure: Conditional Environment key '{}' not found. Skipping conditional key '{}'".format(environment_key, input_key))
                        return None
                    
                    # Environment key must be a boolean
                    environment_key_value = ctx.env[environment_key]
                    if not isinstance(environment_key_value,bool):
                        Logs.debug("lumberyard_configure: Conditional Environment key '{}' is not a boolean value. Skipping conditional key '{}'".format(environment_key, input_key))
                        return None
                    
                    if negation and environment_key_value:
                        return None

                    if not negation and not environment_key_value:
                        return None
                    
                    return processed_input_key

                match_condition = REGEX_CONDITIONAL_ALIAS.search(conditional_key)
                if match_condition:
                    # The match condition refers to an options value
                    option_key = match_condition.group(1)
                    
                    negation = option_key and option_key[0] == '!'
                    if negation:
                        option_key = option_key[1:]
                    
                    # The option must exist
                    if not hasattr(ctx.options, option_key):
                        Logs.debug("lumberyard_configure: Conditional option key '{}' not found. Skipping conditional key '{}'".format(option_key, input_key))
                        return None
                    
                    if negation and ctx.is_option_true(option_key):
                        return None
                    
                    if not negation and not ctx.is_option_true(option_key):
                        return None
                    
                    return processed_input_key
                
                Logs.warn("[WARN] Invalid env key '{}'".format(input_key))
                return None
            else:
                return input_key

        def _process_value(input_value):
            """
            Process a value, substituting and environmental alias key '@ENV_ALIAS@' in its definition
            
            :param input_value: The input value to process
            :return: The process value
            """
            
            # Look for all the matched potential '@ENV_ALIAS@' values in the input and build a substitution list
            alias_match_groups = REGEX_ENV_VALUE_ALIAS.findall(input_value)
            env_match_pairs = []
            for match_group in alias_match_groups:
                match_alias = '@{}@'.format(match_group)
                match_env_key = match_group
                if match_env_key in ctx.env:
                    match_env_value = ctx.env[match_env_key]
                    env_match_pairs.append((match_alias, str(match_env_value)))
            processed_value = input_value
            
            # Go through the substitution list and perform the necessary replacements
            for env_match_pair in env_match_pairs:
                processed_value = processed_value.replace(env_match_pair[0], env_match_pair[1])
                
            return processed_value

        # Get the seperate env dictionary if any
        env_dict = self.platform_env_dict.get(configuration, None)
        if not env_dict:
            return
    
        # Process the environment list into the context's environment table
        for env_key_string, env_values in env_dict.items():
            
            processed_key = _process_key(env_key_string)
            if not processed_key:
                continue
                
            if isinstance(env_values, list):
                
                if processed_key not in ctx.env:
                    # If the env values is a list but the key doesnt exist in the env yet, start with an empty one
                    ctx.env[processed_key] = []
    
                for env_value in env_values:
                    
                    processed_value = _process_value(env_value)
                    if processed_key in APPEND_NONUNIQUE_KEY_LIST:
                        ctx.env[processed_key] += [processed_value]
                    elif processed_value not in ctx.env[processed_key]:
                        ctx.env[processed_key] += [processed_value]
            else:
                processed_value = _process_value(env_values)
                ctx.env[processed_key] = processed_value
                

def _read_platforms_for_host():
    """
    Private initializer function to initialize the map of platform names to their PlatformDetails instance and a map of aliases to their platform names
    :return:    Tuple of : [0] Map of platform names to their PlatformDetails object
                           [1] Map of aliases to a list of platform names that belong in that alias
    """

    validated_platforms_for_host = {}
    alias_to_platforms_map = {}

    # Attempt to read the environment json file that is set by setup assistant
    try:
        environment_json_path = os.path.join(os.getcwd(), '_WAF_', 'environment.json')
        environment_json = parse_json_file(environment_json_path)
    except:
        environment_json = None

    for platform_setting in settings_manager.LUMBERYARD_SETTINGS.get_all_platform_settings():

        # Apply the default aliases if any
        for alias in platform_setting.aliases:
            alias_to_platforms_map.setdefault(alias, set()).add(platform_setting.platform)

        # Extract the base output folder for the platform
        output_folder_key = 'out_folder_{}'.format(platform_setting.platform)
        try:
            output_folder = settings_manager.LUMBERYARD_SETTINGS.get_settings_value(output_folder_key)
        except Errors.WafError:
            raise Errors.WafError("Missing required 'out_folder' in the settings for platform settings {}".format(platform_setting.platform))

        # Determine if the platform is monolithic-only
        is_monolithic = platform_setting.is_monolithic
        
        # Check if there are any platform-specific monolithic override flag
        platform_check_list = [platform_setting.platform] + list(platform_setting.aliases)
        # Special case: for darwin, the original override key was 'mac_monolithic_build', so also check for that
        if platform_setting.platform == 'darwin_x64':
            platform_check_list.append('mac')

        for alias in platform_check_list:
            monolithic_key = '{}_build_monolithic'.format(alias)
            if not monolithic_key in settings_manager.LUMBERYARD_SETTINGS.settings_map:
                continue
            if is_value_true(settings_manager.LUMBERYARD_SETTINGS.settings_map[monolithic_key]):
                is_monolithic = True
                break

        # Determine if the platform is enabled and available
        is_enabled = platform_setting.enabled
        
        # Special case. Android also relies on a special environment file
        if 'android' in platform_setting.aliases:
            if environment_json:
                is_enabled = is_value_true(environment_json.get('ENABLE_ANDROID', 'False'))
    
        # Create the base platform detail object from the platform settings
        base_platform = PlatformDetail(platform=platform_setting.platform,
                                       enabled=is_enabled,
                                       has_server=platform_setting.has_server,
                                       has_tests=platform_setting.has_test,
                                       is_monolithic_value=is_monolithic,
                                       output_folder=output_folder,
                                       aliases=platform_setting.aliases,
                                       attributes=platform_setting.attributes,
                                       needs_java=platform_setting.needs_java,
                                       platform_env_dict=platform_setting.env_dict)
        validated_platforms_for_host[base_platform.name()] = base_platform

    for platform_key in validated_platforms_for_host.keys():
        Logs.debug('settings: Initialized Target Platform: {}'.format(platform_key))
    for alias_name, alias_values in alias_to_platforms_map.items():
        Logs.debug('settings: Platform Alias {}: {}'.format(alias_name, ','.join(alias_values)))

    return validated_platforms_for_host, alias_to_platforms_map


PLATFORM_MAP, ALIAS_TO_PLATFORMS_MAP = _read_platforms_for_host()


@conf
def is_target_platform(ctx, platform):
    """
    Determine if the platform is a target platform or a configure/platform generator platform
    :param ctx:         Context
    :param platform:    Platform to check
    :return:            True if it is a target platform, False if not
    """
    return platform and platform not in ('project_generator', [])


@conf
def check_platform_explicit_boolean_attribute(ctx, platform, attribute):
    """
    Look up an explicit platform specific boolean custom attribute
    :param ctx:         Context
    :param platform:    Explicit platform to check
    :param attribute:   Boolean attribute to check
    :return: Result of the boolean attribute. (False if the platform is not a target platform)
    """
    if ctx.is_target_platform(platform):
        return is_value_true(ctx.get_platform_attribute(platform, attribute, False))
    else:
        return False


@conf
def is_windows_platform(ctx, platform=None):
    """
    Checks to see if the platform is a windows platform. Returns true if it is, otherwise false
    """
    if not platform:
        platform = ctx.env['PLATFORM']
    is_windows = check_platform_explicit_boolean_attribute(ctx, platform, 'is_windows')
    if not is_windows:
        return False
    else:
        return True


@conf
def is_apple_platform(ctx, platform=None):
    """
    Checks to see if the platform is an apple platform. Returns true if it is, otherwise false
    """
    if not platform:
        platform = ctx.env['PLATFORM']
    is_mac = check_platform_explicit_boolean_attribute(ctx, platform, 'is_apple')
    if not is_mac:
        return False
    else:
        return True


@conf
def is_mac_platform(ctx, platform=None):
    """
    Checks to see if the platform is a mac platform. Returns true if it is, otherwise false
    """
    if not platform:
        platform = ctx.env['PLATFORM']
    is_mac = check_platform_explicit_boolean_attribute(ctx, platform, 'is_mac')
    if not is_mac:
        return False
    else:
        return True


@conf
def is_android_platform(ctx, platform):
    """
    Safely checks to see if a platform is an explicit android platform. Returns true if it is, otherwise false
    """
    is_android = check_platform_explicit_boolean_attribute(ctx, platform, 'is_android')
    if not is_android:
        return False
    else:
        return True


@conf
def is_linux_platform(ctx, platform):
    """
    Safely checks to see if a platform is an explicit linux platform. Returns true if it is, otherwise false
    """
    is_linux = check_platform_explicit_boolean_attribute(ctx, platform, 'is_linux')
    if not is_linux:
        return False
    else:
        return True


@conf
def get_target_platform_detail(ctx, platform_name):
    """
    Get a PlatformDetail object based on a platform name
    :param ctx:             Context
    :param platform_name:   Name of the platform to look up
    :exception  Errors.WafError
    """
    try:
        return PLATFORM_MAP[platform_name]
    except KeyError:
        raise Exception("Invalid platform name '{}'. Make sure it is defined in the platform configuration file.".format(platform_name))


VALIDATED_CONFIGURATIONS_FILENAME = "valid_configuration_platforms.json"


@conf
def get_validated_platforms_node(conf):
    """
    Get the validated platforms file node, which tracks which target platforms are enabled during a build
    :param conf: Context
    :return: Node3 object of the validated platforms file
    """
    return conf.get_bintemp_folder_node().make_node(VALIDATED_CONFIGURATIONS_FILENAME)


@conf
def update_valid_configurations_file(ctx):
    """
    Update the validated platforms file that is used for the build commands to see which platforms are valid and enabled
    :param ctx:     Context
    """
    current_available_platforms = ctx.get_enabled_target_platforms(reset_cache=True,
                                                                   apply_validated_platforms=False)
    validated_platforms_node = ctx.get_validated_platforms_node()

    # write out the list of platforms that were successfully configured
    platform_names = [platform.name() for platform in current_available_platforms]
    json_data = json.dumps(platform_names)
    try:
        validated_platforms_node.write(json_data)
    except Exception as e:
        # If the file cannot be written to, warn, but dont fail.
        Logs.warn("[WARN] Unable to update validated configurations file '{}' ({})".format(validated_platforms_node.abspath(),e.message))


@conf
def enable_target_platform(ctx, platform_name):
    """
    Enable a target platform
    :param ctx:             Context
    :param platform_name:   Name of the platform to enable
    """
    get_target_platform_detail(ctx, platform_name).set_enabled(True)
    ctx.get_enabled_target_platforms(True)


@conf
def disable_target_platform(ctx, platform):
    """
    Disable a target platform
    :param ctx:             Context
    :param platform_name:   Name of the platform to disable
    """
    get_target_platform_detail(ctx, platform).set_enabled(False)
    ctx.get_enabled_target_platforms(True)


@conf
def get_all_target_platforms(ctx, apply_valid_platform_filter):
    """
    Get all platforms that are configured for this host
    :param ctx:                             Context
    :param apply_valid_platform_filter:     Flag to apply the 'validated_platforms' file as a filter
    :return: List of target platforms
    """
    def _platform_sort_key(item):
        return item.name()

    enabled_platforms = []

    # Optionally load the platform filter list if possible
    cached_valid_platforms = None
    if apply_valid_platform_filter:
        validated_platforms_node = ctx.get_validated_platforms_node()
        if os.path.exists(validated_platforms_node.abspath()):
            cached_valid_platforms = ctx.parse_json_file(validated_platforms_node)

    for platform_name, platform_details in PLATFORM_MAP.items():
        if cached_valid_platforms and platform_name not in cached_valid_platforms:
            continue
        if not platform_details.is_enabled():
            Logs.debug("Skipping disabled platform '{}'".format(platform_name))
            continue
        enabled_platforms.append(platform_details)

    enabled_platforms.sort(key=_platform_sort_key)

    return enabled_platforms


@conf
def get_enabled_target_platforms(ctx, reset_cache=False, apply_validated_platforms=True):
    """
    Get the enabled target platforms that were successfully configured
    :param ctx:             Context
    :param reset_cache:     Option to reset the cached enabled platforms that this function manages
    :param apply_validated_platforms:   Option to apply the validated platform cache or not
    :return:    List of enabled target platforms
    """
    try:
        if reset_cache:
            ctx.enabled_platforms = ctx.get_all_target_platforms(apply_validated_platforms)
        return ctx.enabled_platforms
    except AttributeError:
        ctx.enabled_platforms = ctx.get_all_target_platforms(apply_validated_platforms)
        return ctx.enabled_platforms


@conf
def get_enabled_target_platform_names(ctx):
    """
    Get a list of enabled target platform names
    :param ctx: Context
    """
    enabled_platform_names = [platform.name() for platform in ctx.get_enabled_target_platforms()]
    return enabled_platform_names


@conf
def is_target_platform_enabled(ctx, platform_name):
    """
    Check if a platform is enabled
    :param ctx:             Context
    :param platform_name:   The platform to check
    :return:                True if this is a valid/enabled platform, False if not
    """
    if platform_name in ALIAS_TO_PLATFORMS_MAP:
        # If the platform name represents an alias, then check the enabled flag across all of the platforms that alias
        # represents
        aliased_platforms = ALIAS_TO_PLATFORMS_MAP[platform_name]
        for aliased_platform in aliased_platforms:
            if PLATFORM_MAP[aliased_platform].enabled:
                return True
        return False
    elif platform_name not in PLATFORM_MAP:
        # The platform name is not recognized
        return False
    else:
        # Check against the concrete platform name
        return ctx.get_target_platform_detail(platform_name).enabled


@conf
def get_platforms_for_alias(ctx, alias_name):
    return list(ALIAS_TO_PLATFORMS_MAP.get(alias_name,set()))


@conf
def get_output_target_folder(ctx, platform_name, configuration_name):
    """
    Determine the output target folder for a platform/configuration combination
    :param ctx:                 Context
    :param platform_name:       The platform name to look up
    :param configuration_name:  The configuration name look up
    :return: The final target output folder based on the platform/configuration input
    """

    platform_details = ctx.get_target_platform_detail(platform_name)

    platform_configuration = platform_details.get_configuration(configuration_name)

    return platform_configuration.output_folder()

@conf
def get_platform_attribute(ctx, platform, attribute, default=None):
    """
    Request any custom platform attribute that is tagged in the platform settings (under the optional 'attributes' key)
    :param ctx:         Context
    :param platform:    Platform to get the attribute
    :param attribute:   The attribute to lookup
    :param default:     The default value if the attribute does not exist
    :return:    The custom attribute for the platform, or the default if it doesnt exist
    """
    return ctx.get_target_platform_detail(platform).attributes.get(attribute, default)


@conf
def get_platform_configuration(ctx, platform, configuration):
    """
    Lookup the configuration settings for a configuration for a platform
    :param ctx:             Context
    :param platform:        Platform
    :param configuration:   Configuration
    :return: The settings for the configuration
    """
    platform_detail = ctx.get_target_platform_detail(platform)
    try:
        return platform_detail.platform_configs[configuration]
    except KeyError:
        raise Errors.WafError("Invalid configuration '{}'".format(configuration))


@conf
def get_platform_configuration_settings(ctx, platform, configuration):
    """
    Lookup the platform configuration object for a platform/configuration
    :param ctx:             Context
    :param platform:        Platform
    :param configuration:   Configuration
    :return: The settings for the configuration
    """
    return ctx.get_platform_configuration(platform, configuration).settings


@conf
def get_current_platform_list(ctx, platform=None, include_alias=True):
    """
    Method to preprocess a platform into a list of platforms. This method handles the case of when the project is
    a non-target platform, in which case it will return all enabled platforms.

    :param ctx:             Context
    :param platform:        Platform to search for any alias. (If none, then use the current platform)
    :param include_alias:   Option to include platform aliases as well
    :return:            List of current platforms
    """
    if not platform:
        platform = ctx.env['PLATFORM']

    if platform in ('project_generator', []):
        # Special case: For project generator contexts, (like MSVS, Xcode), the it may override the target platform
        # specific to its need. If set, then use that instead.
        if hasattr(ctx, 'get_target_platforms'):
            check_platforms = ctx.get_target_platforms()
        else:
            check_platforms = [platform.name() for platform in ctx.get_enabled_target_platforms()]
    else:
        check_platforms = [platform]

    try:
        current_platforms = []
        for check_platform in check_platforms:
            current_platforms.append(check_platform)
            if include_alias:
                for alias, aliased_platforms in ALIAS_TO_PLATFORMS_MAP.items():
                    if check_platform in aliased_platforms and alias not in current_platforms:
                        current_platforms.append(alias)
        return current_platforms
    except KeyError:
        raise Errors.WafError("Invalid platform '{}'".format(check_platforms))


@conf
def get_supported_configurations(ctx, platform_name):
    """
    Get the list of supported configuration names for a platform
    :param ctx:             Context
    :param platform_name:   The name of the platform to lookup the configuration
    """
    platform_configurations = ctx.get_target_platform_detail(platform_name).get_configuration_names()
    return platform_configurations


@conf
def is_build_monolithic(ctx, platform=None, configuration=None):
    """
    Check to see if any variation of the desired platform and configuration is intended
    to be built as monolithic
    :param platform:        Optional platform to specify, otherwise use the current target platform from the context
    :param configuration:   Optional configuration to specify, otherwise use the current target configuration from the context
    """
    platform = platform or getattr(ctx, 'target_platform', None)
    configuration = configuration or getattr(ctx, 'target_configuration', None)
    if not platform or not configuration:
        # If neither platform or configuration is set, then this is not a build_ or clean_ command (and cannot be monolithic)
        return False

    if isinstance(configuration, str):
        configuration = [configuration]

    # Get the list of platforms without the alias so we can look them up in the platform table
    platform_list = ctx.get_current_platform_list(platform, False)
    for platform in platform_list:
        platform_details = ctx.get_target_platform_detail(platform)
        if isinstance(platform_details.is_monolithic, bool) and platform_details.is_monolithic:
            return True
        if configuration:
            # If there are configurations to check, then check those
            for check_configuration in configuration:
                if platform_details.get_configuration(check_configuration).is_monolithic:
                    return True

    return False


@conf
def preprocess_target_platforms(ctx, platforms, auto_populate_empty=False):
    """
    Preprocess a list of platforms from user input and list the concrete platform(s). The resulting list will contain
    concrete platforms expanded from platform aliases if any
    :param ctx:                 Context
    :param platforms:           The list of platforms to preprocess
    :param auto_populate_empty: Option- If the list of platforms is empty, then fill the platforms with all of the enabled platforms
    :return List of concrete platforms from the input platform
    """
    processed_platforms = set()
    if (auto_populate_empty and len(platforms) == 0) or 'all' in platforms:
        for platform in PLATFORM_MAP.keys():
            processed_platforms.add(platform)
    else:
        for platform in platforms:
            if platform in PLATFORM_MAP:
                processed_platforms.add(platform)
            elif platform in ALIAS_TO_PLATFORMS_MAP:
                aliases_platforms = ALIAS_TO_PLATFORMS_MAP[platform]
                for aliased_platform in aliases_platforms:
                    processed_platforms.add(aliased_platform)
    return processed_platforms


@conf
def preprocess_target_configuration_list(ctx, target, target_platform, configurations, auto_populate_empty=False):
    """
    Preprocess a list of configurations based on an optional target_platform filter from user input
    :param ctx:                 Context
    :param target:              Optional target name to report target based errors
    :param target_platform:     The target platform for the configuration list
    :param configurations:      The list of configurations to preprocess
    :param auto_populate_empty: Option to auto-populate the list of configurations for the platform if the input list of configurations is empty
    :return: List of concrete configurations
    """
    if target_platform:
        # Collect the valid configuration names for the platform
        platform_detail = ctx.get_target_platform_detail(target_platform)
        valid_configuration_names = [config_detail.config_name() for config_detail in platform_detail.platform_configs.values()]
    else:
        # If no platform was specified,
        valid_configuration_names = settings_manager.LUMBERYARD_SETTINGS.get_configurations_for_alias('all')

    processed_configurations = set()
    if (auto_populate_empty and len(configurations) == 0) or 'all' in configurations:
        for configuration in valid_configuration_names:
            processed_configurations.add(configuration)
    else:
        for configuration in configurations:
            # Matches an existing configuration
            if configuration in valid_configuration_names:
                processed_configurations.add(configuration)
                continue

            # Matches an alias
            if settings_manager.LUMBERYARD_SETTINGS.is_valid_configuration_alias(configuration):
                configurations_for_alias = settings_manager.LUMBERYARD_SETTINGS.get_configurations_for_alias(configuration)
                if configurations_for_alias:
                    processed_configurations.update(set(configurations_for_alias))
                    continue

            # Matches a platform/configuration pattern
            if ':' in configuration and target_platform:
                # Could be a platform-only configuration, split into components
                configuration_parts = configuration.split(':')
                split_platform = configuration_parts[0]
                split_configuration = configuration_parts[1]
                split_configurations = []

                configurations_for_alias = settings_manager.LUMBERYARD_SETTINGS.get_configurations_for_alias(split_configuration)
                if configurations_for_alias:
                    split_configurations += configurations_for_alias
                elif split_configuration in platform_detail.platform_configs:
                    split_configurations += [split_configuration]
                else:
                    if target:
                        Logs.warn('[WARN] invalid configuration value {} for target {}.'.format(configuration, target))
                    else:
                        Logs.warn('[WARN] invalid configuration value {}.'.format(configuration))
                    continue

                # Determine if the split platform is a platform or an alias
                split_platforms = ctx.preprocess_target_platforms([split_platform])

                # Iterate through the platform(s) and add the matched configurations
                for split_platform in split_platforms:
                    if split_platform == target_platform:
                        for split_configuration in split_configurations:
                            processed_configurations.add(split_configuration)

    return processed_configurations


@conf
def is_valid_platform_request(ctx, debug_tag='lumberyard', **kw):
    target = kw['target']
    current_platform = ctx.env['PLATFORM']

    # if we're restricting to a platform, only build it if appropriate:
    if 'platforms' in kw:
        platforms_allowed = ctx.preprocess_target_platforms(kw['platforms'], True)   # this will be a list like [ 'android_armv7_gcc', 'android_armv7_clang' ]
        if len(platforms_allowed) > 0:
            if current_platform not in platforms_allowed:
                Logs.debug('{}: Disabled module {} because it is only for platforms {}, '
                            'we are not currently targeting that platform.'.format(debug_tag, target, platforms_allowed))
                return False
        else:
            return False

    return True

@conf
def is_valid_configuration_request(ctx, debug_tag='lumberyard', **kw):
    target = kw['target']

    current_platform = ctx.env['PLATFORM']
    current_configuration = ctx.env['CONFIGURATION']

    def debug_log(message, *args):
        Logs.debug('{}: {}'.format(debug_tag, message), *args)

    # If we're restricting to a configuration, only build if appropriate
    if 'configurations' in kw:
        configurations_allowed = ctx.preprocess_target_configuration_list(target, current_platform, kw['configurations'], True)
        if len(configurations_allowed) > 0:
            if current_configuration not in configurations_allowed:
                debug_log('Disabled module %s because it is only for configurations %s on platform %s', target, ','.join(configurations_allowed), current_platform)
                return False
        else:
            return False

    platform_details = ctx.get_target_platform_detail(current_platform)
    platform_configuration = platform_details.get_configuration(current_configuration)

    # If this is restricted to client or server only, check if the platform or configuration is a server
    client_only = kw.get('client_only', False)
    server_only = kw.get('server_only', False)
    if client_only and server_only:
        raise Errors.WafError("Keywords 'client_only' and 'server_only' defined in target '{}' are mutually exclusive. They cannot both be True.".format(target))
    if platform_configuration.is_server and client_only:
        debug_log('Module %s excluded from this server build (client_only=True).', target)
        return False
    if not platform_configuration.is_server and server_only:
        debug_log('Module %s excluded from this client build (server_only=True).', target)
        return False

    # If this is restricted to test configurations or non-test configurations only, check if the configuration is a test configuration
    test_only = kw.get('test_only', False)
    exclude_test = kw.get('exclude_test', False)
    if test_only and exclude_test:
        raise Errors.WafError("Keywords 'test_only' and 'exclude_test' defined in target '{}' are mutually exclusive. They cannot both be True.".format(target))
    if platform_configuration.is_test and exclude_test:
        debug_log('Module %s excluded from this test build (exclude_test=True).', target)
        return False
    if not platform_configuration.is_test and test_only:
        debug_log('Module %s excluded from this non-test build (test_only=True).', target)
        return False

    # If this is meant to be excluded from monolithic builds, check if the configuration is a monolithic one
    exclude_monolithic = kw.get('exclude_monolithic', False)
    if platform_configuration.is_monolithic and exclude_monolithic:
        debug_log('Module %s excluded from this monolithic build (exclude_monolithic=True).', target)
        return False

    return True


def initialize_common_compiler_settings(ctx):
    """
    Initialize the common env values that each platform will populate
    :param ctx: The ctx where the env will be initialized
    """

    env = ctx.env
    # Create empty env values to ensure appending always works
    env['DEFINES'] = []
    env['INCLUDES'] = []
    env['CXXFLAGS'] = []
    env['LIB'] = []
    env['LIBPATH'] = []
    env['LINKFLAGS'] = []
    env['BINDIR'] = ''
    env['LIBDIR'] = ''
    env['PREFIX'] = ''


def load_compile_rules_for_host(conf, waf_host_platform):
    """
    Load host specific compile rules

    :param conf:                Configuration context
    :param waf_host_platform:   The current waf host platform
    :return:                    The host function name to call for initialization
    """
    host_module_file = 'compile_rules_{}_host'.format(waf_host_platform)
    try:
        conf.load(host_module_file, tooldir=[LMBR_WAF_TOOL_DIR])
    except Exception as err:
        conf.fatal("[ERROR] Unable to load compile rules module file '{}': {}".format(host_module_file, str(err)))

    host_function_name = 'load_{}_host_settings'.format(waf_host_platform)
    if not hasattr(conf, host_function_name):
        conf.fatal('[ERROR] Required Configuration Function \'{}\' not found in configuration file {}'.format(host_function_name, host_module_file))

    return host_function_name


@conf
def load_compile_rules_for_enabled_platforms(ctx):
    """
    Load the compiler rules for all of the enabled platforms and initialize all of the build variant's environment
    :param ctx: The configure context
    """

    assert isinstance(ctx, ConfigurationContext), "This function can only be executed from the configure command"

    host_platform = ctx.get_waf_host_platform()

    vanilla_conf = ctx.env.derive()  # grab a snapshot of conf before you pollute it.

    host_function_name = load_compile_rules_for_host(ctx, host_platform)

    installed_platforms = []

    cached_uselib_readers = ctx.get_uselib_third_party_reader_map()

    Logs.info("[WAF] 'Initialize Build Variants' starting...")
    configure_timer = Utils.Timer()
    
    all_target_platforms = ctx.get_all_target_platforms(False)

    # Keep track of uselib's that we found in the 3rd party config files
    third_party_uselib_map = ctx.read_and_mark_3rd_party_libs()
    ctx.env['THIRD_PARTY_USELIBS'] = [uselib_name for uselib_name in third_party_uselib_map]

    # Save off configuration values from the uselib which are necessary during build for modules built with the uselib
    configuration_settings_map = {}
    for uselib_name in third_party_uselib_map:
        configuration_values = ctx.get_configuration_settings(uselib_name)
        if configuration_values:
            configuration_settings_map[uselib_name] = configuration_values
    ctx.env['THIRD_PARTY_USELIB_SETTINGS'] = configuration_settings_map
    
    # Perform a pre-validation on the platforms availability
    ctx.update_platform_availability_from_options()

    for enabled_target_platform in all_target_platforms:
        
        if not enabled_target_platform.enabled:
            continue

        platform_spec_vanilla_conf = vanilla_conf.derive()
        platform_spec_vanilla_conf.detach()

        configuration_names = enabled_target_platform.get_configuration_names()

        try:
            load_platform_common_settings_func, load_platform_configuration_settings_func, is_platform_available_func = enabled_target_platform.validate_platform_settings_functions(ctx)

            # Make sure the platform is available (silently)
            if not is_platform_available_func():
                ctx.disable_target_platform(enabled_target_platform.name())
                continue

            Logs.info('[WAF] Configure "{} - [{}]"'.format(enabled_target_platform.name(), ','.join(configuration_names)))

            # Prepare a platform environment that will be shared across each platform/configuration variant and load the platform's common settings into it
            
            # Set the env table for the platform base. 'setenv' will derive a new env based on the waf base env
            ctx.setenv(enabled_target_platform.name(), platform_spec_vanilla_conf)

            initialize_common_compiler_settings(ctx)

            # Apply the host platform settings for each platform/configuration variant
            getattr(ctx, host_function_name)()
            
            # Apply the target platform's common settings to the current context's env
            load_platform_common_settings_func()
    
            # platform installed
            installed_platforms.append(enabled_target_platform)
    
            # Keep the platform based env so that it can be derived for each platform+configuration variant
            platform_common_env = ctx.get_env()

            # Register the output folder reporter ('Target Output folder ...')
            ctx.register_output_folder_settings_reporter(enabled_target_platform.platform)

            # Iterate through this platforms configurations and set the env for each
            for enabled_configuration in enabled_target_platform.get_configuration_details():
                
                # Prepare a child env from the platform env and detach it so that it wont get cross polluted
                # as it goes through the configurations
                derived_configuration_env = platform_common_env.derive()
                derived_configuration_env.detach()
                
                env_str = '{}_{}'.format(enabled_target_platform.name(), enabled_configuration.config_name())
                ctx.setenv(env_str, derived_configuration_env)
                
                # Tag the platform environment
                ctx.env['PLATFORM'] = enabled_target_platform.name()
    
                # Apply the platform's env values
                enabled_target_platform.apply_env_rules_to_env(ctx)
                # Apply the platform's env rules based on the current configuration
                enabled_target_platform.apply_env_rules_to_env(ctx, enabled_configuration.settings.name)
                # If the current platform is a derived configuration, also apply its base configuration
                if enabled_configuration.settings.base_config:
                    enabled_target_platform.apply_env_rules_to_env(ctx, enabled_configuration.settings.base_config.name)

                # Apply the platform / configuration rules
                load_platform_configuration_settings_func(enabled_configuration)
                
                # Apply test configuration settings if this configuration is a test configuration
                if enabled_configuration.is_test:
                    load_test_settings(ctx)
                    
                # Apply dedicated server configuration settings if this configuration is a server configuration
                if enabled_configuration.is_server:
                    load_dedicated_settings(ctx)
                    
                # Apply the uselib keys from the third party to the platform/config
                uselib_platform_key = enabled_target_platform.third_party_platform_key
                for cached_uselib, cached_uselib_reader in cached_uselib_readers.items():
                    try:
                        cached_uselib_reader.apply(ctx, uselib_platform_key, enabled_configuration)
                    except Exception as err:
                        ctx.warn_once('Unable to apply uselib "{}" : {}.  This may cause compiler errors'.format(cached_uselib, str(err)))
                        
                # Mark this platform as available for builds
                ctx.env['BUILD_ENABLED'] = True

                ctx.get_env().detach()
                
        except Errors.WafError as err:
            Logs.warn('[WARN] Unable to initialize platform ({}). Disabling platform.'.format(err))
            ctx.get_env().detach()
            ctx.disable_target_platform(enabled_target_platform.name())
            continue

    # Update the validated platforms so it will be carried over to project_generator commands
    ctx.update_valid_configurations_file()
    Logs.info("[WAF] 'Initialize Build Variants' successful ({})".format(str(configure_timer)))


def wrap_execute(execute_method):
    """
    Decorator used to set the commands that can be configured automatically

    :param execute_method:     The method to execute
    """
    def execute(self):

        def _get_output_binary_folders():
            """
            Attempt to determine a list of output binary folder outputs
            :return:    List of output folder names
            """
            output_folders = []
            for platform_detail in PLATFORM_MAP.values():
                for platform_config in platform_detail.get_configuration_details():
                    output_folders.append(platform_config.output_folder())
            return output_folders


        def _set_lmbrwaf_version_tag(bin_temp_node, overwrite=False):
            """
            Tag the BinTemp with the lmbr_waf version tag if one is supplied
            """
            if not LMBR_WAF_VERSION_TAG:
                return
            current_lmbrwaf_version = str(LMBR_WAF_VERSION_TAG)
            lmbrwaf_version_path = os.path.join(bin_temp_node.abspath(), 'lmbrwaf.version')
            if os.path.exists(lmbrwaf_version_path) and not overwrite:
                return
            from ConfigParser import ConfigParser
            config = ConfigParser()
            with open(lmbrwaf_version_path, 'w') as lmbrwaf_version_file:
                config.add_section('General')
                config.set('General', 'version_tag', current_lmbrwaf_version)
                config.write(lmbrwaf_version_file)

        def _clean_bintemp(bin_temp_node, reason=None):
            """
            Perform the drastic measure of wiping out BinTemp because we detected a situation that requires it (see
            comments above for _check_bintemp_clean()
            """

            bintemp_path = bin_temp.abspath()
            root_path = bin_temp_node.parent.abspath()

            if not os.path.exists(bintemp_path):
                return
            
            clean_bintemp_lock_file_path = os.path.join(bintemp_path, 'cleanbintemp.lock')
            lock_acquired = False

            try:
                current_pid = str(os.getpid())
                if os.path.isfile(clean_bintemp_lock_file_path):
                    with open(clean_bintemp_lock_file_path, 'r') as clean_bintemp_lock_file:
                        read_pid = clean_bintemp_lock_file.read()
                        if current_pid != read_pid:
                            raise Errors.WafError('[ERROR] Another WAF process is currently performing this action.  Please wait '
                                           'until that finishes and retry again.  If the problem persists, manually delete '
                                           'BinTemp from your root path.')
                else:
                    with open(clean_bintemp_lock_file_path,'w') as clean_bintemp_lock_file:
                        clean_bintemp_lock_file.write('{}'.format(os.getpid()))
                    lock_acquired = True
                    
                # Collect the folders that we want to delete when cleaning BinTemp
                clean_subfolders = [os.path.join(bintemp_path, BINTEMP_CACHE_3RD_PARTY),
                                    os.path.join(bintemp_path, BINTEMP_CACHE_TOOLS),
                                    os.path.join(bintemp_path, CACHE_DIR),
                                    os.path.join(bintemp_path, BINTEMP_MODULE_DEF)]
                # Add all the possible the output binary folders
                clean_subfolders += [os.path.join(root_path, binary_folder) for binary_folder in _get_output_binary_folders()]
                
                # Add all the possible platform+confif variant folders
                for platform_detail in PLATFORM_MAP.values():
                    for platform_config in platform_detail.get_configuration_details():
                        bintemp_subfolder = os.path.join(bintemp_path, '{}_{}'.format(platform_detail.name(), platform_config.config_name()))
                        clean_subfolders.append(bintemp_subfolder)

                # Check if we actually have to clean any folders
                has_folders_to_clean = False
                for clean_subfolder in clean_subfolders:
                    if os.path.exists(clean_subfolder):
                        has_folders_to_clean = True
                        break

                if has_folders_to_clean:
                    Logs.pprint('RED', '[INFO] The current contents of BinTemp is out of sync with the current build scripts. ')
                    if reason:
                        Logs.pprint('RED', '[INFO] {}'.format(reason))
                    Logs.pprint('RED', '[INFO] It will be cleared out to bring all of the intermediate/generated files update to date and prevent any potential')
                    Logs.pprint('RED', '[INFO] WAF-related issues caused by the build script changes.  This will force a one-time re-configure and full rebuild.')
                    Logs.pprint('RED', '[INFO] Deleting BinTemp, please wait...')

                    for subfolder in clean_subfolders:
                        if os.path.exists(subfolder):
                            try:
                                shutil.rmtree(subfolder, ignore_errors=True)
                            except Exception as err:
                                Logs.pprint('RED', '[WARN] Unable to delete output folder "{}" ({}).  Please delete this folder manually '
                                                   'to ensure no stale artifacts are left over.'.format(subfolder, str(err)))
                
                # Clean out the waf pickle files
                
            finally:
                # Clear the lock file
                if lock_acquired:
                    os.remove(clean_bintemp_lock_file_path)

        def _read_lmbrwaf_version_tag(bin_temp_node):
            """
            Attempt to read the lmbr_waf version from the version tag file if possible.  Return None if the file doesnt exist or if there is a problem reading the file
            :return: The value of the version tag from the version file if possible
            """
            lmbrwaf_version_file_path = os.path.join(bin_temp_node.abspath(), 'lmbrwaf.version')
            if not os.path.exists(lmbrwaf_version_file_path):
                return None

            try:
                from ConfigParser import ConfigParser
                parser = ConfigParser()
                parser.read(lmbrwaf_version_file_path)
                current_version_tag = str(parser.get('General', 'version_tag'))
                return current_version_tag
            except Exception as err:
                Logs.warn('[WARN] Problem reading {} file:'.format(lmbrwaf_version_file_path, str(err)))
                return None

        def _clear_waf_pickle(bin_temp_node):
            pickle_files = glob.glob(os.path.join(bin_temp_node.abspath(),'.wafpickle*'))
            for pickle_file in pickle_files:
                try:
                    os.remove(pickle_file)
                except Exception as e:
                    Logs.warn('[WARN] Unable to clear pickle file {}: {}'.format(pickle_file, str(e)))

        def _initialize_bintemp(bin_temp_node):
            """
            Perform the initialization of the BinTemp folder
            """
            has_bintemp_folder = os.path.isdir(bin_temp_node.abspath())
            
            bin_temp_node.mkdir()

            if not LMBR_WAF_VERSION_TAG:
                # If there is no lmbrwaf version tag, we can ignore the clean bintemp check or update
                return False

            current_lmbrwaf_version = _read_lmbrwaf_version_tag(bin_temp_node)
            check_lmbrwaf_version = str(LMBR_WAF_VERSION_TAG)
            
            # If there was a previous lmbr_version, perform the bin temp clean logic if it doesnt match the current lmbr_waf tag
            if current_lmbrwaf_version and current_lmbrwaf_version != check_lmbrwaf_version and has_bintemp_folder:
                # Detected a lmbr_waf version tag update, start the bintemp wipe process
                reason = "BinTemp is not WAF version tagged." if current_lmbrwaf_version is None else \
                         "Existing BinTemp WAF Version '{}' does not match current WAF Version '{}'" \
                            .format(current_lmbrwaf_version,check_lmbrwaf_version)
                _clean_bintemp(bin_temp_node, reason)
                _clear_waf_pickle(bin_temp_node)
                # Inject the updated version tag into BinTemp
                _set_lmbrwaf_version_tag(bin_temp_node, True)
                # Signal that we need a configure
                return True
            # If there was no previous lmbr_version, set it to the current lmbr_version
            if not current_lmbrwaf_version:
                _set_lmbrwaf_version_tag(bin_temp_node, True)

        # Make sure to create all needed temp folders
        bin_temp = self.get_bintemp_folder_node()
        if _initialize_bintemp(bin_temp):
            if self.cmd!='configure':
                Options.commands.insert(0, self.cmd)
                Options.commands.insert(0, 'configure')
                self.skip_finish_message = True
                return
        self.skip_finish_message = False
        tmp_files_folder = bin_temp.make_node('TempFiles')
        tmp_files_folder.mkdir()

        # Before executing any build or configure commands, check for config file
        # and for bootstrap updates
        self.load_user_settings()

        if getattr(self, "do_auto_configure", None):
            if self.do_auto_configure():
                return

        return execute_method(self)
    return execute


BuildContext.execute = wrap_execute(BuildContext.execute)
ConfigurationContext.execute = wrap_execute(ConfigurationContext.execute)


# Create Build Context Commands for multiple platforms/configurations
for platform_detail in PLATFORM_MAP.values():
    for config_detail in platform_detail.get_configuration_details():
        platform_config_key = '{}_{}'.format(platform_detail.name(), config_detail.config_name())
        # Create new class to execute clean with variant
        name = CleanContext.__name__.replace('Context','').lower()
        class tmp_clean(CleanContext):
            cmd = name + '_' + platform_config_key
            variant = platform_config_key
            target_platform = platform_detail.name()
            target_configuration = config_detail.config_name()
            is_build_cmd = True

            def __init__(self, **kw):
                super(CleanContext, self).__init__(**kw)
                self.top_dir = kw.get('top_dir', Context.top_dir)

            def execute(self):
                if Configure.autoconfig:
                    env = ConfigSet.ConfigSet()

                    do_config = False
                    try:
                        env.load(os.path.join(Context.lock_dir, Options.lockfile))
                    except Exception:
                        Logs.warn('Configuring the project')
                        do_config = True
                    else:
                        if env.run_dir != Context.run_dir:
                            do_config = True
                        else:
                            h = 0
                            for f in env['files']:
                                try:
                                    h = hash((h, Utils.readf(f, 'rb')))
                                except (IOError, EOFError):
                                    pass # ignore missing files (will cause a rerun cause of the changed hash)
                            do_config = h != env.hash

                    if do_config:
                        Options.commands.insert(0, self.cmd)
                        Options.commands.insert(0, 'configure')
                        return

                # Execute custom clear command
                self.restore()
                if not self.all_envs:
                    self.load_envs()
                self.recurse([self.run_dir])

                if self.options.targets:
                    self.target_clean()
                else:
                    try:
                        self.clean_output_targets()
                        self.clean()
                    finally:
                        self.store()


        # Create new class to execute build with variant
        name = BuildContext.__name__.replace('Context','').lower()

        class tmp_build(BuildContext):
            cmd = name + '_' + platform_config_key
            variant = platform_config_key
            target_platform = platform_detail.name()
            target_configuration = config_detail.config_name()
            is_build_cmd = True

            def compare_timestamp_file_modified(self, path):
                modified = False

                # create a src node
                src_path = path if os.path.isabs(path) else self.path.make_node(path).abspath()
                timestamp_file = os.path.splitext(os.path.split(path)[1])[0] + '.timestamp'
                timestamp_node = self.get_bintemp_folder_node().make_node(timestamp_file)
                timestamp_stat = 0
                try:
                    timestamp_stat = os.stat(timestamp_node.abspath())
                except OSError:
                    Logs.info('%s timestamp file not found.' % timestamp_node.abspath())
                    modified = True
                else:
                    try:
                        src_file_stat = os.stat(src_path)
                    except OSError:
                        Logs.warn('%s not found' % src_path)
                    else:
                        if src_file_stat.st_mtime > timestamp_stat.st_mtime:
                            modified = True

                return modified

            def add_configure_command(self):
                Options.commands.insert(0, self.cmd)
                Options.commands.insert(0, 'configure')
                self.skip_finish_message = True

            def do_auto_configure(self):
                timestamp_check_files = TIMESTAMP_CHECK_FILES
                if 'android' in platform_config_key:
                    timestamp_check_files += ANDROID_TIMESTAMP_CHECK_FILES

                for timestamp_check_file in timestamp_check_files:
                    if self.compare_timestamp_file_modified(timestamp_check_file):
                        self.add_configure_command()
                        return True
                return False

        # Create derived build class to execute host tools build for host + profile only
        host = Utils.unversioned_sys_platform()
        if platform_detail.name() in ["win_x64", "linux_x64", "darwin_x64"] and config_detail.config_name() == 'profile':
            class tmp_build_host_tools(tmp_build):
                cmd = 'build_host_tools'
                variant = '{}_{}'.format(platform_detail.name(), config_detail.config_name())
                def execute(self):
                    original_project_spec = self.options.project_spec
                    original_targets = self.targets
                    self.options.project_spec = 'host_tools'
                    self.targets = []
                    super(tmp_build_host_tools, self).execute()
                    self.options.project_spec = original_project_spec
                    self.targets = original_targets


@conf
def register_output_folder_settings_reporter(ctx, platform_name):
    """
    Report possible overrides for the 'output_folder_android_armv7_clang' option
    """
    def default_report_settings_output_folder(cur_ctx, is_configure, is_build, attribute, default_value, settings_value):

        if not is_build:
            return
        
        if not str(getattr(cur_ctx, 'cmd', '')).startswith('build_'):
            # Only report output folders from the build_<platform>_<configuration> commands
            return
        
        out_folder_attribute = 'out_folder_{}'.format(platform_name)
        if out_folder_attribute != attribute:
            return
    
        current_platform, current_configuration_name = cur_ctx.get_platform_and_configuration()
    
        if not current_platform or current_platform == 'project_generator':
            return
        if not current_configuration_name or current_configuration_name == 'project_generator':
            return

        current_configuration = settings_manager.LUMBERYARD_SETTINGS.get_root_configuration_name(current_configuration_name)
        
        # Calculate the folder extensions
        default_folder_parts_list = [default_value]
        current_folder_parts_list = [settings_value]

        folder_ext_key = 'output_folder_ext_{}'.format(current_configuration.name)
        assert folder_ext_key in settings_manager.LUMBERYARD_SETTINGS.default_settings_map, "Missing setting '{}' in default settings".format(folder_ext_key)
        if settings_manager.LUMBERYARD_SETTINGS.default_settings_map[folder_ext_key]:
            default_folder_parts_list.append(settings_manager.LUMBERYARD_SETTINGS.default_settings_map[folder_ext_key])
    
        assert folder_ext_key in settings_manager.LUMBERYARD_SETTINGS.settings_map, "Missing setting '{}' in user settings".format(folder_ext_key)
        if settings_manager.LUMBERYARD_SETTINGS.settings_map[folder_ext_key]:
            current_folder_parts_list.append(settings_manager.LUMBERYARD_SETTINGS.settings_map[folder_ext_key])
        
        # Get the platform configuration details to add any additional suffixes to the reported output
        platform_details = ctx.get_target_platform_detail(current_platform)
        platform_configuration = platform_details.get_configuration(current_configuration_name)
        if platform_configuration.is_test:
            default_folder_parts_list.append(TEST_EXTENSION[1:])
            current_folder_parts_list.append(TEST_EXTENSION[1:])
        if platform_configuration.is_server:
            default_folder_parts_list.append(DEDICATED_EXTENSION[1:])
            current_folder_parts_list.append(DEDICATED_EXTENSION[1:])

        default_output_folder = '.'.join(default_folder_parts_list)
        current_output_folder = '.'.join(current_folder_parts_list)
    
        report_msg = 'Target Output folder ({}/{}): {}'.format(current_platform, current_configuration_name, current_output_folder)
        if current_output_folder != default_output_folder:
            report_msg += ' (Default: {})'.format(default_output_folder)

        cur_ctx.print_settings_override_message(report_msg)

    register_func_name = 'report_settings_out_folder_{}'.format(platform_name)

    setattr(Options.OptionsContext, register_func_name, default_report_settings_output_folder)
    setattr(ConfigurationContext, register_func_name, default_report_settings_output_folder)
    setattr(BuildContext, register_func_name, default_report_settings_output_folder)

    REGISTERED_CONF_FUNCTIONS[register_func_name] = default_report_settings_output_folder


@conf
def update_platform_availability_from_options(ctx):
    
    enabled_capabilities = ctx.get_enabled_capabilities()
    
    # Check if  the java module was loaded (for checks against platforms that require it)
    check_java = hasattr(ctx, 'javaw_loaded')
    java_loaded = getattr(ctx, 'javaw_loaded', False)
    
    # Load the platform filter list if any as an additional filter
    is_configure_context = isinstance(ctx, ConfigurationContext)
    validated_platforms_node = ctx.get_validated_platforms_node()
    validated_platforms_json = ctx.parse_json_file(validated_platforms_node) if os.path.exists(validated_platforms_node.abspath()) else None

    for enabled_target_platform in PLATFORM_MAP.values():
        if enabled_target_platform.needs_java and not java_loaded and check_java:
            Logs.info('[INFO] Target platform {} disabled due to not java not properly initialized through Setup Assistant.'.format(enabled_target_platform.name()))
            enabled_target_platform.set_enabled(False)
            continue

        # Check if the platform is enabled in setup assistant (if the platform is filtered by a setup assistant capability of not)
        sa_capability = enabled_target_platform.attributes.get('sa_capability', None)
        if sa_capability and enabled_capabilities:
            capability_key = sa_capability['key']
            capability_desc = sa_capability['description']
            if capability_key not in enabled_capabilities:
                Logs.debug('lumberyard: Removing target platform {} due to "{}" not checked in Setup Assistant.'.format(enabled_target_platform.name(), capability_desc))
                enabled_target_platform.set_enabled(False)
                continue

        if not is_configure_context and validated_platforms_json:
            if enabled_target_platform.platform not in validated_platforms_json:
                enabled_target_platform.set_enabled(False)
                continue

