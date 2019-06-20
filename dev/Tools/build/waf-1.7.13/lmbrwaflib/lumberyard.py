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
import sys
import argparse
import inspect
import random
import zlib
import json
import re
import traceback

from waflib import Logs, Options, Utils, Configure, ConfigSet, Node, Errors
from waflib.TaskGen import taskgen_method
from waflib.Configure import conf, conf_event, ConfigurationContext, deprecated
from waflib.Build import BuildContext, CleanContext, Context

from waf_branch_spec import SUBFOLDERS, VERSION_NUMBER_PATTERN, BINTEMP_MODULE_DEF, \
                            LUMBERYARD_ENGINE_PATH, ADDITIONAL_WAF_MODULES, BINTEMP_FOLDER, \
                            AVAILABLE_LAUNCHERS
from cryengine_modules import SANITIZE_INPUTS
from cry_utils import append_kw_entry, append_to_unique_list
from utils import parse_json_file, convert_roles_to_setup_assistant_description, write_json_file
from third_party import ThirdPartySettings

UNSUPPORTED_PLATFORM_CONFIGURATIONS = set()

WAF_BASE_SCRIPT = os.path.normpath("{}/Tools/build/waf-1.7.13/lmbr_waf".format(LUMBERYARD_ENGINE_PATH))

CURRENT_WAF_EXECUTABLE = '"{}" "{}"'.format(sys.executable, WAF_BASE_SCRIPT)

WAF_TOOL_ROOT_DIR = os.path.join(LUMBERYARD_ENGINE_PATH, "Tools", "build","waf-1.7.13")
LMBR_WAF_TOOL_DIR = os.path.join(WAF_TOOL_ROOT_DIR, "lmbrwaflib")
GENERAL_WAF_TOOL_DIR = os.path.join(WAF_TOOL_ROOT_DIR, "waflib")

# successful configure generates these timestamps
CONFIGURE_TIMESTAMP_FILES = ['bootstrap.timestamp','SetupAssistantConfig.timestamp', 'android_settings.timestamp', 'environment.timestamp']
# if configure is run explicitly, delete these files to force them to rerun
CONFIGURE_FORCE_TIMESTAMP_FILES = ['generate_uber_files.timestamp', 'project_gen.timestamp']
# The prefix for file aliasing 3rd party paths (ie @3P:foo@)
FILE_ALIAS_3P_PREFIX = "3P:"

# List of extra non-build (non project building) commands.
NON_BUILD_COMMANDS = [
    'generate_uber_files',
    'generate_module_def_files',
    'generate_module_dependency_files',
    'generate_game_project',
    'msvs',
    'eclipse',
    'android_studio',
    'xcode_ios',
    'xcode_appletv',
    'xcode_mac'
]

REGEX_PATH_ALIAS = re.compile('@(.+)@')

# Table of lmbr waf modules that need to be loaded by the root waf script, grouped by tooldir where the modules need to be loaded from.
# The module can be restricted to an un-versioned system platform value (win32, darwin, linux) by adding a ':' + the un-versioned system platform value.
# The '.' key represents the default waf tooldir (Code/Tools/waf-1.7.13/waflib)
LMBR_WAFLIB_MODULES = [

    ( GENERAL_WAF_TOOL_DIR , [
        'c_config',
        'c',
        'cxx',
        'c_osx:darwin',
        'winres:win32'
    ] ) ,
    ( LMBR_WAF_TOOL_DIR , [
        'settings_manager',
        # Put the deps modules first as they wrap/intercept c/cxx calls to
        # capture dependency information being written out to stderr. If
        # another module declares a subclass of a c/cxx feature, like qt5 does,
        # and it is before these modules then the subclass/feature does not get
        # wrapped and the header dependency information is printed to the
        # screen.
        'clangdeps',
        'msvs:win32', # msvcdeps implicitly depends on this module, so load it first
        'msvcdeps:win32',

        'cry_utils',
        'project_settings',
        'build_configurations',
        'branch_spec',
        'copy_tasks',
        'lumberyard_sdks',
        'lumberyard_modules',
        'gems',
        'az_code_generator',
        'crcfix',
        'doxygen',

        'lmbr_install_context',
        'packaging',
        'deploy',

        'generate_uber_files',          # Load support for Uber Files
        'generate_module_dependency_files',  # Load support for Module Dependency Files (Used by ly_integration_toolkit_utils.py)

        # Visual Studio support
        'msvs_override_handling:win32',
        'mscv_helper:win32',
        'vscode:win32',

        'xcode:darwin',
        'eclipse:linux',

        'gui_tasks:win32',
        'gui_tasks:darwin',

        'qt5',

        'third_party',
        'cry_utils',
        'winres',

        'bootstrap',
        'lmbr_setup_tools'
    ])
]

#
LMBR_WAFLIB_DATA_DRIVEN_MODULES = [
    (LMBR_WAF_TOOL_DIR , [
        'default_settings',
        'cryengine_modules',
        'incredibuild'
    ])
]


LAST_ENGINE_BUILD_VERSION_TAG_FILE = "engine.version"
def load_lmbr_waf_modules(conf, module_table):
    """
    Load a list of modules (with optional platform restrictions)

    :param module_list: List of modules+platform restrictions to load
    """
    host_platform = Utils.unversioned_sys_platform()

    for tool_dir, module_list in module_table:

        for lmbr_waflib_module in module_list:

            if ':' in lmbr_waflib_module:
                module_platform = lmbr_waflib_module.split(':')
                module = module_platform[0]
                restricted_platform = module_platform[1]
                if restricted_platform != host_platform:
                    continue
            else:
                module = lmbr_waflib_module
            try:
                if tool_dir == GENERAL_WAF_TOOL_DIR:
                    conf.load(module)
                else:
                    conf.load(module, tooldir=[tool_dir])
            except SyntaxError as syntax_err:
                conf.fatal("[Error] Unable to load required module '{}.py: {} (Line:{}, Offset:{})'".format(module, syntax_err.msg, syntax_err.lineno, syntax_err.offset))
            except Exception as e:
                conf.fatal("[Error] Unable to load required module '{}.py: {}'".format(module, e))


@conf
def load_lmbr_general_modules(conf):
    """
    Load all of the required waf lmbr modules
    :param conf:    Configuration Context
    """
    load_lmbr_waf_modules(conf, LMBR_WAFLIB_MODULES)

    load_lmbr_waf_modules(conf, ADDITIONAL_WAF_MODULES)

