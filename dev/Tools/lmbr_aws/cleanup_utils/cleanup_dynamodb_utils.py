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
#
import time
import botocore

from botocore.exceptions import ClientError

from . import cleanup_utils
from . import exception_utils


def delete_dynamodb_tables(cleaner):
    """
    Deletes all dynamodb from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for dynamo tables with names starting with one of {0}'.format(cleaner.describe_prefixes()))
    table_list = []
    # Construct list
    try:
        db_paginator = cleaner.dynamodb.get_paginator('list_tables')
        db_table_iterator = db_paginator.paginate()

        for page in db_table_iterator:
            for table in page['TableNames']:
                if cleaner.has_prefix(table):
                    print('  found table {}'.format(table))
                    table_list.append(table)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting dynamodb tables. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the dynamodb client. {}".format(
            exception_utils.message(e)))
        return

    # Delete the tables
    for table in table_list:
        print('    deleting table {}'.format(table))
        try:
            cleaner.dynamodb.delete_table(TableName=table)
        except ClientError as e:
            if e.response["Error"]["Code"] == "LimitExceededException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: {}'.format(e))
                cleaner.add_to_failed_resources('dynamodb', table)

    # Wait for tables to delete
    waiter = cleaner.dynamodb.get_waiter('table_not_exists')
    for table in table_list:
        print("    ... waiting for table {} to delete.".format(table))
        try:
            waiter.wait(TableName=table,
                        WaiterConfig={'Delay': cleaner.wait_interval, 'MaxAttempts': cleaner.wait_attempts})
        except botocore.exceptions.WaiterError as e:
            if cleanup_utils.WAITER_ERROR_MESSAGE in exception_utils.message(e):
                print("ERROR: Timed out waiting for table {} to delete".format(table))
            else:
                print("ERROR: Unexpected error occurred waiting for table {0} to delete due to {1}".format(
                    table, exception_utils.message(e)))
            cleaner.add_to_failed_resources('dynamodb', table)
        print('    Finished deleting table {}'.format(table))
