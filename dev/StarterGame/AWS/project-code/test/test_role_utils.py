import unittest

import role_utils

from errors import ValidationError

class Test_role_utils(unittest.TestCase):

    # Note that the LambdaConfigurationResourceHandler's integration
    # tests test the core functionality provided by role_utils.

    def test_get_role_name_with_short_name(self):
        stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/ShortName-RandomPart/TestUUID'
        logical_role_name = 'RoleName'
        actual_role_name = role_utils._get_role_name(stack_arn, logical_role_name)
        self.assertEqual(actual_role_name, 'ShortName-RandomPart-RoleName')

    def test_get_role_name_with_long_name(self):
        stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/LongName9A123456789B123456789C123456789D123456789E123456789F123456789G-RandomPart/TestUUID'
        logical_role_name = 'RoleName'
        actual_role_name = role_utils._get_role_name(stack_arn, logical_role_name)
        self.assertEqual(actual_role_name, 'LongName9A123456789B123456789C123456789D1234-RandomPart-RoleName')

    def test_get_role_name_with_too_long_name(self):
        stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/LongName9A123456789B123456789C123456789D123456789E123456789F123456789G-RandomPart/TestUUID'
        logical_role_name = 'RoleName9A123456789B123456789C123456789D123456789E123456789F123456789G'
        self.assertRaises(ValidationError, role_utils._get_role_name, stack_arn, logical_role_name)

if __name__ == '__main__':
    unittest.main()
