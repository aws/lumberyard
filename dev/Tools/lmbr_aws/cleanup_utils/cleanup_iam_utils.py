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


def global_resource(cleanup_function):
    def wrapper(_cleaner):
        if _cleaner.delete_global_resources:
            cleanup_function(_cleaner)
        else:
            print("Skipping deletion of global {} (use --delete-global-resources or exclude --region to delete)".
                  format(cleanup_function.__name__[len("_delete_"):]))

    return wrapper


def _clean_iam_role(cleaner, iam_role_name):
    """
    Cleans out a role by deleting and detaching all role policies.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_role_name: The iam role name to be cleaned
    :return: None
    """
    print('    cleaning role {}'.format(iam_role_name))
    _delete_iam_role_policies(cleaner, iam_role_name)
    _detach_iam_role_policies(cleaner, iam_role_name)
    

def __iam_role_policy_exists(cleaner, iam_role_name, iam_policy_name):
    """
    Verifies if a role policy exists. This is should be replaced once iam supports Waiter objects for policy
    deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_policy_name: Can be retrieved from the boto3 list_role_policies with response['PolicyNames']
    :return: True if the role policy exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iam.get_role_policy(RoleName=iam_role_name, PolicyName=iam_policy_name)
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if role policy {0} exists due to {1}"
              .format(iam_policy_name, err))
        return False
    return True


def _delete_iam_role_policies(cleaner, iam_role_name):
    """
    Deletes all inline role policies embedded in the iam role. It will construct a list of resources, delete them,
    then wait for deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_role_name:  The iam role name to be cleaned
    :return: None
    """
    print('    deleting role policies for {}'.format(iam_role_name))
    iam_role_policy_name_list = []

    # Construct list
    try:
        iam_role_policy_paginator = cleaner.iam.get_paginator('list_role_policies')
        iam_role_policy_iterator = iam_role_policy_paginator.paginate(RoleName=iam_role_name)
        
        for page in iam_role_policy_iterator:
            for role_policy_name in page['PolicyNames']:
                iam_role_policy_name_list.append(role_policy_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting iam role policies. {}".format(e))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(e))
        return

    # Delete role policies
    for role_policy_name in iam_role_policy_name_list:
        print('      deleting role policy {}'.format(role_policy_name))
        try:
            cleaner.iam.delete_role_policy(RoleName=iam_role_name, PolicyName=role_policy_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete role policy {0} due to {1}'
                      .format(role_policy_name, e))
                cleaner.add_to_failed_resources("iam", role_policy_name)

    # Wait for deletion
    for role_policy_name in iam_role_policy_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __iam_role_policy_exists(cleaner, iam_role_name, role_policy_name),
                                   attempts=cleaner.wait_attempts, interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. role policy {0} was not deleted after timeout'.format(role_policy_name))
            cleaner.add_to_failed_resources("iam", role_policy_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for role policy {0} to delete due to {1}"
                      .format(role_policy_name, e))
                cleaner.add_to_failed_resources("iam", role_policy_name)


def __role_policy_attached(cleaner, iam_role_name, policy_name):
    try:
        cleaner.iam.get_role_policy(RoleName=iam_role_name, PolicyName=policy_name)
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if role policy {0} is attached due to {1}"
              .format(policy_name, err))
        return False
    return True


def _detach_iam_role_policies(cleaner, iam_role_name):
    """
    Detaches all inline role policies embedded in the iam role. It will construct a list of resources, detach them,
    then wait for deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_role_name:  The iam role name to be cleaned
    :return: None
    """
    print('    detaching role policies for {}'.format(iam_role_name))
    iam_attached_policy_info_list = []

    # Construct list
    try:
        iam_attached_policy_paginator = cleaner.iam.get_paginator('list_attached_role_policies')
        iam_attached_policy_iterator = iam_attached_policy_paginator.paginate(RoleName=iam_role_name)
        
        for page in iam_attached_policy_iterator:
            for iam_attached_policy_info in page['AttachedPolicies']:
                iam_attached_policy_info_list.append(iam_attached_policy_info)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while detaching iam role policies. {}".format(e))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(e))
        return

    # detach policies
    for attached_policy_info in iam_attached_policy_info_list:
        print('      detaching policy {}'.format(attached_policy_info['PolicyName']))
        try:
            cleaner.iam.detach_role_policy(RoleName=iam_role_name, PolicyArn=attached_policy_info['PolicyArn'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to detach role policy {0} due to {1}'
                      .format(attached_policy_info['PolicyName'], e))
                cleaner.add_to_failed_resources("iam", attached_policy_info['PolicyName'])

    # Wait for detaching to finish
    for attached_policy_info in iam_attached_policy_info_list:
        try:
            cleanup_utils.wait_for(lambda: not __role_policy_attached(cleaner, iam_role_name,
                                                                      attached_policy_info['PolicyName']),
                                   attempts=cleaner.wait_attempts, interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. policy {0} was not detached after timeout'
                  .format(attached_policy_info['PolicyName']))
            cleaner.add_to_failed_resources("iam", attached_policy_info['PolicyName'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for policy role {0} to detach due to {1}"
                      .format(attached_policy_info['PolicyName'], e))
                cleaner.add_to_failed_resources("iam", attached_policy_info['PolicyName'])


# The iam client only contains a waiter for 'user_exists' and does not contain a waiter for deleting a user
def __iam_user_exists(cleaner, iam_user_name):
    """
    Verifies if a user exists. This is should be replaced once iam supports Waiter objects for user deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_user_name: Can be retrieved from the boto3 list_user() with response['Users']['UserName']
    :return: True if the user exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iam.get_user(UserName=iam_user_name)
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if user {0} exists due to {1}"
              .format(iam_user_name, err))
        return False
    return True


