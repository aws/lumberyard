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

# System Imports
import os
import subprocess
import argparse
import shlex
# waflib imports
from waflib import Context, Utils, Logs, Errors, Options
from waflib.Configure import conf, ConfigurationContext

# lmbrwaflib imports
from lmbrwaflib.cry_utils import append_to_unique_list, split_comma_delimited_string

# misc imports
from waf_branch_spec import BINTEMP_FOLDER
from waf_branch_spec import CACHE_FOLDER
from waf_branch_spec import LUMBERYARD_VERSION
from waf_branch_spec import LUMBERYARD_BUILD
from waf_branch_spec import ADDITIONAL_COPYRIGHT_TABLE


# Copyright table lookup by copyright organization ( copyright_org for the wscript keyword. )
# Initially populated for Amazon and updated with any values from waf_branch_spec
COPYRIGHT_TABLE = {}

DEFAULT_COMPANY_NAME = 'Amazon.com, Inc.'
DEFAULT_COPYRIGHT_TABLE = {
    'Amazon':           'Copyright (c) Amazon.com, Inc. or its affiliates',
    'Amazon-CryTek':    'Copyright (c) Amazon.com, Inc., its affiliates or its licensors'
}
for default_copyright_org in DEFAULT_COPYRIGHT_TABLE:
    COPYRIGHT_TABLE[default_copyright_org] = DEFAULT_COPYRIGHT_TABLE[default_copyright_org]
for additional_copyright_org in ADDITIONAL_COPYRIGHT_TABLE:
    COPYRIGHT_TABLE[additional_copyright_org] = ADDITIONAL_COPYRIGHT_TABLE[additional_copyright_org]


@conf
def _get_settings_search_roots(ctx):
    """ Helper function for getting the settings search root nodes in reverse order of precedence """
    search_roots = [ ctx.get_launch_node() ]

    if not ctx.is_engine_local():
        search_roots.insert(0, ctx.get_engine_node())

    return search_roots


@conf
def get_bintemp_folder_node(ctx):
    """
    Get the Node for BinTemp. The location of BinTemp relative to the context depends on the context type and if it has a build variant:
    
    1. If ctx is a ConfigurationContext (ie lmbr_waf configure), then there is no bldnode set in the context, so we will derive the BinTemp
       location from the 'path' attribute, which should be the root of the project
    2. If ctx is a BuildContext, but has no variant (ie it is a 'project_generator' platform like msvs, android_studio, etc), then the 'bldnode'
       will be the BinTemp folder
    3. If ctx is a BuildContext and has a build variant, then it is a build_ command. The BinTemp folder should be the parent folder of the
       variant folder.
    """
    try:
        return ctx.bintemp_node
    except AttributeError:
        if hasattr(ctx, 'bldnode'):
            # If this build context has no variant (ie project generator tasks), then the bldnode will point
            # directly to BinTemp
            if str(ctx.bldnode) == BINTEMP_FOLDER:
                check_bintemp = ctx.bldnode
            else:
                check_bintemp = ctx.bldnode.parent

            if str(check_bintemp) != BINTEMP_FOLDER:
                raise Errors.WafError("[ERROR] Expected BinTemp for bldnode but found {}".format(os.path.dirname(ctx.bldnode.abspath())))

            ctx.bintemp_node = check_bintemp
        else:
            # If the current context does not have a 'bldnode' setup, that means it is not setup yet. We can still
            # derive the location based on the 'path' attribute which should the project root
            ctx.bintemp_node = ctx.path.make_node(BINTEMP_FOLDER)
        return ctx.bintemp_node


@conf
def get_cache_folder_node(ctx):
    """
    Get the Node for the Cache folder
    """
    try:
        return ctx.cache_folder_node
    except AttributeError:
        # The cache folder node will always be at the same level as BinTemp
        bintemp_node = ctx.get_bintemp_folder_node()
        ctx.cache_folder_node = bintemp_node.parent.make_node(CACHE_FOLDER)
        return ctx.cache_folder_node

