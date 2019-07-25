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
# $Revision: #2 $

import os
import json
import shutil
import util
import imp
import re
from resource_manager_common import constant 
import copy
import file_util
import collections
import time
import sys

from StringIO import StringIO
from errors import HandledError
from ConfigParser import RawConfigParser
from botocore.exceptions import ClientError
from cgf_utils.version_utils import Version

RESOURCE_MANAGER_PATH = os.path.dirname(__file__)

class ConfigContext(object):

    '''Implements the configuration model for Lumberyard resource management.

    Uses args to determine the location of resource management data
    in the local file system, then uses the configuration data it finds to
    locatate the configuration bucket used by the project.
    '''

    def __init__(self, context):

        self.__context = context

        self.__deployment_names = None
        self.__project_settings = None
        self.__configuration_bucket_name = None
        self.__project_resources = None
        self.__framework_version = None
        self.__aggregate_settings = None

    @property
    def context(self):
        return self.__context

    def bootstrap(self, args):
        '''Does minimal initialization using the root_directory, game_directory, aws_directory, and user_directory arguments.'''
        
        self.__verbose = args.verbose

        if args.root_directory:
            self.root_directory_path = args.root_directory
        else:
            self.root_directory_path = os.getcwd()

        if args.game_directory:
            self.game_directory_name = os.path.basename(args.game_directory)
            self.game_directory_path = args.game_directory
        else:
            self.game_directory_name = self.get_game_directory_name()
            self.game_directory_path = os.path.join(self.root_directory_path, self.game_directory_name)

        if args.aws_directory:
            self.aws_directory_path = args.aws_directory
        else:
            self.aws_directory_path = os.path.join(self.game_directory_path, constant.PROJECT_AWS_DIRECTORY_NAME)

        if args.user_directory:
            self.user_directory_path = args.user_directory
        else:
            platform_mapping = {"win32": "pc", "darwin": "osx_gl"}
            self.user_directory_path = os.path.join(self.root_directory_path, constant.PROJECT_CACHE_DIRECTORY_NAME, self.game_directory_name, platform_mapping[sys.platform], constant.PROJECT_USER_DIRECTORY_NAME, constant.PROJECT_AWS_DIRECTORY_NAME)

        if args.tools_directory:
            self.tools_directory_path = args.tools_directory
        else:
            self.tools_directory_path = os.path.join(self.root_directory_path, constant.PROJECT_TOOLS_DIRECTORY_NAME)

        self.base_resource_group_directory_path = os.path.join(self.aws_directory_path, 'resource-group')
        self.gem_directory_path = os.path.join(self.root_directory_path, constant.PROJECT_GEMS_DIRECTORY_NAME)
        
        self.region = args.region_override if args.region_override else args.region
        self.local_project_settings = LocalProjectSettings(self.context, self.join_aws_directory_path(constant.PROJECT_LOCAL_SETTINGS_FILENAME), self.region)

    def initialize(self, args):
        self.__load_user_settings()
        self.__load_aws_directory()
        self.__load_resource_name_validation_config()

        self.__assume_role_name = args.assume_role # may be None
        self.__assume_role_deployment_name = args.deployment # may be None        
        self.__no_prompt = args.no_prompt

    def __load_user_settings(self):
        self.user_settings_path = os.path.join(self.user_directory_path, constant.USER_SETTINGS_FILENAME)
        self.gui_refresh_file_path = os.path.join(self.user_directory_path, 'gui-refresh-trigger')
        self.user_settings = self.load_json(self.user_settings_path)
        self.user_default_profile = self.user_settings.get(constant.DEFAULT_PROFILE_NAME, None)
        if not self.context.aws.profile_exists(self.user_default_profile):
            self.user_default_profile = None
        self.context.aws.set_default_profile(self.user_default_profile)

    def refresh_user_settings(self):
        self.__load_user_settings()

    def __load_resource_name_validation_config(self):
        self.resource_name_validation_config_path = os.path.join(self.resource_manager_path, "config", constant.RESOURCE_NAME_VALIDATION_CONFIG_FILENAME)
        self.resource_name_validation_config = self.load_json(self.resource_name_validation_config_path)

    # returns a dict consisting of a boolean indicating if it's valid and the
    # help string
    def validate_resource(self, resource_type, field, resource):
        resource_type_data = self.resource_name_validation_config.get(resource_type, None)
        field_data = resource_type_data.get(field, None)
        regex_str = field_data.get("regex", None) + '$' # re.match needs word end which doesn't work with QValidator
        help_str = field_data.get("help", None)
        min_len = field_data.get("min_length", 0)

        if len(resource) > min_len and re.match(regex_str, resource) is not None:
            return {'isValid' : True, 'help' : help_str}
        else:
            return {'isValid' : False, 'help' : help_str}

    def save_resource_group_template(self, resource_group):
        group = self.context.resource_groups.get(resource_group)
        group.save_template()

    def force_gui_refresh(self):
        dir = os.path.dirname(self.gui_refresh_file_path)
        if not os.path.exists(dir):
            os.makedirs(dir)
        with open(self.gui_refresh_file_path, 'w') as file:
            file.write('This file is written when the Cloud Canvas Resource Manager GUI should be refreshed.')

    def load_json(self, path, default=None):
        '''Reads JSON format data from a file on disk and returns it as dictionary.'''
        self.context.view.loading_file(path)
        obj = default
        if not obj:
            obj = {}
        return util.load_json(path, obj)


    def save_json(self, path, data):
        self.context.view.saving_file(path)
        util.save_json(path,data)

    def is_writable(self, path):
        '''If the target file exists, check if it can be opened for writing'''
        try:
            if os.path.exists(path) == False: 
                '''Path doesn't exist, assume writable'''
                return True
            if os.access(path, os.W_OK):
                '''File is writable'''
                return True
            return False
        except:
            return False
        return True

    def validate_writable(self, path):
        if not self.is_writable(path):

            if self.no_prompt:
                raise HandledError('File Not Writable: {}'.format(path))
                
            writable_list = []
            writable_list.append(path)

            util.validate_writable_list(self.context, writable_list)
        return self.is_writable(path)
        
    @property
    def no_prompt(self):
        return self.__no_prompt

    @property
    def aggregate_settings(self):
        return self.__aggregate_settings

    @aggregate_settings.setter
    def aggregate_settings(self, value):
        self.__aggregate_settings = value

    @property
    def project_stack_id(self):
        id = self.local_project_settings.get(constant.PROJECT_STACK_ID, None)         
        if id is None:
            id = self.local_project_settings.get(constant.PENDING_PROJECT_STACK_ID, None) 
        return id

    @property
    def project_initialized(self):         
        id = self.local_project_settings.get(constant.PROJECT_STACK_ID, None)        
        return id != None

    def __load_aws_directory(self):

        self.has_aws_directory_content = os.path.exists(self.aws_directory_path) and os.listdir(self.aws_directory_path)

        self.__project_template_aggregator = None
        self.__deployment_template_aggregator = None
        self.__deployment_access_template_aggregator = None

    @property
    def framework_aws_directory_path(self):
        # Assumes RESOURCE_MANAGER_PATH is
        # ...\Gems\CloudGemFramework\v?\ResourceManager\resource_manager.
        # We want ...\Gems\CloudGemFramework\v?\AWS.
        return os.path.abspath(os.path.join(RESOURCE_MANAGER_PATH, '..', '..', 'AWS'))

    @property
    def resource_manager_path(self):
        return RESOURCE_MANAGER_PATH

    @property
    def project_lambda_code_path(self):
        return os.path.join(self.aws_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME)


    @property
    def project_template_aggregator(self):
        if self.__project_template_aggregator is None:
            self.__project_template_aggregator = ProjectTemplateAggregator(self.__context)
        return self.__project_template_aggregator


    @property
    def deployment_template_aggregator(self):
        if self.__deployment_template_aggregator is None:
            self.__deployment_template_aggregator = DeploymentTemplateAggregator(self.__context)
        return self.__deployment_template_aggregator


    @property
    def deployment_access_template_aggregator(self):
        if self.__deployment_access_template_aggregator is None:
            self.__deployment_access_template_aggregator = DeploymentAccessTemplateAggregator(self.__context)
        return self.__deployment_access_template_aggregator


    @property
    def project_default_deployment(self):
        deployment = self.project_settings.get_project_default_deployment()
        if deployment and deployment not in self.deployment_names:
            self.context.view.invalid_project_default_deployment_clearing(deployment)
            self.set_project_default_deployment(None)
            return None
        return deployment

    @project_default_deployment.setter
    def project_default_deployment(self, deployment):
        self.project_settings.set_project_default_deployment(deployment)

    @property
    def user_default_deployment(self):
        deployment = self.user_settings.get('DefaultDeployment', None)
        if deployment and deployment not in self.deployment_names:
            self.context.view.invalid_user_default_deployment_clearing(deployment)
            self.set_user_default_deployment(None)
            return None
        return deployment
  

    @user_default_deployment.setter
    def user_default_deployment(self, deployment):
        self.user_settings['DefaultDeployment'] = deployment

    @property
    def project_region(self):
        return util.get_region_from_arn(self.project_stack_id)

    @property
    def default_deployment(self):
        return self.user_default_deployment or self.project_default_deployment


    @property
    def release_deployment(self):
       return self.project_settings.get_release_deployment()

    @release_deployment.setter
    def release_deployment(self, deployment):
        self.project_settings.set_release_deployment(deployment)


    @property
    def project_settings(self):
        if self.__project_settings is None:
            self.__initialize_project_settings()
        return self.__project_settings


    def __initialize_project_settings(self):

        if self.project_initialized:

            self.__project_resources = self.context.stack.describe_resources(self.project_stack_id, recursive=False)

            self.__configuration_bucket_name = self.__project_resources.get('Configuration', {}).get('PhysicalResourceId', None)
            if not self.__configuration_bucket_name:
                raise HandledError('The project stack {} is missing the required Configuration resource. Has {} been modified to remove this resource?'.format(self.project_stack_id,
                    self.project_template_aggregator.base_file_path))

            # If the project is initialized, we load our project file from S3
            self.__project_settings = CloudProjectSettings.factory(self.context, 
                self.configuration_bucket_name, 
                verbose = self.__verbose)

            # If a role was specified, start using it now. All AWS activity other than what is needed
            # to load the project settings and resources should happen after this point. 
            #
            # Note that the create-project-stack command does not support an --assume-role option and
            # it doesn't do anything after creating the project stack, so there is no need to start 
            # using the role after the project stack is created.
            self.assume_role()

        else:

            # If the project hasn't been initilized, then there are no settings in s3 yet so we just pretend we have an empty set(because we do)
            # Because the project hasn't been init'd, we don't have a config bucket to save to yet. The logic dealing with pending project Id
            # will update this to the actual cloud-based settings file
            self.__project_settings = UnitializedCloudProjectSettings(self.__context)
            self.__project_resources = {}

    def assume_role(self):
        if self.__assume_role_name:
            self.context.aws.assume_role(self.__assume_role_name, self.__assume_role_deployment_name)
            self.__assume_role_name = None

    @property
    def deployment_names(self):
        if self.__deployment_names is None:
            self.__deployment_names = self.__find_deployment_names()
        return self.__deployment_names


    @property
    def project_resources(self):
        if self.__project_resources is None:

            if not self.project_initialized:
                raise HandledError('The project has not been initialized.')

            # Initializing the project settings loads the project resources. We
            # want both to happen, and for us to assume a role if --assume-role
            # was specified, before any other AWS activity takes place.
            self.__initialize_project_settings()

        return self.__project_resources


    @property
    def project_resource_handler_id(self):

        project_resource_handler_name = self.project_resources.get('ProjectResourceHandler', {}).get('PhysicalResourceId', None)
        if project_resource_handler_name is None:
            raise HandledError('The project stack {} is missing the required ProjectResourceHandler resource. Has {} been modified to remove this resource?'.format(self.project_stack_id,
                self.project_template_aggregator.base_file_path))

        return 'arn:aws:lambda:{region}:{account_id}:function:{function_name}'.format(region=util.get_region_from_arn(self.project_stack_id),
            account_id=util.get_account_id_from_arn(self.project_stack_id),
            function_name=project_resource_handler_name)


    @property
    def token_exchange_handler_id(self):

        token_exchange_handler_name = self.project_resources.get('PlayerAccessTokenExchange', {}).get('PhysicalResourceId', None)
        if token_exchange_handler_name is None:
            raise HandledError('The project stack {} is missing the required PlayerAccessTokenExchange resource. Has {} been modified to remove this resource?'.format(self.project_stack_id,
                self.project_template_aggregator.base_file_path))

        return 'arn:aws:lambda:{region}:{account_id}:function:{function_name}'.format(region=util.get_region_from_arn(self.project_stack_id),
            account_id=util.get_account_id_from_arn(self.project_stack_id),
            function_name=token_exchange_handler_name)


    def __find_deployment_names(self):

        names = []

        for deployment_name in self.project_settings.get_deployments():
            if deployment_name != "*":
                names.append(deployment_name)

        return names


    def join_aws_directory_path(self, relative_path):
        '''Returns an absolute path when given a path relative to the {root}\{game}\AWS directory.'''
        return os.path.join(self.aws_directory_path, relative_path)


    def set_project_default_deployment(self, deployment, save=True):
        if deployment and deployment not in self.deployment_names:
            raise HandledError('There is no {} deployment'.format(deployment))
        self.project_default_deployment = deployment
        if deployment:
            self.project_settings.set_project_default_deployment(deployment)
        else:
            self.project_settings.remove_project_default_deployment()

        if save:
            self.save_project_settings()

    def set_release_deployment(self, deployment, save=True):
        if deployment and deployment not in self.deployment_names:
            raise HandledError('There is no {} deployment'.format(deployment))
        self.release_deployment = deployment
        if deployment:
            self.project_settings.set_release_deployment(deployment)
        else:
            self.project_settings.remove_release_deployment()
        if save:
            self.save_project_settings()


    def set_pending_project_stack_id(self, project_stack_id):
        self.local_project_settings[constant.PENDING_PROJECT_STACK_ID] = project_stack_id
        self.local_project_settings.save()

    def get_pending_project_stack_id(self):
        return self.local_project_settings.get(constant.PENDING_PROJECT_STACK_ID, None)

    def save_pending_project_stack_id(self):
        if self.local_project_settings.get(constant.PENDING_PROJECT_STACK_ID, None) is not None:
            project_stack_id = self.local_project_settings[constant.PENDING_PROJECT_STACK_ID]
            del self.local_project_settings[constant.PENDING_PROJECT_STACK_ID]
            self.local_project_settings[constant.PROJECT_STACK_ID] = project_stack_id           
            self.local_project_settings.save()

    def clear_project_stack_id(self):
        if constant.PROJECT_STACK_ID in self.local_project_settings:
            del self.local_project_settings[constant.PROJECT_STACK_ID]                      
        if constant.PENDING_PROJECT_STACK_ID in self.local_project_settings:
            del self.local_project_settings[constant.PENDING_PROJECT_STACK_ID] 
        self.local_project_settings.save()

    def save_project_settings(self):
        self.project_settings.save()
        self.force_gui_refresh()

    def set_user_default_deployment(self, deployment):
        if deployment and deployment not in self.deployment_names:
            raise HandledError('There is no {} deployment'.format(deployment))
        self.user_default_deployment = deployment
        if deployment:
            self.user_settings['DefaultDeployment'] = deployment
        else:
            self.user_settings.pop('DefaultDeployment', None) # safe delete
        self.__save_user_settings()


    def set_user_default_profile(self, profile):
        if profile:
            self.user_settings[constant.DEFAULT_PROFILE_NAME] = profile
        else:
            self.user_settings.pop(constant.DEFAULT_PROFILE_NAME, None) # safe delete
        self.context.aws.set_default_profile(profile)
        self.user_default_profile = profile
        self.__save_user_settings()

    def clear_user_default_profile(self):
        self.user_settings.pop(constant.DEFAULT_PROFILE_NAME, None) # safe delete
        self.context.aws.set_default_profile(None)
        self.user_default_profile = None
        self.__save_user_settings()

    def __save_user_settings(self):
        self.save_json(self.user_settings_path, self.user_settings)


    def initialize_aws_directory(self):
        '''Initializes the aws directory if it doesn't exist or is empty.'''

        if not os.path.isdir(self.aws_directory_path):
            self.context.view.creating_directory(self.aws_directory_path)
            os.makedirs(self.aws_directory_path)

        if not os.path.isfile(self.local_project_settings.path):
            self.local_project_settings.save()

        self.__load_aws_directory() # reload


    @property
    def __default_resource_group_content_directory_path(self):
        return os.path.join(RESOURCE_MANAGER_PATH, 'default-resource-group-content')

    def copy_default_resource_group_content(self, destination_path):
        file_util.copy_directory_content(self.context, destination_path, self.__default_resource_group_content_directory_path)

    def copy_default_lambda_code_content(self, destination):
        lambda_code_path = os.path.join(RESOURCE_MANAGER_PATH, "default-lambda-function-code")
        file_util.copy_directory_content(self.context, destination, lambda_code_path)

    @property
    def __example_resource_group_content_directory_path(self):
       return os.path.join(RESOURCE_MANAGER_PATH, 'hello-world-example-resource-group-content')

    def copy_example_resource_group_content(self, destination_path):
        file_util.copy_directory_content(self.context, destination_path, self.__example_resource_group_content_directory_path)

    def get_game_directory_name(self):
        '''Reads {root}\bootstrap.cfg to determine the name of the game directory.'''

        path = os.path.join(self.root_directory_path, "bootstrap.cfg")

        if not os.path.exists(path):
            self.context.view.missing_bootstrap_cfg_file(self.root_directory_path)
            return 'Game'
        else:

            self.context.view.loading_file(path)

            # If we add a section header and change the comment prefix, then
            # bootstrap.cfg looks like an ini file and we can use ConfigParser.
            ini_str = '[default]\n' + open(path, 'r').read()
            ini_str = ini_str.replace('\n--', '\n#')
            ini_fp = StringIO(ini_str)
            config = RawConfigParser()
            config.readfp(ini_fp)

            game_directory_name = config.get('default', 'sys_game_folder')

            game_directory_path = os.path.join(self.root_directory_path, game_directory_name)
            if not os.path.exists(game_directory_path):
                raise HandledError('The game directory identified by the bootstrap.cfg file does not exist: {}'.format(game_directory_path))
            game_config_file_path = os.path.join(game_directory_path, "game.cfg")
            if not os.path.exists(game_config_file_path):
                raise HandledError('The game directory identified by the bootstrap.cfg file does not contain a game.cfg file: {}'.format(game_directory_path))

            return game_directory_name

    def get_deployment_stack_id(self, deployment_name, optional=False):
        stack_id = self.get_pending_deployment_stack_id(deployment_name)
        if stack_id is None:
            stack_id = self.project_settings.get_deployment(deployment_name).get('DeploymentStackId', None)
            if stack_id is None and not optional:
                raise HandledError('Deployment stack {} does not exist.'.format(deployment_name))
        return stack_id

    def get_deployment_access_stack_id(self, deployment_name, optional=False):
        stack_id = self.project_settings.get_deployment(deployment_name).get('DeploymentAccessStackId', None)
        if stack_id is None and not optional:
            raise HandledError('Deployment access stack {} does not exist.'.format(deployment_name))
        return stack_id

    def get_resource_group_stack_id(self, deployment_name, resource_group_name, optional=False):
        deployment_stack_id = self.get_deployment_stack_id(deployment_name, optional=optional)
        return self.context.stack.get_physical_resource_id(deployment_stack_id, resource_group_name, optional=optional)

    def set_user_mappings(self, mappings):
        self.user_settings['Mappings'] = mappings
        self.__save_user_settings()

    def get_project_stack_name(self):
        return util.get_stack_name_from_arn(self.project_stack_id)

    def get_default_deployment_stack_name(self, deployment_name):
        return self.get_project_stack_name() + '-' + deployment_name

    def deployment_stack_exists(self, deployment_name):
        deployment = self.project_settings.get_deployment(deployment_name)
        return 'DeploymentStackId' in deployment

    def get_pending_deployment_stack_id(self, deployment_name):
        return self.project_settings.get_deployment(deployment_name).get('PendingDeploymentStackId', None)

    def get_pending_deployment_access_stack_id(self, deployment_name):
        return self.project_settings.get_deployment(deployment_name).get('PendingDeploymentAccessStackId', None)

    def creating_deployment(self, deployment_name):
        deployment_map = self.__ensure_map(self.project_settings, 'deployment')
        deployment_name_map = self.__ensure_map(deployment_map, deployment_name)
        self.save_project_settings()

    def set_pending_deployment_stack_id(self, deployment_name, deployment_stack_id):
        deployment_map = self.__ensure_map(self.project_settings, 'deployment')
        deployment_name_map = self.__ensure_map(deployment_map, deployment_name)
        deployment_name_map['PendingDeploymentStackId'] = deployment_stack_id
        if deployment_name not in self.deployment_names:
            self.deployment_names.append(deployment_name)
        self.save_project_settings()

    def set_pending_deployment_access_stack_id(self, deployment_name, deployment_access_stack_id):
        deployment_map = self.__ensure_map(self.project_settings, 'deployment')
        deployment_name_map = self.__ensure_map(deployment_map, deployment_name)
        deployment_name_map['PendingDeploymentAccessStackId'] = deployment_access_stack_id
        if(deployment_name not in self.deployment_names):
            self.deployment_names.append(deployment_name)
        self.save_project_settings()

    def finalize_deployment_stack_ids(self, deployment_name):
        deployment_map = self.__ensure_map(self.project_settings, 'deployment')
        deployment_name_map = self.__ensure_map(deployment_map, deployment_name)

        deployment_stack_id = deployment_name_map.pop('PendingDeploymentStackId', None)
        if deployment_stack_id is None:
            raise RuntimeError('There is no PendingDeploymentStackId property.')
        deployment_name_map['DeploymentStackId'] = deployment_stack_id

        deployment_access_stack_id = deployment_name_map.pop('PendingDeploymentAccessStackId', None)
        if deployment_access_stack_id is None:
            raise RuntimeError('There is no PendingDeploymentAccessStackId property.')
        deployment_name_map['DeploymentAccessStackId'] = deployment_access_stack_id
        
        self.save_project_settings()

    def remove_deployment(self, deployment_name):

        if deployment_name == self.project_default_deployment:
            self.set_project_default_deployment(None, save=False)

        if deployment_name == self.user_default_deployment:
            self.set_user_default_deployment(None)

        if deployment_name == self.release_deployment:
            self.set_release_deployment(None, save=False)

        self.project_settings.remove_deployment(deployment_name)
        self.save_project_settings()


    def protect_deployment(self, deployment_name):
        if not deployment_name:
            raise HandledError('No deployment spcified in protect_deployment')
        if deployment_name and deployment_name not in self.deployment_names:
            raise HandledError('There is no {} deployment'.format(deployment_name))

        deployment = self.project_settings.get_deployment(deployment_name)
        deployment['Protected'] = True
        self.save_project_settings()


    def unprotect_deployment(self, deployment_name):
        if not deployment_name:
            raise HandledError('No deployment spcified in unprotect_deployment')
        if deployment_name and deployment_name not in self.deployment_names:
            raise HandledError('There is no {} deployment'.format(deployment_name))

        deployment = self.project_settings.get_deployment(deployment_name)
        deployment.pop('Protected', None)
        self.save_project_settings()


    def get_protected_depolyment_names(self):
        deployments = self.project_settings.get_deployments()
        return [name for name in deployments if name != '*' and 'Protected' in deployments[name]]


    def __ensure_map(self, map, name):
        if not name in map:
            map[name] = {}
        return map[name]

    def init_project_settings(self):

        # This function is called after the project stack has been created with the boostrap template, 
        # but before it is updated with the full template. We need to initialize config to the point
        # that project.update_stack and update it. Basically this means making the configuration bucket
        # name available. 
        #
        # We also take this oppertunity to write the initial project-settings.json to the configuration
        # bucket.

        resources = self.context.stack.describe_resources(self.get_pending_project_stack_id(), recursive=False)
        self.__configuration_bucket_name = resources.get('Configuration', {}).get('PhysicalResourceId', None)
        if not self.__configuration_bucket_name:
            raise HandledError('The project stack was created but it contains no Configuration resource. Has the project-template.json been modified to remove this required resource?')

        settings_default_content = { "deployment": { "*": { "resource-group": {}} } }

        self.__project_settings = CloudProjectSettings.factory(self.context, 
            self.configuration_bucket_name, 
            initial_settings = settings_default_content, 
            verbose = self.__verbose)
        self.project_settings.save()


    @property
    def configuration_bucket_name(self):
        if self.__configuration_bucket_name is None:
            if not self.project_initialized:
                pending_project_stack_id = self.get_pending_project_stack_id()
                if pending_project_stack_id:
                    self.__configuration_bucket_name = self.context.stack.get_physical_resource_id(pending_project_stack_id, 'Configuration')
                else:
                    raise HandledError('The project has not been initialized.')
            self.__initialize_project_settings()
        return self.__configuration_bucket_name


    def verify_framework_version(self):        
        if self.__context.gem.framework_gem.version != self.local_project_settings.framework_version:
            raise HandledError('The project\'s AWS resources are from CloudGemFramework version {} but version {} of the CloudGemFramework gem is now enabled for the project. You must use the command "lmbr_aws project update-framework-version" to update to the framework version used by the project\'s AWS resources.'.format(self.local_project_settings.framework_version, self.__context.gem.framework_gem.version))

        
    @property
    def framework_version(self):
        if self.__framework_version is None:
            self.__framework_version = self.local_project_settings.framework_version
        return self.__framework_version


    def set_pending_framework_version(self, new_version):
        self.__framework_version = new_version

    def save_pending_framework_version(self):
        self.local_project_settings.set_framework_version(self.__framework_version)
        self.local_project_settings.save()


