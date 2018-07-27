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

from waflib import Context, Utils, Errors
from waflib.TaskGen import feature, after_method
from waflib.Configure import conf, Logs
from waflib.Errors import ConfigurationError, WafError

from cry_utils import get_configuration, append_unique_kw_entry
from third_party_sync import P4SyncThirdPartySettings
from utils import parse_json_file, write_json_file, calculate_file_hash, calculate_string_hash
from collections import OrderedDict
from gems import GemManager

import os
import glob
import re
import json
import string
import hashlib
import ConfigParser

ALIAS_SEARCH_PATTERN = re.compile(r'(\$\{([\w]*)})')

# Map platforms to their short version names in 3rd party
PLATFORM_TO_3RD_PARTY_SUBPATH = {
                                    # win_x64 host platform
                                    'win_x64_vs2013'     : 'win_x64/vc120',
                                    'win_x64_vs2015'     : 'win_x64/vc140',
                                    'win_x64_clang'      : 'win_x64/vc140',
                                    'win_x64_vs2017'     : 'win_x64/vc140',  # Not an error, VS2017 links with VS2015 binaries
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

    all_uselib_names = set()
    filename = os.path.basename(config_file_path_node.abspath())
    lib_key = os.path.splitext(filename)[0]

    # Attempt to load the 3rd party configuration
    try:
        config, uselib_names, alias_map, = get_3rd_party_config_record(ctx, config_file_path_node)
        for uselib_name in uselib_names:
            if uselib_name in all_uselib_names:
                Logs.warn('[WARN] Duplicate 3rd party definition detected : {}'.format(uselib_name))
            else:
                all_uselib_names.add(uselib_name)

        if config is not None:
            result, err_msg = ThirdPartyLibReader(ctx, config_file_path_node, config, lib_key, platform_key, configuration_key,
                                                  alias_map, path_prefix_map, warn_on_collision).detect_3rd_party_lib()
            if not result and err_msg:
                raise Errors.WafError(err_msg)

        return True, all_uselib_names
    except Exception:
        return False, all_uselib_names


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
        self.config_file = config_node.abspath()

        # Create an error reference message based on the above settings
        self.error_ref_message = "({}, platform {})".format(config_node.abspath(), self.platform_key)

        # Process the platform key (in case the key is an alias).  If the platform value is a string, then it must start with a '@' to represent
        # an alias to another platform definition
        self.processed_platform_key = self.platform_key

        if "platform" in self.lib_root:

            # There exists a 'platform' node in the 3rd party config. make sure that the input platform key exists in there
            platform_root = self.lib_root["platform"]
            if self.platform_key not in platform_root:

                if self.platform_key == 'win_x64_clang' and 'win_x64_vs2015' in platform_root:
                    vs2015_config = platform_root['win_x64_vs2015']

                    # If vs2015 is also an alias, just copy it
                    if isinstance(vs2015_config, str):
                        platform_root[self.platform_key] = vs2015_config
                    # If vs2015 is a full config, link to it
                    else:
                        platform_root[self.platform_key] = '@win_x64_vs2015'
                else:
                    # If there was no evaluated platform key, that means the configuration file was valid but did not have entries for this
                    # particular platform.
                    if Logs.verbose > 1:
                        Logs.warn('[WARN] Platform {} not specified in 3rd party configuration file {}'.format(self.platform_key,config_node.abspath()))
                    self.processed_platform_key = None

            # If the platform definition is a string, then it must represent an alias to a defined platform
            if isinstance(platform_root.get(self.platform_key), str):

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

            if is_file_path and not self.ctx.cached_does_path_exist(apply_value):
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
                if self.ctx.cached_does_path_exist(lib_file_path):
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
                    if not self.ctx.cached_does_path_exist(import_lib_path):
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
            # String format for a copy_extra value is 'source[:destination]'.
            # Obtain source and destination normalized
            copy_extra_parts = value.split(':')
            if len(copy_extra_parts) == 2:
                source = os.path.normpath("{}/{}".format(src_prefix, copy_extra_parts[0]))
                destination = os.path.normpath(copy_extra_parts[1])
            elif len(copy_extra_parts) == 1:
                source = os.path.normpath("{}/{}".format(src_prefix, copy_extra_parts[0]))
                destination = os.path.normpath("./")
            else:
                Logs.warn("[WARN] Invalid copy_extra '{}'".format(value))
                return False

            value_norm = '{}:{}'.format(source, destination)
            self.ctx.env.append_unique(uselib_env_key, value_norm)

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

            if alias_key.startswith("3P:"):
                # The alias key refers to a 3rd party identifier
                third_party_identifier = alias_key.replace("3P:","").strip()
                resolved_path, enabled, roles, optional = self.ctx.tp.get_third_party_path(self.platform_key, third_party_identifier)

                # If the path was not resolved, it could be an invalid alias (missing from the SetupAssistantConfig.json)
                if not resolved_path:
                    raise Errors.WafError("[ERROR] Invalid 3rd party alias {}".format(alias_key))

                # If the path was resolved, we still need to make sure the 3rd party is enabled based on the roles
                if not enabled and not optional:
                    error_message = "3rd Party alias '{}' specified in {} is not enabled. Make sure that at least one of the " \
                                    "following roles is enabled: [{}]".format(alias_key, self.config_file,', '.join(roles))
                    raise Errors.WafError(error_message)

                processed_path = os.path.normpath(path.replace('@{}@'.format(alias_key), resolved_path))

            elif alias_key not in self.path_alias_map:
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
                if ctx.cached_does_path_exist(os.path.join(libpath, lib_to_process)):
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
    'win_x64_vs2017': 103,
    'win_x64_clang': 104,
    'darwin_x64': 120,
    'ios': 121,
    'appletv': 122,
    'android_armv7_gcc': 130,
    'android_armv7_clang': 131,
    'android_armv8_clang': 132,
    'linux_x64': 140
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

    if ctx.bld.cached_does_path_exist(config_file_abspath):

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
        # try to find the third party name from the dictionary based on the path
        third_party_name = ''
        try:
            third_party_root = ctx.bld.tp.content.get("3rdPartyRoot")
            base_source = os.path.relpath(base_path, third_party_root)
            content_sdks = ctx.bld.tp.content.get("SDKs")
            for key, value in content_sdks.iteritems():
                if os.path.normpath(value['base_source']) == base_source:
                    third_party_name = key
                    break
        except:
            pass    # fallback to ROOT pathing

        if third_party_name:
            third_party_source = '@3P:{}@'.format(third_party_name)
        else:
            third_party_source = '@ROOT@/{}'.format(base_path)

        config = {
                    'name': uselib_name,
                    'source': third_party_source,
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


# The current version of the working 3rd party settings file in bin temp.
CURRENT_3RD_PARTY_SETTINGS_VERSION = 1

# Mark any SDKs that will be exempt from any automatic updates
EXEMPT_SDKS = ('FFmpeg')

SETUP_ASSISTANT_PREF_SECTION = "General"
SETUP_ASSISTANT_ENGINE_PATH_VALID = "SDKEnginePathValid"
SETUP_ASSISTANT_ENGINE_PATH_3RD_PARTY = "SDKSearchPath3rdParty"
CONFIG_FILE_SETUP_ASSISTANT_CONFIG = "SetupAssistantConfig.json"
CONFIG_SETUP_ASSISTANT_USER_PREF = "SetupAssistantUserPreferences.ini"


class ThirdPartySettings:
    """
    This class manages the working 3rd party file in BinTemp that tracks all of the absolute paths to the third
    party libraries, based on the settings in SetupAssistantConfig.json, and the root 3rd party path from
    SetupAssistantUserPreferences.ini if it exists.
    """

    def __init__(self, ctx):
        self.ctx = ctx
        self.file_path_abs = os.path.join(ctx.path.abspath(), "BinTemp", "3rdParty.json")
        self.content = None
        self.third_party_root = None
        self.local_p4_sync_settings = None

        self.third_party_p4_config_file = os.path.join(self.ctx.engine_node.abspath(), "3rd_party_p4.ini")
        self.disable_p4_sync_settings = not os.path.exists(self.third_party_p4_config_file)


    def initialize(self):

        self.content = None

        if not os.path.exists(self.file_path_abs):
            # Create the initial settings file if it doesnt exist
            self.content = self.create_default()
        else:
            read_content = parse_json_file(self.file_path_abs)

            # Check if either the 3rd party root changed or the setup assistant configuration's contents changed
            try:
                check_version = read_content.get("Version")
                check_3rd_party_root = self.calculate_3rd_party_root()
                check_3rd_party_root_hash = read_content.get("3rdPartyRootHash")
                check_setup_assistant_source = read_content.get("SetupAssistantSource")
                check_setup_assistant_source_hash = read_content.get("SetupAssistantSourceHash")
                check_enabled_capabilities_hash = read_content.get("EnabledCapabilitiesHash")
                check_3rd_party_waf_script_hash = read_content.get("3rdPartyWafScriptHash")
                check_configured_platforms_hash = read_content.get("ConfiguredPlatformsHash", '')

                if not check_3rd_party_root or not check_3rd_party_root_hash or \
                    not check_setup_assistant_source or not check_setup_assistant_source_hash or \
                        not check_enabled_capabilities_hash or not check_version:
                    raise RuntimeWarning("Missing 3rd Party settings attributes")

                if check_version != CURRENT_3RD_PARTY_SETTINGS_VERSION:
                    raise RuntimeWarning("Unsupported version")

                # Create a hash of the third party lib so we can detect changes
                hash_digest_3rd_party_path = calculate_string_hash(check_3rd_party_root)
                if check_3rd_party_root_hash != hash_digest_3rd_party_path:
                    raise RuntimeWarning("3rdPartyRoot value changed.")

                if not os.path.exists(check_setup_assistant_source):
                    raise RuntimeWarning("SetupAssistantSource value invalid.")

                # Calculate the hash of the source setup assistant
                hash_src_setup_assistant_config = calculate_file_hash(check_setup_assistant_source)
                if hash_src_setup_assistant_config != check_setup_assistant_source_hash:
                    raise RuntimeWarning("SetupAssistantSourceHash value changed.")

                enabled_capabilities = self.ctx.get_enabled_capabilities()
                enabled_capabilities_hash = calculate_string_hash(",".join(enabled_capabilities))
                if enabled_capabilities_hash != check_enabled_capabilities_hash:
                    raise RuntimeWarning("EnabledCapabilitiesHash value changed.")

                available_platform = sorted(self.ctx.get_available_platforms())
                available_platform_hash = calculate_string_hash(','.join(available_platform))
                if available_platform_hash != check_configured_platforms_hash:
                    raise RuntimeWarning("ConfiguredPlatformsHash value changed.")

                # Calculate the hash of this file
                waf_3rd_party_script_file = os.path.realpath(__file__)
                if not os.path.exists(waf_3rd_party_script_file):
                    self.ctx.fatal('[ERROR] Unable to locate required file {}'.format(waf_3rd_party_script_file))
                waf_3rd_party_script_hash = calculate_file_hash(waf_3rd_party_script_file)
                if waf_3rd_party_script_hash != check_3rd_party_waf_script_hash:
                    raise RuntimeWarning("3rdPartyWafScriptHash value changed.")

                self.content = read_content

            except RuntimeWarning:
                Logs.info('[INFO] Regenerating 3rd Party settings file...')
                self.content = self.create_default()

    def create_content(self, third_party_root, setup_assistant_config_file):

        def _do_roles_match(check_roles, match_roles):
            # Check if roles defined in SetupAssistantConfig.json match with the enabled roles
            if len(check_roles) > 0:
                matched = False
                for check_role in check_roles:
                    if check_role in match_roles:
                        matched = True
                        break
            else:
                matched = True
            return matched

        # Map the target platforms to the compiler types in SetupAssistantConfig.json
        target_platform_to_compiler = {"win_x64_vs2013": "vc120",
                                       "win_x64_vs2015": "vc140",
                                       "win_x64_vs2017": "vc141",
                                       "win_x64_clang":  "vc140"}

        if not os.path.exists(setup_assistant_config_file):
            self.ctx.fatal('[ERROR] Unable to locate required file {}'.format(setup_assistant_config_file))

        # Validate the third party root
        if len(third_party_root)>0 and not os.path.exists(third_party_root):
            self.ctx.fatal('[ERROR] 3rd Party root path is invalid')

        # Create a hash of the third party lib so we can detect changes
        digest_3rd_party_path = hashlib.md5()
        digest_3rd_party_path.update(third_party_root)
        hash_digest_3rd_party_path = digest_3rd_party_path.hexdigest()

        # Create a hash of this script file so we regenerate if any of this logic changes
        waf_3rd_party_script_file = os.path.realpath(__file__)
        if not os.path.exists(waf_3rd_party_script_file):
            self.ctx.fatal('[ERROR] Unable to locate required file {}'.format(waf_3rd_party_script_file))
        waf_3rd_party_script_hash = calculate_file_hash(waf_3rd_party_script_file)

        # Read the setup assistant configuration to get the list of all of the 3rd party identifiers
        setup_assistant_contents = parse_json_file(setup_assistant_config_file)
        if 'SDKs' not in setup_assistant_contents:
            self.ctx.fatal('[ERROR] Invalid setup assistant config file ({}).  Missing "SDKs" key.'.format(setup_assistant_config_file))

        # Calculate the hash of the source setup assistant
        hash_src_setup_assistant_config = calculate_file_hash(setup_assistant_config_file)

        # Get the current enabled capabilities and build up the role list to filter on
        enabled_capabilities = self.ctx.get_enabled_capabilities()
        enabled_capabilities_hash = calculate_string_hash(",".join(enabled_capabilities))

        setup_assistant_capabilities = setup_assistant_contents.get("Capabilities")
        if not setup_assistant_capabilities:
            self.ctx.fatal('[ERROR] Invalid setup assistant config file ({}).  Missing "Capabilities" key.'.format(setup_assistant_config_file))

        filter_roles = []

        for setup_assistant_capability in setup_assistant_capabilities:

            capability_identifier = setup_assistant_capability.get("identifier")
            if not capability_identifier:
                continue

            # If enable capabitilies is not set, then check the default if this is enabled or not
            if len(enabled_capabilities) == 0 and not setup_assistant_capability.get("default", False):
                continue

            capability_categories = setup_assistant_capability.get("categories")
            if not capability_categories or "role" not in capability_categories:
                continue
            capability_tags = setup_assistant_capability.get("tags")
            if not capability_tags:
                continue
            for capability_tag in capability_tags:
                if capability_tag in enabled_capabilities and capability_tag not in filter_roles:
                    filter_roles.append(capability_tag)

        # Filter out any platform-specific libraries that are not configured for the current host platform
        host_platform = Utils.unversioned_sys_platform()

        # Map the system platforms to the platform types in SetupAssistantConfig.json
        sys_platform_to_setup_os = {'darwin': 'macOS',
                                    'win32': 'windows',
                                    'linux': 'linux'}
        restricted_platform = sys_platform_to_setup_os[host_platform]

        available_platforms = sorted(self.ctx.get_available_platforms())
        available_platforms_hash = calculate_string_hash(','.join(available_platforms))

        # Build up the SDKs section of the config
        third_party_sdks = {}
        sdk_index = 0
        for sdk in setup_assistant_contents['SDKs']:

            sdk_index += 1

            # Make sure the required identifier and source are present
            sdk_identifier = sdk.get("identifier", None)
            if not sdk_identifier:
                Logs.warn('[WARN] Missing "identifier" for "SDKs" entry {}'.format(sdk_index-1))
                continue

            sdk_source = sdk.get("source", None)
            if not sdk_source:
                Logs.warn('[WARN] Missing "source" for "SDKs" entry {}'.format(sdk_index-1))
                continue

            sdk_optional = sdk.get("optional", 0) == 1

            sdk_dependencies = sdk.get("dependencies", [])

            sdk_roles = sdk.get("roles", [])

            role_matched = _do_roles_match(sdk_roles, filter_roles)

            # Check for any host platform restrictions at the root level
            host_os_list = sdk.get("hostOS", None)
            if host_os_list:
                # If there is an OS restriction, then make sure the current restricted platform qualifies
                os_qualifies = restricted_platform in host_os_list
            else:
                # If there is no OS restriction, then all platforms qualify to this point
                os_qualifies = True

            if not os_qualifies:
                continue

            # Check for any compiler restrictions
            sdk_compilers = sdk.get("compilers")
            if sdk_compilers:
                compiler_matched = False
                for available_platform in available_platforms:
                    restricted_sdk_compiler = target_platform_to_compiler.get(available_platform)
                    if restricted_sdk_compiler is not None and restricted_sdk_compiler in sdk_compilers:
                        compiler_matched = True
                        break
                if not compiler_matched:
                    continue

            targets = {}

            # the symlinks serve as check sources to make sure certain values exist.  Create a list of
            # check-sources based on the symlinks
            sdk_symlinks = sdk.get("symlinks")
            if sdk_symlinks:

                for available_platform in available_platforms:
                    # Apply the 3rd party rule for each target platform.
                    check_files = []

                    current_platform_optional = sdk_optional
                    current_platform_roles = []
                    sdk_symlink_roles = None

                    # If there are symlinks defined, then treat them individually as targets
                    for sdk_symlink in sdk_symlinks:

                        # Check the compiler restriction if any
                        sdk_symlink_compilers = sdk_symlink.get("compilers")
                        if sdk_symlink_compilers:
                            restricted_compiler = target_platform_to_compiler.get(available_platform)
                            if restricted_compiler not in sdk_symlink_compilers:
                                # Compiler doesnt match any supported compiler
                                continue

                        # Check the hostOS restriction if any
                        sdk_symlink_hostos = sdk_symlink.get("hostOS")
                        if sdk_symlink_hostos:
                            if restricted_platform not in sdk_symlink_hostos:
                                # hostOS does not match the current host platform
                                continue
                                
                        # Check roles
                        sdk_symlink_roles = sdk_symlink.get("roles")
                        if sdk_symlink_roles:
                            if sdk_symlink_roles not in filter_roles:
                                # roles does not match the current roles
                                continue
                                
                        sdk_symlink_check_source = sdk_symlink.get("source")
                        if not sdk_symlink_check_source:
                            continue

                        # Check if there is an optional override at the 'symlinks' level
                        sdk_symlink_optional = sdk_symlink.get("optional", 1 if sdk_optional else 0)
                        current_platform_optional = current_platform_optional and (sdk_symlink_optional==1)

                        sdk_symlink_roles = sdk_symlink.get("roles", None)
                        if sdk_symlink_roles:
                            current_platform_roles += [sdk_symlink_role for sdk_symlink_role in sdk_symlink_roles]

                        sdk_symlink_example = sdk_symlink.get("exampleFile", "")
                        if len(sdk_symlink_example)>0:
                            check_files.append(os.path.normpath(os.path.join(sdk_symlink_check_source, sdk_symlink_example)))

                    if len(current_platform_roles)==0:
                        current_platform_roles = sdk_roles
                    sdk_platform_role_matched = _do_roles_match(current_platform_roles, filter_roles)

                    # Each target consists of the base sdk path, and the check sources
                    target_entry = {
                                    "source": sdk_source,
                                    "check_files": check_files,
                                    "optional": current_platform_optional,
                                    "roles": sdk_symlink_roles,
                                    "dependencies":sdk_dependencies,
                                    "enabled": sdk_platform_role_matched
                                   }

                    targets[available_platform] = target_entry
            else:
                # If there are no symlinks defined, then apply the same targets for all supporting platforms
                for available_platform in available_platforms:
                    targets[available_platform] = {"source": sdk_source,
                                                   "check_files": [],
                                                   "optional": sdk_optional,
                                                   "roles": sdk_roles,
                                                   "dependencies":sdk_dependencies,
                                                   "enabled": role_matched
                                                   }

            third_party_sdk = {"base_source": sdk_source,
                               "targets": targets,
                               "enabled": role_matched}
            third_party_sdks[sdk_identifier] = third_party_sdk

        third_party_settings_content = {
            "Version": CURRENT_3RD_PARTY_SETTINGS_VERSION,
            "3rdPartyRoot": third_party_root,
            "3rdPartyRootHash": hash_digest_3rd_party_path,
            "EnabledCapabilitiesHash": enabled_capabilities_hash,
            "SetupAssistantSource": setup_assistant_config_file,
            "SetupAssistantSourceHash": hash_src_setup_assistant_config,
            "3rdPartyWafScriptHash": waf_3rd_party_script_hash,
            "ConfiguredPlatformsHash" : available_platforms_hash,

            "SDKs": third_party_sdks
        }
        return third_party_settings_content

    def calculate_3rd_party_root(self):
        """
        Calculate where the 3rd party folder will be based on.  The 3rd party root folder must have the 3rdParty.txt marker file

        :return: A third party root path
        """

        def _search_3rd_party(starting_path):
            # Attempt to locate the 3rd party marker file from the current working directory
            # Move up to the root
            current_path = starting_path
            last_dir = None
            while current_path != last_dir:
                third_party_dir_name = os.path.join(current_path, '3rdParty')
                if os.path.exists(os.path.join(third_party_dir_name, '3rdParty.txt')):
                    return third_party_dir_name
                last_dir = current_path
                current_path = os.path.dirname(current_path)
            return None

        def _get_default_third_party_root():
            # If setup assistant user prefs does not exist, then attempt to search for a 3rd party root folder in
            # the current drive and use that as the default for now but warn the user
            third_party_root = _search_3rd_party(self.ctx.path.abspath())
            if not third_party_root:
                raise ConfigurationError('[ERROR] Setup Assistant has not configured this project yet.  You must run SetupAssistant '
                                         'first or specify a root 3rd Party folder (--3rdpartypath). ')

            self.ctx.warn_once('Setup Assistant has not configured this project yet.  Defaulting to "{}" to continue this '
                               'configuration.  It is recommended that you run SetupAssistant to configure this project properly '
                               'before continuing.'.format(third_party_root))
            return third_party_root

        def _read_setup_assistant_user_pref():

            setup_assistant_user_pref_filepath = os.path.join(self.ctx.path.abspath(), CONFIG_SETUP_ASSISTANT_USER_PREF)
            if not os.path.exists(setup_assistant_user_pref_filepath):
                return _get_default_third_party_root()
            try:
                config_parser = ConfigParser.ConfigParser()
                config_parser.read(setup_assistant_user_pref_filepath)

                engine_path_valid = config_parser.get(SETUP_ASSISTANT_PREF_SECTION, SETUP_ASSISTANT_ENGINE_PATH_VALID)
                if engine_path_valid != 'true':
                    raise ConfigurationError('[ERROR] Setup Assistant reports that the engine path is invalid.  Correct this in Setup Assistant before continuing')

                third_party_path = config_parser.get(SETUP_ASSISTANT_PREF_SECTION, SETUP_ASSISTANT_ENGINE_PATH_3RD_PARTY)
                if not os.path.exists(os.path.join(third_party_path, "3rdParty.txt")):
                    raise ConfigurationError('[ERROR] The 3rd Party Root path ({}) configured in setup assistant is invalid.  Correct this in Setup Assistant before continuing'.format(third_party_path))

                return os.path.normpath(third_party_path)

            except ConfigParser.Error as err:
                return _get_default_third_party_root()


        third_party_override = getattr(self.ctx.options,"bootstrap_third_party_override",None)
        if third_party_override:
            if not os.path.exists(os.path.join(third_party_override, "3rdParty.txt")):
                Logs.warn('[WARN] The 3rd party override (--3rdpartypath) path ({})is not a valid 3rd party root folder.'.format(third_party_override))
            else:
                return os.path.normpath(third_party_override)

        return _read_setup_assistant_user_pref()

    def create_default(self, third_party_root_override = None):
        """
        Create the default 3rd party settings file
        :return:
        """
        # Create an initial 3rd party root path
        if third_party_root_override:
            third_party_root_path = third_party_root_override
        else:
            third_party_root_path = self.calculate_3rd_party_root()

        third_party_settings_contents = self.create_content(third_party_root_path,
                                                            os.path.join(self.ctx.engine_node.abspath(),
                                                                         CONFIG_FILE_SETUP_ASSISTANT_CONFIG))

        # Create the 3rd party settings
        write_json_file(third_party_settings_contents, self.file_path_abs)

        return third_party_settings_contents

    def validate_sdk(self, sdk_identifier, sdk_target, third_party_root):
        sdk_validated = True

        sdk_target_source = sdk_target.get("source")
        sdk_check_files = sdk_target.get("check_files")

        sdk_enabled = sdk_target.get("enabled", False)
        sdk_optional = sdk_target.get("optional", True)

        for sdk_check_file in sdk_check_files:
            sdk_check_file_full_path = os.path.join(third_party_root, sdk_check_file)
            sdk_check_source_path = os.path.dirname(sdk_check_file_full_path)
            if not os.path.exists(
                    sdk_check_file_full_path) and sdk_identifier not in EXEMPT_SDKS and sdk_enabled and not sdk_optional:
                if not self.disable_p4_sync_settings:
                    Logs.debug("Lumberyard: Attempting to sync 3rd Party {} from perforce.".format(sdk_identifier))
                    # Build the p4 path based on the full check path file's directory
                    sdk_p4_sync_path = os.path.dirname(sdk_check_file).replace('\\', '/')
                    if not self.sync_from_p4(sdk_p4_sync_path):
                        Logs.debug("Lumberyard: Unable to resolve 3rd Party path {} for library {}.".format(
                            sdk_target_source, sdk_identifier))
                    if not os.path.exists(sdk_check_file_full_path):
                        self.ctx.fatal(
                            "[ERROR] Missing 3rd Party library {}. Expected at {}.".format(sdk_identifier,
                                                                                           sdk_check_file_full_path))
                else:
                    self.ctx.warn_once(
                        "Unable to resolve 3rd Party path {} for library {}.".format(sdk_check_source_path,
                                                                                     sdk_identifier))
                    sdk_validated = False

        return sdk_validated

    def validate_local(self, target_platform):
        """
        Validate the settings against the actual file system
        :return:
        """
        if not self.content:
            raise Errors.WafError('[ERROR] 3rd Party settings file not initialized properly')

        third_party_root = os.path.normpath(self.content.get("3rdPartyRoot"))

        content_sdks = self.content.get("SDKs")
        for sdk_identifier, sdk_content in content_sdks.iteritems():
            sdk_targets = sdk_content.get("targets")
            sdk_target = sdk_targets.get(target_platform)

            sdks_to_validate = [ (sdk_identifier, sdk_target) ]

            if sdk_target:
                enabled_capabilities = self.ctx.get_enabled_capabilities()
                sdk_roles = sdk_target.get("roles")
                sdk_dependencies = sdk_target.get("dependencies")

                if not sdk_roles:
                    continue
                    
                sdk_in_capabilities = False

                for role in sdk_roles:
                    if role in enabled_capabilities:
                        sdk_in_capabilities = True
                        break

                if not sdk_in_capabilities:
                    continue

                for sdk_dependency in sdk_dependencies:
                    dependency_content = content_sdks.get(sdk_dependency)
                    dependency_targets = dependency_content.get('targets')
                    dependency_target = dependency_targets.get(target_platform)

                    sdks_to_validate.append((sdk_dependency, dependency_target))


                for sdk_to_validate in sdks_to_validate:
                    if not self.validate_sdk(sdk_to_validate[0], sdk_to_validate[1], third_party_root):
                        # One or more paths doesnt exist for this alias, do an encapsulating warning for the sdk alias
                        self.ctx.fatal("[ERROR] 3rd Party alias '{}' invalid. It references one or more invalid local paths. Make sure the sdk is configured properly in Setup Assistant.".format(sdk_to_validate[0]))


    def sync_from_p4(self, third_party_subpath):

        if self.local_p4_sync_settings is None:
            self.local_p4_sync_settings = P4SyncThirdPartySettings(self.content.get("3rdPartyRoot"),
                                                                   self.third_party_p4_config_file)
        if not self.local_p4_sync_settings.is_valid():
            self.local_p4_sync_settings = None
            self.disable_p4_sync_settings = True
            return False

        self.local_p4_sync_settings.sync_3rd_party(third_party_subpath)

        return True

    def get_third_party_path(self, target_platform, identifier):
        """
        Resolves a third party library configuration based on the target platform and 3rd party identifier (from SetupAssistantConfig.json)

        :param target_platform:     The target platform to resolve
        :param identifier:          The 3rd party alias identifier to lookup
        :return:                    Tuple representing a 3rd party setup:
                                        source path: The absolute path to the 3rd party library
                                        enabled:     Flag to indicate if the library is enabled or not
                                        roles:       Roles that are configured for the 3rd party library in SetupAssistantConfig.json
                                        optional:    Flag to indicate if the sdk is optional (ie not a critical SDK) where no specs can be built
        """

        if not self.content:
            raise Errors.WafError('3rd Party settings file not initialized properly')

        content_sdks = self.content.get("SDKs")
        sdk_content = content_sdks.get(identifier)
        if not sdk_content:
            raise Errors.WafError('3rd Party settings file is invalid (Missing SDKs/{})'.format(identifier))
        if not sdk_content.get("validated", True):
            raise Errors.WafError("3rd Party alias '{}' not validated locally.".format(identifier))
        sdk_enabled = sdk_content.get('enabled')
        sdk_optional = sdk_content.get('optional')
        sdk_roles = sdk_content.get('roles', [])

        if not target_platform:
            # If the target platform is not specified, then use the base source
            sdk_source = sdk_content.get("base_source")
            if not sdk_source:
                raise Errors.WafError("[ERROR] 3rd Party settings file is invalid (Missing SDKs/{}/base_source)".format(identifier))
        else:

            sdk_content_targets = sdk_content.get("targets")
            if not sdk_content_targets:
                raise Errors.WafError("[ERROR] 3rd Party settings file is invalid (Missing SDKs/{}/targets)".format(identifier))

            sdk_content_target = sdk_content_targets.get(target_platform)
            if not sdk_content_target:
                raise Errors.WafError("[ERROR] 3rd Party settings file is invalid (Missing SDKs/targets/{})".format(target_platform))

            sdk_source = sdk_content_target.get("source")

            # Get any target specific override for 'optional' and 'roles'
            sdk_optional = sdk_content_target.get('optional', sdk_optional)
            sdk_roles = sdk_content_target.get('roles', sdk_roles)


        third_party_root = self.content.get("3rdPartyRoot")
        sdk_source_full_path = os.path.normpath(os.path.join(third_party_root, sdk_source))

        if not os.path.exists(sdk_source_full_path) and sdk_enabled and not sdk_optional:
            self.ctx.warn_once('3rd party alias {} references a missing local SDK path. Make sure it is available through Setup Assistant'.format(identifier))

        return sdk_source_full_path, sdk_enabled, sdk_roles, sdk_optional





