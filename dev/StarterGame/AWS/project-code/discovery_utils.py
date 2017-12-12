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

import boto3
import os
import json
import time
import random

from botocore.exceptions import ClientError
from errors import ValidationError

BACKOFF_BASE_SECONDS = 0.1
BACKOFF_MAX_SECONDS = 20.0
BACKOFF_MAX_TRYS = 5

def try_with_backoff(fn):
    # http://www.awsarchitectureblog.com/2015/03/backoff.html
    backoff = BACKOFF_BASE_SECONDS
    count = 1
    while True:
        try:
            return fn()
        except ClientError as e:

            if count == BACKOFF_MAX_TRYS or e.response['Error']['Code'] != 'Throttling':
                raise e

            backoff = min(BACKOFF_MAX_SECONDS, random.uniform(BACKOFF_BASE_SECONDS, backoff * 3.0))
            print 'Throttled request attempt {}. Sleeping {} seconds'.format(count, backoff)
            time.sleep(backoff)
            count += 1

lambda_name = os.environ.get('AWS_LAMBDA_FUNCTION_NAME')
current_region = os.environ.get('AWS_REGION')

class StackS3Configuration(object):
    stack_name = ""
    configuration_bucket = ""

def get_configuration_bucket():  
    configuration = StackS3Configuration()

    cloud_formation_client = boto3.client('cloudformation', region_name=current_region)
 
    stack_definitions = try_with_backoff(lambda : cloud_formation_client.describe_stack_resources(PhysicalResourceId = lambda_name))
    print 'describe_stack_resources on PhysicalResourceId {} results {}'.format(lambda_name, stack_definitions)
    for stack_definition in stack_definitions['StackResources']:
            if stack_definition.get('LogicalResourceId',None) == 'Configuration':
                configuration.configuration_bucket = stack_definition['PhysicalResourceId']
                configuration.stack_name = stack_definition['StackName']
                break
    return configuration    


def _find_resource_by_physical_name(resources, physical_name):
    for resource in resources:
        if resource.get('PhysicalResourceId', None) == physical_name:
            return resource
    return None


def _find_resource_by_logical_name(resources, logical_name):
    for resource in resources:
        if resource.get('LogicalResourceId', None) == logical_name:
            return resource
    return None


# Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}

def get_stack_name_from_stack_arn(arn):
    return arn.split('/')[1]


def get_region_from_stack_arn(arn):
    return arn.split(':')[3]


def get_account_id_from_stack_arn(arn):
    return arn.split(':')[4]


def get_parameter_value(parameters, name):
    for parameter in parameters:
        if parameter['ParameterKey'] == name:
            return parameter['ParameterValue']
    return None


