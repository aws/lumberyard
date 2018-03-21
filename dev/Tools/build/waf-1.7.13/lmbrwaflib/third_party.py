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

from waflib import Context
from waflib.TaskGen import feature, after_method
from waflib.Configure import conf, Logs
from waflib.Errors import WafError

from cry_utils import get_configuration, append_unique_kw_entry
from collections import OrderedDict
from gems import GemManager

import os
import glob
import re
import json
import string


ALIAS_SEARCH_PATTERN = re.compile(r'(\$\{([\w]*)})')

# Map platforms to their short version names in 3rd party
PLATFORM_TO_3RD_PARTY_SUBPATH = {
                                    # win_x64 host platform
                                    'win_x64_vs2013'     : 'win_x64/vc120',
                                    'win_x64_vs2015'     : 'win_x64/vc140',
                                    'android_armv7_gcc'  : 'win_x64/android_ndk_r12/android-19/armeabi-v7a/gcc-4.9',
                                    'android_armv7_clang': 'win_x64/android_ndk_r12/android-19/armeabi-v7a/clang-3.8',
                                    'android_armv8_clang': 'win_x64/android_ndk_r12/android-21/arm64-v8a/clang-3.8',

                                    # osx host platform
                                    'darwin_x64' : 'osx/darwin-clang-703.0.31',
                                    'ios'        : 'osx/ios-clang-703.0.31',
                                    'appletv'    : 'osx/appletv-clang-703.0.31',

                                    # linux host platform
                                    'linux_x64'  : 'linux/clang-3.7'}

CONFIGURATION_TO_3RD_PARTY_NAME = {'debug'  : 'debug',
                                   'profile': 'release',
                                   'release': 'release'}


def trim_lib_name(platform, lib_name):
    """
    Trim the libname if it meets certain platform criteria
    :param platform: The platform key to base the trim decision on
    :param libname: The libname to conditionally trim
    :return:
    """
    if platform.startswith('win_x64'):
        return lib_name
    if not lib_name.startswith('lib'):
        return lib_name
    return lib_name[3:]


def get_third_party_platform_name(ctx, waf_platform_key):
    """
    Get the third party platform name for a waf platform name

    :param ctx:                 The current context
    :param waf_platform_key:    The waf platform key
    :return:
    """
    if waf_platform_key not in PLATFORM_TO_3RD_PARTY_SUBPATH:
        ctx.fatal('Platform {} does not have a mapped sub path for 3rd Party Static Libraries in {}/wscript'.format(waf_platform_key, ctx.path.abspath()))
    return PLATFORM_TO_3RD_PARTY_SUBPATH[waf_platform_key]


def get_third_party_configuration_name(ctx, waf_configuration_key):
    """
    Get the 3rd party configuration name for a waf configuration name
    :param ctx:                     The current context
    :param waf_configuration_key:   The waf configuration key
    :return:
    """
    if waf_configuration_key not in CONFIGURATION_TO_3RD_PARTY_NAME:
        ctx.fatal('Configuration {} is not a valid 3rd party configuration in {}/wscript'.format(waf_configuration_key, ctx.path.abspath()))
    return CONFIGURATION_TO_3RD_PARTY_NAME[waf_configuration_key]


def evaluate_node_alias_map(lib_root, lib_name):
    """
    Given a root of a lib config, perform an evaluation of all the aliases and produce a map of its value
    :param lib_root:    The lib root
    :return:  The map of aliased values
    """

    def _process_alias_value(name_alias_map, name_alias_value, visit_stack):

        alias_match = ALIAS_SEARCH_PATTERN.search(name_alias_value)
        if alias_match is not None:

            # This contains an aliased value
            alias_key = alias_match.group(2)

            # If the value is another alias
            if alias_key not in name_alias_map:
                # If the alias is not in the map, this is an error
                raise RuntimeError('Invalid alias value {} for 3rd party library {}'.format(name_alias_value, lib_name))
            if alias_key in visit_stack:
                # If the alias is in the stack, this is a recursive error
                raise RuntimeError('Recursive alias value error {} for 3rd party library {}'.format(name_alias_value, lib_name))

            visit_stack.append(alias_key)
            aliased_values = []
            aliased_names = name_alias_map[alias_key] if isinstance(name_alias_map[alias_key], list) else [name_alias_map[alias_key]]
            for alias_value in aliased_names:
                pre_expansion_aliases = _process_alias_value(name_alias_map, alias_value, visit_stack)
                for pre_expansion_alias in pre_expansion_aliases:
                    aliased_values.append(ALIAS_SEARCH_PATTERN.sub(pre_expansion_alias,name_alias_value))
            visit_stack.pop()
            return aliased_values

        else:
            return [name_alias_value]

    # Scan the optional 'aliases' node off of the parent to look for aliases to build the map
    evaluated_name_alias_map = {}
    if 'aliases' not in lib_root.keys():
        return evaluated_name_alias_map

    alias_node = lib_root['aliases']
    # The first pass is to collect the pre-evaluated alias names
    for node_key in alias_node:
        node_alias_value = alias_node[node_key]
        if isinstance(node_alias_value, str):
            node_alias_value = [node_alias_value]
        evaluated_name_alias_map[node_key] = node_alias_value

    # Second pass, go through each list (possible recursively) and expand any aliased values in the list
    stack = []
    for aliased_node_key in evaluated_name_alias_map:
        aliased_node_values = evaluated_name_alias_map[aliased_node_key]
        stack.append(aliased_node_key)
        values = []
        for aliased_node_value in aliased_node_values:
            values += _process_alias_value(evaluated_name_alias_map, aliased_node_value, stack)
            evaluated_name_alias_map[aliased_node_key] = values
        stack.pop()

    return evaluated_name_alias_map

CACHE_JSON_MODEL_AND_ALIAS_MAP = {}

# Keep a cache of the 3rd party uselibs mappings to a tuple of the config file and the alias root map that applies to it
CONFIGURED_3RD_PARTY_USELIBS = {}

@conf
def get_3rd_party_config_record(ctx, lib_config_file):
    """
    Read a config and parse its alias map.  Also provide a caching mechanism to retrieve the config dictionary, list of uselib names, and
    alias map.

    :param ctx:                 Config context
    :param lib_config_file:     The full pall of the config file to read
    :return:    The parsed json dictionary, list of uselib names, and the map of alias values if any
    """
    if lib_config_file in CACHE_JSON_MODEL_AND_ALIAS_MAP:
        return CACHE_JSON_MODEL_AND_ALIAS_MAP[lib_config_file]

    lib_info = ctx.parse_json_file(lib_config_file)
    assert "name" in lib_info
    uselib_names = [userlib_name.upper() for userlib_name in (lib_info["name"] if isinstance(lib_info["name"], list) else [lib_info["name"]])]

    alias_map = evaluate_node_alias_map(lib_info, lib_config_file)
    CACHE_JSON_MODEL_AND_ALIAS_MAP[lib_config_file] = (lib_info, uselib_names, alias_map)
    return lib_info, uselib_names, alias_map


