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
from waflib.Configure import conf
from waflib import Context
from waflib import Logs
from cry_utils import split_comma_delimited_string, append_to_unique_list
import re
import os

PROJECT_SETTINGS_FILE = 'project.json'
PROJECT_NAME_FIELD = 'project_name'

def _load_project_settings(ctx):
    """
    Util function to load the <engine root>/<project name>/project.json file and cache it within the build context.
    """

    if hasattr(ctx, 'projects_settings'):
        return

    engine_root = ctx.root.make_node(Context.launch_dir)

    if os.path.exists(engine_root.make_node('_WAF_').make_node('projects.json').abspath()):
        Logs.warn('projects.json file is deprecated.  Please follow the migration step listed in the release notes.')

    projects_settings = {}
    for project_settings_node in engine_root.ant_glob('*/' + PROJECT_SETTINGS_FILE):
        project_json = ctx.parse_json_file(project_settings_node)
        Logs.debug('lumberyard:  _load_project_settings examining project file %s...' % (project_settings_node.abspath()))
        project_name = project_json.get(PROJECT_NAME_FIELD, '').encode('ASCII', 'ignore')
        if not project_name:
            project_name = project_settings_node.parent.name
            ctx.fatal('"%s" attribute was not found in %s' % (PROJECT_NAME_FIELD, project_settings_node.abspath()))

        if projects_settings.has_key(project_name):
            ctx.fatal('Another project named "%s" has been detected:\n%s\n%s' % (
                    project_name,
                    project_settings_node.parent.abspath(),
                    projects_settings[project_name]['project_node'].abspath()
                ))

        project_json['project_node'] = project_settings_node.parent
        projects_settings[project_name] = project_json
        Logs.debug('lumberyard: project file %s is for project %s' % (project_settings_node.abspath(), project_name))

    ctx.projects_settings = projects_settings

    _validate_project_settings(ctx)

def _validate_project_settings(ctx):
    enabled_game_projects = list(ctx.get_enabled_game_project_list())
        
    # also ensure that the bootstrap.cfg points at an enabled game project:
    if ctx.get_bootstrap_game() not in ctx.projects_settings:
        ctx.fatal('bootstrap.cfg has the enabled game project folder "%s", but this game project was not found '
                  'in the spec file.  Please ensure that the project folder configured in your bootstrap.cfg actually '
                  'exists (check for typos), and is also configured for the spec you are using for compilation.\n' % (ctx.get_bootstrap_game()))
    for project_name in ctx.projects_settings:
        if project_name in enabled_game_projects:
            enabled_game_projects.remove(project_name)

    # the only game projects remaining in enabled_game_projects will be ones which don't have project settings files.
    if enabled_game_projects:
        error_message = ''
        for project in enabled_game_projects:
            error_message += 'Enabled project "%s" was not found. Check if "%s" file exists for the project.\n' % (project, PROJECT_SETTINGS_FILE)
        ctx.fatal(error_message)

def _project_setting_entry(ctx, project, entry, required=True):
    """
    Util function to load an entry from the projects.json file
    """
    _load_project_settings(ctx)

    if not project in ctx.projects_settings:
        # Most functions in this file end up calling _project_setting_entry, so just put the project settings path in so we don't end up in an endless loop.
        ctx.cry_file_error('Cannot find project entry for "%s"' % project, PROJECT_SETTINGS_FILE )

    if not entry in ctx.projects_settings[project]:
        if required:
            ctx.cry_file_error('Cannot find entry "%s" for project "%s"' % (entry, project), PROJECT_SETTINGS_FILE)
        else:
            return None

    return ctx.projects_settings[project][entry]

#############################################################################
@conf
def game_projects(self):
    _load_project_settings(self)

    # Build list of game code folders
    projects = []
    for key, value in self.projects_settings.items():
            projects += [ key ]

    # do not filter this list down to only enabled projects!
    return projects