@conf
def load_lmbr_data_driven_modules(conf):
    """
    Load all of the data driven waf lmbr modules
    :param conf:    Configuration Context
    """
    load_lmbr_waf_modules(conf, LMBR_WAFLIB_DATA_DRIVEN_MODULES)

    # Inspect all of the valid/enabled platforms and load the additional modules if needed
    additional_modules_to_load = []
    additional_java_modules_to_load = []
    platforms_that_need_java = []
    all_platform_names = conf.get_all_platform_names()
    needs_java = False
    for platform_name in all_platform_names:
        platform_settings = conf.get_platform_settings(platform_name)
        if platform_settings.enabled and platform_settings.additional_modules:

            # Make sure the module is valid
            for additional_module in platform_settings.additional_modules:
                if not os.path.isfile(os.path.join(WAF_TOOL_ROOT_DIR, '{}.py'.format(additional_module))):
                    raise Errors.WafError("Invalid module '{}' specified in platform settings for platform '{}'".format(additional_module, platform_name))

            # Add the listed modules
            append_to_unique_list(additional_modules_to_load, platform_settings.additional_modules)
            # Check if this particular platform 'needs' java
            if platform_settings.needs_java:
                platforms_that_need_java.append(platform_name)
                needs_java = True
                # Tag the modules that 'needs' java so we can determine if we need to abort their loading if we
                # have issues loading the javaw module
                append_to_unique_list(additional_java_modules_to_load, platform_settings.additional_modules)

    java_loaded = False
    if needs_java and hasattr(conf, 'env'):
        # We need to validate the JDK path from SetupAssistant before loading the javaw tool.
        # This way we don't introduce a dependency on lmbrwaflib in the core waflib.
        jdk_home = conf.get_env_file_var('LY_JDK', required=True, silent=True)
        if not jdk_home:
            # Disable all of the java required platforms
            for disable_java_required_platform in platforms_that_need_java:
                conf.disable_target_platform(disable_java_required_platform)
            
            conf.warn_once('Missing JDK path from Setup Assistant detected. '
                           'Target platforms that require java will be disabled. '
                           'Please re-run Setup Assistant with "Compile For Android" '
                           'enabled and run the configure command again.')
        else:
            conf.env['JAVA_HOME'] = jdk_home
            conf.load('javaw')
            java_loaded = True
            
    setattr(conf, 'javaw_loaded', java_loaded)

    for additional_module_to_load in additional_modules_to_load:
        if not java_loaded and needs_java and hasattr(conf, 'env') and additional_module_to_load in additional_java_modules_to_load:
            # If we were not able to load java when needed, skip all modules that were tagged as needing java (only for non-Option passes)
            continue

        # Do not load android module if ENABLE_ANDROID is false in environment.json
        enable_android = conf.get_env_file_var('ENABLE_ANDROID', required=True, silent=True)
        if enable_android == 'False' and 'android' in additional_module_to_load.lower():
            continue

        if '/' in additional_module_to_load:
            base_dir = os.path.dirname(additional_module_to_load)
            module_name = os.path.basename(additional_module_to_load)
            tool_dir = os.path.normpath(os.path.join(WAF_TOOL_ROOT_DIR, base_dir))
            conf.load(module_name, tooldir=tool_dir)
        else:
            conf.load(additional_module_to_load)
        
    # Once all the modules are loaded, we can report any settings overrides as needed
    if isinstance(conf, BuildContext):
        platform, configuration = conf.get_platform_and_configuration()
        if platform:
            conf.register_output_folder_settings_reporter(platform)
    
    conf.report_settings_overrides()



@conf
def check_module_load_options(conf):

    if hasattr(check_module_load_options, 'did_check'):
        return

    check_module_load_options.did_check = True

    crash_reporter_file = 'crash_reporting'
    crash_reporter_path = os.path.join(LMBR_WAF_TOOL_DIR,crash_reporter_file + '.py')

    if conf.options.external_crash_reporting:
        if os.path.exists(crash_reporter_path):
            conf.load(crash_reporter_file, tooldir=[LMBR_WAF_TOOL_DIR])


@conf
def add_lmbr_waf_options(opt, az_test_supported):
    """
    Add custom lumberyard options

    :param opt:                 The Context to add the options to
    :param az_test_supported:   Flag that indicates if az_test is supported
    """
    ###########################################
    # Add custom cryengine options
    opt.add_option('-p', '--project-spec', dest='project_spec', action='store', default='', help='Spec to use when building the project')
    opt.add_option('--profile-execution', dest='profile_execution', action='store', default='', help='!INTERNAL ONLY! DONT USE')
    opt.add_option('--profile-output', dest='profile_output', action='store', default='', help='!INTERNAL ONLY! DONT USE')
    opt.add_option('--task-filter', dest='task_filter', action='store', default='', help='!INTERNAL ONLY! DONT USE')
    opt.add_option('--update-settings', dest='update_user_settings', action='store', default='False',
                   help='Option to update the user_settings.options file with any values that are modified from the command line')
    opt.add_option('--copy-3rd-party-pdbs', dest='copy_3rd_party_pdbs', action='store', default='False',
                   help='Option to copy pdbs from 3rd party libraries for debugging.  Warning: This will increase the memory usage of your visual studio development environmnet.')

    # Add special command line option to prevent recursive execution of WAF
    opt.add_option('--internal-dont-check-recursive-execution', dest='internal_dont_check_recursive_execution', action='store', default='False', help='!INTERNAL ONLY! DONT USE')
    opt.add_option('--internal-using-ib-dta', dest='internal_using_ib_dta', action='store', default='False', help='!INTERNAL ONLY! DONT USE')

    # Add options primarily used by the Visual Studio WAF Addin
    waf_addin_group = opt.add_option_group('Visual Studio WAF Addin Options')
    waf_addin_group.add_option('-a', '--ask-for-user-input', dest='ask_for_user_input', action='store', default='False', help='Disables all user prompts in WAF')
    waf_addin_group.add_option('--file-filter', dest='file_filter', action='store', default="", help='Only compile files matching this filter')
    waf_addin_group.add_option('--show-includes', dest='show_includes', action='store', default='False', help='Show all files included (requires a file_filter)')
    waf_addin_group.add_option('--show-preprocessed-file', dest='show_preprocessed_file', action='store', default='False', help='Generate only Preprocessor Output (requires a file_filter)')
    waf_addin_group.add_option('--show-disassembly', dest='show_disassembly', action='store', default='False', help='Generate only Assembler Output (requires a file_filter)')
    waf_addin_group.add_option('--use-asan', dest='use_asan', action='store', default='False', help='Enables the use of /GS or AddressSanitizer, depending on platform')
    waf_addin_group.add_option('--use-aslr', dest='use_aslr', action='store', default='False', help='Enables the use of Address Space Layout Randomization, if supported on target platform')

    # DEPRECATED OPTIONS, only used to keep backwards compatibility
    waf_addin_group.add_option('--output-file', dest='output_file', action='store', default="", help='*DEPRECATED* Specify Output File for Disassemly or Preprocess option (requieres a file_filter)')
    waf_addin_group.add_option('--use-overwrite-file', dest='use_overwrite_file', action='store', default="False",
                               help='*DEPRECATED* Use special BinTemp/lmbr_waf.configuration_overwrites to specify per target configurations')
    waf_addin_group.add_option('--console-mode', dest='console_mode', action='store', default="True", help='No Gui. Display console only')

    # Test specific options
    if az_test_supported:
        test_group = opt.add_option_group('AzTestScanner Options')
        test_group.add_option('--test-params', dest='test_params', action='store', default='', help='Test parameters to send to the scanner (encapsulate with quotes)')