#############################################################################
@conf
def get_company_name(self, project, copyright_org):

    if copyright_org in DEFAULT_COPYRIGHT_TABLE:
        return DEFAULT_COMPANY_NAME
    elif copyright_org in COPYRIGHT_TABLE:
        return copyright_org
    else:
        Logs.warn('[WARN] Copyright org "{}" for project "{}" is not valid.  Using default "{}".'.format(copyright_org, project, DEFAULT_COMPANY_NAME))
        return DEFAULT_COMPANY_NAME

#############################################################################
@conf
def get_lumberyard_version(self):
    return LUMBERYARD_VERSION

#############################################################################
@conf
def get_lumberyard_build(self):
    return LUMBERYARD_BUILD

#############################################################################
@conf
def get_game_project_version(self):
    return self.options.version

#############################################################################
@conf
def get_copyright(self, project, copyright_org):

    if copyright_org in COPYRIGHT_TABLE:
        return COPYRIGHT_TABLE[copyright_org]
    else:
        return COPYRIGHT_TABLE['Amazon']

@conf
def _get_valid_platform_info_filename(self):
    return "valid_configuration_platforms.json"

#############################################################################
#############################################################################


# Attempt to import the winregistry module.
try:
    import winreg

    WINREG_SUPPORTED = True
except ImportError:
    WINREG_SUPPORTED = False
    pass

@conf
def get_project_vs_filter(self, target):
    if not hasattr(self, 'vs_project_filters'):
        self.fatal('vs_project_filters not initialized')

    if not target in self.vs_project_filters:
        self.fatal('No visual studio filter entry found for %s' % target)

    return self.vs_project_filters[target]


def update_android_environment_from_bootstrap_params(environment, bootstrap_params):
    """Allows the updating of the environment block if they are overriden on the command line.
    environment is a dictionary it will write to.  Bootstrap params is a string that is expected
    to be in shell format, containing what params would have been sent to the bootstrap tool.
    """
    bootstrap_param_parser = argparse.ArgumentParser()
    bootstrap_param_parser.add_argument('--jdk', action='store')
    bootstrap_param_parser.add_argument('--android-ndk', action='store')
    bootstrap_param_parser.add_argument('--android-sdk', action='store')
    bootstrap_param_parser.add_argument('--enablecapability', action='append')
    
    # use shlex to lex the command line.  This correctly applies rules for
    # spaces and quotes.  We want to tweak it though, so that it does not
    # take backslash to mean an escape character (this is for backward compat with existing build scripts)
    lexed = shlex.shlex(bootstrap_params, posix=True)
    lexed.whitespace_split = True
    lexed.escape = '' # do not use back slashes to escape.
    params = list(lexed)
    
    #params = shlex.split(bootstrap_params, posix=True, escape='')
    parsed_params = bootstrap_param_parser.parse_known_args(params)[0]
    parsed_capabilities = getattr(parsed_params, 'enablecapability', [])
    jdk_override = getattr(parsed_params, 'jdk', None)
    android_sdk_override = getattr(parsed_params, 'android_sdk', None)
    android_ndk_override = getattr(parsed_params, 'android_ndk', None)
    android_is_enabled = parsed_capabilities and 'compileandroid' in parsed_capabilities
    
    if jdk_override:
        environment['LY_JDK'] = jdk_override
    if android_sdk_override:
        environment['LY_ANDROID_SDK'] = android_sdk_override
    if android_ndk_override:
        environment['LY_ANDROID_NDK'] = android_ndk_override
    if android_is_enabled:
        environment['ENABLE_ANDROID'] = 'True' # this is not a typo, its a JSON string 'True'

