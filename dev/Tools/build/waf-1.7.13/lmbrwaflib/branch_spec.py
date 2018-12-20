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
# Description: Central configuration file for a branch, should never be
#              integrated since it is unique for each branch
#
########### Below are various getter to expose the global values ############
from waflib.Configure import conf, ConfigurationContext
from waflib import Context, Utils, Logs, Errors
from cry_utils import append_to_unique_list, split_comma_delimited_string

from waf_branch_spec import BINTEMP_FOLDER
from waf_branch_spec import CACHE_FOLDER

from waf_branch_spec import PLATFORMS
from waf_branch_spec import CONFIGURATIONS
from waf_branch_spec import MONOLITHIC_BUILDS
from waf_branch_spec import AVAILABLE_LAUNCHERS
from waf_branch_spec import LUMBERYARD_VERSION
from waf_branch_spec import LUMBERYARD_BUILD
from waf_branch_spec import CONFIGURATION_SHORTCUT_ALIASES
from waf_branch_spec import PLATFORM_CONFIGURATION_FILTER
from waf_branch_spec import ADDITIONAL_COPYRIGHT_TABLE
import json
import os
import copy
import subprocess


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


# To handle platform aliases that can map to one or more actual platforms (Above).
# Make sure to maintain and update the alias based on updates to the PLATFORMS
# dictionary
PLATFORM_SHORTCUT_ALIASES = {
    'win': [
        'win_x64_clang',
        'win_x64_vs2017',
        'win_x64_vs2015',
        'win_x64_vs2013',
    ],
    'darwin': [
        'darwin_x64',
    ],
    'android': [
        'android_armv7_gcc',
        'android_armv7_clang',
        'android_armv8_clang',
    ],
    'linux': [
        'linux_x64',
    ],
    'all': [
        'darwin_x64',
        'ios',
        'appletv',
        'win_x64_clang',
        'win_x64_vs2017',
        'win_x64_vs2015',
        'win_x64_vs2013',
        'android_armv7_gcc',
        'android_armv7_clang',
        'android_armv8_clang',
    ],

    # Compilers
    'msvc': [
        'win_x64_vs2017',
        'win_x64_vs2015',
        'win_x64_vs2013',
    ],
    'clang': [
        'win_x64_clang',
        'darwin_x64',
        'ios',
        'appletv',
        'android_armv7_clang',
        'android_armv8_clang',
    ],
    'gcc': [
        'android_armv7_gcc'
    ],
}


# Only allow specific build platforms to be included in specific versions of MSVS since not all target platforms are
# compatible with all MSVS version
MSVS_PLATFORM_VERSION = {
    'win_x64_vs2017'     : ['15'],
    'win_x64_vs2015'     : ['14'],
    'win_x64_vs2013'     : ['2013'],
    'android_armv7_clang': ['2013', '14']
}

# Table of mapping between WAF target platforms and MSVS target platforms
WAF_PLATFORM_TO_VS_PLATFORM_PREFIX_AND_TOOL_SET_DICT = {
    'win_x86'               :   ('Win32', 'win', ['v120'], 'Win32'),
    'win_x64_vs2017'        :   ('x64 vs2017', 'win', ['v141'], 'Win32'),
    'win_x64_vs2015'        :   ('x64 vs2015', 'win', ['v140'], 'Win32'),
    'win_x64_vs2013'        :   ('x64 vs2013', 'win', ['v120'], 'Win32'),
    'android_armv7_clang'   :   ('ARM', 'android', ['v120', 'v140'], 'ARM'),
}

# Helper method to reverse the waf platform to vs platform and prefix dict (WAF_PLATFORM_TO_VS_PLATFORM_AND_PREFIX_DICT)
# to a vs to waf platform and prefix dict dictionary.
def _waf_platform_dict_to_vs_dict(src_dict):

    dict_result = {}
    src_keys = src_dict.viewkeys()

    for src_key in src_keys:
        src_value_tuple = src_dict[src_key]

        if not isinstance(src_value_tuple, tuple):
            raise ValueError('Value of key entry {} must be a tuple'.format(src_key))

        src_value = src_value_tuple[0]
        if not isinstance(src_value, str):
            raise ValueError('Tuple value(0) of key entry {} must be a string'.format(src_key))
        prefix_value = src_value_tuple[1]
        if not isinstance(prefix_value, str):
            raise ValueError('Tuple value(1) of key entry {} must be a string'.format(src_key))
        toolset_value = src_value_tuple[2]
        if not isinstance(toolset_value, list):
            raise ValueError('Tuple value(2) of key entry {} must be a list'.format(src_key))

        if src_value in dict_result:
            raise ValueError('Duplicate value of {} found in source dictionary'.format(src_value))
        dict_result[src_value] = (src_key, prefix_value, list(toolset_value))

    return dict_result


