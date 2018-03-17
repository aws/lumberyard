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
# $Revision$

from botocore.exceptions import ClientError
from errors import HandledError

import os
import imp
import importlib
import sys
import os.path
import traceback

import common_code

class HookContext(object):

    def __init__(self, context):
        self.context = context
        self.__hook_modules = {}

    def load_modules(self, module_name):

        module_hooks = []
        gems_seen = set()

        project_module_path = os.path.join(self.context.config.aws_directory_path, module_name)
        if os.path.isfile(project_module_path):
            module_hooks.append(HookModule(self.context, project_module_path))

        for resource_group in self.context.resource_groups.values():
            resource_group_directory_path = resource_group.directory_path
            resource_group_module_path = os.path.join(resource_group_directory_path, module_name)
            if os.path.isfile(resource_group_module_path):
                module_hooks.append(HookModule(self.context, resource_group_module_path, resource_group = resource_group))
            if resource_group.gem:
                gems_seen.add(resource_group.gem)

        for gem in self.context.gem.enabled_gems:
            if gem in gems_seen:
                continue
            gem_module_path = os.path.join(gem.aws_directory_path, module_name)
            if os.path.isfile(gem_module_path):
                module_hooks.append(HookModule(self.context, gem_module_path, gem = gem))

        self.__hook_modules[module_name] = module_hooks

    def call_single_module_handler(self, module_name, handler_name, resource_group_name, args=(), kwargs={}, deprecated=False):
        '''Calls a function in a hook module for a specified resource group.

        Args:

            module_name - the name of the module
            handler_name - the name of the function
            resource_group_name - The name of the resource group.
            args (named) - a list containing the positional args passed to the handler. Default is ().
            kwargs (named) - dict containing the key word args passed to the handler. Default is {}.
            deprecated (named) - indicates if this is a deprecated hook. Default is False.

        Notes:

            If deprecated is True, a warning is displayed if the hook method exists. It is still invoked.

        '''

        if not self.__hook_modules.get(module_name, None):
            self.load_modules(module_name)

        hook_modules = self.__hook_modules[module_name]

        resource_group = self.context.resource_groups.get(resource_group_name)

        for hook_module in hook_modules:
            if hook_module.resource_group == resource_group:
                hook_module.call_handler(handler_name, args = args, kwargs = kwargs, deprecated = deprecated)

    def call_module_handlers(self, module_name, handler_name, args=(), kwargs={}, deprecated=None):
        '''Calls a function in a hook module.

        Args:

            module_name - the name of the module
            handler_name - the name of the function
            args (named) - a list containing the positional args passed to the handler. Default is ().
            kwargs (named) - dict containing the key word args passed to the handler. Default is {}.
            deprecated (named) - indicates if this is a deprecated hook. Default is False.

        Notes:

            If depcreated is True, a warning is displayed if the hook method exists. It is still invoked.

        '''

        if not self.__hook_modules.get(module_name, None):
            self.load_modules(module_name)

        hook_modules = self.__hook_modules[module_name]

        for hook_module in hook_modules:
            hook_module.call_handler(handler_name, args = args, kwargs = kwargs, deprecated = deprecated)