@conf
def project_idx (self, project):
    _load_project_settings(self)
    enabled_projects = self.get_enabled_game_project_list()
    nIdx = 0
    for key, value in self.projects_settings.items():
        if key == project:
            return nIdx
        nIdx += 1
    return nIdx;

#############################################################################
@conf
def get_project_node(self, project):
    return _project_setting_entry(self, project, 'project_node')

@conf
def get_project_settings_node(ctx, project):
    return ctx.get_project_node(project).make_node(PROJECT_SETTINGS_FILE)

@conf
def legacy_game_code_folder(self, project):
    return _project_setting_entry(self, project, 'code_folder', False)

@conf
def game_gem(self, project):

    enabled_game_gem = None
    configured_gems = self.get_game_gems(project)
    for configured_gem in configured_gems:
        if configured_gem.is_game_gem:
            if enabled_game_gem is None:
                enabled_game_gem = configured_gem
            else:
                self.fatal("[ERROR] Project {} has more than 1 game gem enabled ({},{}).  Only one is allowed per project."
                           .format(project, configured_gem.path, enabled_game_gem.path))
    if enabled_game_gem is None:
        self.fatal("[ERROR] Project {} does not have an enabled game gem.  Please enable one for this project.".format(project))

    return enabled_game_gem.path

@conf
def game_code_folder(conf, project):

    # Legacy game folder takes priority
    code_folder = _project_setting_entry(conf, project, 'code_folder', False)
    if code_folder is not None:
        return code_folder

    game_gem_path = game_gem(conf, project)
    if game_gem_path is not None:
        return game_gem_path

    conf.fatal('[ERROR] Invalid project entry for "{}".  Missing either "code_folder" or the project does not have an enabled game gem.'.format(project))
    return None


@conf
def get_executable_name(self, project):
    return _project_setting_entry(self, project, 'executable_name')

@conf
def get_dedicated_server_executable_name(self, project):
    return _project_setting_entry(self, project, 'executable_name') + '_Server'

@conf
def project_output_folder(self, project):
    return _project_setting_entry(self, project, 'project_output_folder')

#############################################################################
@conf
def get_launcher_product_name(self, game_project):
    return _project_setting_entry(self, game_project, 'product_name')

#############################################################################
def _get_android_settings_option(ctx, project, option, default = None):
    settings = ctx.get_android_settings(project)
    if settings == None:
        return None;

    return settings.get(option, default)

@conf
def get_android_settings(self, project):
    try:
        return _project_setting_entry(self, project, 'android_settings')
    except:
        pass
    return None

@conf
def get_android_package_name(self, project):
    return _get_android_settings_option(self, project, 'package_name', default = 'com.lumberyard.sdk')

@conf
def get_android_version_number(self, project):
    return str(_get_android_settings_option(self, project, 'version_number', default = 1))

@conf
def get_android_version_name(self, project):
    return _get_android_settings_option(self, project, 'version_name', default = '1.0.0.0')

@conf
def get_android_orientation(self, project):
    return _get_android_settings_option(self, project, 'orientation', default = 'landscape')

@conf
def get_android_app_icons(self, project):
    return _get_android_settings_option(self, project, 'icons', default = None)

@conf
def get_android_app_splash_screens(self, project):
    return _get_android_settings_option(self, project, 'splash_screen', default = None)

@conf
def get_android_app_public_key(self, project):
    return _get_android_settings_option(self, project, 'app_public_key', default = "NoKey")

@conf
def get_android_app_obfuscator_salt(self, project):
    return _get_android_settings_option(self, project, 'app_obfuscator_salt', default = '')

@conf
def get_android_use_main_obb(self, project):
    return _get_android_settings_option(self, project, 'use_main_obb', default = 'false')

@conf
def get_android_use_patch_obb(self, project):
    return _get_android_settings_option(self, project, 'use_patch_obb', default = 'false')

@conf
def get_android_enable_keep_screen_on(self, project):
    return _get_android_settings_option(self, project, 'enable_keep_screen_on', default = 'false')

