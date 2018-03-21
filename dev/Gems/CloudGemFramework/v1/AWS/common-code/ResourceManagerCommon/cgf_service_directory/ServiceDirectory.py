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

# python
import json
import urllib
import urlparse

# boto3
import boto3
import botocore.exceptions

# ResoruceManagerCommon
from resource_manager_common import service_interface

# CloudGemFramework Utils
from cgf_utils import aws_utils

class ServiceDirectory(object):

    S3_INTERFACE_PREFIX = 'deployment/{deployment_name}/service-directory/interface/'
    S3_INTERFACE_ID_PREFIX = S3_INTERFACE_PREFIX + '{interface_id}'
    S3_INTERFACE_KEY = S3_INTERFACE_ID_PREFIX + '/service/{interface_url}'

    S3_PROJECT_INTERFACE_PREFIX = 'service-directory/interface/'
    S3_PROJECT_INTERFACE_ID_PREFIX = S3_PROJECT_INTERFACE_PREFIX + '{interface_id}'
    S3_PROJECT_INTERFACE_KEY = S3_PROJECT_INTERFACE_ID_PREFIX + '/service/{interface_url}'

    def __init__(self, bucket_name):
        self.__bucket_name = bucket_name
        self.__s3 = aws_utils.ClientWrapper(boto3.client('s3'))

    def put_service_interfaces(self, deployment_name, service_url, interfaces):
        # make a set of all existing keys
        existing_keys = set(self.__get_all_keys_for_deployment_or_project(deployment_name))

        # put all the interfaces provided by the service, removing their keys from the set
        for interface in interfaces:
            # delete all existing interfaces under the interface id
            self.__delete_all_interfaces_under_interface_id(deployment_name, interface.get('InterfaceId'))

            interface_key = self.__put_service_interface(
                deployment_name,
                service_url,
                interface
            )
            existing_keys.discard(interface_key)

        # delete all the interfaces that are no longer provided by the service
        encoded_service_url = urllib.quote_plus(service_url)
        for existing_key in existing_keys:
            existing_interface_id, existing_encoded_interface_url = self.__parse_s3_interface_key_for_deployment(existing_key) if deployment_name else self.__parse_s3_interface_key_for_project(existing_key)
            if existing_encoded_interface_url.startswith(encoded_service_url):
                self.__s3.delete_object(
                    Bucket = self.__bucket_name,
                    Key = existing_key
                )
    def __delete_all_interfaces_under_interface_id(self, deployment_name, inteface_id):
        if not inteface_id:
            return

        if deployment_name:
            prefix =  self.S3_INTERFACE_ID_PREFIX.format(
                deployment_name = deployment_name,
                interface_id = inteface_id
            )
        else:
            prefix =  self.S3_PROJECT_INTERFACE_ID_PREFIX.format(
                interface_id = inteface_id
            )

        res = self.__s3.list_objects_v2(
            Bucket = self.__bucket_name,
            Prefix = prefix
        )

        interface_keys = [ content['Key'] for content in res.get('Contents', []) ]

        for existing_key in interface_keys:
            self.__s3.delete_object(
                Bucket = self.__bucket_name,
                Key = existing_key
            )

    def __put_service_interface(self, deployment_name, service_url, interface):

        # validate interface id
        interface_id = interface.get('InterfaceId')
        if not interface_id or not isinstance(interface_id, basestring):
            raise ValueError('Missing InterfaceId string property in {}.'.format(interface))
        self.__validate_interface_id(interface_id)

        # validate swagger
        interface_swagger = interface.get('InterfaceSwagger')
        if not interface_swagger or not isinstance(interface_swagger, basestring):
            raise ValueError('Missing InterfaceSwagger string property in {}.'.format(interface))

        # validate url
        interface_url = interface.get('InterfaceUrl')
        if not interface_url or not isinstance(interface_url, basestring):
            raise ValueError('Missing Swagger string property in {}.'.format(interface))
        if not interface_url.startswith(service_url):
            raise ValueError('The interface url {} is not relative to the service url {}.'.format(interface_url, service_url))

        # write an object representing the interface to S3, it contains the swagger
        if deployment_name:
            interface_key = self.S3_INTERFACE_KEY.format(
                deployment_name = deployment_name,
                interface_id = interface_id,
                interface_url = urllib.quote_plus(interface_url)
            )
        else:
            interface_key = self.S3_PROJECT_INTERFACE_KEY.format(
                interface_id = interface_id,
                interface_url = urllib.quote_plus(interface_url)
            )

        self.__s3.put_object(
            Bucket = self.__bucket_name,
            Key = interface_key,
            Body = interface_swagger
        )

        # return the object's key
        return interface_key


    def get_service_interfaces(self, deployment_name, service_url):

        # get all the keys for the deployment
        keys = self.__get_all_keys_for_deployment_or_project(deployment_name)

        # find the keys that represent interfaces implemented by the service and read them
        interfaces = []
        encoded_service_url = urllib.quote_plus(service_url)
        for key in keys:
            interface_id, encoded_interface_url = self.__parse_s3_interface_key_for_deployment(key) if deployment_name else self.__parse_s3_interface_key_for_project(key)
            if encoded_interface_url.startswith(encoded_service_url):
                res = self.__s3.get_object(Bucket = self.__bucket_name, Key = key)
                interface_swagger = res['Body'].read()
                interface = {
                    'InterfaceId': interface_id,
                    'InterfaceSwagger': interface_swagger,
                    'InterfaceUrl': urllib.unquote_plus(encoded_interface_url)
                }
                interfaces.append(interface)

        # return the intefaces found
        return interfaces


    def delete_service_interfaces(self, deployment_name, service_url):

        # get all the keys for the deployment
        keys = self.__get_all_keys_for_deployment_or_project(deployment_name)

        # find the keys that represent interfaces implemented by the service and delete them
        encoded_service_url = urllib.quote_plus(service_url)
        for key in keys:
            interface_id, encoded_interface_url = self.__parse_s3_interface_key_for_deployment(key) if deployment_name else self.__parse_s3_interface_key_for_project(key)
            if encoded_interface_url.startswith(encoded_service_url):
                self.__s3.delete_object(Bucket = self.__bucket_name, Key = key)


    def get_interface_services(self, deployment_name, interface_id):
        # find keys representing interfaces with the requested interface's name and major version
        target_gem_name, target_interface_name, target_interface_version = self.__parse_interface_id(interface_id)
        target_interface_id_prefix = target_gem_name + '_' + target_interface_name + '_' + str(target_interface_version.major) + '_'

        if deployment_name:
            res = self.__s3.list_objects_v2(
                Bucket = self.__bucket_name,
                Prefix = self.S3_INTERFACE_ID_PREFIX.format(
                    deployment_name = deployment_name,
                    interface_id = target_interface_id_prefix
                )
            )
        else:
            res = self.__s3.list_objects_v2(
                Bucket = self.__bucket_name,
                Prefix = self.S3_PROJECT_INTERFACE_ID_PREFIX.format(
                    interface_id = target_interface_id_prefix
                )
            )

        # of those keys, find the ones where the minor version is compatible with the requested interface's version
        services = []
        for content in res.get('Contents', []):
            canidate_interface_id, encoded_interface_url = self.__parse_s3_interface_key_for_deployment(content['Key']) if deployment_name else self.__parse_s3_interface_key_for_project(content['Key'])
            canidate_gem_name, canidate_interface_name, canidate_interface_version = self.__parse_interface_id(canidate_interface_id)
            if canidate_interface_version.is_compatible_with(target_interface_version):
                res = self.__s3.get_object(Bucket = self.__bucket_name, Key = content['Key'])
                interface_swagger = res['Body'].read()
                services.append(
                    {
                        'InterfaceId': canidate_interface_id,
                        'InterfaceUrl': urllib.unquote_plus(encoded_interface_url),
                        'InterfaceSwagger': interface_swagger
                    }
                )

        # return the services found
        return services


    def __parse_s3_interface_key_for_deployment(self, key):
        parts = key.split('/') # expecting an S3_INTERFACE_SWAGGER_KEY as defined above.
        interface_id = parts[4]
        encoded_service_url = parts[6]
        return interface_id, encoded_service_url


    def __parse_s3_interface_key_for_project(self, key):
        parts = key.split('/') # expecting an S3_INTERFACE_SWAGGER_KEY as defined above.
        interface_id = parts[2]
        encoded_service_url = parts[4]
        return interface_id, encoded_service_url


    def __parse_interface_id(self, interface_id):
        try:
            return service_interface.parse_interface_id(interface_id)
        except Exception as e:
            raise ValueError(e.message)


    def __validate_interface_id(self, interface_id):
        self.__parse_interface_id(interface_id)


    def __get_all_keys_for_deployment_or_project(self, deployment_name):
        if deployment_name:
            res = self.__s3.list_objects_v2(
                Bucket = self.__bucket_name,
                Prefix = self.S3_INTERFACE_PREFIX.format(
                    deployment_name = deployment_name
                )
            )
        else:
            res = self.__s3.list_objects_v2(
                Bucket = self.__bucket_name,
                Prefix = self.S3_PROJECT_INTERFACE_PREFIX
            )

        return [ content['Key'] for content in res.get('Contents', []) ]