@conf
def run_bootstrap(conf):
    """
    Execute the bootstrap process

    :param conf:                Configuration context
    """
    # Deprecated

    # Bootstrap is only ran within the engine folder
    if not conf.is_engine_local():
        return

    conf.run_linkless_bootstrap()


@conf
def clear_waf_timestamp_files(conf):
    """
    Remove timestamp files to force builds of generate_uber_files and project gen even if
    some command after configure failes

    :param conf:
    """

    # Remove timestamp files to force builds even if some command after configure fail
    force_timestamp_files = CONFIGURE_FORCE_TIMESTAMP_FILES
    for force_timestamp_file in force_timestamp_files:
        try:
            force_timestamp_node = conf.get_bintemp_folder_node().make_node(force_timestamp_file)
            os.stat(force_timestamp_node.abspath())
        except OSError:
            pass
        else:
            force_timestamp_node.delete()

    # Add timestamp files for files generated by configure
    check_timestamp_files = CONFIGURE_TIMESTAMP_FILES
    for check_timestamp_file in check_timestamp_files:
        check_timestamp_node = conf.get_bintemp_folder_node().make_node(check_timestamp_file)
        if not os.path.exists(check_timestamp_node.abspath()):
            check_timestamp_node.write('')
        else:
            os.utime(check_timestamp_node.abspath(), None)

@conf
def validate_build_command(bld):
    """
    Validate the build command and build context

    :param bld: The BuildContext
    """

    # Only Build Context objects are valid here
    if not isinstance(bld, BuildContext):
        bld.fatal("[Error] Invalid build command: '{}'.  Type in '{} --help' for more information"
                  .format(bld.cmd if hasattr(bld, 'cmd') else str(bld), CURRENT_WAF_EXECUTABLE))

    # If this is a build or clean command, see if this is an unsupported platform+config
    if bld.cmd.startswith('build_'):
        bld_clean_platform_config_key = bld.cmd[6:]
    elif bld.cmd.startswith('clean_'):
        bld_clean_platform_config_key = bld.cmd[5:]
    else:
        bld_clean_platform_config_key = None

    if bld_clean_platform_config_key is not None:
        if bld_clean_platform_config_key in UNSUPPORTED_PLATFORM_CONFIGURATIONS:
            # This is not a supported build_ or  clean_ operation for a platform + configuration
            raise conf.fatal("[Error] This is an unsupported platform and configuration")

    # Check if a valid spec is defined for build commands
    if bld.cmd not in NON_BUILD_COMMANDS:

        # project spec is a mandatory parameter for build commands that perform monolithic builds
        if bld.is_build_monolithic():
            Logs.debug('lumberyard: Processing monolithic build command ')
            # For monolithic builds, the project spec is required
            if bld.options.project_spec == '':
                bld.fatal('[ERROR] No Project Spec defined. Project specs are required for monolithic builds.  Use "-p <spec_name>" command line option to specify project spec. Valid specs are "%s". ' % bld.loaded_specs())

            # Ensure that the selected spec is supported on this platform
            if not bld.options.project_spec in bld.loaded_specs():
                bld.fatal('[ERROR] Invalid Project Spec (%s) defined, valid specs are %s' % (bld.options.project_spec, bld.loaded_specs()))

        # Check for valid single file compilation flag pairs
        if bld.is_option_true('show_preprocessed_file') and bld.options.file_filter == "":
            bld.fatal('--show-preprocessed-file can only be used in conjunction with --file-filter')
        elif bld.is_option_true('show_disassembly') and bld.options.file_filter == "":
            bld.fatal('--show-disassembly can only be used in conjunction with --file-filter')

    # android armv8 api level validation
    if ('android_armv8' in bld.cmd) and (not bld.is_android_armv8_api_valid()):
        bld.fatal('[ERROR] Attempting to build Android ARMv8 against an API that is lower than the min spec: API 21.')

    # Validate the game project version
    if VERSION_NUMBER_PATTERN.match(bld.options.version) is None:
        bld.fatal("[Error] Invalid game version number format ({})".format(bld.options.version))


@conf
def check_special_command_timestamps(bld):
    """
    Check timestamps for special commands and see if the current build needs to be short circuited

    :param bld: The BuildContext
    :return: False to short circuit to restart the command chain, True if not and continue the build process
    """
    # Only Build Context objects are valid here
    if not isinstance(bld, BuildContext):
        bld.fatal("[Error] Invalid build command: '{}'.  Type in '{} --help' for more information"
                  .format(bld.cmd if hasattr(bld, 'cmd') else str(bld), CURRENT_WAF_EXECUTABLE))

    if not 'generate_uber_files' in Options.commands and bld.cmd != 'generate_uber_files':
        generate_uber_files_timestamp = bld.get_bintemp_folder_node().make_node('generate_uber_files.timestamp')
        try:
            os.stat(generate_uber_files_timestamp.abspath())
        except OSError:
            # No generate_uber file timestamp, restart command chain, prefixed with gen uber files cmd
            Options.commands = ['generate_uber_files'] + [bld.cmd] + Options.commands
            return False

    return True


@conf
def prepare_build_environment(bld):
    """
    Prepare the build environment for the current build command and setup optional additional commands if necessary

    :param bld: The BuildContext
    """

    if bld.cmd in NON_BUILD_COMMANDS:
        bld.env['PLATFORM'] = 'project_generator'
        bld.env['CONFIGURATION'] = 'project_generator'
    else:
        if not bld.variant:
            bld.fatal("[ERROR] Invalid build variant.  Please use a valid build configuration. "
                      "Type in '{} --help' for more information".format(CURRENT_WAF_EXECUTABLE))

        platform, configuration = bld.get_platform_and_configuration()
        bld.env['PLATFORM'] = platform
        bld.env['CONFIGURATION'] = configuration

        # Inspect the details of the current platform
        if not bld.is_target_platform_enabled(platform):
            bld.fatal("[ERROR] Target platform '{}' not supported. [on host platform: {}]".format(platform, Utils.unversioned_sys_platform()))
            
        # make sure the android launchers are included in the build
        if bld.check_platform_explicit_boolean_attribute(platform, 'is_android'):
            android_path = os.path.join(bld.path.abspath(), bld.get_android_project_relative_path(), 'wscript')
            if not os.path.exists(android_path):
                bld.fatal('[ERROR] Android launchers not correctly configured. Run \'configure\' again')
            SUBFOLDERS.append(bld.get_android_project_absolute_path())

        # If a spec was supplied, check for platform limitations
        if bld.options.project_spec:
            validated_platforms = bld.preprocess_target_platforms(bld.spec_platforms(), True)
            if platform not in validated_platforms:
                bld.fatal('[ERROR] Target platform "{}" not supported for spec {}'.format(platform, bld.options.project_spec))
            validated_configurations = bld.preprocess_target_configuration_list(None, platform, bld.spec_configurations(), True)
            if configuration not in validated_configurations:
                bld.fatal('[ERROR] Target configuration "{}" not supported for spec {}'.format(configuration, bld.options.project_spec))

        # command chaining for packaging and deployment
        if (getattr(bld.options, 'project_spec', '') != '3rd_party'):
            
            if bld.cmd.startswith('build'):
                package_cmd = 'package_{}_{}'.format(platform, configuration)
                if bld.is_option_true('package_projects_automatically') and (package_cmd not in Options.commands):
                    Options.commands.append(package_cmd)

            elif bld.cmd.startswith('package'):
                deploy_option = 'deploy_{}'.format(bld.platform_to_platform_alias(platform))
                if hasattr(bld.options, deploy_option):
                    deploy_cmd = 'deploy_{}_{}'.format(platform, configuration)
                    if bld.is_option_true(deploy_option) and (deploy_cmd not in Options.commands):
                        Options.commands.append(deploy_cmd)


