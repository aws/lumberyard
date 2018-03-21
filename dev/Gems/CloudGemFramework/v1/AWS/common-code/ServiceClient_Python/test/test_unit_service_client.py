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
# $Revision: #4 $

import mock
import unittest

import cgf_service_client

class UnitTest_CloudGemFramework_ServiceClient_service_client(unittest.TestCase):

    def test_service_client_imports(self):
        self.assertIsNotNone(cgf_service_client.Data)
        self.assertIsNotNone(cgf_service_client.Path)
        self.assertIsNotNone(cgf_service_client.HttpError)
        self.assertIsNotNone(cgf_service_client.ClientError)
        self.assertIsNotNone(cgf_service_client.NotFoundError)
        self.assertIsNotNone(cgf_service_client.NotAllowedError)
        self.assertIsNotNone(cgf_service_client.ServerError)


    @mock.patch('cgf_service_client.Path')
    def test_for_url(self, mock_Path):

        client = cgf_service_client.for_url('http://example.com', A = 10, B = 20)

        self.assertIs(client, mock_Path.return_value)
        mock_Path.assert_called_once_with('http://example.com', A = 10, B = 20)


if __name__ == '__main__':
    unittest.main()