@conf
def get_android_rc_pak_job(self, project):
    return _get_android_settings_option(self, project, 'rc_pak_job', default = 'RcJob_Generic_MakePaks.xml')

@conf
def get_android_rc_obb_job(self, project):
    return _get_android_settings_option(self, project, 'rc_obb_job', default = 'RCJob_Generic_Android_MakeObb.xml')


#############################################################################
@conf
def get_ios_settings(self, project):
    try:
        return _project_setting_entry(self, project, 'ios_settings')
    except:
        pass
    return None

@conf
def get_ios_app_icon(self, project):
    settings = self.get_ios_settings(project)
    if settings == None:
        return 'AppIcon'
    return settings.get('app_icon', None)

@conf
def get_ios_launch_image(self, project):
    settings = self.get_ios_settings(project)
    if settings == None:
        return 'LaunchImage'
    return settings.get('launch_image', None)

#############################################################################
@conf
def get_appletv_settings(self, project):
    try:
        return _project_setting_entry(self, project, 'appletv_settings')
    except:
        pass
    return None

@conf
def get_appletv_app_icon(self, project):
    settings = self.get_appletv_settings(project)
    if settings == None:
        return 'AppIcon'
    return settings.get('app_icon', None)

@conf
def get_appletv_launch_image(self, project):
    settings = self.get_appletv_settings(project)
    if settings == None:
        return 'LaunchImage'
    return settings.get('launch_image', None)

#############################################################################
@conf
def get_mac_settings(self, project):
    try:
        return _project_setting_entry(self, project, 'mac_settings')
    except:
        pass
    return None

@conf
def get_mac_app_icon(self, project):
    settings = self.get_mac_settings(project)
    if settings == None:
        return 'AppIcon'
    return settings.get('app_icon', None)

@conf
def get_mac_launch_image(self, project):
    settings = self.get_mac_settings(project)
    if settings == None:
        return 'LaunchImage'
    return settings.get('launch_image', None)

#############################################################################
@conf
def get_dedicated_server_product_name(self, game_project):
    return _project_setting_entry(self,  game_project, 'product_name') + ' Dedicated Server'

#############################################################################

BUILD_SPECS = ['gems', 'all', 'game_and_engine', 'dedicated_server', 'pipeline', 'resource_compiler', 'shadercachegen']
GAMEPROJECT_DIRECTORIES_LIST = []

@conf
def project_modules(self, project):
    # Using None as a hint to the method that we want the method to get the
    # platfom list itself
    return self.project_and_platform_modules(project, None)

@conf
def project_and_platform_modules(self, project, platforms):
    '''Get the modules for the project that also include modules for the
       specified platforms. The platforms parameter can be either a list of
       platforms to check for, a single platform name, or None to indicate that
       the current supported platforms should be used.'''

    base_modules = _project_setting_entry(self, project, 'modules')

    platform_modules = []
    supported_platforms = [];

    if platforms is None:
        (current_platform, configuration) = self.get_platform_and_configuration()
        supported_platforms = self.get_platform_list(current_platform)
    elif not isinstance(platforms, list):
        supported_platforms.append(platforms)
    else:
        supported_platforms = platforms

    for platform in supported_platforms:
        platform_modules_string = platform + '_modules'
    if platform_modules_string in self.projects_settings[project]:
            platform_modules = platform_modules + _project_setting_entry(self, project, platform_modules_string)

    return base_modules + platform_modules

# Deprecated modules that may be in legacy flavor configurations but have since been deprecated
DEPRECATED_FLAVOR_MODULES = ('LmbrCentral','LmbrCentralEditor')
    
