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
import json
from ConfigParser import RawConfigParser
from waflib.Configure import conf
from waflib import Logs
from cry_utils import append_to_unique_list
from waflib.Build import Context


GEMS_UUID_KEY = "Uuid"
GEMS_VERSION_KEY = "Version"
GEMS_PATH_KEY = "Path"
GEMS_LIST_FILE = "gems.json"
GEMS_SERVER_LIST_FILE = "gems_server.json"
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
GEM_ID_BY_PATH = {}     # Cache of gem ids by the absolute path of the gem.json file
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
    global GEM_ID_BY_PATH

    # Check the cache first
    if input_gem_id in GEM_NAME_BY_ID:
        return GEM_NAME_BY_ID[input_gem_id]

    for search_subfolder in GEM_NAME_SEARCH_SUBFOLDERS:
        # Glob through each folder to search for the gem definition file (gem.json).
        # This is a potentially expensive file search so limit the scope of this search as much
        # as possible
        gems_search_node = ctx.srcnode.find_node(search_subfolder)
        if gems_search_node is None:
            continue

        gem_definitions = gems_search_node.ant_glob('**/gem.json')
        for gem_def in gem_definitions:
            # Check the id against any of the cached results first before attempting to actually
            # read from the json files again
            if gem_def.abspath() in GEM_ID_BY_PATH:
                cached_id_by_path = GEM_ID_BY_PATH[gem_def.abspath()]
                if cached_id_by_path in GEM_NAME_BY_ID and input_gem_id == cached_id_by_path:
                    return GEM_NAME_BY_ID[cached_id_by_path]
                continue
            try:
                gem_json = ctx.parse_json_file(gem_def)
                reader = _create_field_reader(ctx, gem_json, 'Gem in ' + gem_def.abspath())
                gem_name = reader.field_req('Name')
                gem_id = reader.uuid()

                GEM_NAME_BY_ID[gem_id] = gem_name
                GEM_ID_BY_PATH[gem_def.abspath()] = gem_id
                if input_gem_id == gem_id:
                    return gem_name
            except:
                # This is a bad config file, so mark it so it wont try to parse it again
                GEM_ID_BY_PATH[gem_def.abspath()] = BAD_GEM_PATH

        return None


@conf
def add_gems_to_specs(conf):

    manager = GemManager.GetInstance(conf)

    # Add Gems to the specs
    manager.add_to_specs()

    conf.env['REQUIRED_GEMS'] = conf.scan_required_gems()

@conf
def process_gems(self):
    manager = GemManager.GetInstance(self)

    # Recurse into all sub projects
    manager.recurse()

@conf
def GetEnabledGemUUIDs(ctx):
    return [gem.id for gem in GemManager.GetInstance(ctx).gems]

@conf
def GetGemsOutputModuleNames(ctx):
    return [gem.dll_file for gem in GemManager.GetInstance(ctx).gems]

@conf
def GetGemsModuleNames(ctx):
    return [gem.name for gem in GemManager.GetInstance(ctx).gems]

