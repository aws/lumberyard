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

import subprocess, os, shutil, plistlib

from utils import *
from branch_spec import spec_modules
from gems import Gem
from waflib import Context, Build, Utils, Logs, TaskGen
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waf_branch_spec import PLATFORMS, CONFIGURATIONS
from contextlib import contextmanager
from lumberyard_sdks import get_dynamic_lib_extension, get_platform_lib_prefix
from waflib.Utils import Timer
from waflib.Scripting import run_command

def run_xcode_build(pkg, target, destination):
    if pkg.platform not in ['darwin_x64', 'ios', 'applettv'] or not pkg.is_option_true('run_xcode_for_packaging'):
        Logs.debug("package: Not running xcode because either we are not on a macOS platform or the command line option disabled it")
        return

    Logs.info("Running xcode build command to create App Bundle")

    platform = pkg.platform
    if 'darwin' in platform:
        platform = "mac" 

    try:
        result = subprocess.check_output(["xcodebuild", 
            "-project", "{}/{}.xcodeproj".format(getattr(pkg.options, platform + "_project_folder", None), getattr(pkg.options, platform + "_project_name", None)),
            "-target", target, 
            "-quiet",
            "RUN_WAF_BUILD=NO", 
            "CONFIGURATION="+pkg.config, 
            "CONFIGURATION_BUILD_DIR="+destination.abspath()])
        Logs.debug("package: xcode result is {}".format(result))
    except Exception as err:
        Logs.error("Can't run xcode {}".format(err))


def should_copy_and_not_link(pkg):
    return Utils.is_win32 or 'release' in pkg.config or pkg.is_option_true('copy_assets') or pkg.force_copy_of_assets


