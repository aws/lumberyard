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
import os
import re
import uuid
import glob
import utils
from ConfigParser import RawConfigParser
from waflib.Configure import conf, ConfigurationContext
from waflib import Errors, Logs
import cry_utils
from waflib.TaskGen import feature, before_method, after_method


GEMS_UUID_KEY = "Uuid"
GEMS_VERSION_KEY = "Version"
GEMS_PATH_KEY = "Path"
GEMS_LIST_FILE = "gems.json"
GEMS_DEFINITION_FILE = "gem.json"
GEMS_CODE_FOLDER = 'Code'
GEMS_FOLDER = 'Gems'
GEMS_FORMAT_VERSION = 2

BUILD_SPECS = ['gems', 'all', 'game_and_engine', 'dedicated_server', 'pipeline', 'resource_compiler', 'shadercachegen']
"""Build specs to add Gems to"""

manager = None

def _create_field_reader(ctx, obj, parse_context):
    '''
    Creates a field reader for reading from Json
    Calls ctx.cry_error() if a field not found (except opt variants).

    Args:
        ctx: the Context object to call cry_error() on failure
        obj: the object to read from
        parse_context: string describing the current context, should work following the word 'in'
    Returns:
        A Reader object, used for reading from a JSON object
    '''
    class Reader(object):
        def field_opt(self, field_name, default=None):
            return obj.get(field_name, default)

        def field_req(self, field_name):
            val = self.field_opt(field_name)
            if val != None: # != None required because [] is valid, but triggers false
                return val
            else:
                ctx.cry_error('Failed to read field {} in {}.'.format(
                    field_name, parse_context))
                return None

        def field_int(self, field_name):
            val_str = self.field_req(field_name)
            val = None
            try:
                val = int(val_str)
            except ValueError as err:
                ctx.cry_error('Failed to parse {} field with value {} in {}: {}'.format(
                    field_name, val, parse_context, err.message))
            return val

        def uuid(self, field_name = GEMS_UUID_KEY):
            '''
            Attempt to parse the field into a UUID.
            '''
            id_str = self.field_req(field_name)
            id = None
            try:
                id = uuid.UUID(hex=id_str)
            except ValueError as err:
                ctx.cry_error('Failed to parse {} field with value {} in {}: {}.'.format(
                    field_name, id_str, parse_context, err.message))
            return id

        def version(self, field_name = GEMS_VERSION_KEY):
            ver_str = self.field_req(field_name)
            return Version(ctx, ver_str)

    return Reader()


GEM_NAME_BY_ID = {}     # Cache of the gem name by its id
BAD_GEM_PATH = "____"   # Special case to handle caching of bad gem.json config files
GEM_NAME_SEARCH_SUBFOLDERS = ('Gems','Code')   # The subfolders under the root folder to search for gems


def find_gem_name_by_id(ctx, input_gem_id):
    """
    Helper function to scan and search for a gem name based on a gem uuid.  The gem manager
    will only load gems that are configured for the game project(s), even if there are more
    that exists in the project.
    This only scans for gems under the Gems subfolder
    :param ctx:             Configuration Context
    :param input_gem_id:    The input gem-id to lookup the name if possible
    :return:                The gem name if found, None if not found
    """

    global GEM_NAME_BY_ID

    if not GEM_NAME_BY_ID:
        # populate the cache once
        for search_subfolder in GEM_NAME_SEARCH_SUBFOLDERS:
            # Glob through each folder to search for the gem definition file (gem.json).
            # This is a potentially expensive file search so limit the scope of this search as much
            # as possible
            gems_search_node = ctx.srcnode.find_node(search_subfolder)
            if gems_search_node is None:
                continue

            gem_definitions = gems_search_node.ant_glob('**/gem.json')
            for gem_def in gem_definitions:
                try:
                    gem_json = ctx.parse_json_file(gem_def)
                    reader = _create_field_reader(ctx, gem_json, 'Gem in ' + gem_def.abspath())
                    gem_name = reader.field_req('Name')
                    gem_id = reader.uuid()

                    GEM_NAME_BY_ID[gem_id] = gem_name
                except:
                    # This is a bad config file
                    Logs.warn('[WARN] invalid config file {}'.format(gem_def))

    # return result from the cache
    if input_gem_id in GEM_NAME_BY_ID:
        return GEM_NAME_BY_ID[input_gem_id]

    return None


@conf
def add_gems_to_specs(conf):

    manager = GemManager.GetInstance(conf)

    # Add Gems to the specs
    manager.add_to_specs()

    conf.env['REQUIRED_GEMS'] = conf.scan_required_gems_paths()

@conf
def process_gems(self):
    manager = GemManager.GetInstance(self)

    # Recurse into all sub projects
    manager.recurse()

@conf
def GetGemsOutputModuleNames(ctx):
    target_names = []
    for gem in GemManager.GetInstance(ctx).gems:
        for module in gem.modules:
            target_names.append(module.file_name)
    return target_names

def apply_gem_to_kw(ctx, kw, gem):
    """
    Apply a gem to the kw
    """
    gem_include_path = gem.get_include_path()
    if os.path.exists(gem_include_path):
        cry_utils.append_unique_kw_entry(kw,'includes',gem_include_path)

    for module in gem.modules:
        if module.requires_linking(ctx):
            cry_utils.append_unique_kw_entry(kw, 'use', module.target_name)

    if not isinstance(ctx, ConfigurationContext):
        # On non-configure commands, determine based on the configuration whether to add the gem local uselibs or not
        if (gem.export_uselibs or ctx.is_build_monolithic()) and len(gem.local_uselibs)>0:
            configuration = ctx.env['CONFIGURATION']
            if not (gem.local_uselib_non_release and configuration.lower() in ["release", "performance"]):
                cry_utils.append_unique_kw_entry(kw, 'uselib', gem.local_uselibs)


