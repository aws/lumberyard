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
# $Revision: #1 $

import base64
from boto3.dynamodb.conditions import Key
import unittest
import json
import mock
from mock import MagicMock, call

import dynamodb_pagination

class UnitTest_CloudGemPlayerAccount_ServiceLambda_DynamodbPagination(unittest.TestCase):

    INDEX_NAME = 'TestIndex'
    PARTITION_KEY_NAME = 'TestPartitionKey'
    SORT_KEY_NAME = 'TestSortKey'
    PARTITION_COUNT = 3
    REQUIRED_FIELDS = {'RequiredField': 'Test'}

    maxDiff = None

    # Test a search with 3 partitions all starting at the beginning with interleaved results.
    # Also test a partition with all results used, but isn't on the last page.
    def test_first_page(self):
        partition_1 = self.create_results(1, 'A', 'D')
        partition_2 = self.create_results(2, 'B', 'E', 'G')
        partition_3 = self.create_results(3, 'C', 'F', 'H')
        expected_page = [
            partition_1[0],
            partition_2[0],
            partition_3[0],
            partition_1[1],
            partition_2[1]
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': partition_1, 'LastEvaluatedKey': partition_1[1]},
            {'Items': partition_2},
            {'Items': partition_3}
        ]

        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, None)
        page_token.set_start(2, None)
        page_token.set_start(3, None)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 5
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'limit': page_size, 'forward': True},
            {'partition': 2, 'limit': page_size, 'forward': True},
            {'partition': 3, 'limit': page_size, 'forward': True}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]},
                    '3': {'key': partition_3[0]}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {'key': partition_3[0]}
                }
            }
        )

    # Test a forward search using an inclusive page token
    def test_forward_inclusive(self):
        query_for_direction_change_1 = self.create_results(1, 'D')
        partition_1 = self.create_results(1, 'F', 'H', 'J')
        query_for_direction_change_2 = self.create_results(1, 'C')
        partition_2 = self.create_results(2, 'E', 'G', 'I')
        expected_page = [
            partition_2[0],
            partition_1[0],
            partition_2[1],
            partition_1[1],
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': query_for_direction_change_1},
            {'Items': partition_1},
            {'Items': query_for_direction_change_2},
            {'Items': partition_2}
        ]

        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, partition_1[0], True)
        page_token.set_start(2, partition_2[0], True)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': partition_1[0], 'limit': 1, 'forward': False},
            {'partition': 1, 'key': query_for_direction_change_1[0], 'limit': page_size, 'forward': True},
            {'partition': 2, 'key': partition_2[0], 'limit': 1, 'forward': False},
            {'partition': 2, 'key': query_for_direction_change_2[0], 'limit': page_size, 'forward': True}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {}
                }
            }
        )

    # Test a backward search using an inclusive page token
    def test_backward_inclusive(self):
        query_for_direction_change_1 = self.create_results(1, 'H')
        partition_1 = self.create_results(1, 'F', 'D', 'B')
        query_for_direction_change_2 = self.create_results(1, 'G')
        partition_2 = self.create_results(2, 'E', 'C', 'A')
        expected_page = [
            partition_2[1],
            partition_1[1],
            partition_2[0],
            partition_1[0],
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': query_for_direction_change_1},
            {'Items': partition_1},
            {'Items': query_for_direction_change_2},
            {'Items': partition_2}
        ]

        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.forward = False
        page_token.set_start(1, partition_1[0], True)
        page_token.set_start(2, partition_2[0], True)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': partition_1[0], 'limit': 1, 'forward': True},
            {'partition': 1, 'key': query_for_direction_change_1[0], 'limit': page_size, 'forward': False},
            {'partition': 2, 'key': partition_2[0], 'limit': 1, 'forward': True},
            {'partition': 2, 'key': query_for_direction_change_2[0], 'limit': page_size, 'forward': False}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]}
                }
            }
        )

    # Test a forward search using a match_sort_key page token
    def test_forward_inclusive_match_sort_key(self):
        # 'D' is found on the second page and used as the exclusive start.
        query_for_direction_change_1A = self.create_results(1, 'F', 'F')
        query_for_direction_change_1B = self.create_results(1, 'F', 'D')
        partition_1 = self.create_results(1, 'F', 'H', 'J')
        # 'C' is found on the first page and used as the exclusive start.
        query_for_direction_change_2 = self.create_results(1, 'C')
        partition_2 = self.create_results(2, 'E', 'G', 'I')
        # All pages include matching keys.
        query_for_direction_change_3 = self.create_results(1, 'F', 'F')
        partition_3 = []
        expected_page = [
            partition_2[0],
            partition_1[0],
            partition_2[1],
            partition_1[1],
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': query_for_direction_change_1A, 'LastEvaluatedKey': query_for_direction_change_1A[-1]},
            {'Items': query_for_direction_change_1B},
            {'Items': partition_1},
            {'Items': query_for_direction_change_2},
            {'Items': partition_2},
            {'Items': query_for_direction_change_3, 'LastEvaluatedKey': query_for_direction_change_3[-1]},
            {'Items': query_for_direction_change_3, 'LastEvaluatedKey': query_for_direction_change_3[-1]},
            {'Items': query_for_direction_change_3, 'LastEvaluatedKey': query_for_direction_change_3[-1]},
            {'Items': partition_3}
        ]

        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, partition_1[0], 'match_sort_key')
        page_token.set_start(2, partition_2[0], 'match_sort_key')
        page_token.set_start(3, partition_1[0], 'match_sort_key')

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': partition_1[0], 'limit': 20, 'forward': False},
            {'partition': 1, 'key': query_for_direction_change_1A[-1], 'limit': 20, 'forward': False},
            {'partition': 1, 'key': query_for_direction_change_1B[-1], 'limit': page_size, 'forward': True},
            {'partition': 2, 'key': partition_2[0], 'limit': 20, 'forward': False},
            {'partition': 2, 'key': query_for_direction_change_2[0], 'limit': page_size, 'forward': True},
            {'partition': 3, 'key': partition_1[0], 'limit': 20, 'forward': False},
            {'partition': 3, 'key': query_for_direction_change_3[-1], 'limit': 20, 'forward': False},
            {'partition': 3, 'key': query_for_direction_change_3[-1], 'limit': 20, 'forward': False},
            {'partition': 3, 'key': query_for_direction_change_3[-1], 'limit': page_size, 'forward': True}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {}
                }
            }
        )


    # Test a forward search using an exclusive page token
    def test_forward_exclusive(self):
        partition_1 = self.create_results(1, 'F', 'H', 'J')
        partition_2 = self.create_results(2, 'E', 'G', 'I')
        expected_page = [
            partition_2[0],
            partition_1[0],
            partition_2[1],
            partition_1[1],
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': partition_1},
            {'Items': partition_2}
        ]

        start_key_1 = self.create_results(1, 'E')[0]
        start_key_2 = self.create_results(2, 'D')[0]
        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, start_key_1, False)
        page_token.set_start(2, start_key_2, False)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': start_key_1, 'limit': page_size, 'forward': True},
            {'partition': 2, 'key': start_key_2, 'limit': page_size, 'forward': True},
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {}
                }
            }
        )

    # Test a backward search using an exclusive page token
    def test_backward_exclusive(self):
        partition_1 = self.create_results(1, 'F', 'D', 'B')
        partition_2 = self.create_results(2, 'E', 'C', 'A')
        expected_page = [
            partition_2[1],
            partition_1[1],
            partition_2[0],
            partition_1[0],
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [
            {'Items': partition_1},
            {'Items': partition_2}
        ]

        start_key_1 = self.create_results(1, 'E')[0]
        start_key_2 = self.create_results(2, 'D')[0]
        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.forward = False
        page_token.set_start(1, start_key_1, False)
        page_token.set_start(2, start_key_2, False)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': start_key_1, 'limit': page_size, 'forward': False},
            {'partition': 2, 'key': start_key_2, 'limit': page_size, 'forward': False}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {'key': partition_2[0]},
                    '3': {}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[1]},
                    '2': {'key': partition_2[1]}
                }
            }
        )

    # Test reaching the last page going forward.
    def test_forward_last_page(self):
        # One result.
        partition_1 = self.create_results(1, 'B')
        # No results.
        partition_3 = []
        expected_page = [
            partition_1[0]
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [{'Items': partition_1}, {'Items': partition_3}]

        start_key = self.create_results(1, 'A')[0]
        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, start_key)
        # Partition 2 is is already at the end.
        page_token.set_start(3, start_key)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': start_key, 'limit': page_size, 'forward': True},
            {'partition': 3, 'key': start_key, 'limit': page_size, 'forward': True}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        if forward_page_token:
            print 'Page token was not None: ', json.loads(base64.b64decode(forward_page_token))
        self.assertIsNone(forward_page_token)

        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {},
                    '3': {}
                }
            }
        )

    # Test reaching the last page going backward.
    def test_backward_last_page(self):
        # One result.
        partition_1 = self.create_results(1, 'B')
        # No results.
        partition_3 = []
        expected_page = [
            partition_1[0]
        ]

        table_mock = MagicMock()
        table_mock.query.side_effect = [{'Items': partition_1}, {'Items': partition_3}]

        start_key = self.create_results(1, 'C')[0]
        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.forward = False
        page_token.set_start(1, start_key)
        # Partition 2 is is already at the end.
        page_token.set_start(3, start_key)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'key': start_key, 'limit': page_size, 'forward': False},
            {'partition': 3, 'key': start_key, 'limit': page_size, 'forward': False}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        self.assertEqual(json.loads(base64.b64decode(forward_page_token)), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {},
                    '3': {}
                }
            }
        )
        backward_page_token = search.get_page_token(forward=False)
        if backward_page_token:
            print 'Page token was not None: ', json.loads(base64.b64decode(backward_page_token))
        self.assertIsNone(backward_page_token)

    # Test reaching the last page going forward, with all results on the same partition, and a full page of results.
    def test_forward_last_page_single_partition(self):
        partition_1 = self.create_results(1, 'A', 'B', 'C', 'D')
        expected_page = partition_1

        table_mock = MagicMock()
        table_mock.query.side_effect = [{'Items': partition_1}]

        page_token = dynamodb_pagination.PageToken(self.get_config(table_mock))
        page_token.set_start(1, None)

        search = dynamodb_pagination.PaginatedSearch(self.get_config(table_mock), page_token)

        page_size = 4
        page = search.get_next_page(page_size)

        self.assert_query_calls(table_mock.query,
            {'partition': 1, 'limit': page_size, 'forward': True}
        )

        self.assertEqual(page, expected_page)

        forward_page_token = search.get_page_token(forward=True)
        if forward_page_token:
            print 'Page token was not None: ', json.loads(base64.b64decode(forward_page_token))
        self.assertIsNone(forward_page_token)

        backward_page_token = search.get_page_token(forward=False)
        self.assertEqual(json.loads(base64.b64decode(backward_page_token)), {
                'direction': 'backward',
                'partitions': {
                    '1': {'key': partition_1[0]},
                    '2': {},
                    '3': {}
                }
            }
        )

    def test_get_page_token_for_inclusive_start(self):
        table_mock = MagicMock()
        page_token = dynamodb_pagination.get_page_token_for_inclusive_start(self.get_config(table_mock), 'Start', True)

        self.assertEqual(json.loads(base64.b64decode(page_token.serialize())), {
                'direction': 'forward',
                'partitions': {
                    '1': {'key': {self.PARTITION_KEY_NAME: 1, self.SORT_KEY_NAME: 'Start', 'RequiredField': 'Test'}, 'inclusive': 'match_sort_key'},
                    '2': {'key': {self.PARTITION_KEY_NAME: 2, self.SORT_KEY_NAME: 'Start', 'RequiredField': 'Test'}, 'inclusive': 'match_sort_key'},
                    '3': {'key': {self.PARTITION_KEY_NAME: 3, self.SORT_KEY_NAME: 'Start', 'RequiredField': 'Test'}, 'inclusive': 'match_sort_key'}
                }
            }
        )

    def create_results(self, partition, *values):
        return [{self.PARTITION_KEY_NAME: partition, self.SORT_KEY_NAME: value, 'RequiredField': 'Test'} for value in values]

    def get_config(self, table_mock):
        return dynamodb_pagination.PartitionedIndexConfig(
            table=table_mock,
            index_name=self.INDEX_NAME,
            partition_key_name=self.PARTITION_KEY_NAME,
            sort_key_name=self.SORT_KEY_NAME,
            partition_count=self.PARTITION_COUNT,
            required_fields=self.REQUIRED_FIELDS
        )

    def assert_query_calls(self, query, *expected_calls):
        self.assertEqual(len(query.call_args_list), len(expected_calls))
        for index, expected in enumerate(expected_calls):
            try:
                args, kwargs = query.call_args_list[index]
                self.assertEqual(kwargs['ConsistentRead'], False)
                self.assertEqual(kwargs['IndexName'], self.INDEX_NAME)
                self.assert_conditions_equal(kwargs['KeyConditionExpression'], Key(self.PARTITION_KEY_NAME).eq(expected['partition']))
                self.assertEqual(kwargs['Limit'], expected['limit'])
                self.assertEqual(kwargs['ScanIndexForward'], expected['forward'])
                if 'key' in expected:
                    self.assertEqual(kwargs['ExclusiveStartKey'], expected['key'])
                else:
                    self.assertNotIn('ExclusiveStartKey', kwargs)
            except:
                print 'Test failed while checking call at index {}.'.format(index)
                print 'Expected: {}'.format(expected)
                print 'Actual (the structure is expected to be different): {}'.format(kwargs)
                raise

    def assert_conditions_equal(self, condition1, condition2):
        expression1 = condition1.get_expression()
        expression2 = condition2.get_expression()
        # Check the key name.
        self.assertEqual(expression1['values'][0].name, expression2['values'][0].name)
        # Check the key value.
        self.assertEqual(expression1['values'][1], expression2['values'][1])
