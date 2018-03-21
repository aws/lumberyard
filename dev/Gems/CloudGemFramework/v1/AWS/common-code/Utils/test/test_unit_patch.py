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

from cgf_utils import patch

class UnitTest_CloudGemFramework_ProjectResourceHandler_patch_OperationListBuilder(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(UnitTest_CloudGemFramework_ProjectResourceHandler_patch_OperationListBuilder, self).__init__(*args, **kwargs)
        self.maxDiff = None

    def test_diff_with_no_changes(self):

        # Setup

        new = { "A": "AA", "B": "BB" }
        old = { "A": "AA", "B": "BB" }

        # Execute

        target = patch.OperationListBuilder()
        target.diff('/', old, new)
        result = target.get()

        # Verify

        self.assertEquals(result, [])


    def test_diff_with_changes(self):

        # Setup

        new = { "A": "AA", "B": "BBBB", "D": "DD" }
        old = { "A": "AA", "B": "BB", "C": "CC" }

        # Execute

        target = patch.OperationListBuilder()
        target.diff('/', old, new)
        result = target.get()

        # Verify

        expected = [
            {
                'op': patch.OperationListBuilder.OP_REPLACE,
                'path': '/B',
                'value': 'BBBB'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/D',
                'value': 'DD'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/C'
            }
        ]

        result_dict = { e['path'] : e for e in result }
        expected_dict = { e['path']: e for e in expected }

        print 'EXPECTED', expected_dict
        print 'ACTUAL  ', result_dict 

        self.assertEquals(expected_dict, result_dict)


    def test_diff_nested_with_no_changes(self):

        # Setup

        new = { "Top": { "A": "AA", "B": "BB", "Nested": { "P": "PP" } } }
        old = { "Top": { "A": "AA", "B": "BB", "Nested": { "P": "PP" } } }

        # Execute

        target = patch.OperationListBuilder()
        target.diff('/', old, new)
        result = target.get()

        # Verify

        self.assertEquals(result, [])


    def test_diff_nested_with_changes(self):

        # Setup

        new = { 
            "Top": { 
                "A": "AA", 
                "B": "BBBB", 
                "D": "DD", 
                "Nested": { 
                    "P": "PP",
                    "Q": "QQQQ",
                    "S": "SS"
                 }
            }, 
            "NewTop": { 
                "W": "WW",
                "X": "XXXX", 
                "Z": "ZZ" 
            } 
        }

        old = { 
            "Top": { 
                "A": "AA", 
                "B": "BB", 
                "C": "CC", 
                "Nested": { 
                    "P": "PP",
                    "Q": "QQ",
                    "R": "RR"
                 }
            }, 
            "OldTop": { 
                "W": "WW",
                "X": "XX",
                "Y": "YY" 
            } 
        }

        # Execute

        target = patch.OperationListBuilder()
        target.diff('/', old, new)
        result = target.get()

        # Verify

        expected = [
            {
                'op': patch.OperationListBuilder.OP_REPLACE,
                'path': '/Top/B',
                'value': 'BBBB'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/Top/C'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/Top/D',
                'value': 'DD'
            },
            {
                'op': patch.OperationListBuilder.OP_REPLACE,
                'path': '/Top/Nested/Q',
                'value': 'QQQQ'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/Top/Nested/R'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/Top/Nested/S',
                'value': 'SS'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/NewTop/W',
                'value': 'WW'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/NewTop/X',
                'value': 'XXXX'
            },
            {
                'op': patch.OperationListBuilder.OP_ADD,
                'path': '/NewTop/Z',
                'value': 'ZZ'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/OldTop/W'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/OldTop/X'
            },
            {
                'op': patch.OperationListBuilder.OP_REMOVE,
                'path': '/OldTop/Y'
            }
        ]

        result_dict = { e['path'] : e for e in result }
        expected_dict = { e['path']: e for e in expected }

        print 'EXPECTED', expected_dict
        print 'ACTUAL  ', result_dict 

        self.assertEquals(expected_dict, result_dict)
        


