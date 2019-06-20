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

import json
import os
import resource_manager_common.constant as  c
from resource_manager.test import base_stack_test
from resource_manager.test import lmbr_aws_test_support
import test_constant

class IntegrationTest_CloudGemFramework_ExternalResource(base_stack_test.BaseStackTestCase):

    # Fails in cleanup to keep the deployment stack intact for the next test rerun.
    FAST_TEST_RERUN = False

    def __init__(self, *args, **kwargs):
        self.base = super(IntegrationTest_CloudGemFramework_ExternalResource, self)
        self.base.__init__(*args, **kwargs)

    def setUp(self):
        self.set_deployment_name(lmbr_aws_test_support.unique_name())
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('rg'))
        self.prepare_test_environment("cloud_gem_external_resource_test")
        self.register_for_shared_resources()

    def test_security_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):
       self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_RESOURCE_GROUP_NAME, '--initial-content', 'no-resources', '--enable', ignore_failure=True)
       self.enable_shared_gem(self.TEST_RESOURCE_GROUP_NAME, 'v1', path=os.path.join(self.context[test_constant.ATTR_ROOT_DIR], os.path.join(test_constant.DIR_GEMS,  self.TEST_RESOURCE_GROUP_NAME)))
       self.base_create_project_stack()

    def __010_add_external_resources_to_project(self):
        project_template_path = self.get_gem_aws_path(
            self.TEST_RESOURCE_GROUP_NAME, c.PROJECT_TEMPLATE_FILENAME)
        if not os.path.exists(project_template_path):
            with open(project_template_path, 'w') as f:
                f.write('{}')
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME,  c.PROJECT_TEMPLATE_FILENAME) as gem_project_template:
            project_extension_resources = gem_project_template['Resources'] = {}
            project_extension_resources[_EXTERNAL_RESOURCE_1_NAME] = _EXTERNAL_RESOURCE_1_INSTANCE
            project_extension_resources[_EXTERNAL_RESOURCE_2_NAME] = _EXTERNAL_RESOURCE_2_INSTANCE
            project_extension_resources['GameDBTableRefernece'] = _EXTERNAL_RESOURCE_1_REFERENCE

    def __020_update_project(self):
        self.base_update_project_stack()

    def __030_verify_external_resource_metadata_on_s3(self):
        configuration_bucket = self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'Configuration')
        external_source_1_key = self.get_reference_metadata_key(_EXTERNAL_RESOURCE_1_NAME)
        self.verify_reference_metadata_on_s3(configuration_bucket, external_source_1_key, _EXTERNAL_RESOURCE_1_REFERENCE_METADATA)

        external_source_2_key = self.get_reference_metadata_key(_EXTERNAL_RESOURCE_2_NAME)
        self.verify_reference_metadata_on_s3(configuration_bucket, external_source_2_key, _EXTERNAL_RESOURCE_2_REFERENCE_METADATA)

    def __40_verify_project_service_lambda_permission(self):
        project_stack_arn = self.get_project_stack_arn()
        project_service_lambda_role = self.get_lambda_function_execution_role(project_stack_arn, 'ServiceLambda')

        self.verify_role_permissions('project',
            self.get_project_stack_arn(),
            project_service_lambda_role,
            [
                {
                    'Resources': map(lambda suffix: _EXTERNAL_RESOURCE_1_ARN + suffix, _EXTERNAL_RESOURCE_1_RESOURCE_SUFFIX),
                    'Allow': _EXTERNAL_RESOURCE_1_ACTIONS
                }
            ])

    def __999_cleanup(self):
        if self.FAST_TEST_RERUN:
            print 'Tests passed enough to reach cleanup, failing in cleanup to prevent stack deletion since FAST_TEST_RERUN is true.'
            self.assertFalse(self.FAST_TEST_RERUN)
        self.unregister_for_shared_resources()
        self.base_delete_deployment_stack()
        self.base_delete_project_stack()

    def get_lambda_function_execution_role(self, stack_arn, function_name):
        function_arn = self.get_stack_resource_arn(stack_arn, function_name)
        res = self.aws_lambda.get_function(FunctionName = function_arn)
        role_arn = res['Configuration']['Role']
        return role_arn[role_arn.rfind('/')+1:]

    def get_reference_metadata_key(self, resource_name):
        return 'reference-metadata/{}/{}.json'.format(self.TEST_PROJECT_STACK_NAME, resource_name)

    def verify_reference_metadata_on_s3(self, configuration_bucket, key, expected_content):
        self.verify_s3_object_exists(configuration_bucket, key)
        content = self.aws_s3.get_object(Bucket=configuration_bucket, Key=key)['Body'].read()
        self.assertEqual(json.loads(content), expected_content)