'''A dict-like class that persists a python dictionary to disk.'''
class LocalProjectSettings():

    def __init__(self, context, settings_path, region=None):
          
        self.__path = settings_path
        self.__context = context                
        self.__default = None        
        self.__framework_version = None
        
        self.__context.view.loading_file(settings_path)
        settings = util.load_json(settings_path, optional=True, default=None)

        self.__dict = dict(settings) if settings else None

        if self.__dict is None:
            #new local_project_settings file              
            self.__dict = dict({})         
            self.create_default_section()
            self.__framework_version = self.__context.gem.framework_gem.version
        elif self.default_set() is not None:
            self.default(self.default_set()[constant.SET])
        
        if self.default_set() and self.default_set().get(constant.DISABLED_RESOURCE_GROUPS_KEY, []) and self[constant.DISABLED_RESOURCE_GROUPS_KEY] is None:
            self[constant.DISABLED_RESOURCE_GROUPS_KEY] = self.default_set()[constant.DISABLED_RESOURCE_GROUPS_KEY]

        self.__context.view.loading_file(region)
        if region is not None and constant.DEFAULT.lower() in self.__dict:            
            is_lazy_migration = self.__dict[self.default_set()[constant.SET]].get(constant.LAZY_MIGRATION, False) if self.default_set() is not None and self.default_set()[constant.SET] in self.__dict else False            
            migration_set = self.__dict[self.default_set()[constant.SET]] if is_lazy_migration else {}
            self.default(region)
            if is_lazy_migration:
                self.migrate_to_default_set(migration_set)
                migration_set.pop(constant.LAZY_MIGRATION, None)
                self.save()

        if self.__framework_version is None:
            self.__framework_version = Version( self.__default[constant.FRAMEWORK_VERSION_KEY] if self.__default != None and constant.FRAMEWORK_VERSION_KEY in self.__default else self.__dict[constant.FRAMEWORK_VERSION_KEY] if constant.FRAMEWORK_VERSION_KEY in self.__dict else "1.0.0")                        
        
    @property
    def path(self):
        return self.__path

    @property
    def framework_version(self):
        return self.__framework_version
    
    def set_framework_version(self, value):
        if not isinstance(value, Version):
            value = Version(value)
        self.__framework_version = value
        self[constant.FRAMEWORK_VERSION_KEY] = str(value)

    def is_default_set(self):
        return self.__dict is not None and constant.DEFAULT.lower() in self.__dict

    def project_stack_exists(self):
        return constant.PROJECT_STACK_ID in self.__default or constant.PENDING_PROJECT_STACK_ID in self.__default

    def create_default_section(self):
        self.__default = self.__dict.setdefault(constant.DEFAULT.lower(), {})        
        self.__dict[constant.DEFAULT.lower()][constant.SET] = constant.DEFAULT.lower()   

    def raw_dict(self):
        return self.__dict

    def save(self):
        try:
            self.__context.view.saving_file(self.path)
            dir = os.path.dirname(self.path)
            if not os.path.exists(dir):
                os.makedirs(dir)
            with open(self.path, 'w') as file:
                return json.dump(self.__dict, file, indent=4)
        except Exception as e:
            raise HandledError('Could not save {}.'.format(self.path), e)
            
    def default(self, value):                
        self.__default = self.__dict.setdefault(value.lower(), {})        
        self.__dict[constant.DEFAULT.lower()][constant.SET] = str(value).lower()
        self.__default.setdefault(constant.FRAMEWORK_VERSION_KEY, str(self.__context.gem.framework_gem.version))         

    def default_set(self):        
        if constant.DEFAULT.lower() in self.__dict:            
            return self.__dict[constant.DEFAULT.lower()]

        return None

    def get(self, key, default=None):
        if self.__default is None:
            return default
        return self.__default.get(key, default)

    def is_default_set_to_region(self):
        return self.__dict[constant.DEFAULT] != self.__default

    def set(self, key, value):
        self.__default[key] = value

    def pop(self, key, default=None):
        self.__dict.pop(key, default)

    '''Dictionary overrides'''
    def __setitem__(self, key, value):
        self.set(key,value)

    def __getitem__(self, key):        
        return self.__default.get(key, None)

    def __delitem__(self, key):        
        del self.__default[key]

    def __len__(self):
        return len(self.__default)

    def __contains__(self, key):
        return key in self.__default

    def __iter__(self):
        for key in self.__default:
            yield key

    def setdefault(self, key, default=None):
        return self.__default.setdefault(key, default)

    def migrate_to_default_set(self, migration_set):        
        if constant.ENABLED_RESOURCE_GROUPS_KEY in migration_set:
            del migration_set[constant.ENABLED_RESOURCE_GROUPS_KEY]            
        
        if constant.DISABLED_RESOURCE_GROUPS_KEY in migration_set:
            self[constant.DISABLED_RESOURCE_GROUPS_KEY] = migration_set[constant.DISABLED_RESOURCE_GROUPS_KEY]
            migration_set.pop(constant.DISABLED_RESOURCE_GROUPS_KEY, None)            

        if constant.PROJECT_STACK_ID in migration_set:
            self[constant.PROJECT_STACK_ID] = migration_set[constant.PROJECT_STACK_ID]
            migration_set.pop(constant.PROJECT_STACK_ID, None)            

        if constant.PENDING_PROJECT_STACK_ID in migration_set:
            self[constant.PENDING_PROJECT_STACK_ID] = migration_set[constant.PENDING_PROJECT_STACK_ID]
            migration_set.pop(constant.PENDING_PROJECT_STACK_ID, None)            

