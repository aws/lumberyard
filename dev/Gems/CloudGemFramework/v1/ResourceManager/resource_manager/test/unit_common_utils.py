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
    """
        Patch up os.path.exists with the side_effects required.
        Note: Need to call  patcher.stop() to avoid bleed through to other unit tests
    """
    patcher = mock.patch('os.path.exists')
    mock_os_exists = patcher.start()
    mock_os_exists.side_effect = side_effect
    return patcher


def make_os_path_method_patcher(method='exists', side_effect=None):
    """
        Patch up os.path.{method} with the side_effects required.
        Note: Need to call  patcher.stop() to avoid bleed through to other unit tests
    """
    patcher = mock.patch(f'os.path.{method}')
    mock_os_exists = patcher.start()
    mock_os_exists.side_effect = side_effect
    return patcher


class MockedPopen:
    """ Mocks subprocess open wrapper """

    def __init__(self, args, **kwargs):
        self.args = args
        self.returncode = 0
        self.__stdin = mock.MagicMock()
        self.__stdout = mock.MagicMock()
        self.__expected_output = []
        self.__stdout.readline.side_effect = iter([self.__expected_output])
        self.__exit_code = 0

    def set_output(self, expected_output=None):
        if expected_output is None:
            expected_output = []
        self.__expected_output = expected_output
        self.__stdout.readline.side_effect = iter(expected_output)

    def set_exit_code(self, exit_code=0):
        self.__exit_code = exit_code

    def __enter__(self):
        return self

    def __exit__(self, exc_type, value, traceback):
        pass

    def wait(self):
        """Return None / 0 for normal exit"""
        return self.__exit_code

    @property
    def stdout(self):
        return self.__stdout

    @property
    def stdin(self):
        return self.__stdin

    def communicate(self, input=None, timeout=None):
        if self.args[0] == 'ls':
            stdout = '\n'.join(self.__expected_output)
            stderr = ''
            self.returncode = 0
        else:
            stdout = ''
            stderr = 'unknown command'
            self.returncode = 1

        return stdout, stderr
