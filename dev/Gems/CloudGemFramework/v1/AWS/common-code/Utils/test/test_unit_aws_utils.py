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
import unittest
from cgf_utils import aws_utils


class UnitTest_CloudGemFramework_AwsUtils_ArnParser(unittest.TestCase):

    def test_with_valid_simple_arn(self):
        arn = aws_utils.ArnParser('arn:partition:service:region:account-id:resource-id')

        # Checking arn parts
        self.assertEqual('partition', arn.partition, 'partition')
        self.assertEqual('service', arn.service, 'service')
        self.assertEqual('region', arn.region, 'region')
        self.assertEqual('account-id', arn.account_id, 'account id')
        self.assertEqual('resource-id', arn.resource_id, 'resource id')
        self.assertEqual(None, arn.resource_type, 'resource type')

        arn_args = {
            'service': arn.service,
            'partition': arn.partition,
            'region': arn.region,
            'accountId': arn.account_id,
            'resourceId': arn.resource_id
        }
        remade_arn = aws_utils.ArnParser.format_arn_string(arn_args)
        self.assertEqual(arn.arn, remade_arn)

    def __test_real_aws_arns(self):
        lambda_arns = {
            # Lambda arns
            'arn:aws:lambda:us-east-1:677027324277:function:cctest0NPM4I0-CRH-CoreResourceTypes-Custom_CognitoIdentityPool',
            'arn:aws:lambda:us-east-1:677027324277:function:cctestC6L1JI1-AH-CoreResourceTypes-AWS_SQS_Queue',
        }

        for arn in lambda_arns:
            parser = aws_utils.ArnParser(arn)

            self.assertTrue(aws_utils.ArnParser.is_valid_arn(arn))
            self.assertEqual(parser.region, 'us-east-1')
            self.assertEqual(parser.resource_type, 'function')

        iam_arns = {
            'arn:aws:iam::677027324277:role/cctestU7METFZ'
        }

        for arn in iam_arns:
            parser = aws_utils.ArnParser(arn)
            self.assertTrue(aws_utils.ArnParser.is_valid_arn(arn))
            self.assertEqual(parser.resource_type, 'role')

    def __test_with_separator(self, separator):
        arn = aws_utils.ArnParser('arn:partition:service:region:account-id:resource-type{}resource-id'.format(separator))

        # Checking arn parts
        self.assertEqual('partition', arn.partition, 'partition')
        self.assertEqual('service', arn.service, 'service')
        self.assertEqual('region', arn.region, 'region')
        self.assertEqual('account-id', arn.account_id, 'account id')
        self.assertEqual('resource-id', arn.resource_id, 'resource id')
        self.assertEqual('resource-type', arn.resource_type, 'resource type')

        arn_args = {
            'service': arn.service,
            'partition': arn.partition,
            'region': arn.region,
            'accountId': arn.account_id,
            'resourceId': arn.resource_id,
            'resourceType': arn.resource_type,
            'separator': separator
        }
        remade_arn = aws_utils.ArnParser.format_arn_string(arn_args)
        self.assertEqual(arn.arn, remade_arn)

    def test_with_valid_arn_with_colon_separator(self):
        self.__test_with_separator(separator=":")

    def test_with_valid_arn_with_slash_separator(self):
        self.__test_with_separator(separator="/")

    def test_for_format_error_with_invalid_separator(self):
        arn_args = {
            "separator": "@"
        }
        with self.assertRaises(Exception):
            aws_utils.ArnParser.format_arn_string(arn_args)

    def test_with_arn_with_missing_region(self):
        source_arn = "arn:aws:iam::111111111111:role/Cognito_MyIdentityPoolAuth_Role"
        ap = aws_utils.ArnParser(source_arn)
        self.assertEqual('', ap.region)
        self.assertEqual('role', ap.resource_type)