_EXTERNAL_RESOURCE_1_NAME = 'ExternalResource1'
_EXTERNAL_RESOURCE_2_NAME = 'ExternalResource2'

_EXTERNAL_RESOURCE_1_ARN = "arn:aws:dynamodb:us-west-2:9816236123:table/GameDBTable"

_EXTERNAL_RESOURCE_1_ACTIONS = [
    "dynamodb:Scan",
    "dynamodb:Query",
    "dynamodb:PutItem",
    "dynamodb:GetItem",
]

_EXTERNAL_RESOURCE_1_RESOURCE_SUFFIX = ["/*",""]

_EXTERNAL_RESOURCE_1_REFERENCE_METADATA = {
    "Arn": _EXTERNAL_RESOURCE_1_ARN,
    "PhysicalId": "GameDBTable",
    "Permissions": {
        "Action": _EXTERNAL_RESOURCE_1_ACTIONS,
        "ResourceSuffix": _EXTERNAL_RESOURCE_1_RESOURCE_SUFFIX
    }
}

_EXTERNAL_RESOURCE_1_INSTANCE = {
    "Type": "Custom::ExternalResourceInstance",
    "Properties": {
        "ServiceToken": { "Fn::Join": [ "", [ "arn:aws:lambda:", { "Ref": "AWS::Region" }, ":", { "Ref": "AWS::AccountId" }, ":function:", { "Ref": "ProjectResourceHandler" } ] ] },
        "ReferenceMetadata": _EXTERNAL_RESOURCE_1_REFERENCE_METADATA
    },
    "DependsOn": [
        "CoreResourceTypes"
    ]
}

_EXTERNAL_RESOURCE_2_REFERENCE_METADATA = {
    "Arn": "arn:aws:dynamodb:us-west-2:9816236123:table/PlayerDBTable",
    "PhysicalId": "PlayerDBTable",
    "Permissions": {
        "Action": [
            "dynamodb:Scan",
            "dynamodb:PutItem",
            "dynamodb:GetItem",
            "dynamodb:DeleteItem",
            "dynamodb:UpdateItem"
        ],
        "ResourceSuffix": ["/*",""]
    }
}

_EXTERNAL_RESOURCE_2_INSTANCE = {
    "Type": "Custom::ExternalResourceInstance",
    "Properties": {
        "ServiceToken": { "Fn::Join": [ "", [ "arn:aws:lambda:", { "Ref": "AWS::Region" }, ":", { "Ref": "AWS::AccountId" }, ":function:", { "Ref": "ProjectResourceHandler" } ] ] },
        "ReferenceMetadata": _EXTERNAL_RESOURCE_2_REFERENCE_METADATA
    },
    "DependsOn": [
        "CoreResourceTypes"
    ]
}

_EXTERNAL_RESOURCE_1_REFERENCE = {
    "Type":"Custom::ExternalResourceReference",
    "Metadata": {
        "CloudCanvas": {
            "Permissions": [
                {
                    "AbstractRole": "ServiceLambda"
                }
            ]
        }
    },
    "Properties": {
        "ReferenceName": _EXTERNAL_RESOURCE_1_NAME,
        "ServiceToken": { "Fn::Join": [ "", [ "arn:aws:lambda:", { "Ref": "AWS::Region" }, ":", { "Ref": "AWS::AccountId" }, ":function:", { "Ref": "ProjectResourceHandler" } ] ] }
    },
    "DependsOn": [
        "CoreResourceTypes",
        _EXTERNAL_RESOURCE_1_NAME,
        _EXTERNAL_RESOURCE_2_NAME
    ]
}