@global_resource
def delete_iam_users(cleaner):
    """
    Deletes all users from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for iam users with names or paths starting with one of {}'.format(
            cleaner.describe_prefixes()))
    iam_user_name_list = []

    # Construct list
    try:
        iam_paginator = cleaner.iam.get_paginator('list_users')
        iam_iterator = iam_paginator.paginate()
        
        for page in iam_iterator:
            for user in page['Users']:
                path = user['Path'][1:]  # remove leading /
                user_name = user['UserName']
                if cleaner.has_prefix(path) or cleaner.has_prefix(user_name):
                    iam_user_name_list.append(user_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError iam users. {}".format(e))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(e))
        return

    # Delete users
    for user_name in iam_user_name_list:
        print('  cleaning user {}'.format(user_name))
        # Clean user dependencies before deleting the user
        _clean_user(cleaner, user_name)
        print('    deleting user {}'.format(user_name))
        try:
            cleaner.iam.delete_user(UserName=user_name)
        except ClientError as e:
            print('      ERROR: Failed to delete user {0} due to {1}'.format(user_name, e))
            cleaner.add_to_failed_resources("iam", user_name)

    # Wait for deletion
    for user_name in iam_user_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __iam_user_exists(cleaner, user_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. user {0} was not deleted after timeout'.format(user_name))
            cleaner.add_to_failed_resources("iam", user_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for user {0} to delete due to {1}".format(
                    user_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", user_name)


def __iam_policy_exists(cleaner, iam_policy_arn):
    """
    Verifies if a policy exists. This is should be replaced once iam supports Waiter objects for policy deletion
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_policy_arn: Can be retrieved from the boto3 list_policies with response['Policies']['Arn']
    :return: True if the policy exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iam.get_policy(PolicyArn=iam_policy_arn)
    except cleaner.iam.exceptions.InvalidInputException:
        return False
    except cleaner.iam.exceptions.NoSuchEntityException:
        print("     WARNING: Unable to check if policy exists on {0}".format(iam_policy_arn))
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if policy {0} exists due to {1}"
              .format(iam_policy_arn, exception_utils.message(err)))
        return False
    return True


