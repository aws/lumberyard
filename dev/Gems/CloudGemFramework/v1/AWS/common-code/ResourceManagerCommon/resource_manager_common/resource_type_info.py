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
import json
import re
import sys

import constant
try:
    from cgf_utils import aws_utils
except:
    print 'Failed to import from cgf_utils, path is {}'.format(sys.path)

from cgf_utils import lambda_utils

s3 = aws_utils.ClientWrapper(boto3.client('s3'))

CUSTOM_RESOURCE_LAMBDA_TAG = "CRH"
ARN_LAMBDA_TAG = "AH"
LAMBDA_TAGS = {CUSTOM_RESOURCE_LAMBDA_TAG, ARN_LAMBDA_TAG}


class ResourceTypeInfo(object):
    def __init__(self, stack_arn, source_resource_name, resource_type_name, data):
        self.__stack_arn = stack_arn
        self.__stack_name = aws_utils.get_stack_name_from_stack_arn(stack_arn)
        self.__source_resource_name = source_resource_name
        self.__resource_type_name = resource_type_name
        self.__permission_metadata = data.get('PermissionMetadata', {})
        self.__service_api = data.get('ServiceApi', None)
        self.__arn_format = data.get('ArnFormat', None)
        self.__arn_url = data.get('ArnUrl', None)
        self.__display_info = data.get('DisplayInfo', None)
        self.__handler_url = data.get('HandlerUrl', None)
        self.__arn_function = data.get('ArnFunction', None)
        self.__handler_function = data.get('HandlerFunction', None)
        self._validate()

    def _validate(self):
        if not self.__arn_format:
            # TODO: Rules like arn_format ^ arn_url, arn_url && service_api, etc.
            #raise RuntimeError("Definition for resource type '%s' in definition '%s' does not specify an ArnFormat"
            #                   % (self.__resource_type_name, self.__source_resource_name))
            #
            # Validate that an arn_function or handler_function has a policy_document for the role
            pass

    @property
    def stack_arn(self):
        return self.__stack_arn

    @property
    def stack_name(self):
        return self.__stack_name

    @property
    def source_resource_name(self):
        return self.__source_resource_name

    @property
    def resource_type_name(self):
        return self.__resource_type_name

    @property
    def permission_metadata(self):
        return self.__permission_metadata

    @property
    def service_api(self):
        return self.__service_api

    @property
    def arn_format(self):
        return self.__arn_format

    @property
    def arn_url(self):
        return self.__arn_url

    @property
    def display_info(self):
        return self.__display_info

    @property
    def handler_url(self):
        return self.__handler_url

    @property
    def arn_function(self):
        return self.__arn_function

    @property
    def handler_function(self):
        return self.__handler_function

    def get_lambda_function_name(self, tag):
        name = "%s-%s-%s-%s" % (self.__stack_name, tag, self.__source_resource_name,
                                self.__resource_type_name.replace("::", "_"))
        result = lambda_utils.sanitize_lambda_name(name)

        return result

    def get_arn_lambda_function_name(self):
        return self.get_lambda_function_name(ARN_LAMBDA_TAG)

    def get_custom_resource_lambda_function_name(self):
        return self.get_lambda_function_name(CUSTOM_RESOURCE_LAMBDA_TAG)


class ResourceTypesPathInfo(object):
    # Validate that a path looks like <resource-dir>/<path-components>/<resource-name>.json
    _re_validate_path = re.compile(r"^%(dir)s%(sep)s(.+)%(sep)s([A-Za-z0-9_\-]+)\.json$" % {
        'dir': constant.RESOURCE_DEFINITIONS_PATH, 'sep': constant.S3_DELIMETER})

    def __init__(self, path):
        match = ResourceTypesPathInfo._re_validate_path.match(path)

        if match is None:
            raise RuntimeError("Invalid resource types file path '%s'" % path)

        components = constant.S3_DELIMETER.split(match.group(1))
        self.path = path
        self.resource_name = match.group(2)
        self.stack_hierarchy = components
        self.project_stack = components[0]

        if len(components) >= 3:
            self.deployment_stack = components[1]
            self.resource_group_stack = components[2]
        else:
            self.deployment_stack = None
            self.resource_group_stack = None


def _load_mappings_for_prefix(destination, bucket, prefix, delimiter, s3_client):
    bucket_entries = s3_client.list_objects_v2(Bucket=bucket, Prefix=prefix, Delimiter=delimiter)
    for entry in bucket_entries.get('Contents', []):
        file_key = entry['Key']
        if not file_key.endswith(".json"):
            continue

        # Load the file from the configuration bucket
        contents = s3_client.get_object(Bucket=bucket, Key=file_key)['Body'].read()
        info = json.loads(contents)
        resource_stack_id = info['StackId']
        definitions = info['Definitions']
        path_info = ResourceTypesPathInfo(file_key)

        # Add resource type definitions from the file to the dictionary
        for resource_type_name, data in definitions.iteritems():
            destination[resource_type_name] = ResourceTypeInfo(resource_stack_id, path_info.resource_name,
                                                               resource_type_name, data)


def load_resource_type_mapping(bucket, stack, s3_client=None):
    if not s3_client:
        s3_client = s3

    result = {}
    ancestry = [x.stack_name for x in stack.ancestry]

    # We are going to build a dictionary of ResourceTypeInfo objects, based on ResourceType definitions coming from the
    # CloudFormation template. This means combining all resource type definitions from all of the resource groups for
    # this deployment.
    #
    # ResourceTypesResourceHandler should have enforced that there are no naming collisions, so we don't need to check
    # that here. Types from the deployment are allowed to override types defined at the project level, though.
    #
    # Start by loading the core types from the project. Use a delimiter to prevent loading descendant directories.
    _load_mappings_for_prefix(
        destination=result,
        bucket=bucket,
        prefix=aws_utils.s3_key_join(constant.RESOURCE_DEFINITIONS_PATH, ancestry[0], ""),
        delimiter=constant.S3_DELIMETER,
        s3_client=s3_client
    )

    # Now if this stack is or belongs to a deployment, load all of the descendant types for the deployment.
    if len(ancestry) > 1:
        _load_mappings_for_prefix(
            destination=result,
            bucket=bucket,
            prefix=aws_utils.s3_key_join(constant.RESOURCE_DEFINITIONS_PATH, ancestry[0], ancestry[1], ""),
            delimiter="",  # No delimiter so we get all nested children
            s3_client=s3_client
        )

    return result