'''
Base class for the cloud project settings.
Project settings are stored in the project configuration bucket at the root.
There are two types of serializations.

V1 (legacy before 1.16)
V2 

'''
class CloudProjectSettings(dict):

    ATTR_RELEASE_DEPLOYMENT_NAME = "ReleaseDeployment"
    ATTR_DEFAULT_DEPLOYMENT_NAME = "DefaultDeployment"
    ATTR_DEPLOYMENT = "deployment"
    DEFAULT = "default"

    def __init__(self, context, bucket, initial_settings, verbose):          
        if not initial_settings:
            initial_settings = self.load(context, bucket, initial_settings, verbose)                
        context.view.loaded_project_settings(initial_settings)            
        super(CloudProjectSettings, self).__init__(initial_settings)          
        self.__bucket = bucket        
        self.__context = context      

    @property
    def bucket(self):
        return self.__bucket

    @property
    def context(self):
        return self.__context

    @staticmethod
    def factory(context, bucket, initial_settings=None, verbose=None):  
        if not initial_settings:            
            try:
                return CloudProjectSettings_V1(context, bucket, initial_settings, verbose)
            except Exception as e:                  
                return CloudProjectSettings_V2(context, bucket, initial_settings, verbose)
        return CloudProjectSettings_V2(context, bucket, initial_settings, verbose)