def _load_environment_file(ctx):
    """ Helper function for loading the environment file created by Setup Assistant """

    if hasattr(ctx, 'local_environment'):
        return

    if isinstance(ctx, Options.OptionsContext):
        # during 'options' phase (where we declare the command line options, before we parse them)
        # it is not okay to go loading or overriding any files, since the actual command line
        # does not exist.
        return

    settings_files = list()
    for root_node in _get_settings_search_roots(ctx):
        settings_node = root_node.make_node(['_WAF_', 'environment.json'])
        if os.path.exists(settings_node.abspath()):
            settings_files.append(settings_node)

    ctx.local_environment = dict()
    for settings_file in settings_files:
        try:
            ctx.local_environment.update(ctx.parse_json_file(settings_file))
        except Exception as e:
            ctx.cry_file_error(str(e), settings_file.abspath())

    # allow command line params to override this (for build systems doing android).
    # this is hardcoded here for build systems to override the stuff that would be written into environment.json to
    # just load from the command line instead.
    bootstrap_params = getattr(ctx.options,'bootstrap_tool_param',None)
    if bootstrap_params:
        update_android_environment_from_bootstrap_params(ctx.local_environment, bootstrap_params)

def _get_environment_file_var(ctx, env_var):
    _load_environment_file(ctx)
    return ctx.local_environment[env_var]

@conf
def get_env_file_var(conf, env_var, required = False, silent = False):
    """Gets a value of a variable inside the android settings file called _WAF_/environment.json"""
    try:
        return _get_environment_file_var(conf, env_var)
    except KeyError: # other exceptions such as failure to parse malformed command line or JSON must still function!
        if not silent:
            message = 'Failed to find {} in _WAF_/environment.json or bootstrap-tool-param command line: {}'.format(env_var, e)
            if required:
                Logs.error('[ERROR] %s' % message)
            else:
                Logs.warn('[WARN] %s' % message)

    return ''


def _load_specs(ctx):
    """ Helper function to find all specs stored in _WAF_/specs/*.json """
    if hasattr(ctx, 'loaded_specs_dict'):
        return

    ctx.loaded_specs_dict = {}

    search_roots = _get_settings_search_roots(ctx)

    # load the spec files from both the engine and project location
    # were the project version will take prior if a duplicate is found
    spec_files = dict()
    for root_node in search_roots:
        spec_folder_node = root_node.make_node(['_WAF_', 'specs'])
        if os.path.exists(spec_folder_node.abspath()):
            spec_file_nodes = spec_folder_node.ant_glob('**/*.json')
            spec_files.update(
                { node.name : node for node in spec_file_nodes }
            )

    for file in spec_files.values():
        try:
            spec = ctx.parse_json_file(file)
            spec_name = str(file).split('.')[0]
            ctx.loaded_specs_dict[spec_name] = spec
        except Exception as e:
            ctx.cry_file_error(str(e), file.abspath())

    # index '-1', or the last element in the list, is guaranteed to be launch dir (project)
    launch_node = search_roots[-1]

    # For any enabled game project, see if it has a WAFSpec subfolder as well
    enabled_game_projects_list = split_comma_delimited_string(ctx.options.enabled_game_projects, True)
    for project in enabled_game_projects_list:
        game_project_spec = launch_node.make_node([project, 'WAFSpec', '{}.json'.format(project)])
        if os.path.exists(game_project_spec.abspath()):
            try:
                spec = ctx.parse_json_file(game_project_spec)
                if project in ctx.loaded_specs_dict:
                    Logs.warn("[WARN] Game project WAF spec '{}' in game project folder '{}' collides with a spec in _WAF_/specs.  "
                              "Overriding the _WAF_/specs version with the one in the game project"
                              .format(project, project))
                ctx.loaded_specs_dict[project] = spec
            except Exception as e:
                ctx.cry_file_error(str(e), game_project_spec.abspath())



@conf
def loaded_specs(ctx):
    """ Get a list of the names of all specs """
    _load_specs(ctx)

    ret = []
    for (spec,entry) in list(ctx.loaded_specs_dict.items()):
        ret.append(spec)

    return ret

spec_entry_cache = {}