class package_task(Task):
    """
    Package an executable and any resources and assets it uses into a platform specific format.

    Extended `Task`
    """

    color = 'GREEN'
    optional = False

    def __init__(self, *k, **kw):
        """
        Extended Task.__init__ to store the kw pairs passed in as attributes on the class and assign executable_name, task_gen_name, executable-task_gen and destination_node attributes to values based on the kw args.

        :param target: name of the target/executable to be packaged.
        :type target: string
        :param task_gen_name: [Optional] Name of the task_gen that will build the target/executable. Needed for launchers where the target will be the game project (like StarterGame) but the task gen name will include the platform and 'Launcher' word (i.e. StarterGameWinLauncher).
        :type task_gen_name: string
        :param destination: [Optional] Path to the place where the packaged executable should be
        :type destination: string
        :param include_all_libs: [Optional] Force the task to include all libs that are in the same directory as the built executable to be part of the package
        :type include_all_libs: boolean
        :param include_spec_dependencies: [Optional] Include dependencies that are found by inspecting modules included in the spec
        :type include_spec_dependencies: boolean
        :param spec: [Optional] The spec to use to get module dependencies. Defaults to game_and_engine if not specified
        :type spec: string
        :param gem_types: [Optional] Types of gem modules to include as dependencies. Defaults to a list containing GameModule if not specified
        :type gem_types: list of Gem.Module.Type
        :param resources: [Optional] Files that should be copied to the resource directory. Resource directory is determined by calling get_resource_node
        :type resources: list of strings
        :param dir_resources: [Optional] Directories that contain resources required for the executable (such as QtLibs). These directories will either be linked or copied into the location of the executable
        :type dir_resources: list of strings
        :param assets_path: [Optional] Path to where the assets for the executbale are located. They will be either copied into the package or a symlink will be created to them.
        :type assets_path: string
        :param use_pak_files: [Optional] If pak files should be used instead of assets. This takes precendence over assets_path parameter if both are specified.
        :type use_pak_files: boolean
        :param pak_file_path: [Optional] Location of the pak files. if not specified will default to "[project name]_[asset platform name]_paks"
        :type pak_file_path: string
        :param finalize_func: [Optional] A function to execute when the package task has finished. The package context and node containing the destination of the executable is passed into the function.
        :type finalize_func: function
        """

        super(package_task, self).__init__(self, *k, **kw)
        for key, val in kw.items():
            setattr(self, key, val)

        self.executable_name = kw['target']
        self.task_gen_name = kw.get('task_gen_name', self.executable_name)
        self.executable_task_gen = self.bld.get_tgen_by_name(self.task_gen_name)
        self.destination_node = kw.get('destination', None)

    def scan(self):
        """
        Overrided scans to check for extra dependencies. 
        
        This is done by inspecting the task_generator for its dependencies and
        if include_spec_dependencies has been specified to include modules that
        are specified in the spec and are potentially part of the project.
        """

        if getattr(self, 'include_all_libs', False):
            return ([], [])

        self.dependencies = get_dependencies_recursively_for_task_gen(self.bld, self.executable_task_gen)

        if getattr(self, 'include_spec_dependencies', False):
            spec_to_use = getattr(self, 'spec', 'game_and_engine')
            gem_types = getattr(self, 'gem_types', [Gem.Module.Type.GameModule])
            self.dependencies = self.dependencies.union(get_spec_dependencies(self.bld, spec_to_use, gem_types))

        return (list(self.dependencies), [])

    def run(self):
        Logs.info("Running package task for {}".format(self.executable_name))

        executable_source_node = self.inputs[0]

        if not self.destination_node:
            # destination not specified so assume we are putting the package
            # where the built executable is located, which is the input's
            # parent since the input node is the actual executable
            self.destination_node = self.inputs[0].parent

        Logs.debug("package: packaging {} to destination {}".format(executable_source_node.abspath(), self.destination_node.abspath()))

        if 'darwin' in self.bld.platform:
            run_xcode_build(self.bld, self.task_gen_name, self.destination_node) 

        self.process_executable()
        self.process_resources()
        self.process_assets()

    def process_executable(self):
        executable_source_node = self.inputs[0]
        executable_source_location_node = executable_source_node.parent
        dependency_source_location_nodes = [self.bld.engine_node.make_node(self.bld.get_output_folders(self.bld.platform, self.bld.config)[0].name)]
        if dependency_source_location_nodes != executable_source_location_node:
            dependency_source_location_nodes.append(executable_source_location_node)

        executable_dest_node = self.outputs[0].parent
        executable_dest_node.mkdir()

        Logs.info("Putting final packaging into base output folder {}, executable folder {}".format(self.destination_node.abspath(), executable_dest_node.abspath()))

        if executable_source_location_node != executable_dest_node:
            self.bld.install_files(executable_dest_node.abspath(), self.executable_name, cwd=executable_source_location_node, chmod=Utils.O755, postpone=False)
            # Remove the built executable since we just copied it into the
            # package. Leaving it laying around may cause confusion in
            # customers
            executable_source_node.delete()

            if getattr(self, 'include_all_libs', False):
                self.bld.symlink_libraries(executable_source_location_node, executable_dest_node.abspath())
            else:
                # self.dependencies comes from the scan function
                self.bld.symlink_dependencies(self.dependencies, dependency_source_location_nodes, executable_dest_node.abspath())
        else:
            Logs.debug("package: source {} = dest {}".format(executable_source_location_node.abspath(), executable_dest_node.abspath()))

        for dir in getattr(self, 'dir_resources', []):
            Logs.debug("package: extra directory to link/copy into the package is: {}".format(dir))
            self.bld.create_symlink_or_copy(executable_source_location_node.make_node(dir), executable_dest_node.make_node(dir).abspath(), postpone=False)

        if getattr(self, 'finalize_func', None):
            self.finalize_func(self.bld, executable_dest_node)

    def process_resources(self):
        resources_dest_node = get_resource_node(self.bld.platform, self.executable_name, self.destination_node)
        resources = getattr(self, 'resources', None)
        if resources:
            resources_dest_node.mkdir()
            self.bld.install_files(resources_dest_node.abspath(), resources)

    def process_assets(self):
        """ 
        Packages any assets. 
        
        Assets can come from the asset_source attribute or will use pak_files instead of use_pak_files has been specified. 
        """

        assets_source = getattr(self, 'assets_path', None)
        if getattr(self, 'use_pak_files', False):
            assets_source = getattr(self, 'pak_file_path', None)
            if assets_source and not os.path.exists(assets_source):
                Logs.warn("Specified pak file location {} does not exist. Defaulting to use the standard generated file path".format(assets_source))
                assets_source = None

            if not assets_source:
                assets_platform = self.bld.get_bootstrap_assets(self.bld.platform)
                pak_file_dir = self.bld.project + "_" + assets_platform + "_paks"
                pak_file_dir = pak_file_dir.lower()
                assets_source = self.bld.Path(pak_file_dir)

        game_assets_node = get_game_assets_node(self.bld.platform, self.executable_name, self.destination_node)

        Logs.debug("package: source {} dest {}".format(assets_source, game_assets_node.abspath()))

        if assets_source:
            if not os.path.exists(assets_source):
                Logs.warn("[WARNING] Asset source location {} does not exist on the file system. No assets will be put into the package.".format(assets_source))
                return

            if os.path.isdir(game_assets_node.abspath()):
                if should_copy_and_not_link(self.bld):
                    if os.path.islink(game_assets_node.abspath()):
                        # Need to remove the junction as rmtree does not do that and
                        # fails if there is a junction
                        remove_junction(game_assets_node.abspath())
                else: 
                    # Going from a copy to a link so remove the directory if it exists
                    if not os.path.islink(game_assets_node.abspath()):
                        shutil.rmtree(game_assets_node.abspath())

            # Make the parent directory so that when we either make a link or copy
            # assets the parent directory is there and waf install/symlink commands
            # will work correctly
            game_assets_node.parent.mkdir()

            Logs.info("Putting assets into folder {}".format(game_assets_node.abspath()))
            self.bld.create_symlink_or_copy(self.bld.root.find_node(assets_source), game_assets_node.abspath(), postpone=False)


