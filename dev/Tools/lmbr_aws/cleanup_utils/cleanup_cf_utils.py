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
import botocore

from botocore.exceptions import ClientError

from . import cleanup_utils
from . import exception_utils


def delete_cf_stacks(cleaner):
    """
    Call CloudFormation to delete the stack. We make use of CloudFormation's Waiter to check the status of the stack
    (https://boto3.readthedocs.io/en/latest/reference/services/cloudformation.html#CloudFormation.Waiter.StackDeleteComplete)
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for stacks with names starting with one of {}'.format(cleaner.describe_prefixes()))
    stack_list = []
    
    # Construct list of stacks to delete
    try:
        stack_paginator = cleaner.cf.get_paginator('list_stacks')
        # Ignore stacks in DELETE_COMPLETE or DELETE_IN_PROGRESS
        # Status list: https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/cloudformation.html#CloudFormation.Client.list_stacks
        stack_page_iterator = stack_paginator.paginate(
            StackStatusFilter=['CREATE_IN_PROGRESS', 'CREATE_FAILED', 'CREATE_COMPLETE', 'ROLLBACK_IN_PROGRESS',
                               'ROLLBACK_FAILED', 'ROLLBACK_COMPLETE', 'DELETE_FAILED', 'UPDATE_IN_PROGRESS',
                               'UPDATE_COMPLETE_CLEANUP_IN_PROGRESS', 'UPDATE_COMPLETE',
                               'UPDATE_ROLLBACK_IN_PROGRESS', 'UPDATE_ROLLBACK_FAILED',
                               'UPDATE_ROLLBACK_COMPLETE_CLEANUP_IN_PROGRESS',
                               'UPDATE_ROLLBACK_COMPLETE', 'REVIEW_IN_PROGRESS', 'IMPORT_IN_PROGRESS',
                               'IMPORT_COMPLETE', 'IMPORT_ROLLBACK_IN_PROGRESS', 'IMPORT_ROLLBACK_FAILED',
                               'IMPORT_ROLLBACK_COMPLETE'])

        for page in stack_page_iterator:
            summaries = page['StackSummaries']
            for stack in summaries:
                if cleaner.has_prefix(stack['StackName']):
                    print('  found stack {0} with status {1}'.format(stack['StackName'], stack['StackStatus']))
                    stack_list.append(stack)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting cloud formation stacks. {}".format(
            exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the cloud formation client. {}".format(
            exception_utils.message(e)))
        return

    # Delete the stacks
    for stack in stack_list:
        stack_id = stack['StackId']
        retained_resources = _clean_stack(cleaner, stack_id)
        print('    deleting stack {}'.format(stack_id))
        try:
            if stack['StackStatus'] == 'DELETE_FAILED':
                cleaner.cf.delete_stack(StackName=stack_id, RetainResources=retained_resources)
            else:
                cleaner.cf.delete_stack(StackName=stack_id)
        except ClientError as e:
            print('      ERROR. Failed to delete stack {0} due to {1}'.format(stack_id, exception_utils.message(e)))
            cleaner.add_to_failed_resources('cloudformation', stack_id)

    # Wait for the stacks to delete
    waiter = cleaner.cf.get_waiter('stack_delete_complete')
    for stack in stack_list:
        stack_id = stack['StackId']
        try:
            waiter.wait(StackName=stack_id,
                        WaiterConfig={'Delay': cleaner.wait_interval, 'MaxAttempts': cleaner.wait_attempts})
        except botocore.exceptions.WaiterError as e:
            if cleanup_utils.WAITER_ERROR_MESSAGE in exception_utils.message(e):
                print("      ERROR: Timed out waiting for stack {} to delete".format(stack_id))
            else:
                print("      ERROR: Unexpected error occurred waiting for stack {0} to delete due to {1}".format(
                    stack_id, exception_utils.message(e)))
            cleaner.add_to_failed_resources('cloudformation', stack_id)
        print('    Finished deleting stack {}'.format(stack_id))


def _clean_stack(cleaner, stack_id):
    """
    Recursively searches a cloud formation stack for resources that failed to delete. It will then return a list
    of these resources so that delete_stacks() can be called with the RetainResources param.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param stack_id: The cloud formation stack id to search
    :return: a list of resource stack id's
    """
    retained_resources = []
    print('    getting resources for stack {}'.format(stack_id))

    try:
        response = cleaner.cf.describe_stack_resources(StackName=stack_id)
    except ClientError as e:
        print('      ERROR: Error occurred when describe stack resources for {0}. {1}'.format(stack_id,
                                                                                              exception_utils.message(e)))
        return

    # Don't clean roles from stack, let the stack and IAM clean-up take care of that
    for resource in response['StackResources']:
        resource_id = resource.get('PhysicalResourceId', None)
        if resource_id is not None:
            if resource['ResourceType'] == 'AWS::CloudFormation::Stack':
                _clean_stack(cleaner, resource_id)
        if resource['ResourceStatus'] == 'DELETE_FAILED':
            retained_resources.append(resource['LogicalResourceId'])
    return retained_resources