'''
Version two of the cloud project settings stores 
each deployment in a seperate json file with the suffix 'dstack'.
This was done to remove the bottleneck deployments had when performing actions 
as they would require mutually exclusion locks on the project stack.

Example:
dstack.ReleaseDeployment.default.json
    Contents:   TestDeployment1

dstack.DefaultDeployment.default.json
    Contents:   TestDeployment1

dstack.deployment.TestDeployment1.json
    Contents:   {
                    "DeploymentAccessStackId": "arn:aws:cloudformation:us-east-1:XXXXXXXX:stack/cctestWUQW4NE-hyptest-cctest0DZ2Y8A-Access/9f5585f0-767a-11e8-a9bc-500abe22848d", 
                    "DeploymentStackId": "arn:aws:cloudformation:us-east-1:XXXXXXXX:stack/cctestWUQW4NE-hyptest-cctest0DZ2Y8A/7557dbe0-767a-11e8-a1f5-503aca4a58d1"
                }
'''
class CloudProjectSettings_V2(CloudProjectSettings):   

    def __init__(self, context, bucket, initial_settings, verbose):          
        super(CloudProjectSettings_V2, self).__init__(context, bucket, initial_settings, verbose)   
        self.__initial_state_of_deployments = None

    '''
    Suffix for the files
    '''
    @staticmethod
    def file_name():
        return "dstack"

    '''
    S3 key name to use during the loading
    '''
    @property
    def key(self):
        return CloudProjectSettings_V2.file_name()

    '''
    Get the default project deployment stack name.  
    User defaults have high priorities than the project default.
    '''
    def get_project_default_deployment(self):
        return self.get(self.ATTR_DEFAULT_DEPLOYMENT_NAME, None)

    '''
    Set the default project deployment.
    Requires the deletion of the s3 file associated to the previous default.
    '''
    def set_project_default_deployment(self, new_default):
        self.remove_project_default_deployment()
        self[self.ATTR_DEFAULT_DEPLOYMENT_NAME] = new_default

    '''
    Remove the project default deployment stack.
    '''
    def remove_project_default_deployment(self):
        if self.ATTR_DEFAULT_DEPLOYMENT_NAME in self:
            self.delete(self.ATTR_DEFAULT_DEPLOYMENT_NAME, self.DEFAULT)
            self.pop(self.ATTR_DEFAULT_DEPLOYMENT_NAME, None)            

    '''
    Get the release deployment stack name
    '''
    def get_release_deployment(self):
        return self.get(self.ATTR_RELEASE_DEPLOYMENT_NAME, None)

    '''
    Set the release deployment stack name
    Requires the deletion of the s3 file associated to the previous default.
    '''
    def set_release_deployment(self, new_release_deployment):
        self.remove_release_deployment()
        self[self.ATTR_RELEASE_DEPLOYMENT_NAME] = new_release_deployment

    '''
    Remove the project default deployment stack.
    '''
    def remove_release_deployment(self):
        if self.ATTR_RELEASE_DEPLOYMENT_NAME in self:
            self.delete(self.ATTR_RELEASE_DEPLOYMENT_NAME, self.DEFAULT)
            self.pop(self.ATTR_RELEASE_DEPLOYMENT_NAME, None)        

    '''
    Get a list of all of the deployments
    '''
    def get_deployments(self):
        deployments = self.get(self.ATTR_DEPLOYMENT, {})        
        return deployments

    '''
    Get information on a specific deployment
    '''
    def get_deployment(self, deployment_name):        
        return self.get_deployments().get(deployment_name, {})

    '''
    Remove a specific deployment and any associated default files.
    '''
    def remove_deployment(self, deployment_name):        
        self.delete(self.ATTR_DEPLOYMENT, deployment_name)
        if self.get_release_deployment() == deployment_name:
            self.remove_release_deployment()
        if self.get_project_default_deployment() == deployment_name:
            self.remove_project_default_deployment()
        self.get_deployments().pop(deployment_name, None)        

    '''
    Get a resource group from within a deployment
    '''
    def get_resource_group_settings(self, deployment_name):        
        return self.get_deployment(deployment_name).get("resource-group")

    '''
    Get the default resource group
    '''
    def get_default_resource_group_settings(self):
        return self.get_resource_group_settings("*")

    '''
    Convert the individual files in the project configuration bucket to a dictionary
    Each individual file and contents becomes a key and value in the dictionary
    '''
    def load(self, context, bucket, initial_settings, verbose):
        try:            
            s3 = context.aws.client('s3')
            res = s3.list_objects_v2(Bucket=bucket, Prefix=CloudProjectSettings_V2.file_name())
            cloud_deployments = res.get('Contents', {})
            initial_settings = {
                self.ATTR_DEPLOYMENT:{}
            }
            entries = initial_settings[self.ATTR_DEPLOYMENT]
            for deployment in cloud_deployments:
                file_name = deployment['Key']
                prefix, group, deployment_name, ext = file_name.split('.')                
                if group == self.ATTR_DEFAULT_DEPLOYMENT_NAME or group == self.ATTR_RELEASE_DEPLOYMENT_NAME:                    
                    initial_settings[group] = s3.get_object(Bucket=bucket, Key=file_name)["Body"].read()
                else:                    
                    contents = json.load(s3.get_object(Bucket=bucket, Key=file_name)["Body"]) 
                    self.__initial_state_of_deployments = entries[deployment_name] = contents                   
            return initial_settings
        except Exception as e:
            raise HandledError('Cloud not read V2 project settings from bucket {} object {}: {}.'.format(bucket,
                CloudProjectSettings_V2.file_name(),
                e.message))

    '''
    Convert the dictionary to individual files in the project configuration bucket.
    Each key of the dictionary becomes a individual file.
    Only save what changed locally! Or you could overwrite another process deployment changes.
    '''
    def save(self):
        s3 = self.context.aws.client('s3')                
        for key1 in self.keys():            
            if isinstance(self[key1], dict):                
                for key2 in self[key1].keys():                    
                    file_name = "{}.{}.{}.json".format(self.key, key1, key2)
                    value = self[key1][key2]
                    if self.__initial_state_of_deployments != None and key2 in self.__initial_state_of_deployments and value == self.__initial_state_of_deployments[key2]:
                        continue
                    s3.put_object(Bucket=self.bucket, Key=file_name, Body=json.dumps(value, indent=4, sort_keys=True))        
            else:
                file_name = "{}.{}.default.json".format(self.key, key1)
                value = self[key1]                            
                s3.put_object(Bucket=self.bucket, Key=file_name, Body=value)                
        print "Saving the project configuration to {}".format(self.bucket)
        self.context.view.saved_project_settings(self)        
        
    def delete(self, group_name, deployment):        
        s3 = self.context.aws.client('s3')
        file_name = "{}.{}.{}.json".format(self.key, group_name, deployment)        
        res = s3.delete_object(Bucket=self.bucket, Key=file_name)        
        
