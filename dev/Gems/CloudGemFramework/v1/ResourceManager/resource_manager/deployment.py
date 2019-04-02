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

from errors import HandledError
import util
import copy
import json
import time

import resource_group
import mappings
import project
import stack

from botocore.exceptions import NoCredentialsError

from uploader import ProjectUploader, Phase, Uploader
from resource_manager_common import constant
from resource_manager_common import stack_info


PENDING_CREATE_REASON = 'The deployment''s resource group defined resources have not yet been created in AWS.'
ACCESS_PENDING_CREATE_REASON = 'The deployment''s access control resources have not been created in AWS.'
CONFIGURATION_SUFFIX = 'Configuration'
CONFIGURATION_KEY_SUFFIX = 'ConfigurationKey'


def create_stack(context, args):

    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    # Does a deployment with that name already exist?
    if context.config.deployment_stack_exists(args.deployment):
        raise HandledError('The project already has a {} deployment.'.format(args.deployment))

    # Does deployment-template.json include resource group from a gem which isn't enabled for the project?
    for resource_group_name in context.resource_groups.keys():
         __check_resource_group_gem_status(context, resource_group_name)

    # Is the deployment name valid?
    util.validate_stack_name(args.deployment)

    # If there is no project default deployment, make this the project default deployment
    if context.config.project_default_deployment is None:
        args.make_project_default = True

    # If there is no release deployment, make this the release deployment
    if context.config.release_deployment is None:
        args.make_release_deployment = True

    # Need to handle situations where the deployment and/or access stack were
    # not successfully created on previous attempts.

    pending_deployment_stack_id = context.config.get_pending_deployment_stack_id(args.deployment)
    pending_deployment_access_stack_id = context.config.get_pending_deployment_access_stack_id(args.deployment)

    pending_deployment_stack_status = context.stack.get_stack_status(pending_deployment_stack_id)
    pending_deployment_access_stack_status = context.stack.get_stack_status(pending_deployment_access_stack_id)

    if args.tags:
        add_tags(context, args.deployment, args.tags)

    # Does a stack with the name already exist? It's ok if a previous attempt
    # at creation left a stack with this name behind, we'll deal with that later.
    deployment_stack_name = args.stack_name or context.config.get_default_deployment_stack_name(args.deployment)
    deployment_region = util.get_region_from_arn(context.config.project_stack_id)
    if pending_deployment_stack_id is None or deployment_stack_name != util.get_stack_name_from_arn(pending_deployment_stack_id):
        if context.stack.name_exists(deployment_stack_name, deployment_region):
            raise HandledError('An AWS Cloud Formation stack with the name {} already exists in region {}. Use the --stack-name option to provide a different name.'.format(deployment_stack_name, deployment_region))

    # Resource group (and other) file write checks
    create_and_validate_writable_list(context)

    # Is it ok to use AWS?

    pending_resource_status = __get_pending_combined_resource_status(context, args.deployment)

    __check_custom_definitions(context, pending_resource_status)

    capabilities = context.stack.confirm_stack_operation(
        None, # stack id
        'deployment {}'.format(args.deployment),
        args,
        pending_resource_status,
        ignore_resource_types = [ 'Custom::EmptyDeployment' ]
    )

    # We have the following scenerios to deal with:
    #
    # 1) This is the first attempt to create the deployment, or previous attempts didn't
    #    get as far as creating any stacks.
    #
    # 2) The previous attempt failed to create or update the deployment stack, which was
    # left in a ROLLBACK_COMPLETED, UPDATE_ROLLBACK_FAILED, or ROLLBACK_FAILED state. This
    # stack must be deleted and a new one created.
    #
    # 3) The previous attempt created the deployment stack but failed to create the access
    # stack, leaving it in the ROLLBACK_COMPLETED state. In this case we update the deployment
    # stack (to make sure it reflects any changes that may have been made), delete the access
    # stack and attempt to create a new one.
    #
    # 4) Both the deployment and access stacks were created successfully, but the pending
    # stack id properites in the config were not replaced with the non-pending properties
    # (this could happen if someone kills the client during the access stack creation
    # process, which then runs to a successful completion). In this case we update both
    # stacks to make sure they reflect any changes, then replace the "pending" stack id
    # properties.

    project_uploader = ProjectUploader(context)
    deployment_uploader = project_uploader.get_deployment_uploader(args.deployment)

    template_url = before_update(context, deployment_uploader)

    deployment_stack_parameters = __get_deployment_stack_parameters(context, args.deployment, uploader = deployment_uploader)

    for resource_group in context.resource_groups.values():
        if resource_group.is_enabled:
            deployment_stack_parameters[resource_group.name + CONFIGURATION_KEY_SUFFIX] = deployment_stack_parameters[CONFIGURATION_KEY_SUFFIX]

    # wait a bit for S3 to help insure that templates can be read by cloud formation
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    try:
        if pending_deployment_stack_status not in [None,
                                                   context.stack.STATUS_UPDATE_ROLLBACK_COMPLETE,
                                                   context.stack.STATUS_ROLLBACK_COMPLETE,
                                                   context.stack.STATUS_DELETE_COMPLETE,
                                                   context.stack.STATUS_UPDATE_ROLLBACK_FAILED,
                                                   context.stack.STATUS_ROLLBACK_FAILED]:

            # case 3 or 4 - deployment stack was previously created successfully, update it

            context.stack.update(
                pending_deployment_stack_id,
                template_url,
                deployment_stack_parameters,
                capabilities = capabilities
            )
            deployment_stack_id = pending_deployment_stack_id

        else:
            if pending_deployment_stack_status in [context.stack.STATUS_ROLLBACK_COMPLETE,
                                                   context.stack.STATUS_ROLLBACK_FAILED,
                                                   context.stack.STATUS_UPDATE_ROLLBACK_FAILED]:
                # case 2 - deployment stack failed to create previously, delete it

                context.stack.delete(pending_deployment_stack_id)

            effective_template = context.config.deployment_template_aggregator.effective_template
            is_empty_deployment = 'EmptyDeployment' in effective_template['Resources']

            if args.parallel or is_empty_deployment:

                # case 1 and 2 - deployment stack wasn't creatred previously or was just
                # deleted, attempt to create it

                deployment_stack_id = context.stack.create_using_url(
                    deployment_stack_name,
                    template_url,
                    deployment_region,
                    deployment_stack_parameters,
                    created_callback=lambda id: context.config.set_pending_deployment_stack_id(args.deployment, id),
                    capabilities=capabilities)

            else:
                # Do a rolling create
                template_body = copy.deepcopy(effective_template)
                saved_resources = template_body['Resources']

                if pending_deployment_stack_status == context.stack.STATUS_UPDATE_ROLLBACK_COMPLETE:
                    # Resuming partial completion of a deployment stack.
                    existing_template_body = context.stack.get_current_template(pending_deployment_stack_id)
                    template_resources = existing_template_body['Resources']
                    template_body['Resources'] = template_resources
                    resource_group_names = sorted([key for key, group in context.resource_groups if
                                                   group.is_enabled and key not in template_resources])
                    deployment_stack_id = pending_deployment_stack_id

                else:
                    # Remove resources from it so that we can add them back individually
                    template_resources = {}
                    template_body['Resources'] = template_resources
                    resource_group_names = sorted([key for key, group in context.resource_groups if group.is_enabled])

                    # Can't have an empty resource template so start with the first resource group in our deployment
                    template_resources[resource_group_names[0]] = saved_resources[resource_group_names[0]]
                    template_resources[resource_group_names[0] + "Configuration"] = saved_resources[
                        resource_group_names[0] + "Configuration"]
                    del resource_group_names[0]

                    # Create the initial stack with only one resource group
                    deployment_stack_id = context.stack.create_using_template(
                        deployment_stack_name,
                        json.dumps(template_body, indent=4, sort_keys=True),
                        deployment_region,
                        parameters=deployment_stack_parameters,
                        created_callback=lambda id: context.config.set_pending_deployment_stack_id(args.deployment, id),
                        capabilities=capabilities,
                        throw_failed_resources=True
                    )

                # Iterate over the remaining resource groups and add them to the stack
                for resource_group_name in resource_group_names:
                    template_resources[resource_group_name] = saved_resources[resource_group_name]
                    template_resources[resource_group_name + "Configuration"] = saved_resources[resource_group_name + "Configuration"]

                    context.stack.update(
                        deployment_stack_id,
                        None,
                        parameters=deployment_stack_parameters,
                        capabilities=capabilities,
                        template_body=json.dumps(template_body, indent=4, sort_keys=True),
                        throw_failed_resources=True
                    )

                # Add the cross-gem resolver last, if it is required
                cross_gem_resolver = saved_resources.get(constant.CROSS_GEM_RESOLVER_KEY, None)
                if cross_gem_resolver:
                    template_resources[constant.CROSS_GEM_RESOLVER_KEY] = cross_gem_resolver
                    context.stack.update(
                        deployment_stack_id,
                        None,
                        parameters=deployment_stack_parameters,
                        capabilities=capabilities,
                        template_body=json.dumps(template_body, indent=4, sort_keys=True),
                        throw_failed_resources=True
                    )

        # Now create or update the access stack...

        context.view.processing_template('{} deployment'.format(args.deployment))

        access_template_url = deployment_uploader.upload_content(
            constant.DEPLOYMENT_ACCESS_TEMPLATE_FILENAME,
            json.dumps(context.config.deployment_access_template_aggregator.effective_template, indent=4, sort_keys=True),
            'processed deployment access temmplate')

        access_stack_parameters = __get_access_stack_parameters(
            context,
            args.deployment,
            deployment_stack_id = deployment_stack_id,
            uploader = deployment_uploader
        )

        if pending_deployment_access_stack_status not in [None, context.stack.STATUS_ROLLBACK_COMPLETE, context.stack.STATUS_DELETE_COMPLETE]:

            # case 4 - access stack was previously created successfully but the pending
            # stack id properties were not replaced. Update the stack.

            context.stack.update(
                pending_deployment_access_stack_id,
                access_template_url,
                deployment_stack_parameters,
                capabilities = capabilities
            )

            deployment_access_stack_id = pending_deployment_access_stack_id

        else:

            if pending_deployment_access_stack_status == context.stack.STATUS_ROLLBACK_COMPLETE:

                # case 3 - access stack failed to create previously, delete it

                context.stack.delete(pending_deployment_access_stack_id)

            # case 1 or 3 - access stack wasn't created before, or was just deleted. Attempt
            # to create.

            deployment_access_stack_name = deployment_stack_name + '-Access'

            deployment_access_stack_id = context.stack.create_using_url(
                deployment_access_stack_name,
                access_template_url,
                deployment_region,
                parameters = access_stack_parameters,
                created_callback=lambda id: context.config.set_pending_deployment_access_stack_id(args.deployment, id),
                capabilities = capabilities)

    except:
        context.config.force_gui_refresh()
        raise

    context.config.force_gui_refresh()

    context.config.finalize_deployment_stack_ids(args.deployment)

    context.view.deployment_stack_created(args.deployment, deployment_stack_id, deployment_access_stack_id)

    for resource_group in context.resource_groups.values():
        if resource_group.is_enabled:
            resource_group.record_lambda_language_usage(context)

    # Should the new deployment become the project default deployment or the release deployment?
    updated_mappings = False
    if args.make_project_default:
        context.config.set_project_default_deployment(args.deployment)
        context.view.default_deployment(context.config.user_default_deployment, context.config.project_default_deployment)
        __update_mappings(context, args.deployment)
        updated_mappings = True

    if args.make_release_deployment:
        __set_release_deployment(context, args.deployment)
        context.view.release_deployment(context.config.release_deployment)
        # We don't need to get mappings twice if this is both default and release.
        if not updated_mappings:
            __update_mappings(context, args.deployment)

    after_update(context, deployment_uploader)