def apply_gems_to_kw(ctx, kw, gems, game_name):
    """
    Apply gems to the kw
    """
    for gem in gems:
        gem_include_path = gem.get_include_path()
        if os.path.exists(gem_include_path):
            cry_utils.append_unique_kw_entry(kw,'includes',gem_include_path)

        if not isinstance(ctx, ConfigurationContext):
            # On non-configure commands, determine based on the configuration whether to add the gem local uselibs or not
            if (gem.export_uselibs or ctx.is_build_monolithic()) and len(gem.local_uselibs)>0:
                configuration = ctx.env['CONFIGURATION']
                if not (gem.local_uselib_non_release and configuration.lower() in ["release", "performance"]):
                	cry_utils.append_unique_kw_entry(kw, 'uselib', gem.local_uselibs)

        cry_utils.append_unique_kw_entry(kw, 'use', _get_linked_module_targets(gems, game_name, ctx))

@conf
def DefineGem(ctx, *k, **kw):
    """
    Gems behave very similarly to engine modules, but with a few extra options
    """
    manager = GemManager.GetInstance(ctx)

    gem = manager.get_gem_by_path(ctx.path.parent.abspath())
    if not gem:
        ctx.cry_error("DefineGem must be called by a wscript file in a Gem's 'Code' folder. Called from: {}".format(ctx.path.abspath()))

    manager.current_gem = gem

    # Generate list of resolved dependencies
    dependency_objects = []
    for dep_id in gem.dependencies:
        dep = manager.get_gem_by_spec(dep_id)
        if not dep:
            unmet_name = find_gem_name_by_id(ctx, dep_id)
            if unmet_name is None:
                ctx.cry_error('Gem {}({}) has an unmet dependency with ID {} (Unable to locate in disk).'.format(gem.id, gem.name, dep_id))
            else:
                ctx.cry_error('Gem {}({}) has an unmet dependency with ID {}({}). Please use the Project Configurator to correct this.'.format(gem.id, gem.name, dep_id, unmet_name))

            continue
        dependency_objects.append(dep)

    # Applies dependencies to args list
    def apply_dependencies(args):
        for dep in dependency_objects:
            dep_include = dep.get_include_path()
            if os.path.exists(dep_include):
                cry_utils.append_to_unique_list(args['includes'], dep_include)
            for module in dep.modules:
                if module.requires_linking(ctx):
                    cry_utils.append_to_unique_list(args['use'], module.target_name)

    # Iterate over each module and setup build
    for module in gem.modules:
        if module.type == Gem.Module.Type.Standalone:
            # Managed within the wscript independently
            continue

        if module.name:
            module_kw = kw.get(module.name, None)

            # If no based on name, try lowercasing
            if module_kw == None:
                module_kw = kw.get(module.name.lower(), None)

            # If still no kw, error
            if module_kw == None:
                ctx.cry_error("Gem {0}'s wscript missing definition for module {1} (valid dict names are {1} and {2}.".format(gem.name, module.name, module.name.lower()))
        else:
            module_kw = kw

        module_file_list_base = module.target_name.replace('.', '_').lower()

        # Set default properties
        default_settings = {
            'target': module.target_name,
            'output_file_name': module.file_name,
            'vs_filter': gem.name if gem.is_game_gem else 'Gems',
            'file_list': ["{}.waf_files".format(module_file_list_base)],
            'platforms': ['all'],
            'configurations': ['all'],
            'defines': [],
            'includes': [],
            'export_includes': [],
            'lib': [],
            'libpath': [],
            'features': [],
            'use': [],
            'uselib': []
        }

        # Builders have some special settings
        if module.type in [Gem.Module.Type.Builder, Gem.Module.Type.EditorModule]:
            default_settings['platforms'] = ['win', 'darwin']
            default_settings['configurations'] = ['all']
            kw['exclude_monolithic'] = True
            default_settings['client_only'] = True

        if module.parent:
            parent_module = None
            for parent_module_itr in gem.modules:
                if (parent_module_itr.name == module.parent or
                    (module.parent == 'GameModule' and parent_module_itr.type == Gem.Module.Type.GameModule and
                    parent_module_itr.name == None)):
                    parent_module = parent_module_itr
                    break
            if not parent_module:
                ctx.cry_error('{}\'s Gem.json Module "{}" "Extends" non-existent module {}.'.format(gem.name, module.name, module.parent))

            parent_kw = getattr(parent_module, 'kw', None)
            if not parent_kw:
                ctx.cry_error('{}\'s wscript defines module {} before parent {}, please reverse the order.'.format(gem.name, module.name, module.parent))

            EXTENDABLE_FIELDS = [
                'file_list',
                'defines',
                'includes',
                'features',
                'lib',
                'libpath',
                'use',
                'uselib'
            ]

            INHERITABLE_FIELDS = [
                'pch',
            ]

            for field in EXTENDABLE_FIELDS:
                default_settings[field].extend(parent_kw.get(field, []))

            for field in INHERITABLE_FIELDS:
                parent_value = parent_kw.get(field, None)
                if parent_value:
                    default_settings[field] = parent_value

        # Apply defaults to the project
        for key, value in default_settings.iteritems():
            if key not in module_kw:
                module_kw[key] = value

        # Make it so gems can be replaced while executable is still running
        cry_utils.append_to_unique_list(module_kw['features'], ['link_running_program'])

        # Add tools stuff to the editor modules
        if module.type == Gem.Module.Type.EditorModule:
            cry_utils.append_unique_kw_entry(module_kw, 'features', ['qt5'])
            cry_utils.append_unique_kw_entry(module_kw, 'use', ['AzToolsFramework', 'AzQtComponents'])
            cry_utils.append_unique_kw_entry(module_kw, 'uselib', ['QT5CORE', 'QT5QUICK', 'QT5GUI', 'QT5WIDGETS'])

        # If the Gem is a game gem, we may need to apply enabled gems for all of the enabled game projects so it will build
        if gem.is_game_gem and module.type != Gem.Module.Type.Builder:

            # We need to let cryengine_modules.RunTaskGenerator know that this is a game gem and must be built always
            setattr(ctx, 'is_game_gem', True)

            # The gem only builds once, so we need apply the union of all non-game-gem gems enabled for all enabled game projects
            unique_gems = []
            enabled_projects = ctx.get_enabled_game_project_list()
            for enabled_project in enabled_projects:
                gems_for_project = ctx.get_game_gems(enabled_project)
                for gem_for_project in gems_for_project:
                    if gem_for_project.name != gem.name and not gem_for_project.is_game_gem:
                        cry_utils.append_to_unique_list(unique_gems, gem_for_project)

            for unique_gem in unique_gems:
                if unique_gem.id in gem.dependencies or unique_gem.is_required:
                    apply_gem_to_kw(ctx, kw, unique_gem)

        working_path = ctx.path.abspath()
        dir_contents = os.listdir(working_path)

        # Setup PCH if disable_pch is false (default), and pch is not set (default)
        if not module_kw.get('disable_pch', False) and module_kw.get('pch', None) == None:

            # default casing for the relative path to the pch file
            source_dir = 'Source'
            pch_file = 'StdAfx.cpp'

            for entry in dir_contents:
                if entry.lower() == 'source':
                    source_dir = entry
                    break

            source_contents = os.listdir(os.path.join(working_path, source_dir))
            # see if they have a legacy StdAfx precompiled header
            for entry in source_contents:
                if entry.lower() == 'stdafx.cpp':
                    pch_file = entry
                    break
            # if they have a precompiled file then we will prefer that
            for entry in source_contents:
                if entry.lower().endswith('precompiled.cpp'):
                    pch_file = entry
                    break

            # has to be forward slash because the pch is string compared with files
            # in the waf_files which use forward slashes
            module_kw['pch'] = "{}/{}".format(source_dir, pch_file)

        # Apply any additional 3rd party uselibs
        if len(gem.local_uselibs) > 0:
            cry_utils.append_unique_kw_entry(module_kw, 'uselib', gem.local_uselibs)

        # Link the auto-registration symbols so that Flow Node registration will work
        if module.type in [Gem.Module.Type.GameModule, Gem.Module.Type.EditorModule]:
            cry_utils.append_unique_kw_entry(module_kw, 'use', ['CryAction_AutoFlowNode', 'AzFramework'])

        cry_utils.append_unique_kw_entry(module_kw, 'features', ['link_running_program'])

        # If disable_tests=False or disable_tests isn't specified, enable Google test
        disable_test_settings = ctx.GetPlatformSpecificSettings(module_kw, 'disable_tests', ctx.env['PLATFORM'], ctx.env['CONFIGURATION'])
        disable_tests = module_kw.get('disable_tests', False) or any(disable_test_settings)
        # Disable tests when doing monolithic build, except when doing project generation (which is always monolithic)
        disable_tests = disable_tests or (ctx.env['PLATFORM'] != 'project_generator' and ctx.is_build_monolithic())
        # Disable tests on non-test configurations, except when doing project generation
        disable_tests = disable_tests or (ctx.env['PLATFORM'] != 'project_generator' and 'test' not in ctx.env['CONFIGURATION'])
        if not disable_tests:
            cry_utils.append_unique_kw_entry(module_kw, 'use', 'AzTest')
            test_waf_files = "{}_tests.waf_files".format(module_file_list_base)
            if ctx.path.find_node(test_waf_files):
                cry_utils.append_unique_kw_entry(module_kw, 'file_list', test_waf_files)

        # Setup includes
        include_paths = []

        # Most gems use the upper case directories, however some have lower case source directories.
        # This will add the correct casing of those include directories
        local_includes = [ entry for entry in dir_contents if entry.lower() in [ 'include', 'source' ] ]

        # Add local includes
        for local_path in local_includes:
            node = ctx.path.find_node(local_path)
            if node:
                include_paths.append(node)

        # if the gem includes aren't already in the list, ensure they are prepended in order
        gem_includes = []
        for include_path in include_paths:
            if not include_path in module_kw['includes']:
                gem_includes.append(include_path)
        module_kw['includes'] = gem_includes + module_kw['includes']

        # Take the gems include folder if it exists and add it to the export_includes in case the gem is being 'used'
        export_include_node = ctx.path.find_node('Include')
        if export_include_node:
            module_kw['export_includes'] = [export_include_node.abspath()] + module_kw['export_includes']

        apply_dependencies(module_kw)

        # Save the build settings so we can access them later
        module.kw = module_kw

        cry_utils.append_unique_kw_entry(module_kw, 'is_gem', True)

        if module.type in [Gem.Module.Type.GameModule, Gem.Module.Type.EditorModule]:
            ctx.CryEngineSharedLibrary(**module_kw)
        elif module.type == Gem.Module.Type.StaticLib:
            ctx.CryEngineStaticLibrary(**module_kw)
        elif module.type == Gem.Module.Type.Builder:
            ctx.BuilderPlugin(**module_kw)

        # INTERNAL USE ONLY
        # Apply export_defines to ENTIRE BUILD. USE LIGHTLY.
        if module.type == Gem.Module.Type.GameModule:
            export_defines = module_kw.get('export_defines', []) + ctx.GetPlatformSpecificSettings(module_kw, 'export_defines', ctx.env['PLATFORM'], ctx.env['CONFIGURATION'])
            cry_utils.append_to_unique_list(ctx.env['DEFINES'], export_defines)

    manager.current_gem = None


