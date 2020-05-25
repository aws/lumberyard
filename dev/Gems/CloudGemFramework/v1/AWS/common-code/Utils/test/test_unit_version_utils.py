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

from cgf_utils.version_utils import Version


class UnitTest_CloudGemFramework_Utils_version_utils_Version(unittest.TestCase):

    def test_init(self):
        v = Version("1.2.3")
        self.assertEqual(v.major, 1)
        self.assertEqual(v.minor, 2)
        self.assertEqual(v.revision, 3)

    def test_is_compatible_with(self):
        v = Version("2.2.2")
        
        self.assertTrue(v.is_compatible_with("2.2.2"))
        self.assertTrue(v.is_compatible_with("2.2.1"))
        self.assertTrue(v.is_compatible_with("2.1.2"))
        self.assertTrue(v.is_compatible_with("2.0.0"))

        self.assertFalse(v.is_compatible_with("2.2.3"))
        self.assertFalse(v.is_compatible_with("2.3.2"))
        self.assertFalse(v.is_compatible_with("3.2.2"))
        self.assertFalse(v.is_compatible_with("1.2.2"))

        self.assertTrue(v.is_compatible_with(Version("2.2.2")))

    def test_cmp(self):
        v = Version("2.2.2")

        self.assertTrue(v == "2.2.2")
        self.assertTrue(v == Version("2.2.2"))

        self.assertTrue(v != "1.2.2")
        self.assertTrue(v != "2.1.2")
        self.assertTrue(v != "2.2.1")

        self.assertTrue(v > "2.2.1")
        self.assertTrue(v > "2.1.2")
        self.assertTrue(v > "2.0.0")
        self.assertTrue(v > "1.2.2")

        self.assertTrue(v < "2.2.3")
        self.assertTrue(v < "2.3.2")
        self.assertTrue(v < "3.2.2")


if __name__ == '__main__':
    unittest.main()