def __set_release_deployment(context, deployment):
    context.config.set_release_deployment(deployment)
    if deployment is None:
        mappings.set_launcher_deployment(context, context.config.default_deployment)
    else:
        mappings.set_launcher_deployment(context, deployment)


def delete_stack(context, args):

    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    if not args.deployment in context.config.deployment_names:
        raise HandledError('The project has no {} deployment.'.format(args.deployment))

    deployment_stack_id = _get_effective_deployment_stack_id(context, args.deployment)
    if not context.stack.id_exists(deployment_stack_id):
        deployment_stack_id = None

    deployment_access_stack_id = _get_effective_access_stack_id(context, args.deployment)
    if not context.stack.id_exists(deployment_access_stack_id):
        deployment_access_stack_id = None

    if not args.confirm_resource_deletion and (deployment_stack_id is not None or deployment_access_stack_id is not None):
        descriptions = {}
        if deployment_stack_id is not None:
            descriptions.update(context.stack.describe_resources(deployment_stack_id, recursive=True))
        if deployment_access_stack_id is not None:
            access_descriptions = context.stack.describe_resources(deployment_access_stack_id, recursive=True)
            descriptions.update({'Access.' + k:v for k,v in access_descriptions.iteritems()})
        context.view.confirm_resource_deletion(descriptions, '{} deployment and access stacks'.format(args.deployment))

    if deployment_access_stack_id is not None:
        context.stack.delete(deployment_access_stack_id)

    if deployment_stack_id is not None:
        try:
            if not args.parallel:
                # Do a rolling delete of resources in the deployment
                old_template = context.stack.get_current_template(deployment_stack_id)
                old_params = context.stack.get_current_parameters(deployment_stack_id)
                old_resources = old_template['Resources']
                remove_stacks = [k for k, v in old_resources.iteritems() if v['Type'] == "AWS::CloudFormation::Stack"]

                # Remove the cross gem resolver first, if it exists
                old_resources.pop(constant.CROSS_GEM_RESOLVER_KEY, None)

                # Remove the resource groups one at a time
                for remove_stack in remove_stacks[:-1]:  # Can't have an empty stack, so leave one resource group present
                    del old_resources[remove_stack]
                    del old_resources[remove_stack + "Configuration"]

                    context.stack.update(
                        deployment_stack_id,
                        None,
                        parameters=old_params,
                        capabilities=['CAPABILITY_IAM'],
                        template_body=json.dumps(old_template, indent=4, sort_keys=True),
                        throw_failed_resources=True,
                        pending_resource_status=context.stack.get_pending_resource_status(
                            deployment_stack_id,
                            new_template=old_template,
                            new_parameter_values=old_params
                        )
                    )

            context.stack.delete(deployment_stack_id)

        except stack.StackOperationException:
            pass

    context.config.remove_deployment(args.deployment)

    # Wait a bit for S3 to help insure that the updated project-settings.json object is available
    # This is only an issue when test scripts delete deployments and projects in rapid succession.
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    context.view.deployment_stack_deleted(args.deployment, deployment_stack_id, deployment_access_stack_id)


