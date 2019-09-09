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
import os
from resource_manager_common import constant
import util
import json

class ResourceGroupList(list):
    def __init__(self, context):
        super(ResourceGroupList, self).__init__(self)        

    def __iter__(self):
        return self.__dict__.iteritems()

    def __contains__(self, value):
        return value in self.__dict__

    def keys(self):
        return self.__dict__.keys()

    def values(self):
        return self.__dict__.values()

    def add_resource_group(self, resource_group):
        if resource_group.name in self.__dict__.keys():
            raise HandledError('Multiple resource groups named {} are enabled for the project.'.format(resource_group.name))

        try:
            util.validate_stack_name(resource_group.name)
        except HandledError as e:
            raise HandledError('The resource group name \'{}\' is not a valid name. '.format(e))
        
        self.__dict__[resource_group.name] = resource_group

    def remove_resource_group(self, resource_group):
        self.__dict__.pop(resource_group.name, None)


    def get(self, name, optional=False):
        result = self.__dict__.get(name, None)
        if result is None and not optional:
            raise HandledError('The resource group {} does not exist.'.format(name))
        return result

class ResourceGroupController(object):

    def __init__(self, context):
        self.__context = context
        self.__resource_groups = ResourceGroupList(context)

    def bootstrap(self, args):
        pass

    def initialize(self, args):
        self.bootstrap(args)

    @property
    def resource_groups(self):
        return self.__resource_groups

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

    def before_update_framework_version_to_1_1_4(self, from_version):                
        self.__context.view.version_update(from_version, json.dumps(self.__context.config.local_project_settings.raw_dict()), '1.1.4')                
        self.__context.view.version_update_complete('1.1.4', json.dumps(self.__context.config.local_project_settings.raw_dict()))     

    def __convert_enabled_list_to_disabled_list(self):

        # Converts the EnabledResourceGroups property from local_project_settings.json into
        # a DisabledResourceGroups property.
        keys = self.resource_groups.keys()        
        complete_set = set(keys)
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