@conf
def read_and_mark_3rd_party_libs(ctx):

    def _process(config_folder_node, config_file_abs, path_alias_map):

        filename = os.path.basename(config_file_abs)

        # Attempt to load the 3rd party configuration
        lib_config_file = config_folder_node.make_node(filename)
        config, uselib_names, alias_map = get_3rd_party_config_record(ctx, lib_config_file)

        # Mark all the uselib names as processed
        for uselib_name in uselib_names:
            CONFIGURED_3RD_PARTY_USELIBS[uselib_name] = (lib_config_file, path_alias_map)

    config_3rdparty_folder = ctx.engine_node.make_node('_WAF_/3rdParty')
    config_3rdparty_folder_path = config_3rdparty_folder.abspath()
    config_files = glob.glob(os.path.join(config_3rdparty_folder_path, '*.json'))

    if ctx.is_engine_local():
        root_alias_map = {'ROOT' : ctx.srcnode.abspath()}
    else:
        root_alias_map = {'ROOT': ctx.engine_path}

    for config_file in config_files:
        _process(config_3rdparty_folder, config_file, root_alias_map)

    # Read the 3rd party configs with export 3rd party set to true
    all_gems = GemManager.GetInstance(ctx).gems
    for gem in all_gems:
        ctx.root.make_node(gem.abspath).make_node('3rdParty')
        gem_3p_abspath = os.path.join(gem.abspath, '3rdParty', '*.json')
        gem_3p_config_files = glob.glob(gem_3p_abspath)
        gem_3p_node = ctx.root.make_node(gem.abspath).make_node('3rdParty')
        gem_alias_map = {'ROOT' : ctx.srcnode.abspath(),
                         'GEM'  : gem.abspath }
        for gem_3p_config_file in gem_3p_config_files:
            _process(gem_3p_node, gem_3p_config_file, gem_alias_map)

    return CONFIGURED_3RD_PARTY_USELIBS

@conf
def read_3rd_party_config(ctx, config_file_path_node, platform_key, configuration_key, path_prefix_map, warn_on_collision=False):

    error_messages = set()
    all_uselib_names = set()

    filename = os.path.basename(config_file_path_node.abspath())
    lib_key = os.path.splitext(filename)[0]

    # Attempt to load the 3rd party configuration
    try:
        config, uselib_names, alias_map, = get_3rd_party_config_record(ctx, config_file_path_node)

        if config is not None:
            result, err_msg = ThirdPartyLibReader(ctx, config_file_path_node, config, lib_key, platform_key, configuration_key,
                                                  alias_map, path_prefix_map, warn_on_collision).detect_3rd_party_lib()
            if not result and err_msg is not None:
                if err_msg is not None:
                    error_messages.add(err_msg)
            elif not result:
                pass
            else:
                for uselib_name in uselib_names:
                    if uselib_name in all_uselib_names:
                        Logs.warn('[WARN] Duplicate 3rd party definition detected : {}'.format(uselib_name))
                    else:
                        all_uselib_names.add(uselib_name)
    except Exception as err:
        error_messages.add('[3rd Party] {}'.format(err.message))

    return error_messages, all_uselib_names

@conf
def detect_all_3rd_party_libs(ctx, config_path_node, platform_key, configuration_key, path_prefix_map, warn_on_collision=False):

    config_3rdparty_folder_path = config_path_node.abspath()
    config_files = glob.glob(os.path.join(config_3rdparty_folder_path, '*.json'))

    error_messages = set()
    all_uselib_names = set()

    for config_file in config_files:

        filename = os.path.basename(config_file)
        lib_config_file = config_path_node.make_node(filename)

        lib_error_messages, lib_uselib_names = read_3rd_party_config(ctx, lib_config_file, platform_key, configuration_key, path_prefix_map,warn_on_collision)
        error_messages = error_messages.union(lib_error_messages)
        all_uselib_names = all_uselib_names.union(all_uselib_names)

    return error_messages, all_uselib_names


# Regex pattern to search for path alias tags (ie @ROOT@) in paths
PATTERN_PATH_ALIAS_ = r'@(.*)@'
REGEX_PATH_ALIAS = re.compile('@(.*)@')

# The %LIBPATH macro only supports basic filenames with lowercase/uppercase letters, digits, ., and _.
PATTERN_SEARCH_LIBPATH = r'\%LIBPATH\(([a-zA-Z0-9\.\_]*)\)'
REGEX_SEARCH_LIBPATH = re.compile(PATTERN_SEARCH_LIBPATH)

CACHE_JSON_MODEL_AND_ALIAS_MAP = {}


