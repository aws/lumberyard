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

import dateutil
import json

import boto3
from botocore.exceptions import ClientError

import aws_utils

def get_cloud_formation_client(stack_arn):
    region = aws_utils.get_region_from_stack_arn(stack_arn)
    return aws_utils.ClientWrapper(boto3.client('cloudformation', region_name=region))

s3 = aws_utils.ClientWrapper(boto3.client('s3'))

class ResourceInfo(object):

    def __init__(self, stack_info, resource_summary):
        self.__stack_info = stack_info
        self.__resource_summary = resource_summary
        self.__resource_detail = None
        self.__metadata = None

    @property
    def __summary(self):
        return self.__resource_summary

    @property
    def __detail(self):
        if not self.__resource_detail :
            res = self.stack.client.describe_stack_resource(StackName=self.stack.stack_arn, LogicalResourceId=self.logical_id)
            self.__resource_detail  = res.get('StackResourceDetail', {})
        return self.__resource_detail

    @property
    def resource_arn(self):
        return aws_utils.get_resource_arn(self.stack.stack_arn, self.type, self.physical_id)

    @property
    def stack(self):
        return self.__stack_info

    @property
    def physical_id(self):
        return self.__summary.get('PhysicalResourceId')

    @property
    def logical_id(self):
        return self.__summary.get('LogicalResourceId')

    @property
    def type(self):
        return self.__summary.get('ResourceType')

    @property
    def status(self):
        return self.__summary.get('ResourceStatus')

    @property
    def status_reason(self):
        return self.__summary.get('ResourceStatusReason')

    @property
    def last_updated_time(self):
        timestamp = self.__summary.get('LastUpdatedTimestamp')
        return dateutil.parser.parse(timestamp) if timestamp else None

    @property
    def description(self):
        return self.__detail.get('Description')

    @property
    def metadata(self):
        if not self.__metadata:
            metadata_string = self.__detail.get('Metadata', '{}')
            try:
                self.__metadata = json.loads(metadata_string)
            except ValueError as e:
                raise RuntimeError('Could not parse stack {} resource {} metadata {}: {}'.format(self.stack.stack_arn, self.logical_id, metadata_string, e))
        return self.__metadata

    def get_cloud_canvas_metadata(self, *names):
        result = self.metadata.get('CloudCanvas', {})
        for name in names:
            if name not in result:
                return None
            result = result.get(name, {})
        return result

    

class ResourceInfoList(list):

    def __init__(self, stack_info):
        self.__stack_info = stack_info

    @property
    def stack(self):
        return self.__stack_info

    def __find_resource(self, attribute, value, expected_type, optional):

        for resource in self:
            if getattr(resource, attribute) == value:
                if expected_type is not None:
                    if resource.type != expected_type:
                        raise RuntimeError('The {} resource in stack {} has type {} but type {} was expected.'.format(
                            getattr(resource, attribute), 
                            self.stack.stack_arn, 
                            resource.type, 
                            expected_type))
                return resource

        if not optional:
            raise RuntimeError('There is no {} resource in stack {}.'.format(value, self.stack.stack_arn))

        return None

    def get_by_physical_id(self, physical_id, expected_type=None, optional=False):
        return self.__find_resource('physical_id', physical_id, expected_type, optional)

    def get_by_logical_id(self, logical_id, expected_type=None, optional=False):
        return self.__find_resource('logical_id', logical_id, expected_type, optional)

    def get_by_type(self, resource_type):
        return [resource for resource in self if resource.type == resource_type]


class ParametersDict(dict):

    def __init__(self, stack_arn):
        self.__stack_arn = stack_arn

    def __getitem__(self, key):
        value = self.get(key)
        if not value:
            raise RuntimeError('The {} stack has no {} parameter. This parameter is required.'.format(self.__stack_arn, key))
        return value