'''
Client abstraction for project settings 
The original format for storing project information.

Example:
{
    "DefaultDeployment": "TestDeployment1", 
    "ReleaseDeployment": "TestDeployment1", 
    "deployment": {
        "*": {
            "resource-group": {}
        }, 
        "TestDeployment1": {
            "DeploymentAccessStackId": "arn:aws:cloudformation:us-east-1:XXXXXXXX:stack/cctestLL53V6J-hyptest-TestDeployment1-Access/d25fb420-6f59-11e8-9d4b-500c524294f2", 
            "DeploymentStackId": "arn:aws:cloudformation:us-east-1:XXXXXXXX:stack/cctestLL53V6J-hyptest-TestDeployment1/5d61c800-6f56-11e8-a36c-500c28680ac6"
        }      
    }
}

'''
class CloudProjectSettings_V1(CloudProjectSettings):

    def __init__(self, context, bucket, initial_settings, verbose):
        super(CloudProjectSettings_V1, self).__init__(context, bucket, initial_settings, verbose)    

    '''
    File name of the s3 file.
    '''
    @staticmethod
    def file_name():
        return constant.PROJECT_SETTINGS_FILENAME

    '''
    S3 key name to use during the loading
    '''
    @property
    def key(self):
        return CloudProjectSettings_V1.file_name()

    def get_project_default_deployment(self):
        return self.get(self.ATTR_DEFAULT_DEPLOYMENT_NAME, None)

    def set_project_default_deployment(self, new_default):
        self[self.ATTR_DEFAULT_DEPLOYMENT_NAME] = new_default

    def remove_project_default_deployment(self):
        self.pop(self.ATTR_DEFAULT_DEPLOYMENT_NAME, None)

    def get_release_deployment(self):
        return self.get(self.ATTR_RELEASE_DEPLOYMENT_NAME, None)

    def set_release_deployment(self, new_release_deployment):
        self[self.ATTR_RELEASE_DEPLOYMENT_NAME] = new_release_deployment

    def remove_release_deployment(self):
        self.pop(self.ATTR_RELEASE_DEPLOYMENT_NAME, None)

    def get_deployments(self):
        return self.get(self.ATTR_DEPLOYMENT, {})

    def get_deployment(self, deployment_name):
        return self.get_deployments().get(deployment_name, {})

    def remove_deployment(self, deployment_name):
        self.get_deployments().pop(deployment_name, None)

    def get_resource_group_settings(self, deployment_name):
        return self.get_deployment(deployment_name).get("resource-group")

    def get_default_resource_group_settings(self):
        return self.get_resource_group_settings("*")
    
    def load(self, context, bucket, initial_settings, verbose):
        try:            
            s3 = context.aws.client('s3')                
            res = s3.get_object(Bucket=bucket, Key=CloudProjectSettings_V1.file_name())             
            settings = res['Body'].read()                
            initial_settings = json.loads(settings)                        
            return initial_settings
        except Exception as e:                           
            raise HandledError('Cloud not read legacy V1 project settings from bucket {} object {}: {}.'.format(bucket,
                CloudProjectSettings_V1.file_name(),
                e.message))        

    def save(self):
        s3 = self.context.aws.client('s3')
        s3.put_object(Bucket=self.bucket, Key=self.key, Body=json.dumps(self, indent=4, sort_keys=True))
        self.context.view.saved_project_settings(self)