class ThirdPartyLibReader:
    """
    Reader class that reads a 3rd party configuration dictionary for a 3rd party library read from a json file and applies the
    appropriate environment settings to consume that library
    """

    def __init__(self, ctx, config_node, lib_root, lib_key, platform_key, configuration_key, alias_map, path_alias_map, warn_on_collision):
        """
        Initializes the class

        :param ctx:                 Configuration context

        :param lib_root:            The root level of the library configuration
        :param lib_key:             The library key
        :param platform_key:        The 'target' platform name (from waf_branch_spec)
        :param configuration_key:   The build configuration key (from waf_branch_spec)
        :param alias_map:           A map of aliases that was preprocessed from the root node before this method call
        :return:                    True,None: Library detected and applied
                                    False,String: Error occurred
        """
        self.ctx = ctx
        self.lib_root = lib_root
        self.lib_key = lib_key
        self.platform_key = platform_key
        self.configuration_key = configuration_key
        self.name_alias_map = alias_map
        self.path_alias_map = path_alias_map
        self.warn_on_collision = warn_on_collision

        # Create an error reference message based on the above settings
        self.error_ref_message = "({}, platform {})".format(config_node.abspath(), self.platform_key)

        # Process the platform key (in case the key is an alias).  If the platform value is a string, then it must start with a '@' to represent
        # an alias to another platform definition
        self.processed_platform_key = self.platform_key

        if "platform" in self.lib_root:

            # There exists a 'platform' node in the 3rd party config. make sure that the input platform key exists in there
            platform_root = self.lib_root["platform"]
            if self.platform_key not in platform_root:
                # If there was no evaluated platform key, that means the configuration file was valid but did not have entries for this
                # particular platform.
                if Logs.verbose > 1:
                    Logs.warn('[WARN] Platform {} not specified in 3rc party configuration file {}'.format(self.platform_key,config_node.abspath()))
                self.processed_platform_key = None

            # If the platform definition is a string, then it must represent an alias to a defined platform
            elif isinstance(platform_root[self.platform_key], str):

                # Validate the platform alias
                platform_alias_target = platform_root[self.platform_key]
                if not platform_alias_target.startswith('@'):
                    self.raise_config_error("Invalid platform alias format {}.  Must be in the form @<platform> where <platform> "
                                            "represents an explicitly defined platform.".format(self.platform_key))
                # Validate the target platform is explicitly declared
                platform_alias_target = platform_alias_target[1:]
                if platform_alias_target not in platform_root:
                    self.raise_config_error("Invalid platform alias {}.  Target platform alias not defined in the configuration file".format(self.platform_key))

                self.processed_platform_key = platform_alias_target

                # Update the reference message based on the above processed platform key
                self.error_ref_message = "({}, platform {})".format(config_node.abspath(), self.lib_key, self.platform_key)

    def raise_config_error(self, message):
        raise RuntimeError("{} {}".format(message, self.error_ref_message))

    def get_platform_version_options(self):
        """
        Gets the possible version options for the given platform target
        e.g. 
            Android => the possible APIs the pre-builds were built against
        """
        if self.ctx.is_android_platform(self.processed_platform_key):
            possible_android_apis = self.ctx.get_android_api_lib_list()
            possible_android_apis.sort()
            return possible_android_apis

        return None

    def detect_3rd_party_lib(self):
        """
        Detect the 3rd party libs for the current platform and configuration.  This method must be called per variant
        """

        def _get_optional_boolean_flag(key_name, default):
            if key_name in self.lib_root:
                required = self.lib_root[key_name].lower() == 'true'
            else:
                required = default
            return required

        suppress_warning = _get_optional_boolean_flag("suppress_warning", False)
        non_release_only = _get_optional_boolean_flag("non_release_only", False)

        if self.processed_platform_key is None:
            return False, None

        try:
            # If the library is configured only for non-release (debug, profile, et all), but we are currently in release/performance,
            # then short circuit out and return without setting any environment
            if non_release_only and self.configuration_key.lower() in ["release", "release_dedicated", "performance", "performance_dedicated"]:
                return False, None

            # Validate that this is a valid target platform on this host platform
            validated_platforms = self.ctx.get_supported_platforms()
            if self.platform_key not in validated_platforms:
                self.raise_config_error("Invalid platform {} on this host".format(self.platform_key))

            # Locate the library key
            if "name" not in self.lib_root:
                self.raise_config_error("Missing required 'name' attribute in configuration file")
            lib_name = self.lib_root["name"]

            header_only_library = _get_optional_boolean_flag("header_only", False)
            lib_required = _get_optional_boolean_flag("lib_required", False)
            engine_configurations = _get_optional_boolean_flag("engine_configs", False)

            if engine_configurations:
                # This is the rare case where the library configuration matches the engine's configuration
                lib_configuration = self.configuration_key.split('_')[0]
            else:
                # This is the common case where the library only has debug and release, which matches the engine's debug and non-debug configurations
                is_debug = self.configuration_key.lower() in ["debug", "debug_dedicated", "debug_test", "debug_test_dedicated"]
                if is_debug:
                    lib_configuration = "debug"
                else:
                    lib_configuration = "release"

            self.apply_uselib_from_config(lib_configuration, lib_name, header_only_library, lib_required)

        except RuntimeError as err:
            if Logs.verbose > 1:
                Logs.pprint("RED", 'Failed to detected 3rd Party Library {} : {}'.format(self.lib_key,err.message))
            error_msg = None if suppress_warning else err.args[0]
            return False, error_msg
        if Logs.verbose > 1:
            Logs.pprint("GREEN", 'Detected 3rd Party Library {}'.format(self.lib_key))
        return True, None

    def apply_uselib_from_config(self, lib_configuration, input_uselib_name, header_only_library, lib_required):
        """
        Apply all the uselib related values (INCLUDES, LIB, etc) for a uselib based on the configuration

        :param lib_configuration:       debug or release
        :param input_uselib_name:       The base uselib name
        :param header_only_library:     Flag to indicate this is a header only lib (no libs)
        :param lib_required:            Flag to indicate that this is a required lib
        """

        # Pre-process the required "source" key at the root
        if "source" not in self.lib_root:
            self.raise_config_error("Missing required 'source' attribute in configuration file")
        lib_rel_source = self.lib_root["source"]
        if len(lib_rel_source) == 0:
            self.raise_config_error("Invalid empty 'source' attribute in configuration file")

        # Pre-process the path
        lib_source_path = self.apply_optional_path_alias(lib_rel_source, None)

        # Since copy_extra commands are most likely duplicated across multiple uselibs in the same library definition file, we will cache
        # the values and apply aliases as much as possible to reduce overhead in the cache file
        copy_extra_cache = {}

        # Maintain a map of static libs based on the compound key ${platform}_${configuration}
        platform_config_lib_map = {}

        # Work on either a list of uselibs or a single uselib
        uselib_names = [input_uselib_name] if isinstance(input_uselib_name,str) else input_uselib_name

        def debug_log(message):
            Logs.debug('third_party: {}'.format(message))

        debug_log('----------------')
        debug_log('Processing {} for {}_{}'.format(self.lib_key, self.platform_key, self.configuration_key))

        for uselib_name in uselib_names:

            uselib_env_name = uselib_name.upper()

            # Find the most specific include path and apply it
            include_paths = self.get_most_specific_entry("includes", uselib_name, lib_configuration)
            self.apply_uselib_values_file_path(uselib_env_name, include_paths, "INCLUDES", lib_source_path)

            # Read and apply the values for DEFINES_(uselib_name)
            defines = self.get_most_specific_entry("defines", uselib_name, lib_configuration, False)
            self.apply_uselib_values_general(uselib_env_name, defines, "DEFINES")

            if header_only_library:
                # If this is a header only library, then no need to look for libs/binaries
                return True, None

            # Determine what kind of libraries we are using
            static_lib_paths = self.get_most_specific_entry("libpath", uselib_name, lib_configuration, False)
            static_lib_filenames = self.get_most_specific_entry("lib", uselib_name, lib_configuration, False)

            import_lib_paths = self.get_most_specific_entry("importlibpath", uselib_name, lib_configuration, False)
            import_lib_filenames = self.get_most_specific_entry("import", uselib_name, lib_configuration, False)

            shared_lib_paths = self.get_most_specific_entry("sharedlibpath", uselib_name, lib_configuration, False)
            shared_lib_filenames = self.get_most_specific_entry("shared", uselib_name, lib_configuration, False)

            framework_paths = self.get_most_specific_entry("frameworkpath", uselib_name, lib_configuration, False)
            frameworks = self.get_most_specific_entry("framework", uselib_name, lib_configuration, False)

            apply_lib_types = []

            # since the uselib system will pull all variable variants specified for that lib, we need to make sure the
            # correct version of the library is specified for the current variant.
            if static_lib_paths and (shared_lib_paths or import_lib_paths):
                debug_log(' - Detected both STATIC and SHARED libs for {}'.format(uselib_name))

                if self.ctx.is_variant_monolithic(self.platform_key, self.configuration_key):
                    apply_lib_types.append('static')
                else:
                    apply_lib_types.append('shared')

            elif static_lib_paths:
                apply_lib_types.append('static')

            elif shared_lib_paths or import_lib_paths:
                apply_lib_types.append('shared')

            if framework_paths:
                apply_lib_types.append('framework')

            # apply the static libraries, if specified
            if 'static' in apply_lib_types:
                debug_log(' - Adding STATIC libs for {}'.format(uselib_name))

                defines = self.get_most_specific_entry("defines_static", uselib_name, lib_configuration, False)
                self.apply_uselib_values_general(uselib_env_name, defines, "DEFINES")

                self.apply_uselib_values_file_path(uselib_env_name, static_lib_paths, 'STLIBPATH', lib_source_path)
                self.apply_lib_based_uselib(static_lib_filenames, static_lib_paths, 'STLIB', uselib_env_name, lib_source_path, lib_required, False, lib_configuration, platform_config_lib_map)

            # apply the frameworks, if specified
            if 'framework' in apply_lib_types:
                debug_log(' - Adding FRAMEWORKS for {}'.format(uselib_name))
                self.apply_uselib_values_file_path(uselib_env_name, framework_paths, "FRAMEWORKPATH", lib_source_path)
                self.apply_framework(uselib_env_name, frameworks)

            # Apply the link flags flag if any
            link_flags = self.get_most_specific_entry("linkflags", uselib_name, lib_configuration, False)
            self.apply_uselib_values_general(uselib_env_name, link_flags, "LINKFLAGS",lib_configuration, platform_config_lib_map)

            # Apply any optional c/cpp compile flags if any
            cc_flags = self.get_most_specific_entry("ccflags", uselib_name, lib_configuration, False)
            self.apply_uselib_values_general(uselib_env_name, cc_flags, "CFLAGS", lib_configuration, platform_config_lib_map)
            self.apply_uselib_values_general(uselib_env_name, cc_flags, "CXXFLAGS", lib_configuration, platform_config_lib_map)

            # apply the shared libraries, if specified
            if 'shared' in apply_lib_types:
                debug_log(' - Adding SHARED libs for {}'.format(uselib_name))

                linkage_lib_paths = shared_lib_paths
                linkage_lib_filenames = shared_lib_filenames

                if import_lib_paths and import_lib_filenames:
                    debug_log('- Adding IMPORT libs for {}'.format(uselib_name))
                    linkage_lib_paths = import_lib_paths
                    linkage_lib_filenames = import_lib_filenames

                self.apply_uselib_values_file_path(uselib_env_name, linkage_lib_paths, "LIBPATH", lib_source_path)
                self.apply_lib_based_uselib(linkage_lib_filenames, linkage_lib_paths, "LIB", uselib_env_name, lib_source_path, lib_required, False, lib_configuration, platform_config_lib_map)

                defines = self.get_most_specific_entry("defines_shared", uselib_name, lib_configuration, False)
                self.apply_uselib_values_general(uselib_env_name, defines, "DEFINES")

                # Special case, specific to qt.  Since we are copying the dlls as part of the qt5 bootstrap, we will ignore the shared variables if we are working on the qt5 lib
                if self.lib_key == 'qt5':
                    continue

                # keep the SHAREDLIB* vars around to house the full library names for copying
                self.apply_uselib_values_file_path(uselib_env_name, shared_lib_paths, "SHAREDLIBPATH", lib_source_path)
                self.apply_uselib_values_general(uselib_env_name, shared_lib_filenames, "SHAREDLIB")

                # Read and apply the values for PDB_(uselib_name)
                pdb_filenames = self.get_most_specific_entry("pdb", uselib_name, lib_configuration, False)
                combined_paths = (static_lib_paths if static_lib_paths is not None else []) + (shared_lib_paths if shared_lib_paths is not None else []) + (import_lib_paths if import_lib_paths is not None else [])
                if len(combined_paths) > 0:
                    self.apply_lib_based_uselib(pdb_filenames, combined_paths, "PDB", uselib_env_name, lib_source_path, False, True)

            copy_extras = self.get_most_specific_entry("copy_extra", uselib_name, lib_configuration, False)
            if copy_extras is not None:
                copy_extras_key = '.'.join(copy_extras)
                if copy_extras_key in copy_extra_cache:
                    copy_extra_base = copy_extra_cache[copy_extras_key]
                    uselib_env_key = 'COPY_EXTRA_{}'.format(uselib_env_name)
                    self.ctx.env[uselib_env_key] = '@{}'.format(copy_extra_base)
                else:
                    copy_extra_cache[copy_extras_key] = uselib_env_name
                    self.apply_copy_extra_values(lib_source_path, copy_extras, uselib_env_name)

    def get_most_specific_entry(self,
                                key_base_name,
                                uselib_name,
                                lib_configuration,
                                fail_if_missing=True):
        """
        Attempt to get the most specific entry list based on platform and/or configuration for a library.

        :param key_base_name:       Base name of the entry to lookup
        :param uselib_name:         The name of the uselib this entry that is being looked up
        :param lib_configuration:   Library configuration
        :param fail_if_missing:     Raise a RuntimeError if the value is missing
        :return: The requested entry value list if not, empty string if not
        """

        entry = None
        platform_node = None

        # Platform specific nodes are optional provided as long as we can eventually determine the value
        if "platform" in self.lib_root:
            # Has list of platforms in the config
            platform_root_node = self.lib_root["platform"]

            # Has the specific platform
            if self.processed_platform_key in platform_root_node:
                platform_node = platform_root_node[self.processed_platform_key]

        uselib_name_specific_entry_key = "{}/{}".format(uselib_name, key_base_name)
        configuration_specific_entry_key = "{}_{}".format(key_base_name, lib_configuration)
        uselib_name_and_configuration_specific_entry_key = "{}/{}_{}".format(uselib_name, key_base_name, lib_configuration)

        entry_keys = [
            key_base_name,
            uselib_name_specific_entry_key,
            configuration_specific_entry_key,
            uselib_name_and_configuration_specific_entry_key,
        ]

        version_options = self.get_platform_version_options()
        if version_options:

            entry_keys_master = entry_keys[:]
            entry_keys = []

            for key in entry_keys_master:
                entry_keys.append(key)

                for version in version_options:
                    version_key = "{}/{}".format(key, version)
                    entry_keys.append(version_key)

        # reverse the key order so the most specific is found first
        entry_keys.reverse()

        # Check against the value at the root level
        for key in entry_keys:
            if key in self.lib_root:
                entry = self.lib_root[key]
                break

        # Check against the value at the platform level
        if platform_node is not None:
            for key in entry_keys:
                if key in platform_node:
                    entry = platform_node[key]
                    break

        # Fix up the entry if its not in a list
        if entry is not None and not isinstance(entry, list):
            entry = [entry]

        # Validate if the entry was found or not
        if entry is None:
            if fail_if_missing:
                self.raise_config_error("Cannot determine {} for 3rd Party library {} and platform {}".format(key_base_name, self.lib_key, self.platform_key))
            return entry
        else:
            # Apply any alias if the entry implies an alias
            vetted_list = []
            for single_entry in entry:
                alias_match = ALIAS_SEARCH_PATTERN.search(single_entry)
                if alias_match is not None:
                    # This contains an aliased value
                    alias_key = alias_match.group(2)
                    if alias_key not in self.name_alias_map:
                        raise RuntimeError("Invalid alias key {} for 3rd Party library {} and platform {}".format(alias_key, key_base_name, self.lib_key, self.platform_key))
                    aliased_names = self.name_alias_map[alias_key] if isinstance(self.name_alias_map[alias_key],list) else [self.name_alias_map[alias_key]]
                    for aliased_name in aliased_names:
                        updated_name = ALIAS_SEARCH_PATTERN.sub(aliased_name, single_entry)
                        vetted_list.append(updated_name)
                    pass
                else:
                    # This is a normal value
                    vetted_list.append(single_entry)

            return vetted_list

    def apply_general_uselib_values(self, uselib_env_name, values, uselib_var, is_file_path, path_prefix, configuration, platform_config_lib_map):
        """
        Given a list of values for a library uselib variable, apply it optionally as a file (with validation) or a value

        :param uselib_env_name:         The uselib ENV environment key header (i.e. FOO for FOO_DEFINES, FOO_LIBATH)
        :param values:                  List of values to apply
        :param uselib_var:              The uselib name (DEFINES, LIBPATH, etc)
        :param is_file_path:            Is this a filepath? (vs value).  Adds extra path validation if it is
        :param path_prefix:             If this is a file, then the path prefix is used to represent the full path prefix
        :param configuration:           The configuration for value being applied
        :param platform_config_lib_map: Optional platform configuration alias map

        """
        env = self.ctx.env
        if values is None:
            return

        for value in values:
            if is_file_path and path_prefix is not None:
                if value == '.':
                    # Single dot means current dir relative to the path prefix
                    apply_value = path_prefix
                else:
                    apply_value = self.apply_optional_path_alias(value, path_prefix)
            elif configuration is not None and platform_config_lib_map is not None and '%LIBPATH' in value:
                apply_value = self.process_value_libpath(configuration, value, platform_config_lib_map)
            else:
                apply_value = value

            if is_file_path and not os.path.exists(apply_value):
                self.raise_config_error("Invalid/missing {} value".format(uselib_var))

            uselib_env_key = '{}_{}'.format(uselib_var, uselib_env_name)
            if self.warn_on_collision and uselib_env_key in env:
                if Logs.verbose > 1:
                    Logs.warn('[WARN] 3rd party uselib collision on key {}.  '
                              'The previous value ({}) will be overridden with ({})'.format(uselib_env_key, env[uselib_env_key], apply_value))
                else:
                    Logs.warn('[WARN] 3rd party uselib collision on key {}.  The previous value will be overridden'.format(uselib_env_key))

            env.append_unique(uselib_env_key, apply_value)

    def apply_uselib_values_file_path(self, uselib_env_name, values, uselib_var, path_prefix=None, configuration=None, platform_config_lib_map=None):
        """
        Given a list of filepath related values for a library uselib variable, apply it optionally as a file (with validation) or a value

        :param uselib_env_name:         The uselib ENV environment key header (i.e. FOO for FOO_DEFINES, FOO_LIBATH)
        :param values:                  List of values to apply
        :param uselib_var:              The uselib name (LIBPATH, etc)
        :param path_prefix:             If this is a file, then the path prefix is used to represent the full path prefix
        :param configuration:           The configuration for value being applied to
        :param platform_config_lib_map: Optional platform configuration alias map

        """
        self.apply_general_uselib_values(uselib_env_name, values, uselib_var, True, path_prefix, configuration, platform_config_lib_map)

    def apply_uselib_values_general(self, uselib_env_name, values, uselib_var, configuration=None, platform_config_lib_map=None):
        """
        Given a list of values for a library uselib variable, apply it optionally as a value

        :param uselib_env_name:         The uselib ENV environment key header (i.e. FOO for FOO_DEFINES, FOO_LIBATH)
        :param values:                  List of values to apply
        :param uselib_var:              The uselib name (DEFINES, LIB, etc)
        :param configuration:           The configuration for value being applied to
        :param platform_config_lib_map: Optional platform configuration alias map
        """
        self.apply_general_uselib_values(uselib_env_name, values, uselib_var, False, None, configuration, platform_config_lib_map)

    def apply_lib_based_uselib(self, lib_filenames, lib_paths, uselib_var, uselib_env_name, lib_source_path, is_required, fullpath, configuration=None, platform_config_lib_map=None):
        """
        Given a list of lib filenames, validate and apply the uselib value LIB_ against the library and platform

        :param lib_filenames:           The list of file names from the library configuration
        :param lib_paths:               The list of library paths from the library configuration
        :param uselib_var:              The uselib variable to apply to
        :param uselib_env_name:         The uselib environment to apply the uselib variable to
        :param lib_source_path:         The root path to the library in case any of the lib paths do not use any aliases
        :param is_required:             Flag to validate if the key is required or not
        :param fullpath:                Is the path a full path (vs relative)
        :param configuration:           Configuration to use to build the valid library full path dictionary (platform_config_lib_map)
        :param platform_config_lib_map: Map to collect the library full paths based on a compound key of ${}_${}
        :return:
        """
        if lib_filenames is None:
            return

        for lib_filename in lib_filenames:
            uselib_key = "{}_{}".format(uselib_var, uselib_env_name)

            # Validate that the file exists
            lib_found_fullpath = None
            for lib_path in lib_paths:
                lib_file_path = os.path.normpath(os.path.join(self.apply_optional_path_alias(lib_path, lib_source_path), lib_filename))
                if os.path.exists(lib_file_path):
                    lib_found_fullpath = lib_file_path
                    break

            if lib_found_fullpath is None:
                if is_required:
                    self.raise_config_error("Invalid/missing library file '{}'".format(lib_filename))
                else:
                    continue

            if configuration is not None and platform_config_lib_map is not None:
                library_plat_conf_key = '{}_{}'.format(self.processed_platform_key, configuration)
                if library_plat_conf_key not in platform_config_lib_map:
                    platform_config_lib_map[library_plat_conf_key] = [lib_found_fullpath]
                else:
                    platform_config_lib_map[library_plat_conf_key].append(lib_found_fullpath)

            # the windows linker is different in the sense it only takes in .libs regardless of the library type
            if uselib_var == 'LIB':
                if self.platform_key.startswith('win_x64'):
                    import_lib_path = "{}.lib".format(os.path.splitext(lib_file_path)[0])
                    if not os.path.exists(import_lib_path):
                        continue

            if fullpath:
                self.ctx.env.append_unique(uselib_key, lib_found_fullpath)
            else:
                lib_parts = os.path.splitext(lib_filename)
                lib_name = trim_lib_name(self.processed_platform_key, lib_parts[0])
                self.ctx.env.append_unique(uselib_key, lib_name)

    def apply_framework(self, uselib_env_name, frameworks):
        """
        Apply a framework value to the env (darwin)

        :param uselib_env_name: The uselib env name
        :param frameworks:      The framework list
        """
        if frameworks is None:
            return

        framework_key = 'FRAMEWORK_{}'.format(uselib_env_name)
        for framework in frameworks:
            self.ctx.env.append_unique(framework_key, framework)

    def apply_copy_extra_values(self, src_prefix, values, uselib_env_name):
        """
        Apply a custom copy_extra value to the env

        :param src_prefix:      Prefix (path) to the source file(s) to copy
        :param values:          The list of filenames to copy
        :param uselib_env_name: The uselib env name
        :return:
        """
        uselib_env_key = 'COPY_EXTRA_{}'.format(uselib_env_name)
        for value in values:
            self.ctx.env.append_unique(uselib_env_key, os.path.normpath("{}/{}".format(src_prefix, value)))

    def apply_optional_path_alias(self, path, path_prefix):
        """
        Given a path, a default prefix, and an alias dictionary, apply the rules to a path based on
        whether the path has an alias that is defined in an alias dictionary, or apply the default
        logic of prepending the path with the path_prefix to get an absolute path

        :param path:                The relative path as defined in the config file
        :param path_prefix:         The path prefix to apply by default
        :return: Processed path depending on whether there exists an alias or not
        """
        alias_match_groups = REGEX_PATH_ALIAS.search(path)
        if alias_match_groups is None:
            processed_path = os.path.normpath(os.path.join(path_prefix, path))
        else:
            alias_key = alias_match_groups.group(1)
            if alias_key not in self.path_alias_map:
                if path_prefix is None:
                    processed_path = os.path.normpath(path)
                else:
                    # if the alias key extracted does not match, revert to the basic processed_path
                    processed_path = os.path.normpath(os.path.join(path_prefix, path))
            else:
                replace_value = self.path_alias_map[alias_key]
                processed_path = os.path.normpath(path.replace('@{}@'.format(alias_key), replace_value))

        return processed_path

    def process_value_libpath(self, configuration, value, platform_config_lib_map):
        """
        Handle special processing for values for the value keywword %LIBPATH.  %LIBPATH will be expanded
        to the defined lib path values detected in the current configuration

        :param configuration:           The configuration being applied
        :param value:                   The value (must contain the %LIBATH expression)
        :param platform_config_lib_map: the library platform map to use to lookup the values
        :return: The applied value to %LIBPATH
        """

        # If we found LIBPATH in the value, make sure it follows %LIBPATH(<filename>)
        found_libpath = REGEX_SEARCH_LIBPATH.search(value)
        if found_libpath is None:
            self.raise_config_error("Invalid macro value for macro %LIBPATH value detected ()".format(value))

        # Extract the library name
        libname_macro_parameter = found_libpath.group(1)

        # Lookup the collected full library paths that was calculated from collecting the library paths earlier
        libpath_key = '{}_{}'.format(self.processed_platform_key, configuration)
        if libpath_key not in platform_config_lib_map:
            self.raise_config_error("%LIBPATH macro was specified for a platform/configuration with no libraries defined")

        # There may be more than one library path, so search for the particular one
        platform_config_libs = platform_config_lib_map[libpath_key]
        evaluated_lib_fullpath = None
        for platform_config_lib in platform_config_libs:
            if platform_config_lib.endswith(libname_macro_parameter):
                evaluated_lib_fullpath = platform_config_lib
        if evaluated_lib_fullpath is None:
            self.raise_config_error("Invalid/missing library for $LIBPATH macro value {}".format(value))

        # If there is any spaces in the full lib path, surround with double quotes
        if ' ' in evaluated_lib_fullpath:
            evaluated_lib_fullpath = '{}'.format(evaluated_lib_fullpath)

        # Perform the replacement of the macro to the actual value
        apply_value = re.sub(PATTERN_SEARCH_LIBPATH, evaluated_lib_fullpath, value)

        return apply_value