class HookModule(object):

    '''An individual hook module.  Can belong to a project, gem, or resource group'''

    def __init__(self, context, module_path, resource_group = None, gem = None):
        self.__context = context
        self.__resource_group = resource_group
        self.__gem = gem
        self.__hook_name = resource_group.name if resource_group else gem.name if gem else 'PROJECT'
        self.__module_path = module_path  # Full path to module
        self.__module_directory = os.path.split(module_path)[0]  # Module directory without file/module name
        self.__module_lib_directory = os.path.join(self.__module_directory, 'lib')
        self.__module_name_with_extension = os.path.split(module_path)[1] # Module name with .py or other extension if exists
        self.__module_name = os.path.splitext(self.__module_name_with_extension)[0] # No path, no extension, just module name
        self.__hook_module_name = self.__hook_name + self.__module_name # Mangled name to prevent collisions among same module names under different resources
        self.__module = None
        self.__load_module()

    def __load_module(self):
        try:
            self.__module = sys.modules.get(self.__hook_module_name, None)
            if self.__module is None:
                imp.acquire_lock()
                self.__module = sys.modules.get(self.__hook_module_name, None)
                if self.__module is not None:
                    imp.release_lock()
                    return
                added_paths, multi_imports = self.__add_plugin_paths()
                try:
                    for module, imported_gem_names in multi_imports.iteritems():
                        loader = MultiImportModuleLoader(module, imported_gem_names)
                        loader.load_module(module)
                    self.__module = imp.load_source(self.__hook_module_name, self.__module_path)
                finally:
                    self.__remove_plugin_paths(added_paths)
                    imp.release_lock()
        except Exception as e:
            raise HandledError('Failed to load hook module {}. {}'.format(self.__module_path, traceback.format_exc()))

    def call_handler(self, handler_name, args = (), kwargs={}, deprecated = False):
        try:
            thisHandler = getattr(self.__module, handler_name,None)
            if thisHandler is not None:
                if deprecated:
                    self.context.view.calling_deprecated_hook(self.__module, handler_name)
                else:
                    if args:
                        raise ValueError('The args parameter is only supported for deprecated hooks. Using only kwargs (key word args), and requiring all hooks should have an **kwargs parameter, allows new args to be added in the future.')
                    self.context.view.calling_hook(self.__module, handler_name)
                return thisHandler(self, *args, **kwargs)
        except:
            raise HandledError('{} in {} failed. {}'.format(handler_name, self.__module_path, traceback.format_exc()))

    def __add_plugin_paths(self):
        added_paths = [self.__module_directory, self.__module_lib_directory]
        imported_paths, multi_imports = common_code.resolve_imports(self.context, self.__module_directory)
        added_paths.extend(imported_paths)
        sys.path.extend(added_paths)
        return added_paths, multi_imports

    def __remove_plugin_paths(self, added_paths):
        for added_path in added_paths: 
            sys.path.remove(added_path)
   
    @property
    def context(self):
        return self.__context

    @property
    def resource_group(self):
        '''The resource group that implements the hook, if any.'''
        return self.__resource_group

    @property
    def gem(self):
        '''The gem that implements the hook, if any.'''
        return self.__gem

    @property     
    def hook_name(self):
        '''Name of the resource group or Gem that provides the hook.'''
        return self.__hook_name

    # Deprecated in 1.9. TODO: remove.
    @property     
    def group_name(self):
        '''Depcreated. Use hook_name.'''
        return self.__hook_name

    # Deprecated in 1.9. TODO: remove.
    @property     
    def path(self):
        '''Deprecated. Use hook_path instead.'''
        return self.__module_directory

    @property     
    def hook_path(self):
        '''Path to the resource group or Gem directory where the hook is defined.'''
        return self.__module_directory

class MultiImportModuleLoader(object):

    '''A module loader that handles loading multiple sub-modules from different directories that need to be imported into a single module namespace.
    The implementation here should be the local workspace equivalent of how zip_and_upload_lambda_function_code in uploader.py handles multi-imports.'''

    def __init__(self, import_package_name, imported_gem_names):
        self.__import_package_name = import_package_name
        self.__imported_gem_names = sorted(imported_gem_names)

    def load_module(self, fullname):
        if fullname != self.__import_package_name:
            raise ImportError('Unable to load {}. This loader only supports loading {}'.format(fullname, self.__import_package_name))

        imp.acquire_lock()
        try:
            module_name = self.__import_package_name
            module = sys.modules.get(module_name)
            if module is not None:
                return module

            module = imp.new_module(module_name)
            module.__file__ = '<CloudCanvas::import::{}>'.format(self.__import_package_name)
            module.__loader__ = self
            module.__path__ = []
            module.__package__ = module_name

            # Define __all__ for "from package import *".
            module.__all__ = self.__imported_gem_names

            module.imported_modules = {}

            for gem_name in self.__imported_gem_names:
                top_level_name = '{}__{}'.format(module_name, gem_name)
                try:
                    imported_module = importlib.import_module(top_level_name)
                except:
                    raise HandledError('Failed to import {} while importing "*.{}". {}'.format(top_level_name, module_name, traceback.format_exc()))

                # Import each top level module ( MyModule__CloudGemName ) using the gem name as the sub-module name ( MyModule.CloudGemName ).
                setattr(module, gem_name, imported_module)

                # A dictionary of gem name to loaded module for easy iterating ( imported_modules = { CloudGemName: <the_loaded_CloudGemName_module> } ).
                module.imported_modules[gem_name] = imported_module

            sys.modules[module_name] = module
            return module
        finally:
            imp.release_lock()