@conf
def DefineGem(ctx, *k, **kw):
    """
    Gems behave very similarly to engine modules, but with a few extra options
    """
    manager = GemManager.GetInstance(ctx)

    gem = manager.get_gem_by_path(ctx.path.parent.abspath())
    if not gem:
        ctx.cry_error("DefineGem must be called by a wscript file in a Gem's 'Code' folder. Called from: {}".format(ctx.path.abspath()))

    # Detect any 3rd party libs in this GEM
    detected_uselib_names = []
    if ctx.cmd.startswith('build_'):

        platform = ctx.env['PLATFORM']
        configuration = ctx.env['CONFIGURATION']
        reported_errors = set()

        path_alias_map = {'ROOT': ctx.root.make_node(Context.launch_dir).abspath(),
                          'GEM': ctx.path.parent.abspath()}
        config_3rdparty_folder = ctx.path.parent.make_node('3rdParty')
        thirdparty_error_msgs, detected_uselib_names = ctx.detect_all_3rd_party_libs(config_3rdparty_folder, platform, configuration, path_alias_map, False)
        for thirdparty_error_msg in thirdparty_error_msgs:
            if thirdparty_error_msg not in reported_errors:
                reported_errors.add(thirdparty_error_msg)
                Logs.warn('[WARN] {}'.format(thirdparty_error_msg))

    # Set default properties
    default_settings = {
        'target': gem.name,
        'output_file_name': gem.dll_file,
        'vs_filter': gem.name if gem.is_game_gem else 'Gems',
        'file_list': ["{}.waf_files".format(gem.name.lower())],
        'platforms': ['all'],
        'configurations' : ['all'],
        'defines': [],
        'includes': [],
        'export_includes': [],
        'lib': [],
        'libpath': [],
        'features': [],
        'use': [],
        'uselib': []
    }

    for key, value in default_settings.iteritems():
        if key not in kw:
            kw[key] = value

    # If the Gem is a game gem, we may need to apply enabled gems for all of the enabled game projects so it will build
    if gem.is_game_gem:
        # The gem only builds once, so we need apply the union of all non-game-gem gems enabled for all enabled game projects
        unique_gems = set()
        enabled_projects = ctx.get_enabled_game_project_list()
        for enabled_project in enabled_projects:
            gems_for_project = ctx.get_game_gems(enabled_project)
            for gem_for_project in gems_for_project:
                if gem_for_project.name != gem.name and not gem_for_project.is_game_gem:
                    unique_gems.add(gem_for_project)

        is_android = ctx.env['PLATFORM'] in ('android_armv7_gcc', 'android_armv7_clang')

        for unique_gem in unique_gems:
            if unique_gem.id in gem.dependencies or unique_gem.is_required or is_android:
                if os.path.exists(unique_gem.get_include_path().abspath()):
                    kw['includes'] += [unique_gem.get_include_path()]
                if Gem.LinkType.requires_linking(ctx, unique_gem.link_type):
                    kw['use'] += [unique_gem.name]

    working_path = ctx.path.abspath()
    dir_contents = os.listdir(working_path)

    # Setup PCH if disable_pch is false (default), and pch is not set (default)
    if not kw.get('disable_pch', False) and kw.get('pch', None) == None:

        # default casing for the relative path to the pch file
        source_dir = 'Source'
        pch_file = 'StdAfx.cpp'

        for entry in dir_contents:
            if entry.lower() == 'source':
                source_dir = entry
                break

        source_contents = os.listdir(os.path.join(working_path, source_dir))
        for entry in source_contents:
            if entry.lower() == 'stdafx.cpp':
                pch_file = entry
                break

        # has to be forward slash because the pch is string compared with files
        # in the waf_files which use forward slashes
        kw['pch'] = "{}/{}".format(source_dir, pch_file)

    # Apply any additional 3rd party uselibs
    if len(detected_uselib_names)>0:
        append_to_unique_list(kw['uselib'], list(detected_uselib_names))

    # Link the auto-registration symbols so that Flow Node registration will work
    append_to_unique_list(kw['use'], ['CryAction_AutoFlowNode', 'AzFramework'])
    
    # Make it so gems can be replaced while executable is still running
    append_to_unique_list(kw['features'], ['link_running_program'])

    # Setup includes
    include_paths = []

    # Most gems use the upper case directories, however some have lower case source directories.
    # This will add the correct casing of those include directories
    local_includes = [ entry for entry in dir_contents if entry.lower() in [ 'include', 'source' ] ]

    # If disable_tests=False or disable_tests isn't specified, enable Google test
    disable_test_settings = ctx.GetPlatformSpecificSettings(kw, 'disable_tests', ctx.env['PLATFORM'], ctx.env['CONFIGURATION'])
    disable_tests = kw.get('disable_tests', False) or any(disable_test_settings)
    # Disable tests when doing monolithic build, except when doing project generation (which is always monolithic)
    disable_tests = disable_tests or (ctx.env['PLATFORM'] != 'project_generator' and ctx.spec_monolithic_build())
    # Disable tests on non-test configurations, except when doing project generation
    disable_tests = disable_tests or (ctx.env['PLATFORM'] != 'project_generator' and 'test' not in ctx.env['CONFIGURATION'])
    if not disable_tests:
        append_to_unique_list(kw['use'], 'AzTest')
        test_waf_files = "{}_tests.waf_files".format(gem.name.lower())
        if ctx.path.find_node(test_waf_files):
            append_to_unique_list(kw['file_list'], test_waf_files)

    # Add local includes
    for local_path in local_includes:
        node = ctx.path.find_node(local_path)
        if node:
            include_paths.append(node)

    # if the gem includes aren't already in the list, ensure they are prepended in order
    gem_includes = []
    for include_path in include_paths:
        if not include_path in kw['includes']:
            gem_includes.append(include_path)
    kw['includes'] = gem_includes + kw['includes']

    # Take the gems include folder if it exists and add it to the export_includes in case the gem is being 'used'
    export_include_node = ctx.path.find_node('Include')
    if export_include_node:
        kw['export_includes'] = [export_include_node.abspath()] + kw['export_includes']

    # Generate list of resolved dependencies
    dependency_objects = [ ]
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
            if os.path.exists(dep_include.abspath()):
                append_to_unique_list(args['includes'], dep_include)
            if Gem.LinkType.requires_linking(ctx, dep.link_type):
                append_to_unique_list(args['use'], dep.name)

    apply_dependencies(kw)

    if gem.is_game_gem and ctx.is_monolithic_build() and ctx.env['PLATFORM'] in ('android_armv7_gcc', 'android_armv7_clang'):

        # Special case for android & monolithic builds.  If this is the game gem for the project, it needs to apply the
        # enabled gems here instead of the launcher where its normally applied.  (see cryengine_modules.CryLauncher_Impl
        # for details).  In legacy game projects, this the game dll is declared as a CryEngineModule
        setattr(ctx,'game_project',gem.name)
        ctx.CryEngineModule(ctx, *k, **kw)
    else:
        ctx.CryEngineSharedLibrary(ctx, *k, **kw)

    # If Gem is marked for having editor module, add it
    if gem.editor_module and ctx.editor_gems_enabled():
        # If 'editor' object is added to the wscript, ensure it's settings are kept
        editor_kw = kw.get('editor', { })

        # Overridable settings, not to be combined with Game version
        editor_default_settings = {
            'target': gem.editor_module,
            'output_file_name': gem.editor_dll_file,
            'vs_filter': 'Gems',
            'platforms': ['win', 'darwin'],
            'configurations' : ['debug', 'debug_test', 'profile', 'profile_test'],
            'file_list': kw['file_list'] + ["{}_editor.waf_files".format(gem.name.lower())],
            'pch':       kw['pch'],
            'defines':   list(kw['defines']),
            'includes':  list(kw['includes']),
            'export_includes':  list(kw['export_includes']),
            'lib':       list(kw['lib']),
            'libpath':   list(kw['libpath']),
            'features':  list(kw['features']),
            'use':       list(kw['use']),
            'uselib':    list(kw['uselib']),
        }

        for key, value in editor_default_settings.iteritems():
            if key not in editor_kw:
                editor_kw[key] = value

        # Include required settings
        append_to_unique_list(editor_kw['features'], ['qt5', 'link_running_program'])
        append_to_unique_list(editor_kw['use'], ['AzToolsFramework', 'AzQtComponents'])
        append_to_unique_list(editor_kw['uselib'], ['QT5CORE', 'QT5QUICK', 'QT5GUI', 'QT5WIDGETS'])

        # Set up for testing
        if not disable_tests:
            append_to_unique_list(editor_kw['use'], 'AzTest')
            test_waf_files = "{}_editor_tests.waf_files".format(gem.name.lower())
            if ctx.path.find_node(test_waf_files):
                append_to_unique_list(editor_kw['file_list'], test_waf_files)

        apply_dependencies(editor_kw)

        ctx.CryEngineModule(ctx, *k, **editor_kw)


