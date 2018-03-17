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

import base64
from boto3.dynamodb.conditions import Key
from decimal import Decimal
import json

DISTINCT_KEY_SEARCH_MAX_PAGES = 3
DISTINCT_KEY_SEARCH_MAX_RESULTS_PER_PAGE = 20

# The main class for returning pages of results from a partitioned global secondary index.
class PaginatedSearch:
    def __init__(self, partitioned_index_config, page_token, start_player_name = ''):
        self.__config = partitioned_index_config

        self.__forward = page_token.forward
        self.__partitions = {}
        for partition in self.__partition_keys:
            self.__partitions[partition] = PaginatedPartition(self.__config, partition, page_token, start_player_name)

    # Gets the next page of results and internally updates the search position.
    def get_next_page(self, max_results):
        partition_results = {}
        partition_has_next_page = {}
        page_results = []
        sort_key = self.__config.sort_key_name

        # Collect the results for each partition.
        for partition in self.__partition_keys:
            partition_results[partition], partition_has_next_page[partition] = self.__partitions[partition].get_next_page(max_results)

            # Dynamo sorts by utf-8 bytes.  http://docs.aws.amazon.com/amazondynamodb/latest/developerguide/QueryAndScan.html
            # This converts the sort keys to utf-8 bytes to make the next sort easier.
            for item in partition_results[partition]:
                page_results.append({'partition': partition, 'item': item, 'sort': bytearray(item.get(sort_key, ''), 'utf-8')})

        # Sort the combined results by utf-8 bytes.
        # This should preserve the order of results within each partition.
        page_results.sort(lambda x,y: cmp(x['sort'], y['sort']), reverse=not self.__forward)

        # Truncate the results if needed to get one page.
        if len(page_results) > max_results:
            del page_results[max_results:]

        # Determine how many results came from each partition.
        results_per_partition = {}
        for result in page_results:
            results_per_partition[result['partition']] = results_per_partition.get(result['partition'], 0) + 1

        # Advance each partition to the start of the next page, which may or may not be the end of the query results.
        # This assumes that ordering was preserved during the above merge.
        for partition in self.__partition_keys:
            items_selected_from_partition = results_per_partition.get(partition, 0)
            is_at_end = items_selected_from_partition == len(partition_results[partition]) and not partition_has_next_page[partition]
            if items_selected_from_partition > 0:
                # The partition start needs to be advanced for the items being returned.
                last_partition_result = partition_results[partition][items_selected_from_partition - 1]
                self.__partitions[partition].update_page_boundaries(partition_results[partition][0], last_partition_result, is_at_end)
            elif is_at_end:
                # The partition reached the end without results.
                self.__partitions[partition].update_page_boundaries(None, None, is_at_end)

        # Only the items are returned.  Discard the result metadata.
        page_items = [result['item'] for result in page_results]

        # Page results are always returned in ascending order regardless of the search direction.
        if not self.__forward:
            page_items.reverse()

        return page_items

    # Get a serialized page token for either the next (forward=True) or previous (forward=False) page.
    def get_page_token(self, forward):
        page_token = PageToken(self.__config, forward=forward)
        for partition in self.__partition_keys:
            self.__partitions[partition].update_page_token(page_token, forward)
        if not page_token.has_next_page():
            return None
        return page_token.serialize()

    @property
    def __partition_keys(self):
        return range(1, self.__config.partition_count + 1)

