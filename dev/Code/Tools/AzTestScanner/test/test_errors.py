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
import unittest
from aztest.errors import RunnerReturnCodes, InvalidUseError


class RunnerReturnCodesTests(unittest.TestCase):

    def test_Constructor_NoParams_ThrowsException(self):
        with self.assertRaises(InvalidUseError) as ex:
            runner_return_codes = RunnerReturnCodes()

    def test_ToString_PassCode_ReturnsNone(self):
        return_code = 0  # TESTS_SUCCEEDED
        error_string = RunnerReturnCodes.to_string(return_code)
        self.assertIsNone(error_string)

    def test_ToString_FailedCode_ReturnsMessage(self):
        return_code = 1  # TESTS_FAILED
        error_string = RunnerReturnCodes.to_string(return_code)
        self.assertEqual(error_string, "Tests Failed")

    def test_ToString_UnknownCode_ReturnsDefaultMessage(self):
        return_code = -1  # Unknown error code
        error_string = RunnerReturnCodes.to_string(return_code)
        self.assertEqual(error_string, "Unknown return code")