#############################################################################
##
@feature('cxxshlib', 'cxxprogram')
@after_method('process_use')
def add_gem_dependencies(self):
    if not getattr(self, 'link_task', None):
        return

    manager = GemManager.GetInstance(self.bld)
    gem = manager.get_gem_by_path(self.path.parent.abspath())
    if not gem:
        return

    for dep_id in gem.dependencies:
        dep_gem = manager.get_gem_by_spec(dep_id)
        for module in dep_gem.modules:
            if module.requires_linking(self.bld):
                dep_gem_task_gen = self.bld.get_tgen_by_name(module.target_name)
                if not getattr(dep_gem_task_gen, 'link_task', None):
                    dep_gem_task_gen.post()
                if getattr(dep_gem_task_gen, 'link_task', None):
                    self.link_task.set_run_after(dep_gem_task_gen.link_task)

class Version(object):
    def __init__(self, ctx, ver_str = None):
        self.major = 0
        self.minor = 0
        self.patch = 0
        self.ctx = ctx

        if ver_str:
            self.parse(ver_str)

    def parse(self, string):
        match = re.match(r'(\d+)\.(\d+)\.(\d+)', string)
        if match:
            self.major = int(match.group(1))
            self.minor = int(match.group(2))
            self.patch = int(match.group(3))
        else:
            self.ctx.cry_error('Invalid version format {}. Should be [MAJOR].[MINOR].[PATCH].'.format(string))

    def __str__(self):
        return '{}.{}.{}'.format(self.major, self.minor, self.patch)

    def __eq__(self, other):
        return (isinstance(other, self.__class__)
            and self.major == other.major
            and self.minor == other.minor
            and self.patch == other.patch)

    def __ne__(self, other):
        return not self.__eq__(other)