# Handles serializing the state of the partitioned search.
class PageToken:
    # serialized_token is base64 encoded json:
    #   {
    #       direction: "forward"|"backward", # optional, default forward.
    #       partitions: {
    #           "<partitionNumber>": {       # optional, absent if the partition has no more results.
    #               "key": {...},            # optional, absent if no results have been returned from this partition.
    #               "inclusive"              # optional, valid values: true, false, 'match_sort_key'.  The default is false (exclusive start key).
    #                                        #     'match_sort_key' will attempt to include all values that have the matching sort key value.
    #           }
    #       }
    #   }
    def __init__(self, partitioned_index_config, serialized_token = None, forward = True):
        self.__config = partitioned_index_config
        self.__partitions = {}
        
        if serialized_token:
            token = json.loads(base64.b64decode(serialized_token))
            self.__forward = token.get('direction', 'forward') != 'backward'
            for partition, partition_state in token.get('partitions', {}).iteritems():
                self.__partitions[int(partition)] = partition_state
        else:
            self.__forward = forward

    def serialize(self):
        partitions = {}
        for partition, start in self.__partitions.iteritems():
            serializable_partition = {}
            if 'key' in start:
                serializable_key = {}
                for k,v in start['key'].iteritems():
                    # Decimal is not json serializable.
                    if isinstance(v, Decimal):
                        serializable_key[k] = int(v)
                    else:
                        serializable_key[k] = v
                serializable_partition['key'] = serializable_key
            if 'inclusive' in start:
                serializable_partition['inclusive'] = start['inclusive']
            partitions[partition] = serializable_partition
        token = {
            'direction': 'forward' if self.forward else 'backward',
            'partitions': partitions
        }
        return base64.b64encode(json.dumps(token))

    @property
    def forward(self):
        return self.__forward

    @forward.setter
    def forward(self, forward):
        self.__forward = forward

    def is_partition_at_end(self, partition):
        return partition not in self.__partitions

    def is_inclusive(self, partition):
        return self.__partitions.get(partition, {}).get('inclusive')

    def has_next_page(self):
        return bool(self.__partitions)

    def get_start_key(self, partition):
        return self.__partitions.get(partition, {}).get('key')

    def set_start(self, partition, item, inclusive = False):
        start = {}
        if item != None:
            start['key'] = self.__config.convert_item_to_start_key(item)
            if inclusive:
                start['inclusive'] = inclusive
        self.__partitions[partition] = start

# Constructs a page token given a starting value that should be at the beginning of the results.
def get_page_token_for_inclusive_start(config, inclusive_start, forward):
    page_token = PageToken(config)
    page_token.forward = forward

    for partition in range(1, config.partition_count + 1):
        key = {config.partition_key_name: partition, config.sort_key_name: inclusive_start}
        page_token.set_start(partition, key, inclusive='match_sort_key')

    return page_token

