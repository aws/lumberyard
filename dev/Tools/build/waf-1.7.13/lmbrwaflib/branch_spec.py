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

import os
import subprocess

from waflib import Context, Utils, Logs, Errors
from waflib.Configure import conf, ConfigurationContext
from cry_utils import append_to_unique_list, split_comma_delimited_string

from waf_branch_spec import BINTEMP_FOLDER
from waf_branch_spec import CACHE_FOLDER
from waf_branch_spec import AVAILABLE_LAUNCHERS
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
    COPYRIGHT_TABLE[additional_copyright_org] = COPYRIGHT_TABLE[additional_copyright_org]


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
def get_dep_proj_folder_name(self, msvs_ver):
    if msvs_ver == "14":
        return self.options.visual_studio_solution_name + '_vc' + msvs_ver + '0.depproj'
    else:
        return self.options.visual_studio_solution_name + '_vs' + msvs_ver + '.depproj'


@conf
def get_project_output_folder(self, msvs_ver):
    project_folder_node = self.root.make_node(Context.launch_dir).make_node(self.options.visual_studio_solution_folder).make_node(self.get_dep_proj_folder_name(msvs_ver))
    project_folder_node.mkdir()
    return project_folder_node

#############################################################################
@conf
def get_solution_name(self, msvs_ver):
    if msvs_ver == "14":
        return self.options.visual_studio_solution_folder + '/' + self.options.visual_studio_solution_name + '_vc' + msvs_ver + '0.sln'
    else:
        return self.options.visual_studio_solution_folder + '/' + self.options.visual_studio_solution_name + '_vs' + msvs_ver + '.sln'

#############################################################################
@conf
def get_solution_dep_proj_folder_name(self, msvs_ver):
    return self.options.visual_studio_solution_folder + '/' + self.get_dep_proj_folder_name(msvs_ver)

#############################################################################
#############################################################################
@conf
def get_appletv_project_name(self):
    return self.options.appletv_project_folder + '/' + self.options.appletv_project_name

@conf
def get_ios_project_name(self):
    return self.options.ios_project_folder + '/' + self.options.ios_project_name


@conf
def get_mac_project_name(self):
    return self.options.mac_project_folder + '/' + self.options.mac_project_name


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
    import _winreg

    WINREG_SUPPORTED = True
except ImportError:
    WINREG_SUPPORTED = False
    pass


def get_msbuild_root(toolset_version):
    """
    Get the MSBuild root installed folder path from the registry
    """

    reg_key = 'SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\{}.0'.format(toolset_version)
    msbuild_root_regkey = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, reg_key, 0, _winreg.KEY_READ)
    (root_path, type) = _winreg.QueryValueEx(msbuild_root_regkey, 'MSBuildToolsRoot')
    return root_path.encode('utf-8')


def find_vswhere():
    vs_path = os.environ['ProgramFiles(x86)']
    vswhere_exe = os.path.join(vs_path, 'Microsoft Visual Studio\\Installer\\vswhere.exe')
    if not os.path.isfile(vswhere_exe):
        vswhere_exe = ''
    return vswhere_exe


def check_cpp_platform_tools(toolsetVer, platform_tool_name, vs2017vswhereOptions):
    """
    Check the cpp tools for a platform based on the presence of a platform specific 'Platform.props' folder
    """

    try:
        if toolsetVer == '15':
            vswhere_exe = find_vswhere()
            if vswhere_exe == '':
                return False

            installation_path = subprocess.check_output([vswhere_exe, '-property', 'installationPath'] + vs2017vswhereOptions)
            if not installation_path:
                try:
                    version_arg_index = vs2017vswhereOptions.index('-version')
                    Logs.warn('[WARN] VSWhere could not find an installed version of Visual Studio matching the version requirements provided (-version {}). Attempting to fall back on any available installed version.'.format(vs2017vswhereOptions[version_arg_index + 1]))
                    Logs.warn('[WARN] Lumberyard defaults the version range to the maximum version tested against before each release. You can modify the version range in the WAF user_settings\' option win_vs2017_vswhere_args under [Windows Options].')
                    del vs2017vswhereOptions[version_arg_index : version_arg_index + 2]
                    installation_path = subprocess.check_output([vswhere_exe, '-property', 'installationPath'] + vs2017vswhereOptions)
                except ValueError:
                    pass

            installation_path = installation_path[:len(installation_path)-2]
            Logs.info('[INFO] Using Visual Studio version installed at: {}'.format(installation_path))
            build_root = os.path.join(installation_path, 'MSBuild', toolsetVer+'.0')
            props_file = os.path.join(build_root, 'Microsoft.common.props')
            return os.path.exists(props_file)
        else:
            platform_root_dir = os.path.join(get_msbuild_root(toolsetVer),
                                             'Microsoft.Cpp','v4.0','V{}0'.format(toolsetVer),
                                             'Platforms',
                                             platform_tool_name)
            platform_props_file = os.path.join(platform_root_dir,'Platform.props')
            return os.path.exists(platform_props_file)
    except Exception as err:
        Logs.warn("[WARN] Unable to determine toolset path for platform vetting : {}".format(err.message))
        return True


