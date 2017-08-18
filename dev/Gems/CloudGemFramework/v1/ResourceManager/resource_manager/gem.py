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

import os
import util
from errors import HandledError
import constant

class GemContext(object):

    def __init__(self, context):
        self.__context = context
        self.__enabled_gems = None
        self.__gems = {}

    @property
    def enabled_gems(self):
        self.__load_enabled_gems()
        return self.__enabled_gems

    def __load_enabled_gems(self):

        if self.__enabled_gems:
            return

        self.__enabled_gems = []
            
        gems_file_path = os.path.join(self.__context.config.game_directory_path, constant.PROJECT_GEMS_DEFINITION_FILENAME)
        if os.path.exists(gems_file_path):
            gems_file = self.__context.config.load_json(gems_file_path)
            for gems_file_object in gems_file.get('Gems',[]):
                relative_path = gems_file_object.get('Path', None)
                if relative_path is None:
                    raise HandledError('No Path property provided for {} in {}.'.format(gems_file_object, gems_file_path))
                root_directory_path = os.path.abspath(os.path.join(self.__context.config.root_directory_path, relative_path.replace('/', os.sep)))
                gem = Gem(self.__context, root_directory_path, is_enabled = True)
                self.__gems[root_directory_path] = gem
                self.__enabled_gems.append(gem)
        else:
            # fail gracefully in this case - tests do this
            self.__context.view.missing_gems_file(gems_file_path)

    def get_by_root_directory_path(self, root_directory_path):
        self.__load_enabled_gems()
        gem = self.__gems.get(root_directory_path, None)
        if gem is None:
            gem = Gem(self.__context, root_directory_path, is_enabled = False)
            self.__gems[root_directory_path] = gem
        return gem

    def get_by_name(self, gem_name):
        self.__load_enabled_gems()
        for gem in self.__enabled_gems:
            if gem.name == gem_name:
                return gem
        return None


class Gem(object):

    def __init__(self, context, root_directory_path, is_enabled):
        self.__context = context
        self.__root_directory_path = root_directory_path
        self.__is_enabled = is_enabled
        self.__aws_directory_path = os.path.join(self.__root_directory_path, constant.GEM_AWS_DIRECTORY_NAME)
        self.__gem_file_path = os.path.join(self.__root_directory_path, constant.GEM_DEFINITION_FILENAME)
        self.__gem_file_object_ = None
        self.__cli_plugin_code_path = os.path.join(self.__aws_directory_path, 'cli-plugin-code')
        self.__cgp_resource_code_paths = os.path.join(self.__aws_directory_path, 'cgp-resource-code')
    
    @property
    def __gem_file_object(self):
        if self.__gem_file_object_ is None:
            if not os.path.isfile(self.__gem_file_path):
                raise HandledError('The {} directory contains no gem.json file.'.format(self.__root_directory_path))
            self.__gem_file_object_ = self.__context.config.load_json(self.__gem_file_path)
        return self.__gem_file_object_

    @property
    def is_enabled(self):
        return self.__is_enabled

    @property
    def is_defined(self):
        return os.path.isfile(self.__gem_file_path)

    @property
    def name(self):
        name = self.__gem_file_object.get('Name', None)
        if name is None:
            raise HandledError('No Name property found in the gem.json file at {}'.format(self.__root_directory_path))
        return name

    @property
    def version(self):
        return self.__gem_file_object.get('Version', None)

    @property
    def display_name(self):
        display_name = self.__gem_file_object.get('DisplayName', None)
        if display_name is None:
            display_name = self.name
        return display_name

    @property
    def root_directory_path(self):
        return self.__root_directory_path

    @property
    def relative_path(self):
        return self.__relative_path

    @property
    def aws_directory_path(self):
        return self.__aws_directory_path

    @property
    def aws_directory_exists(self):
        return os.path.isdir(self.__aws_directory_path)

    @property
    def cpp_directory_path(self):
        return os.path.join(self.root_directory_path, constant.GEM_CODE_DIRECTORY_NAME, constant.GEM_AWS_DIRECTORY_NAME)

    @property
    def cli_plugin_code_path(self):
        return self.__cli_plugin_code_path

    @property
    def cgp_resource_code_path(self):
        return self.__cgp_resource_code_path

    @property
    def file_object(self):
        return self.__gem_file_object

    @property
    def uuid(self):
        return self.__gem_file_object['Uuid'] if 'Uuid' in self.__gem_file_object else None

 
