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

# LambdaService
import service
import errors

# LambdaSettings
import CloudCanvas

# DirectoryService
from cgf_service_directory import ServiceDirectory

# Bucket used to store service data
bucket_name = CloudCanvas.get_setting('ProjectConfigurationBucket')
impl = ServiceDirectory(bucket_name)

@service.api
def put_service_interfaces(request, deployment_name, service_url, service_interfaces_body):

    items = service_interfaces_body.get('Items', None)
    if items is None:
        raise errors.ClientError('Missing required Items property in the request body.')

    try:
        impl.put_service_interfaces(deployment_name, service_url, items)
    except ValueError as e:
        raise errors.ClientError(e.message)


@service.api
def delete_service_interfaces(request, deployment_name, service_url):

    try:
        impl.delete_service_interfaces(deployment_name, service_url)
    except valueError as e:
        raise errors.ClientError(e.message)


@service.api
def get_service_interfaces(request, deployment_name, service_url):

    try:
        interfaces = impl.get_service_interfaces(deployment_name, service_url)
    except ValueError as e:
        raise errors.ClientError(e.message)

    return { 'Items': interfaces }


@service.api
def get_interface_services(request, deployment_name, interface_id):

    try:
        services = impl.get_interface_services(deployment_name, interface_id)
    except ValueError as e:
        raise errors.ClientError(e.message)

    return { 'Items': services }


@service.api
def get_interface_service_swagger(request, deployment_name, interface_id, interface_url):

    try:
        swagger_document = impl.get_interface_service_swagger(deployment_name, interface_id, interface_url)
    except ValueError as e:
        raise errors.ClientError(e.message)

    if not swagger_document:
        raise errors.NotFoundError()

    return swagger_document