@global_resource
def delete_iam_policies(cleaner):
    """
    Deletes all policies from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for iam policies with names or paths starting with one of {}'
          .format(cleaner.describe_prefixes()))
    iam_policy_arn_list = []

    # Construct list
    try:
        iam_policy_paginator = cleaner.iam.get_paginator('list_policies')
        iam_policy_page_iterator = iam_policy_paginator.paginate()
        
        for page in iam_policy_page_iterator:
            for policy_info in page['Policies']:
                policy_path = policy_info['Path'][1:]  # remove leading /
                if cleaner.has_prefix(policy_path) or cleaner.has_prefix(policy_info['PolicyName']):
                    print('  found policy {}'.format(policy_info['PolicyName']))
                    iam_policy_arn_list.append(policy_info['Arn'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting iam policies. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(exception_utils.message(e)))
        return

    # Delete iam policies
    for policy_arn in iam_policy_arn_list:
        print('    deleting policy {}'.format(policy_arn))
        try:
            cleaner.iam.delete_policy(PolicyArn=policy_arn)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete iam policy {0} due to {1}'.format(policy_arn,
                                                                                       exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", policy_arn)

    # Wait for deletion
    for policy_arn in iam_policy_arn_list:
        try:
            cleanup_utils.wait_for(lambda: not __iam_policy_exists(cleaner, policy_arn),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. policy {0} was not deleted after timeout'.format(policy_arn))
            cleaner.add_to_failed_resources("iam", policy_arn)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for policy {0} to delete due to {1}".format(
                    policy_arn, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", policy_arn)


def _clean_user(cleaner, iam_user_name):
    """
    Cleans out a user by deleting and detaching all user policies. Also deletes all access keys.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_user_name: The iam user name to be cleaned
    :return: None
    """
    print('    cleaning iam user {}'.format(iam_user_name))
    _delete_user_policies(cleaner, iam_user_name)
    _detach_user_policies(cleaner, iam_user_name)
    _delete_access_keys(cleaner, iam_user_name)


def __user_policy_exists(cleaner, iam_user_name, iam_policy_name):
    """
    Verifies if a user policy exists. This is should be replaced once iam supports Waiter objects for policy
    deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_policy_name: Can be retrieved from the boto3 list_role_policies with response['PolicyNames']
    :return: True if the role policy exists, False if it doesn't exist or an error occurs.
    """
    # The iam client only contains a waiter for 'policy_exists' and does not contain a waiter for deleting a policy
    try:
        cleaner.iam.get_user_policy(UserName=iam_user_name, PolicyName=iam_policy_name)
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if user policy {0} exists due to {1}"
              .format(iam_policy_name, exception_utils.message(err)))
        return False
    return True


def _delete_user_policies(cleaner, iam_user_name):
    """
    Deletes all inline user policies embedded in the iam role. It will construct a list of resources, delete them,
    then wait for deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_user_name:  The iam user name to be cleaned
    :return: None
    """
    print('    deleting iam user policies for {}'.format(iam_user_name))
    iam_user_policy_list = []

    # Construct list
    try:
        iam_user_policy_paginator = cleaner.iam.get_paginator('list_user_policies')
        iam_user_policy_page_iterator = iam_user_policy_paginator.paginate(UserName=iam_user_name)

        for page in iam_user_policy_page_iterator:
            for policy_name in page['PolicyNames']:
                iam_user_policy_list.append(policy_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting iam user policies. {}.".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(exception_utils.message(e)))
        return

    # Delete policies
    for policy_name in iam_user_policy_list:
        print('      deleting policy {}'.format(policy_name))
        try:
            cleaner.iam.delete_user_policy(UserName=iam_user_name, PolicyName=policy_name)
        except ClientError as e:
            print('      ERROR: Failed to delete policy {0} due to {1}'.format(policy_name, exception_utils.message(e)))

    # Wait for deletion
    for policy_name in iam_user_policy_list:
        try:
            cleanup_utils.wait_for(lambda: not __user_policy_exists(cleaner, iam_user_name, policy_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. policy {0} was not deleted after timeout'.format(policy_name))
            cleaner.add_to_failed_resources("iam", policy_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for policy {0} to delete due to {1}".format(
                    policy_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", policy_name)


def _detach_user_policies(cleaner, iam_user_name):
    """
    Detaches all inline role policies embedded in the iam user. It will construct a list of resources, detach them,
    then wait for deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_user_name: The iam user name to be cleaned
    :return: None
    """
    print('\n\nlooking for user policies with names starting with one of {0}'.format(cleaner.describe_prefixes()))
    iam_attached_user_policy_info_list = []

    # Construct list
    try:
        iam_attached_user_policy_paginator = cleaner.iam.get_paginator('list_attached_user_policies')
        iam_attached_user_policy_page_iterator = iam_attached_user_policy_paginator.paginate(UserName=iam_user_name)

        for page in iam_attached_user_policy_page_iterator:
            for iam_attached_user_policy_info in page['AttachedPolicies']:
                iam_attached_user_policy_info_list.append(iam_attached_user_policy_info)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting user policies. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(exception_utils.message(e)))
        return

    # Detach policies
    for policy_info in iam_attached_user_policy_info_list:
        print('      detaching policy {}'.format(policy_info['PolicyName']))
        try:
            cleaner.iam.detach_user_policy(UserName=iam_user_name, PolicyArn=policy_info['PolicyArn'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to detach policy {0} due to {1}'.format(policy_info['PolicyName'],
                                                                                   exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", policy_info['PolicyName'])

    # Wait for detaching
    for policy_info in iam_attached_user_policy_info_list:
        try:
            cleanup_utils.wait_for(lambda: not __user_policy_exists(cleaner, iam_user_name,
                                                                    policy_info['PolicyName']),
                                   attempts=cleaner.wait_attempts, interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. user policy {0} was not deleted after timeout'.format(policy_info['PolicyName']))
            cleaner.add_to_failed_resources("iam", policy_info['PolicyName'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for user policy {0} to delete due to {1}"
                      .format(policy_info['PolicyName'], exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", policy_info['PolicyName'])


def __access_key_exists(cleaner, iam_user_name, iam_access_key_id):
    """
    Verifies if an access key exists. This is should be replaced once iam supports Waiter objects for access key
    deletions.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_access_key_id: Can be retrieved from the boto3 list_access_keys() with
    response['AccessKeyMetadata']['AccessKeyId']
    :return: True if the access key exists, False if it doesn't exist or an error occurs.
    """
    try:
        # We use update_access_key() to verify if a single key exists. Setting 'Status' to 'Inactive' because
        # it is a required field and we do not want to set to 'Active'
        cleaner.iam.update_access_key(UserName=iam_user_name, AccessKeyId=iam_access_key_id, Status='Inactive')
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if access key {0} exists due to {1}"
              .format(iam_access_key_id, exception_utils.message(err)))
        return False
    return True


def _delete_access_keys(cleaner, iam_user_name):
    """
    Deletes all access keys from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_user_name: The iam user name to be cleaned
    :return: None
    """
    print('\n\nlooking for access keys with names starting with one of {0}'.format(cleaner.describe_prefixes()))
    iam_access_key_id_list = []

    # Construct list
    try:
        iam_access_key_paginator = cleaner.iam.get_paginator('list_access_keys')
        iam_access_key_page_iterator = iam_access_key_paginator.paginate(UserName=iam_user_name)

        for page in iam_access_key_page_iterator:
            for iam_access_key_info in page['AccessKeyMetadata']:
                iam_access_key_id_list.append(iam_access_key_info['AccessKeyId'])
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting access keys. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(exception_utils.message(e)))
        return

    # delete access keys
    for access_key_id in iam_access_key_id_list:
        print('      deleting access key {}'.format(access_key_id))
        try:
            cleaner.iam.delete_access_key(UserName=iam_user_name, AccessKeyId=access_key_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete access key {0} due to {1}'.format(access_key_id,
                                                                                       exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", access_key_id)

    # Wait for deletion
    for access_key_id in iam_access_key_id_list:
        try:
            cleanup_utils.wait_for(lambda: not __access_key_exists(cleaner, iam_user_name, access_key_id),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. access key {0} was not deleted after timeout'.format(access_key_id))
            cleaner.add_to_failed_resources("iam", access_key_id)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for access key {0} to delete due to {1}"
                      .format(access_key_id, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", access_key_id)


def __iam_role_exists(cleaner, iam_role_name):
    """
    Verifies if a role exists. This is should be replaced once iam supports Waiter objects for role deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param iam_role_name: Can be retrieved from the boto3 list_roles() with response['Roles']['RoleName']
    :return: True if the role exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.iam.get_role(RoleName=iam_role_name)
    except cleaner.iam.exceptions.NoSuchEntityException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if role {0} exists due to {1}"
              .format(iam_role_name, exception_utils.message(err)))
        return False
    return True