@conf
def configure_game_projects(conf):

    for project in conf.game_projects():
        legacy_game_code_folder = conf.legacy_game_code_folder(project)
        # If there exists a code_folder attribute in the project configuration, then see if it can be included in the build
        if legacy_game_code_folder is not None:
            must_exist = project in conf.get_enabled_game_project_list()
            # projects that are not currently enabled don't actually cause an error if their code folder is missing - we just don't compile them!
            try:
                conf.recurse(legacy_game_code_folder, mandatory=True)
            except TypeError:
                raise
            except Exception as err:
                if must_exist:
                    conf.fatal(
                        '[ERROR] Project "%s" is enabled, but failed to execute its wscript at (%s) - consider disabling it in your _WAF_/user_settings.options file.\nException: %s' % (
                            project, legacy_game_code_folder, err))


@conf
def setup_game_projects(bld):
    """
    Setup the game projects for non-build commands

    :param bld:     The BuildContext
    """

    previous_game_project = bld.game_project
    # Load Game Specific parts, but only if the current spec is a game...
    if bld.env['CONFIGURATION'] == 'project_generator' or not bld.spec_disable_games():
        # If this bld command is to generate the use maps, then recurse all of the game projects, otherwise
        # only recurse the enabled game project
        game_project_list = bld.game_projects() if bld.cmd in ('generate_module_def_files', 'generate_module_dependency_files') \
            else bld.get_enabled_game_project_list()
        for project in game_project_list:

            bld.game_project = project
            legacy_game_code_folder = bld.legacy_game_code_folder(project)
            # If there exists a code_folder attribute in the project configuration, then see if it can be included in the build
            if legacy_game_code_folder is not None:
                must_exist = project in bld.get_enabled_game_project_list()
                # projects that are not currently enabled don't actually cause an error if their code folder is missing - we just don't compile them!
                try:
                    bld.recurse(legacy_game_code_folder, mandatory=True)
                except Exception as err:
                    if must_exist:
                        bld.fatal('[ERROR] Project "%s" is enabled, but failed to execute its wscript at (%s) - consider disabling it in your _WAF_/user_settings.options file.\nException: %s' % (
                        project, legacy_game_code_folder, err))

    elif bld.env['CONFIGURATION'] != 'project_generator':
        Logs.debug('lumberyard:  Not recursing into game directories since no games are set up to build in current spec')

    # restore project in case!
    bld.game_project = previous_game_project

LMBR_WAF_CONFIG_FILENAME = 'lmbr_waf.json'

@conf
def calculate_engine_path(ctx):
    """
    Determine the engine root path from SetupAssistantUserPreferences. if it exists

    :param conf     Context
    """

    def _make_engine_node_lst(node):
        engine_node_lst = []
        cur_node = node
        while cur_node.parent:
            if ' ' in cur_node.name:
                engine_node_lst.append(cur_node.name.replace(' ', '_'))
            else:
                engine_node_lst.append(cur_node.name)
            cur_node = cur_node.parent
        engine_node_lst.reverse()
        # For win32, remove the ':' if the node represents a drive letter
        if engine_node_lst and Utils.is_win32 and len(engine_node_lst[0]) == 2 and engine_node_lst[0].endswith(':'):
            engine_node_lst[0] = engine_node_lst[0][0]
        return engine_node_lst

    # Default the engine path, node, and version
    ctx.engine_node = ctx.path
    ctx.engine_path = ctx.path.abspath()
    ctx.engine_root_version = '0.0.0.0'
    ctx.engine_node_list = _make_engine_node_lst(ctx.engine_node)

    engine_json_file_path = ctx.path.make_node('engine.json').abspath()
    if os.path.exists(engine_json_file_path):
        engine_json = parse_json_file(engine_json_file_path)
        ctx.engine_root_version = engine_json.get('LumberyardVersion',ctx.engine_root_version)
        if 'ExternalEnginePath' in engine_json:
            external_engine_path = engine_json['ExternalEnginePath']
            if os.path.isabs(external_engine_path):
                engine_root_abs = external_engine_path
            else:
                engine_root_abs = os.path.normpath(os.path.join(ctx.path.abspath(),external_engine_path))
            if not os.path.exists(engine_root_abs):
                ctx.fatal('[ERROR] Invalid external engine path in engine.json : {}'.format(engine_root_abs))
            if os.path.normcase(engine_root_abs)!=os.path.normcase(ctx.engine_path):
                ctx.engine_node = ctx.root.make_node(engine_root_abs)
                ctx.engine_path = engine_root_abs
                ctx.engine_node_list = _make_engine_node_lst(ctx.engine_node)

    if ctx.engine_path != ctx.path.abspath():
        last_build_engine_version_node = ctx.get_bintemp_folder_node().make_node(LAST_ENGINE_BUILD_VERSION_TAG_FILE)
        if os.path.exists(last_build_engine_version_node.abspath()):
            last_built_version = last_build_engine_version_node.read()
            if last_built_version!= ctx.engine_root_version:
                Logs.warn('[WARN] The current engine version ({}) does not match the last version {} that this project was built against'.format(ctx.engine_root_version,last_built_version))
                last_build_engine_version_node.write(ctx.engine_root_version)

    ctx.register_script_env_var("LUMBERYARD_BUILD", ctx.get_lumberyard_build())

@conf
def is_engine_local(conf):
    """
    Determine if we are within an engine path
    :param conf: The configuration context
    :return: True if we are running from the engine folder, False if not
    """
    current_dir = os.path.normcase(os.path.abspath(os.path.curdir))
    engine_dir = os.path.normcase(os.path.abspath(conf.engine_path))
    return current_dir == engine_dir


