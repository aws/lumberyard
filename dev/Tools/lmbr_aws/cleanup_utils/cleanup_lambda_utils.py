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


def __lambda_exists(cleaner, lambda_function_arn):
    """
    Verifies if a function exists. This is should be replaced once lambda supports Waiter objects for function
    deletions.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param lambda_function_arn: Can be retrieved from the boto3 list_functions with
    response['Functions']['FunctionName']
    :return: True if the function exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.lambda_client.get_function(FunctionName=lambda_function_arn)
    except cleaner.lambda_client.exceptions.ResourceNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if lambda {0} exists due to {1}"
              .format(lambda_function_arn, exception_utils.message(err)))
        return False
    return True


def delete_lambdas(cleaner):
    """
    Deletes all lambda functions from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for lambda functions starting with one of {}'.format(cleaner.describe_prefixes()))
    lambda_arn_list = []

    # Construct list
    try:
        lambda_paginator = cleaner.lambda_client.get_paginator('list_functions')
        lambda_page_iterator = lambda_paginator.paginate()
        
        for page in lambda_page_iterator:
            for lambda_info in page['Functions']:
                if cleaner.has_prefix(lambda_info['FunctionName']):
                    print('  found lambda {}'.format(lambda_info['FunctionName']))
                    lambda_arn_list.append(lambda_info['FunctionArn'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting lambda functions. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the lambda client. {}".format(
            exception_utils.message(e)))
        return

    # Delete lambdas
    for lambda_arn in lambda_arn_list:
        try:
            print('  deleting lambda {}'.format(lambda_arn))
            cleaner.lambda_client.delete_function(FunctionName=lambda_arn)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete lambda {0} due to {1}'.format(lambda_arn,
                                                                                   exception_utils.message(e)))
                cleaner.add_to_failed_resources("lambda", lambda_arn)

    # Wait for deletion
    for lambda_arn in lambda_arn_list:
        try:
            cleanup_utils.wait_for(lambda: not __lambda_exists(cleaner, lambda_arn),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. lambda {0} was not deleted after timeout'.format(lambda_arn))
            cleaner.add_to_failed_resources("lambda", lambda_arn)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for lambda {0} to delete due to {1}".format(
                    lambda_arn, exception_utils.message(e)))
                cleaner.add_to_failed_resources("lambda", lambda_arn)
