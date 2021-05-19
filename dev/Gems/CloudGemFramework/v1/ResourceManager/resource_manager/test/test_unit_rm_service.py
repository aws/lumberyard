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
# $Revision$

import unittest
from unittest.mock import MagicMock

from resource_manager.service import Service

TEST_STACK_NAME = "TestStackName"
TEST_REGION = "TestRegion"
TEST_UUID = "90ce8630-ec89-11ea-930d-0a7bf50fa3c2"
TEST_ACCOUNT = "0123456789"
TEST_STACK_ID = f"arn:aws:cloudformation:{TEST_REGION}:{TEST_ACCOUNT}:stack/{TEST_STACK_NAME}/{TEST_UUID}"


class UnitTest_Service(unittest.TestCase):

    def test_url_fails_with_empty_stack(self):
        # Given
        _context = MagicMock()
        _context.stack = MagicMock()
        _context.stack.describe_stack.return_value = {}
        expected_url = "https://12345.expected.com"

        # When
        with self.assertRaises(RuntimeError) as raised_error:
            service = Service(context=_context, stack_id=TEST_STACK_ID)
            url = service.url

        self.assertTrue('Missing ServiceUrl' in str(raised_error.exception))

    def test_client_is_as_expected(self):
        # Given
        expected_url = "https://aspaopap.some.url"
        _session = MagicMock()
        _context = MagicMock()
        _context.stack = MagicMock()
        _context.stack.describe_stack.return_value = {'Outputs': [{'OutputKey': 'ServiceUrl', 'OutputValue': expected_url}]}

        # When
        service = Service(context=_context, stack_id=TEST_STACK_ID)
        url = service.url
        client = service.get_service_client(session=_session)

        self.assertEqual(expected_url, url)
        self.assertEqual(expected_url, client.url)

