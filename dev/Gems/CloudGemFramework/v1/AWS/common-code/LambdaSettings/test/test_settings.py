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
import os
import sys
import unittest

from cgf_utils.data_utils import Data

class UnitTest_CloudGemFramework_LambdaSettings(unittest.TestCase):

    def setUp(self):

        if 'cgf_lambda_settings' in sys.modules:
            del sys.modules['cgf_lambda_settings']

        if 'CloudCanvas' in sys.modules:
            del sys.modules['CloudCanvas']


    @mock.patch('cgf_lambda_settings._LambdaSettingsModule__load_settings')
    def test_settings_loaded_when_imported(self, mock_load_settings):
        import cgf_lambda_settings
        self.assertIs(cgf_lambda_settings.settings, mock_load_settings.return_value)
        self.assertIs(cgf_lambda_settings.settings, mock_load_settings.return_value)
        mock_load_settings.assert_called_once_with()


    @mock.patch('cgf_lambda_settings._LambdaSettingsModule__load_settings', return_value = Data({'Test': 'Settings'}))
    def test_get_settings_returns_setting_value(self, mock_load_settings):
        import CloudCanvas
        import cgf_lambda_settings
        self.assertEquals(cgf_lambda_settings.settings.Test, 'Settings')
        self.assertEquals(cgf_lambda_settings.get_setting('Test'), 'Settings')
        self.assertEquals(CloudCanvas.get_setting('Test'), 'Settings')


    @mock.patch('os.path.isfile', return_value = False)
    def test_load_settings_raises_if_no_settings_file(self, mock_isfile):
        import cgf_lambda_settings
        expected_path = os.path.abspath(os.path.join(__file__, '..', '..', 'cgf_lambda_settings', 'settings.json'))
        with self.assertRaisesRegexp(RuntimeError, 'settings.json'):
            cgf_lambda_settings._LambdaSettingsModule__load_settings()
        mock_isfile.assert_called_once_with(expected_path)


    @mock.patch('os.path.isfile', return_value = True)
    @mock.patch('json.load', return_value = { 'Test': 'Setting' })
    def test_load_settings_loads_settings_file(self, mock_json_load, mock_isfile):
        import cgf_lambda_settings
        mock_open = mock.mock_open()
        with mock.patch('__builtin__.open', mock_open, create = True):
            expected_path = os.path.abspath(os.path.join(__file__, '..', '..', 'cgf_lambda_settings', 'settings.json'))
            result = cgf_lambda_settings._LambdaSettingsModule__load_settings()
            self.assertEquals(result.DATA, mock_json_load.return_value)
            mock_isfile.assert_called_once_with(expected_path)
            mock_open.assert_called_once_with(expected_path, 'r')
            mock_handle = mock_open()
            mock_json_load.assert_called_once_with(mock_handle)