#############################################################################
##
from waflib.TaskGen import feature, before_method, after_method
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
        dep_gem_name = find_gem_name_by_id(self.bld, dep_id)
        dep_gem_task_gen = self.bld.get_tgen_by_name(dep_gem_name)
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

        @classmethod
        def requires_linking(cls, ctx, link_type):
            # If NoCode, never link for any reason
            if link_type == cls.NoCode:
                return False

            # When doing monolithic builds, always link (the module system will not do that for us)
            if ctx.spec_monolithic_build():
                return True

            return link_type in [cls.DynamicStatic]

    def __init__(self, ctx):
        self.name = ""
        self.id = None
        self.path = ""
        self.abspath = ""
        self.version = Version(ctx)
        self.dependencies = []
        self.dll_file = ""
        self.waf_files = ""
        self.link_type = Gem.LinkType.Dynamic
        self.ctx = ctx
        self.games_enabled_in = []
        self.editor_targets = []
        self.is_legacy_igem = False
        self.editor_module = None
        self.is_game_gem = False
        self.is_required = False

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
        self.dll_file = reader.field_opt('DllFile', self._get_output_name())
        self.editor_dll_file = self._get_editor_output_name()
        self.editor_targets = reader.field_opt('EditorTargets', [])
        self.is_legacy_igem = reader.field_opt('IsLegacyIGem', False)

        # If EditorModule specified, provide name of name + Editor
        if reader.field_opt('EditorModule', False):
            self.editor_module = self.name + '.Editor'
            self.editor_targets.append(self.editor_module)

        for dep_obj in reader.field_opt('Dependencies', list()):
            dep_reader = _create_field_reader(self.ctx, dep_obj, '"Dependencies" field in Gem in ' + self.path)
            self.dependencies.append(dep_reader.uuid())

        self.link_type = reader.field_req('LinkType')
        if self.link_type not in Gem.LinkType.Types:
            self.ctx.cry_error('Gem.json file\'s "LinkType" value "{}" is invalid, please supply one of:\n{}'.format(self.link_type, Gem.LinkType.Types))

        self.is_game_gem = reader.field_opt('IsGameGem', False)
        self.is_required = reader.field_opt('IsRequired', False)

    def _upgrade_gem_json(self, gem_def):
        """
        If gems file is in an old format, update the data within.
        """
        reader = _create_field_reader(self.ctx, gem_def, 'Gem in ' + self.path)

        latest_format_version = 3
        gem_format_version = reader.field_int('GemFormatVersion')

        # Can't upgrade ancient versions or future versions
        if (gem_format_version < 2 or gem_format_version > latest_format_version):
            reader.ctx.cry_error(
                'Field "GemsFormatVersion" is {}, not expected version {}. Please update your Gem file in {}.'.format(
                    gem_format_version, latest_format_version, self.path))

        # v2 -> v3
        if (gem_format_version < 3):
            gem_def['IsLegacyIGem'] = True

        # File is now up to date
        gem_def['GemFormatVersion'] = latest_format_version

    def _get_output_name(self):
        return 'Gem.{}.{}.v{}'.format(self.name, self.id.hex, str(self.version))

    def _get_editor_output_name(self):
        return 'Gem.{}.Editor.{}.v{}'.format(self.name, self.id.hex, str(self.version))

    def get_include_path(self):
        code_root_node = self.ctx.srcnode.make_node(self.path).make_node(GEMS_CODE_FOLDER)
        cased_include_dir = 'Include' # default casing is upper

        # some gems have lower case include directories
        if os.path.exists(code_root_node.abspath()):
            dir_contents = os.listdir(code_root_node.abspath())
            if 'include' in dir_contents:
                cased_include_dir = 'include'

        return code_root_node.make_node(cased_include_dir)

    def _get_spec(self):
        return (self.id, self.version, self.path)
    spec = property(_get_spec)

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
        self.search_paths = set()
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
            lambda gem: check_funs[1](gem) and gem.path == spec[2],
            ]

        for gem in self.gems:
            if check_funs[len(spec) - 1](gem):
                return gem

        return None

    def contains_gem(self, *gem_spec):
        return self.get_gem_by_spec(*gem_spec) != None

    def process(self, is_server=False):
        """
        Process current directory for gems

        Note that this has to check each game project to know which gems are enabled
        and build a list of all enabled gems so that those are built.
        To debug gems output during build, use --zones=gems in your command line
        """

        this_path = self.ctx.path

        self.search_paths.add(this_path.abspath())
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
                    self.search_paths.add(new_path)

        # Load all the gems under the Gems folder to search for required gems
        self.required_gems = self.ctx.load_required_gems()

        game_projects = self.ctx.get_enabled_game_project_list()

        for game_project in game_projects:
            Logs.debug('gems: Game Project: %s' % game_project)

            gems_list_file = self.ctx.get_project_node(game_project).make_node(GEMS_SERVER_LIST_FILE if is_server else GEMS_LIST_FILE)

            if not os.path.isfile(gems_list_file.abspath()):
                if self.ctx.is_option_true('gems_optional'):
                    Logs.debug("gems: Game has no gems file, skipping [%s]" % gems_list_file)
                    continue # go to the next game
                else:
                    self.ctx.cry_error('Project {} is missing {} file.'.format(game_project, GEMS_LIST_FILE))

            Logs.debug('gems: reading gems file at %s' % gems_list_file)

            gem_info_list = self.ctx.parse_json_file(gems_list_file)
            list_reader = _create_field_reader(self.ctx, gem_info_list, 'Gems list for project ' + game_project)

            # Verify that the project file is an up-to-date format
            gem_format_version = list_reader.field_int('GemListFormatVersion')
            if gem_format_version != GEMS_FORMAT_VERSION:
                self.ctx.cry_error(
                    'Gems list file at {} is of version {}, not expected version {}. Please update your project file.'.format(
                        gems_list_file, gem_format_version, GEMS_FORMAT_VERSION))

            for idx, gem_info_obj in enumerate(list_reader.field_req('Gems')):
                # String for error reporting.
                reader = _create_field_reader(self.ctx, gem_info_obj, 'Gem {} in game project {}'.format(idx, game_project))

                gem_id = reader.uuid()
                version = reader.version()
                path = os.path.normpath(reader.field_req('Path'))

                gem = self.get_gem_by_spec(gem_id, version, path)
                if not gem:
                    Logs.debug('gems: Gem not found in cache, attempting to load from disk: ({}, {}, {})'.format(gem_id, version, path))

                    for search_path in self.search_paths:
                        def_file = os.path.join(search_path, path, GEMS_DEFINITION_FILE)
                        if not os.path.isfile(def_file):
                            continue # Try again with the next path

                        gem = Gem(self.ctx)
                        gem.path = path
                        gem.abspath = os.path.join(search_path, path)
                        gem.load_from_json(self.ctx.parse_json_file(self.ctx.root.make_node(def_file)))

                        # Validate that the Gem loaded from the path specified actually matches the id and version.
                        if gem.id != gem_id:
                            self.ctx.cry_error("Gem at path {} has ID {}, instead of ID {} specified in {}'s {}.".format(
                                path, gem.id, gem_id, game_project, GEMS_LIST_FILE))

                        if gem.version != version:
                            self.ctx.cry_error("Gem at path {} has version {}, instead of version {} specified in {}'s {}.".format(
                                path, gem.version, version, game_project, GEMS_LIST_FILE))

                        self.add_gem(gem)

                if not gem:
                    self.ctx.cry_error('Failed to load from path "{}"'.format(path))

                gem.games_enabled_in.append(game_project)

        for gem in self.gems:
            Logs.debug("gems: gem %s is used by games: %s" % (gem.name, gem.games_enabled_in))

        # Always add required gems to the gems manager
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
        if os.path.isdir(code_path):
            self.dirs.append(code_path)
        else:
            Logs.debug('gems: gem Code folder does not exist %s - this is okay if the gem contains only assets and no code' % code_path)

    def add_gem_to_spec(self, gem, spec_name):

        Logs.debug('gems: adding gem %s to spec %s' % (gem.name, spec_name))
        spec_dict = self.ctx.loaded_specs_dict[spec_name]
        if 'modules' not in spec_dict:
            spec_dict['modules'] = []
        spec_list = spec_dict['modules']
        if gem.name not in spec_list:
            spec_list.append(gem.name)

        if gem.editor_targets and self.ctx.editor_gems_enabled():
            for editor_target in gem.editor_targets:
                Logs.debug('gems: adding editor target of gem %s - %s to spec %s' % (gem.name, editor_target, spec_name))
                spec_list.append(editor_target)

    def add_to_specs(self):

        specs_to_include = self.ctx.loaded_specs() + ['gems']

        system_specs = ('gems','all')

        # Create Gems spec
        if not 'gems' in self.ctx.loaded_specs_dict:
            self.ctx.loaded_specs_dict['gems'] = dict(description="Configuration to build all Gems.",
                                                      valid_configuration=['debug', 'profile', 'release'],
                                                      valid_platforms=['win_x64', 'win_x64_vs2015', 'win_x64_vs2013'],
                                                      visual_studio_name='Gems',
                                                      modules=['CryAction', 'CryAction_AutoFlowNode'])

        for spec_name in specs_to_include:

            # Get the defined game project per spec and only add gems from game projects
            # to specs that have them defined
            game_projects = self.ctx.spec_game_projects(spec_name)

            if len(game_projects) == 0 and not self.ctx.spec_disable_games(spec_name):
                # Handle the legacy situation where the game projects is set in the enabled_game_projects in user_settings.options
                fallback_enabled_game_projects_string = self.ctx.options.enabled_game_projects.replace(' ', '').strip()
                game_projects = fallback_enabled_game_projects_string.split(',')

            if len(game_projects) == 0 and spec_name not in system_specs:
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
            ctx.gem_manager.process(ctx.cmd.endswith('_dedicated'))
        return ctx.gem_manager

