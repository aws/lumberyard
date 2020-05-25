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

# Common utils used in test_unit_rm test
from unittest import mock


class AnyArg(object):
    """Provide an ANY object for assert_called_with tests"""
    def __eq__(a, b):
        return True


def make_os_path_patcher(side_effect):
    """Patch up os.path.exists with the side_effects required"""
    patcher = mock.patch('os.path.exists')
    mock_os_exists = patcher.start()
    mock_os_exists.side_effect = side_effect
    return mock_os_exists