'''This class is used when no project has yet been defined.
We just want our cloud-based interface that will return nothing but None.
Once the project is setup, init_project_settings will create the inital settings in the cloud'''
class UnitializedCloudProjectSettings(CloudProjectSettings_V2):

    def __init__(self, context):
        self.__context = context

    @property
    def framework_version(self):
        return self.__context.gem.framework_gem.version

    def save(self):
        pass

class TemplateAggregator(object):
    '''Encapsulates a Cloud Formation Stack template composed of a base template file in the resource 
    manager's templates directory and an optional extension template file in the project's AWS directory.'''

    
    DEFAULT_EXTENSION_TEMPLATE = {
        "AWSTemplateFormatVersion": "2010-09-09",
        "Parameters": {},
        "Resources": {},
        "Outputs": {}
    }


    def __init__(self, context, base_file_name, extension_file_name, gem_file_name=None, base_file_path=None, extension_file_path=None):
        '''Initializes the object with the specified file names but does not load any content from those files.

        Arguments:

           context: A Context object.

           base_file_name: The name of the file in the resource manager templates directory.

           extension_file_name: The name of the file in the project's AWS directory.

           gem_file_name (optional): The name of the file in each enabled gem AWS's directory.
           This is used to include the content of gem's AWS\project-template.json files in the
           effective template. That feature, and this parameter, was deprecated in Lumberyard
           1.10.
        '''

        self.__context = context
        self.__base_file_name = base_file_name
        self.__extension_file_name = extension_file_name
        self.__gem_file_name = gem_file_name
        self.__base_template = None
        self.__extension_template = None
        self.__effective_template = None
        self.__base_file_path = base_file_path or os.path.join(RESOURCE_MANAGER_PATH, 'templates')
        self.__extension_file_path = extension_file_path or context.config.aws_directory_path


    @property
    def context(self):
        return self.__context


    @property
    def base_file_path(self):
        return os.path.join(self.__base_file_path, self.__base_file_name)


    @property
    def base_template(self):
        if self.__base_template is None:
            path = self.base_file_path
            self.__context.view.loading_file(path)
            self.__context.config.load_json(path)
            self.__base_template = util.load_json(path, optional = False)
        return self.__base_template


    @property
    def extension_file_path(self):
        return os.path.join(self.__extension_file_path, self.__extension_file_name)


    @property
    def extension_template(self):
        if self.__extension_template is None:
            path = self.extension_file_path
            self.__context.view.loading_file(path)
            self.__extension_template = util.load_json(path, optional = True)
            if self.__extension_template is None:
                self.__extension_template = copy.deepcopy(self.DEFAULT_EXTENSION_TEMPLATE)
        return self.__extension_template


    def save_extension_template(self):
        path = self.extension_file_path
        self.__context.view.saving_file(path)
        util.save_json(path, self.extension_template)
        self.__effective_template = None # force recompute


    def _get_attribution(self, path):
        if os.path.splitdrive(path)[0] == os.path.splitdrive(self.context.config.root_directory_path):
            return os.path.relpath(path, self.context.config.root_directory_path)
        else:
            return path

    @property
    def effective_template(self):
        if self.__effective_template is None:
            self.__effective_template = copy.deepcopy(self.base_template)
            self._add_effective_content(self.__effective_template, self._get_attribution(self.extension_file_path))
            self.__update_access_control_dependencies(self.__effective_template)
            self.__add_framework_version_parameter(self.__effective_template)
        return self.__effective_template


    def _add_effective_content(self, effective_template, attribution):
        self._copy_parameters(effective_template, self.extension_template, self.extension_file_path)
        self._copy_resources(effective_template, self.extension_template, self.extension_file_path, attribution)
        self._copy_outputs(effective_template, self.extension_template, self.extension_file_path)


    def _copy_parameters(self, target, source, source_path):
        parameters = target.setdefault('Parameters', {})
        for name, definition in source.get('Parameters', {}).iteritems():
            if name in parameters:
                if self.__apply_merge_function(definition, parameters[name], source_path):
                    raise HandledError('Parameter {} cannot be overridden by extension template {}.'.format(name, source_path))
            else:
                if 'Default' not in definition:
                    raise HandledError('Parameter {} has no default value in extension template {}.'.format(name, source_path))
                parameters[name] = definition


    def _copy_resources(self, target, source, source_path, attribution):
        resources = target.setdefault('Resources', {})
        for name, definition in source.get('Resources', {}).iteritems():
            if name in resources:
                if not self.__apply_merge_function(definition, resources[name], source_path):
                    raise HandledError('Resource {} cannot be overridden by extension template {}.'.format(name, source_path))
            else:
                definition = copy.deepcopy(definition)
                definition.setdefault('Metadata', {}).setdefault('CloudGemFramework', {})['Source'] = attribution
                resources[name] = definition


    def _copy_outputs(self, target, source, source_path):
        outputs = target.setdefault('Outputs', {})
        for name, definition in source.get('Outputs', {}).iteritems():
            if name in outputs:
                if not self.__apply_merge_function(definition, outputs[name], source_path):
                    raise HandledError('Output {} cannot be overridden by extension template {}.'.format(name, source_path))
            else:
                outputs[name] = definition


    def __apply_merge_function(self, fn, target, source_path):
        '''Determines if a value is a merge function definition and, if so, applies 
        it to a target value.

        Arguments:

            fn - A dict with a single keyt that starts with "MergeFn::".

            target - a value to which the function will be applied.

            source_path - path to the file that defines the function.

        Returns:

            True if a function was provided and target successfully updated.

            False if value is not a merge function definition.

        Raises:
       
            HandledError if the merge function definition is invalid or 
            could not be applied to target.
        '''

        if not isinstance(fn, dict) or not fn:
            return False

        for fn_name, fn_args in fn.iteritems():

            if not fn_name.startswith('MergeFn'):
                return False

            handler = self.MERGE_FUNCTIONS.get(fn_name, None)
            if handler is None:
                raise HandledError('Found unknown MergeFn {} in {}.'.format(fn_name, source_path))

            handler(self, fn_args, target, source_path)

        return True


    def __merge_fn_append_to_array(self, args, target, source_path):
        '''Appends elements to a list.

        Arguments:

          args - a dict where keys identify a list typed property to which the corresponding
          value will be appended. The keys should be strings consisting of dot seperated 
          property names.

          target - a dict which will be recursively navigated using the property names from 
          args.

        '''

        for path, value in args.iteritems():
            path_target = self.__get_merge_fn_path_list(path, target, source_path)
            path_target.append(value)


    def __merge_fn_set_property(self, args, target, source_path):
        '''Sets property values on a target.

        Arguments:

          args - a dict where keys identify a property that will be set to the corresponding value.
          The keys should be strings consisting of dot seperated property names.

          target - a dict which will be recursively navigated using the property names from args.

          source_path - The path to the file that defines the function.
        '''

        for path, new_value in args.iteritems():
            lvalue = self.__get_merge_fn_path_lvalue(path, target, source_path)
            if not isinstance(lvalue.container, dict):
                raise HandledError('MergeFn::SetProperty path {} did not identify an object property when applied to {} as requsted by {}.'.format(path, target, source_path))
            lvalue.container[lvalue.key] = new_value


    MERGE_FUNCTIONS = {
        'MergeFn::AppendToArray': __merge_fn_append_to_array,
        'MergeFn::SetProperty': __merge_fn_set_property
    }


    # Represents an assignable value (a "Left Value"), such as property in a dict or a 
    # element in a list. The container field identifies the container that holds the 
    # value and the key field identifies the key that can be provided to that container 
    # when setting a value. The value field will be the actual value.
    __LValue = collections.namedtuple('LValue', ['container', 'key', 'value'])


    def __get_merge_fn_path_list(self, path, target, source_path):
        '''Returns the list object identified by a path when applied to a target.

        Arguments:

            path - a string consisting of dot seperated property names.

            target - A dict to which the path is applied.

            source_path - the path to the file that contains the path being applied.

        Returns:

            The list object identified by the path.

        Raises:

            HandledError if the path did not identify a list object.

        '''
        path_target = self.__get_merge_fn_path_value(path, target, source_path)
        if not isinstance(path_target, list):
            raise HandledError('MergeFn::AppendToArray path {} did not identify an array when applied to {} as requsted by {}.'.format(path, target, source_path))
        return path_target


    def __get_merge_fn_path_value(self, path, target, source_path):
        '''Returns the value identified by a path when applied to a target.

        Arguments:

            path - a string consisting of dot seperated property names.

            target - A dict to which the path is applied.

            source_path - the path to the file that contains the path being applied.

        Returns:

            The value identified by the path.

        Raises:

            HandledError if the path did not identify a value.

        '''
        lvalue = self.__get_merge_fn_path_lvalue(path, target, source_path)
        if lvalue.value is None:
            raise HandledError('The property identified by the MergeFn path {} was not found in {} as specified by {}.'.format(path, target, source_path))
        return lvalue.value


    def __get_merge_fn_path_lvalue(self, path, target, source_path):
        '''Returns the lvalue identified by a path when applied to a target.

        Arguments:

            path - a string consisting of dot seperated property names.

            target - A dict to which the path is applied.

            source_path - the path to the file that contains the path being applied.

        Returns:

            An __LValue instance intialized as follows:

                container - the dict that contains the property at the end of the path

                key - the name of the property at the end of the path

                value - the key's value in the container, None if the contained doesn't contain the key

        Raises:

            HandledError if the path did not identify a lvalue.

        '''

        # TODO: this could be replaced with a json path libary to enable
        # a lot more functionality without breaking exising uses.

        previous_target = None
        current_target = target

        property_names = path.split('.')
        for property_name in property_names[:-1]:
            if isinstance(current_target, dict):
                previous_target = current_target
                current_target = current_target.get(property_name, None)
                if current_target is not None:
                    continue
            raise HandledError('MergeFn path {} could not be applied to {} as requested by {}. The property {} does not exist in {}.'.format(path, target, source_path, property_name, previous_target))

        last_property_name = property_names[-1]
        result = self.__LValue(current_target, last_property_name, current_target.get(last_property_name, None))

        return result



    def __update_access_control_dependencies(self, target):

        # The AccessControl resource, if there is one, depends on all resources that
        # don't directly depend on it.

        access_control_definition = target.get('Resources', {}).get('AccessControl')
        if access_control_definition:

            access_control_depends_on = []

            for name, definition in target.get('Resources', {}).iteritems():

                if name == 'AccessControl':
                    continue

                resource_depends_on_access_control = False

                resource_depends_on = definition.get('DependsOn', None)
                if resource_depends_on:
                    if isinstance(resource_depends_on, list):
                        for entry in resource_depends_on:
                            if entry == 'AccessControl':
                                resource_depends_on_access_control = True
                                break
                    elif resource_depends_on == 'AccessControl':
                        resource_depends_on_access_control = True
                
                if not resource_depends_on_access_control:
                    access_control_depends_on.append(name)

            access_control_definition['DependsOn'] = access_control_depends_on


    def __add_framework_version_parameter(self, target):
        parameters = target.setdefault('Parameters', {})
        parameters['FrameworkVersion'] = {
            "Type": "String",
            "Description": "Identifies the version of the Cloud Gem Framework used to manage this stack.",
            "Default": str(self.__context.config.framework_version)
        }