def default(context, args):

    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    if args.clear:
        if args.project:
            context.config.set_project_default_deployment(None)
        else:
            context.config.set_user_default_deployment(None)

    if args.set:
        if args.project:
            context.config.set_project_default_deployment(args.set)
        else:
            context.config.set_user_default_deployment(args.set)
    
    mappings.update(context, args)

    context.view.default_deployment(context.config.user_default_deployment, context.config.project_default_deployment)


def protect(context, args):
    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    if args.set:
        context.config.protect_deployment(args.set)

    if args.clear:
        context.config.unprotect_deployment(args.clear)

    context.view.protected_deployment_list(context.config.get_protected_depolyment_names())


def release(context, args):
    # Has the project been initialized?
    if not context.config.project_initialized:
        raise HandledError('The project has not been initialized.')

    old_release_deployment_name = context.config.release_deployment

    if args.clear:
        __set_release_deployment(context, None)

    if args.set:
        __set_release_deployment(context, args.set)

    # update mappings if the release deployment changed
    if old_release_deployment_name != context.config.release_deployment:
        args.release = True
        mappings.update(context, args)

    context.view.release_deployment(context.config.release_deployment)


def upload_resources(context, args):

    # call deployment.update_stack, resoure_group.update_stack, resoure_group.create_stack,
    # or resoure_group.delete_stack as needed

    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError('No deployment was specified and there is no default deployment configured.')
        args.deployment = context.config.default_deployment

    if not context.config.deployment_stack_exists(args.deployment):
        # This could be a pending deployment which failed creation -
        # this should be handled in create_stack
        if context.config.get_pending_deployment_stack_id(args.deployment):
            return create_stack(context, args)
        raise HandledError('There is no {} deployment stack.'.format(args.deployment))

    if args.resource_group is not None:
        __resource_group_upload(context, args)
    else:
        update_stack(context, args)