class Gem(object):
    class LinkType(object):
        Dynamic         = 'Dynamic'
        DynamicStatic   = 'DynamicStatic'
        NoCode          = 'NoCode'

        Types           = [Dynamic, DynamicStatic, NoCode]

    class Module(object):
        class Type(object):
            GameModule      = 'GameModule'
            EditorModule    = 'EditorModule'
            StaticLib       = 'StaticLib'
            Builder         = 'Builder'
            Standalone      = 'Standalone'

            Types           = [GameModule, EditorModule, StaticLib, Builder, Standalone]

        def requires_linking(self, ctx):
            # If NoCode, never link for any reason
            if self.link_type == Gem.LinkType.NoCode:
                return False

            # Always link static libs
            if self.type == Gem.Module.Type.StaticLib:
                return True

            # Never link in Builders
            if self.type == Gem.Module.Type.Builder:
                return False

            # Never link in Standalones
            if self.type == Gem.Module.Type.Standalone:
                return False

            # Don't link in EditorModules if doing a monolithic build
            if self.type == Gem.Module.Type.EditorModule and ctx.is_build_monolithic():
                return False

            # When doing monolithic builds, always link (the module system will not do that for us)
            if ctx.is_build_monolithic():
                return True

            return self.link_type in [Gem.LinkType.DynamicStatic]

        def __init__(self):
            self.type = Gem.Module.Type.GameModule
            self.name = None
            self.target_name = None
            self.link_type = Gem.LinkType.Dynamic
            self.file_name = None
            self.parent = None

    def __init__(self, ctx):
        self.name = ""
        self.id = None
        self.path = ""
        self.abspath = ""
        self.version = Version(ctx)
        self.dependencies = []
        self.ctx = ctx
        self.games_enabled_in = []
        self.editor_targets = []
        self.modules = []
        self.is_legacy_igem = False
        self.is_game_gem = False
        self.is_required = False
        self.export_uselibs = False
        self.local_uselibs = []

    def load_from_json(self, gem_def):
        '''
        Parses a Gem description file from JSON.
        Requires that self.path already be set.
        '''
        self._upgrade_gem_json(gem_def)

        reader = _create_field_reader(self.ctx, gem_def, 'Gem in ' + self.path)

        self.name = reader.field_req('Name')
        self.version = reader.version()
        self.id = reader.uuid()
        self.editor_targets = reader.field_opt('EditorTargets', [])
        self.is_legacy_igem = reader.field_opt('IsLegacyIGem', False)
        self.export_uselibs = self.ctx.get_export_internal_3rd_party_lib(self.name)

        for dep_obj in reader.field_opt('Dependencies', list()):
            dep_reader = _create_field_reader(self.ctx, dep_obj, '"Dependencies" field in Gem in ' + self.path)
            self.dependencies.append(dep_reader.uuid())

        self.is_game_gem = reader.field_opt('IsGameGem', False)
        self.is_required = reader.field_opt('IsRequired', False)

        # There is a special flag for 3rd party configurations that will prevent the libs to be considered when we are building in performance/release mode
        # We need to make sure to take this into consideration
        self.local_uselib_non_release = False

        # If this gem has its own 3rd party configs, then read the 3rd party config records
        # to get the uselib names of the libraries being defined
        if len(self.abspath) > 0 and self.abspath.startswith(self.ctx.engine_path):
            gem_3p_base_node = self.ctx.engine_node.make_node(self.path).find_node('3rdParty')
        else:
            gem_3p_base_node = self.ctx.path.make_node(self.path).find_node('3rdParty')
        if gem_3p_base_node:
            gem_3p_base_search_pattern = os.path.join(gem_3p_base_node.abspath(), '*.json')
            gem_3p_config_files = glob.glob(gem_3p_base_search_pattern)
            for gem_3p_config_file in gem_3p_config_files:
                gem_3p_config_node = gem_3p_base_node.make_node(os.path.basename(gem_3p_config_file));
                lib_config, uselib_names,_ = self.ctx.get_3rd_party_config_record(gem_3p_config_node)
                for uselib_name in uselib_names:
                    self.local_uselibs.append(uselib_name)
                if "non_release_only" in lib_config:
                    self.local_uselib_non_release = lib_config["non_release_only"].lower() == 'true'

        found_default_module = False
        modules_list = reader.field_opt('Modules', [])
        for module_obj in modules_list:
            module_reader = _create_field_reader(self.ctx, module_obj, '"Modules" field in Gem in ' + self.path)

            # Read type, branch reading based on that
            module = Gem.Module()
            module.type = module_reader.field_req('Type')

            # No Editor modules unless they are enabled
            if module.type == Gem.Module.Type.EditorModule and (not self.ctx.editor_gems_enabled()):
                continue

            if module.type not in Gem.Module.Type.Types:
                self.ctx.cry_error(self.path + '/Gem.json file\'s "Modules" value "{}" is invalid, please supply one of:\n{}'.format(module.type, Gem.Module.Type.Types))
            if module.type == Gem.Module.Type.GameModule:
                module.link_type = module_reader.field_opt('LinkType', Gem.LinkType.Dynamic)
            else:
                module.parent = module_reader.field_opt('Extends', None)

            # Name is optional in Modules (assuming there's only 1 default), but no where else
            if module.type == Gem.Module.Type.GameModule:
                if not found_default_module:
                    module.name = module_reader.field_opt('Name', None)
                    found_default_module = True
                else:
                    self.ctx.cry_error('Gem.json file\'s "Modules" list can only contain one module without a name.')
            else:
                module.name = module_reader.field_req('Name')


            if module.name:
                module.target_name = '{}.{}'.format(self.name, module.name)
            else:
                module.target_name = self.name

            module.file_name = 'Gem.{}.{}.v{}'.format(module.target_name, self.id.hex, str(self.version))

            self.modules.append(module)

    def _upgrade_gem_json(self, gem_def):
        """
        If gems file is in an old format, update the data within.
        """
        reader = _create_field_reader(self.ctx, gem_def, 'Gem in ' + self.path)

        latest_format_version = 4
        gem_format_version = reader.field_int('GemFormatVersion')

        # Can't upgrade ancient versions or future versions
        if (gem_format_version < 2 or gem_format_version > latest_format_version):
            reader.ctx.cry_error(
                'Field "GemsFormatVersion" is {}, not expected version {}. Please update your Gem file in {}.'.format(
                    gem_format_version, latest_format_version, self.path))

        # v2 -> v3
        if (gem_format_version < 3):
            gem_def['IsLegacyIGem'] = True

        # v3 -> v4
        if (gem_format_version < 4):
            # Get generally helpful fields
            name = reader.field_req('Name')
            version = reader.version()
            gem_id = reader.uuid()

            # Get old required fields, and remove them
            link_type = reader.field_req('LinkType')
            if link_type not in Gem.LinkType.Types:
                self.ctx.cry_error('Gem.json file\'s "LinkType" value "{}" is invalid, please supply one of:\n{}'.format(link_type, Gem.LinkType.Types))

            has_editor_module = reader.field_opt('EditorModule', False)

            # Generate module list
            if link_type != Gem.LinkType.NoCode:
                game_module = dict(
                    Type     = Gem.Module.Type.GameModule,
                    LinkType = link_type,
                )
                modules = [game_module]

                if has_editor_module:
                    editor_module = dict(
                        Name    = 'Editor',
                        Type    = Gem.Module.Type.EditorModule,
                        Extends = 'GameModule',
                    )
                    modules.append(editor_module)

                gem_def['Modules'] = modules

        # File is now up to date
        gem_def['GemFormatVersion'] = latest_format_version

    def get_include_path(self):
        # The default casing is upper case 'I' include for the folder name
        abs_include = os.path.join(self.abspath, GEMS_CODE_FOLDER, 'Include')
        if os.path.exists(abs_include):
            return abs_include
        # In case 'I'nclude is not found, fall back to all lowercase (legacy)
        abs_include = os.path.join(self.abspath, GEMS_CODE_FOLDER, 'include')
        return abs_include

    def _get_spec(self):
        return (self.id, self.version, self.path)
    spec = property(_get_spec)

    def __eq__(self, other):
        return (isinstance(other, self.__class__)
            and self.id == other.id
            and self.version == other.version
            and self.path == other.path)

    def __ne__(self, other):
        return not self.__eq__(other)


