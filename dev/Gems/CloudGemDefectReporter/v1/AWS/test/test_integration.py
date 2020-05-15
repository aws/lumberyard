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
# $Revision: #1 $

import json
from resource_manager.test import base_stack_test

# Note: Use CloudGemDefectReporterSample as project

class IntegrationTest_CloudGemDefectReporter_BasicFunctionality(base_stack_test.BaseStackTestCase):
    GEM_NAME = 'CloudGemDefectReporter'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemDefectReporter_BasicFunctionality, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_environment("cloud_gem_defect_reporter_test")
        self.register_for_shared_resources()
        self.enable_shared_gem(self.GEM_NAME, 'v1')

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_setup_stacks(self):
        new_proj_created, new_deploy_created = self.setup_base_stack()

        if not new_deploy_created and self.UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK:
            self.lmbr_aws('function', 'upload-code', '--resource-group', self.GEM_NAME, '-d', self.TEST_DEPLOYMENT_NAME)

    def __100_ensure_notification_lambda_has_expected_policy(self):
        """ Test that S3 trigger lambda has the expected statement """
        gem_stack_arn = self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME)
        lambda_name = self.get_stack_resource_physical_id(gem_stack_arn, 'SanitizationLambda')
        lambda_arn = self.get_stack_resource_arn(gem_stack_arn, 'SanitizationLambda')

        s3_bucket_arn = "arn:aws:s3:::" + self.get_stack_resource_physical_id(gem_stack_arn, 'AttachmentBucket')
        aws_account_id = self.__get_owning_account_from_arn(gem_stack_arn)

        lambda_function_policy = json.loads(self.aws_lambda.get_policy(FunctionName=lambda_name)['Policy'])
        print("lambda_function_policy: {}".format(lambda_function_policy))

        policy_to_match = {
            "Sid": "",              # Not comparing Sid as its not important
            "Effect": "Allow",
            "Principal": {
                "Service": "s3.amazonaws.com"
            },
            "Action": "lambda:InvokeFunction",
            "Resource": lambda_arn,
            "Condition": {
                "StringEquals": {
                    "AWS:SourceAccount": aws_account_id
                },
                "ArnLike": {
                    "AWS:SourceArn": s3_bucket_arn
                }
            }
        }
        statements = lambda_function_policy['Statement']
        self.assertEquals(1, len(statements), "Expected only 1 statement in execution policy")

        lambda_function_statement = statements[0]
        lambda_function_statement['Sid'] = ""           # Ignore for comparison

        self.assertEquals(policy_to_match, lambda_function_statement, "Expected statements do not match")

    def __900_delete_deployment_stack(self):
        self.unregister_for_shared_resources()
        self.lmbr_aws(
            'deployment', 'delete',
            '--deployment', self.TEST_DEPLOYMENT_NAME,
            '--confirm-resource-deletion'
        )

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()

    @staticmethod
    def __get_owning_account_from_arn(arn):
        """
        Get the owning aws account id part of the arn
        """
        if arn is not None:
            return arn.split(':')[4]
        return None