def __resource_group_upload(context, args):
    # is the resource group from a gem which isn't enabled for the project?
    __check_resource_group_gem_status(context, args.resource_group)

    stack_id = context.config.get_resource_group_stack_id(
        args.deployment, args.resource_group, optional=True)
    if args.resource_group in context.resource_groups:
        context.config.aggregate_settings = {}
        for group in context.resource_groups.values():
            if group.is_enabled:
                group.add_aggregate_settings(context)

        group = context.resource_groups.get(args.resource_group)
        if stack_id is None:
            if group.is_enabled:
                resource_group.create_stack(context, args)
            else:
                raise HandledError(
                    'The {} resource group is disabled and no stack exists.'.format(group.name))
        else:
            if group.is_enabled:
                resource_group.update_stack(context, args)
            else:
                resource_group.delete_stack(context, args)
    else:
        if stack_id is None:
            raise HandledError(
                'There is no {} resource group.'.format(args.resource_group))
        resource_group.delete_stack(context, args)

    __update_mappings(context, args.deployment, True)


def update_stack(context, args):

    # Use default deployment if necessary

    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError('No default deployment has been set. Provide the --deployment parameter or use the default-deployment command to set a default deployment.')
        args.deployment = context.config.default_deployment

    # Does deployment-template.json include resource group from a gem which isn't enabled for the project?
    for resource_group_name in context.resource_groups.keys():
         __check_resource_group_gem_status(context, resource_group_name)

    # Resource group (and other) file write checks
    create_and_validate_writable_list(context)

    # Get necessary data, verifies project has been initialized and that the stack exists.

    deployment_stack_id = context.config.get_deployment_stack_id(args.deployment)
    pending_resource_status = __get_pending_deployment_resource_status(context, args.deployment)

    __check_custom_definitions(context, pending_resource_status)

    has_changes = context.stack.has_changed_or_deleted_resources(
        deployment_stack_id,
        'deployment {}'.format(args.deployment),
        args,
        pending_resource_status,
        ignore_resource_types = [ 'Custom::EmptyDeployment' ],
        only_resource_types = []
    )

    # Is it ok to do this?
    capabilities = context.stack.confirm_stack_operation(
        deployment_stack_id,
        'deployment {}'.format(args.deployment),
        args,
        pending_resource_status,
        ignore_resource_types = [ 'Custom::EmptyDeployment' ]
    )

    # Do the upload ...

    project_uploader = ProjectUploader(context)
    deployment_uploader = project_uploader.get_deployment_uploader(args.deployment)

    deployment_template_url = before_update(context, deployment_uploader)

    parameters = __get_deployment_stack_parameters(context, args.deployment, uploader = deployment_uploader)
    enabled_resource_groups = [resource_group for resource_group in context.resource_groups.values() if resource_group.is_enabled]
    configuration_key = parameters[CONFIGURATION_KEY_SUFFIX]
    parameters.update(
        {resource_group.name + CONFIGURATION_KEY_SUFFIX: None if parameters.get(resource_group.name + CONFIGURATION_KEY_SUFFIX) else configuration_key for resource_group in enabled_resource_groups})

    # wait a bit for S3 to help insure that templates can be read by cloud formation
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    if args.parallel:
        # Update all configuration keys at once
        parameters.update({resource_group.name + CONFIGURATION_KEY_SUFFIX: configuration_key for resource_group in enabled_resource_groups})
        context.stack.update(
            deployment_stack_id,
            deployment_template_url,
            parameters,
            pending_resource_status = pending_resource_status,
            capabilities = capabilities
        )

    else:
        # Do a rolling update.
        #
        # Find out which resource groups are being created, updated or deleted.
        pending_resource_status = context.stack.get_pending_resource_status(
            deployment_stack_id,
            new_template=context.config.deployment_template_aggregator.effective_template,
            new_parameter_values=parameters
        )

        old_template = context.stack.get_current_template(deployment_stack_id)
        old_resources = old_template['Resources']
        old_parameter_defs = old_template['Parameters']

        new_template = copy.deepcopy(context.config.deployment_template_aggregator.effective_template)
        saved_resources = copy.deepcopy(new_template['Resources'])
        saved_parameter_defs = copy.deepcopy(new_template['Parameters'])
        new_resources = new_template['Resources']
        new_parameter_defs = new_template['Parameters']

        stack_resources = {k: v for k, v in pending_resource_status.iteritems() if v['ResourceType'] == "AWS::CloudFormation::Stack"}
        config_resources = {k + "Configuration": pending_resource_status[k + "Configuration"] for k in stack_resources.keys()}
        create_resources = {}
        update_resources = {}
        delete_resources = {}
        stack_op_mapping = {
            context.stack.PENDING_CREATE: create_resources,
            context.stack.PENDING_UPDATE: update_resources,
            context.stack.PENDING_DELETE: delete_resources
        }

        # Iterate through all of our stack resources and attempt to discern if they are being created, updated or
        # deleted. (Sometimes the PendingAction exists on the main stack, sometimes it exists on the Configuration.)
        for k, v in stack_resources.iteritems():
            pending_action = v.get('PendingAction', None) or \
                             config_resources[k + "Configuration"].get('PendingAction', None)
            if pending_action:
                stack_op_mapping.get(pending_action, {})[k] = v

        # Do not add created resources to the template yet
        for k in create_resources.keys():
            del new_resources[k]
            del new_resources[k + CONFIGURATION_SUFFIX]
            del new_parameter_defs[k + CONFIGURATION_KEY_SUFFIX]
            del parameters[k + CONFIGURATION_KEY_SUFFIX]

        # Do not remove deleted resources from the template yet
        for k in delete_resources.keys():
            new_resources[k] = old_resources[k]
            new_resources[k + CONFIGURATION_SUFFIX] = old_resources[k + CONFIGURATION_SUFFIX]
            if k + CONFIGURATION_KEY_SUFFIX in old_parameter_defs:
                new_parameter_defs[k + CONFIGURATION_KEY_SUFFIX] = old_parameter_defs[k + CONFIGURATION_KEY_SUFFIX]
            parameters[k + CONFIGURATION_KEY_SUFFIX] = None

        # Remove the cross-gem interface resolver if it exists
        cross_gem_resolver = new_resources.pop(constant.CROSS_GEM_RESOLVER_KEY, None)

        try:
            # Do individual stack updates for each newly created resource
            for k in create_resources.keys():
                new_resources[k] = saved_resources[k]
                new_resources[k + CONFIGURATION_SUFFIX] = saved_resources[k + CONFIGURATION_SUFFIX]
                new_parameter_defs[k + CONFIGURATION_KEY_SUFFIX] = saved_parameter_defs[k + CONFIGURATION_KEY_SUFFIX]
                parameters[k + CONFIGURATION_KEY_SUFFIX] = parameters[CONFIGURATION_KEY_SUFFIX]
                context.stack.update(
                    deployment_stack_id,
                    None,
                    parameters=parameters,
                    pending_resource_status=pending_resource_status,
                    capabilities=capabilities,
                    template_body=json.dumps(new_template, indent=4, sort_keys=True),
                    throw_failed_resources=True
                )

            # Do individual stack updates for each deleted resource
            for k in delete_resources.keys():
                del new_resources[k]
                del new_resources[k + CONFIGURATION_SUFFIX]
                del parameters[k + CONFIGURATION_KEY_SUFFIX]
                if k + CONFIGURATION_KEY_SUFFIX in new_parameter_defs:
                    del new_parameter_defs[k + CONFIGURATION_KEY_SUFFIX]

                context.stack.update(
                    deployment_stack_id,
                    None,
                    parameters=parameters,
                    pending_resource_status=pending_resource_status,
                    capabilities=capabilities,
                    template_body=json.dumps(
                        new_template, indent=4, sort_keys=True),
                    throw_failed_resources=True
                )

            # Do individual stack updates for each modified resource
            for k in update_resources.keys():
                parameters[k + CONFIGURATION_KEY_SUFFIX] = parameters[CONFIGURATION_KEY_SUFFIX]
                context.stack.update(
                    deployment_stack_id,
                    None,
                    parameters=parameters,
                    pending_resource_status=pending_resource_status,
                    capabilities=capabilities,
                    template_body=json.dumps(new_template, indent=4, sort_keys=True),
                    throw_failed_resources=True
                )


            # Re-add the cross gem resolver last
            if cross_gem_resolver:
                new_resources[constant.CROSS_GEM_RESOLVER_KEY] = cross_gem_resolver
                context.stack.update(
                    deployment_stack_id,
                    None,
                    parameters=parameters,
                    pending_resource_status=pending_resource_status,
                    capabilities=capabilities,
                    template_body=json.dumps(new_template, indent=4, sort_keys=True),
                    throw_failed_resources=True
                )

        except stack.StackOperationException:
            pass

    after_update(context, deployment_uploader)

    # Update mappings...
    __update_mappings(context, args.deployment, has_changes)


