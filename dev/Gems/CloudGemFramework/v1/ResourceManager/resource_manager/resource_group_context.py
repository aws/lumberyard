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
# $Revision: #1 $

from errors import HandledError
from resource_group import ResourceGroup
import os
from resource_manager_common import constant
import util
import json

class ResourceGroupContext(object):

    def __init__(self, context):
        self.__context = context
        self.__resource_groups = {}

    def __iter__(self):
        return self.__resource_groups.iteritems()

    def __contains__(self, value):
        return value in self.__resource_groups

    def keys(self):
        return self.__resource_groups.keys()

    def values(self):
        return self.__resource_groups.values()

    def get(self, name, optional=False):
        result = self.__resource_groups.get(name, None)
        if result is None and not optional:
            raise HandledError('The resource group {} does not exist.'.format(name))
        return result

    def verify_exists(self, name):
        self.get(name, optional=False)

    def bootstrap(self, args):
        self.load_resource_groups()

    def initialize(self, args):
        self.bootstrap(args)


    def gem_enabled(self, gem):

        if gem.has_aws_file(constant.RESOURCE_GROUP_TEMPLATE_FILENAME):

            if gem.name in self.__resource_groups.keys():
                if self.__resource_groups[gem.name].gem is not gem:
                    raise HandledError('Multiple cloud gems named {} are enabled for the project. Cloud gems must have unique names within a project.'.format(gem.name))

            try:
                util.validate_stack_name(gem.name)
            except HandledError as e:
                raise HandledError('The gem name is not valid for use as a Cloud Gem. {}. You need to rename or disable the gem, or remove the gem\s AWS\resource-group-template.json file.'.format(e))

            self.__resource_groups[gem.name] = ResourceGroup(self.__context, gem.name, gem.aws_directory_path, gem.cpp_base_directory_path, gem.cpp_aws_directory_path, gem)


    def gem_disabled(self, gem):
        self.__resource_groups.pop(gem.name, None)


    def load_resource_groups(self):
        
        self.__resource_groups = {}

        # load resource groups defined by gems
        for gem in self.__context.gem.enabled_gems:
            self.gem_enabled(gem)

        # load resource groups defined by the project
        # TODO: Remove (deprecated in Lumberyard 1.11, CGF 1.1.1).
        if os.path.exists(self.__context.config.base_resource_group_directory_path):
            for name in os.listdir(self.__context.config.base_resource_group_directory_path):
                directory_path = os.path.join(self.__context.config.base_resource_group_directory_path, name)
                if os.path.isfile(os.path.join(directory_path, constant.RESOURCE_GROUP_TEMPLATE_FILENAME)):
                    if name in self.__resource_groups.keys():
                        raise HandledError(
                            'The project defines a resource group named {} and has enabled a cloud gem named {}. Cloud gems and resource groups must have unique names within a project. The resource group definition can be found at {}.'.format(
                                name, name, os.path.join(self.__context.config.base_resource_group_directory_path, name)))
                    cpp_base_directory_path = os.path.join(self.__context.config.root_directory_path, constant.PROJECT_CODE_DIRECTORY_NAME, self.__context.config.game_directory_name)
                    cpp_aws_directory_path = os.path.join(cpp_base_directory_path, constant.GEM_AWS_DIRECTORY_NAME, name)
                    self.__resource_groups[name] = ResourceGroup(self.__context, name, directory_path, cpp_base_directory_path, cpp_aws_directory_path)

    def before_update_framework_version_to_1_1_1(self, from_version):                 
        self.__context.view.version_update(from_version, json.dumps(self.__context.config.local_project_settings.raw_dict()), '1.1.1')
        self.__convert_enabled_list_to_disabled_list()
        self.__context.view.version_update_complete('1.1.1', json.dumps(self.__context.config.local_project_settings.raw_dict()))        

    def before_update_framework_version_to_1_1_2(self, from_version):        
        self.__context.view.version_update(from_version, json.dumps(self.__context.config.local_project_settings.raw_dict()), '1.1.2')
        self.__convert_enabled_list_to_disabled_list() 
        self.__context.view.version_update_complete('1.1.2', json.dumps(self.__context.config.local_project_settings.raw_dict()))                

    def before_update_framework_version_to_1_1_3(self, from_version):                
        self.__context.view.version_update(from_version, json.dumps(self.__context.config.local_project_settings.raw_dict()), '1.1.3')
        self.__create_default_section()
        self.__set_default_section_from_project_stack_id(from_version)
        self.__move_attributes()        
        self.__context.view.version_update_complete('1.1.3', json.dumps(self.__context.config.local_project_settings.raw_dict()))        

    def __convert_enabled_list_to_disabled_list(self):

        # Converts the EnabledResourceGroups property from local_project_settings.json into
        # a DisabledResourceGroups property.

        complete_set = set(self.keys())
        settings = self.__context.config.local_project_settings.raw_dict()        
        enabled_set = set(settings.get(constant.ENABLED_RESOURCE_GROUPS_KEY, []))     
        disabled_set = complete_set - enabled_set        
        settings[constant.DISABLED_RESOURCE_GROUPS_KEY] = list(disabled_set)    
        settings.pop(constant.ENABLED_RESOURCE_GROUPS_KEY, None)    
        # save not needed, will happen after updating the framework version number

    def __create_default_section(self):
         self.__context.config.local_project_settings.create_default_section()

    def __set_default_section_from_project_stack_id(self,from_version):
        # if the local project setting is initialized
        # parse the region from the PendingProjectStackId or the ProjectStackId
        settings = self.__context.config.local_project_settings.raw_dict()     
        if constant.PROJECT_STACK_ID in settings:
            project_stack_id = settings[constant.PROJECT_STACK_ID]
            region = util.get_region_from_arn(project_stack_id)
            #create the region section
            self.__context.config.local_project_settings.default(region)
            self.__context.config.local_project_settings[constant.PROJECT_STACK_ID] = project_stack_id

        if constant.PENDING_PROJECT_STACK_ID in settings:
            pending_project_stack_id = settings[constant.PENDING_PROJECT_STACK_ID]
            region = util.get_region_from_arn(pending_project_stack_id)
            #create the region section
            self.__context.config.local_project_settings.default(region)
            

        # if the local project setting is NOT initialized
        # locate the region in the request context and set the default to that region
        if not self.__context.config.local_project_settings.is_default_set_to_region():                    
            #move to migration section defined by the version
            self.__context.config.local_project_settings.default(str(from_version))
            self.__context.config.local_project_settings[constant.LAZY_MIGRATION] = True            
        
    
    def __move_attributes(self):        
        if not self.__context.config.local_project_settings.is_default_set_to_region():
            return

        settings = self.__context.config.local_project_settings.raw_dict()     
        if constant.ENABLED_RESOURCE_GROUPS_KEY in settings:            
            settings.pop(constant.ENABLED_RESOURCE_GROUPS_KEY, None)
        
        if constant.ENABLED_RESOURCE_GROUPS_KEY in self.__context.config.local_project_settings:   
            del self.__context.config.local_project_settings[constant.ENABLED_RESOURCE_GROUPS_KEY]

        if constant.ENABLED_RESOURCE_GROUPS_KEY in self.__context.config.local_project_settings.default_set():   
            del self.__context.config.local_project_settings.default_set()[constant.ENABLED_RESOURCE_GROUPS_KEY]

        if constant.DISABLED_RESOURCE_GROUPS_KEY in settings:
            self.__context.config.local_project_settings[constant.DISABLED_RESOURCE_GROUPS_KEY] = settings[constant.DISABLED_RESOURCE_GROUPS_KEY]
            settings.pop(constant.DISABLED_RESOURCE_GROUPS_KEY, None)

        if constant.PROJECT_STACK_ID in settings:
            self.__context.config.local_project_settings[constant.PROJECT_STACK_ID] = settings[constant.PROJECT_STACK_ID]
            settings.pop(constant.PROJECT_STACK_ID, None)

        if constant.PENDING_PROJECT_STACK_ID in settings:
            self.__context.config.local_project_settings[constant.PENDING_PROJECT_STACK_ID] = settings[constant.PENDING_PROJECT_STACK_ID]
            settings.pop(constant.PENDING_PROJECT_STACK_ID, None)

        settings.pop(constant.FRAMEWORK_VERSION_KEY, None)