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

from botocore.exceptions import ClientError

from . import exception_utils
from . import cleanup_utils


def __log_group_exists(cleaner, log_group_name_prefix):
    """
    Verifies if a log exists. This is should be replaced once logs supports Waiter objects for log deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param log_group_name_prefix: Can be retrieved from the boto3 describe_log_groups() with
    response['logGroups']['logGroupName']
    :return: True if the log exists, False if it doesn't exist or an error occurs.
    """
    try:
        response = cleaner.logs.describe_log_groups(logGroupNamePrefix=log_group_name_prefix)
        if not len(response['logGroups']):
            return False
    except KeyError:
        print("      ERROR: Unexpected KeyError trying to search for key 'logGroups' in log group {}"
              .format(log_group_name_prefix))
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if log group {0} exists due to {1}"
              .format(log_group_name_prefix, exception_utils.message(err)))
        return False
    else:
        return True


def delete_log_groups(cleaner):
    """
    Deletes all logs from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for cloudwatch log groups starting with one of {}'.format(cleaner.describe_prefixes()))
    log_group_name_list = []

    # Construct list
    try:
        log_group_paginator = cleaner.logs.get_paginator('describe_log_groups')
        log_group_paginator = log_group_paginator.paginate()

        for page in log_group_paginator:
            for log_group in page['logGroups']:
                log_group_name = log_group['logGroupName']
                prefix_name = log_group_name.replace('/aws/lambda/', '')
                if cleaner.has_prefix(prefix_name):
                    print('  found log {}'.format(log_group_name))
                    log_group_name_list.append(log_group_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting cloudwatch log groups. {}".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the cloudwatch client. {}".format(
            exception_utils.message(e)))
        return

    # Delete log groups
    for log_group_name in log_group_name_list:
        try:
            print('  deleting log group {0}'.format(log_group_name))
            cleaner.logs.delete_log_group(logGroupName=log_group_name)
        except ClientError as e:
            print('      ERROR. Failed to delete log group {0} due to {1}'.format(log_group_name,
                                                                                  exception_utils.message(e)))
            cleaner.add_to_failed_resources("cloudwatch logs", log_group_name)

    # Wait for log groups to be deleted
    for log_group_name in log_group_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __log_group_exists(cleaner, log_group_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. Log group {0} was not deleted after timeout'.format(log_group_name))
            cleaner.add_to_failed_resources("cloudwatch logs", log_group_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for logs {0} to delete due to {1}".format(
                    log_group_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("cloudwatch logs", log_group_name)
