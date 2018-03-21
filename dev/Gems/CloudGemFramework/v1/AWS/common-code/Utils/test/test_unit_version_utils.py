import unittest

from cgf_utils.version_utils import Version

class UnitTest_CloudGemFramework_Utils_version_utils_Version(unittest.TestCase):
    

    def test_init(self):
        v = Version("1.2.3")
        self.assertEquals(v.major, 1)
        self.assertEquals(v.minor, 2)
        self.assertEquals(v.revision, 3)


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
