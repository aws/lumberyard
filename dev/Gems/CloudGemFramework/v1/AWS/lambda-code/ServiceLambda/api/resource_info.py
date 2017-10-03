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
    project_stack_arn = CloudCanvas.get_setting('PROJECT_STACK_ARN')
    project = stack_info.ProjectInfo(project_stack_arn)
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
def get_resource_group_resource_info(request, deployment_name, resource_group_name, resource_name):
    project_stack_arn = CloudCanvas.get_setting('PROJECT_STACK_ARN')
    project = stack_info.ProjectInfo(project_stack_arn)
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

