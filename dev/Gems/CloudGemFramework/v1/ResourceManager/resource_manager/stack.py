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
# $Revision: #1 $

import time

from errors import HandledError
from botocore.exceptions import ClientError

import util
import random
import os
from datetime import datetime
from dateutil.tz import tzlocal
import json

from cgf_utils import aws_utils

MONITOR_WAIT_SECONDS = 10

class StackContext(object):

    STATUS_CREATE_COMPLETE        = 'CREATE_COMPLETE'
    STATUS_CREATE_FAILED          = 'CREATE_FAILED'
    STATUS_CREATE_IN_PROGRESS     = 'CREATE_IN_PROGRESS'
    STATUS_DELETE_COMPLETE        = 'DELETE_COMPLETE'
    STATUS_DELETE_FAILED          = 'DELETE_FAILED',
    STATUS_DELETE_IN_PROGRESS     = 'DELETE_IN_PROGRESS'
    STATUS_ROLLBACK_COMPLETE      = 'ROLLBACK_COMPLETE'
    STATUS_ROLLBACK_FAILED        = 'ROLLBACK_FAILED'
    STATUS_UPDATE_COMPLETE        = 'UPDATE_COMPLETE'
    STATUS_UPDATE_FAILED          = 'UPDATE_FAILED',
    STATUS_UPDATE_IN_PROGRESS     = 'UPDATE_IN_PROGRESS'
    STATUS_UPDATE_ROLLBACK_FAILED = 'UPDATE_ROLLBACK_FAILED'
    STATUS_UNKNOWN                = 'UNKNOWN'

    PENDING_CREATE  = 'CREATE'
    PENDING_DELETE  = 'DELETE'
    PENDING_UPDATE  = 'UPDATE'

    PENDING_DELETE_REASON = 'Not in local configuration.'
    PENDING_CREATE_REASON = 'Does not exist in AWS.'
    IS_DISABLED_REASON = 'The resource is disabled.'

    def __init__(self, context):
        self.context = context

    def initialize(self, args):
        pass

    def name_exists(self, stack_name, region):
        cf = self.context.aws.client('cloudformation', region=region)
        try:
            res = cf.describe_stacks(StackName=stack_name)
            for summary in res['Stacks']:
                if summary['StackName'] == stack_name and summary['StackStatus'] != self.STATUS_DELETE_COMPLETE:
                    return True
        except ClientError as e:
            if e.response['Error']['Code'] != 'ValidationError':
                raise HandledError('Could not get stack {} description.'.format(stack_name), e)
        return False

    def id_exists(self, stack_id):
        status = self.get_stack_status(stack_id)
        return status != None and status != self.STATUS_DELETE_COMPLETE

    def get_stack_status(self, stack_id):

        if stack_id is not None:

            cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))
            try:
                res = cf.describe_stacks(StackName=stack_id)
                for summary in res['Stacks']:
                    return summary['StackStatus']
            except ClientError as e:
                if e.response['Error']['Code'] != 'ValidationError': # does not exist or can't access it
                    raise e

        return None

    def create_using_url(self, stack_name, template_url, region, parameters = None, created_callback = None, capabilities = []):

        self.context.view.creating_stack(stack_name)

        parameter_list = []
        if parameters:
            for key, value in parameters.iteritems():
                parameter_list.append(
                    {
                        'ParameterKey': key,
                        'ParameterValue': value
                    }
                )

        cf = self.context.aws.client('cloudformation', region=region)

        try:
            res = cf.create_stack(
                StackName = stack_name,
                TemplateURL = template_url,
                Capabilities = capabilities,
                Parameters = parameter_list
            )
        except ClientError as e:
            raise HandledError('Could not start creation of {0} stack.'.format(stack_name), e)

        if created_callback is not None:
            created_callback(res['StackId'])

        monitor = Monitor(self.context, res['StackId'], 'CREATE')
        monitor.wait()

        return res['StackId']

    def create_using_template(self, stack_name, template_body, region, created_callback = None, capabilities = []):

        self.context.view.creating_stack(stack_name)

        cf = self.context.aws.client('cloudformation', region=region)

        try:
            res = cf.create_stack(
                StackName = stack_name,
                TemplateBody = template_body,
                Capabilities = capabilities
            )
        except ClientError as e:
            raise HandledError('Could not start creation of {0} stack.'.format(stack_name), e)

        if created_callback is not None:
            created_callback(res['StackId'])

        monitor = Monitor(self.context, res['StackId'], 'CREATE')
        monitor.wait()

        return res['StackId']

    def update(self, stack_id, template_url, parameters={}, pending_resource_status={}, capabilities = []):

        stack_name = util.get_stack_name_from_arn(stack_id)

        self.context.view.updating_stack(stack_name, template_url, parameters)

        if pending_resource_status is not None:
            self.__clean_undeltable_resources(stack_id, pending_resource_status=pending_resource_status)

        monitor = Monitor(self.context, stack_id, 'UPDATE')

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        parameter_list = [ { 'ParameterKey': k, 'ParameterValue': v } for k, v in parameters.iteritems() ]

        try:
            res = cf.update_stack(
                StackName = stack_id,
                TemplateURL = template_url,
                Capabilities = capabilities,
                Parameters = parameter_list
            )
        except ClientError as e:
            raise HandledError('Could not start update of {} stack ({}).'.format(stack_name, stack_id), e)

        monitor.wait()

        self.__clean_log_groups(stack_id, pending_resource_status = pending_resource_status)


    def delete(self, stack_id, pending_resource_status = None):

        stack_name = util.get_stack_name_from_arn(stack_id)

        self.context.view.deleting_stack(stack_name, stack_id)

        self.__clean_undeltable_resources(stack_id, pending_resource_status = pending_resource_status)

        monitor = Monitor(self.context, stack_id, 'DELETE')

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        try:
            res = cf.delete_stack(StackName = stack_id)
        except ClientError as e:
            raise HandledError('Could not start delete of {} stack ({}).'.format(stack_name, stack_id), e)

        monitor.wait()

        self.__clean_log_groups(stack_id, pending_resource_status = pending_resource_status)

    def __clean_log_groups(self, stack_id, pending_resource_status = None):
        '''Recursivly removes log groups of the lambda functions that are about to be deleted.'''
        self.__recursivly_remove_resources('log_group', stack_id, pending_resource_status)

    def __clean_undeltable_resources(self, stack_id, pending_resource_status = None):
        '''Recursivly removes content from S3 buckets that are about to be deleted.'''
        self.__recursivly_remove_resources('s3', stack_id, pending_resource_status)

    def __recursivly_remove_resources(self, resource_type, stack_id, pending_resource_status = None):
        '''
        Args:

            stack_id - identifies the stack that may contain resources or nested
            stacks that may contain resources.

            pending_resource_status - The data returned by get_pending_resource_status.
            If specified, only the resources with a DELETE PendingAction are be considered
            when looking for resources that need to get cleaned. If None, then all
            resources are subject to cleaning. When called by stack.update, this identifies
            resources that will be deleted by the update. When called by stack.delete, this is
            None because all resources will be deleted.

        '''        
        deleted_resource_logical_ids, resource_definitions = self.__get_deleted_resources(stack_id, pending_resource_status)

        for logical_resource_id, resource in resource_definitions.iteritems():
            if deleted_resource_logical_ids is None or logical_resource_id in deleted_resource_logical_ids:
                if resource_type == 's3' and resource.get('Type') == 'AWS::S3::Bucket' and resource.get('DeletionPolicy', 'Delete') == 'Delete':
                    self.__remove_bucket_contents(stack_id, logical_resource_id)

                elif resource_type == 'log_group' and resource.get('Type') == 'AWS::Lambda::Function':
                    self.__remove_log_group(stack_id, logical_resource_id)

                elif resource.get('Type') == 'AWS::CloudFormation::Stack':
                    child_stack_id = self.get_physical_resource_id(stack_id, logical_resource_id, optional = True)
                    if child_stack_id:
                        self.__recursivly_remove_resources(resource_type, child_stack_id)

    def __get_deleted_resources(self, stack_id, pending_resource_status):
        # We are given a dict like
        #
        # {
        #   "<logical-resource-id>": <resource-definition>, ...
        #   "<nested-stack-logical-resource-id>": <resource-definition>, ...
        #   "<nested-stack-logical-resource-id>.<logical-resource-id>": <resource-definition>, ...
        # }
        #
        # What we want are the logical ids of the resources being deleted from the target stack only,
        # not the ones in nested stacks (they will be handled because __clear_log_groups and __clean_undeltable_resources works
        # recursivly on nested stacks). So we look for keys with no '.' and ignore all the others.

        if pending_resource_status is not None:
            deleted_resource_logical_ids = set()
            resource_definitions = {}
            for key, value in pending_resource_status.iteritems():
                if key.count('.') == 0 and value.get('PendingAction') == self.PENDING_DELETE:
                    deleted_resource_logical_ids.add(key)
                    resource_definitions[key] = value['OldDefinition']
        else:
            deleted_resource_logical_ids = None
            template = self.get_current_template(stack_id)
            resource_definitions = template.get('Resources')

        return deleted_resource_logical_ids, resource_definitions

    def __remove_bucket_contents(self, stack_id, logical_resource_id):
        physical_bucket_id = self.get_physical_resource_id(stack_id, logical_resource_id)
        stack_name = util.get_stack_name_from_arn(stack_id)
        util.delete_bucket_contents(self.context, stack_name, logical_resource_id, physical_bucket_id)

    def __remove_log_group(self, stack_id, logical_resource_id):
        physical_lambda_id = self.get_physical_resource_id(stack_id, logical_resource_id, optional=True)
        if physical_lambda_id is not None:
            log_group_name = '/aws/lambda/{}'.format(physical_lambda_id)

            region = util.get_region_from_arn(stack_id)
            logs = self.context.aws.client('logs', region=region)
            try:
                logs.delete_log_group(logGroupName = log_group_name)
            except ClientError as e:
                if e.response['Error']['Code'] != 'ResourceNotFoundException':
                    raise HandledError('Could not delete log group {}.'.format(log_group_name), e)

    def get_resource_arn(self, stack_id, logical_resource_id):
        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        try:
            res = cf.describe_stack_resource(
                StackName=stack_id,
                LogicalResourceId=logical_resource_id)
        except ClientError as e:
            if optional and e.response['Error']['Code'] == 'ValidationError':
                return None
            raise HandledError('Could not get the id for the {} resource from the {} stack.'.format( logical_resource_id, stack_id ), e)

        resource_name = res['StackResourceDetail']['PhysicalResourceId']
        resource_type = res['StackResourceDetail']['ResourceType']
        type_definitions = self.context.resource_types.get_type_definitions_for_stack_id(stack_id)

        return aws_utils.get_resource_arn(type_definitions, stack_id, resource_type, resource_name, True)


    def get_physical_resource_id(self, stack_id, logical_resource_id, expected_type = None, optional=False):

        '''Map a logical resource id to a physical resource id.'''

        if stack_id is None:
            if optional:
                return None
            else:
                raise ValueError('No stack_id provided.')

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        try:
            res = cf.describe_stack_resource(
                StackName=stack_id,
                LogicalResourceId=logical_resource_id)
        except ClientError as e:
            if optional and e.response['Error']['Code'] == 'ValidationError':
                return None
            raise HandledError('Could not get the id for the {} resource from the {} stack.'.format( logical_resource_id, stack_id ), e)

        physical_id = res['StackResourceDetail'].get('PhysicalResourceId', None)

        if physical_id is None:
            if not optional:
                raise HandledError('Could not get the id for the {} resource from the {} stack.'.format(logical_resource_id, stack_id))
        else:
            if expected_type:
                if res['StackResourceDetail'].get('ResourceType', None) != expected_type:
                    raise HandledError('The {} resource in stack {} does not have type {} (it has type {})'.format(logical_resource_id, stack_id, expected_type, res['StackResourceDetail'].get('ResourceType', '(unknown)')))

        return physical_id


    def describe_resources(
        self,
        stack_id,
        recursive=True,
        optional=False
    ):

        region = util.get_region_from_arn(stack_id)
        cf = self.context.aws.client('cloudformation', region=region)

        self.context.view.describing_stack_resources(stack_id)

        try:
            res = cf.describe_stack_resources(StackName=stack_id)
        except ClientError as e:
            if optional and e.response['Error']['Code'] == 'ValidationError':
                return {}
            message = e.message
            if e.response['Error']['Code'] == 'ValidationError':
                message += ' Make sure the AWS credentials you are using have access to the project\'s resources.'
            raise HandledError('Could not get stack {} resource data. {}'.format(
                util.get_stack_name_from_arn(stack_id), message))

        resource_descriptions = {}

        for entry in res['StackResources']:
            resource_descriptions[entry['LogicalResourceId']] = entry
            if recursive and entry['ResourceType'] == 'AWS::CloudFormation::Stack':
                physical_resource_id = entry.get('PhysicalResourceId', None)
                if physical_resource_id is not None:
                    logical_resource_id = entry['LogicalResourceId']
                    nested_map = self.describe_resources(physical_resource_id)
                    for k,v in nested_map.iteritems():
                        resource_descriptions[entry['LogicalResourceId'] + '.' + k] = v
            elif entry['ResourceType'] == 'Custom::CognitoUserPool':    # User Pools require extra information (client id/secret)
                resource_descriptions[entry['LogicalResourceId']]['UserPoolClients'] = []
                idp = self.context.aws.client('cognito-idp', region=region)
                pool_id = entry.get('PhysicalResourceId', None)
                # Lookup client IDs if the pool ID is valid.  Valid pool ids must contain an underscore.
                # CloudFormation initializes the physical ID to a UUID without an underscore before the resource is created.
                # If the pool creation doesn't happen or it fails, the physical ID isn't updated to a valid value.
                if pool_id is not None and pool_id.find('_') >= 0:
                    try:
                        client_list = idp.list_user_pool_clients(UserPoolId=pool_id, MaxResults=60)['UserPoolClients']
                    except ClientError as e:
                        client_list = {}
                        if e.response['Error']['Code'] == 'ResourceNotFoundException':
                            continue
                    collected_details = {}
                    for client in client_list:
                        client_name = client['ClientName']
                        client_id = client['ClientId']
                        client_description = idp.describe_user_pool_client(UserPoolId=pool_id, ClientId=client_id)['UserPoolClient']
                        collected_details[client_name] = {
                            'ClientId': client_id
                        }
                    resource_descriptions[entry['LogicalResourceId']]['UserPoolClients'] = collected_details

        return resource_descriptions


    def get_pending_resource_status(
        self,
        stack_id,
        new_template = {},
        new_parameter_values = {},
        new_content_paths = {},
        is_enabled = True
    ):

        if stack_id:
            resource_descriptions = self.describe_resources(stack_id, recursive=False)
            old_template = self.get_current_template(stack_id)
            old_parameter_values = self.get_current_parameters(stack_id)
        else:
            resource_descriptions = {}
            old_template = {}
            old_parameter_values = {}

        new_resource_definitions = new_template.get('Resources', {})
        old_resource_definitions = old_template.get('Resources', {})

        changed_reference_targets = self.__determine_changed_parameters(new_template, old_template, new_parameter_values, old_parameter_values)

        # look for added or changed resource definitions...

        for logical_resource_name, new_resource_definition in new_resource_definitions.iteritems():

            resource_description = resource_descriptions.get(logical_resource_name, None)

            if resource_description is None:

                if is_enabled:

                    # is pending create because it is in new template but not in descriptions map, and is enabled

                    resource_description = {
                        'ResourceType': new_resource_definition.get('Type', None),
                        'PendingAction': self.PENDING_CREATE,
                        'PendingReason': self.PENDING_CREATE_REASON,
                        'NewDefinition': new_resource_definition
                    }

                    self.__check_for_security_metadata(resource_description, new_resource_definition)
    
                else:

                    # is not enabled

                    resource_description = {
                        'ResourceType': new_resource_definition.get('Type', None),
                        'ResourceStatus': 'DISABLED',
                        'NewDefinition': new_resource_definition
                    }

                resource_descriptions[logical_resource_name] = resource_description

            else:

                old_resource_definition = old_resource_definitions.get(logical_resource_name)

                if is_enabled:

                    # is in new template and in description map, look for changes...

                    self.__diff_resource(
                        logical_resource_name,
                        resource_description,
                        new_resource_definition,
                        old_resource_definition,
                        changed_reference_targets,
                        new_content_paths
                    )

                else:

                    # Is pending delete because resources are disabled.

                    resource_description.update(
                        {
                            'PendingAction': self.PENDING_DELETE,
                            'PendingReason': self.IS_DISABLED_REASON,
                            'OldDefinition': old_resource_definition
                        }
                    )


        # look for removed resource definitions...

        for logical_resource_name, old_resource_definition in old_resource_definitions.iteritems():

            # skip if in new template (was processed above)

            if logical_resource_name in new_resource_definitions.keys():
                continue

            resource_description = resource_descriptions.get(logical_resource_name)
            if resource_description:

                # Is pending delete because it is in the old template but not the old one.
                #
                # Ignore definitions in the old template for which there are not descriptions.
                # This can happen if the stack changes between when the resource descriptions
                # are read and the current template is read.

                resource_description.update(
                    {
                        'PendingAction': self.PENDING_DELETE,
                        'PendingReason': self.PENDING_DELETE_REASON,
                        'OldDefinition': old_resource_definition
                    }
                )

        # any changes to IAM resources are security related

        for resource_description in resource_descriptions.values():
            if resource_description.get('ResourceType').startswith('AWS::IAM::') and resource_description.get('PendingAction'):
                self.__set_pending_security_change(resource_description, 'Has a security related resource type.')

        return resource_descriptions


    def __diff_resource(
        self,
        logical_resource_name,
        resource_description,
        new_resource_definition,
        old_resource_definition,
        changed_reference_targets,
        new_content_paths
    ):

        update_reasons = []

        if old_resource_definition:
            # Ignore descriptiosn for which there are no definitions in the old template.
            # This can happen if the stack changes between when the resource descriptions
            # are read and the current template is read.
            self.__compare_resource_to_template(
                logical_resource_name,
                resource_description,
                new_resource_definition,
                old_resource_definition,
                changed_reference_targets,
                update_reasons
            )

        self.__compare_resource_to_content(
            resource_description.get('Timestamp'),
            new_content_paths.get(resource_description.get('ResourceType')),
            changed_reference_targets,
            update_reasons
        )

        # if there are changes, set status to update pending

        if update_reasons:
            resource_description.update(
                {
                    'PendingAction': self.PENDING_UPDATE,
                    'PendingReason': ' '.join(update_reasons)
                }
            )
            changed_reference_targets[logical_resource_name] = {
                'type': 'resource',
                'reasons': update_reasons
            }

        resource_description.update(
            {
                'OldDefinition': old_resource_definition,
                'NewDefinition': new_resource_definition
            }
        )


    def __determine_changed_parameters(self, new_template, old_template, new_parameter_values, old_parameter_values):

        new_parameter_definitions = new_template.get('Parameters', {})
        old_parameter_definitions = old_template.get('Parameters', {})

        old_parameters_seen = set()

        changed_parameters = {}

        # look for added and changed parameters...

        for parameter_name, new_parameter_definition in new_parameter_definitions.iteritems():

            if parameter_name in ['ConfigurationBucket', 'ConfigurationKey']:
                continue # always ignore these parameters

            new_value = new_parameter_values.get(parameter_name)
            if new_value is None:
                new_value = new_parameter_definition.get('Default')

            old_parameter_definition = old_parameter_definitions.get(parameter_name)
            if old_parameter_definition is None:

                # added parameter...

                changed_parameters[parameter_name] = {
                    'type': 'parameter',
                    'reasons': [ 'Parameter {} added (has value {}).'.format(parameter_name, new_value) ]
                }

            else:

                # look for changed value...

                old_value = old_parameter_values.get(parameter_name)
                if old_value is None:
                    old_value = old_parameter_definition.get('Default') if old_parameter_definition else None

                if new_value != old_value:
                    changed_parameters[parameter_name] = {
                        'type': 'parameter',
                        'reasons': [ 'Parameter {} value changed from {} to {}.'.format(parameter_name, old_value, new_value) ]
                    }

        # look for removed parameters

        for parameter_name, old_parameter_definition in old_parameter_definitions.iteritems():

            if new_parameter_definitions.has_key(parameter_name) or parameter_name == 'ConfigurationKey':
                continue

            old_value = old_parameter_values.get(parameter_name)
            if old_value is None:
                old_value = old_parameter_definition.get('Default')

            changed_parameters[parameter_name] = {
                'type': 'parameter',
                'reasons': [ 'Parameter {} removed (had value {}).'.format(parameter_name, old_value) ]
            }

        return changed_parameters


    def __compare_resource_to_template(
        self,
        logical_resource_name,
        resource_description,
        new_resource_definition,
        old_resource_definition,
        changed_reference_targets,
        update_reasons
    ):

        # look for type change

        old_type = old_resource_definition.get('Type')
        new_type = new_resource_definition.get('Type')
        if old_type != new_type:
            update_reasons.append('type changed from {} to {}.'.format(old_type, new_type))

        # look for property changes

        old_properties = old_resource_definition.get('Properties', {})
        new_properties = new_resource_definition.get('Properties', {})
        self.__compare_template_properties(old_properties, new_properties, changed_reference_targets, update_reasons)

        # look for metadata changes

        old_metadata = old_resource_definition.get('Metadata', {})
        new_metadata = new_resource_definition.get('Metadata', {})
        is_security_related_change = self.__compare_template_metadata(old_metadata, new_metadata, changed_reference_targets, update_reasons)
        if is_security_related_change:
            resource_description['IsPendingSecurityChange'] = True

    def __get_modification_time(self, path):
        return datetime.fromtimestamp(os.path.getmtime(path), tzlocal())

    def __get_root_relative_file_path(self, path):
        try:
            relative_path = os.path.relpath(path, self.context.config.root_directory_path)
            if relative_path.startswith('.'):
                return path
            else:
                return relative_path
        except:
            # Unable to convert to relative path.
            # For example, this happens when the given path is on a different drive than the root.
            return path

    def __compare_resource_to_content(self, resource_timestamp, new_content_paths, changed_reference_targets, update_reasons):

        if resource_timestamp is None or new_content_paths is None:
            return

        if not isinstance(new_content_paths, list):
            new_content_paths = [ new_content_paths ]

        for new_content_path in new_content_paths:

            if os.path.isdir(new_content_path):

                for root, directory_names, file_names in os.walk(new_content_path):
                    for file_name in file_names:
                        file_path = os.path.join(root, file_name)
                        new_content_timestamp = self.__get_modification_time(file_path)
                        if new_content_timestamp > resource_timestamp:
                            update_reasons.append('Content changed: {}'.format(self.__get_root_relative_file_path(file_path)))
                            return

            else:

                new_content_timestamp = self.__get_modification_time(new_content_path)
                if new_content_timestamp > resource_timestamp:
                    update_reasons.append('Content changed: {}'.format(self.__get_root_relative_file_path(new_content_path)))
                    return


    def __compare_template_properties(self, old_properties, new_properties, changed_reference_targets, update_reasons):

        for property_name, new_property_value in new_properties.iteritems():

            old_property_value = old_properties.get(property_name)

            if old_property_value is None:

                update_reasons.append('Property {} added.'.format(property_name))

            elif old_property_value != new_property_value:

                update_reasons.append('Property {} changed.'.format(property_name))

            else:

                impacted_references = self.__find_impacted_references(new_property_value, changed_reference_targets)
                if impacted_references:
                    update_reasons.append('Property {} references changed.'.format(property_name))

        for property_name, old_property_value in old_properties.iteritems():

            if property_name in new_properties.keys():
                continue

            update_reasons.append('Property {} removed.'.format(property_name))


    def __compare_template_metadata(self, old_metadata, new_metadata, changed_reference_targets, update_reasons, path=''):

        is_security_related_change = False

        for key, new_value in new_metadata.iteritems():

            full_path = path + key
            is_changed = False

            old_value = old_metadata.get(key)

            if old_value is None:

                update_reasons.append('Metadata {} added.'.format(full_path))
                is_changed = True

            elif isinstance(old_value, dict) and isinstance(new_value, dict):

                if self.__compare_template_metadata(old_value, new_value, changed_reference_targets, update_reasons, path = full_path + '.'):
                    is_security_related_change = True

            elif old_value != new_value:

                update_reasons.append('Metadata {} changed.'.format(full_path))
                is_changed = True

            else:

                impacted_references = self.__find_impacted_references(new_value, changed_reference_targets)
                if impacted_references:
                    update_reasons.append('Metadata {} references changed.'.format(full_path))

            if is_changed:
                if full_path.startswith('CloudCanvas.Permissions') or full_path.startswith('CloudCanvas.RoleMappings'):
                    is_security_related_change = True

        for key, old_value in old_metadata.iteritems():

            if key in new_metadata.keys():
                continue

            full_path = path + key

            update_reasons.append('Metadata {} removed.'.format(full_path))

            if full_path.startswith('CloudCanvas.Permissions') or full_path.startswith('CloudCanvas.RoleMappings'):
                is_security_related_change = True

        return is_security_related_change


    def __set_pending_security_change(self, resource_description, reason):

        pending_reason = resource_description.get('PendingReason', '')

        if pending_reason:
            pending_reason = pending_reason + ' '

        pending_reason = pending_reason + reason

        resource_description['PendingReason'] = pending_reason
        resource_description['IsPendingSecurityChange'] = True


    def __check_for_security_metadata(self, resource_description, resource_definition):

        metadata = resource_definition.get('Metadata', {}).get('CloudCanvas', {})

        permissions = metadata.get('Permissions')
        if permissions:
            self.__set_pending_security_change(resource_description, 'Has CloudCanvas.Permissions metadata.')

        role_mappings = metadata.get('RoleMappings')
        if role_mappings:
            self.__set_pending_security_change(resource_description, 'Has CloudCanvas.RoleMappings metadata.')


    def __find_impacted_references(self, target, changed_reference_targets):

        impacted_references = []

        if isinstance(target, dict):

            # { "Ref": "..." } for parameters and physical resource ids
            ref = target.get('Ref')
            if ref is not None:
                if ref in changed_reference_targets.keys():
                    impacted_references.append(ref)

            # { "Fn:GetAtt": [ "...", "..." ] } for resource outputs
            get_attr = target.get('Fn::GetAtt')
            if get_attr is not None:
                if isinstance(get_attr, list) or len(get_attr) < 1: # nothing else if valid
                    src = get_attr[0]
                    if src in changed_reference_targets.keys():
                        impacted_references.append(src)

            # look for nested references inside "Fn::Join", etc.
            for value in target.values():
                impacted_references.extend(
                    self.__find_impacted_references(value, changed_reference_targets)
                )

        elif isinstance(target, list):

            # look for nested references inside "Fn::Join", etc.
            for value in target:
                impacted_references.extend(
                    self.__find_impacted_references(value, changed_reference_targets)
                )

        return impacted_references


    def has_changed_or_deleted_resources(self,
        stack_id,
        stack_description,
        args,
        pending_resource_status,
        ignore_resource_types = []
    ):
        for resource_name, resource_description in pending_resource_status.iteritems():
            resource_type = resource_description.get('ResourceType', '')

            if resource_type in ignore_resource_types:
                continue

            if resource_type.startswith('AWS::IAM::'):
                are_iam_resources = True

            pending_action = resource_description.get('PendingAction')
            if pending_action:
                return True
        
        return False

    def confirm_stack_operation(
        self,
        stack_id,
        stack_description,
        args,
        pending_resource_status,
        ignore_resource_types = []
    ):

        capabilities = set()

        changed_resources = {}
        deleted_resources = {}
        are_deletions = False
        are_security_changes = False
        are_iam_resources = False

        for resource_name, resource_description in pending_resource_status.iteritems():

            resource_type = resource_description.get('ResourceType', '')

            if resource_type in ignore_resource_types:
                continue

            if resource_type.startswith('AWS::IAM::'):
                are_iam_resources = True

            pending_action = resource_description.get('PendingAction')
            if pending_action:
                changed_resources[resource_name] = resource_description
                if pending_action == self.PENDING_DELETE:
                    deleted_resources[resource_name] = resource_description
                    are_deletions = True
                elif resource_description.get('IsPendingSecurityChange'):
                    are_security_changes = True

        if not args.is_gui:
            self.context.view.stack_changes(stack_id, stack_description, changed_resources)

        # don't prompt if it is OK to use AWS when only doing deletes
        if not args.confirm_aws_usage and len(deleted_resources) != len(changed_resources):
            self.context.view.confirm_aws_usage()

        if are_deletions and not args.confirm_resource_deletion:
            self.context.view.confirm_stack_resource_deletion()

        if are_security_changes and not args.confirm_security_change:
            self.context.view.confirm_stack_security_change()

        if are_iam_resources:
            capabilities.add('CAPABILITY_IAM')

        return list(capabilities)


    def describe_stack(self, stack_id, optional = False):

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        self.context.view.describing_stack(stack_id)

        try:
            res = cf.describe_stacks(StackName=stack_id)
        except ClientError as e:
            if optional and e.response['Error']['Code'] == 'ValidationError':
                return None
            if e.response['Error']['Code'] == 'AccessDenied':
                return {
                    'StackStatus': 'UNKNOWN',
                    'StackStatusReason': 'Access denied.'
                }
            raise HandledError('Could not get stack {} description.'.format(stack_id), e)

        stack_description = res['Stacks'][0];
        return {
            'StackId': stack_description.get('StackId', None),
            'StackName': stack_description.get('StackName', None),
            'CreationTime': stack_description.get('CreationTime', None),
            'LastUpdatedTime': stack_description.get('LastUpdatedTime', None),
            'StackStatus': stack_description.get('StackStatus', None),
            'StackStatusReason': stack_description.get('StackStatusReason', None),
            'Outputs': stack_description.get('Outputs', None) }


    def get_current_template(self, stack_id):

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        self.context.view.getting_stack_template(stack_id)

        try:
            res = cf.get_template(StackName=stack_id)
        except ClientError as e:
            raise HandledError('Could not get stack {} template.'.format(stack_id), e)

        return res['TemplateBody']


    def get_current_parameters(self, stack_id):

        cf = self.context.aws.client('cloudformation', region=util.get_region_from_arn(stack_id))

        self.context.view.describing_stack(stack_id)

        try:
            res = cf.describe_stacks(StackName=stack_id)
        except ClientError as e:
            raise HandledError('Could not get stack {} description.'.format(stack_id), e)

        stack_description = res['Stacks'][0];
        parameter_list = stack_description['Parameters']
        parameter_map = { p['ParameterKey']:p['ParameterValue'] for p in parameter_list }

        return parameter_map



