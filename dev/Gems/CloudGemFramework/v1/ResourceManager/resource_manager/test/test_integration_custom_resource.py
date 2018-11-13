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

import itertools
import json
import subprocess
import os
import time
import urllib

from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import base_stack_test
from cgf_utils import custom_resource_utils
import cgf_service_client
import test_constant

_LAMBDA_NAME = lmbr_aws_test_support.unique_name("VerTestType")
_LAMBDA_FILE_NAME = "VersioningTest"
_TEST_TYPE_NAME = lmbr_aws_test_support.unique_name("Custom::CR")
_TEST_PHYSICAL_ID = "Foo"
_TEST_INSTANCE_A = "TestInstanceA"
_TEST_RESOURCE_TYPE = lmbr_aws_test_support.unique_name("TRT")

class IntegrationTest_CloudGemFramework_CustomResourceHandlers(base_stack_test.BaseStackTestCase):    

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_CustomResourceHandlers, self).__init__(*args, **kwargs)
        self.lambda_code_path = ["project-code", "lambda-code", _LAMBDA_NAME, "resource_types"]

    def setUp(self):        
        self.set_deployment_name(lmbr_aws_test_support.unique_name())
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('icr'))
        self.prepare_test_environment("cloud_gem_framework_custom_resource_test")        

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):                                 
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME,
            '--initial-content', 'no-resources', 
            '--enable','--no-sln-change', ignore_failure=True)
        self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')


    def __010_add_test_type_files(self):
        # Create the project-template.json file that defines our custom resources
        self.make_gem_aws_json(_TEST_RESOURCE_TYPE_DEFINITION, self.TEST_RESOURCE_GROUP_NAME, "project-template.json")

        # Create the files necessary for the custom resource handler lambda code
        self.make_gem_aws_file(_TEST_RESOURCE_LAMBDA_IMPORTS, self.TEST_RESOURCE_GROUP_NAME,
                               *(self.lambda_code_path[:-1] + [".import"]))
        self.make_gem_aws_file("", self.TEST_RESOURCE_GROUP_NAME, *(self.lambda_code_path + ["__init__.py"]))
        self.make_gem_aws_file(_TEST_RESOURCE_LAMBDA_CODE, self.TEST_RESOURCE_GROUP_NAME,
                               *(self.lambda_code_path + ["{}.py".format(_LAMBDA_FILE_NAME)]))

    def __020_update_project(self):
        # Update the project stack to create the new type
        self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change')

    def __030_instantiate_resource(self):
        # Add an instance of the resource to the deployment.
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, "resource-template.json") as template:
            template['Resources'][_TEST_INSTANCE_A] = {
                'Type': _TEST_TYPE_NAME,
                'Properties': {
                    'ServiceToken': {
                        'Ref': "ProjectResourceHandler"
                    }
                }
            }

    def __040_update_deployment(self):
        # Add the new instance to AWS.
        self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-resource-deletion', 
                      '--confirm-security-change')

    def __050_validate_correct_physical_id(self):
        stack_arn = self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME)
        physical_id = self.get_stack_resource_physical_id(stack_arn, _TEST_INSTANCE_A)
        info = custom_resource_utils.get_custom_resource_info(physical_id)
        self.assertEqual(info.physical_id, _TEST_PHYSICAL_ID)

    def __060_update_custom_resource_lambda(self):
        self.make_gem_aws_file(_TEST_RESOURCE_LAMBDA_CODE_BAD, self.TEST_RESOURCE_GROUP_NAME,
                               *(self.lambda_code_path + ["{}.py".format(_LAMBDA_FILE_NAME)]))
        self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change')

    def __070_resource_update_uses_previous_version(self):
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, "resource-template.json") as template:
            template['Resources'][_TEST_INSTANCE_A] = {
                'Type': _TEST_TYPE_NAME,
                'Properties': {
                    'ServiceToken': {
                        'Ref': "ProjectResourceHandler"
                    },
                    'Foo': "Bar"
                }
            }
        self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage',  '--confirm-resource-deletion', 
                      '--confirm-security-change')

    def __080_resource_update_with_coercion(self):
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, "resource-template.json") as template:
            template['Resources'][_TEST_INSTANCE_A] = {
                'Type': _TEST_TYPE_NAME,
                'Metadata': {
                    'CloudCanvas': {
                        'CustomResourceVersion': "2"
                    }
                },
                'Properties': {
                    'ServiceToken': {
                        'Ref': "ProjectResourceHandler"
                    }
                }
            }
        result = self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage',
                      '--confirm-security-change', ignore_failure=True)
        self.assertTrue("UPDATE_FAILED" in self.lmbr_aws_stdout and "raise Exception()" in self.lmbr_aws_stdout)

    def __999_cleanup(self):
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')


_TEST_RESOURCE_TYPE_DEFINITION = {
    "Resources": {
        _TEST_RESOURCE_TYPE: {
            "Type": "Custom::ResourceTypes",
            "Properties": {
                "ServiceToken": {
                    "Fn::GetAtt": [
                        "ProjectResourceHandler",
                        "Arn"
                    ]
                },
                "LambdaConfiguration": {
                    "Fn::GetAtt": [
                        "{}LambdaConfig".format(_TEST_RESOURCE_TYPE),
                        "ComposedLambdaConfiguration"
                    ]
                },
                "LambdaTimeout": 30,
                "Definitions": {
                    _TEST_TYPE_NAME: {
                        "ArnFormat": "*",
                        "HandlerFunction": {
                            "Function": "{}.handler".format(_LAMBDA_FILE_NAME)
                        }
                    }
                }
            }
        },
        "{}LambdaConfig".format(_TEST_RESOURCE_TYPE): {
            "Properties": {
                "ConfigurationBucket": {
                    "Ref": "Configuration"
                },
                "ConfigurationKey": {
                    "Ref": "ConfigurationKey"
                },
                "FunctionName": "{}".format(_LAMBDA_NAME),
                "Runtime": "python2.7",
                "ServiceToken": {
                    "Fn::GetAtt": [
                        "ProjectResourceHandler",
                        "Arn"
                    ]
                }
            },
            "Type": "Custom::LambdaConfiguration",
            "DependsOn": [
                "CoreResourceTypes"
            ]
        }
    }
}

_TEST_RESOURCE_LAMBDA_IMPORTS = """
CloudGemFramework.Utils
""".strip()

_TEST_RESOURCE_LAMBDA_CODE = """
from cgf_utils import custom_resource_response

def handler(event, context):
    return custom_resource_response.success_response({}, "%s")
""" % _TEST_PHYSICAL_ID

_TEST_RESOURCE_LAMBDA_CODE_BAD = """
from cgf_utils import custom_resource_response

def handler(event, context):
    raise Exception()
    return custom_resource_response.success_response({}, "%s")
""" % _TEST_PHYSICAL_ID