class StackInfo(object):

    def __init__(self, stack_arn, resources=None, client=None):
        self.stack_arn = stack_arn
        self.stack_name = get_stack_name_from_stack_arn(stack_arn)
        self.region = get_region_from_stack_arn(stack_arn)
        self.account_id = get_account_id_from_stack_arn(stack_arn)
        self._resources = resources
        self._client = client
        self._owning_stack_resources = None
        self._stack_description = None

    def get_client(self):
        if self._client is None:
            self._client = boto3.client('cloudformation', region_name=self.region)
        return self._client

    def get_resources(self):
        if self._resources is None:
            try:
                res = try_with_backoff(lambda : self.get_client().describe_stack_resources(StackName=self.stack_arn))
                print 'describe_stack_resources(StackName="{}") response: {}'.format(self.stack_arn, res)
            except Exception as e:
                print 'describe_stack_resources(StackName="{}") error: {}'.format(self.stack_arn, getattr(e, 'response', e))
                raise e

            self._resources = res['StackResources']
        return self._resources

    def get_resource(self, logical_name, expected_type=None, optional=False):
        resources = self.get_resources()
        resource = _find_resource_by_logical_name(resources, logical_name)
        if expected_type is not None:
            if resource is None:
                if optional:
                    return None
                else:
                    raise ValidationError('There is no {} resource in stack {}.'.format(logical_name, self.stack_arn))
            if resource['ResourceType'] != expected_type:
                raise ValidationError('The {} resource in stack {} has type {} but type {} was expected.'.format(
                    logical_name, 
                    self.stack_arn, 
                    resource['ResourceType'], 
                    expected_type))
        return resource

    def get_owning_stack_resources(self):
        if self._owning_stack_resources is None:
            client = self.get_client()
            try:
                res = try_with_backoff(lambda : client.describe_stack_resources(PhysicalResourceId=self.stack_arn))
                print 'describe_stack_resources(PhysicalResourceId="{}") response: {}.'.format(self.stack_arn, res)
            except Exception as e:
                print 'describe_stack_resources(PhysicalResourceId="{}") error: {}.'.format(self.stack_arn, getattr(e, 'response', e))
                raise e
            self._owning_stack_resources = res['StackResources']
        return self._owning_stack_resources

    def get_stack_description(self):
        if self._stack_description is None:
            client = self.get_client()
            try:
                res = try_with_backoff(lambda : client.describe_stacks(StackName=self.stack_arn))
                print 'describe_stacks(StackName="{}") response: {}.'.format(self.stack_arn, res)
            except Exception as e:
                print 'describe_stacks(StackName="{}") error: {}.'.format(self.stack_arn, getattr(e, 'response', e))
                raise e
            self._stack_description = res['Stacks'][0]
        return self._stack_description 



class ResourceGroupInfo(StackInfo):

    def __init__(self, resource_group_stack_arn, resource_group_stack_resource=None, deployment_info=None, resources=None, client=None):
        
        super(ResourceGroupInfo, self).__init__(resource_group_stack_arn, resources=resources, client=client)
        
        if resource_group_stack_resource is None:
            resource_group_stack_resource = _find_resource_by_physical_name(self.get_owning_stack_resources(), self.stack_arn)
        self.resource_group_name = resource_group_stack_resource['LogicalResourceId']

        if deployment_info is None:
            deployment_info = DeploymentInfo(resource_group_stack_resource['StackId'], resources=self._owning_stack_resources, client=self._client)
        self.deployment = deployment_info

    def __repr__(self):
        return 'ResourceGroupInfo(resource_group_name="{}")'.format(self.resource_group_name)


class DeploymentInfo(StackInfo):

    def __init__(self, deployment_stack_arn, deployment_stack_resource=None, project_info=None, resources=None, client=None):

        super(DeploymentInfo, self).__init__(deployment_stack_arn, resources=resources, client=client)

        stack_description = self.get_stack_description()
        parameters = stack_description.get('Parameters', [])

        self.deployment_name = get_parameter_value(parameters, 'DeploymentName')
        project_stack_id = get_parameter_value(parameters, 'ProjectStackId')

        if self.deployment_name is None or project_stack_id is None:
            raise RuntimeError('Deployment stack parameters DeploymentName and/or ProjectStackId are not set.')

        if project_info is None:
            project_info = ProjectInfo(project_stack_id, resources=self._owning_stack_resources, client=self._client)
        self.project = project_info

        self._access_stack_info = None
        self._resource_group_infos = None

    def __repr__(self):
        return 'DeploymentInfo(deployment_name="{}")'.format(self.deployment_name)

    def get_deployment_access_stack_info(self):
        if self._access_stack_info is None:
            access_stack_name = self.stack_name + '-Access'
            access_stack_arn = None

            try:
                
                client = self.get_client()
                res = try_with_backoff(lambda : client.describe_stacks(StackName=access_stack_name))

                print 'describe_stacks(StackName="{}") response: {}.'.format(self.stack_arn, res)

                for stack in res.get('Stacks', []):
                    if stack['StackStatus'] != 'DELETE_COMPLETE':
                        access_stack_arn = stack['StackId']
                        break

            except ClientError as e:
                print 'describe_stacks(StackName="{}") error: {}.'.format(self.stack_arn, getattr(e, 'response', e))
                # if ValidationError, stack doesn't exist or the lambda doesn't have access
                # we assume the former is the problem rather than trying to parse the error message
                if e.response['Error']['Code'] != 'ValidationError':
                    raise e
            except Exception as e:
                print 'describe_stacks(StackName="{}") error: {}.'.format(self.stack_arn, getattr(e, 'response', e))
                raise e

            if access_stack_arn is not None:
                self._access_stack_info = StackInfo(access_stack_arn)

        return self._access_stack_info

    def get_resource_group_infos(self):
        if self._resource_group_infos is None:
            resource_group_infos = []
            for resource in self.get_resources():
                if resource.get('ResourceType', None) == 'AWS::CloudFormation::Stack':
                    stack_id = resource.get('PhysicalResourceId', None)
                    if stack_id is not None:
                        resource_group_info = ResourceGroupInfo(
                            stack_id, 
                            resource_group_stack_resource=resource, 
                            client=self._client, 
                            deployment_info=self)
                        resource_group_infos.append(resource_group_info)
            self._resource_group_infos = resource_group_infos
        return self._resource_group_infos