ALIAS_SEARCH_PATTERN = re.compile(r'(\$\{([\w]*)})')


def evaluate_node_alias_map(lib_root, lib_name):
    """
    Given a root of a lib config, perform an evaluation of all the aliases and produce a map of its value
    :param lib_root:    The lib root
    :return:  The map of aliased values
    """

    def _process_alias_value(name_alias_map, name_alias_value, visit_stack):

        alias_match = ALIAS_SEARCH_PATTERN.search(name_alias_value)
        if alias_match is not None:

            # This contains an aliased value
            alias_key = alias_match.group(2)

            # If the value is another alias
            if alias_key not in name_alias_map:
                # If the alias is not in the map, this is an error
                raise RuntimeError('Invalid alias value {} for 3rd party library {}'.format(name_alias_value, lib_name))
            if alias_key in visit_stack:
                # If the alias is in the stack, this is a recursive error
                raise RuntimeError('Recursive alias value error {} for 3rd party library {}'.format(name_alias_value, lib_name))

            visit_stack.append(alias_key)
            aliased_values = []
            aliased_names = name_alias_map[alias_key] if isinstance(name_alias_map[alias_key], list) else [name_alias_map[alias_key]]
            for alias_value in aliased_names:
                pre_expansion_aliases = _process_alias_value(name_alias_map, alias_value, visit_stack)
                for pre_expansion_alias in pre_expansion_aliases:
                    aliased_values.append(ALIAS_SEARCH_PATTERN.sub(pre_expansion_alias,name_alias_value))
            visit_stack.pop()
            return aliased_values

        else:
            return [name_alias_value]

    # Scan the optional 'aliases' node off of the parent to look for aliases to build the map
    evaluated_name_alias_map = {}
    if 'aliases' not in lib_root.keys():
        return evaluated_name_alias_map

    alias_node = lib_root['aliases']
    # The first pass is to collect the pre-evaluated alias names
    for node_key in alias_node:
        node_alias_value = alias_node[node_key]
        if isinstance(node_alias_value, str):
            node_alias_value = [node_alias_value]
        evaluated_name_alias_map[node_key] = node_alias_value

    # Second pass, go through each list (possible recursively) and expand any aliased values in the list
    stack = []
    for aliased_node_key in evaluated_name_alias_map:
        aliased_node_values = evaluated_name_alias_map[aliased_node_key]
        stack.append(aliased_node_key)
        values = []
        for aliased_node_value in aliased_node_values:
            values += _process_alias_value(evaluated_name_alias_map, aliased_node_value, stack)
            evaluated_name_alias_map[aliased_node_key] = values
        stack.pop()

    return evaluated_name_alias_map


