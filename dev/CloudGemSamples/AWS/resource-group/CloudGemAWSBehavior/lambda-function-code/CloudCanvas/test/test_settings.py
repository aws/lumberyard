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

import unittest
import mock
import pkgutil

from .. import get_setting

class TestSettings(unittest.TestCase):

    def test_returns_None_when_no_settings_module(self):
        actual = get_setting('TestLogicalName')
        self.assertIsNone(actual)


    def test_loads_settings_module(self):
        
        settings = {
            'TestLogicalName': 'TestPhysicalName'
        }

        with mock.patch.object(pkgutil, 'find_loader') as mock_find_loader:

            mock_find_loader.return_value = mock.MagicMock()
            mock_find_loader.return_value.load_module = mock.MagicMock()
            mock_find_loader.return_value.load_module.return_value = mock.MagicMock()
            mock_find_loader.return_value.load_module.return_value.settings = settings

            self.assertEquals(get_setting('TestLogicalName'), 'TestPhysicalName')
            self.assertEquals(get_setting('NotDefined'), None)

            mock_find_loader.assert_called_once_with('CloudCanvas.settings')
            mock_find_loader.return_value.load_module.called_once_with('CloudCanvas.settings')