# Table of MSVS target platforms to WAF platforms.  This is a reverse list of WAF_PLATFORM_TO_VS_PLATFORM_PREFIX_AND_TOOL_SET_DICT
VS_PLATFORM_TO_WAF_PLATFORM_PREFIX_AND_TOOL_SET_DICT = _waf_platform_dict_to_vs_dict(WAF_PLATFORM_TO_VS_PLATFORM_PREFIX_AND_TOOL_SET_DICT)



#############################################################################
#############################################################################
@conf
def get_bintemp_folder_node(self):
    return self.root.make_node(Context.launch_dir).make_node(BINTEMP_FOLDER)

#############################################################################
@conf
def get_cache_folder_node(self):
    return self.root.make_node(Context.launch_dir).make_node(CACHE_FOLDER)

#############################################################################
@conf
def get_dep_proj_folder_name(self, msvs_ver):
    if msvs_ver == "14":
        return self.options.visual_studio_solution_name + '_vc' + msvs_ver + '0.depproj'
    else:
        return self.options.visual_studio_solution_name + '_vs' + msvs_ver + '.depproj'

#############################################################################
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
def get_platform_alias(self, platform_key):
    """
    Return the expanded value of a (target) platform alias if any.  If no alias, return None
    """
    if platform_key in PLATFORM_SHORTCUT_ALIASES:
        return PLATFORM_SHORTCUT_ALIASES[platform_key]
    else:
        return None

#############################################################################
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


#############################################################################
#############################################################################


@conf
def get_supported_platforms(self):
    """
    Deprecated in favor of get_available_platforms()
    """
    return self.get_available_platforms()


@conf
def mark_supported_platform_for_removal(conf, platform_to_remove):
    """
    Deprecated in favor of remove_platform_from_available_platforms()
    """
    conf.remove_platform_from_available_platforms(platform_to_remove)


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
                    Logs.warn('[WARN] VSWhere could not find an installed version of Visual Studio matching the version requirements provided (-version {}). Attempting to fall back on any available installed version.'.format(vs_where_args[version_arg_index + 1]))
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
def check_msvs_compatibility(self, waf_platform, version):
    """
    Determine if a waf_platform is compatible with a version of MSVS
    """
    if not waf_platform in MSVS_PLATFORM_VERSION:
        return False
    if not version in MSVS_PLATFORM_VERSION[waf_platform]:
        return False

    # Additionally filter out platforms that do not have a valid installed toolset
    msvs_toolset_name = WAF_PLATFORM_TO_VS_PLATFORM_PREFIX_AND_TOOL_SET_DICT[waf_platform][3]

    if not check_cpp_platform_tools(self.msvs_version, msvs_toolset_name, self.options.win_vs2017_vswhere_args.split()):
        Logs.warn("[WARN] Skipping platform '{}' because it is not installed "
                  "properly for this version of visual studio".format(waf_platform))
        return False

    else:
        return True


#############################################################################
@conf
def get_supported_configurations(self, platform=None):
    return PLATFORM_CONFIGURATION_FILTER.get(platform, CONFIGURATIONS)


#############################################################################
@conf
def get_available_launchers(self, project):
    # Get the dictionary for the launchers
    available_launchers = copy.deepcopy(AVAILABLE_LAUNCHERS)

    # Get the list of all the launchers
    projects_settings = self.get_project_settings_map()
    additional_launchers = projects_settings.get(project, {}).get('additional_launchers', [])

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


#############################################################################
#############################################################################
@conf
def get_project_vs_filter(self, target):
    if not hasattr(self, 'vs_project_filters'):
        self.fatal('vs_project_filters not initialized')

    if not target in self.vs_project_filters:
        self.fatal('No visual studio filter entry found for %s' % target)

    return self.vs_project_filters[target]