def execute(self):
    """
    Extended Context.execute to perform packaging on games and tools.

    For an executable package to be processed by this context the wscript file must implement the package_[platform] function (i.e. package_darwin_x64), which can call the package_game or package_tool methods on this context. Those functions will create the necessary package_task objects that will be executed after all directories have been recursed through. The package_game/tool functions accept keyword arguments that define how the package_task should packge executable, resources, and assets that are needed. For more information about valid keyword arguments look at the package_task.__init__ method.
    """

    # On windows when waf is run with incredibuild we get called multiple times
    # but only need to be executed once. The check for the class variable is
    # here to make sure we run once per package command (either specified on
    # the command line or when it is auto added). If multiple package commands
    # are executed on the command line they will get run and this check does
    # not interfere with that.
    if getattr(self.__class__, 'is_running', False):
        return
    else:
        self.__class__.is_running = True

    # When the package_* functions are called they will set the group to
    # packaging then back to build. This way we can filter out the package
    # tasks and only execute them and not the build task_generators that will
    # be added as we recurse through the directories
    self.add_group('build')
    self.add_group('packaging')
    self.set_group('build')
        
    self.project = self.get_bootstrap_game()
    self.restore()
    if not self.all_envs:
        self.load_envs()


    # The package command may be executed before SetupAssistant is executed to
    # configure the project, which is valid. If that is the case an exception
    # will be thrown by lumberyard.py to indicate this. Catch the exception and
    # return so that builds can complete correctly.
    try:
        self.recurse([self.run_dir])
    except:
        Logs.info("Could not run the package command as the build has not been run yet.")
        return

    # display the time elapsed in the progress bar
    self.timer = Utils.Timer()

    group = self.get_group('packaging')

    # Generating the xcode project should only be done on macOS and if we actually have something to package (len(group) > 0)
    if len(group) > 0 and self.is_option_true('run_xcode_for_packaging') and self.platform in ['darwin_x64', 'ios', 'appletv']:
        Logs.debug("package: checking for xcode project... ") 
        platform = self.platform
        if 'darwin' in platform:
            platform = "mac" 

        # Check if the Xcode solution exists. We need it to perform bundle
        # stuff (processing Info.plist and icon assets...)
        project_name_and_location = "/{}/{}.xcodeproj".format(getattr(self.options, platform + "_project_folder", None), getattr(self.options, platform + "_project_name", None))
        if not os.path.exists(self.path.abspath() + project_name_and_location):
            Logs.debug("package: running xcode_{} command to generate the project {}".format(platform, self.path.abspath() + project_name_and_location)) 
            run_command('xcode_' + platform) 

    for task_generator in group:
        try:
            rs = task_generator.runnable_status
            scan = task_generator.scan
            run = task_generator.run
        except AttributeError:
            pass
        else:
            scan()
            run()


def package_game(self, **kw):
    """ 
    Packages a game up by creating and configuring a package_task object.

    This method should be called by wscript files when they want to package a
    game into a final form. See package_task.__init__ for more information
    about the various keywords that can be passed in to configure the packaing
    task.
    """

    if is_valid_package_request(self, **kw):
        if 'release' in self.config:
            kw['use_pak_files'] = True

        create_package_task(self, **kw)


def package_tool(self, **kw):
    """ 
    Packages a tool up by creating and configuring a package_task object.

    This method should be called by wscript files when they want to package a
    game into a final form. See package_task.__init__ for more information
    about the various keywords that can be passed in to configure the packaing
    task. Note that option to use pak files does not pertain to tools and is
    explicitly set to false by this function.
    """

    if is_valid_package_request(self, **kw):
        if kw.get('use_pak_files', False):
            Logs.info("package: Using pak files not supported for tools. Ignoring the option.")
            kw['use_pak_files'] = False

        create_package_task(self, **kw)