def update_access_stack(context, args):

    # Use default deployment if necessary
    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError('No default deployment has been set. Provide the --deployment parameter or use the default-deployment command to set a default deployment.')
        args.deployment = context.config.default_deployment

    if args.deployment == '*':
        for deployment_name in context.config.deployment_names:
            _update_access_stack(context, args, deployment_name)
    else:
        _update_access_stack(context, args, args.deployment)


def _update_access_stack(context, args, deployment_name):

    # Get the data we need...

    deployment_stack_id = context.config.get_deployment_stack_id(deployment_name)
    deployment_access_stack_id = context.config.get_deployment_access_stack_id(deployment_name)
    pending_resource_status = __get_pending_access_resource_status(context, deployment_name)

    # Is it ok to do this?

    capabilities = context.stack.confirm_stack_operation(
        deployment_access_stack_id,
        'deployment {} access'.format(deployment_name),
        args,
        pending_resource_status
    )

    # Do the update...

    project_uploader = ProjectUploader(context)
    deployment_uploader = project_uploader.get_deployment_uploader(deployment_name)

    context.view.processing_template('{} deployment'.format(deployment_name))

    access_template_url = deployment_uploader.upload_content(
        constant.DEPLOYMENT_ACCESS_TEMPLATE_FILENAME,
        json.dumps(context.config.deployment_access_template_aggregator.effective_template, indent=4, sort_keys=True),
        'Configured Deployment Access Template')

    parameters = __get_access_stack_parameters(
        context,
        deployment_name,
        deployment_stack_id = deployment_stack_id,
        uploader = deployment_uploader
    )

    # wait a bit for S3 to help insure that templates can be read by cloud formation
    time.sleep(constant.STACK_UPDATE_DELAY_TIME)

    context.stack.update(
        deployment_access_stack_id,
        access_template_url,
        parameters,
        pending_resource_status = pending_resource_status,
        capabilities = capabilities
    )