def _spec_entry(ctx, entry, spec_name=None, platform=None, configuration=None):
    """ Util function to load a specific spec entry (always returns a list) """

    if not spec_name: # Fall back to command line specified spec if none was given
        spec_name = ctx.options.project_spec

    cache_key = '{}_{}_{}_{}'.format(entry, spec_name, platform, configuration)
    if cache_key in spec_entry_cache:
        return spec_entry_cache[cache_key]

    _load_specs(ctx)

    spec = ctx.loaded_specs_dict.get(spec_name, None)
    if not spec:
        ctx.fatal('[ERROR] Unknown Spec "%s", valid settings are "%s"' % (spec_name, ', '.join(ctx.loaded_specs())))

    def _to_list( value ):
        if isinstance(value,list):
            return value
        return [ value ]

    # copy the list since its kept in memory to prevent the 'get platform specifics' call
    # from mutating the list with a bunch of duplicate entries
    res = _to_list( spec.get(entry, []) )[:]

    # include the platform/config specific modules, if supplied
    if platform and configuration:
        res += ctx.GetPlatformSpecificSettings(spec, entry, platform, configuration)

    spec_entry_cache[cache_key] = res

    return res

spec_modules_cache = {}
@conf
def spec_modules(ctx, spec_name = None, platform=None, configuration=None):
    """ Get all a list of all modules needed for this spec """

    if not spec_name: # Fall back to command line specified spec if none was given
        spec_name = ctx.options.project_spec

    cache_key = '{}_{}_{}'.format(spec_name, platform, configuration)
    if cache_key in spec_modules_cache:
        return spec_modules_cache[cache_key]

    module_list = _spec_entry(ctx, 'modules', spec_name, platform, configuration)

    spec_name_for_warning = spec_name if spec_name else ctx.options.project_spec

    filtered_module_list = []
    for module in module_list:
        if module in filtered_module_list:
            ctx.warn_once("Duplicate module '{}' in spec '{}'".format(module, spec_name_for_warning))
        elif not ctx.is_valid_module(module):
            ctx.warn_once("Invalid module '{}' in spec '{}'".format(module, spec_name_for_warning))
        else:
            filtered_module_list.append(module)

    if platform is not None:
        platform_filtered_module_list = [module for module in filtered_module_list if platform in ctx.get_module_platforms(module)]
        filtered_module_list = platform_filtered_module_list
    if configuration is not None:
        configuration_filtered_module_list = [module for module in filtered_module_list if configuration in ctx.get_module_configurations(module, platform)]
        filtered_module_list = configuration_filtered_module_list

    spec_modules_cache[cache_key] = filtered_module_list
    return filtered_module_list


@conf
def is_project_spec_specified(ctx):
    return len(ctx.options.project_spec) > 0


@conf
def is_target_enabled(ctx, target, is_launcher):
    """
    Check if the target is enabled for the build based on whether a spec file was specified
    """
    if not is_project_spec_specified(ctx):
        # No spec means all projects are enabled
        return True

    project_spec = ctx.options.project_spec
    if is_launcher:
        # If we are building a launcher, the only restriction is if the spec explicitly disables building game launchers
        return not ctx.spec_disable_games(project_spec)

    # prevent sending an empty list form being passed down in the unlikely chance the keys don't exist
    platform = ctx.env['PLATFORM'] or None
    config = ctx.env['CONFIGURATION'] or None

    # If the target in one of the explicit spec modules
    if target in ctx.spec_modules(project_spec, platform, config):
        return True

    # If the target is implied by a use dependency of one of the spec modules
    if target in ctx.get_all_module_uses(project_spec, platform, config):
        return True

    return False