@conf
def get_enabled_capabilities(ctx):
    """
    Get the capabilities that were set through setup assistant
    :param ctx: Context
    :return: List of capabilities that were set by the setup assistant
    """

    try:
        return ctx.parsed_capabilities
    except AttributeError:
        pass

    raw_capability_string = getattr(ctx.options,'bootstrap_tool_param',None)

    if raw_capability_string:
        capability_parser = argparse.ArgumentParser()
        capability_parser.add_argument('--enablecapability', '-e', action='append')
        params = [token.strip() for token in raw_capability_string.split()]
        parsed_params = capability_parser.parse_known_args(params)[0]
        parsed_capabilities = getattr(parsed_params, 'enablecapability', [])
    else:
        host_platform = Utils.unversioned_sys_platform()

        # If bootstrap-tool-param is not set, that means setup assistant needs to be ran or we use default values
        # if the source for setup assistant is available but setup assistant has not been built yet

        base_lmbr_setup_path = os.path.join(ctx.engine_path, 'Tools', 'LmbrSetup')
        setup_assistant_code_path = os.path.join(ctx.engine_path, 'Code', 'Tools', 'AZInstaller')
        setup_assistant_spec_path = os.path.join(ctx.engine_path, '_WAF_', 'specs', 'lmbr_setup_tools.json')

        # Use the defaults if this hasnt been set yet
        parsed_capabilities = ['compilegame', 'compileengine', 'compilesandbox']
        if host_platform == 'darwin':
            setup_assistant_name = 'SetupAssistant'
            lmbr_setup_platform = 'Mac'
            parsed_capabilities.append('compileios')
            parsed_capabilities.append('macOS')
            minimal_setup_assistant_build_command = './lmbr_waf.sh build_darwin_x64_profile -p lmbr_setup_tools'
        elif host_platform == 'win32':
            setup_assistant_name = 'SetupAssistant.exe'
            lmbr_setup_platform = 'Win'
            parsed_capabilities.append('windows')
            parsed_capabilities.append('vc141')
            parsed_capabilities.append('vc140')
            parsed_capabilities.append('vc120')
            minimal_setup_assistant_build_command = 'lmbr_waf build_win_x64_vs2017_profile -p lmbr_setup_tools'
        else:
            lmbr_setup_platform = 'Linux'
            setup_assistant_name = 'SetupAssistantBatch'
            parsed_capabilities.append('setuplinux')
            minimal_setup_assistant_build_command = './lmbr_waf.sh build_linux_x64_profile -p lmbr_setup_tools'

        setup_assistant_binary_path = os.path.join(base_lmbr_setup_path, lmbr_setup_platform, setup_assistant_name)

        setup_assistant_code_exists = os.path.exists(os.path.join(setup_assistant_code_path, 'wscript'))

        if not os.path.exists(setup_assistant_binary_path):
            # Setup assistant binary does not exist
            if not setup_assistant_code_exists or not os.path.exists(setup_assistant_spec_path):
                # The Setup Assistant binary does not exist and the source to build it does not exist.  We cannot continue
                raise Errors.WafError('[ERROR] Unable to locate the SetupAssistant application required to configure the build '
                                      'settings for the project. Please contact support for assistance.')
            else:
                # If the source code exists, setup assistant will need to enable the minim capabilities in order to build it from source
                ctx.warn_once('Defaulting to the minimal build options.  Setup Assistant needs to be built and run to configure specific build options.')
                ctx.warn_once('To build Setup Assistant, run "{}" from the command line after the configure is complete'.format(minimal_setup_assistant_build_command))
        else:
            if setup_assistant_code_exists:
                # If the setup assistant binary exists, and the source code exists, default to the minimal build capabilities needed to build setup assistant but warn
                # the user
                ctx.warn_once('Defaulting to the minimal build options.  Setup Assistant needs to be ran to configure specific build options.')
            else:
                # The setup assistant binary exists but setup assistant hasnt been run yet.  Halt the process and inform the user
                raise Errors.WafError('[ERROR] Setup Assistant has not configured the build settings for the project yet.  You must run the tool ({}) and configure the build options first.'.format(setup_assistant_binary_path))

    setattr(ctx, 'parsed_capabilities', parsed_capabilities)

    return parsed_capabilities


###############################################################################
@conf
def CreateRootRelativePath(self, path):
    """
    Generate a path relative from the root
    """
    result_path = self.engine_node.make_node(path)
    return result_path.abspath()


#
# Unique Target ID Management
#
UID_MAP_TO_TARGET = {}

# Start the UID from a minimum value to prevent collision with the previously used 'idx' values in the generated
# names
MIN_UID_VALUE = 1024

# Constant value to cap the upper limit of the target uid.  The target uid will become part of intermediate constructed
# files Increase this number if there are more chances of collisions with target names.  Reduce if we start getting
# issues related to path lengths caused by the appending of the number
MAX_UID_VALUE = 9999999


def apply_target_uid_range(uid):
    """
    Apply the uid range restrictions rule based on the min and max uid values
    :param uid:     The raw uid to apply to
    :return:    The applied range on the input uid
    """
    return ((uid & 0xffffffff) + MIN_UID_VALUE) % MAX_UID_VALUE


def find_unique_target_uid():
    """
    Simple function to find a unique number for the target uid
    """
    global UID_MAP_TO_TARGET
    random_retry = 1024
    random_unique_id = apply_target_uid_range(random.randint(MIN_UID_VALUE, MAX_UID_VALUE))
    while random_retry > 0 and random_unique_id in UID_MAP_TO_TARGET:
        random_unique_id = random.randint(MIN_UID_VALUE, MAX_UID_VALUE)
        random_retry -= 1

    if random_retry == 0:
        raise Errors.WafError("Unable to generate a unique target id.  Current ids are : {}".format(
            ",".join(UID_MAP_TO_TARGET.keys())))
    return random_unique_id


def derive_target_uid(target_name):
    """
    Derive a target uid based on the target name if possible.  If a collision occurs, raise a WAF Error
    indicating the collision and suggest a unique id that doesnt collide
    :param target_name: The target name to derive the target uid from
    :return: The derived target uid based on the name if possible.
    """
    global UID_MAP_TO_TARGET

    # Calculate a deterministic UID to this target initially based on the CRC32 value of the target name
    uid = apply_target_uid_range(zlib.crc32(target_name))
    if uid in UID_MAP_TO_TARGET and UID_MAP_TO_TARGET[uid] != target_name:
        raise Errors.WafError(
            "[ERROR] Unable to generate a unique target id for target '{}' due to collision with target '{}'. "
            "Please update the target definition for target '{}' to include an override target_uid keyword (ex: {}) )"
            "Or consider increasing the MAX_UID_VALUE defined in {}"
                .format(target_name, UID_MAP_TO_TARGET[uid], target_name, find_unique_target_uid(), __file__))
    UID_MAP_TO_TARGET[uid] = target_name
    return uid


def validate_override_target_uid(target_name, override_target_uid):
    """
    If an override target_uid is provided in the kw list, then valid it doesnt collide with a different target.
    Otherwise add it to the management structures
    :param target_name:         The target name to validate
    :param override_target_uid: The overrride uid to validate
    """
    global UID_MAP_TO_TARGET
    if override_target_uid in UID_MAP_TO_TARGET:
        if UID_MAP_TO_TARGET[override_target_uid] != target_name:
            raise Errors.WafError(
                "Keyword 'target_uid' defined for target '{}' conflicts with another target module '{}'. "
                "Please update this value to a unique value (i.e. {})"
                    .format(target_name, UID_MAP_TO_TARGET[override_target_uid], find_unique_target_uid()))
    else:
        UID_MAP_TO_TARGET[override_target_uid] = target_name