@conf
def editor_gems_enabled(ctx):
    capabilities = ctx.get_enabled_capabilities()
    
    return (("compileengine" in capabilities) or ("compilesandbox" in capabilities))

@conf
def get_game_gems(ctx, game_project):
    return [gem for gem in GemManager.GetInstance(ctx).gems if game_project in gem.games_enabled_in]

@conf
def get_required_gems(ctx):
    return [gem for gem in GemManager.GetInstance(ctx).required_gems if gem.is_required]

@conf
def apply_gems_to_context(ctx, game_name, k, kw):

    """this function is called whenever its about to generate a target that should be aware of gems"""
    game_gems = ctx.get_game_gems(game_name)

    Logs.debug('gems: adding gems to %s (%s_%s) : %s' % (game_name, ctx.env['CONFIGURATION'], ctx.env['PLATFORM'], [gem.name for gem in game_gems]))

    kw['includes']   += [gem.get_include_path() for gem in game_gems]
    kw['use']        += [gem.name for gem in game_gems if Gem.LinkType.requires_linking(ctx, gem.link_type)]


@conf
def apply_required_gems_to_context(ctx, target, kw):

    required_gems = ctx.get_required_gems()

    Logs.debug('gems: adding required gems to target {} : %s'.format(target, ','.join([gem.name for gem in required_gems])))

    for gem in required_gems:
        if gem.name != target:
            append_to_unique_list(kw['includes'], gem.get_include_path().abspath())
            if Gem.LinkType.requires_linking(ctx, gem.link_type):
                append_to_unique_list(kw['use'], gem.name)