@conf
def project_flavor_modules(self, project, flavor):
    """
    Return modules used by the named flavor (ex: 'Game', 'Editor').
    If these entries are not defined, then default values are used.
    """
    flavors_entry = _project_setting_entry(self, project, 'flavors', False)
    if flavors_entry:
        if flavor in flavors_entry:
            flavor_entry = flavors_entry[flavor]
            if 'modules' in flavor_entry:
                flavor_modules = flavor_entry['modules']
                # Filter out flavor modules that are deprecated and shouldnt be considered (ie modules that were converted into Gems)
                filtered_flavor_modules = [flavor_module for flavor_module in flavor_modules if flavor_module not in DEPRECATED_FLAVOR_MODULES]
                return filtered_flavor_modules
        else:
            self.cry_file_error('Cannot find entry "flavors.%s" for project "%s"' % (flavor, project), get_project_settings_node(self, project).abspath())
    
    return []

@conf
def add_game_projects_to_specs(self):
    """Add game projects to specs that have them defined"""

    specs_to_include = self.loaded_specs()

    available_launchers = self.get_available_launchers()

    for spec_name in specs_to_include:

        # Get the defined game project per spec and only add game projects
        # to specs that have them defined
        game_projects = self.spec_game_projects(spec_name)

        if len(game_projects) == 0 and not self.spec_disable_games(spec_name):
            # Handle the legacy situation where the game projects is set in the enabled_game_projects in user_settings.options
            game_projects = split_comma_delimited_string(self.options.enabled_game_projects,True)

        # Skip if there are no game projects for this spec
        if len(game_projects) == 0:
            continue

        # Add both the game modules for the project (legacy) and all the launchers
        spec_dict = self.loaded_specs_dict[spec_name]
        if 'modules' not in spec_dict:
            spec_dict['modules'] = []
        if 'projects' not in spec_dict:
            spec_dict['projects'] = []
        spec_list = spec_dict['modules']
        spec_proj = spec_dict['projects']

        for project in game_projects:
            spec_proj.append(project)

            # Add any additional modules from the project's project.json configuration
            for module in project_modules(self, project):
                if not module in spec_list:
                    spec_list.append(module)
                    Logs.debug("lumberyard: Added module to spec list: %s for %s" % (module, spec_name))

            # if we have game projects, also allow the building of the launcher from templates:
            for available_launcher_spec in available_launchers:
                if available_launcher_spec not in spec_dict:
                    spec_dict[available_launcher_spec] = []
                spec_list_to_append_to = spec_dict[available_launcher_spec]
                available_spec_list = available_launchers[available_launcher_spec]
                for module in available_spec_list:
                    launcher_name = project + module
                    Logs.debug("lumberyard: Added launcher %s for %s (to %s spec in in %s sub_spec)" % (launcher_name, project, spec_name, available_launcher_spec))
                    spec_list_to_append_to.append(launcher_name)

#############################################################################
@conf
def get_product_name(self, target, game_project):
    if target == 'PCLauncher' or target == 'OrbisLauncher' or target == 'DurangoLauncher': # ACCEPTED_USE
        return self.get_launcher_product_name(game_project)
    elif target == 'DedicatedLauncher':
        return self.get_dedicated_server_product_name(game_project)
    else:
        return target

#############################################################################