@conf
def process_additional_code_folders(ctx):
    additional_code_folders = ctx.get_additional_code_folders_from_spec()
    if len(additional_code_folders)>0:
        ctx.recurse(additional_code_folders)
    
    supported_platforms = ctx.get_current_platform_list(include_alias=False)
    if any(platform for platform in supported_platforms if ctx.is_android_platform(platform)):

        process_android_func = getattr(ctx, 'process_android_projects', None)
        if not process_android_func:
            ctx.fatal('[ERROR] Failed to find required Android process function - process_android_projects')

        process_android_func()


@conf
def Path(ctx, path):

    # Alias to 'CreateRootRelativePath'
    def _get_source_file_and_function():
        stack = traceback.extract_stack(limit=10)
        max_index = len(stack) - 1
        for index in range(max_index, 0, -1):
            if stack[index][2] in ('_get_source_file_and_function', 'PreprocessFilePath', 'fun', 'Path', '_invalid_alias_callback', '_alias_not_enabled_callback'):
                continue
            return stack[index][0], stack[index][1], stack[index][2]
        raise RuntimeError('Invalid stack trace')

    def _invalid_alias_callback(alias_key):
        error_file, error_line, error_function = _get_source_file_and_function()
        error_message = "Invalid alias '{}' specified in {}:{} ({})".format(alias_key, error_file, error_line,
                                                                            error_function)
        raise Errors.WafError(error_message)

    def _alias_not_enabled_callback(alias_key, roles):
        error_file, error_line, error_function = _get_source_file_and_function()
        warning_message = "3rd Party alias '{}' specified in {}:{} ({}) is not enabled. Make sure that at least one of the " \
                          "following roles is enabled: [{}]".format(alias_key, error_file, error_line, error_function,
                                                                    ', '.join(roles or ['None']))
        ctx.warn_once(warning_message)

    processed_path = ctx.PreprocessFilePath(path, _invalid_alias_callback, _alias_not_enabled_callback)
    if not processed_path:
        Logs.warn("[WARN] Invalid path alias:{}".format(path))
    if os.path.isabs(processed_path):
        return processed_path
    else:
        return CreateRootRelativePath(ctx, processed_path)


@conf
def PreprocessFilePath(ctx, input, alias_invalid_callback, alias_not_enabled_callback):
    """
    Perform a preprocess to a path to perform the following:

    * Substitute any alias (@<ALIAS>@) with the alias value
    * Convert relative paths to absolute paths using the base_path as the pivot (if set)

    :param ctx:             Context
    :param input_path:      Relative, absolute, or aliased pathj
    :param base_path:       Base path to resolve relative paths if supplied.  If not, relative paths stay relative
    :param return_filenode: Flag to return a Node object instead of an absolute path
    :return:    The processed path
    """

    if isinstance(input, Node.Node):
        input_path = input.abspath()
    else:
        input_path = input

    # Perform alias check first
    alias_match_groups = REGEX_PATH_ALIAS.search(input_path)
    if alias_match_groups:

        # Alias pattern detected, evaluate the alias
        alias_key = alias_match_groups.group(1)

        if alias_key == "ENGINE":
            # Apply the @ENGINE@ Alias
            processed_path = os.path.normpath(input_path.replace('@{}@'.format(alias_key), ctx.engine_node.abspath()))
            return processed_path

        elif alias_key.startswith(FILE_ALIAS_3P_PREFIX):
            # Apply a third party alias reference
            third_party_identifier = alias_key.replace(FILE_ALIAS_3P_PREFIX, "").strip()
            resolved_path, enabled, roles, optional = ctx.tp.get_third_party_path(None, third_party_identifier)

            # If the path was not resolved, it could be an invalid alias (missing from the SetupAssistantConfig.json
            if not resolved_path:
                alias_invalid_callback(alias_key)

            # If the path was resolved, we still need to make sure the 3rd party is enabled based on the roles
            if not enabled and not optional:
                sdk_roles = ctx.get_sdk_setup_assistant_roles_for_sdk(third_party_identifier)
                alias_not_enabled_callback(alias_key, sdk_roles)

            processed_path = os.path.normpath(input_path.replace('@{}@'.format(alias_key), resolved_path))
            return processed_path
        else:
            return input_path
    else:
        return input_path


###############################################################################
@conf
def ThirdPartyPath(ctx, third_party_identifier, path=''):

    def _get_source_file_and_function():
        stack = traceback.extract_stack(limit=10)
        max_index = len(stack) - 1
        for index in range(max_index, 0, -1):
            if stack[index][2] in ('_get_source_file_and_function', 'fun', 'ThirdPartyPath', '_invalid_alias_callback', '_alias_not_enabled_callback'):
                continue
            return stack[index][0], stack[index][1], stack[index][2]
        raise RuntimeError('Invalid stack trace')

    resolved_path, enabled, roles, optional = ctx.tp.get_third_party_path(None, third_party_identifier)

    # If the path was not resolved, it could be an invalid alias (missing from the SetupAssistantConfig.json
    if not resolved_path:
        error_file, error_line, error_function = _get_source_file_and_function()
        error_message = "Invalid alias '{}' specified in {}:{} ({})".format(third_party_identifier, error_file, error_line,
                                                                            error_function)
        raise Errors.WafError(error_message)

    # If the path was resolved, we still need to make sure the 3rd party is enabled based on the roles
    if not enabled and not optional:

        try:
            sdk_roles = ctx.get_sdk_setup_assistant_roles_for_sdk(third_party_identifier)
            required_checks = convert_roles_to_setup_assistant_description(sdk_roles)
            error_file, error_line, error_function = _get_source_file_and_function()

            if len(required_checks) > 0:
                warning_message = "3rd Party alias '{}' specified in {}:{} ({}) is not enabled. This may cause build errors.  Make sure that at least one of the " \
                                  "following options are checked in setup assistant: [{}]".format(third_party_identifier, error_file, error_line, error_function,
                                                                        ', '.join(required_checks))
            else:
                warning_message = "3rd Party alias '{}' specified in {}:{} ({}) is not enabled"

            ctx.warn_once(warning_message)
        except AttributeError as e:
            raise Errors.WafError('Invalid setup assistant config file:{}'.format(e.msg or e.message))

    path_trimmed = path.strip()
    if len(path_trimmed)>0 and path_trimmed != '/' and path_trimmed != '\\':
        processed_path = os.path.normpath(os.path.join(resolved_path, path_trimmed))
    else:
        processed_path = os.path.normpath(resolved_path)

    return processed_path


#
# Unique Target ID Management
#
UID_MAP_TO_TARGET = {}

# Start the UID from a minimum value to prevent collision with the previously used 'idx' values in the generated
# names
MIN_UID_VALUE = 1024

# Constant value to cap the upper limit of the target uid.  The target uid will become part of intermediate constructed
# files Increase this number if there are more chances of collisions with target names.  Reduce if we start getting
# issues related to path lengths caused by the appending of the number
MAX_UID_VALUE = 9999999


