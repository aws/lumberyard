#!/usr/bin/env python
# encoding: utf-8

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

from waflib import Logs, Utils, Errors, Build
import os
import collections
import json

# The Visual Code schema version for launch tasks that we are basing on
LAUNCH_JSON_VERSION = '0.2.0'

# The Visual Code schema version for (build) tasks that we are basing on
TASK_JSON_VERSION = '2.0.0'

# Map of unversioned host platforms to platform specific values and format strings that we will use during construction
# of the visual code json configuration files
VSCODE_PLATFORM_SETTINGS_MAP = {
    
    'win32':  dict( exePattern          = "%s.exe",
                    scriptPattern       = "%s.bat",
                    dbgType             = "cppvsdbg",
                    problemMatcher      = '$msCompile',
                    wafPlatformPrefix   = "win_x64"),
    
    'darwin': dict( exePattern          = "./%s",
                    scriptPattern       = "./%s.sh",
                    dbgType             = "cppdbg",
                    problemMatcher      = "$gcc",
                    wafPlatformPrefix   = "darwin_"),

    'linux':  dict( exePattern          = "./%s",
                    scriptPattern       = "./%s.sh",
                    dbgType             = "cppdbg",
                    problemMatcher      = "$gcc",
                    wafPlatformPrefix   = "linux_")
}


