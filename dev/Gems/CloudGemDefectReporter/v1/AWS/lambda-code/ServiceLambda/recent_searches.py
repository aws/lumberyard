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

import boto3
import CloudCanvas
from datetime import datetime
import uuid

from errors import ClientError

RECENT_SEARCHES_TABLE = None
MAX_SIZE = 10

def add_new_search(search_entry):
    global RECENT_SEARCHES_TABLE
    __validate_search_entry(search_entry)

    search_entry['sql_id'] = str(uuid.uuid4())
    search_entry['timestamp'] = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    search_entry['query_params'] = '*' if not search_entry.get('query_params', '') else search_entry['query_params']

    count = 0
    oldest_entry_index = 0

    __scan_recent_searches_table()

    for index, entry in enumerate(RECENT_SEARCHES_TABLE):
        if entry['user_id'] == search_entry['user_id']:
            count = count + 1

            current_entry_timestamp = datetime.strptime(entry['timestamp'], "%Y-%m-%dT%H:%M:%SZ")
            oldest_entry_timestamp = datetime.strptime(RECENT_SEARCHES_TABLE[oldest_entry_index]['timestamp'], "%Y-%m-%dT%H:%M:%SZ")

            if current_entry_timestamp < oldest_entry_timestamp:
                oldest_entry_index = index

    if (count >= MAX_SIZE):
        oldest_entry_key = {'user_id': RECENT_SEARCHES_TABLE[oldest_entry_index]['user_id'],  'sql_id': RECENT_SEARCHES_TABLE[oldest_entry_index]['sql_id']}
        __get_table().delete_item(Key = oldest_entry_key)
        del RECENT_SEARCHES_TABLE[oldest_entry_index]
  
    __get_table().put_item(Item = search_entry)
    RECENT_SEARCHES_TABLE.append(search_entry)

    return 'SUCCESS'

def get_recent_searches(user_id):
    __scan_recent_searches_table()

    recent_searches = []
    for search_entry in RECENT_SEARCHES_TABLE:
        if search_entry['user_id'] == user_id:
            recent_searches.append(search_entry)

    return recent_searches

def __validate_search_entry(search_entry):
    valid_fields = ['user_id', 'query_params']

    for valid_field in valid_fields:
        if valid_field not in search_entry:
            raise ClientError(valid_field + ' is missing in the request body')

    if not search_entry['user_id']:
        raise ClientError(valid_field + ' is empty in the request body')

def __get_table():
    if not hasattr(__get_table,'recent_searches'):
        __get_table.recent_searches = boto3.resource('dynamodb').Table(CloudCanvas.get_setting('RecentSearches'))
    return __get_table.recent_searches

def __scan_recent_searches_table():
    global RECENT_SEARCHES_TABLE

    if not RECENT_SEARCHES_TABLE:
        response = __get_table().scan()
        RECENT_SEARCHES_TABLE = response.get('Items', [])