@global_resource
def delete_iam_roles(cleaner):
    """
    Deletes all roles from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for iam roles with names or paths starting with one of {}'.format(cleaner.describe_prefixes()))
    iam_role_name_list = []

    # Construct list
    try:
        iam_role_paginator = cleaner.iam.get_paginator('list_roles')
        iam_role_iterator = iam_role_paginator.paginate()

        for page in iam_role_iterator:
            for iam_role_info in page['Roles']:
                role_path = iam_role_info['Path'][1:]  # remove leading /
                role_name = iam_role_info['RoleName']
                if cleaner.has_prefix(role_path) or cleaner.has_prefix(role_name):
                    print('  found role {}'.format(role_name))
                    iam_role_name_list.append(role_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting iam roles. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the iam client. {}".format(exception_utils.message(e)))
        return

    # Delete roles
    for role_name in iam_role_name_list:
        # Clean role dependencies before deleting role
        _clean_iam_role(cleaner, role_name)
        try:
            print('    deleting role {}'.format(role_name))
            cleaner.iam.delete_role(RoleName=role_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete role {0} due to {1}'.format(role_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", role_name)

    # Wait for deletion
    for role_name in iam_role_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __iam_role_exists(cleaner, role_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. role {0} was not deleted after timeout'.format(role_name))
            cleaner.add_to_failed_resources("iam", role_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for role {0} to delete due to {1}".format(
                    role_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("iam", role_name)