def get_3rd_party_config_record(ctx, lib_config_file):
    """
    Read a config and parse its alias map.  Also provide a caching mechanism to retrieve the config dictionary, list of uselib names, and
    alias map.

    :param ctx:                 Config context
    :param lib_config_file:     The full pall of the config file to read
    :return:    The parsed json dictionary, list of uselib names, and the map of alias values if any
    """
    if lib_config_file in CACHE_JSON_MODEL_AND_ALIAS_MAP:
        return CACHE_JSON_MODEL_AND_ALIAS_MAP[lib_config_file]

    lib_info = ctx.parse_json_file(lib_config_file)
    assert "name" in lib_info
    uselib_names = [userlib_name.upper() for userlib_name in (lib_info["name"] if isinstance(lib_info["name"], list) else [lib_info["name"]])]

    alias_map = evaluate_node_alias_map(lib_info, lib_config_file)
    CACHE_JSON_MODEL_AND_ALIAS_MAP[lib_config_file] = (lib_info, uselib_names, alias_map)
    return lib_info, uselib_names, alias_map

# Maintain a list of all the uselib prefixes that we apply to the env per platform/config
ALL_3RD_PARTY_USELIB_PREFIXES = (
    'INCLUDES',
    'DEFINES',
    'LIBPATH',
    'FRAMEWORKPATH',
    'LINKFLAGS',
    'SHAREDLIBPATH',
    'SHAREDLIB',
    'PDB',
    'COPY_EXTRA'
)