class Monitor(object):

    '''Reads and displays stack events until the end (sucessful or otherwise) of an operation is detected.'''

    def __init__(self, context, stack_id, operation):

        self.context = context
        self.stack_id = stack_id
        self.stack_name = util.get_stack_name_from_arn(stack_id)
        self.operation = operation
        self.events_seen = {}
        self.success_status = operation + '_COMPLETE'
        self.finished_status = [
            self.success_status,
            operation + '_FAILED',
            operation + '_ROLLBACK_COMPLETE',
            operation + '_ROLLBACK_FAILED',
            context.stack.STATUS_ROLLBACK_COMPLETE,
            context.stack.STATUS_ROLLBACK_FAILED
        ]
        self.client = self.context.aws.client('cloudformation', region=util.get_region_from_arn(self.stack_id))
        self.client.verbose = False
        self.start_nested_stack_status = [
            context.stack.STATUS_UPDATE_IN_PROGRESS,
            context.stack.STATUS_CREATE_IN_PROGRESS,
            context.stack.STATUS_DELETE_IN_PROGRESS
        ]
        self.end_nested_stack_status = [
            context.stack.STATUS_UPDATE_COMPLETE,
            context.stack.STATUS_UPDATE_FAILED,
            context.stack.STATUS_CREATE_COMPLETE,
            context.stack.STATUS_CREATE_FAILED,
            context.stack.STATUS_DELETE_COMPLETE,
            context.stack.STATUS_DELETE_FAILED,
            context.stack.STATUS_ROLLBACK_COMPLETE,
            context.stack.STATUS_ROLLBACK_FAILED
        ]
        self.monitored_stacks = [ stack_id ]

        if operation != 'CREATE':
            self.__load_existing_events()


    def __load_existing_events(self):
        self.__load_existing_events_for_stack(self.stack_id)

    def __load_existing_events_for_stack(self, stack_id):

        try:
            response = self.client.describe_stack_events(StackName = stack_id)
        except ClientError as e:
            raise HandledError('Could not get events for {0} stack.'.format(stack_id), e)

        for event in response['StackEvents']:
            self.events_seen[event['EventId']] = True

        response = self.client.describe_stack_resources(StackName=stack_id)
        for resource in response['StackResources']:
            if resource['ResourceType'] == 'AWS::CloudFormation::Stack':
                nested_stack_id = resource.get('PhysicalResourceId', None)
                if nested_stack_id is not None:
                    self.__load_existing_events_for_stack(nested_stack_id)


    def wait(self):

        '''Waits for the operation to complete, displaying events as they occur.'''

        errors = []

        done = False
        while not done:

            for monitored_stack_id in self.monitored_stacks:

                try:
                    res = self.client.describe_stack_events(StackName = monitored_stack_id)
                    stack_events = reversed(res['StackEvents'])
                except ClientError as e:
                    if e.response['Error']['Code'] == 'Throttling':
                        time.sleep(MONITOR_WAIT_SECONDS)
                        stack_events = []
                    else:
                        raise HandledError('Could not get events for {0} stack.'.format(self.stack_id), e)

                for event in stack_events:
                    if event['EventId'] not in self.events_seen:

                        resource_status = event.get('ResourceStatus', None)

                        self.events_seen[event['EventId']] = True

                        self.context.view.sack_event(event)

                        if resource_status.endswith('_FAILED'):
                            errors.append(
                                '{status} for {logical} ({type} with id "{physical}") - {reason}'.format(
                                    status = event.get('ResourceStatus', ''),
                                    type = event.get('ResourceType', 'unknown type'),
                                    logical = event.get('LogicalResourceId', 'unknown resource'),
                                    reason = event.get('ResourceStatusReason', 'No reason reported.'),
                                    physical = event.get('PhysicalResourceId', '{unknown}')
                                )
                            )

                        if event['StackId'] == self.stack_id:
                            if resource_status in self.finished_status and event['PhysicalResourceId'] == self.stack_id:
                                if errors:
                                    self.context.view.stack_event_errors(errors, resource_status == self.success_status);
                                if resource_status == self.success_status:
                                    done = True
                                    break
                                else:
                                    raise HandledError("The operation failed.")

                        if event['ResourceType'] == 'AWS::CloudFormation::Stack':
                            if resource_status in self.start_nested_stack_status and resource_status not in self.monitored_stacks:
                                if event['PhysicalResourceId'] is not None and event['PhysicalResourceId'] != '':
                                    self.monitored_stacks.append(event['PhysicalResourceId'])

                            if resource_status in self.end_nested_stack_status and resource_status in self.monitored_stacks:
                                self.monitored_stacks.remove(event['PhysicalResourceId'])

            if not done:
                time.sleep(MONITOR_WAIT_SECONDS) # seconds