# Handles fetching pages for one partition.
class PaginatedPartition:
    def __init__(self, partitioned_index_config, partition, page_token, start_player_name = ''):
        self.__partitioned_index_config = partitioned_index_config
        self.__partition = partition
        self.__forward = page_token.forward
        self.__is_at_end = page_token.is_partition_at_end(partition)
        self.__previous_page_first_item = None
        self.__start_player_name = start_player_name
        if not self.__is_at_end:
            self.__inclusive = page_token.is_inclusive(partition)
            self.__start_key = page_token.get_start_key(partition)

    # Gets the next page of results.  Does not internally update the search position since not all results may be used.
    def get_next_page(self, max_results):
        if self.__is_at_end:
            return [], False

        # DynamoDB only supports exclusive start.
        # If the key is inclusive, convert it to an exclusive start.
        if self.__start_key and self.__inclusive:
            # Search the other direction for the item that came before the start key.
            if self.__inclusive == 'match_sort_key':
                # Search backward to find the new exclusive start key, or None if it reached the end.
                self.__start_key = self.__find_next_distinct_sort_key(self.__start_key, not self.__forward)
            else:
                reverse_search_results, has_next_page = self.__query(self.__start_key, not self.__forward, 1)
                if reverse_search_results:
                    # This is the new exlusive start key.
                    self.__start_key = self.__partitioned_index_config.convert_item_to_start_key(reverse_search_results[0])
                else:
                    # The start key is the first item for the requested direction.
                    # Clear the start key to start at the beginning.
                    self.__start_key = None

            self.__inclusive = False

        # The key is an exclusive start, or it's missing.  Get the next page.
        return self.__query(self.__start_key, self.__forward, max_results)

    # Updates the internal search position.
    # first_item: The first item from the previous page.
    # last_item: The last item used from the previous page.
    # is_at_end: When true, no attempt will be made to get the next page, and the backward token will start from the end.
    def update_page_boundaries(self, first_item, last_item, is_at_end):
        self.__is_at_end = is_at_end
        self.__inclusive = False
        self.__previous_page_first_item = first_item
        if is_at_end:
            self.__start_key = None
        else:
            self.__start_key = self.__partitioned_index_config.convert_item_to_start_key(last_item)

    # Store the current search position in the page_token.
    def update_page_token(self, page_token, forward):
        if self.__forward == forward:
            if not self.__is_at_end:
                # Continuing to search the same direction.  Use the current start key.
                page_token.set_start(self.__partition, self.__start_key, inclusive=self.__inclusive)
        else:
            # Switching to search the other direction.
            if self.__previous_page_first_item:
                # If a search was just done and there were results, use the first item from that to construct the token.
                page_token.set_start(self.__partition, self.__previous_page_first_item, inclusive=False)
            else:
                # The only position information available is the current start key.
                page_token.set_start(self.__partition, self.__start_key, inclusive=(not self.__inclusive))

    def __find_next_distinct_sort_key(self, start_key, forward):
        config = self.__partitioned_index_config
        sort_key_name = config.sort_key_name

        next_start_key = start_key

        # Search for the next distint key for a limited number of pages.
        for attempt in range(DISTINCT_KEY_SEARCH_MAX_PAGES):
            reverse_search_results, has_next_page = self.__query(next_start_key, forward, DISTINCT_KEY_SEARCH_MAX_RESULTS_PER_PAGE)
            for item in reverse_search_results:
                if item[sort_key_name] != start_key[sort_key_name]:
                    return config.convert_item_to_start_key(item)
            if not has_next_page or not reverse_search_results:
                return None
            next_start_key = config.convert_item_to_start_key(reverse_search_results[-1])

        # Give up and return the last key found.
        return next_start_key

    def __query(self, exclusive_start, forward, max_results):
        config = self.__partitioned_index_config

        # The query limit is set to the same limit as the number of returned results in case the results all come from
        # the same partition key.  An eventually consistent query consumes half a unit per 4KB regardless of how
        # many items contributed to the 4KB, so it's not too inefficient to read a lot of small items and throw most of them away.
        # This implementation expects searches to be relatively infrequent.
        queryArgs = {
            'ConsistentRead': False,
            'IndexName': config.index_name,
            'KeyConditionExpression': Key(config.partition_key_name).eq(self.__partition),
            'Limit': max_results,
            'ScanIndexForward': forward
        }

        if self.__start_player_name:
            queryArgs['KeyConditionExpression'] = queryArgs['KeyConditionExpression'] & Key(config.sort_key_name).eq(self.__start_player_name)

        if exclusive_start:
            queryArgs['ExclusiveStartKey'] = exclusive_start

        response = config.table.query(**queryArgs)

        return response.get('Items', []), 'LastEvaluatedKey' in response

# The configuration of the partitioned global secondary index.
class PartitionedIndexConfig:
    def __init__(self, table, index_name, partition_key_name, sort_key_name, partition_count, required_fields = {}):
        self.__table = table
        self.__index_name = index_name
        self.__partition_key_name = partition_key_name
        self.__sort_key_name = sort_key_name
        self.__partition_count = partition_count
        self.__required_fields = required_fields

    # Client for the table.
    @property
    def table(self):
        return self.__table

    # The name of the global secondary index.
    @property
    def index_name(self):
        return self.__index_name

    # The name of the partition key for the index.
    @property
    def partition_key_name(self):
        return self.__partition_key_name

    # The name of the sort key for the index.
    @property
    def sort_key_name(self):
        return self.__sort_key_name

    # The number of partitions in the index.
    @property
    def partition_count(self):
        return self.__partition_count

    # Fields other than the partition key and sort key that are needed to construct a key that will be considered valid for this index.
    @property
    def required_fields(self):
        return self.__required_fields

    # Given an item from the table, return a copy that contains only the fields that query expects for this index.
    def convert_item_to_start_key(self, item):
        result = {}
        result[self.__partition_key_name] = item[self.__partition_key_name]
        result[self.__sort_key_name] = item[self.__sort_key_name]
        for key, value in self.__required_fields.iteritems():
            if key in item:
                result[key] = item[key]
            else:
                result[key] = value

        return result