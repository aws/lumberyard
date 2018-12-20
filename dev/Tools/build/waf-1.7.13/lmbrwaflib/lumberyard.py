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
import random
import zlib
import json

import re
import traceback

from waflib import Logs, Options, Utils, Configure, ConfigSet, Errors
from waflib.TaskGen import feature, before_method
from waflib.Configure import conf, ConfigurationContext
from waflib.Build import BuildContext, CleanContext, Context
from waflib import Node
from waf_branch_spec import SUBFOLDERS, VERSION_NUMBER_PATTERN, PLATFORM_CONFIGURATION_FILTER, PLATFORMS, CONFIGURATIONS, LUMBERYARD_ENGINE_PATH
from utils import parse_json_file
from third_party import ThirdPartySettings

UNSUPPORTED_PLATFORM_CONFIGURATIONS = set()

WAF_BASE_SCRIPT = os.path.normpath("{}/Tools/build/waf-1.7.13/lmbr_waf".format(LUMBERYARD_ENGINE_PATH))

CURRENT_WAF_EXECUTABLE = '"{}" "{}"'.format(sys.executable, WAF_BASE_SCRIPT)

WAF_TOOL_ROOT_DIR = os.path.join(LUMBERYARD_ENGINE_PATH, "Tools", "build","waf-1.7.13")
LMBR_WAF_TOOL_DIR = os.path.join(WAF_TOOL_ROOT_DIR, "lmbrwaflib")
GENERAL_WAF_TOOL_DIR = os.path.join(WAF_TOOL_ROOT_DIR, "waflib")

# some files write a corresponding .timestamp files in BinTemp/ as part of the configuration process
if LUMBERYARD_ENGINE_PATH=='.':
    TIMESTAMP_CHECK_FILES = ['SetupAssistantConfig.json']
else:
    TIMESTAMP_CHECK_FILES = [os.path.join(LUMBERYARD_ENGINE_PATH, "SetupAssistantConfig.json"),'bootstrap.cfg']
ANDROID_TIMESTAMP_CHECK_FILES = ['_WAF_/android/android_settings.json', '_WAF_/environment.json']

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

# list of platforms that have a deploy command registered
DEPLOYABLE_PLATFORMS = [
    'android_armv7_gcc',
    'android_armv7_clang',
    'android_armv8_clang'
]

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
        # Put the deps modules first as they wrap/intercept c/cxx calls to
        # capture dependency information being written out to stderr. If
        # another module declares a subclass of a c/cxx feature, like qt5 does,
        # and it is before these modules then the subclass/feature does not get
        # wrapped and the header dependency information is printed to the
        # screen.
        'gccdeps',
        'clangdeps',
        'msvs:win32', # msvcdeps implicitly depends on this module, so load it first
        'msvcdeps:win32',

        'cry_utils',
        'project_settings',
        'branch_spec',
        'copy_tasks',
        'lumberyard_sdks',
        'gems',
        'az_code_generator',
        'crcfix',
        'doxygen',
        'packaging',

        'generate_uber_files',          # Load support for Uber Files
        'generate_module_def_files',    # Load support for Module Definition Files
        'generate_module_dependency_files',  # Load support for Module Dependency Files (Used by ly_integration_toolkit_utils.py)

        # Visual Studio support
        'msvs_override_handling:win32',
        'mscv_helper:win32',
        'vscode:win32',

        'xcode:darwin',
        'eclipse:linux',

        'android',
        'android_library',
        'android_studio',

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

