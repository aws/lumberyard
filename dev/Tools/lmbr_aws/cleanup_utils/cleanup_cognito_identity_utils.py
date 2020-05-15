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


def __identity_pool_exists(cleaner, identity_pool_id):
    """
    Verifies if an identity pool exists. This is should be replaced once cognito supports Waiter objects for
    identity pool deletions.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param identity_pool_id: Can be retrieved from the boto3 list_identity_pools() with
    response['IdentityPools']['IdentityPoolId']
    :return: True if the identity pool exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.cognito_identity.describe_identity_pool(IdentityPoolId=identity_pool_id)
    except cleaner.cognito_identity.exceptions.ResourceNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if pool {0} exists due to {1}"
              .format(identity_pool_id, exception_utils.message(err)))
        return False
    return True


def delete_identity_pools(cleaner):
    """
    Deletes all cognito identity pools with the specified prefix. After deletion, this function will poll the
    client until all specified pools are deleted. list_identity_pools() is not paginatable in LY boto3 version,
    but is paginatable in latest version of boto3.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    params = {'MaxResults': 60}
    pool_id_list = []

    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.cognito_identity.list_identity_pools, params, 'NextToken',
                                             'NextToken'):
            for pool in result['IdentityPools']:
                pool_name = pool['IdentityPoolName']
                for removed in ['PlayerAccess', 'PlayerLogin']:
                    if pool_name.startswith(removed):
                        pool_name = pool_name.replace(removed, '')
                    if cleaner.has_prefix(pool_name):
                        print('  found identity pool {}'.format(pool_name))
                        pool_id_list.append(pool['IdentityPoolId'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting cognito identity pools. {}".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the cognito client. {}".format(
            exception_utils.message(e)))
        return

    # Delete pools
    print('\n\nlooking for cognito identity pools with names starting with one of {}'
          .format(cleaner.describe_prefixes()))
    for pool_id in pool_id_list:
        print('  deleting identity pool {}'.format(pool_id))
        try:
            cleaner.cognito_identity.delete_identity_pool(IdentityPoolId=pool_id)
        except ClientError as e:
            print('      ERROR. Failed to delete identity pool {0} due to {1}'.format(pool_id,
                                                                                      exception_utils.message(e)))
            cleaner.add_to_failed_resources("cognito identity pools", pool_id)

    # Wait for the list to be empty
    for pool_id in pool_id_list:
        try:
            cleanup_utils.wait_for(lambda: not __identity_pool_exists(cleaner, pool_id),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. identity pool {0} was not deleted after timeout'.format(pool_id))
            cleaner.add_to_failed_resources("cognito identity pools", pool_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for identity pool {0} to delete due to {1}"
                      .format(pool_id, exception_utils.message(e)))
                cleaner.add_to_failed_resources("cognito identity pools", pool_id)