@conf
def get_available_launchers(self, project):

    launchers_to_exclude = set()
    if not self.is_target_platform_enabled('android'):
        # If android is disabled, remove add it to the launchers to exclude
        launchers_to_exclude.add('AndroidLauncher')

    # Get the dictionary for the launchers
    available_launchers = { 'modules': [fl for fl in AVAILABLE_LAUNCHERS['modules'] if fl not in launchers_to_exclude] }

    # Get the list of all the launchers
    projects_settings = self.get_project_settings_map()
    additional_launchers = projects_settings.get(project, {}).get('additional_launchers', [])
    
    for p0, _, _, _ in self.env['RESTRICTED_PLATFORMS']:
        restricted_launcher_name = '{}Launcher'.format(p0)
        append_to_unique_list(additional_launchers, restricted_launcher_name)
        pass

    # Update the modules in the dictionary to include the additional launchers
    for additional_launcher in additional_launchers:
        if additional_launcher not in available_launchers['modules']:
            available_launchers['modules'].append(additional_launcher)

    # Check if there is a list of restricted launchers
    restricted_launchers = projects_settings.get(project, {}).get('restricted_launchers', [])

    # Remove the modules in the dictionary if there is a restricted list
    if len(restricted_launchers) > 0:
        for key in available_launchers:
            modules = available_launchers[key]
            launchers = [launcher for launcher in modules if launcher in restricted_launchers]
            available_launchers[key] = launchers

    return available_launchers


@conf
def get_project_vs_filter(self, target):
    if not hasattr(self, 'vs_project_filters'):
        self.fatal('vs_project_filters not initialized')

    if not target in self.vs_project_filters:
        self.fatal('No visual studio filter entry found for %s' % target)

    return self.vs_project_filters[target]


def _load_android_settings(ctx):
    """ Helper function for loading the global android settings """

    if hasattr(ctx, 'android_settings'):
        return

    ctx.android_settings = {}
    settings_file = ctx.engine_node.make_node(['_WAF_','android','android_settings.json'])
    try:
        ctx.android_settings = ctx.parse_json_file(settings_file)
    except Exception as e:
        ctx.cry_file_error(str(e), settings_file.abspath())


def _get_android_setting(ctx, setting, default_value = None):
    """" Helper function for getting android settings """
    _load_android_settings(ctx)

    return ctx.android_settings.get(setting, default_value)

@conf
def get_android_dev_keystore_alias(conf):
    return _get_android_setting(conf, 'DEV_KEYSTORE_ALIAS')

@conf
def get_android_dev_keystore_path(conf):
    local_path = _get_android_setting(conf, 'DEV_KEYSTORE')
    debug_ks_node = conf.engine_node.make_node(local_path)
    return debug_ks_node.abspath()

@conf
def get_android_distro_keystore_alias(conf):
    return _get_android_setting(conf, 'DISTRO_KEYSTORE_ALIAS')

@conf
def get_android_distro_keystore_path(conf):
    local_path = _get_android_setting(conf, 'DISTRO_KEYSTORE')
    release_ks_node = conf.engine_node.make_node(local_path)
    return release_ks_node.abspath()

@conf
def get_android_build_environment(conf):
    env = _get_android_setting(conf, 'BUILD_ENVIRONMENT')
    if env == 'Development' or env == 'Distribution':
        return env
    else:
        Logs.fatal('[ERROR] Invalid android build environment, valid environments are: Development and Distribution')

@conf
def get_android_env_keystore_alias(conf):
    env = conf.get_android_build_environment()
    if env == 'Development':
        return conf.get_android_dev_keystore_alias()
    elif env == 'Distribution':
        return conf.get_android_distro_keystore_alias()

@conf
def get_android_env_keystore_path(conf):
    env = conf.get_android_build_environment()
    if env == 'Development':
        return conf.get_android_dev_keystore_path()
    elif env == 'Distribution':
        return conf.get_android_distro_keystore_path()

