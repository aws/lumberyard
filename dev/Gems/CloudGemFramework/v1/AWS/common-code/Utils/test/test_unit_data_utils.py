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
import json

from cgf_utils.data_utils import Data

class UnitTest_CloudGemFramework_Utils_data_utils_Data(unittest.TestCase):

    def test_init_with_no_args_creates_new_dict(self):
        
        target1 = Data()
        self.assertEquals(target1.DATA, {})

        target2 = Data()
        self.assertEquals(target2.DATA, {})

        self.assertFalse(target1.DATA is target2.DATA)


    def test_init_with_empty_dict_uses_dict(self):

        expected = {}
        target = Data(expected)

        self.assertIs(target.DATA, expected)


    def test_init_sets_simple_attributes_from_dict(self):

        target = Data({ 'A': 1, 'B': 'foo' })

        self.assertEquals(target.A, 1)
        self.assertEquals(target.B, 'foo')


    def test_init_sets_simple_attributes_from_dict_when_read_only(self):

        target = Data({ 'A': 1, 'B': 'foo' }, read_only = True)

        self.assertEquals(target.A, 1)
        self.assertEquals(target.B, 'foo')


    def test_init_wraps_dicts(self):

        target = Data({ 'A': { 'x': 1 } })

        self.assertIsInstance(target.A, Data)
        self.assertEquals(target.A.x, 1)


    def test_init_wraps_dicts_in_lists(self):

        target = Data({ 'A': [ {'x': 1}, {'y': 2} ] })

        self.assertIsInstance(target.A, list)
        self.assertIsInstance(target.A[0], Data)
        self.assertIsInstance(target.A[1], Data)


    def test_init_wraps_dicts_in_nested_lists(self):

        target = Data({ 'A': [ [ {'x': 1} ], [ {'y': 2} ] ] })

        self.assertIsInstance(target.A, list)
        self.assertIsInstance(target.A[0][0], Data)
        self.assertIsInstance(target.A[1][0], Data)


    def test_init_uses_kwargs_for_metadata_attributes(self):

        target = Data(FOO = 10, BAR = 20)

        self.assertEquals(target.FOO, 10)
        self.assertEquals(target.BAR, 20)
        self.assertEquals(target.DATA, {})


    def test_can_set_simple_attributes(self):

        target = Data()

        target.A = 1
        target.B = 'foo'

        self.assertEquals(target.DATA, { 'A': 1, 'B': 'foo' } )


    def test_can_set_dict_attributes(self):

        target = Data()

        target.A = { 'x': 1 }

        self.assertIsInstance(target.A, Data)
        self.assertEquals(target.A.DATA, { 'x': 1 })
        self.assertEquals(target.DATA, { 'A': { 'x': 1 } } )


    def test_can_set_lists_with_simple_items(self):

        target = Data()

        target.A = [1, 2]

        self.assertIsInstance(target.A, list)
        self.assertEquals(target.A, [1, 2])


    def test_can_set_lists_with_dicts(self):

        target = Data()

        target.A = [ {'x': 1}, {'y': 2} ]

        self.assertIsInstance(target.A, list)
        self.assertIsInstance(target.A[0], Data)
        self.assertIsInstance(target.A[1], Data)


    def test_can_set_lists_with_nested_dicts(self):

        target = Data()

        target.A = [ [ {'x': 1} ], [ {'y': 2} ] ]

        self.assertIsInstance(target.A, list)
        self.assertIsInstance(target.A[0][0], Data)
        self.assertIsInstance(target.A[1][0], Data)


    def test_getattr_returns_Data_when_not_read_only(self):

        target = Data()

        self.assertIsInstance(target.A.B.C, Data)


    def test_getattr_rases_when_read_only(self):

        target = Data(read_only = True)

        with self.assertRaises(RuntimeError):
            target.A


    def test_eq(self):

        self.assertEquals(Data({'A': 10}), Data({'A': 10}))
        self.assertNotEquals(Data({'A': 10}), Data({'A': 20}))

        self.assertEquals(Data({'A': 10}), {'A': 10})
        self.assertNotEquals(Data({'A': 10}), {'A': 20})

        self.assertNotEquals(Data({'A': 10}), 'foo')


    def test_nonzero(self):

        self.assertTrue(Data({'A': 10 }))
        self.assertFalse(Data())


    def test_len(self):

        self.assertEquals(len(Data({'A': 10})), 1)
        self.assertEquals(len(Data()), 0)


    def test_contains(self):

        target = Data()
        target.B = 10

        self.assertNotIn('A', target)
        self.assertIn('B', target)


    def test_create_new_with_None(self):

        target = Data(create_new = None)
        target.B = 10

        self.assertIsNone(target.A)
        self.assertEquals(target.B, 10)


    def test_create_new_with_string(self):

        target = Data(create_new = str)
        target.B = 10

        self.assertEquals(target.A, '')
        self.assertEquals(target.B, 10)



if __name__ == '__main__':
    unittest.main()