def apply_target_uid_range(uid):
    """
    Apply the uid range restrictions rule based on the min and max uid values
    :param uid:     The raw uid to apply to
    :return:    The applied range on the input uid
    """
    return ((uid & 0xffffffff) + MIN_UID_VALUE) % MAX_UID_VALUE


def find_unique_target_uid():
    """
    Simple function to find a unique number for the target uid
    """
    global UID_MAP_TO_TARGET
    random_retry = 1024
    random_unique_id = apply_target_uid_range(random.randint(MIN_UID_VALUE, MAX_UID_VALUE))
    while random_retry > 0 and random_unique_id in UID_MAP_TO_TARGET:
        random_unique_id = random.randint(MIN_UID_VALUE, MAX_UID_VALUE)
        random_retry -= 1

    if random_retry == 0:
        raise Errors.WafError("Unable to generate a unique target id.  Current ids are : {}".format(
            ",".join(UID_MAP_TO_TARGET.keys())))
    return random_unique_id


def derive_target_uid(target_name):
    """
    Derive a target uid based on the target name if possible.  If a collision occurs, raise a WAF Error
    indicating the collision and suggest a unique id that doesnt collide
    :param target_name: The target name to derive the target uid from
    :return: The derived target uid based on the name if possible.
    """
    global UID_MAP_TO_TARGET

    # Calculate a deterministic UID to this target initially based on the CRC32 value of the target name
    uid = apply_target_uid_range(zlib.crc32(target_name))
    if uid in UID_MAP_TO_TARGET and UID_MAP_TO_TARGET[uid] != target_name:
        raise Errors.WafError(
            "[ERROR] Unable to generate a unique target id for target '{}' due to collision with target '{}'. "
            "Please update the target definition for target '{}' to include an override target_uid keyword (ex: {}) )"
            "Or consider increasing the MAX_UID_VALUE defined in {}"
            .format(target_name, UID_MAP_TO_TARGET[uid], target_name, find_unique_target_uid(), __file__))
    UID_MAP_TO_TARGET[uid] = target_name
    return uid


def validate_override_target_uid(target_name, override_target_uid):
    """
    If an override target_uid is provided in the kw list, then valid it doesnt collide with a different target.
    Otherwise add it to the management structures
    :param target_name:         The target name to validate
    :param override_target_uid: The overrride uid to validate
    """
    global UID_MAP_TO_TARGET
    if override_target_uid in UID_MAP_TO_TARGET:
        if UID_MAP_TO_TARGET[override_target_uid] != target_name:
            raise Errors.WafError(
                "Keyword 'target_uid' defined for target '{}' conflicts with another target module '{}'. "
                "Please update this value to a unique value (i.e. {})"
                .format(target_name, UID_MAP_TO_TARGET[override_target_uid], find_unique_target_uid()))
    else:
        UID_MAP_TO_TARGET[override_target_uid] = target_name


@conf_event(after_methods=['clear_waf_timestamp_files'])
def update_target_uid_map(ctx):
    """
    Update the cached target uid map
    """
    global UID_MAP_TO_TARGET
    target_uid_cache_file = ctx.bldnode.make_node('target_uid.json')
    uid_map_to_target_json = json.dumps(UID_MAP_TO_TARGET, indent=1, sort_keys=True)
    target_uid_cache_file.write(uid_map_to_target_json)


@conf
def assign_target_uid(ctx, kw):
    target = kw['target']
    # Assign a deterministic UID to this target initially based on the CRC32 value of the target name
    if 'target_uid' not in kw:
        kw['target_uid'] = derive_target_uid(target)
    else:
        # If there is one overridden already, validate it against the known uuids
        validate_override_target_uid(kw['target_uid'])


@conf
def initialize_third_party_settings(conf):
    try:
        third_party_config = conf.tp
    except AttributeError:
        third_party_config = conf.tp = ThirdPartySettings(conf)

    third_party_config.initialize()


def _get_module_def_filename(target):
    return '{}_module_def.json'.format(target)


PLATFORM_KEYWORDS = None

@conf
def update_module_definition(ctx, module_type, build_type, kw):
    """
    Update the module definition file for the current module

    :param ctx:             Configuration context
    :param module_type:     The module type (from cryengine_modules)
    :param build_type:      The WAF type (shlib, stlib, program)
    :param kw:              The keyword dictionary for the current module
    """

    def _to_set(input):

        if isinstance(input, list):
            return set(input)
        else:
            return set([input])

    target = kw.get('target', '')
    if len(target) == 0:
        raise Errors.WafError("Missing/invalid 'target' keyword in {}/".format(ctx.path.abspath()))

    platforms = list(_to_set(kw.get('platforms',['all'])))
    configurations = list(_to_set(kw.get('configurations',['all'])))
    use = _to_set(kw.get('use',[]))
    uselib = _to_set(kw.get('uselib', []))
    uses = list(use | uselib)
    path = ctx.path.path_from(ctx.engine_node)

    module_def = {
        'target': target,
        'type': build_type,
        'module_type': module_type,
        'path': path,
        'platforms': sorted(platforms),
        'configurations': sorted(configurations),
        'uses': sorted(uses)
    }
    module_def_folder = os.path.join(ctx.engine_path, BINTEMP_FOLDER, BINTEMP_MODULE_DEF)
    if not ctx.cached_does_path_exist(module_def_folder):
        os.makedirs(module_def_folder)
        ctx.cached_does_path_exist(module_def_folder, True)

    module_def_file = os.path.join(module_def_folder, _get_module_def_filename(target))
    write_json_file(module_def, module_def_file)


def _get_module_def_map(ctx):
    try:
        return ctx.module_def_by_target
    except AttributeError:
        ctx.module_def_by_target = {}
        return ctx.module_def_by_target


def _has_module_def(ctx, target):
    module_def_folder = os.path.join(ctx.engine_path, BINTEMP_FOLDER, BINTEMP_MODULE_DEF)
    module_def_file = os.path.join(module_def_folder, _get_module_def_filename(target))
    return os.path.isfile(module_def_file)


def _read_module_def_file(ctx, target):

    module_def_folder = os.path.join(ctx.engine_path, BINTEMP_FOLDER, BINTEMP_MODULE_DEF)
    module_def_file = os.path.join(module_def_folder, _get_module_def_filename(target))
    if not os.path.exists(module_def_file):
        raise Errors.WafError("Invalid target '{}'. Missing module definition file. Make sure that the target exists".format(target))

    module_def = parse_json_file(module_def_file)
    return module_def


def _get_module_def(ctx, target):
    module_def_map = _get_module_def_map(ctx)
    try:
        module_def = module_def_map[target]
    except KeyError:
        module_def = _read_module_def_file(ctx, target)
        module_def_map[target] = module_def
    return module_def


@conf
def get_module_platforms(ctx, target, pre_process_results=True):
    """
    Quickly look up the supported platforms for a target

    :param ctx:                 Context
    :param target:              Name of the target to lookup
    :param pre_process_results: Pre-process the result (i.e. apply any aliased platforms)
    :return: The list of supported platforms for a module
    """
    if not _has_module_def(ctx, target):
        # If this target does not have a module dep, return an empty list of platforms
        return []
    module_def_map = _get_module_def(ctx, target)
    platforms = module_def_map.get('platforms')[:]
    return ctx.preprocess_target_platforms(platforms) if pre_process_results else platforms