def is_valid_package_request(pkg, **kw):
    """ Returns if the platform and configuration specified for the package_task match what the package context has been created for"""
    executable_name = kw.get('target', None)
    if not executable_name:
        Logs.info("Skipping package because no target was specified.")
        return False

    has_valid_platform = False
    for platform in kw['platforms']:
        if pkg.platform.startswith(platform):
            has_valid_platform = True
            break

    if 'all' not in kw['platforms'] and not has_valid_platform:
        Logs.info("Skipping packaging {} because the host platform {} is not supported".format(executable_name, pkg.platform))
        return False

    if 'all' not in kw['configurations'] and pkg.config not in kw['configurations']:
        Logs.info("Skipping packaging {} because the configuration {} is not supported".format(executable_name, pkg.config))
        return False

    if pkg.options.project_spec:
        task_gen_name = kw.get('task_gen_name', executable_name)
        modules_in_spec = pkg.loaded_specs_dict[pkg.options.project_spec]['modules']
        if task_gen_name not in modules_in_spec:
            Logs.info("Skipping packaging {} because it is not part of the spec {}".format(executable_name, pkg.options.project_spec))
            return False

    return True


def create_package_task(self, **kw):
    executable_name = kw.get('target', None)

    Logs.debug("package: create_package_task {}".format(executable_name))

    has_valid_platform = False
    for platform in kw['platforms']:
        if platform in self.platform:
            has_valid_platform = True
            break

    if (not has_valid_platform and 'all' not in kw['platforms']):
        Logs.info("Skipping packaging {} because the host platform {} is not supported".format(executable_name, self.platform))
        return

    if (self.config not in kw['configurations'] and 'all' not in kw['configurations']):
        Logs.info("Skipping packaging {} because the configuration {} is not supported".format(executable_name, self.config))
        return

    kw['bld'] = self # Needed for when we build the task

    task_gen_name = kw.get('task_gen_name', executable_name)
    executable_task_gen = self.get_tgen_by_name(task_gen_name)

    if executable_task_gen and getattr(executable_task_gen,'output_folder', None):
        executable_source_node = self.srcnode.make_node(executable_task_gen.output_folder)
    else:
        executable_source_node = self.srcnode.make_node(self.get_output_folders(self.platform, self.config)[0].name)

    destination_node = getattr(self, 'destination', None)
    if not destination_node:
        destination_node = executable_source_node

    executable_dest_node = get_path_to_executable_package_location(self.platform, executable_name, destination_node)
    executable_source_node = executable_source_node.make_node(executable_name)
    if os.path.exists(executable_source_node.abspath()):
        new_task = package_task(env=self.env, **kw)
        new_task.set_inputs(executable_source_node)
        new_task.set_outputs(executable_dest_node.make_node(executable_name))
        self.add_to_group(new_task, 'packaging')
    else:
        if os.path.exists(executable_dest_node.make_node(executable_name).abspath()):
            Logs.info("Final package output already exists, skipping packaging of %s" % executable_source_node.abspath())
        else:
            Logs.warn("[WARNING] Source executable %s does not exist and final package artifact does not exist either. Did you run the build command before the package ommand?" % executable_source_node.abspath())


def symlink_libraries(self, source, destination): 
    """ Creates a smybolic link for libraries.
    
    An ant_glob is executed on the source node using "*" + result of get_dynamic_lib_extension to determine all the libraries that need to be linked into the destination. If the package is being built for a release configuration or the platform does not support symbolic links a copy will be made of the library.

    :param source: Source of the libraries
    :type source: waflib.Node
    :param destination: Location/path to create the link (or copy) of any libraries found in Source
    :type destination: String 
    """

    lib_extension = get_dynamic_lib_extension(self)
    Logs.debug("package: Copying files with pattern {} from {} to {}".format("*"+lib_extension, source.abspath(), destination))
    lib_files = source.ant_glob("*" + lib_extension)
    for lib in lib_files:
        self.create_symlink_or_copy(lib, destination, postpone=False)