#############################################################################
#############################################################################
def _load_android_settings(ctx):
    """ Helper function for loading the global android settings """

    if hasattr(ctx, 'android_settings'):
        return

    ctx.android_settings = {}
    settings_file = ctx.root.make_node(Context.launch_dir).make_node([ '_WAF_', 'android', 'android_settings.json' ])
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
    debug_ks_node = conf.root.make_node(Context.launch_dir).make_node(local_path)
    return debug_ks_node.abspath()

@conf
def get_android_distro_keystore_alias(conf):
    return _get_android_setting(conf, 'DISTRO_KEYSTORE_ALIAS')

@conf
def get_android_distro_keystore_path(conf):
    local_path = _get_android_setting(conf, 'DISTRO_KEYSTORE')
    release_ks_node = conf.root.make_node(Context.launch_dir).make_node(local_path)
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

#############################################################################
#############################################################################
def _load_environment_file(ctx):
    """ Helper function for loading the environment file created by Setup Assistant """

    if hasattr(ctx, 'local_environment'):
        return

    ctx.local_environment = {}
    settings_file = ctx.root.make_node(Context.launch_dir).make_node([ '_WAF_', 'environment.json' ])
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
    except:
        if not silent:
            message = 'Failed to find %s in _WAF_/environment.json.' % env_var
            if required:
                Logs.error('[ERROR] %s' % message)
            else:
                Logs.warn('[WARN] %s' % message)
        pass

    return ''

#############################################################################
#############################################################################

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
    """ Util function to load a specifc spec entry (always returns a list) """

    cache_key = '{}_{}_{}_{}'.format(entry, spec_name, platform, configuration)
    if cache_key in spec_entry_cache:
        return spec_entry_cache[cache_key]

    _load_specs(ctx)

    if not spec_name: # Fall back to command line specified spec if none was given
        spec_name = ctx.options.project_spec

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

    # The options context is missing an env attribute, hence don't try to find platform settings in this case
    if hasattr(ctx, 'env'):
        if not platform:
            platform = ctx.env['PLATFORM']
        if not configuration:
            configuration = ctx.env['CONFIGURATION']
        res += ctx.GetPlatformSpecificSettings(spec, entry, platform, configuration)

    spec_entry_cache[cache_key] = res

    return res

spec_modules_cache = {}
@conf
def spec_modules(ctx, spec_name = None, platform=None, configuration=None):
    """ Get all a list of all modules needed for this spec """

    cache_key = '{}_{}_{}'.format(spec_name, platform, configuration)
    if cache_key in spec_modules_cache:
        return spec_modules_cache[cache_key]

    module_list = _spec_entry(ctx, 'modules', spec_name)
    if platform is not None:
        platform_filtered_module_list = [module for module in module_list if platform in ctx.get_module_platforms(module)]
        module_list = platform_filtered_module_list
    if configuration is not None:
        configuration_filtered_module_list = [module for module in module_list if configuration in ctx.get_module_configurations(module, platform)]
        module_list = configuration_filtered_module_list

    spec_modules_cache[cache_key] = module_list
    return module_list


@conf
def is_project_spec_specified(ctx):
    return len(ctx.options.project_spec) > 0


@conf
def is_target_enabled(ctx, target):
    """
    Check if the target is enabled for the build based on whether a spec file was specified
    """
    if is_project_spec_specified(ctx):
        return (target in ctx.spec_modules(ctx.options.project_spec)) or (target in ctx.get_all_module_uses(ctx.options.project_spec))
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
def spec_monolithic_build(ctx, spec_name = None):
    """ Return true if the current platform|configuration should be a monolithic build in this spec """
    if not spec_name: # Fall back to command line specified spec if none was given
        spec_name = ctx.options.project_spec

    configuration = ctx.env['CONFIGURATION']
    platform = ctx.get_platform_list(ctx.env['PLATFORM'])

    is_monolithic = ctx.is_variant_monolithic(platform, configuration)
    Logs.debug("lumberyard: spec_monolithic_build(" + str(platform) + spec_name + ") == {}".format(str(is_monolithic)))

    return is_monolithic


@conf
def is_variant_monolithic(ctx, platform, configuration):
    """
    Check to see if any variation of the desired platform and configuration is intended
    to be built as monolithic
    """
    current_plats = platform
    if isinstance(current_plats, str):
        current_plats = ctx.get_platform_list(current_plats)

    current_confs = configuration
    if isinstance(current_confs, str):
        current_confs = [current_confs] # it needs to be a list

    # Check for entry in <platform> style
    variants = [plat.strip() for plat in current_plats]

    # Check for entry in <configuration>
    variants += [config.strip() for config in current_confs]

    # Check for entry in <platform>_<configuration> style
    variants += ['{}_{}'.format(plat.strip(), config.strip()) for plat in current_plats for config in current_confs]

    return len(set(variants).intersection(MONOLITHIC_BUILDS)) > 0