@conf
def get_android_build_tools_version(conf):
    """
    Get the version of build-tools to use for Android APK packaging process.  Also
    sets the 'buildToolsVersion' when generating the Android Studio gradle project.
    The following is require in order for validation and use of "latest" value.

    def configure(conf):
        conf.load('android')
    """
    build_tools_version = conf.env['ANDROID_BUILD_TOOLS_VER']

    if not build_tools_version:
        build_tools_version = _get_android_setting(conf, 'BUILD_TOOLS_VER')

    return build_tools_version

@conf
def get_android_sdk_version(conf):
    """
    Gets the desired Android API version used when building the Java code,
    must be equal to or greater than the ndk_platform.  The following is
    required for the validation to work and use of "latest" value.

    def configure(conf):
        conf.load('android')
    """
    sdk_version = conf.env['ANDROID_SDK_VERSION']

    if not sdk_version:
        sdk_version = _get_android_setting(conf, 'SDK_VERSION')

    return sdk_version

@conf
def get_android_ndk_platform(conf):
    """
    Gets the desired Android API version used when building the native code,
    must be equal to or less than the sdk_version.  If not specified, the
    specified sdk version, or closest match, will be used.  The following is
    required for the auto-detection and validation to work.

    def configure(conf):
        conf.load('android')
    """
    ndk_platform = conf.env['ANDROID_NDK_PLATFORM']

    if not ndk_platform:
        ndk_platform = _get_android_setting(conf, 'NDK_PLATFORM')

    return ndk_platform

@conf
def get_android_project_relative_path(self):
    return self.options.android_studio_project_folder + '/' + self.options.android_studio_project_name

@conf
def get_android_project_absolute_path(self):
    return self.path.make_node(self.options.android_studio_project_folder + '/' + self.options.android_studio_project_name).abspath()

@conf
def get_android_patched_libraries_relative_path(self):
    return self.get_android_project_relative_path() + '/' + 'AndroidLibraries'


def _load_environment_file(ctx):
    """ Helper function for loading the environment file created by Setup Assistant """

    if hasattr(ctx, 'local_environment'):
        return

    ctx.local_environment = {}
    settings_file = ctx.get_engine_node().make_node('_WAF_').make_node('environment.json')
    try:
        ctx.local_environment = ctx.parse_json_file(settings_file)
    except Exception as e:
        ctx.cry_file_error(str(e), settings_file.abspath())


def _get_environment_file_var(ctx, env_var):
    _load_environment_file(ctx)
    return ctx.local_environment[env_var]


@conf
def get_env_file_var(conf, env_var, required = False, silent = False):

    try:
        return _get_environment_file_var(conf, env_var)
    except Exception as e:
        if not silent:
            message = 'Failed to find {} in _WAF_/environment.json: {}'.format(env_var, e)
            if required:
                Logs.error('[ERROR] %s' % message)
            else:
                Logs.warn('[WARN] %s' % message)
        pass

    return ''


def _load_specs(ctx):
    """ Helper function to find all specs stored in _WAF_/specs/*.json """
    if hasattr(ctx, 'loaded_specs_dict'):
        return

    ctx.loaded_specs_dict = {}
    spec_file_folder    = ctx.root.make_node(Context.launch_dir).make_node('/_WAF_/specs')
    spec_files              = spec_file_folder.ant_glob('**/*.json')

    for file in spec_files:
        try:
            spec = ctx.parse_json_file(file)
            spec_name = str(file).split('.')[0]
            ctx.loaded_specs_dict[spec_name] = spec
        except Exception as e:
            ctx.cry_file_error(str(e), file.abspath())

    # For any enabled game project, see if it has a WAFSpec subfolder as well
    enabled_game_projects_list = split_comma_delimited_string(ctx.options.enabled_game_projects, True)
    for project in enabled_game_projects_list:
        game_project_spec = ctx.path.make_node(project).make_node('WAFSpec').make_node('{}.json'.format(project))
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
    for (spec,entry) in ctx.loaded_specs_dict.items():
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
def is_target_enabled(ctx, target):
    """
    Check if the target is enabled for the build based on whether a spec file was specified
    """
    project_spec = ctx.options.project_spec

    # prevent sending an empty list form being passed down in the unlikely chance the keys don't exist
    platform = ctx.env['PLATFORM'] or None
    config = ctx.env['CONFIGURATION'] or None

    if is_project_spec_specified(ctx):
        return (target in ctx.spec_modules(project_spec, platform, config)) or (target in ctx.get_all_module_uses(project_spec, platform, config))
    else:
        # No spec means all projects are enabled
        return True


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
        for game_name, game_folder in spec_game_folder_map.items():
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
    for project_name, project_values in projects_settings.items():
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