class StackInfo(object):

    STACK_TYPE_RESOURCE_GROUP = 'ResourceGroup'
    STACK_TYPE_DEPLOYMENT = 'Deployment'
    STACK_TYPE_DEPLOYMENT_ACCESS = 'DeploymentAccess'
    STACK_TYPE_PROJECT = 'Project'

    def __init__(self, stack_arn, stack_type, client=None, stack_description = None):
        self.__stack_arn = stack_arn
        self.__resources = None
        self.__client = client
        self.__stack_description = stack_description
        self.__stack_type = stack_type
        self.__parameters = None
        self.__template = None

    @property
    def stack_arn(self):
        return self.__stack_arn

    @property
    def stack_name(self):
        return aws_utils.get_stack_name_from_stack_arn(self.stack_arn)

    @property
    def stack_type(self):
        return self.parameters['CloudCanvasStack']

    @property
    def region(self):
        return aws_utils.get_region_from_stack_arn(self.stack_arn)

    @property
    def account_id(self):
        return aws_utils.get_account_id_from_stack_arn(self.stack_arn)

    @property
    def stack_type(self):
        return self.__stack_type

    @property
    def client(self):
        if self.__client is None:
            self.__client = aws_utils.ClientWrapper(boto3.client('cloudformation', region_name=self.region))
        return self.__client

    @property
    def resources(self):
        if self.__resources is None:
            resources = ResourceInfoList(self)
            res = self.client.list_stack_resources(StackName=self.stack_arn)
            for resource_summary in res['StackResourceSummaries']:
                resources.append(ResourceInfo(self, resource_summary))
            while('NextToken' in res):
                res = lambda : self.client.list_stack_resources(StackName=self.stack_arn, NextToken=res['NextToken'])
                for resource_summary in res['StackResourceSummaries']:
                    resources.append(ResourceInfo(self, resource_summary))
            self.__resources = resources
        return self.__resources

    @property
    def stack_description(self):
        if self.__stack_description is None:
            res = self.client.describe_stacks(StackName=self.stack_arn)
            self.__stack_description = res['Stacks'][0]
        return self.__stack_description 

    @property
    def parameters(self):
        if self.__parameters is None:
            parameters_dict = ParametersDict(self.stack_arn)
            parameter_list = self.stack_description.get('Parameters', [])
            for parameter in parameter_list:
                parameters_dict[parameter['ParameterKey']] = parameter['ParameterValue']
            self.__parameters = parameters_dict
        return self.__parameters

    @property
    def template(self):
        if self.__template is None:
            res = self.client.get_template(StackName=self.stack_arn)
            self.__template = json.loads(res['TemplateBody'])
        return self.__template


class ResourceGroupInfo(StackInfo):

    def __init__(self, resource_group_stack_arn, resource_group_name=None, deployment_info=None, client=None, stack_description = None):
        super(ResourceGroupInfo, self).__init__(resource_group_stack_arn, StackInfo.STACK_TYPE_RESOURCE_GROUP, client=client, stack_description=stack_description)
        self.__resource_group_name = resource_group_name
        self.__deployment_info = deployment_info

    def __repr__(self):
        return 'ResourceGroupInfo(stack_name="{}")'.format(self.stack_name)

    @property
    def permission_context_name(self):
        return self.resource_group_name

    @property
    def resource_group_name(self):
        if self.__resource_group_name is None:
            self.__resource_group_name = self.parameters['ResourceGroupName']
        return self.__resource_group_name

    @property
    def deployment(self):
        if self.__deployment_info is None:
            deployment_stack_arn = self.parameters['DeploymentStackArn']
            self.__deployment_info = DeploymentInfo(deployment_stack_arn, client=self.client)
        return self.__deployment_info


class DeploymentAccessInfo(StackInfo):

    def __init__(self, deployment_access_stack_arn, deployment_info = None, client = None, stack_description = None):
        super(DeploymentAccessInfo, self).__init__(deployment_access_stack_arn, StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS, client=client, stack_description=stack_description)
        self.__deployment_info = deployment_info

    def __repr__(self):
        return 'DeploymentAccessInfo(stack_name="{}")'.format(self.stack_name)

    @property
    def deployment(self):
        if self.__deployment_info is None:
            deployment_stack_arn = self.parameters['DeploymentStackArn']
            self.__deployment_info = DeploymentInfo(deployment_stack_arn, deployment_access_info = self, client=self.client)
        return self.__deployment_info

    
class DeploymentInfo(StackInfo):

    def __init__(self, deployment_stack_arn, project_info=None, deployment_access_info = None, deployment_access_stack_arn = None, client=None, stack_description = None):
        super(DeploymentInfo, self).__init__(deployment_stack_arn, StackInfo.STACK_TYPE_DEPLOYMENT, client=client, stack_description=stack_description)
        self.__project_stack_info = project_info
        self.__deployment_name = None
        self.__deployment_access_info = deployment_access_info
        self.__resource_group_infos = None
        self.__deployment_access_stack_arn = deployment_access_stack_arn

    def __repr__(self):
        return 'DeploymentInfo(stack_name="{}")'.format(self.stack_name)

    @property
    def deployment_name(self):
        if not self.__deployment_name:
            self.__deployment_name = self.parameters['DeploymentName']
        return self.__deployment_name

    @property
    def deployment_access(self):
        if self.__deployment_access_info is None:

            if self.__deployment_access_stack_arn is None:

                access_stack_name = self.stack_name + '-Access'
                try:
                
                    res = self.client.describe_stacks(StackName=access_stack_name)
                    for stack in res.get('Stacks', []):
                        if stack['StackStatus'] != 'DELETE_COMPLETE':
                            self.__deployment_access_stack_arn = stack['StackId']
                            break

                except ClientError as e:
                    # if RuntimeError, stack doesn't exist or the lambda doesn't have access
                    if e.response['Error']['Code'] != 'ValidationError':
                        raise e

            if self.__deployment_access_stack_arn is not None:
                self.__deployment_access_info = DeploymentAccessInfo(self.__deployment_access_stack_arn, deployment_info=self, client=self.client)

        return self.__deployment_access_info

    @property
    def project(self):
        if self.__project_stack_info is None:
            project_stack_id = self.parameters['ProjectStackId']
            self.__project_stack_info = ProjectInfo(project_stack_id, client=self.client)
        return self.__project_stack_info

    @property
    def resource_groups(self):
        if self.__resource_group_infos is None:
            resource_group_infos = []
            for resource in self.resources:
                if resource.type == 'AWS::CloudFormation::Stack':
                    stack_id = resource.physical_id
                    if stack_id is not None:
                        resource_group_info = ResourceGroupInfo(
                            stack_id, 
                            resource_group_name=resource.logical_id, 
                            client=self.client, 
                            deployment_info=self)
                        resource_group_infos.append(resource_group_info)
            self.__resource_group_infos = resource_group_infos
        return self.__resource_group_infos