class ProjectInfo(StackInfo):

    def __init__(self, project_stack_arn, resources=None, client=None):
        super(ProjectInfo, self).__init__(project_stack_arn, resources=resources, client=client)
        self.project_name = self.stack_name

    def __repr__(self):
        return 'ProjectInfo(project_name="{}")'.format(self.project_name)

    def get_deployment_infos(self):
        if self._deployment_infos is None:
            deployment_infos = []
            resources = self.get_resources()
            for resource in resources:
                if resource.get('ResourceType', None) == 'AWS::CloudFormation::Stack' and not resource.get('LogicalResourceId', '').endswith('Access'):
                    pyhsical_resource_id = resource.get('PhysicalResourceId', None)
                    if pyhsical_resource_id is not None:
                        deployment_info = DeploymentInfo(
                            pyhsical_resource_id, 
                            deployment_stack_resource=resource, 
                            client=self._client, 
                            project_info=self)
                        deployment_infos.append(deployment_info)
            self._deployment_infos = deployment_infos
        return self._deployment_infos

def get_cloud_canvas_metadata(resource, metadata_name):

    metadata_string = resource.get('Metadata', None)
    if metadata_string is None: return
    
    try:
        metadata = json.loads(metadata_string)
    except ValueError as e:
        raise ValidationError('Could not parse CloudCanvas {} Metadata: {}. {}'.format(metadata_name, metadata_string, e))

    cloud_canvas_metadata = metadata.get('CloudCanvas', None)
    if cloud_canvas_metadata is None: return

    return cloud_canvas_metadata.get(metadata_name, None)


RESOURCE_ARN_PATTERNS = {
    'AWS::DynamoDB::Table': 'arn:aws:dynamodb:{region}:{account_id}:table/{resource_name}',
    'AWS::Lambda::Function': 'arn:aws:lambda:{region}:{account_id}:function:{resource_name}',
    'AWS::SQS::Queue': 'arn:aws:sqs:{region}:{account_id}:{resource_name}',
    'AWS::SNS::Topic': 'arn:aws:sns:{region}:{account_id}:{resource_name}',
    'AWS::S3::Bucket': 'arn:aws:s3:::{resource_name}'
}


def get_resource_arn(stack_arn, resource_type, resource_name):
    
    pattern = RESOURCE_ARN_PATTERNS.get(resource_type, None)
    if pattern is None:
        raise ValidationError('Unsupported ARN mapping for resource type {} on resource {}. To add support for additional resource types, add an entry to RESOURCE_ARN_PATTERNS in project-code\discovery_utils.py.'.format(resource_type, resource_name))

    return pattern.format(
        region=get_region_from_stack_arn(stack_arn),
        account_id=get_account_id_from_stack_arn(stack_arn),
        resource_name=resource_name)