@conf
def get_enabled_game_project_list(self):
    """Utility function which returns the current game projects."""

    # Get either the current spec being built (build) or all the specs for the solution generation (configure/msvs)
    current_spec = getattr(self.options, 'project_spec', '')
    if len(current_spec)==0:
        spec_string_list = getattr(self.options, 'specs_to_include_in_project_generation', '').strip()
        if len(spec_string_list) == 0:
            self.fatal("[ERROR] Missing/Invalid specs ('specs_to_include_in_project_generation') in user_settings.options")
        spec_list = [spec.strip() for spec in spec_string_list.split(',')]
        if len(spec_list) == 0:
            self.fatal("[ERROR] Empty spec list ('specs_to_include_in_project_generation') in user_settings.options")
    else:
        spec_list = [current_spec]

    # Vet the list and make sure all of the specs are valid specs
    for spec in spec_list:
        if not self.is_valid_spec_name(spec):
            self.fatal("[ERROR] Invalid spec '{}'.  Make sure it exists in the specs folder and is a valid spec file.".format(spec))

    # Build up the list of project names
    ordered_unique_list = list()
    for spec in spec_list:
        # Get the projects defined for the particular spec
        projects_for_spec = self.spec_game_projects(spec)

        # If there are no games defined for the spec, and it is not marked with 'disable_game_projects',
        # use the legacy method of getting the enabled game project
        if len(projects_for_spec) == 0 and not self.spec_disable_games(spec):
            projects_for_spec = split_comma_delimited_string(self.options.enabled_game_projects, True)

        # Build up the list
        for project_for_spec in projects_for_spec:
            if len(project_for_spec) > 0:
                append_to_unique_list(ordered_unique_list, project_for_spec)

    # Make sure that the game that is set bootstrap.cfg is specified as an enabled game project, otherwise produce a warning if we
    # have specified at least one game project during build commands
    if len(ordered_unique_list) > 0 and self.cmd.startswith('build_'):
        bootstrap_game = self.get_bootstrap_game()
        bootstrap_game_enabled = False
        for enabled_game in ordered_unique_list:
            if enabled_game == bootstrap_game:
                bootstrap_game_enabled = True
        if not bootstrap_game_enabled:
            if len(ordered_unique_list) > 1:
                self.warn_once("Game project '{}' configured in bootstrap.cfg is not an enabled game for this build.  "
                               "In order to run or debug for the any of the enabled games '{}', one of them needs to be set in bootstrap.cfg under the "
                               "'sys_game_folder' entry accordingly".format(bootstrap_game, ','.join(ordered_unique_list)))
            else:
                self.warn_once("Game project '{}' configured in bootstrap.cfg is the enabled game for this build.  "
                               "In order to run or debug for the game '{}', they need to be set in bootstrap.cfg under the "
                               "'sys_game_folder' entry accordingly".format(bootstrap_game, ordered_unique_list[0]))

    return ordered_unique_list


@conf
def get_bootstrap_game(self, default_game='SamplesProject'):
    """
    :param self:            Context
    :param default_game:    Default game to set to if we cannot read bootstrap.cfg.
    :return: Name of the game enabled in bootstrap.cfg
    """
   
    game = default_game
    project_folder_node = getattr(self,'srcnode',self.path)
    bootstrap_cfg = project_folder_node.make_node('bootstrap.cfg')
    bootstrap_contents = bootstrap_cfg.read()
    try:
        game = re.search('^\s*sys_game_folder\s*=\s*(\w+)', bootstrap_contents, re.MULTILINE).group(1)
    except:
        pass
    return game

@conf
def get_bootstrap_vfs(self):
    """
    :param self:
    :return: if vfs is enabled in bootstrap.cfg
    """
    project_folder_node = getattr(self,'srcnode',self.path)
    bootstrap_cfg = project_folder_node.make_node('bootstrap.cfg')
    bootstrap_contents = bootstrap_cfg.read()
    vfs = '0'
    try:
        vfs = re.search('^\s*remote_filesystem\s*=\s*(\w+)', bootstrap_contents, re.MULTILINE).group(1)
    except:
        pass
    return vfs

GAME_PLATFORM_MAP = {
}

@conf
def get_bootstrap_assets(self, platform=None):
    """
    :param self:
    :param platform: optional, defaults to current build's platform
    :return: Asset type requested for the supplied platform in bootstrap.cfg
    """
    if platform is None:
        platform = self.env['PLATFORM']
    bootstrap_cfg = self.path.make_node('bootstrap.cfg')
    bootstrap_contents = bootstrap_cfg.read()
    assets = 'pc'
    game_platform = GAME_PLATFORM_MAP.get(platform, platform)
    try:
        assets = re.search('^\s*assets\s*=\s*(\w+)', bootstrap_contents, re.MULTILINE).group(1)
        assets = re.search('^\s*%s_assets\s*=\s*(\w+)' % (game_platform), bootstrap_contents, re.MULTILINE).group(1)
    except:
        pass

    return assets