class GenerateVSCodeWorkspace(Build.BuildContext):
    cmd = 'vscode'
    fun = 'build'
    
    default_build_configuration = 'profile'
    
    default_launch_targets = ['Editor', 'AssetProcessor']
    
    def __init__(self):
        
        Build.BuildContext.__init__(self)
        
        # Get the host-platform specific values for this visual code generation and initialize them as
        # object variables to this build class

        host_name = Utils.unversioned_sys_platform()
        try:
            platform_settings = VSCODE_PLATFORM_SETTINGS_MAP[host_name]
        except KeyError:
            raise Errors.WafError("Unsupported platform '{}' for VSCode".format(host_name))
        try:
            self.exePattern         = platform_settings['exePattern']
            self.dbgType            = platform_settings['dbgType']
            self.wafPlatformPrefix  = platform_settings['wafPlatformPrefix']
            self.scriptPattern      = platform_settings['scriptPattern']
            self.problemMatcher     = platform_settings['problemMatcher']
        except KeyError as err:
            raise Errors.WafError("VSCode settings '{}' for platform '{}' is missing from the definition table ({})".format(str(err.message), host_name, __file__))

    @staticmethod
    def write_vscode_node(node, json_obj):
        node.write(json.dumps(json_obj, indent=4, separators=(',', ': ')))

    def execute(self):
        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()
            
        self.load_user_settings()
        
        self.generate_vscode_project()

    def generate_build_tasks_json(self, vscode_dir):
        """
        Generate the build tasks (task.json) file for Visual Code
        :param vscode_dir:  The base folder node to save the task.json file
        """
        
        # Collect all the specs that are specified in 'specs_to_include_in_project_generation'
        enabled_specs = [spec_name.strip() for spec_name in self.options.specs_to_include_in_project_generation.split(',')]
        
        # Collect all the enabled platforms
        enabled_platforms = self.get_enabled_target_platform_names()
        
        task_list = []

        # Prepare the command line for this build task
        lmbr_waf_command = os.path.join('${workspaceFolder}', self.scriptPattern % 'lmbr_waf')
        
        default_build_set = False

        for platform in enabled_platforms:
            # The top level hierarchy will be the 'filtered' platforms
            
            if not platform.startswith(self.wafPlatformPrefix):
                # Skip if it doesnt match the platform prefix
                continue
                
            # Iterate through the specs
            for spec_name in enabled_specs:
                
                # The second level heirarchy will be based on the spec name
                configurations_for_platform = self.get_supported_configurations(platform)
                for configuration in configurations_for_platform:
                    
                    # The third level heirarchy will be based on the configuration name
                    
                    # Set a 'default' build tasks from the first one that matches the desired default build
                    # configuration
                    if not default_build_set and configuration == GenerateVSCodeWorkspace.default_build_configuration:
                        default_build_set = True
                        is_default_build = True
                    else:
                        is_default_build = False
                    
                    task = collections.OrderedDict()

                    task['label']           = '[{}] {} : ({})'.format(spec_name, configuration, platform)
                    task['type']            = 'shell'
                    task['command']         = lmbr_waf_command
                    task['args']            = ['build_{}_{}'.format(platform, configuration),
                                               '-p',
                                                spec_name]
                    task['problemMatcher']  = {
                                                "base": self.problemMatcher,
                                                "fileLocation": "absolute"
                                              }
                    if is_default_build:
                        # Default build tasks specifically requires another dictionary
                        task['group']       = {
                                                'kind': 'build',
                                                'isDefault': True
                                              }
                    else:
                        task['group']       = 'build'
                    
                    task_list.append(task)
        
        tasks_json = collections.OrderedDict()
        
        tasks_json['version'] = TASK_JSON_VERSION
        tasks_json['tasks'] = task_list
        
        tasks_json_node = vscode_dir.make_node('tasks.json')

        GenerateVSCodeWorkspace.write_vscode_node(tasks_json_node, tasks_json)

    def generate_launch_json(self, vscode_dir):
        """
        Generate the launch configurations based on specific exe's and the enabled game project's game launchers
        :param vscode_dir:  The base folder node to save the task.json file
        """
        
        # Collect the possible launch targets
        launcher_targets = [default_launch_target for default_launch_target in GenerateVSCodeWorkspace.default_launch_targets]
        enabled_game_projects = [game_name.strip() for game_name in self.options.enabled_game_projects.split(',')]
        launcher_targets.extend(enabled_game_projects)
        
        # Collect the enabled platforms for this host platform
        enabled_platforms = self.get_enabled_target_platform_names()

        launch_configurations = []

        # Follow the same logic as the build tasks to filter the platforms
        for platform in enabled_platforms:
            
            if not platform.startswith(self.wafPlatformPrefix):
                # Skip if it doesnt match the platform prefix
                continue
    
            # Iterate through the configurations for the current platform
            configurations_for_platform = self.get_supported_configurations(platform)
            for configuration in configurations_for_platform:
                
                is_dedicated = '_dedicated' in configuration
                    
                bin_folder = self.get_output_folders(platform, configuration)[0]
                
                for launcher_target in launcher_targets:
                    # For dedicated platforms, the name of the launcher will be different, so we need to adjust the
                    # name for these configurations. For non-game launcher targets, they do not exist at all,
                    # so dont create them
                    if is_dedicated:
                        if launcher_target in enabled_game_projects:
                            exename = self.exePattern % ('{}Launcher_Server'.format(launcher_target))
                        else:
                            # This is not a game launcher, so for dedicated configurations skip
                            continue
                    else:
                        if launcher_target in enabled_game_projects:
                            exename = self.exePattern % ('{}Launcher'.format(launcher_target))
                        else:
                            exename = self.exePattern % (launcher_target)
                    
                    working_path = os.path.join('${workspaceFolder}', bin_folder.name)
                    program = os.path.join(working_path, exename)
                    
                    launch_config = collections.OrderedDict()

                    launch_config['name']               = '{} ({})'.format(launcher_target, configuration)
                    launch_config['type']               = self.dbgType
                    launch_config['request']            = 'launch'
                    launch_config['program']            = program
                    launch_config['args']               = []
                    launch_config['cwd']                = working_path
                    launch_config['environment']        = []
                    launch_config['externalConsole']    =   False

                    launch_configurations.append(launch_config)
        
        launch_json = collections.OrderedDict()
        
        launch_json['version']        = LAUNCH_JSON_VERSION
        launch_json['configurations'] = launch_configurations
        
        launch_json_node = vscode_dir.make_node('launch.json')
        GenerateVSCodeWorkspace.write_vscode_node(launch_json_node, launch_json)

    def generate_settings_json(self, vscode_dir):
        """
        Generate the additional (environment) visual code settings
        
        :param vscode_dir:  The base folder node to save the task.json file
        """
        settings_json = {
            "files.exclude": {
                "Bin*"              : True,
                "Cache"             : True,
                "AssetProcessorTemp": True,
                "Solutions"         : True
            },
            "files.associations": {
                "wscript"       : "python",
                "*.py"          : "python"
            },
        }

        settings_json_node = vscode_dir.make_node('settings.json')
        GenerateVSCodeWorkspace.write_vscode_node(settings_json_node, settings_json)

    def generate_vscode_project(self):
        """
        Generate all of the visual code settings
        """
        # ensure that the .vscode dir exists
        vscode_dir = self.srcnode.make_node('.vscode')
        vscode_dir.mkdir()

        self.generate_build_tasks_json(vscode_dir)
        self.generate_launch_json(vscode_dir)
        self.generate_settings_json(vscode_dir)