def upload_resource_group_settings(context, deployment_name):
    settings_uploader = Uploader(context, key='{}/{}'.format(constant.RESOURCE_SETTINGS_FOLDER,deployment_name))
    settings_uploader.upload_content(
        constant.DEPLOYMENT_RESOURCE_GROUP_SETTINGS,
        json.dumps(context.config.aggregate_settings, indent=4, sort_keys=True),
        'Aggregate settings file from resource group settings files')


def before_update(context, deployment_uploader):

    context.config.aggregate_settings = {}

    for group in context.resource_groups.values():
        if group.is_enabled:
            resource_group.before_update(
                deployment_uploader,
                group.name
            )

    upload_resource_group_settings(context,deployment_uploader.deployment_name)

    context.view.processing_template('{} deployment'.format(deployment_uploader.deployment_name))

    deployment_template_url = deployment_uploader.upload_content(
        constant.DEPLOYMENT_TEMPLATE_FILENAME,
        json.dumps(context.config.deployment_template_aggregator.effective_template, indent = 4, sort_keys = True),
        "Configured Deployment Template")

    # Deprecated in 1.9. TODO remove
    deployment_uploader.execute_uploader_pre_hooks()

    return deployment_template_url


def after_update(context, deployment_uploader):

    for group in context.resource_groups.values():
        if group.is_enabled:
            resource_group.after_update(
                deployment_uploader,
                group.name
            )

    # Deprecated in 1.9 - TODO: remove
    deployment_uploader.execute_uploader_post_hooks()

    # Deprecated in 1.9 - TODO: remove
    context.hooks.call_module_handlers('cli-plugin-code/resource_group_hooks.py', 'on_post_update',
        args = [deployment_uploader.deployment_name, None],
        deprecated = True
    )


def tags(context, args):
    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError(
                'No deployment was specified and there is no default deployment configured.')
        args.deployment = context.config.default_deployment
    if args.clear:
        if args.add or args.delete:
            raise HandledError('Cannot have --clear along with --add or --delete')
        clear_tags(context, args.deployment)
    if args.add:
        add_tags(context, args.deployment, args.add)
    if args.delete:
        delete_tags(context, args.deployment, args.delete)

    if args.list:
        context.view._output_message(list_tags(context, args.deployment))


def clear_tags(context, deployment):
    if not constant.DEPLOYMENT_TAGS in context.config.local_project_settings:
        return
    context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment] = []
    context.config.local_project_settings.save()


def add_tags(context, deployment, tags):
    if not constant.DEPLOYMENT_TAGS in context.config.local_project_settings:
        context.config.local_project_settings[constant.DEPLOYMENT_TAGS] = {}
    if not deployment in context.config.local_project_settings[constant.DEPLOYMENT_TAGS]:
        context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment] = []
    for tag in tags:
        if not tag in context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment]:
            context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment].append(tag)
    context.config.local_project_settings.save()


def delete_tags(context, deployment, tags):
    if not constant.DEPLOYMENT_TAGS in context.config.local_project_settings:
        context.config.local_project_settings[constant.DEPLOYMENT_TAGS] = {}
    if not deployment in context.config.local_project_settings[constant.DEPLOYMENT_TAGS]:
        context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment] = []
    for tag in tags:
        if not tag in context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment]:
            context.view._output_message("Tried to delete tag {}, but it was not found on the deployment {}".format(tag, deployment))
        else:
            context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment].remove(tag)
    context.config.local_project_settings.save()


def list_tags(context, deployment):
    if not constant.DEPLOYMENT_TAGS in context.config.local_project_settings:
        return []
    if not deployment in context.config.local_project_settings[constant.DEPLOYMENT_TAGS]:
        return []
    return json.dumps(context.config.local_project_settings[constant.DEPLOYMENT_TAGS][deployment])


def list(context, args):

    deployments = []

    for deployment_name in context.config.deployment_names:
        stack_description = _get_deployment_stack_description(context, deployment_name)
        deployments.append(stack_description)

    context.view.deployment_list(deployments)


def describe_stack(context, args):
    stack_description = _get_deployment_stack_description(context, args.deployment)
    context.view.deployment_stack_description(args.deployment, stack_description)