class GemManager(object):
    """This class loads all gems for all enabled game projects
    Once loaded:
       the dirs property contains all gem folders
       the gems property contains a list of Gem objects, for only the ACTIVE, ENABLED gems that cover all game projects
       the ctx property is the build context
       """

    def __init__(self, ctx):
        self.dirs = []
        self.gems = []
        self.required_gems = []
        self.search_paths = []
        # Keeps track of Gem currently being defined to avoid recursive 'use's
        self.current_gem = None
        self.ctx = ctx

    def get_gem_by_path(self, path):
        """
        Gets the Gem with the corresponding path
        :param path The path to the Gem to look up.
        :ptype path str
        :rtype : Gem
        """
        norm_path = os.path.normpath(path)
        for gem in self.gems:
            if norm_path == os.path.normpath(gem.abspath):
                return gem

        return None

    def get_gem_by_spec(self, *spec):
        """
        Gets the Gem with the corresponding id
        :param spec The spec to the Gem to look up (id, [opt] version, [opt] path).
        :ptype path tuple
        :rtype : Gem
        """
        if spec.count < 1:
            return None

        check_funs = [
            lambda gem: gem.id == spec[0],
            lambda gem: check_funs[0](gem) and gem.version == spec[1],
            lambda gem: check_funs[1](gem) and os.path.normcase(gem.path) == os.path.normcase(spec[2]),
            ]

        for gem in self.gems:
            if check_funs[len(spec) - 1](gem):
                return gem

        return None

    def contains_gem(self, *gem_spec):
        return self.get_gem_by_spec(*gem_spec) != None

    def load_gem_from_disk(self, gem_id, version, path, gems_list_context_msg):
        Logs.debug('gems: Gem not found in cache, attempting to load from disk: ({}, {}, {})'.format(gem_id,
                                                                                                     version,
                                                                                                     path))
        detected_gem_versions = {}

        gem = None

        for search_path in self.search_paths:
            def_file = os.path.join(search_path, path, GEMS_DEFINITION_FILE)
            if not os.path.isfile(def_file):
                continue  # Try again with the next path

            gem = Gem(self.ctx)
            gem.path = path
            gem.abspath = os.path.join(search_path, path)
            gem.load_from_json(self.ctx.parse_json_file(self.ctx.root.make_node(def_file)))

            # Protect against loading duplicate gems from different locations, showing a warning if detected
            dup_gem = detected_gem_versions.get(gem.version.__str__(), None)
            if dup_gem is not None:
                Logs.warn(
                    '[WARN] Duplicate gem {} (version {}) found in multiple paths.  Accepting the one at {}'.format(
                        gem.name, gem.version, dup_gem.abspath))
                gem = dup_gem
                break
            detected_gem_versions[gem.version.__str__()] = gem

            # Validate that the Gem loaded from the path specified actually matches the id and version.
            if gem.id != gem_id:
                self.ctx.cry_error(
                    "Gem at path {} has ID {}, instead of ID {} specified in {}.".format(
                        path, gem.id, gem_id, gems_list_context_msg))

            if gem.version != version:
                self.ctx.cry_error(
                    "Gem at path {} has version {}, instead of version {} specified in {}.".format(
                        path, gem.version, version, gems_list_context_msg))
            self.add_gem(gem)

        return gem

    def add_gems_from_file(self, gems_list_file, game_project=None):

        Logs.debug('gems: reading gems file at %s' % gems_list_file)

        gems_list_context_msg = 'Gems list for project {}'.format(game_project) if game_project \
                                 else 'Gems list {}'.format(gems_list_file.abspath())

        gem_info_list = self.ctx.parse_json_file(gems_list_file)
        list_reader = _create_field_reader(self.ctx, gem_info_list, gems_list_context_msg)

        # Verify that the project file is an up-to-date format
        gem_format_version = list_reader.field_int('GemListFormatVersion')
        if gem_format_version != GEMS_FORMAT_VERSION:
            self.ctx.cry_error(
                'Gems list file at {} is of version {}, not expected version {}. Please update your project file.'.format(
                    gems_list_file, gem_format_version, GEMS_FORMAT_VERSION))

        for idx, gem_info_obj in enumerate(list_reader.field_req('Gems')):

            # String for error reporting.
            gem_context_msg = 'Gem {} in game project {}'.format(idx, game_project) if game_project \
                                else 'Gem {}'.format(idx)

            reader = _create_field_reader(self.ctx, gem_info_obj, gem_context_msg)

            gem_id = reader.uuid()
            version = reader.version()
            path = os.path.normpath(reader.field_req('Path'))

            gem = self.get_gem_by_spec(gem_id, version, path)
            if not gem:
                Logs.debug('gems: Gem not found in cache, attempting to load from disk: ({}, {}, {})'.format(gem_id,
                                                                                                             version,
                                                                                                             path))
                gem = self.load_gem_from_disk(gem_id, version, path, gems_list_context_msg)																											 

            if not gem:
                self.ctx.cry_error('Failed to load from path "{}"'.format(path))

            if game_project:
                gem.games_enabled_in.append(game_project)

    def process(self):
        """
        Process current directory for gems

        Note that this has to check each game project to know which gems are enabled
        and build a list of all enabled gems so that those are built.
        To debug gems output during build, use --zones=gems in your command line
        """


        this_path = self.ctx.path

        cry_utils.append_to_unique_list(self.search_paths, os.path.normpath(this_path.abspath()))

        # Parse Gems search path
        config = RawConfigParser()
        if config.read(this_path.make_node('SetupAssistantUserPreferences.ini').abspath()):
            if config.has_section(GEMS_FOLDER) and config.has_option(GEMS_FOLDER, 'SearchPaths\\size'):
                # Parse QSettings style array (i.e. read 'size' attribute, then 1-based-idx\Path)
                array_len = config.getint(GEMS_FOLDER, 'SearchPaths\\size');
                for i in range(0, array_len):
                    new_path = config.get(GEMS_FOLDER, 'SearchPaths\\{}\\Path'.format(i + 1))
                    new_path = os.path.normpath(new_path)
                    Logs.debug('gems: Adding search path {}'.format(new_path))
                    cry_utils.append_to_unique_list(self.search_paths, os.path.normpath(new_path))

        if not self.ctx.is_engine_local():
            cry_utils.append_to_unique_list(self.search_paths,os.path.realpath(self.ctx.engine_path))

        # Load all the gems under the Gems folder to search for required gems
        self.required_gems = self.ctx.load_required_gems()

        game_projects = self.ctx.get_enabled_game_project_list()

        add_gems = False

        if len(game_projects)>0:
            for game_project in game_projects:
                Logs.debug('gems: Game Project: %s' % game_project)

                gems_list_file = self.ctx.get_project_node(game_project).make_node(GEMS_LIST_FILE)

                if not os.path.isfile(gems_list_file.abspath()):
                    if self.ctx.is_option_true('gems_optional'):
                        Logs.debug("gems: Game has no gems file, skipping [%s]" % gems_list_file)
                        continue # go to the next game
                    else:
                        self.ctx.cry_error('Project {} is missing {} file.'.format(game_project, GEMS_LIST_FILE))

                self.add_gems_from_file(gems_list_file, game_project)
                add_gems = True
        else:
            # If there are no enabled, valid game projects, then see if we have an override gems list from the enable specs
            override_gems_list = self.ctx.get_override_gems_list()
            path_check_base = this_path.abspath()
            for override_gems_file_path in override_gems_list:
                if not os.path.exists(os.path.join(path_check_base, override_gems_file_path)):
                    self.ctx.cry_error('Invalid override gem file {} specified in one of the enabled specs.')
                gems_list_file = this_path.make_node(override_gems_file_path)
                self.add_gems_from_file(gems_list_file, None)
                add_gems = True

        for gem in self.gems:
            Logs.debug("gems: gem %s is used by games: %s" % (gem.name, gem.games_enabled_in))

        # Always add required gems to the gems manager if gems are added
        if add_gems:
            for required_gem in self.required_gems:
                self.add_gem(required_gem)

    def add_gem(self, gem):
        """Adds gem to the collection"""
        # Skip any disabled Gems
        # don't add duplicates!
        if (self.contains_gem(*gem.spec)):
            return
        self.gems.append(gem)
        code_path = self.ctx.root.make_node(gem.abspath).make_node(GEMS_CODE_FOLDER).abspath()
        if gem.modules and os.path.isdir(code_path):
            self.dirs.append(code_path)
        else:
            Logs.debug('gems: gem Code folder does not exist %s - this is okay if the gem contains only assets and no code' % code_path)

    def add_gem_to_spec(self, gem, spec_name):

        Logs.debug('gems: adding gem %s to spec %s' % (gem.name, spec_name))
        spec_dict = self.ctx.loaded_specs_dict[spec_name]
        if 'modules' not in spec_dict:
            spec_dict['modules'] = []
        spec_list = spec_dict['modules']
        for module in gem.modules:
            if module.target_name not in spec_list:
                spec_list.append(module.target_name)
        if gem.editor_targets and self.ctx.editor_gems_enabled():
            for editor_target in gem.editor_targets:
                Logs.debug('gems: adding editor target of gem %s - %s to spec %s' % (gem.name, editor_target, spec_name))
                spec_list.append(editor_target)

    def add_to_specs(self):

        enabled_game_projects = self.ctx.get_enabled_game_project_list()

        specs_to_include = self.ctx.loaded_specs() + ['gems']

        system_specs = ('gems','all')

        # Create Gems spec
        if not 'gems' in self.ctx.loaded_specs_dict:
            self.ctx.loaded_specs_dict['gems'] = dict(description="Configuration to build all Gems.",
                                                      visual_studio_name='Gems',
                                                      modules=['CryAction', 'CryAction_AutoFlowNode'])

        # If there are enabled game projects, then specifically add the gems to each current spec that the gem is
        # enabled for
        for spec_name in specs_to_include:

            # Get the defined game project per spec and only add gems from game projects
            # to specs that have them defined
            game_projects = self.ctx.spec_game_projects(spec_name)

            if len(game_projects) == 0 and not self.ctx.spec_disable_games(spec_name):
                # Handle the legacy situation where the game projects is set in the enabled_game_projects in user_settings.options
                fallback_enabled_game_projects_string = self.ctx.options.enabled_game_projects.replace(' ', '').strip()
                game_projects = fallback_enabled_game_projects_string.split(',')

            if len(game_projects) == 0 and spec_name not in system_specs:
                # If there are no game projects enabled at all, then add the gems automatically if specified
                if len(enabled_game_projects) == 0:
                    for gem in self.gems:
                        # If this is a system spec, then always add the gem
                        self.add_gem_to_spec(gem, spec_name)
                    continue

            # Add to build specs per project
            for gem in self.gems:
                # If this is a system spec, then always add the gem
                if spec_name in system_specs:
                    self.add_gem_to_spec(gem,spec_name)
                else:
                    for game_project in game_projects:
                        # Only include the gem in the current spec if it is enabled in any of its game projects
                        if game_project not in gem.games_enabled_in:
                            continue
                        self.add_gem_to_spec(gem, spec_name)

    def recurse(self):
        """Tells WAF to build active Gems"""
        self.ctx.recurse(self.dirs)

    @staticmethod
    def GetInstance(ctx):
        """
        Initializes the Gem manager if it hasn't already
        :rtype : GemManager
        """
        if not hasattr(ctx, 'gem_manager'):
            ctx.gem_manager = GemManager(ctx)
            ctx.gem_manager.process()
        return ctx.gem_manager