@conf
def is_cmd_monolithic(ctx):
    '''
    Determine if a build command is tagged as a monolithic build
    '''
    for monolithic_key in MONOLITHIC_BUILDS:
        if monolithic_key in ctx.cmd:
            return True

    return False


# Set global output directory
setattr(Context.g_module, Context.OUT, BINTEMP_FOLDER)

# List of platform specific envs that will be filtered out in the env cache
ELIGIBLE_PLATFORM_SPECIFIC_ENV_KEYS = [ 'INCLUDES',
                                        'FRAMEWORKPATH',
                                        'DEFINES',
                                        'CPPFLAGS',
                                        'CCDEPS',
                                        'CFLAGS',
                                        'ARCH',
                                        'CXXDEPS',
                                        'CXXFLAGS',
                                        'DFLAGS',
                                        'LIB',
                                        'STLIB',
                                        'LIBPATH',
                                        'STLIBPATH',
                                        'LINKFLAGS',
                                        'RPATH',
                                        'LINKDEPS',
                                        'FRAMEWORK',
                                        'ARFLAGS',
                                        'ASFLAGS']


@conf
def preprocess_platform_list(ctx, platforms, auto_populate_empty=False):
    """
    Preprocess a list of platforms from user input
    """
    host_platform = Utils.unversioned_sys_platform()
    processed_platforms = set()
    if (auto_populate_empty and len(platforms) == 0) or 'all' in platforms:
        for platform in PLATFORMS[host_platform]:
            processed_platforms.add(platform)
    else:
        for platform in platforms:
            if platform in PLATFORMS[host_platform]:
                processed_platforms.add(platform)
            else:
                aliased_platforms = ctx.get_platform_alias(platform)
                if aliased_platforms:
                    for aliased_platform in aliased_platforms:
                        processed_platforms.add(aliased_platform)
    return processed_platforms

@conf
def preprocess_configuration_list(ctx, target, target_platform, configurations, auto_populate_empty=False):
    """
    Preprocess a list of configurations based on an optional target_platform filter from user input
    """

    processed_configurations = set()
    if (auto_populate_empty and len(configurations) == 0) or 'all' in configurations:
        for configuration in CONFIGURATIONS:
            processed_configurations.add(configuration)
    else:
        for configuration in configurations:
            # Matches an existing configuration
            if configuration in CONFIGURATIONS:
                processed_configurations.add(configuration)
            # Matches an alias
            elif configuration in CONFIGURATION_SHORTCUT_ALIASES:
                for aliased_configuration in CONFIGURATION_SHORTCUT_ALIASES[configuration]:
                    processed_configurations.add(aliased_configuration)
            elif ':' in configuration and target_platform is not None:
                # Could be a platform-only configuration, split into components
                configuration_parts = configuration.split(':')
                split_platform = configuration_parts[0]
                split_configuration = configuration_parts[1]

                # Apply the configuration alias here as well
                if split_configuration in CONFIGURATION_SHORTCUT_ALIASES:
                    split_configurations = CONFIGURATION_SHORTCUT_ALIASES[split_configuration]
                elif split_configuration in CONFIGURATIONS:
                    split_configurations = [split_configuration]
                else:
                    split_configurations = []
                    if target:
                        Logs.warn('[WARN] configuration value {} in module {} is invalid.'.format(configuration, target))
                    else:
                        Logs.warn('[WARN] configuration value {}.'.format(configuration))

                # Apply the platform alias here as well
                split_platforms = preprocess_platform_list(ctx,[split_platform])

                # Iterate through the platform(s) and add the matched configurations
                for split_platform in split_platforms:
                    if split_platform == target_platform:
                        for split_configuration in split_configurations:
                            processed_configurations.add(split_configuration)
            else:
                if target:
                    Logs.warn('[WARN] configuration value {} in module {} is invalid.'.format(configuration, target))
                else:
                    Logs.warn('[WARN] configuration value {}.'.format(configuration))

    return processed_configurations