@conf
def add_target_to_spec(ctx, target, spec_name=None):
    """
    Forcibly add a target to a spec even if its not in spec file
    
    :param ctx:         Context
    :param target:      The target to add to the spec
    :param spec_name:   (optional) the name of a specific spec to add to. None means add to all specs
    """

    # Prepare the list of specs to add the target to
    specs_to_add = []
    if spec_name:
        # A specific spec
        specs_to_add.append(spec_name)
    elif is_project_spec_specified(ctx):
        # If a spec was specified through the command line
        specs_to_add.append(ctx.options.project_spec)
    else:
        # No spec, load all of the loaded specs
        specs_to_add.extend(list(ctx.self.loaded_specs_dict.keys()))
        
    for spec_to_add in specs_to_add:
        # For each spec, check if the target even needs to be added to the spec
        spec_dict = ctx.loaded_specs_dict[spec_to_add]
        spec_module_list = spec_dict.setdefault('modules', [])
        if target not in spec_module_list:
            # Add to the module list for the spec
            spec_module_list.append(target)
            # Update any cached module list for this spec with this target as well
            cache_key_prefix = '{}_'.format(spec_to_add)
            for cache_key, cache_key_values in list(spec_modules_cache.items()):
                if cache_key.startswith(cache_key_prefix):
                    cache_key_values.append(target)


@conf
def spec_defines(ctx, spec_name = None):
    """ Get all a list of all defines needed for this spec """
    """ This function is deprecated """
    return _spec_entry(ctx, 'defines', spec_name)

@conf
def spec_platforms(ctx, spec_name = None):
    """ Get all a list of all defines needed for this spec """
    """ This function is deprecated """
    return _spec_entry(ctx, 'platforms', spec_name)

@conf
def spec_configurations(ctx, spec_name = None):
    """ Get all a list of all defines needed for this spec """
    """ This function is deprecated """
    return _spec_entry(ctx, 'configurations', spec_name)

@conf
def spec_visual_studio_name(ctx, spec_name = None):
    """ Get all a the name of this spec for Visual Studio """
    visual_studio_name =  _spec_entry(ctx, 'visual_studio_name', spec_name)

    if not visual_studio_name:
        ctx.fatal('[ERROR] Mandatory field "visual_studio_name" missing from spec "%s"' % (spec_name or ctx.options.project_spec) )

    # _spec_entry always returns a list, but client code expects a single string
    assert(len(visual_studio_name)== 1)
    return visual_studio_name[0]


@conf
def spec_exclude_test_configurations(ctx, spec_name = None):

    exclude_test_configurations = _spec_entry(ctx, 'exclude_test_configurations', spec_name)
    if not exclude_test_configurations:
        return False
    return exclude_test_configurations[0]


@conf
def spec_exclude_monolithic_configurations(ctx, spec_name=None):

    exclude_test_configurations = _spec_entry(ctx, 'exclude_monolithic_configurations', spec_name)
    if not exclude_test_configurations:
        return False
    return exclude_test_configurations[0]


# The GAME_FOLDER_MAP is used as an optional dictionary that maps a game name to a specific folder.  This dictionary is
# populated based on the current spec that is used.  By default, the game name and the folder name are the same.
GAME_FOLDER_MAP = {}


@conf
def get_game_asset_folder(_, game_name):
    global GAME_FOLDER_MAP
    return GAME_FOLDER_MAP.get(game_name,game_name)


@conf
def spec_override_gems_json(ctx, spec_name = None):
    return _spec_entry(ctx, 'override_gems_json', spec_name)


@conf
def spec_additional_code_folders(ctx, spec_name = None):
    return _spec_entry(ctx, 'additional_code_folders', spec_name)


@conf
def spec_game_projects(ctx, spec_name=None):
    """
    Get and game projects defined for the spec.  Will strip out duplicate entries
    """
    project_settings_map = ctx.get_project_settings_map()

    game_projects = _spec_entry(ctx, 'game_projects', spec_name)
    if game_projects is None:
        return []
    unique_list = list()
    for game_project in game_projects:
        if game_project in project_settings_map:
            append_to_unique_list(unique_list, game_project)

    # Check if this spec has any game->folder map
    global GAME_FOLDER_MAP
    spec_game_folder_map_list = _spec_entry(ctx, 'game_folders', spec_name)
    if spec_game_folder_map_list and len(spec_game_folder_map_list) > 0:
        # If there is an override game folder map in the spec, then validate its uniqueness and add it to the map
        spec_game_folder_map = spec_game_folder_map_list[0]
        for game_name, game_folder in list(spec_game_folder_map.items()):
            if game_name in GAME_FOLDER_MAP:
                current_game_folder = GAME_FOLDER_MAP[game_name]
                if current_game_folder != game_folder:
                    raise Errors.WafError('Conflicting game name to folder map detected in spec {}'.format(spec_name))
            else:
                GAME_FOLDER_MAP[game_name] = game_folder

    return unique_list