@conf
def editor_gems_enabled(ctx):

    if ctx.is_build_monolithic():
        return False
    capabilities = ctx.get_enabled_capabilities()

    return (("compileengine" in capabilities) or ("compilesandbox" in capabilities))

@conf
def get_game_gems(ctx, game_project):
    return [gem for gem in GemManager.GetInstance(ctx).gems if game_project in gem.games_enabled_in]

@conf
def get_required_gems(ctx):
    return [gem for gem in GemManager.GetInstance(ctx).required_gems if gem.is_required]

def _get_linked_module_targets(gem_list, game_name, ctx):
    modules = list()
    current_gem = ctx.gem_manager.current_gem
    for gem in gem_list:
        # Don't add the Gem currently being defined as a dependency of itself
        if gem == current_gem:
            continue

        for module in gem.modules:
            # Link against the module if it requires linking, and it isn't the game itself
            # (unless in monolithic builds, in which case game_name is irrelevant beacuse we're actaully linking against the launcher)
            if module.requires_linking(ctx) and (module.target_name != game_name or ctx.is_build_monolithic()):
                modules.append(module.target_name)
    return modules

@conf
def apply_gems_to_context(ctx, game_name, k, kw):

    """this function is called whenever its about to generate a target that should be aware of gems"""
    game_gems = ctx.get_game_gems(game_name)

    Logs.debug('gems: adding gems to %s (%s_%s) : %s' % (game_name, ctx.env['CONFIGURATION'], ctx.env['PLATFORM'], [gem.name for gem in game_gems]))

    apply_gems_to_kw(ctx, kw, game_gems, game_name)