class ProjectTemplateAggregator(TemplateAggregator):
    
    def __init__(self, context):
        super(ProjectTemplateAggregator, self).__init__(context, constant.PROJECT_TEMPLATE_FILENAME, constant.PROJECT_TEMPLATE_EXTENSIONS_FILENAME)

    def _add_effective_content(self, effective_template, attribution):

        super(ProjectTemplateAggregator, self)._add_effective_content(effective_template, attribution)

        # TODO: The use of poject-template.json files in gems was deprecated in Lumberyard 1.10
        for gem in self.context.gem.enabled_gems:
            path = os.path.join(gem.aws_directory_path, constant.PROJECT_TEMPLATE_FILENAME)
            if os.path.isfile(path):
                template = util.load_json(path)
                self._copy_parameters(effective_template, template, path)
                self._copy_resources(effective_template, template, path, gem.name)
                self._copy_outputs(effective_template, template, path)
        # End deprecated code

class DeploymentTemplateAggregator(TemplateAggregator):
    
    def __init__(self, context):
        super(DeploymentTemplateAggregator, self).__init__(context, constant.DEPLOYMENT_TEMPLATE_FILENAME, constant.DEPLOYMENT_TEMPLATE_EXTENSIONS_FILENAME)

    def _add_effective_content(self, effective_template, attribution):

        super(DeploymentTemplateAggregator, self)._add_effective_content(effective_template, attribution)

        resources = effective_template.setdefault('Resources', {})
        parameters = effective_template['Parameters']
        config_key_param = parameters['ConfigurationKey']

        enabled_resource_group_names = [r.name for r in self.context.resource_groups.values() if r.is_enabled]
        inter_gem_deps_map = {}
        resolver_dependencies = set()
        number_of_resources = len(self.context.resource_groups.values())
        for resource_group in self.context.resource_groups.values():

            if not resource_group.is_enabled:
                continue
            cross_gem_comms_dependencies = []
            for dep in resource_group.get_inter_gem_dependencies():
                if dep["gem"] in enabled_resource_group_names or dep["gem"] == "CloudGemFramework":
                    cross_gem_comms_dependencies.append(dep)

            configuration_name = resource_group.name + 'Configuration'
            configuration_key_name = resource_group.name + 'ConfigurationKey'

            resources[configuration_name] = {
                "Type": "Custom::ResourceGroupConfiguration",
                "Properties": {
                    "ServiceToken": { "Ref": "ProjectResourceHandler" },
                    "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
                    "ConfigurationKey": { "Ref": configuration_key_name },
                    "ResourceGroupName": resource_group.name
                }
            }

            resources[resource_group.name] = {
                "Type": "AWS::CloudFormation::Stack",
                "Properties": {
                    "TemplateURL": { "Fn::GetAtt": [configuration_name, "TemplateURL"] },
                        "Tags": [
                        {
                            "Key": "Gem",
                            "Value": resource_group.name
                        },
                        {
                            "Key": "Deployment",
                            "Value": {"Ref": "DeploymentName"}
                        }
                    ],
                    "Parameters": {
                        "ProjectResourceHandler": { "Ref": "ProjectResourceHandler" },
                        "ConfigurationBucket": { "Fn::GetAtt": [configuration_name, "ConfigurationBucket"] },
                        "ConfigurationKey": { "Fn::GetAtt": [configuration_name, "ConfigurationKey"] },
                        "DeploymentStackArn": { "Ref": "AWS::StackId" },
                        "DeploymentName": { "Ref": "DeploymentName" },
                        "ResourceGroupName": resource_group.name
                    },
                    "TimeoutInMinutes": 10+(5*number_of_resources)
                }
            }

            parameters[configuration_key_name] = config_key_param

            if cross_gem_comms_dependencies:
                inter_gem_deps_map[resource_group.name] = cross_gem_comms_dependencies
                resolver_dependencies.add(resource_group.name)
                for dep in cross_gem_comms_dependencies:
                    if dep["gem"] != "CloudGemFramework":
                        resolver_dependencies.add(dep["gem"])

        if inter_gem_deps_map:
            resources["CrossGemCommunicationInterfaceResolver"] = {
                "Type": "Custom::InterfaceDependencyResolver",
                "Properties": {
                    "UpdateTime": int(round(time.time())), # We need this to force an update
                    "ServiceToken": { "Ref": "ProjectResourceHandler" },
                    "InterfaceDependencies": inter_gem_deps_map
                },
                "DependsOn": list(resolver_dependencies)
            }
        # CloudFormation doesn't like an empty Resources list.
        if len(resources) == 0:
            resources['EmptyDeployment'] = {
                "Type": "Custom::EmptyDeployment",
                "Properties": {
                    "ServiceToken": { "Ref": "ProjectResourceHandler" }
                }
            }


