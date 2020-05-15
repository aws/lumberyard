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

from . import cleanup_utils
from . import exception_utils


def __user_pool_exists(cleaner, user_pool_id):
    """
    Verifies if a user pool exists. This is should be replaced once cognito supports Waiter objects for user
    pool deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param user_pool_id: Can be retrieved from the boto3 list_user_pools() with response['UserPools']['Id']
    :return: True if the user pool exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.cognito_idp.describe_user_pool(UserPoolId=user_pool_id)
    except cleaner.cognito_idp.exceptions.ResourceNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if user pool {0} exists due to {1}"
              .format(user_pool_id, exception_utils.message(err)))
        return False
    return True


def delete_user_pools(cleaner):
    """
    Deletes all cognito identity pools with the specified prefix. After deletion, this function will poll the
    client until all specified pools are deleted. list_identity_pools() is not paginatable in LY boto3 version,
    but is paginatable in latest version of boto3.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for cognito user pools with names starting with one of {}'.format(cleaner.describe_prefixes()))
    params = {'MaxResults': 60}
    user_id_list = []

    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.cognito_idp.list_user_pools, params, 'NextToken', 'NextToken'):
            for pool in result['UserPools']:
                pool_name = pool['Name']
                if pool_name.startswith('PlayerAccess'):
                    pool_name = pool_name.replace('PlayerAccess', '')
                if cleaner.has_prefix(pool_name):
                    print('  found user pool {}'.format(pool_name))
                    user_id_list.append(pool['Id'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting cognito user pools. {}".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the cognito client. {}".format(
            exception_utils.message(e)))
        return

    # Delete users
    for user_id in user_id_list:
        print('  deleting user pool {}'.format(user_id))
        try:
            cleaner.cognito_idp.delete_user_pool(UserPoolId=user_id)
        except ClientError as e:
            print('      ERROR. Failed to delete user pool {0} due to {1}'.format(user_id, exception_utils.message(e)))
            cleaner.add_to_failed_resources("cognito user pools", user_id)

    # Wait for the list to be empty
    for user_id in user_id_list:
        try:
            cleanup_utils.wait_for(lambda: not __user_pool_exists(cleaner, user_id),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. user pool {0} was not deleted after timeout'.format(user_id))
            cleaner.add_to_failed_resources("cognito user pools", user_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for pool {0} to delete due to {1}".format(
                    user_id, exception_utils.message(e)))
                cleaner.add_to_failed_resources("cognito user pools", user_id)