REGISTERED_3RD_PARTY_USELIB = set()

@conf
def register_3rd_party_uselib(ctx, use_name, target_platform, *k, **kw):
    """
    Register a 3rd party uselib manually
    :param ctx              configuration context
    :param use_name         The uselib name (ie library name) to register
    :param target_platform  The target platform for special library name handling.
    :param k,kw:            keyword inputs
    """

    def _apply_to_list(uselib_prefix, value_key):
        """
        Apply a general value to a uselib key
        :param uselib_prefix:   uselib prefix (category, ie DEFINES_, INCLUDES_, etc)
        :param value_key:       The value from the keyword input to extract the value
        """
        if value_key not in kw:
            return
        value = kw[value_key]
        if isinstance(value, list):
            ctx.env['{}_{}'.format(uselib_prefix, use_name)] = value
        else:
            ctx.env['{}_{}'.format(uselib_prefix, use_name)] = [value]

    def _apply_lib_to_list(lib_uselib_prefix, lib_key, libpath_uselib_prefix, libpath_key, process_libname):
        """
        Apply the uselib to a library and its library path.  The library value must be in the form of its full filename
        :param lib_uselib_prefix:       The uselib prefix for the library declaration
        :param lib_key:                 The kw keyname to get the library name(s)
        :param libpath_uselib_prefix:   The uselib prefix for the library path(s)
        :param libpath_key:             The kw keyname to get the library path(s)
        """
        if lib_key not in kw or libpath_key not in kw:
            return

        # Convert the values to a list always
        lib_value = kw[lib_key]
        libs_to_process = lib_value if isinstance(lib_value, list) else [lib_value]

        libpath_value = kw[libpath_key]
        libpaths_to_process = libpath_value if isinstance(libpath_value, list) else [libpath_value]

        # Check each lib's path validity and adjust the lib name if necessary (the lib values are the actual filenames)
        processed_libs = []
        for lib_to_process in libs_to_process:
            path_valid = False
            for libpath in libpaths_to_process:
                if os.path.exists(os.path.join(libpath, lib_to_process)):
                    path_valid = True
                    break
            if not path_valid:
                ctx.warn_once('Invalid library path for uselib {}.  This may cause linker errors.'.format(use_name))
                continue
            processed_lib = os.path.splitext(trim_lib_name(target_platform, lib_to_process))[0] if process_libname else lib_to_process
            processed_libs.append(processed_lib)

        ctx.env['{}_{}'.format(lib_uselib_prefix, use_name)] = processed_libs
        ctx.env['{}_{}'.format(libpath_uselib_prefix, use_name)] = libpaths_to_process


    if 'lib' in kw and 'sharedlib' in kw:
        raise WafError("Cannot register both a static lib and a regular lib for the same library ({})".format(use_name))
    if 'sharedlib' in kw and 'importlib' not in kw:
        raise WafError("Cannot register a shared library without declaring its import library({})".format(use_name))
    if 'importlib' in kw and 'sharedlib' not in kw:
        raise WafError("Cannot register an import library without declaring its shared library({})".format(use_name))

    # Apply the library specific values
    if 'lib' in kw:
        _apply_lib_to_list('STLIB', 'lib', 'STLIBPATH', 'libpath', True)

    elif 'sharedlib' in kw:
        _apply_lib_to_list('LIB', 'importlib', 'LIBPATH', 'sharedlibpath', True)
        _apply_lib_to_list('SHAREDLIB', 'sharedlib', 'SHAREDLIBPATH', 'sharedlibpath', False)

    # Apply the general uselib keys if provided
    _apply_to_list('INCLUDES', 'includes')
    _apply_to_list('DEFINES', 'defines')
    _apply_to_list('FRAMEWORKPATH', 'frameworkpath')
    _apply_to_list('LINKFLAGS', 'linkflags')
    _apply_to_list('PDB', 'pdb')
    _apply_to_list('COPY_EXTRA', 'copy_extra')

    REGISTERED_3RD_PARTY_USELIB.add(use_name)

