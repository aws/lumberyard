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
# $Revision: #2 $

import util
import dateutil
from errors import HandledError
import textwrap
from datetime import datetime
import json

def date_time_formatter(v):
    if isinstance(v, datetime):
        return v.strftime('%c')
    return '--'

class ViewContext(object):

    def __init__(self, context):
        self.context = context

    def bootstrap(self, args):
        self.__verbose = args.verbose
        self.__args = args

    def initialize(self, args):
        self.bootstrap(args)

    def _output(self, key, value):
        print value

    def _output_message(self, msg):
        self._output('message', msg)

    def importable_resource_list(self, importable_resources):
        self.__output_table(importable_resources,
            [
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'ARN', 'Heading': 'ARN' }
            ])

    def import_resource(self, name):
        self._output_message('{name} imported successfully'.format(name=name))

    def auto_added_resource(self, name):
        self._output_message('Added related resource with the name {name}'.format(name=name))

    def loading_file(self, path):
        if self.__verbose:
            self._output_message('Loading {}'.format(path))

    def saving_file(self, path):
        self._output_message('Saving {}'.format(path))

    def processing_swagger(self, path):
        self._output_message('\nProcessing swagger file: {}'.format(path))

    def uploading_content(self, bucket, key, description):
        self._output_message('Uploading {description} to bucket {bucket} object {key}.'.format(bucket=bucket, key=key, description=description))

    def uploading_file(self, bucket, key, path):
        self._output_message('Uploading {path} to bucket {bucket} object {key}'.format(path=path, bucket=bucket, key=key))

    def downloading_content(self, bucket, key):
        self._output_message('Downloading from bucket {bucket} object {key}'.format(bucket=bucket, key=key))

    def downloading(self, description):
        self._output_message('Downloading {}.'.format(description))

    def deleting_file(self, path):
        self._output_message('Deleting {}'.format(path))

    def creating_file(self, path):
        self._output_message('Creating {}'.format(path))

    def copying_file(self, source, destination):
        self._output_message('Creating {} from {}'.format(destination, source))

    def file_exists(self, path):
        self._output_message('WARNING: an {} file already exists. Not creating.'.format(path))
    
    def add_zip_content(self, description, destination):
        self._output_message('Adding {} to {}'.format(description, destination))

    def creating_directory(self, path):
        self._output_message('Creating {}'.format(path))

    def emptying_bucket(self, bucket):
         self._output_message('Deleting contents of bucket {}'.format(bucket))

    def deleting_bucket(self, bucket):
        self._output_message('Deleting bucket {}'.format(bucket))

    def creating_stack(self, stack_name):
        self._output_message('\nCreating stack {}'.format(stack_name))

    def updating_stack(self, stack_name, template_url, parameters):
        if self.__verbose:
            self._output_message('\nUpdating stack {} using template {} with parameters: {}'.format(stack_name, template_url, json.dumps(parameters, indent=4, sort_keys=True)))
        else:
            self._output_message('\nUpdating stack {} using template {}'.format(stack_name, template_url))

    def deleting_stack(self, stack_name, stack_id):
        self._output_message('\nDeleting stack {} ({})'.format(stack_name, stack_id))

    def processing_lambda_code(self, description, function_name):
        self._output_message('\nProcessing code for the {} {} Lambda function.'.format(description, function_name))

    def processing_template(self, description):
        self._output_message('\nProcessing the {} Cloud Formation template.'.format(description))

    def version_update(self, from_version, to_version, json):
        self._output_message('Converting local project settings from {} with a local project settings file defined by \n\'{}\'\n to {} format'.format(from_version, json, to_version))

    def version_update_complete(self, to_version, json):
        self._output_message('Local project settings for version {} is now \'{}\''.format(to_version, json))

    def sack_event(self, event):

        stack_name = event.get('StackName', '')
        resource_type = event.get('ResourceType', '')
        resource_name = event.get('LogicalResourceId', '')
        resource_status = event.get('ResourceStatus', '')
        resource_status_reason = event.get('ResourceStatusReason', None)

        msg = ''

        if 'ERROR' in resource_status or 'FAILED' in resource_status:
            msg += 'ERROR'
        else:
            msg += 'Stack'
            resource_status = resource_status.lower()

        resource_status = resource_status.replace('_', ' ')

        msg += ' \''
        msg += stack_name
        msg += '\''

        if stack_name != resource_name:
            msg += ' resource \''
            msg += resource_name
            msg += '\''

        msg += ' '
        msg += resource_status        

        if resource_status_reason is not None and resource_status_reason != 'User Initiated':
            msg += ': '
            msg += resource_status_reason

        self._output_message(msg)


    def stack_event_errors(self, errors, was_successful):
        if was_successful:
            self._output_message('\nThe following errors occured during the stack operation (but the operation completed successfully):\n')
        else:
            self._output_message('\nThe following errors occured during the stack operation:\n')
        for error in errors:
            self._output_message('    ' + error)


    def default_deployment(self, user_default, project_default):
        user_default = user_default or '(none)'
        project_default = project_default or '(none)'
        self._output_message('\nUser default:    {}\nProject default: {}\n'.format(user_default, project_default))


    def release_deployment(self, release_deployment_name):
        release_deployment_name = release_deployment_name or '(none)'
        self._output_message('\nRelease deployment: {}\n'.format(release_deployment_name))


    def retrieving_mappings(self, deployment_name, deployment_stack_id, role):
        self._output_message("Loading mappings for deployment '{}' with role '{}' from stack '{}'.".format(
            deployment_name,
            role,
            util.get_stack_name_from_arn(deployment_stack_id)))


    def this_may_take_a_while(self):
        self._output_message('\nThis operation may take a few minutes to complete...\n')

    def confirm_aws_usage(self):
        self.__confirm(
'''There is no additional charge for Cloud Canvas or CloudFormation. You pay for AWS resources
created using Cloud Canvas and CloudFormation in the same manner as if you created them manually.
You only pay for what you use, as you use it; there are no minimum fees and no required upfront
commitments, and most services include a free tier.

Learn more at https://docs.aws.amazon.com/lumberyard/latest/userguide/cloud-canvas-intro.html.''')

    def confirm_resource_deletion(self, resources, stack_description = None):
        '''Prompts the user to confirm that it is ok to delete the specified resources.

        The resources parameter should be a dictionary as returned by StackContext.describe_resources.
        '''
        list = sorted(map(lambda e: '{} - {} ({})'.format(e[0],e[1]['ResourceType'],e[1].get('PhysicalResourceId', 'none')), resources.iteritems()))
        if stack_description:
            prompt = 'The following resources will be deleted from the {} stack:\n\n\t{}'.format(stack_description, '\n\t'.join(list))
        else:
            prompt = 'The following resources will be deleted:\n\n\t{}'.format('\n\t'.join(list))

        self.__confirm(prompt, default_to_yes = False)

    def confirm_stack_resource_deletion(self):
        self.__confirm('This operation will permamently DELETE some of the resources listed above.', default_to_yes = False)

    def confirm_stack_security_change(self):
        self.__confirm('This operation may create or update the SECURITY configuration for some of the resources listed above.', default_to_yes = False)

    def confirm_file_replacements(self, file_paths):
        prompt = 'The contents of the following files will be replaced:\n\n\t{}'.format('\n\t'.join(file_paths))
        self.__confirm(prompt, default_to_yes = False)

    def __confirm(self, prompt = None, confirm_string = 'Is this OK', default_to_yes=True):
        '''Prompts the user to confirm an action and raises an error if they don't.'''

        yes_answers = ['y', 'yes']

        if default_to_yes:
            options = '[Y|n]'
            yes_answers.append('')
        else:
            options = '[y|N]'

        if prompt:
            prompt = '\n' + prompt
        else:
            prompt = ''

        if self.__args.no_prompt:

            self._output_message(prompt)
            raise HandledError('Needed confirmation not provided and --no-prompt was specified.')

        else:

            answer = raw_input('{}\n\n{} {}? '.format(prompt, confirm_string, options))
            print ''

            if answer.lower() not in yes_answers:
                raise HandledError('Needed confirmation not provided.')

            return True
        
    def confirm_writable_try_again(self, file_list):
        self._output_message('\nFiles not writable:\n\n\t{}'.format('\n\t'.join(file_list)))
        return self.__confirm(None,'Try again')

    def adding_resource(self, template_path, resource_name):
        self._output_message('Adding resource {} to template {}.'.format(resource_name, template_path))

    def resource_exists(self, template_path, resource_name):
        self._output_message('WARNING: an {} resource already exists in the template {}. Not adding.'.format(resource_name, template_path))

    def removing_resource(self, template_path, resource_name):
        self._output_message('Removing resource {} from template {}.'.format(resource_name, template_path))

    def resource_not_found(self, template_path, resource_name):
        self._output_message('WARNING: no {} resource was found in the template {}.'.format(resource_name, template_path))

    def adding_output(self, template_path, output_name):
        self._output_message('Adding output {} to template {}.'.format(output_name, template_path))

    def output_exists(self, template_path, output_name):
        self._output_message('WARNING: an {} output already exists in the template {}. Not adding.'.format(output_name, template_path))

    def removing_output(self, template_path, output_name):
        self._output_message('Removing output {} from template {}.'.format(output_name, template_path))

    def output_not_found(self, template_path, output_name):
        self._output_message('WARNING: no {} output was found in the template {}.'.format(output_name, template_path))

    def adding_parameter(self, template_path, parameter_name):
        self._output_message('Adding parameter {} to template {}.'.format(parameter_name, template_path))

    def parameter_exists(self, template_path, parameter_name):
        self._output_message('WARNING: an {} parameter already exists in the template {}. Not adding.'.format(parameter_name, template_path))

    def removing_parameter(self, template_path, parameter_name):
        self._output_message('Removing parameter {} from template {}.'.format(parameter_name, template_path))

    def parameter_not_found(self, template_path, parameter_name):
        self._output_message('WARNING: no {} parameter was found in the template {}.'.format(parameter_name, template_path))

    def resource_group_enabled(self, resource_group_name):
        self._output_message('\n{} resource group has been enabled. Use "lmbr_aws resource-group upload --resource-group {} --deployment DEPLOYMENT" to create the resource group\'s resources in AWS.'.format(resource_group_name, resource_group_name))

    def resource_group_disabled(self, resource_group_name):
        self._output_message('\n{} resource group has been disabled. Use "lmbr_aws resource-group upload --resource-group {} --deployment DEPLOYMENT" to delete the resource group\'s resouces from AWS.'.format(resource_group_name, resource_group_name))

    def deployment_stack_created(self, deployment_name, deployment_stack_id, deployment_access_stack_id):
        self._output_message('\n{} deployment stack {} and access stack {} have been created.'.format(
            deployment_name,
            util.get_stack_name_from_arn(deployment_stack_id),
            util.get_stack_name_from_arn(deployment_access_stack_id)))

    def deployment_stack_deleted(self, deployment_name, deployment_stack_id, deployment_access_stack_id):
        self._output_message('\n{} deployment stack {} and access stack {} have been deleted.'.format(
            deployment_name,
            util.get_stack_name_from_arn(deployment_stack_id),
            util.get_stack_name_from_arn(deployment_access_stack_id)))

    def describing_stack_resources(self, stack_id):
        if self.__verbose:
            self._output_message('Retrieving resource descriptions for stack {}.'.format(util.get_stack_name_from_arn(stack_id)))

    def describing_stack(self, stack_id):
        if self.__verbose:
            self._output_message('Retrieving description of stack {}.'.format(util.get_stack_name_from_arn(stack_id)))

    def mapping_list(self, deployment_name, mappings, protected):
        self._output_message('\nMapping Protected: {}'.format(protected))
        self._output_message('\nMappings for deployment {}:\n'.format(deployment_name))        
        self.__output_table(mappings,
            [
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'ResourceType', 'Heading': 'Type' },
                { 'Field': 'PhysicalResourceId', 'Heading': 'Id' }
            ])
    
    def path_list(self, mappings):        
        self._output_message('\nPath mappings for the Cloud Gem Framework:')        
        self.__output_table(mappings,
            [
                { 'Field': 'Type', 'Heading': 'Type' },
                { 'Field': 'Path', 'Heading': 'Path' },                
            ])

    def mapping_update(self, deployment_name, args):        
        self._output_message("Updating mappings for deployment '{}'. Release mode is set to '{}'".format(            
            deployment_name,                        
            args.release))
        

    def resource_group_list(self, deployment_name, resource_groups):

        if deployment_name is not None:
            self._output_message('\nResource groups for deployment {}:\n'.format(deployment_name))

        for resource_group in resource_groups:
            resource_group['Reason'] = self.__get_resource_reason(resource_group)

        self.__output_table(resource_groups,
            [
                { 'Field': 'PendingAction', 'Heading': 'Pending', 'HideWhenEmpty': True },
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'ResourceStatus', 'Heading': 'Status', 'Default': '--' },
                { 'Field': 'Timestamp', 'Heading': 'Timestamp', 'Formatter': date_time_formatter, 'Default': '--' },
                { 'Field': 'PhysicalResourceId', 'Heading': 'Id', 'Hidden': not self.__args.show_id, 'Default': '--' },
                { 'Field': 'Reason', 'Heading': 'Reason', 'HideWhenEmpty': True }
            ],
            first_sort_column = 1
        )

    def deployment_list(self, deployments):

        for deployment in deployments:
            deployment['Reason'] = self.__get_stack_reason(deployment)

        self.__output_table(deployments,
            [
                { 'Field': 'PendingAction', 'Heading': 'Pending', 'HideWhenEmpty': True },
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'StackStatus', 'Heading': 'Status', 'Default': '--' },
                { 'Field': 'Timestamp', 'Heading': 'Timestamp', 'Formatter': date_time_formatter, 'Default': '--' },
                { 'Field': 'Protected', 'Heading': 'Protected', 'Default': '--' },
                { 'Field': 'StackId', 'Heading': 'Id', 'Hidden': not self.__args.show_id, 'Default': '--' },
                { 'Field': 'Reason', 'Heading': 'Reason', 'HideWhenEmpty': True }
            ],
            first_sort_column = 1
        )

        def get_deployment_name(property_name):
            return next((deployment for deployment in deployments if deployment[property_name]), { 'Name': '(none)' })['Name']

        self._output_message('\n')
        self._output_message('User Default Deployment:    {}'.format(get_deployment_name('UserDefault')))
        self._output_message('Project Default Deployment: {}'.format(get_deployment_name('ProjectDefault')))
        self._output_message('Release Deployment:         {}'.format(get_deployment_name('Release')))


    def protected_deployment_list(self, deployment_names):
        self._output_message('Currently Protected Deployments:\n\n\t{}'.format('\n\t'.join(deployment_names)))


    def deprecated_resource_list(self, stack_id, resources_map):
        self._output_message('\nResources for stack {}:'.format(stack_id))
        self.__view_resource_list(resources_map)

    def project_resource_list(self, stack_id, resources_map):
        self._output_message('\nResources for project (stack {}):'.format(stack_id))
        self.__view_resource_list(resources_map)

    def deployment_resource_list(self, stack_id, deployment_name, resources_map):
        self._output_message('\nResources for deployment {} (stack {}):'.format(deployment_name, stack_id))
        self.__view_resource_list(resources_map)

    def resource_group_resource_list(self, stack_id, deployment_name, resource_group_name, resources_map):
        if deployment_name:
            self._output_message('\nResources for resource group {} in deployment {} (stack {}):'.format(resource_group_name, deployment_name, stack_id))
        else:
            self._output_message('\nResources for resource group {}:'.format(resource_group_name))
        self.__view_resource_list(resources_map)

    def __get_resource_reason(self, resource):
        return self.__get_reason(resource, 'ResourceStatusReason')

    def __get_stack_reason(self, resource):
        return self.__get_reason(resource, 'StackStatusReason')

    def __get_reason(self, resource, status_reason_property_name):
        status_reason = resource.get(status_reason_property_name)
        pending_reason = resource.get('PendingReason')
        if status_reason and pending_reason:
            return pending_reason + ' ' + status_reason
        elif status_reason:
            return status_reason
        elif pending_reason:
            return pending_reason
        else:
            return ''

    def __view_resource_list(self, resources_map):

        def security_formatter(v):
            if v:
                return 'Security'
            else:
                return ''

        resources_list = []
        for key, resource in resources_map.iteritems():
            resource['Name'] = key
            resource['Reason'] = self.__get_resource_reason(resource)
            resources_list.append(resource)

        self.__output_table(resources_list,
            [
                { 'Field': 'PendingAction', 'Heading': 'Pending', 'HideWhenEmpty': True },
                { 'Field': 'IsPendingSecurityChange', 'Heading': 'Impacts', 'Formatter': security_formatter, 'HideWhenEmpty': True },
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'ResourceType', 'Heading': 'Type' },
                { 'Field': 'ResourceStatus', 'Heading': 'Status', 'Default': '--' },
                { 'Field': 'Timestamp', 'Heading': 'Timestamp', 'Formatter': date_time_formatter, 'Default': '--' },
                { 'Field': 'PhysicalResourceId', 'Heading': 'Id', 'Hidden': not self.__args.show_id, 'Default': '--' },
                { 'Field': 'Reason', 'Heading': 'Reason', 'HideWhenEmpty': True }
            ],
            first_sort_column = 2
        )


    def stack_changes(self, stack_id, stack_description, change_map):
        
        if stack_id:
            full_stack_description = '{} stack ({})'.format(stack_description, util.get_stack_name_from_arn(stack_id))
        else:
            full_stack_description = '{} stack'.format(stack_description)

        self._output_message('\nThis operation will perform the following pending actions on the {} resources in AWS:'.format(full_stack_description))

        if change_map:
            self.__view_resource_list(change_map)
        else:
            self._output_message('\n(no changes detected)')

        print ''


    def profile_list(self, profiles, credentials_file_path):

        self._output_message('\nAWS Credentials from {}:'.format(credentials_file_path))

        self.__output_table(profiles,
            [
                { 'Field': 'Name', 'Heading': 'Name' },
                { 'Field': 'AccessKey', 'Heading': 'Access Key' },
                { 'Field': 'SecretKey', 'Heading': 'Secret Key' },
                { 'Field': 'Account', 'Heading': 'Account' },
                { 'Field': 'UserName', 'Heading': 'User Name' },
                { 'Field': 'Default', 'Heading': 'Default' }
            ])

        default_profile_name = next((profile for profile in profiles if profile['Default']), { 'Name': '(none)' })['Name']

        self._output_message('\nDefault Profile: {}'.format(default_profile_name))


    def added_profile(self, profile_name):
        self._output_message('\nAdded Profile: {}'.format(profile_name))


    def removed_profile(self, profile_name):
        self._output_message('\nRemoved Profile: {}'.format(profile_name))


    def removed_default_profile(self, profile_name):
        self._output_message('\nRemoved Profile: {}\nWarning: this was your default profile.'.format(profile_name))


    def updated_profile(self, profile_name):
        self._output_message('\nUpdated Profile: {}'.format(profile_name))

    def renamed_profile(self, old_name, new_name):
        self._output_message('\nRenamed profile {} to {}'.format(old_name, new_name))

    def default_profile(self, profile_name):
        if profile_name is None: profile_name = '(none)'
        self._output_message('\nDefault Profile: {}'.format(profile_name))

    def getting_stack_template(self, stack_id):
        if self.__verbose:
            self._output_message('Getting template for stack {}'.format(stack_id))

    def resource_group_stack_deleted(self, deployment_name, resource_group_name):
        self._output_message('\nDeleted {} resource group stack in {} deployment.'.format(resource_group_name, deployment_name))

    def resource_group_stack_created(self, deployment_name, resource_group_name):
        self._output_message('\nCreated {} resource group stack in {} deployment.'.format(resource_group_name, deployment_name))

    def clearing_project_stack_id(self, stack_id):
        self._output_message('\nThe stack {} does not exist in AWS. Removing from the project configuration.\n'.format(stack_id))

    def missing_gems_file(self, path):
        if self.__verbose:
            self._output_message('No Gem definitions for the project were found at {}'.format(path))

    def missing_bootstrap_cfg_file(self, path):
        self._output_message('Warning: a bootstrap.cfg file was not found at {}, using "Game" as the project directory name.'.format(path))

    def loaded_project_settings(self, settings):
        if self.__verbose: 
            self._output_message('Loaded project settings: {}'.format(json.dumps(settings, indent=4, sort_keys=True)))

    def saved_project_settings(self, settings):
        if self.__verbose: 
            self._output_message('Saved project settings: {}'.format(json.dumps(settings, indent=4, sort_keys=True)))

    def log_event(self, timestamp, message):
        self._output_message('{} {}'.format(timestamp, message))

    def parameter_list(self, parameter_list):
        self.__output_table(parameter_list, [
            { 'Field': 'deployment_name', 'Heading': 'Deployment' },
            { 'Field': 'resource_group_name', 'Heading': 'Resource Group' },
            { 'Field': 'parameter_name', 'Heading': 'Parameter' },
            { 'Field': 'parameter_value', 'Heading': 'Value' }
        ], sort_column_count = 3)

    def parameter_changed(self, deployment_name, resource_group_name, parameter_name, new_parameter_value, old_parameter_value):
        parameter_change_list = [
            {
                'deployment_name': deployment_name,
                'resource_group_name': resource_group_name,
                'parameter_name': parameter_name,
                'new_parameter_value': new_parameter_value,
                'old_parameter_value': old_parameter_value
            }
        ]
        self.__output_table(parameter_change_list, [
            { 'Field': 'deployment_name', 'Heading': 'Deployment' },
            { 'Field': 'resource_group_name', 'Heading': 'Resource Group' },
            { 'Field': 'parameter_name', 'Heading': 'Parameter' },
            { 'Field': 'old_parameter_value', 'Heading': 'Old Value', 'Default': '(none)' },
            { 'Field': 'new_parameter_value', 'Heading': 'New Value', 'Default': '(none)' }
        ])
        self._output_message('\n')

    def confirm_parameter_clear(self, change_list, confirmed):

        self._output_message("\nThe following parameter values will be deleted:")

        self.__output_table(change_list, [
            { 'Field': 'deployment_name', 'Heading': 'Deployment' },
            { 'Field': 'resource_group_name', 'Heading': 'Resource Group' },
            { 'Field': 'parameter_name', 'Heading': 'Parameter' },
            { 'Field': 'parameter_value', 'Heading': 'Value' }
        ], sort_column_count = 3, indent=True)

        if len(change_list) > 1:
            self.__confirm()
        else:
            self._output_message('\n')

        return True

    def no_such_deployment_parameter_warning(self, name):
        self._output_message('\nWARNING: The {} deployment does not exist. A parameter value will still be set.\n'.format(name))

    def no_such_resource_group_parameter_warning(self, name):
        self._output_message('\nWARNING: The {} resource group does not exist. A parameter value will still be set.\n'.format(name))

    def parameter_not_found(self, deployment_name, resource_group_name, parameter_name):
        if deployment_name and resource_group_name:
            self._output_message('\nWARNING: No {} parameter for the {} deployment and {} resource group was found.\n'.format(parameter_name, deployment_name, resource_group_name))
        elif deployment_name:
            self._output_message('\nWARNING: No {} parameter for the {} deployment was found.\n'.format(parameter_name, deployment_name))
        elif resource_group_name:
            self._output_message('\nWARNING: No {} parameter for the {} resource group was found.\n'.format(parameter_name, resource_group_name))
        else:
            self._output_message('\nWARNING: No {} parameter was found.\n'.format(parameter_name))

    def deleting_bucket_contents(self, stack_name, bucket_name, count, total):
        if total > count:
            self._output_message('Stack \'{}\' resource \'{}\' deleting {} objects ({} total)'.format(stack_name, bucket_name, count, total))
        else:
            self._output_message('Stack \'{}\' resource \'{}\' deleting {} objects'.format(stack_name, bucket_name, count))

    def using_role(self, deployment_name, logical_role_name, role_physical_name):
        if deployment_name:
            self._output_message('\nUsing deployment {} role {} ({})\n'.format(deployment_name, logical_role_name, role_physical_name))
        else:
            self._output_message('\nUsing project role {} ({})\n'.format(logical_role_name, role_physical_name))

    def using_profile(self, profile_name):
        self._output_message('\nUsing profile {}.\n'.format(profile_name))

    def role_added(self, scope, role):
        self._output_message('\nAdded {} role {}.\n'.format(scope, role))

    def role_removed(self, scope, role):
        self._output_message('\nRemoved {} role {}.\n'.format(scope, role))

    def role_list(self, role_list):
        self.__output_table(role_list, [
            { 'Field': 'Scope', 'Heading': 'Scope' },
            { 'Field': 'Name', 'Heading': 'Name' }
        ], sort_column_count = 2)
                                                      
    def role_mapping_added(self, scope, role, abstract_role_pattern):
        self._output_message('\nAdded {} role {} mapping for {}.\n'.format(scope, role, abstract_role_pattern))

    def role_mapping_removed(self, scope, role, abstract_role_pattern):
        self._output_message('\nRemoved {} role {} mapping for {}.\n'.format(scope, role, abstract_role_pattern))

    def role_mapping_list(self, role_list):
        self.__output_table(role_list, [
            { 'Field': 'Scope', 'Heading': 'Scope' },
            { 'Field': 'Role', 'Heading': 'Actual Role' },
            { 'Field': 'Pattern', 'Heading': 'Abstract Role' },
            { 'Field': 'Effect', 'Heading': 'Effect' }
        ], sort_column_count = 3)
                                                      
    def permission_added(self, resource_group, resource, abstract_role):
        self._output_message(
            '\nAdded {} resource {} permission for role {}.\n'.format(
                resource_group, 
                resource, 
                abstract_role
            )
        )

    def access_control_dependency_changed(self, resource_group, resource):
        self._output_message(
            '\nAdded {} resource AccessControl dependency on resource {}.\n'.format(
                resource_group, 
                resource
            )
        )

    def permission_removed(self, resource_group, resource, abstract_role):
        self._output_message(
            '\nRemoved {} resource {} permission for role {}.\n'.format(
                resource_group, 
                resource, 
                abstract_role
            )
        )

    def permission_list(self, permission_list):
        self.__output_table(permission_list, [
            { 'Field': 'ResourceGroup', 'Heading': 'Resource Group' },
            { 'Field': 'ResourceName', 'Heading': 'Resource' },
            { 'Field': 'ResourceType', 'Heading': 'Resource Type' },
            { 'Field': 'Roles', 'Heading': 'Roles' },
            { 'Field': 'Actions', 'Heading': 'Actions' },
            { 'Field': 'Suffixes', 'Heading': 'ARN Suffixes' }
        ], sort_column_count = 3)

    def calling_hook(self, module, handler_name):
        if self.__verbose:
            self._output_message('Calling hook function {} in module {}.'.format(handler_name, module.__file__))

    def calling_deprecated_hook(self, module, handler_name):
        self._output_message('WARNING: calling deprecated hook function {} in module {}.'.format(handler_name, module.__file__))

    def using_deprecated_lambda_code_path(self, function_name, path, prefered_path):
        self._output_message('WARNING: using deprecated Lambda Function code directory path naming convention for {} at {}. The prefered path is {}.'.format(function_name, path, prefered_path))

    def invalid_user_default_deployment_clearing(self, name):
        self._output_message('\nWARNING: The {} user default deployment is invalid.  Setting default to none.\n'.format(name))
        
    def invalid_project_default_deployment_clearing(self, name):
        self._output_message('\nWARNING: The {} project default deployment is invalid.  Setting default to none.\n'.format(name))    

    def updating_framework_version(self, from_version, to_version):
        self._output_message('Updating the CloudGemFramework used by the project from version {} to {}.'.format(from_version, to_version))

    def executing_subprocess(self, args):
        self._output_message('\nExecuting: {}'.format(' '.join(args)))

    def executed_subprocess(self, args):
        self._output_message('(end external execution)')

    def copying_initial_gem_content(self, content_name):
        self._output_message('\nCopying initial gem content: {}'.format(content_name))

    def gem_created(self, gem_name, directory_path):
        self._output_message('\nGem {} has been created in {}.'.format(gem_name, directory_path))

    def gem_enabled(self, gem_name):
        self._output_message('\nGem {} has been enabled.'.format(gem_name))

    def gem_disabled(self, gem_name):
        self._output_message('\nGem {} has been disabled.'.format(gem_name))

    def backing_up_file(self, origional_file_path, backup_file_path):
        self._output_message('Backing up {} to {}.'.format(origional_file_path, backing_up_file)) 

    def using_deprecated_command(self, old, new):
        if isinstance(new, list):
            # ['a', 'b', 'c'] --> '"a", "b", or "c"'
            new_msg = ', '.join([ '"' + i + '"' for i in new[:-1]]) + ', or ' + '"' + new[-1:][0] + '"'
            self._output_message('\nWARNING: The "{}" command has been deprecated. It still works, but its behavior may have changed. You can use the {} commands instead.\n'.format(old, new_msg))
        else:
            self._output_message('\nWARNING: The "{}" command has been deprecated. It still works, but its behavior may have changed. You can use the "{}" command instead.\n'.format(old, new))

    def __output_table(self, items, specs, sort_column_count = 1, indent = False, first_sort_column=0):

        ''' Displays a table containing data from items formatted as defined by specs.

        items is an array of dict. The properties shown are determined by specs.

        specs is an array of dict with the following properties:

            Field -- Identifies a property in an item. Required.
            Heading -- The heading that is displayed. Required.
            Default -- A default value for the property. Defaults to ''
            Formatter -- A function that is called to format the property value or the default value.
            Hidden -- If present and True, the column is not displayed.
            HideWhenEmpty -- If present and True, the column is not displayed if there are no values.

        The columns are arranged in the order of the specs. The column widths are automatically determiend.

        The items are sorted in ascending order by the formatted value of the first n columns, where n
        is specified by the sort_column_count parameter (which defaults to 1, causing the the table to 
        be sorted by the first column only).

        '''

        def default_formatter(v):
            return str(v) if v is not None else ''

        def get_formatted_value(item, spec):
            field = spec['Field']
            formatter = spec.get('Formatter', default_formatter)
            default = spec.get('Default', None)
            return formatter(item.get(field, default))

        # For simplicity we generate the formatted value multiple times. If this
        # ends up being used to display large tables this may need to be changed.
        # We sort working up to the first column and python guarnetees that a
        # stable sort is used, so things work out how we want.

        for sort_column in range((sort_column_count+first_sort_column)-1, first_sort_column-1, -1):
            items = sorted(items, key=lambda item: get_formatted_value(item, specs[sort_column]))

        # determine width of each column

        lengths = {}

        for item in items:
            for spec in specs:
                field = spec['Field']
                lengths[field] = max(lengths.get(field, 0), len(get_formatted_value(item, spec)))

        def is_hidden(spec):
            return spec.get('Hidden', False) or (spec.get('HideWhenEmpty', False) and lengths.get(spec['Field'], 0) == 0)
        
        specs = [ spec for spec in specs if not is_hidden(spec) ]

        for spec in specs:
            field = spec['Field']
            lengths[field] = max(lengths.get(field, 0), len(spec['Heading']))

        # determine the prefix for each line

        if indent:
            prefix = '    '
        else:
            prefix = ''

        # show the headings

        heading = '\n'
        heading += prefix
        for spec in specs:
            heading += '{0:{1}}  '.format(spec['Heading'], lengths[spec['Field']])
        self._output_message(heading)

        # show a dividing line under the headings

        divider = prefix
        for spec in specs:
            divider += ('-' * lengths[spec['Field']]) + '  '
        self._output_message(divider)

        # show the items

        for item in items:
            line = prefix
            for spec in specs:
                formatted_value = get_formatted_value(item, spec)
                line += '{0:{1}}  '.format(formatted_value, lengths[spec['Field']])
            self._output_message(line)