VC120_CAPABILITY = ('vc120','Visual Studio 2013')
VC140_CAPABILITY = ('vc140','Visual Studio 2015')
VC141_CAPABILITY = ('vc141','Visual Studio 2017')
ANDROID_CAPABILITY = ('compileandroid','Compile For Android')
IOS_CAPABILITY = ('compileios','Compile for iOS')

PLATFORM_TO_CAPABILITY_MAP = {
    'win_x64_clang':        VC140_CAPABILITY,
    'win_x64_vs2017':       VC141_CAPABILITY,
    'win_x64_vs2015':       VC140_CAPABILITY,
    'win_x64_vs2013':       VC120_CAPABILITY,
    'android_armv7_gcc':    ANDROID_CAPABILITY,
    'android_armv7_clang':  ANDROID_CAPABILITY,
    'android_armv8_clang':  ANDROID_CAPABILITY,
    'ios':                  IOS_CAPABILITY,
    'appletv':              IOS_CAPABILITY,
}

VALIDATED_CONFIGURATIONS_FILENAME = "valid_configuration_platforms.json"
AVAILABLE_PLATFORMS = None


@conf
def get_validated_platforms_node(conf):
    return conf.get_bintemp_folder_node().make_node(VALIDATED_CONFIGURATIONS_FILENAME)


@conf
def get_available_platforms(conf):

    is_configure_context = isinstance(conf, ConfigurationContext)
    validated_platforms_node = conf.get_validated_platforms_node()
    validated_platforms_json = conf.parse_json_file(validated_platforms_node) if os.path.exists(validated_platforms_node.abspath()) else None

    global AVAILABLE_PLATFORMS
    if AVAILABLE_PLATFORMS is None:

        # Get all of the available target platforms for the current host platform
        host_platform = Utils.unversioned_sys_platform()

        # fallback option for android if the bootstrap params is empty
        android_enabled_var = conf.get_env_file_var('ENABLE_ANDROID', required=False, silent=True)
        android_enabled = (android_enabled_var == 'True')

        # Check the enabled capabilities from the bootstrap parameter.  This value is set by the setup assistant
        enabled_capabilities = conf.get_enabled_capabilities()
        validated_platforms = []
        for platform in PLATFORMS[host_platform]:

            platform_capability = PLATFORM_TO_CAPABILITY_MAP.get(platform,None)
            if platform_capability is not None:
                if len(enabled_capabilities) > 0 and platform_capability[0] not in enabled_capabilities:
                    # Only log the target platform removal during the configure process
                    if isinstance(conf, ConfigurationContext):
                        Logs.info('[INFO] Removing target platform {} due to "{}" not checked in Setup Assistant.'
                                  .format(platform, platform_capability[1]))
                    continue

            # Perform extra validation of platforms that can be disabled through options
            if platform.startswith('android') and not android_enabled:
                continue

            if platform.endswith('clang') and platform.startswith('win'):
                if not conf.is_option_true('win_enable_clang_for_windows'):
                    continue
                elif not conf.find_program('clang', mandatory=False, silent_output=True):
                    Logs.warn('[INFO] Removing target platform {}. Could not find Clang for Windows executable.'.format(platform))
                    continue

            if not is_configure_context and validated_platforms_json:
                if platform not in validated_platforms_json:
                    continue

            validated_platforms.append(platform)

        AVAILABLE_PLATFORMS = validated_platforms
    return AVAILABLE_PLATFORMS


@conf
def remove_platform_from_available_platforms(conf, platform_to_remove):

    current_available_platforms = conf.get_available_platforms()
    updated_available_platforms = []
    for current_available_platform in current_available_platforms:
        if current_available_platform != platform_to_remove:
            updated_available_platforms.append(current_available_platform)

    global AVAILABLE_PLATFORMS
    AVAILABLE_PLATFORMS = updated_available_platforms


@conf
def update_validated_platforms(conf):

    current_available_platforms = conf.get_available_platforms()
    validated_platforms_node = conf.get_validated_platforms_node()

    # write out the list of platforms that were successfully configured
    json_data = json.dumps(current_available_platforms)
    try:
        validated_platforms_node.write(json_data)
    except Exception as e:
        # If the file cannot be written to, warn, but dont fail.
        Logs.warn("[WARN] Unable to update validated configurations file '{}' ({})".format(validated_platforms_node.abspath(),e.message))