def _get_effective_deployment_stack_id(context, deployment_name):
    stack_id = context.config.get_deployment_stack_id(deployment_name, optional=True)
    if stack_id is None:
        stack_id = context.config.get_pending_deployment_stack_id(deployment_name)
    return stack_id


def _get_effective_access_stack_id(context, deployment_name):
    stack_id = context.config.get_deployment_access_stack_id(deployment_name, optional=True)
    if stack_id is None:
        stack_id = context.config.get_pending_deployment_access_stack_id(deployment_name)
    return stack_id


def _get_deployment_stack_description(context, deployment_name):

    description = {
        'Name': deployment_name,
        'Protected': deployment_name in context.config.get_protected_depolyment_names(),
        'UserDefault': context.config.user_default_deployment == deployment_name,
        'ProjectDefault': context.config.project_default_deployment == deployment_name,
        'Release': context.config.release_deployment == deployment_name,
        'Default': context.config.default_deployment == deployment_name
    }

    if context.config.project_initialized:

        stack_id = _get_effective_deployment_stack_id(context, deployment_name)

        if stack_id is None:

            # no pending or final deployment stack id

            description_update = {
                'StackStatus': '',
                'PendingAction': context.stack.PENDING_CREATE,
                'PendingReason': PENDING_CREATE_REASON
            }

        else:
            try:

                description_update = context.stack.describe_stack(stack_id, optional = True)
                if description_update is None or description_update.get('StackStatus', None) == context.stack.STATUS_DELETE_COMPLETE:

                    # have deployment stack id (maybe pending) but the stack doesn't actually exist

                    description_update = {
                        'StackStatus': '',
                        'PendingAction': context.stack.PENDING_CREATE,
                        'PendingReason': PENDING_CREATE_REASON
                    }

                if description_update.get('StackStatus', None) in [context.stack.STATUS_ROLLBACK_COMPLETE, context.stack.STATUS_UPDATE_ROLLBACK_FAILED]:

                    # deployment stack exists but wasn't created successfully

                    description_update['StackStatus'] = context.stack.STATUS_CREATE_FAILED
                    description_update['StackStatusReason'] = 'The creation of the stack for the deployment has failed. You can delete the deployment or attempt to create it again.'

                elif description_update.get('StackStatus', None) in [context.stack.STATUS_CREATE_COMPLETE, context.stack.STATUS_UPDATE_COMPLETE]:

                    # The deployment stack exists, isn't in an error state and isn't currently
                    # being updated. Use the status of the access stack instead.
                    #
                    # TODO: change the ui to have a seperate table access stack status. This
                    # will be a lot simpler then.

                    deployment_access_stack_id = _get_effective_access_stack_id(context, deployment_name)

                    deployment_acesss_stack_description = None
                    if deployment_access_stack_id is not None:
                        deployment_acesss_stack_description = context.stack.describe_stack(deployment_access_stack_id, optional=True)

                    if deployment_acesss_stack_description is None:
                        deployment_access_stack_status = None
                    else:
                        deployment_access_stack_status = deployment_acesss_stack_description.get('StackStatus', None)

                    if deployment_access_stack_status in [None, context.stack.STATUS_DELETE_COMPLETE]:

                        # There is no access stack.

                        description_update = {
                            'StackStatus': '',
                            'PendingAction': context.stack.PENDING_CREATE,
                            'PendingReason': ACCESS_PENDING_CREATE_REASON
                        }

                    elif deployment_access_stack_status in [context.stack.STATUS_ROLLBACK_COMPLETE, context.stack.STATUS_UPDATE_ROLLBACK_FAILED]:

                        # Creating the access stack failed.

                        description_update['StackStatus'] = context.stack.STATUS_CREATE_FAILED
                        description_update['StackStatusReason'] = 'The creation of the access control stack for the deployment has failed. You can delete the deployment or attempt to create it again.'

                    elif deployment_access_stack_status not in [context.stack.STATUS_CREATE_COMPLETE, context.stack.STATUS_UPDATE_COMPLETE]:

                        # Use access stack status.

                        description_update['StackStatus'] = deployment_access_stack_status
                        description_update['StackStatusReason'] = deployment_acesss_stack_description.get('StackStatusReason', '')

            except NoCredentialsError:
                description_update = {
                    'StackStatus': context.stack.STATUS_UNKNOWN,
                    'StackStatusReason': 'No AWS credentials provided.'
                }

        description.update(description_update)

    return description


def gather_writable_check_list(context):
    write_check_list = []

    # Deprecated in 1.9 - TODO: remove
    context.hooks.call_module_handlers('cli-plugin-code/resource_group_hooks.py','gather_writable_check_list',
        args = [write_check_list],
        deprecated = True
    )

    context.hooks.call_module_handlers('resource-manager-code/update.py', 'gather_writable_check_list',
        kwargs = {
            'check_list': write_check_list
        }
    )

    return write_check_list


def create_and_validate_writable_list(context):
    write_check_list = gather_writable_check_list(context)
    util.validate_writable_list(context, write_check_list)


def list_deployment_resources(context, args):

    # Use default deployment if necessary
    deployment_name = args.deployment
    if deployment_name is None:
        if context.config.default_deployment is None:
            raise HandledError('No deployment was specified and there is no default deployment configured.')
        deployment_name = context.config.default_deployment

    # TODO: change the ui to have a seperate table for access stack resources

    pending_resource_status = __get_pending_combined_resource_status(context, deployment_name)

    deployment_stack_id = _get_effective_deployment_stack_id(context, deployment_name)

    context.view.deployment_resource_list(deployment_stack_id, deployment_name, pending_resource_status)