@conf
def get_module_configurations(ctx, target, restricted_platform=None, pre_process_results=True):
    """
    Quickly look up the supported platforms for a target

    :param ctx:                 Context
    :param target:              Name of the target to lookup
    :param restricted_platform: Optional platform to restrict the list of possible configurations
    :param pre_process_results: Pre-process the result (i.e. apply any aliased configurations)
    :return: The list of supported configurations for a module
    """

    module_def_map = _get_module_def(ctx, target)
    configurations = module_def_map.get('configurations')[:]
    return ctx.preprocess_target_configuration_list(target, restricted_platform, configurations) if pre_process_results else configurations


@conf
def is_valid_module(ctx, target):
    """
    Perform a check if a target is a valid, declared target.  The target will either be invalid, or if was somehow missed during
    the module def generation during the configure command.

    :param ctx:     Context
    :param target:  The target name to check
    :return: True if the module is valid, False if not
    """
    try:
        _get_module_def(ctx, target)
        return True
    except Errors.WafError:
        return False


@conf
def get_export_internal_3rd_party_lib(ctx, target):
    """
    Determine if a module has the 'export_internal_3rd_party_lib' flag set to True

    :param ctx:     Context
    :param target:  Name of the module
    :param parent_spec: Name of the parent spec
    :return:    True if 'export_internal_3rd_party_lib' ios set to True, default to False if not specified
    """
    if not ctx.is_valid_module(target):
        return False

    module_def = _get_module_def(ctx, target)
    if not module_def:
        return False
    return module_def.get('export_internal_3rd_party_lib', False)


@conf
def get_module_uses(ctx, target, parent_spec):
    """
    Get all the module uses for a particular module (from the generated module def file)
    :param ctx:     Context
    :param target:  Name of the module
    :param parent_spec: Name of the parent spec
    :return:        Set of all modules that this module is dependent on
    """

    visited_modules = set()

    def _get_module_use(ctx, target_name):
        """
        Determine all of the uses for a module recursivel
        :param ctx:          Context
        :param target_name:  Name of the module
        :return:        Set of all modules that this module is dependent on
        """
        module_def_map = _get_module_def(ctx, target_name)
        declared_uses = module_def_map.get('uses')[:]
        visited_modules.add(target_name)

        module_uses = []
        for module_use in declared_uses:

            if module_use not in module_uses:
                module_uses.append(module_use)

            if ctx.is_third_party_uselib_configured(module_use):
                # This is a third party uselib, there are no dependencies, so skip
                continue
            elif module_use in visited_modules:
                # This module has been visited, skip
                continue
            elif not ctx.is_valid_module(module_use):
                # This use dependency is an invalid module.  Warn and continue
                
                # Special case for 'RAD_TELEMETRY'. This module is only available when both the Gem is enabled and the machine has a valid
                # RAD Telemetry license, otherwise references to it should be #ifdef'd out of the code. So if the invalid module is
                # RAD Telemetry, we can suppress the warning
                if module_use != 'RAD_TELEMETRY':
                    ctx.warn_once("Module use dependency '{}' for target '{}' refers to an invalid module".format(module_use, target_name))
                continue
            elif module_use == target:
                raise Errors.WafError("Cyclic use-dependency discovered for target '{}'".format(target_name))
            else:
                child_uses = _get_module_use(ctx, module_use)
                for child_use in child_uses:
                    module_uses.append(child_use)

        return module_uses
    return _get_module_use(ctx, target)



CACHED_SPEC_MODULE_USE_MAP = {}


@conf
def get_all_module_uses(ctx, input_spec_name=None, input_platform=None, input_configuration=None):
    """
    Get all of the module uses for a spec
    :param ctx:             Context
    :param input_spec_name:       name of the spec (or None, use the current spec)
    :param input_platform:        the platform (or None, use the current platform)
    :param input_configuration:   the configuration (or None, use the current configuration)
    :return: List of all modules that are referenced for a spec (and platform/configuration)
    """

    # Get the spec name, fall back to command line specified spec if none was given
    spec_name = input_spec_name if input_spec_name else ctx.options.project_spec

    # Get the platform/configuration for the composite key, falling back on 'none'
    platform = input_platform if input_platform else ctx.env['PLATFORM']
    configuration = input_configuration if input_configuration else ctx.env['CONFIGURATION']
    composite_key = '{}_{}_{}'.format(spec_name, platform, configuration)

    global CACHED_SPEC_MODULE_USE_MAP
    try:
        return CACHED_SPEC_MODULE_USE_MAP[composite_key]
    except KeyError:
        pass

    all_modules = set()
    spec_modules = ctx.spec_modules(spec_name,platform,configuration)

    for spec_module in spec_modules:
        all_spec_module_uses = ctx.get_module_uses(spec_module,spec_name)
        for i in all_spec_module_uses:
            all_modules.add(i)

    CACHED_SPEC_MODULE_USE_MAP[composite_key] = all_modules
    return all_modules


EXEMPT_USE_MODULES = set()


EXEMPT_SPEC_MODULES = set()


@conf
def mark_module_exempt(ctx, target):
    EXEMPT_USE_MODULES.add(target)


@conf
def is_module_exempt(ctx, target):
    # Ignore the Exempt modules that are not part of the spec system
    if target in EXEMPT_USE_MODULES:
        return True

    # Ignore any available launcher derived uses
    if target in AVAILABLE_LAUNCHERS.get('modules', []):
        return True

    return False


def deprecated(reason=None):
    """
    Special deprecated decorator to tag functions that are marked as deprecated. These functions will be outputted to the debug log
    :param reason:  Optional reason/message to display in the debug log
    :return: deprecated decorator
    """
    def deprecated_decorator(func):
        callstack = inspect.stack()
        func_module = callstack[1][1]
        func_name = '{}:{}'.format(func_module,func.__name__)
        deprecated_message = "[WARN] Function '{}' is deprecated".format(func_name)
        if reason:
            deprecated_message += ": {}".format(reason)
        Logs.debug(deprecated_message)
    return deprecated_decorator


class ProjectGenerator(BuildContext):
    pass


@conf
def validate_monolithic_specified_targets(bld):
    """
    Check monolithic builds for situations where explicit targets (in the case of building in Visual Studio)
    are valid taget names.
    
    :param bld: The build context
    """
    
    platform, configuration = bld.get_platform_and_configuration()
    
    if bld.is_build_monolithic():
        if len(bld.spec_modules()) == 0:
            bld.fatal('[ERROR] no available modules to build for that spec "%s" in "%s|%s"' % (
            bld.options.project_spec, platform, configuration))
        
        # Ensure that, if specified, target is supported in this spec
        if bld.options.targets:
            for target in bld.options.targets.split(','):
                if not target in bld.spec_modules():
                    bld.fatal('[ERROR] Module "%s" is not configured to build in spec "%s" in "%s|%s"' % (
                    target, bld.options.project_spec, platform, configuration))

