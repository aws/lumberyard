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

import service
import errors
import CloudCanvas
from resource_manager_common import stack_info


@service.api
def get_deployment_access_resource_info(request, deployment_name, resource_name):
    project_stack_arn = CloudCanvas.get_setting('ProjectStackArn')
    stack_info_manager = stack_info.StackInfoManager()
    project = stack_info.ProjectInfo(stack_info_manager, project_stack_arn)
    for deployment in project.deployments:
        if deployment.deployment_name == deployment_name:
            physical_id = deployment.deployment_access.resources.get_by_logical_id(resource_name).physical_id
            if physical_id:
                return {
                    'PhysicalId': physical_id
                }
            raise errors.NotFoundError('Resource {} not found.'.format(resource_name))
    raise errors.NotFoundError('Deployment {} not found'.format(deployment_name))


@service.api
def list_deployment_resources(request, deployment_name):
    project_stack_arn = CloudCanvas.get_setting('ProjectStackArn')
    stack_info_manager = stack_info.StackInfoManager()
    project = stack_info.ProjectInfo(stack_info_manager, project_stack_arn)
    for deployment in project.deployments:
        if deployment.deployment_name == deployment_name:
            resources = {}
            for resource_group in deployment.resource_groups:
                for resource in resource_group.resources:
                    full_logical_id = '.'.join([resource_group.resource_group_name, resource.logical_id])
                    if resource.type == 'Custom::ServiceApi':
                        resources[full_logical_id] = __get_service_api_mapping(resource_group, resource)
                    else:
                        resources[full_logical_id] = {
                            'PhysicalResourceId': resource.physical_id,
                            'ResourceType': resource.type
                        }
            return {'Resources': resources}
    raise errors.NotFoundError('Deployment {} not found'.format(deployment_name))


@service.api
def get_resource_group_resource_info(request, deployment_name, resource_group_name, resource_name):
    project_stack_arn = CloudCanvas.get_setting('ProjectStackArn')
    stack_info_manager = stack_info.StackInfoManager()
    project = stack_info.ProjectInfo(stack_info_manager, project_stack_arn)
    for deployment in project.deployments:
        if deployment.deployment_name == deployment_name:
            for resource_group in deployment.resource_groups:
                if resource_group.resource_group_name == resource_group_name:
                    physical_id = resource_group.resources.get_by_logical_id(resource_name).physical_id
                    if physical_id:
                        return {
                            'PhysicalId': physical_id
                        }
                    raise errors.NotFoundError('Resource {} not found.'.format(resource_name))
            raise errors.NotFoundError('Resource Group {} not found.'.format(resource_group))
    raise errors.NotFoundError('Deployment {} not found'.format(deployment_name))


def __get_service_api_mapping(resource_group, resource):
    outputs = resource_group.stack_description.get('Outputs', [])
    for output in outputs:
        if output.get('OutputKey') == 'ServiceUrl':
            service_url = output.get('OutputValue')
            if service_url:
                return {
                    'PhysicalResourceId': service_url,
                    'ResourceType': resource.type
                }
