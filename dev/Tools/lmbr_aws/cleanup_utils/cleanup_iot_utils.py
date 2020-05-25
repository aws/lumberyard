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


def __iot_policy_exists(cleaner, iot_policy_name):
    """
    Verifies if a policy exists. This is should be replaced once iot supports Waiter objects for policy deletion
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iot_policy_name: Can be retrieved from the boto3 list_policies() with
    response['policies']['policyName']
    :return: True if the policy exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iot_client.get_policy(policyName=iot_policy_name)
    except cleaner.iot_client.exceptions.ResourceNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if iot policy {0} exists due to {1}"
              .format(iot_policy_name, exception_utils.message(err)))
        return False
    return True


def delete_iot_policies(cleaner):
    """
    Deletes all iot policies from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for iot policies with names starting with one of {}'.format(cleaner.describe_prefixes()))
    iot_policy_name_list = []

    # Iot policies are created by the WebCommunicator Gem
    # Construct list
    try:
        iot_policies_paginator = cleaner.iot_client.get_paginator('list_policies')
        iot_policies_page_iterator = iot_policies_paginator.paginate()
        
        for page in iot_policies_page_iterator:
            for iot_policy_info in page['policies']:
                policy_name = iot_policy_info['policyName']
                if cleaner.has_prefix(policy_name):
                    print('  found iot policy {}'.format(policy_name))
                    iot_policy_name_list.append(policy_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting iot policies. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iot client. {}".format(exception_utils.message(e)))
        return

    # Delete policies
    for policy_name in iot_policy_name_list:
        # Clean any dependencies before we delete the policy
        _clean_iot_policies(cleaner, policy_name)
        try:
            print('  deleting iot policy {}'.format(policy_name))
            cleaner.iot_client.delete_policy(policyName=policy_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete iot policy {0} due to {1}'.format(policy_name,
                                                                                       exception_utils.message(e)))
                cleaner.add_to_failed_resources("iot", policy_name)

    # Wait for deletion
    for policy_name in iot_policy_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __iot_policy_exists(cleaner, policy_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. iot policy {0} was not deleted after timeout'.format(policy_name))
            cleaner.add_to_failed_resources("iam", policy_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for iot policy {0} to delete due to {1}".format(
                    policy_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iot", policy_name)


def _clean_iot_policies(cleaner, iot_policy_name):
    """
    Cleans the iot policy by deleting and detaching policy dependencies
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iot_policy_name: The name of the iot policy to clean
    :return: None
    """
    _delete_old_policy_versions(cleaner, iot_policy_name)
    _detach_policies_from_principals(cleaner, iot_policy_name)


def __iot_policy_version_exists(cleaner, iot_policy_name, iot_policy_version_id):
    """
    Verifies if a policy version exists. This is should be replaced once iot supports Waiter objects for policy
    deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iot_policy_version_id: Can be retrieved from the boto3 list_policy_versions with
    response['policyVersions']['versionId']
    :return: True if the policy exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iot.get_policy_version(policyName=iot_policy_name, policyVersionId=iot_policy_version_id)
    except cleaner.iot.exceptions.ResourceNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if policy version {0} exists due to {1}"
              .format(iot_policy_version_id, exception_utils.message(err)))
        return False
    return True


def _delete_old_policy_versions(cleaner, iot_policy_name):
    """
    Deletes all old policy versions from the client. It will construct a list of resources, delete them, then wait
    for each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    params = {'policyName': iot_policy_name}
    iot_policy_version_id_list = []

    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.iot_client.list_policy_versions, params, 'NextToken', 'NextToken'):
            for policy_info in result['policyVersions']:
                if not policy_info['isDefaultVersion']:
                    iot_policy_version_id_list.append(policy_info['versionId'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting policy version. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iot client. {}".format(exception_utils.message(e)))
        return

    # Delete glue crawlers
    for policy_version_id in iot_policy_version_id_list:
        try:
            print('  deleting iot policy {} version {}'.format(iot_policy_name, policy_version_id))
            cleaner.iot_client.delete_policy_version(policyName=iot_policy_name, policyVersionId=policy_version_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete iot policy {0} due to {1}'
                      .format(policy_version_id, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iot", policy_version_id)

    # Wait for deletion
    for policy_version_id in iot_policy_version_id_list:
        try:
            cleanup_utils.wait_for(lambda: not __iot_policy_version_exists(cleaner, iot_policy_name, policy_version_id),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. iot policy {0} was not deleted after timeout'.format(policy_version_id))
            cleaner.add_to_failed_resources("iot", policy_version_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for iot policy {0} to delete due to {1}"
                      .format(policy_version_id, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iot", policy_version_id)


def _detach_policies_from_principals(cleaner, iot_policy_name):
    """
    Detaches all policies from the client. It will construct a list of resources and detaches them. There is
    currently no boto3 functionality to check if a policy is attached to a principal, so we are skipping validation.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    iot_principal_name_list = []

    # Construct list
    try:
        iot_principal_policies_paginator = cleaner.iot_client.get_paginator('list_policy_principals')
        iot_principal_policies_iterator = iot_principal_policies_paginator.paginate(policyName=iot_policy_name)

        for page in iot_principal_policies_iterator:
            for iot_principal_info in page['principals']:
                if ':cert/' in iot_principal_info:
                    # For a cert, the principal is the full arn
                    principal_name = iot_principal_info
                else:
                    # Response is in the form of accountId:CognitoId - when we detach we only want cognitoId
                    principal_name = iot_principal_info.split(':', 1)[1]
                print('  found principal {}'.format(principal_name))
                iot_principal_name_list.append(principal_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while detaching policies from principals. {}".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iot client. {}".format(exception_utils.message(e)))
        return

    # Detach policies from principals
    for principal_name in iot_principal_name_list:
        try:
            print('  Detaching policy {} from principal {}'.format(iot_policy_name, principal_name))
            cleaner.iot_client.detach_principal_policy(policyName=iot_policy_name, principal=principal_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to detach policy {0} from principal {1} due to {2}'
                      .format(iot_policy_name, principal_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iot", principal_name)