@conf
def apply_required_gems_to_context(ctx, target, kw):

    required_gems = [required_gem for required_gem in ctx.get_required_gems() if required_gem.name != target]
    if len(required_gems) != 0:
        Logs.debug('gems: adding required gems to target {} : {}'.format(target, ','.join([gem.name for gem in required_gems])))


    apply_gems_to_kw(ctx, kw, required_gems, target)

@conf
def is_gem(ctx, module_name):
    for gem in GemManager.GetInstance(ctx).gems:
        for module in gem.modules:
            if module.target_name == module_name:
                return True
    return False


@conf
def scan_required_gems_paths(ctx):
    """
    Scan the folder for any required gems

    :param ctx: Configuration Context
    """

    required_gems = {}

    def _search_for_gem(base_path, folder):
        content_path = base_path.make_node(folder)
        if os.path.exists(os.path.join(content_path.abspath(), GEMS_DEFINITION_FILE)):
            # Use the standard gem config loader to determine if the gem is required or not
            gem = Gem(ctx)
            gem.path = os.path.join(str(base_path), folder)
            gem.abspath = os.path.join(base_path.abspath(),folder)
            gems_config_file = content_path.make_node(GEMS_DEFINITION_FILE)
            gem.load_from_json(ctx.parse_json_file(gems_config_file))
            gem.abspath = content_path.abspath()
            if gem.is_required:
                if gem.path in required_gems:
                    ctx.cry_error('[ERROR] Duplicate required gems path ({}) found in the gems search paths for gem locations "{}" and "{}".'.format(gem.path,
                                                                                                                     required_gems[gem.path],
                                                                                                                     content_path.abspath()))
                required_gems[gem.path] = content_path.abspath()
            # Return true if a gem folder was processed (ie contains a gem.json, regardless of whether its required or not)
            return True
        else:
            # Return false if the folder is not a gem folder.  We may need to recurse
            return False

    def _search_for_gem_folders(base_path):
        gem_folders = base_path.listdir()
        for gem_folder in gem_folders:
            gem_folder_abs = base_path.make_node(gem_folder).abspath()
            if os.path.isdir(gem_folder_abs):

                # Make sure we dont recurse into an asset cache folder by using the
                # 'assetdb.sqlite' file marker
                if os.path.exists(os.path.join(gem_folder_abs,'assetdb.sqlite')):
                    continue

                if not _search_for_gem(base_path, gem_folder):
                    _search_for_gem_folders(base_path.make_node(gem_folder))

    # Build search paths (which include config search paths, and root/Gems)
    manager = GemManager.GetInstance(ctx)
    search_paths = set(manager.search_paths)

    # Remove the engine path from the search path
    engine_path = os.path.normpath(os.path.realpath(ctx.engine_path))
    if engine_path in search_paths:
        search_paths.remove(engine_path)

    search_paths.add(os.path.normpath(ctx.engine_node.find_node('Gems').abspath()))
    for search_path in search_paths:
        current_base_path = ctx.root.find_node(search_path)
        _search_for_gem_folders(current_base_path)

    return required_gems