@conf
def is_gem(ctx, module_name):
    for gem in GemManager.GetInstance(ctx).gems:
        if module_name == gem.name or module_name == gem.editor_module:
            return True
    return False


@conf
def scan_required_gems(ctx):
    """
    Scan the folder for any required gems

    :param ctx: Configuration Context
    """

    required_gems = []

    def _search_for_gem(base_path, folder):
        content_path = base_path.make_node(folder)
        if os.path.exists(os.path.join(content_path.abspath(), GEMS_DEFINITION_FILE)):
            # Use the standard gem config loader to determine if the gem is required or not
            gem = Gem(ctx)
            gem.path = '{}/{}'.format(base_path,folder)
            gems_config_file = content_path.make_node(GEMS_DEFINITION_FILE)
            gem.load_from_json(ctx.parse_json_file(gems_config_file))
            if gem.is_required:
                required_gems.append(gem.path)
            # Return true if a gem folder was processed (ie contains a gem.json, regardless of whether its required or not)
            return True
        else:
            # Return false if the folder is not a gem folder.  We may need to recurse
            return False

    def _search_for_gem_folders(base_path):
        gem_folders = base_path.listdir()
        for gem_folder in gem_folders:
            if not _search_for_gem(base_path, gem_folder) and os.path.isdir(gem_folder):
                _search_for_gem_folders(base_path.make_node(gem_folder))

    # Build search paths (which include config search paths, and root/Gems)
    manager = GemManager.GetInstance(ctx)
    search_paths = set(manager.search_paths)
    search_paths.remove(ctx.path.abspath())
    search_paths.add(ctx.path.find_node('Gems').abspath())
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
    if REQUIRED_GEMS_CACHE is None:
        try:
            required_gems = []
            required_gems_location = ctx.scan_required_gems()
            for required_gem_location in required_gems_location:
                required_gem_path = ctx.path.make_node(str(required_gem_location))
                required_gem_def_file_path = required_gem_path.make_node(GEMS_DEFINITION_FILE)
                gem = Gem(ctx)
                gem.path = str(required_gem_location)
                gem.load_from_json(ctx.parse_json_file(required_gem_def_file_path))
                if gem.is_required:
                    required_gems.append(gem)
            REQUIRED_GEMS_CACHE = required_gems
        except Exception as e:
            Logs.error('[ERROR] Unable to determine any required Gems ({})'.format(e.message))

    return REQUIRED_GEMS_CACHE