def is_third_party_uselib_configured(ctx, use_name):
    """
    Determine if a name is a configured uselib that was detected during the 3rd party framework initialization

    :param ctx:         Context that contains both the env and the all_envs attributes
    :param use_name:    The name to validate
    :return:            True if its a valid uselib, False if not
    """

    # First check against the evaluated 3rd party set
    if use_name in CONFIGURED_3RD_PARTY_USELIBS:
        return True
    if use_name in REGISTERED_3RD_PARTY_USELIB:
        return True

    # Check against the global env
    global_env_key = ''  # Within all_envs, the key for the global env is a blank string
    third_party_uselibs = getattr(ctx.all_envs[global_env_key], 'THIRD_PARTY_USELIBS', [])
    if use_name in third_party_uselibs:
        return True

    # Second check the current env against all the possible combinations
    for prefix in ALL_3RD_PARTY_USELIB_PREFIXES:
        prefix_key = '{}_{}'.format(prefix,use_name)
        if prefix_key in ctx.env:
            return True

    return False

# Attempt to retrieve configuration_settings for the uselib from either our cached tables in memory during
# configuration or our context environment.
@conf
def get_configuration_settings(ctx, use_name):
    configdict = {}

# During the configuration step the uselib .json file is parsed and cached in memory
    lib_config_file, _ = CONFIGURED_3RD_PARTY_USELIBS.get(use_name, (None, None))
    if lib_config_file:
        config, uselib_names, alias_map = get_3rd_party_config_record(ctx, lib_config_file)
        configdict = config.get('configuration_settings', {})
# During the build step check for configuration_settings which were saved to the context environment
    else:
        third_party_uselib_additional_settings = getattr(ctx.all_envs[''], 'THIRD_PARTY_USELIB_SETTINGS', {})
        configdict = third_party_uselib_additional_settings.get(use_name, {})
    return configdict

# A uselib can declare a 'configuration_settings' section which will be added to the kw dictionary
# for modules consuming the uselib
@conf
def append_dependency_configuration_settings(ctx, use_name, kw):
    configdict = ctx.get_configuration_settings(use_name)

    for _key, _val in configdict.iteritems():
        append_unique_kw_entry(kw, _key, _val)


# Weight table for our custom ordering of the json config file
THIRD_PARTY_CONFIG_KEY_WEIGHT_TABLE = {
    'name': 0,
    'source': 1,
    'description': 2,
    'aliases': 3,

    'includes': 10,
    'includes_debug': 11,
    'includes_profile': 12,
    'includes_performance': 13,
    'includes_release': 14,
    'defines': 20,
    'defines_debug': 21,
    'defines_profile': 22,
    'defines_performance': 23,
    'defines_release': 24,

    'lib_required': 30,
    'shared_required': 31,
    'platform': 32,

    'libpath': 50,
    'libpath_debug': 51,
    'libpath_release': 52,
    'lib': 53,
    'lib_debug': 54,
    'lib_release': 55,

    # Will start the key weight at 100 for the platforms just in case
    'win_x64': 100,
    'win_x64_vs2013': 101,
    'win_x64_vs2015': 102,
    'darwin_x64': 105,
    'ios': 106,
    'appletv': 107,
    'android_armv7_gcc': 108,
    'android_armv7_clang': 109,
    'android_armv8_clang': 110,
    'linux_x64': 111,
}