def __get_deployment_stack_parameters(context, deployment_name, uploader = None):
    return {
        'ConfigurationBucket': uploader.bucket if uploader else None,
        'ConfigurationKey': uploader.key if uploader else None,
        'ProjectResourceHandler': context.config.project_resource_handler_id,
        'DeploymentName': deployment_name,
        'ProjectStackId': context.config.project_stack_id
    }


def __get_access_stack_parameters(context, deployment_name, deployment_stack_id = None, uploader = None):
    return {
        'ConfigurationBucket': uploader.bucket if uploader else None,
        'ConfigurationKey': uploader.key if uploader else None,
        'ProjectResourceHandler': context.config.project_resource_handler_id,
        'PlayerAccessTokenExchange': context.config.token_exchange_handler_id,
        'ProjectStack': util.get_stack_name_from_arn(context.config.project_stack_id),
        'DeploymentName': deployment_name,
        'DeploymentStack': util.get_stack_name_from_arn(deployment_stack_id) if deployment_stack_id else None,
        'DeploymentStackArn': deployment_stack_id
    }


def __get_pending_access_resource_status(context, deployment_name):

    access_stack_id = _get_effective_access_stack_id(context, deployment_name)
    deployment_stack_id = _get_effective_deployment_stack_id(context, deployment_name)

    template = context.config.deployment_access_template_aggregator.effective_template
    parameters = __get_access_stack_parameters(context, deployment_name, deployment_stack_id = deployment_stack_id, uploader = None)

    return context.stack.get_pending_resource_status(
        access_stack_id,
        new_template = template,
        new_parameter_values = parameters
    )


def __get_pending_deployment_resource_status(context, deployment_name):

    deployment_stack_id = _get_effective_deployment_stack_id(context, deployment_name)

    template = context.config.deployment_template_aggregator.effective_template
    parameters = __get_deployment_stack_parameters(context, deployment_name, uploader = None)

    return get_pending_deployment_resource_status(context, deployment_name, deployment_stack_id, template, parameters)

def get_pending_deployment_resource_status(context, deployment_name, deployment_stack_id, template, parameters):
    pending_resource_status = context.stack.get_pending_resource_status(
        deployment_stack_id,
        new_template = template,
        new_parameter_values = parameters
    )

    # add pending delete descriptions for all resources in nested stacks
    # only supports a single level of nesting (deployment / resource group)
    pending_resource_status_updates = {}
    for resource_name, resource_description in pending_resource_status.iteritems():
        if resource_description.get('ResourceType') == 'AWS::CloudFormation::Stack':
            stack_id = resource_description.get('PhysicalResourceId')
            if stack_id:
                resource_group_pending_resource_status = context.stack.get_pending_resource_status(
                    stack_id,
                    new_template = {} # resource status will be pending DELETE
                )
                for key, value in resource_group_pending_resource_status.iteritems():
                    pending_resource_status_updates[resource_name + '.' + key] = value
    pending_resource_status.update(pending_resource_status_updates)

    for group in context.resource_groups.values():
        resource_group_pending_resource_status = group.get_pending_resource_status(deployment_name)
        for key, value in resource_group_pending_resource_status.iteritems():
            pending_resource_status[group.name + '.' + key] = value

    return pending_resource_status


def __get_pending_combined_resource_status(context, deployment_name):

    pending_resource_status = __get_pending_deployment_resource_status(context, deployment_name)

    pending_access_resource_status = __get_pending_access_resource_status(context, deployment_name)
    for key, value in pending_access_resource_status.iteritems():
        pending_resource_status['(Access) ' + key] = value

    return pending_resource_status


def __check_resource_group_gem_status(context, resource_group_name):
    #it is valid if the resource group exists in the set of enabled gems    
    for gem in context.gem.enabled_gems:
        if gem.name == resource_group_name:
            return True
    return False


def __update_mappings(context, deployment_name, force = False):
    if deployment_name != context.config.default_deployment and deployment_name != context.config.release_deployment:
        return
    temp_args = util.Args()
    if deployment_name == context.config.release_deployment:
        temp_args.release = True
    mappings.update(context, temp_args, force)


def __check_custom_definitions(context, pending_resources):
    stack = stack_info.StackInfoManager().get_stack_info(context.config.project_stack_id, no_logging=True)
    for resource_name, resource_info in pending_resources.iteritems():
        if resource_info.get("ResourceStatus", None) == "DISABLED":
            continue
        if not __resource_handler_exists(stack, resource_info["ResourceType"]):
            raise HandledError(
                "The project stack has no definition for resource type {} to stand up resource {} "\
                "Update your project stack to add new custom resource handlers".format(resource_info["ResourceType"], resource_name))


def __resource_handler_exists(stack, resource_type):
    # These resources are always on the project stack
    CUSTOM_RESOURCE_WHITELIST = ["Custom::LambdaConfiguration"]

    if not resource_type.startswith("Custom::"):
        return True

    if resource_type in CUSTOM_RESOURCE_WHITELIST:
        return True

    type_definition = stack.resource_definitions.get(
        resource_type, None)

    if type_definition is None:
        return False

    if type_definition.handler_function is None:
        return False

    return True
