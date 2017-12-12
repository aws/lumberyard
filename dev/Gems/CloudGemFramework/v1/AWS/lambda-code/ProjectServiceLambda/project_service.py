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
# $Revision$

import os
from resource_manager_common import stack_info

def handler(event, context):
    print 'handling event', event
    function = event.get('function')
    if function == 'get_resource_physical_id':
        return get_resource_physical_id(event)
    raise RuntimeError('Invalid function {}'.format(function))

def get_resource_physical_id(event):
    project_stack_arn = os.environ.get('PROJECT_STACK_ARN')
    if not project_stack_arn:
        raise RuntimeError('Configuration error: PROJECT_STACK_ARN was not defined')

    stack = event.get('stack')
    if stack != 'Access':
        raise RuntimeError('Invalid stack {}'.format(stack))

    deployment_name = event.get('deployment_name')
    if not deployment_name:
        raise RuntimeError('Missing deployment_name')

    resource_name = event.get('resource_name')
    if not resource_name:
        raise RuntimeError('Missing resource_name')

    project = stack_info.ProjectInfo(project_stack_arn)
    for deployment in project.deployments:
        if deployment.deployment_name == deployment_name:
            physical_id = deployment.deployment_access.resources.get_by_logical_id(resource_name).physical_id
            return {'physical_id': physical_id}

    raise RuntimeError('Deployment {} not found'.format(deployment_name))