def _get_config_key_weight(key):
    '''
    Get the weight of a 3rd party config key so we can custom order it
    :param key: The key to evaluate
    :return:  The key weight
    '''
    if key in THIRD_PARTY_CONFIG_KEY_WEIGHT_TABLE :
        return THIRD_PARTY_CONFIG_KEY_WEIGHT_TABLE[key]
    else:
        return 99999


@feature('generate_3p_static_lib_config')
@after_method('set_pdb_flags')
def generate_3p_config(ctx):

    def _compare_list(left,right):
        if not isinstance(left, list):
            return False
        if not isinstance(right, list):
            return False
        left_str = string.join(left,':')
        right_str = string.join(right,':')
        return left_str == right_str

    platform = ctx.env['PLATFORM']
    # Short circuit out if its not a true build platform
    if platform=='project_generator':
        return

    name = getattr(ctx,'target',None)
    uselib_name = getattr(ctx, 'output_file_name', name)
    description = getattr(ctx, 'description', uselib_name)
    base_path = getattr(ctx,'base_path', None)
    config_includes = getattr(ctx,'config_includes', ['.'])
    config_defines = getattr(ctx,'config_defines', [])

    if name is None or uselib_name is None or base_path is None:
        return

    config_filename = '{}.json'.format(uselib_name)
    config_3rdparty_folder = ctx.bld.root.make_node(Context.launch_dir).make_node('_WAF_/3rdParty')
    config_file_node = config_3rdparty_folder.make_node(config_filename)
    config_file_abspath = config_file_node.abspath()

    # Determine the platform, config_platform, and config_configuration
    platform = ctx.env['PLATFORM']
    if platform not in PLATFORM_TO_3RD_PARTY_SUBPATH:
        return

    config_platform = PLATFORM_TO_3RD_PARTY_SUBPATH[platform]
    configuration = get_configuration(ctx, name)
    if configuration not in CONFIGURATION_TO_3RD_PARTY_NAME:
        return
    config_configuration = CONFIGURATION_TO_3RD_PARTY_NAME[configuration]

    output_filename = ctx.env['cxxstlib_PATTERN'] % uselib_name
    lib_path = 'build/{}/{}'.format(config_platform, config_configuration)

    if os.path.exists(config_file_abspath):

        # The file exist, lookup the platform

        # open the config file contents
        target_config_file_node = config_3rdparty_folder.make_node('{}.json'.format(uselib_name))
        config = ctx.bld.parse_json_file(target_config_file_node)

        # Track if we need to update the file
        need_update = False

        if 'description' not in config:
            config['description'] = description
            need_update = True
        elif config['description'] != description:
            config['description'] = description
            need_update = True

        if 'includes' not in config:
            config['includes'] = config_includes
            need_update = True
        elif not _compare_list(config['includes'],config_includes):
            config['includes'] = config_includes
            need_update = True

        if 'defines' not in config:
            config['defines'] = config_defines
            need_update = True
        elif not _compare_list(config['defines'], config_defines):
            config['defines'] = config_defines
            need_update = True

        if 'platform' not in config:
            config['platform'] = {}
            need_update = True
        platforms = config['platform']

        if platform not in platforms:
            # Create a new platform entry for the current platform without a configuration specifier
            cur_platform = {
                                "libpath": [lib_path],
                                "lib": [output_filename]
                           }
            platforms[platform] = cur_platform
            need_update = True
        else:
            cur_platform = platforms[platform]
            # Platform exists.  Depending on whats in the platform, do different actions

            config_specific_update_libpath_key = 'libpath_{}'.format(config_configuration)

            if 'libpath_debug' in cur_platform and 'libpath_release' in cur_platform:

                # Both debug and release configurations exist, we will update one of them if needed
                if cur_platform[config_specific_update_libpath_key][0] != lib_path:
                    cur_platform[config_specific_update_libpath_key] = [lib_path]
                    need_update = True

            elif 'libpath' in cur_platform:
                # Only one libpath exists, it may be either configuration.  Check if we need to update the existing or
                # split into 2
                if cur_platform['libpath'][0]!=lib_path:
                    # The libpath doesnt match, 'libpath' needs to be split
                    replace_libpath_key = 'libpath_release' if config_configuration == 'debug' else 'libpath_debug'
                    new_libpath_key = 'libpath_debug' if config_configuration == 'debug' else 'libpath_release'
                    cur_platform[replace_libpath_key] = cur_platform['libpath']
                    cur_platform.pop('libpath',None)
                    cur_platform[new_libpath_key] = [lib_path]
                    need_update = True
            elif 'libpath_debug' in cur_platform or 'libpath_release' in cur_platform:

                # There is a config specific key already, but only one.
                if config_specific_update_libpath_key in cur_platform:

                    # This is possibly updating the existing key
                    if cur_platform[config_specific_update_libpath_key][0] != lib_path:
                        cur_platform[config_specific_update_libpath_key] = [lib_path]
                        need_update = True
                    else:
                        # This will create a new key
                        cur_platform[config_specific_update_libpath_key] = [lib_path]
                        need_update = True
            else:
                # If all else fails, that means there are no recognized libpath values.  Create a default single one
                cur_platform[config_specific_update_libpath_key] = [lib_path]

        # Make sure the 'lib' value exists
        if 'lib' not in cur_platform:
            cur_platform['lib'] = [output_filename]
            need_update = True

        # Check if we need to update the lib value
        if cur_platform['lib'][0] != output_filename:
            cur_platform['lib'] = [output_filename]
            need_update = True

    else:
        config = {
                    'name': uselib_name,
                    'source': '@ROOT@/{}'.format(base_path),
                    'description': description,
                    'includes': config_includes,
                    'defines': config_defines,
                    'lib_required': "True",
                    'shared_required': "False",
                    'platform': {
                        platform : {
                            'libpath': [lib_path],
                            'lib': [output_filename]
                        }
                    }
                }
        need_update = True

    if need_update:
        # Level 1 : Order the root
        ordered_config = OrderedDict(sorted(config.items(), key=lambda t: _get_config_key_weight(t[0])))

        # Level 2 : Order the platforms
        if 'platform' in ordered_config:
            config_platforms = ordered_config['platform']
            ordered_config['platform'] = OrderedDict(sorted(config_platforms.items(), key=lambda t: _get_config_key_weight(t[0])))
            for config_platform_name in ordered_config['platform']:
                config_platform_def = ordered_config['platform'][config_platform_name]
                ordered_config['platform'][config_platform_name] = OrderedDict(sorted(config_platform_def.items(), key=lambda t: _get_config_key_weight(t[0])))

        config_content = json.dumps(ordered_config, indent=4, separators=(',', ': '))
        try:
            config_file_node.write(config_content)
        except Exception as err:
            Logs.warn('[WARNING] Unable to update 3rd Party Configuration File {}:{}'.format(config_filename, err.message))