class DeploymentAccessTemplateAggregator(TemplateAggregator):
    
    def __init__(self, context):
        super(DeploymentAccessTemplateAggregator, self).__init__(context, constant.DEPLOYMENT_ACCESS_TEMPLATE_FILENAME, constant.DEPLOYMENT_ACCESS_TEMPLATE_EXTENSIONS_FILENAME)

    def _add_effective_content(self, effective_template, attribution):
        super(DeploymentAccessTemplateAggregator, self)._add_effective_content(effective_template, attribution)

        for gem in self.context.gem.enabled_gems:
            path = os.path.join(gem.aws_directory_path, constant.DEPLOYMENT_ACCESS_TEMPLATE_FILENAME)
            if os.path.isfile(path):
                template = util.load_json(path)
                self._copy_parameters(effective_template, template, path)
                self._copy_resources(effective_template, template, path, gem.name)
                self._copy_outputs(effective_template, template, path)

        for resource_group_name in [r.name for r in self.context.resource_groups.values() if r.is_enabled]:
            effective_template["Resources"][resource_group_name + "AccessControl"] = {
                "Type": "Custom::AccessControl",
                "Properties": {
                    "ServiceToken": {"Ref": "ProjectResourceHandler"},
                    "ConfigurationBucket": {"Ref": "ConfigurationBucket"},
                    "ConfigurationKey": {"Ref": "ConfigurationKey"},
                    "Gem": resource_group_name
                },
                "DependsOn": ["Player", "Server", "DeploymentOwner", "DeploymentAdmin", "AuthenticatedPlayer"]
            }


class ResourceTemplateAggregator(TemplateAggregator):
    def __init__(self, context, base_file_path, extension_file_path):
        super(ResourceTemplateAggregator, self).__init__(context, constant.RESOURCE_GROUP_TEMPLATE_FILENAME,
                                                                 constant.RESOURCE_GROUP_TEMPLATE_EXTENSIONS_FILENAME, None, base_file_path, extension_file_path)