class ProjectInfo(StackInfo):

    def __init__(self, project_stack_arn, client=None, stack_description = None):
        super(ProjectInfo, self).__init__(project_stack_arn, StackInfo.STACK_TYPE_PROJECT, client=client, stack_description=stack_description)
        self.__deployment_infos = None
        self.__project_settings = None

    def __repr__(self):
        return 'ProjectInfo(stack_name="{}")'.format(self.stack_name)

    @property
    def permission_context_name(self):
        return self.project_name

    @property
    def project_name(self):
        return self.stack_name

    @property
    def deployments(self):
        if self.__deployment_infos is None:
            deployment_infos = []
            for deployment_name, deployment_settings in self.project_settings.get('deployment', {}).iteritems():
                deployment_stack_arn = deployment_settings.get('DeploymentStackId')
                deployment_access_stack_arn = deployment_settings.get('DeploymentAccessStackId')
                if deployment_stack_arn:
                    deployment_infos.append(DeploymentInfo(deployment_stack_arn, deployment_access_stack_arn = deployment_access_stack_arn, client=self.client, project_info=self))
            self.__deployment_infos = deployment_infos
        return self.__deployment_infos

    @property
    def configuration_bucket(self):
        resource = self.resources.get_by_logical_id('Configuration', expected_type='AWS::S3::Bucket')
        return resource.physical_id

    @property
    def project_settings(self):
        if self.__project_settings is None:
            try:
                res = s3.get_object(Bucket = self.configuration_bucket, Key='project-settings.json')
                json_string = res['Body'].read()
                print 'read project-settings.json contents: {}'.format(json_string)
                self.__project_settings = json.loads(json_string)
            except ClientError as e:
                # When the project stack is being deleted, the configuration bucket's contents 
                # will have been cleared before the stack is deleted. During the stack delete, 
                # custom resources such as AccessControl may attempt actions that cause project
                # settings to load, which will fail with either access denied or no such key,
                # depending on if we have list objects permissions on the bucket.
                if e.response.get('Error', {}).get('Code', '') not in [ 'AccessDenied', 'NoSuchKey' ]:
                    raise e
                print 'WARNING: could not read project-settings.json from the project configuration bucket.'
                self.__project_settings = {}
        return self.__project_settings


# TODO: add StackInfo object caching? Would need to insure that
# list of deployments in project and resource groups in deployment
# reflect recent changes, so may not be able to cache those across
# requests.
#
# Currently we create duplicate info objects even within a request
# if this occurs (which doesn't happen with current use cases):
#
#    1) get_stack_info for resource group stack
#    2) get resource group deployment info
#    3) get all resource group infos
#
# The origional resource group info is not reused in step 3.
#
# The current implemeantion also creates circular references between
# StackInfo objects. This are inefficient for Python to garbage 
# collect. If we have a cache by stack arn, direct references can
# be replaced with arns and a cache lookup.

def get_stack_info(stack_arn):
    client = get_cloud_formation_client(stack_arn)

    res = client.describe_stacks(StackName = stack_arn)
    stack_description = res['Stacks'][0]
    parameters = stack_description.get('Parameters', [])

    stack_type = None
    for parameter in parameters:
        if parameter['ParameterKey'] == 'CloudCanvasStack':
            stack_type = parameter['ParameterValue']

    if not stack_type:
        raise RuntimeError('The stack {} is not a Lumberyard Cloud Canvas managed stack.'.format(stack_arn))

    if stack_type == StackInfo.STACK_TYPE_RESOURCE_GROUP:
        stack_info = ResourceGroupInfo(stack_arn, client=client, stack_description = stack_description)
    elif stack_type == StackInfo.STACK_TYPE_DEPLOYMENT:
        stack_info = DeploymentInfo(stack_arn, client=client, stack_description = stack_description)
    elif stack_type == StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS:
        stack_info = DeploymentAccessInfo(stack_arn, client=client, stack_description = stack_description)
    elif stack_type == StackInfo.STACK_TYPE_PROJECT:
        stack_info = ProjectInfo(stack_arn, client=client, stack_description = stack_description)
    else:
        raise RuntimeError('The stack {} has an unexpected Lumberyard Cloud Canvas managed stack type: {}'.format(stack_arn, stack_type))

    return stack_info