@conf
def is_valid_spec_name(ctx, spec_name):
    """ Check a spec name if its a valid one"""
    _load_specs(ctx)
    return spec_name in ctx.loaded_specs_dict


@conf
def spec_description(ctx, spec_name = None):
    """ Get description for this spec """
    description = _spec_entry(ctx, 'description', spec_name)
    if description == []:
        ctx.fatal('[ERROR] Mandatory field "description" missing from spec "%s"' % (spec_name or ctx.options.project_spec) )

    # _spec_entry always returns a list, but client code expects a single string
    assert( len(description) == 1 )
    return description[0]


@conf
def spec_disable_games(ctx, spec_name = None):
    """ For a given spec, are game projects disabled?  For example, tool-only specs do this."""

    # If no spec is supplied, then no games are disabled
    if len(ctx.options.project_spec)==0 and not spec_name:
        return False

    disable_games = _spec_entry(ctx, 'disable_game_projects', spec_name)
    if disable_games == []:
        return False

    return disable_games[0]


@conf
def spec_additional_launchers(ctx, spec_name = None):
    """ For a given spec, add any additional custom launchers"""
    if spec_name is None and len(ctx.options.project_spec)==0:
        specs_to_include = [spec for spec in ctx.loaded_specs() if not ctx.spec_disable_games(spec)]
        additional_launcher_for_all_specs = []
        for spec in specs_to_include:
            spec_additional_launchers = _spec_entry(ctx, 'additional_launchers', spec)
            append_to_unique_list(additional_launcher_for_all_specs, spec_additional_launchers)
        return additional_launcher_for_all_specs
    else:
        additional_launcher_projects = _spec_entry(ctx, 'additional_launchers', spec_name)
    return additional_launcher_projects


@conf
def project_launchers(ctx, default_launchers, spec_name = None):

    launchers = set(default_launchers)

    projects_settings = ctx.get_project_settings_map()

    enabled_game_projects = ctx.get_enabled_game_project_list()
    if len(enabled_game_projects) == 0:
        # No game projects, so no launchers
        return []

    # Add any spec defined launchers
    spec_launchers = spec_additional_launchers(ctx,spec_name)
    for spec_launcher in spec_launchers:
        launchers.add(spec_launcher)

    # First pass is to collect all the additional launchers
    for project_name, project_values in list(projects_settings.items()):
        if 'additional_launchers' in project_values:
            for additional_launcher in project_values['additional_launchers']:
                launchers.add(additional_launcher)

    return list(launchers)


@conf
def spec_restricted_launchers(ctx, spec_name = None):
    """ For a given spec, See if we are restricting particular launchers for the spec"""
    if spec_name is None and len(ctx.options.project_spec) == 0:
        specs_to_restrict = [spec for spec in ctx.loaded_specs() if not ctx.spec_disable_games(spec)]
        restricted_launcher_list = []
        for spec in specs_to_restrict:
            spec_restrict_launchers = _spec_entry(ctx, 'restricted_launchers', spec)
            append_to_unique_list(restricted_launcher_list, spec_restrict_launchers)
        return restricted_launcher_list
    else:
        additional_launcher_projects = _spec_entry(ctx, 'restricted_launchers', spec_name)
    return additional_launcher_projects


# Set global output directory
setattr(Context.g_module, Context.OUT, BINTEMP_FOLDER)