# List of all compile settings configuration modules
PLATFORM_COMPILE_SETTINGS = [
    'compile_settings_cryengine',
    'compile_settings_msvc',
    'compile_settings_gcc',
    'compile_settings_clang',
    'compile_settings_windows',
    'compile_settings_linux',
    'compile_settings_linux_x64',
    'compile_settings_darwin',
    'compile_settings_dedicated',
    'compile_settings_test',
    'compile_settings_android',
    'compile_settings_android_armv7',
    'compile_settings_android_armv8',
    'compile_settings_android_gcc',
    'compile_settings_android_clang',
    'compile_settings_ios',
    'compile_settings_appletv',
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
                conf.fatal("[Error] Unable to load required module '{}.py: {}'".format(module, e.message or e.msg))


@conf
def load_lmbr_general_modules(conf):
    """
    Load all of the required waf lmbr modules
    :param conf:    Configuration Context
    """
    load_lmbr_waf_modules(conf, LMBR_WAFLIB_MODULES)


@conf
def load_lmbr_data_driven_modules(conf):
    """
    Load all of the data driven waf lmbr modules
    :param conf:    Configuration Context
    """
    load_lmbr_waf_modules(conf, LMBR_WAFLIB_DATA_DRIVEN_MODULES)

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

def configure_general_compile_settings(conf):
    """
    Perform all the necessary configurations
    :param conf:        Configuration context
    """
    # Load general compile settings
    load_setting_count = 0
    absolute_lmbr_waf_tool_path = LMBR_WAF_TOOL_DIR if os.path.isabs(LMBR_WAF_TOOL_DIR) else conf.path.make_node(LMBR_WAF_TOOL_DIR).abspath()

    for compile_settings in PLATFORM_COMPILE_SETTINGS:
        t_path = os.path.join(absolute_lmbr_waf_tool_path, "{}.py".format(compile_settings))
        if os.path.exists(os.path.join(absolute_lmbr_waf_tool_path, "{}.py".format(compile_settings))):
            conf.load(compile_settings, tooldir=[LMBR_WAF_TOOL_DIR])
            load_setting_count += 1
    if load_setting_count == 0:
        conf.fatal('[ERROR] Unable to load any general compile settings modules')

    available_platforms = conf.get_available_platforms()
    if len(available_platforms)==0:
        raise Errors.WafError('[ERROR] No available platforms detected.  Make sure you enable at least one platform in Setup Assistant.')

    # load the android specific tools, if enabled
    if any(platform for platform in available_platforms if platform.startswith('android')):
        # We need to validate the JDK path from SetupAssistant before loading the javaw tool.
        # This way we don't introduce a dependency on lmbrwaflib in the core waflib.
        jdk_home = conf.get_env_file_var('LY_JDK', required = True)
        if not jdk_home:
            conf.fatal('[ERROR] Missing JDK path from Setup Assistant detected.  Please re-run Setup Assistant with "Compile For Android" enabled and run the configure command again.')

        conf.env['JAVA_HOME'] = jdk_home

        conf.load('javaw')
        conf.load('android', tooldir=[LMBR_WAF_TOOL_DIR])


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
    except:
        conf.fatal("[ERROR] Unable to load compile rules module file '{}'".format(host_module_file))

    host_function_name = 'load_{}_host_settings'.format(waf_host_platform)
    if not hasattr(conf, host_function_name):
        conf.fatal('[ERROR] Required Configuration Function \'{}\' not found in configuration file {}'.format(host_function_name, host_module_file))

    return host_function_name


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
def configure_settings(conf):
    """
    Perform all the necessary configurations

    :param conf:        Configuration context
    """
    configure_general_compile_settings(conf)


@conf
def run_bootstrap(conf):
    """
    Execute the bootstrap process

    :param conf:                Configuration context
    """
    # Bootstrap is only ran within the engine folder
    if not conf.is_engine_local():
        return

    conf.run_linkless_bootstrap()

@conf
def load_compile_rules_for_supported_platforms(conf, platform_configuration_filter):
    """
    Load the compile rules for all the supported target platforms for the current host platform

    :param conf:                            Configuration context
    :param platform_configuration_filter:   List of target platforms to filter out
    """

    host_platform = conf.get_waf_host_platform()

    absolute_lmbr_waf_tool_path = LMBR_WAF_TOOL_DIR if os.path.isabs(LMBR_WAF_TOOL_DIR) else conf.path.make_node(LMBR_WAF_TOOL_DIR).abspath()

    vanilla_conf = conf.env.derive()  # grab a snapshot of conf before you pollute it.

    host_function_name = load_compile_rules_for_host(conf, host_platform)

    installed_platforms = []

    for platform in conf.get_available_platforms():

        platform_spec_vanilla_conf = vanilla_conf.derive()
        platform_spec_vanilla_conf.detach()

        platform_spec_vanilla_conf['PLATFORM'] = platform

        # Determine the compile rules module file and remove it and its support if it does not exist
        compile_rule_script = 'compile_rules_' + host_platform + '_' + platform
        if not os.path.exists(os.path.join(absolute_lmbr_waf_tool_path, compile_rule_script + '.py')):
            conf.remove_platform_from_available_platforms(platform)
            continue

        Logs.info('[INFO] Configure "%s - [%s]"' % (platform, ', '.join(conf.get_supported_configurations(platform))))
        conf.load(compile_rule_script, tooldir=[LMBR_WAF_TOOL_DIR])

        conf.tp.validate_local(platform)

        # platform installed
        installed_platforms.append(platform)

        # Keep track of uselib's that we found in the 3rd party config files
        third_party_uselib_map = conf.read_and_mark_3rd_party_libs()
        conf.env['THIRD_PARTY_USELIBS'] = [uselib_name for uselib_name in third_party_uselib_map]

        # Save off configuration values from the uselib which are necessary during build for modules built with the uselib
        configuration_settings_map = {}
        for uselib_name in third_party_uselib_map:
            configuration_values = conf.get_configuration_settings(uselib_name)
            if configuration_values:
                configuration_settings_map[uselib_name] = configuration_values
        conf.env['THIRD_PARTY_USELIB_SETTINGS'] = configuration_settings_map

        if not hasattr(conf,'InvalidUselibs'):
            setattr(conf,'InvalidUselibs',set())

        for supported_configuration in conf.get_supported_configurations():
            # if the platform isn't going to generate a build command, don't require that the configuration exists either
            if platform in platform_configuration_filter:
                if supported_configuration not in platform_configuration_filter[platform]:
                    continue

            conf.setenv(platform + '_' + supported_configuration, platform_spec_vanilla_conf.derive())
            conf.init_compiler_settings()

            # add the host settings into the current env
            getattr(conf, host_function_name)()

            # make a copy of the config for certain variant loading redirection (e.g. test, dedicated)
            # this way we can pass the raw configuration to the third pary reader to properly configure
            # each library
            config_redirect = supported_configuration

            # Use the normal configurations as a base for dedicated server
            is_dedicated = False
            if config_redirect.endswith('_dedicated'):
                config_redirect = config_redirect.replace('_dedicated', '')
                is_dedicated = True

            # Use the normal configurations as a base for test
            is_test = False
            if '_test' in config_redirect:
                config_redirect = config_redirect.replace('_test', '')
                is_test = True

            # Use the specialized function to load platform specifics
            function_name = 'load_%s_%s_%s_settings' % ( config_redirect, host_platform, platform )
            if not hasattr(conf, function_name):
                conf.fatal('[ERROR] Required Configuration Function \'%s\' not found' % function_name)

            # Try to load the function
            getattr(conf, function_name)()

            # Apply specific dedicated server settings
            if is_dedicated:
                getattr(conf, 'load_dedicated_settings')()

            # Apply specific test settings
            if is_test:
                getattr(conf, 'load_test_settings')()

            if platform in conf.get_supported_platforms():
                # If the platform is still supported (it will be removed if the load settings function fails), then
                # continue to attempt to load the 3rd party uselib defs for the platform

                for uselib_info in third_party_uselib_map:

                    third_party_config_file = third_party_uselib_map[uselib_info][0]
                    path_alias_map = third_party_uselib_map[uselib_info][1]

                    is_valid, uselib_names = conf.read_3rd_party_config(third_party_config_file, platform, supported_configuration, path_alias_map)

                    if not is_valid:
                        conf.warn_once('Invalid local paths defined in file {} detected.  The following uselibs will not be valid:{}'.format(third_party_config_file, ','.join(uselib_names)))
                        for uselib_name in uselib_names:
                            conf.InvalidUselibs.add(uselib_name)


@conf
def process_custom_configure_commands(conf):
    """
    Add any additional custom commands that need to be run during the configure phase

    :param conf:                Configuration context
    """

    host = Utils.unversioned_sys_platform()

    if host == 'win32':
        # Win32 platform optional commands

        # Generate the visual studio projects & solution if specified
        if conf.is_option_true('generate_vs_projects_automatically'):
            Options.commands.insert(0, 'msvs')

    elif host == 'darwin':

        # Darwin/Mac platform optional commands

        # Create Xcode-iOS-Projects automatically during configure when running on mac
        if conf.is_option_true('generate_ios_projects_automatically'):
            # Workflow improvement: for all builds generate projects after the build
            # except when using the default build target 'utilities' then do it before
            if 'build' in Options.commands:
                build_cmd_idx = Options.commands.index('build')
                Options.commands.insert(build_cmd_idx, 'xcode_ios')
            else:
                Options.commands.append('xcode_ios')

        # Create Xcode-AppleTV-Projects automatically during configure when running on mac
        if conf.is_option_true('generate_appletv_projects_automatically'):
            # Workflow improvement: for all builds generate projects after the build
            # except when using the default build target 'utilities' then do it before
            if 'build' in Options.commands:
                build_cmd_idx = Options.commands.index('build')
                Options.commands.insert(build_cmd_idx, 'xcode_appletv')
            else:
                Options.commands.append('xcode_appletv')

        # Create Xcode-darwin-Projects automatically during configure when running on mac
        if conf.is_option_true('generate_mac_projects_automatically'):
            # Workflow improvement: for all builds generate projects after the build
            # except when using the default build target 'utilities' then do it before
            if 'build' in Options.commands:
                build_cmd_idx = Options.commands.index('build')
                Options.commands.insert(build_cmd_idx, 'xcode_mac')
            else:
                Options.commands.append('xcode_mac')

    # Android target platform commands
    if any(platform for platform in conf.get_supported_platforms() if platform.startswith('android')):

        # this is required for building any android projects. It adds the Android launchers
        # to the list of build directories
        android_builder_func = getattr(conf, 'create_and_add_android_launchers_to_build', None)
        if android_builder_func != None and android_builder_func():
            SUBFOLDERS.append(conf.get_android_project_absolute_path())

        # rebuild the project if invoked from android studio or sepcifically requested to do so
        if conf.options.from_android_studio or conf.is_option_true('generate_android_studio_projects_automatically'):
            if 'build' in Options.commands:
                build_cmd_idx = Options.commands.index('build')
                Options.commands.insert(build_cmd_idx, 'android_studio')
            else:
                Options.commands.append('android_studio')

    # Make sure the intermediate files are generated and updated
    if len(Options.commands) == 0:
        Options.commands.insert(0, 'generate_uber_files')
        Options.commands.insert(1, 'generate_module_def_files')
    else:
        has_generate_uber_files = 'generate_uber_files' in Options.commands
        has_generate_module_def_files = 'generate_module_def_files' in Options.commands
        if not has_generate_uber_files:
            Options.commands.insert(0, 'generate_uber_files')
        if not has_generate_module_def_files:
            Options.commands.insert(1, 'generate_module_def_files')


@conf
def configure_game_projects(conf):
    """
    Perform the configuration processing for any enabled game project

    :param conf:
    """
    # Load Game Specific parts
    for project in conf.get_enabled_game_project_list():
        conf.game_project = project

        # If the project contains the 'code_folder', then assume that it is a legacy project loads a game-specific dll
        legacy_game_code_folder = conf.legacy_game_code_folder(project)
        if legacy_game_code_folder is not None:
            conf.recurse(legacy_game_code_folder)
        else:
            # Perform a validation of the gems.json configuration to make sure
            conf.game_gem(project)


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
        if bld.is_cmd_monolithic():
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

        (platform, configuration) = bld.get_platform_and_configuration()
        bld.env['PLATFORM'] = platform
        bld.env['CONFIGURATION'] = configuration

        if platform in PLATFORM_CONFIGURATION_FILTER:
            if configuration not in PLATFORM_CONFIGURATION_FILTER[platform]:
                bld.fatal('[ERROR] Configuration ({}) for platform ({}) currently not supported'.format(configuration, platform))

        if not platform in bld.get_supported_platforms():
            bld.fatal('[ERROR] Target platform "%s" not supported. [on host platform: %s]' % (platform, Utils.unversioned_sys_platform()))

        # make sure the android launchers are included in the build
        if bld.is_android_platform(bld.env['PLATFORM']):
            android_path = os.path.join(bld.path.abspath(), bld.get_android_project_relative_path(), 'wscript')
            if not os.path.exists(android_path):
                bld.fatal('[ERROR] Android launchers not correctly configured. Run \'configure\' again')
            SUBFOLDERS.append(bld.get_android_project_absolute_path())

        # If a spec was supplied, check for platform limitations
        if bld.options.project_spec != '':
            validated_platforms = bld.preprocess_platform_list(bld.spec_platforms(), True)
            if platform not in validated_platforms:
                bld.fatal('[ERROR] Target platform "{}" not supported for spec {}'.format(platform, bld.options.project_spec))
            validated_configurations = bld.preprocess_configuration_list(None, platform, bld.spec_configurations(), True)
            if configuration not in validated_configurations:
                bld.fatal('[ERROR] Target configuration "{}" not supported for spec {}'.format(configuration, bld.options.project_spec))

        if bld.is_cmd_monolithic():
            if len(bld.spec_modules()) == 0:
                bld.fatal('[ERROR] no available modules to build for that spec "%s" in "%s|%s"' % (bld.options.project_spec, platform, configuration))

            # Ensure that, if specified, target is supported in this spec
            if bld.options.targets:
                for target in bld.options.targets.split(','):
                    if not target in bld.spec_modules():
                        bld.fatal('[ERROR] Module "%s" is not configured to build in spec "%s" in "%s|%s"' % (target, bld.options.project_spec, platform, configuration))

        # command chaining for packaging and deployment
        if bld.cmd.startswith('build') and (getattr(bld.options, 'project_spec', '') != '3rd_party'):
            package_cmd = 'package_{}_{}'.format(platform, configuration)
            if bld.is_option_true('package_projects_automatically') and (package_cmd not in Options.commands):
                Options.commands.append(package_cmd)

            deploy_cmd = 'deploy_{}_{}'.format(platform, configuration)
            if (platform in DEPLOYABLE_PLATFORMS) and (deploy_cmd not in Options.commands):
                Options.commands.append(deploy_cmd)


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
            engine_root_abs = engine_json['ExternalEnginePath']
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


# Create Build Context Commands for multiple platforms/configurations
for platform in PLATFORMS[Utils.unversioned_sys_platform()]:
    for configuration in CONFIGURATIONS:
        platform_config_key = platform + '_' + configuration
        # for platform/for configuration generates invalid configurations
        # if a filter exists, don't generate all combinations
        if platform in PLATFORM_CONFIGURATION_FILTER:
            if configuration not in PLATFORM_CONFIGURATION_FILTER[platform]:
                # Dont add the command but track it to report that this platform + configuration is explicitly not supported (yet)
                UNSUPPORTED_PLATFORM_CONFIGURATIONS.add(platform_config_key)
                continue
        # Create new class to execute clean with variant
        name = CleanContext.__name__.replace('Context','').lower()
        class tmp_clean(CleanContext):
            cmd = name + '_' + platform_config_key
            variant = platform_config_key

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
                if 'android' in platform:
                    timestamp_check_files += ANDROID_TIMESTAMP_CHECK_FILES

                for timestamp_check_file in timestamp_check_files:
                    if self.compare_timestamp_file_modified(timestamp_check_file):
                        self.add_configure_command()
                        return True
                return False

        # Create derived build class to execute host tools build for host + profile only
        host = Utils.unversioned_sys_platform()
        if platform in ["win_x64", "linux_x64", "darwin_x64"] and configuration == 'profile':
            class tmp_build_host_tools(tmp_build):
                cmd = 'build_host_tools'
                variant = platform + '_' + configuration
                def execute(self):
                    original_project_spec = self.options.project_spec
                    original_targets = self.targets
                    self.options.project_spec = 'host_tools'
                    self.targets = []
                    super(tmp_build_host_tools, self).execute()
                    self.options.project_spec = original_project_spec
                    self.targets = original_targets


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
        parsed_params = capability_parser.parse_args(params)
        parsed_capabilities = getattr(parsed_params,'enablecapability',[])
    else:
        host_platform = Utils.unversioned_sys_platform()

        # If bootstrap-tool-param is not set, that means setup assistant needs to be ran or we use default values
        # if the source for setup assistant is available but setup assistant has not been built yet

        base_lmbr_setup_path = os.path.join(ctx.path.abspath(), 'Tools', 'LmbrSetup')
        setup_assistant_code_path = os.path.join(ctx.path.abspath(), 'Code', 'Tools', 'AZInstaller')
        setup_assistant_spec_path = os.path.join(ctx.path.abspath(), '_WAF_', 'specs', 'lmbr_setup_tools.json')

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

@conf
def initialize_third_party_settings(conf):
    if not hasattr(conf,"tp"):
        conf.tp = ThirdPartySettings(conf)
        conf.tp.initialize()


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
def process_additional_code_folders(ctx):
    additional_code_folders = ctx.get_additional_code_folders_from_spec()
    if len(additional_code_folders)>0:
        ctx.recurse(additional_code_folders)


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
                alias_not_enabled_callback(alias_key, roles)

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
        error_file, error_line, error_function = _get_source_file_and_function()
        warning_message = "3rd Party alias '{}' specified in {}:{} ({}) is not enabled. Make sure that at least one of the " \
                          "following roles is enabled: [{}]".format(third_party_identifier, error_file, error_line, error_function,
                                                                    ', '.join(roles))
        ctx.warn_once(warning_message)

    processed_path = os.path.normpath(os.path.join(resolved_path, path))
    return processed_path