def symlink_dependencies(self, dependencies, dependency_source_location_nodes, executable_dest_path):
    """ Creates a smybolic link dependencies.
    
    If the package is being built for a release configuration or the platform does not support symbolic links a copy will be made of the library.

    :param dependencies: Dependencies to link/copy to executable_dest_path
    :type dependencies: list of strings
    :param dependency_source_location_nodes: Source locations to find dependencies
    :type destination: List of Node objects 
    :param executable_dest_path: Path where the executable/dependencies should be placed
    :type executable_dest_path: String 
    """

    if not isinstance(dependency_source_location_nodes, list): 
        dependency_source_location_nodes = [dependency_source_location_nodes]

    Logs.debug('package: linking module dependencies {}'.format(dependencies))

    lib_prefix = get_platform_lib_prefix(self)
    lib_extension = get_dynamic_lib_extension(self)
    
    for dependency in dependencies:
        depend_node = []
        source_node = None
        for source_location in dependency_source_location_nodes:
            depend_node = source_location.ant_glob(lib_prefix + dependency + lib_extension, ignorecase=True) 
            if depend_node and len(depend_node) > 0:
                source_node = source_location
                break

            elif 'AWS' in dependency:
                # AWS libraries in use/use_lib use _ to separate the name,
                # but the actual library uses '-'. Transform the name and try
                # the glob again to see if we pick up the dependent library
                Logs.debug('package: Processing AWS lib so changing name to lib*{}*.dylib'.format(dependency.replace('_','-')))
                depend_node = source_location.ant_glob("lib*" + dependency.replace('_','-') + "*.dylib", ignorecase=True) 
                if depend_node and len(depend_node) > 0:
                    source_node = source_location
                    break

        if len(depend_node) > 0:
            Logs.debug('package: found dependency {} in {}'.format(depend_node, source_node.abspath(), depend_node))
            self.create_symlink_or_copy(depend_node[0], executable_dest_path, postpone=False)
        else:
            Logs.debug('package: Could not find the dependency {}. It may be a static library, in which case this can be ignored, or a directory that contains dependencies is missing'.format(dependency))


def create_symlink_or_copy(self, source_node, destination_path, **kwargs):
    """ 
    Creates a symlink or copies a file/directory to a destination.

    If the configuration is release or the python platform does not support symbolic links (like windows) this method will instead copy the source to the destination. Any keyword arguments that install_files or symlink_as support can also be passed in to this method (they will get passed to the underlying install_files or symlink_as calls without modification).

    :param source_node: Source to be linked/copied
    :param destination_path: Location to link/copy the source
    :type destination_path: string
    """

    source_path_and_name = source_node.abspath()
    Logs.debug("package: create_symlink_or_copy called with source: {} destination: {} args: {}".format(source_path_and_name, destination_path, kwargs))
    try:
        if os.path.isdir(source_path_and_name):
            Logs.debug("package: -> path is a directory")
            if should_copy_and_not_link(self):
                # recursively go through the directory and copy its stuff...
                files_to_copy = source_node.ant_glob("**/*")
                Logs.debug("package: -> in release so calling install_files on the directory")
                for file in files_to_copy:
                    self.install_files(destination_path, file, cwd=source_node, relative_trick=True, **kwargs)
            else:
                Logs.debug("package: -> can create a link")
                if not os.path.isdir(destination_path):
                    Logs.debug("package: -> calling junction_directory for {}. Does it exist? {}".format(destination_path, os.path.isdir(destination_path)))
                    junction_directory(source_path_and_name, destination_path)
                else:
                    Logs.debug("package: -> skipping creating a link as it exists already")

        else:
            Logs.debug("package: -> path is a file")
            if should_copy_and_not_link(self):
                Logs.debug("package: -> need to copy so calling install_files for the file {} {}".format(destination_path, source_node.abspath()))
                self.install_files(destination_path, [source_node], **kwargs)
            else:
                Logs.debug("package: -> calling symlink_as {} / {} ".format(destination_path, source_node.name))
                self.symlink_as(destination_path + "/" + source_node.name, source_path_and_name, **kwargs)
    except Exception as err:
        Logs.debug("package: -> got an exception {}".format(err))


for platform in PLATFORMS[Utils.unversioned_sys_platform()]:
    for configuration in CONFIGURATIONS:
        platform_config_key = platform + '_' + configuration

        # Create new class to execute package command with variant
        class_attributes = {
            'cmd'                    : 'package_' + platform_config_key, 
            'variant'                : platform_config_key, 
            'fun'                    : 'package_' + platform, 
            'config'                 : configuration, 
            'platform'               : platform, 
            'force_copy_of_assets'   : False,

            'execute'                : execute,
            'package_game'           : package_game,
            'package_tool'           : package_tool,
            'symlink_libraries'      : symlink_libraries, 
            'symlink_dependencies'   : symlink_dependencies,
            'create_symlink_or_copy' : create_symlink_or_copy,
        }

        subclass = type('PackageContext'+platform_config_key, (Build.InstallContext,), class_attributes)

