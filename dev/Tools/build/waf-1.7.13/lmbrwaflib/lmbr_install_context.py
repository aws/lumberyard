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
import os, sys

from waflib import Build, Context, Errors, Logs, Runner
from waflib.Configure import conf


class LmbrInstallContext(Build.InstallContext):

    def __init__(self, **kw):
        super(LmbrInstallContext, self).__init__(**kw)

        self.project = self.get_bootstrap_game_folder()
        self.platform_alias = self.platform_to_platform_alias(self.platform)

        self.assets_platform = self.get_bootstrap_assets(self.platform).lower()
        self.assets_cache_path = os.path.join('Cache', self.project, self.assets_platform)

        self.paks_required = any(pak_config for pak_config in ('release', 'performance') if self.config.startswith(pak_config))

        if self.is_windows_platform(self.platform):
            self.shaders_type = 'D3D11'
        elif self.is_apple_platform(self.platform):
            self.shaders_type = 'METAL'
        elif self.is_android_platform(self.platform):
            self.shaders_type = 'GLES3'
        else:
            self.shaders_type = self.platform_alias.title()

        # this is necessary for multiple commands to share the same variant directory and have 
        # proper dependency tracking
        self._old_dbfile = Context.DBFILE
        Context.DBFILE = '.wafpickle-{}-{}-{}-{}'.format(self.fun, sys.platform, sys.hexversion, Context.ABI)

    def __del__(self): 
        Context.DBFILE = self._old_dbfile

    def execute(self):
        Logs.info("[WAF] Executing '{}'".format(self.cmd))

        self.restore()
        if not self.all_envs:
            self.load_envs()

        # Setup the groups to filter out the command specific tasks
        self.add_group('build')
        self.add_group(self.group_name)

        self.set_group('build')
        try:
            self.recurse([self.run_dir])
        except Exception as the_error:
            Logs.warn("[WARN] Could not run the {} command: {}.".format(self.fun, the_error))
            return

        if hasattr(self, 'preprocess_tasks'):
            self.preprocess_tasks()

        self.execute_tasks()

    def total(self):
        total = 0
        for tg in self.get_group(self.group_name):
            try:
                num_tasks = len(tg.tasks)
                if num_tasks != 0:
                    total += num_tasks
                else:
                    total += 1
            except AttributeError:
                total += 1
        return total

    def get_build_iterator(self):
        self.set_group(self.group_name)
        self.cur = self.current_group

        self.post_group()

        tasks = self.get_tasks_group(self.cur)
        self.cur_tasks = tasks

        if tasks:
            yield tasks
        yield []

    def execute_tasks(self):
        self.compile()

    def use_vfs(self):
        if not hasattr(self, 'cached_use_vfs'):
            if self.paks_required:
                self.cached_use_vfs = False
            else:
                self.cached_use_vfs = (self.get_bootstrap_vfs() == '1')

        return self.cached_use_vfs

    def use_paks(self):
        if not hasattr(self, 'cached_use_paks'):
            self.cached_use_paks = False

            if self.paks_required:
                self.cached_use_paks = True
            else:
                paks_option = '{}_paks'.format(self.platform_alias)
                if hasattr(self.options, paks_option):
                    self.cached_use_paks = self.is_option_true(paks_option)

        return self.cached_use_paks

    def get_bootstrap_files(self):
        return [
            'bootstrap.cfg',
            os.path.join(self.project.lower(), 'config', 'game.xml')
        ]

    def get_project_root_node(self):
        if not hasattr(self, '_project_root_node'):
            self._project_root_node = self.launch_node()
            if self.is_engine_local():
                self._project_root_node = self._project_root_node.make_node(self.project)
        return self._project_root_node
    project_root_node = property(get_project_root_node, None)

    def get_assets_cache_node(self):
        if not hasattr(self, '_assets_cache_node'):
            if self.is_engine_local():
                self._assets_cache_node = self.launch_node().find_dir(self.assets_cache_path)
            else:
                self._assets_cache_node = self.project_root_node.find_dir(self.assets_cache_path)
        return self._assets_cache_node
    assets_cache_node = property(get_assets_cache_node, None)

    def get_resource_node(self):
        if not hasattr(self, '_resources_node'):
            project_settings_node = self.project_root_node.make_node('project.json')
            project_settings = self.parse_json_file(project_settings_node)

            code_folder = project_settings.get('code_folder', 'Gem')
            if code_folder:
                search_nodes = [
                    self.project_root_node,
                    self.engine_node,
                    self.root
                ]

                for path_node in search_nodes:
                    code_folder_node = path_node.find_dir(code_folder)
                    if code_folder_node:
                        break
                else:
                    self.fatal('[ERROR] Unable to locate code folder - {} - specified in project.json'.format(code_folder))
            else:
                code_folder_node = self.project_root_node.make_node('Gem')

            launcher_name = self.get_default_platform_launcher_name(self.platform)
            if launcher_name:
                self._resources_node = code_folder_node.make_node(['Resources', launcher_name])
            else:
                self._resources_node = code_folder_node.make_node('Resources')

        return self._resources_node
    resources_node = property(get_resource_node, None)

    def _get_base_layout_folders(self):
        launcher_name = self.get_default_platform_launcher_name(self.platform)
        if launcher_name:
            platform_folder = '{}{}'.format(self.project, launcher_name)
        else:
            platform_folder = '{}{}launcher'.format(self.project, self.platform_alias)
        return ['layouts', platform_folder.lower()]

    def get_layout_node(self):
        if not hasattr(self, 'layout_node'):
            layout_folders = self._get_base_layout_folders()
            layout_folders.append(self.config)

            if self.use_vfs():
                layout_folders.append('vfs')
            elif self.use_paks():
                layout_folders.append('pak')
            else:
                layout_folders.append('full')

            platform_specific_subfolders = getattr(self, '{}_layout_subfolders'.format(self.platform), [])
            layout_folders.extend(platform_specific_subfolders)

            self.layout_node = self.path.make_node(layout_folders)
        return self.layout_node

    def get_layout_vs_node(self):
        if not hasattr(self, 'vs_layout_node'):
            layout_folders = self._get_base_layout_folders()
            layout_folders.append('vs')

            platform_specific_subfolders = getattr(self, '{}_layout_subfolders'.format(self.platform), [])
            layout_folders.extend(platform_specific_subfolders)

            self.vs_layout_node = self.path.make_node(layout_folders)
        return self.vs_layout_node

    def is_platform_and_config_valid(self, **kw):

        if not self.is_valid_platform_request(debug_tag=self.fun, **kw):
            return False

        if not self.is_valid_configuration_request(debug_tag=self.fun, **kw):
            return False

        return True
