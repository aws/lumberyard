#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #17 $

import os
import json
import shutil
import unittest
from copy import deepcopy
from time import sleep

from botocore.exceptions import ClientError

import resource_manager_common.constant

import lmbr_aws_test_support
import mock_specification

class IntegrationTest_CloudGemFramework_ResouceManager_ResourceHandlerPlugins(lmbr_aws_test_support.lmbr_aws_TestCase):


    def setUp(self):        
        self.prepare_test_envionment(type(self).__name__)
        

    def test_ressource_handler_plugins_end_to_end(self):
        self.run_all_tests()


    def __120_create_project_stack(self):
        self.lmbr_aws(
            'project', 'create', 
            '--stack-name', self.TEST_PROJECT_STACK_NAME, 
            '--confirm-aws-usage',
            '--confirm-security-change', 
            '--region', lmbr_aws_test_support.REGION
        )


    def __180_create_test_gem(self):     
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME,
            '--initial-content', 'no-resources',
            '--enable')


    def __440_add_custom_resource_handler_in_resource_group(self):
        self.add_custom_resource_to_gem()
        self.add_custom_resource_handler_to_gem()


    def __450_update_project_to_add_custom_resource_handler(self):
        self.lmbr_aws(
            'project', 'upload-resources', 
            '--confirm-aws-usage', 
            '--confirm-security-change'
        )


    def __455_create_deployment_stack(self):
        self.lmbr_aws(
            'deployment', 'create', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change'
        )            


    def __460_verify_custom_resource_handler_in_resource_group(self):
        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'CREATE_COMPLETE',
                'StackResources': {
                    'Custom': {
                        'ResourceType': 'Custom::Test'
                    }
                }
            })


    def __920_delete_deployment_stack(self):
        self.lmbr_aws(
            'deployment', 'delete',
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-resource-deletion'
        )


    def __950_delete_project_stack(self):
        self.lmbr_aws(
            'project', 'delete', 
            '--confirm-resource-deletion'
        )


    CUSTOM_RESOURCE_HANDLER = '''
from cgf_utils import custom_resource_response

def handler(event, context):
    data = { 'TestProperty': 'TestValue' }
    physical_resource_id = 'TestPyhsicalResourceId'
    return custom_resource_response.success_response(data, physical_resource_id)
'''

    CUSTOM_RESOURCE_NAME = 'Test'

    LAMBDA_IMPORTS = '''CloudGemFramework.LambdaService
    CloudGemFramework.ResourceManagerCommon
    CloudGemFramework.Utils'''


    def add_custom_resource_handler_to_gem(self):

        resource_group_custom_resource_file_path = os.path.join(
            self.resource_group_resource_types_code_path, "Custom_" + self.CUSTOM_RESOURCE_NAME + ".py")

        os.makedirs(self.resource_group_resource_types_code_path)

        with open(resource_group_custom_resource_file_path, 'w') as f:
            f.write(self.CUSTOM_RESOURCE_HANDLER)

        with open(os.path.join(self.resource_group_resource_types_code_path, "__init__.py"), "w") as f:
            pass

        with open(os.path.join(self.resource_group_resource_types_code_path, "..", ".import"), "w") as f:
            f.write(self.LAMBDA_IMPORTS)


    def add_custom_resource_to_gem(self):
        project_template_path = self.get_gem_aws_path(
            self.TEST_RESOURCE_GROUP_NAME, 'project-template.json')
        if not os.path.exists(project_template_path):
            with open(project_template_path, 'w') as f:
                f.write('{}')

        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, 'project-template.json') as gem_project_template:

            project_extension_resources = gem_project_template['Resources'] = {}
            project_extension_resources['testCustomResourceConfiguration'] = {
                "Properties": {
                    "ConfigurationBucket": {
                        "Ref": "Configuration"
                    },
                    "ConfigurationKey": {
                        "Ref": "ConfigurationKey"
                    },
                    "FunctionName": "testCustomResource",
                    "Runtime": "python2.7",
                    "ServiceToken": {
                        "Fn::GetAtt": [
                            "ProjectResourceHandler",
                            "Arn"
                        ]
                    },
                    "Settings": {}
                },
                "Type": "Custom::LambdaConfiguration"
            }

            project_extension_resources['CustomType'] = {
                "Type": "Custom::ResourceTypes",
                "Properties": {
                    "ServiceToken": {
                        "Fn::GetAtt": [
                            "ProjectResourceHandler",
                            "Arn"
                        ]
                    },
                    "LambdaConfiguration": { "Fn::GetAtt": [ "testCustomResourceConfiguration", "ComposedLambdaConfiguration" ] },
                    "Definitions": {
                        "Custom::" + self.CUSTOM_RESOURCE_NAME: {
                            "ArnFormat": "*",
                            "HandlerFunction": {
                                "Function": "Custom_" + self.CUSTOM_RESOURCE_NAME + ".handler"
                            }
                        }
                    }
                }
            }

        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, 'resource-template.json') as resource_template:
                resource_template_resources = resource_template['Resources'] = {}
                resource_template_resources['Custom'] = {
                    "Type": "Custom::" + self.CUSTOM_RESOURCE_NAME,
                    "Properties": {
                        "ServiceToken": {"Ref": "ProjectResourceHandler"},
                        "ConfigurationKey": {"Ref": "ConfigurationKey"} 
                    }
                }

    @property
    def resource_group_resource_types_code_path(self):
        return self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'project-code', 'lambda-code', 'testCustomResource', 'resource_types')