REQUIRED_GEMS_CACHE = None


@conf
def load_required_gems(ctx):
    """
    Attempt to read the required gems into a cache and use that cache in subsequent calls
    :param ctx:     Context
    :return:    List of required Gems paths
    """

    global REQUIRED_GEMS_CACHE
    if REQUIRED_GEMS_CACHE is not None:
        return REQUIRED_GEMS_CACHE

    required_gems = []
    required_gems_location = ctx.scan_required_gems_paths()
    for required_gem_location in required_gems_location:

        required_gem_abs_path = required_gems_location[required_gem_location]
        required_gem_def_file_path = ctx.root.make_node(required_gem_abs_path).make_node(GEMS_DEFINITION_FILE)

        try:
            gem = Gem(ctx)
            gem.path = str(required_gem_location)
            gem.load_from_json(ctx.parse_json_file(required_gem_def_file_path))
            gem.abspath = required_gem_abs_path
            if gem.is_required:
                required_gems.append(gem)
        except Exception as e:
            ctx.fatal('Error reading gems information for gem at path {}: {}'.format(required_gem_def_file_path.abspath(), str(e)))

    REQUIRED_GEMS_CACHE = required_gems

    return REQUIRED_GEMS_CACHE


@conf
def load_gems_from_gem_spec(ctx, gem_spec_file, add_to_manager=False):
    """
    Load gems from a specific gem-spec file.
    :param ctx:
    :param gem_spec_file:   The path of the gem spec file to read
    :param add_to_manager:  Option to add any missing gem that is discovered in the spec file but not in the manager
    :return: List of gems from the gem spec
    """
    if not gem_spec_file or not os.path.exists(gem_spec_file):
        raise Errors.WafError('Invalid empty gem_spec_file {}'.format(gem_spec_file or ""))

    # Read the gem spec file
    gem_info_list = utils.parse_json_file(gem_spec_file)
    list_reader = _create_field_reader(ctx, gem_info_list, 'Gems from {}'.format(gem_spec_file))

    # Verify that the project file is an up-to-date format
    gem_format_version = list_reader.field_int('GemListFormatVersion')
    if gem_format_version != GEMS_FORMAT_VERSION:
        raise Errors.WafError('Gems list file at {} is of version {}, not expected version {}. Please update your project file.'.format(gem_spec_file, gem_format_version, GEMS_FORMAT_VERSION))

    manager = GemManager.GetInstance(ctx)

    gems_list = list()

    for idx, gem_info_obj in enumerate(list_reader.field_req('Gems')):

        # String for error reporting.
        gem_context_msg = 'Gem {} from gems spec {}'.format(idx, gem_spec_file)

        reader = _create_field_reader(ctx, gem_info_obj, gem_context_msg)

        gem_id = reader.uuid()
        version = reader.version()
        path = os.path.normpath(reader.field_req('Path'))

        gem = manager.get_gem_by_spec(gem_id, version, path)
        if not gem:
            if add_to_manager:
                gems_list_context_msg = 'Gems list {}'.format(gem_spec_file)
                manager.load_gem_from_disk(gem_id, version, path, gems_list_context_msg)
            else:
                raise Errors.WafError('Invalid gem {}'.format(gem_id))

        gems_list.append(gem)

    return gems_list

@conf
def apply_gem_spec_to_context(ctx, gem_spec_file, kw):

    gems_list = ctx.load_gems_from_gem_spec(gem_spec_file)

    modules = list()

    for gem in gems_list:
        gem_include_path = gem.get_include_path()
        if os.path.exists(gem_include_path):
            cry_utils.append_unique_kw_entry(kw, 'includes', gem_include_path)

            for module in gem.modules:
                # Link against the module if it requires linking, and it isn't the game itself
                # (unless in monolithic builds, in which case game_name is irrelevant beacuse we're actaully linking against the launcher)
                if module.requires_linking(ctx) and (ctx.is_build_monolithic()):
                    modules.append(module.target_name)

    cry_utils.append_unique_kw_entry(kw, 'use', modules)
