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

import itertools
import json
import mock
import sys
import unittest
import urllib

from cgf_utils.test import mock_aws

class UnitTest_CloudGemFramework_ServiceLambda_service_directory(unittest.TestCase):

    TEST_REQUEST = {}

    TEST_BUCKET_NAME = 'test-bucket'

    TEST_DEPLOYMENT_NAME = 'test-deployment'

    TEST_COMPATIBLE_INTERFACE_MAJOR_VERSION = '1_'
    TEST_INCOMPATIBLE_INTERFACE_MAJOR_VERSION = '2_'
    TEST_BASE_INTERFACE_VERSION = TEST_COMPATIBLE_INTERFACE_MAJOR_VERSION + '0_0'
    TEST_COMPATIBLE_INTERFACE_VERSION = TEST_COMPATIBLE_INTERFACE_MAJOR_VERSION + '1_0'
    TEST_INCOMPATIBLE_INTERFACE_VERSION = TEST_INCOMPATIBLE_INTERFACE_MAJOR_VERSION + '0_0'

    TEST_INTERFACE_VERSIONS = [ 
        TEST_BASE_INTERFACE_VERSION, 
        TEST_COMPATIBLE_INTERFACE_VERSION, 
        TEST_INCOMPATIBLE_INTERFACE_VERSION 
    ]

    def TEST_INTERFACE_ID(self, version):
       return 'Test_Interface_' + version

    def TEST_INTERFACE_PATH(self, version):
        return '/common/interface_' + version

    TEST_SCHEME = 'https'

    HOST_A = 'a'
    HOST_B = 'b'

    TEST_HOSTS = ['a', 'b']

    def TEST_HOST(self, host):
       return 'test-{}.com'.format(host)

    def TEST_SERVICE_URL(self, host):
        return self.TEST_SCHEME + '://' + self.TEST_HOST(host)

    def TEST_SERVICE_URL_ENCODED(self, host):
        return urllib.quote_plus(self.TEST_SERVICE_URL(host))

    def TEST_INTERFACE_URL(self, host, version):
        return self.TEST_SERVICE_URL(host) + self.TEST_INTERFACE_PATH(version)

    def TEST_INTERFACE_URL_ENCODED(self, host, version):
        return urllib.quote_plus(self.TEST_INTERFACE_URL(host, version))

    def TEST_INTERFACE_SWAGGER(self, host, version):
        return {
            'schemes': [self.TEST_SCHEME],
            'host': self.TEST_HOST(host),
            'basePath' : self.TEST_INTERFACE_PATH(version) 
        }

    def TEST_INTERFACE_SWAGGER_STRING(self, host, version):
        return json.dumps(
            self.TEST_INTERFACE_SWAGGER(host, version), 
            indent = 4, 
            sort_keys = True
        )


    def setUp(self):
        old_module = sys.modules.pop('api.service_directory', None)
        print 'removed old module', old_module

    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'put_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_put_service_interfaces_with_no_existing_keys(self, mock_get_setting, mock_put_object, mock_list_objects_v2):

        import api.service_directory as target
        # reload(target)

        # no keys in bucket
        mock_list_objects_v2.return_value = {
            'Contents': []
        }

        # put intefaces for host a
        target.put_service_interfaces(
            self.TEST_REQUEST, 
            self.TEST_DEPLOYMENT_NAME, 
            self.TEST_SERVICE_URL(self.HOST_A), 
            {
                "Items": [ 
                    { 
                        'InterfaceId': self.TEST_INTERFACE_ID(version), 
                        'InterfaceUrl': self.TEST_INTERFACE_URL(self.HOST_A, version),
                        'InterfaceSwagger': self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version) 
                    } for version in self.TEST_INTERFACE_VERSIONS
                ]
            }
        )

        # deployment name used when listing bucket contents?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_PREFIX.format(deployment_name = self.TEST_DEPLOYMENT_NAME)
        )

        # correct object added to bucket for each interface?
        for version in self.TEST_INTERFACE_VERSIONS:
            mock_put_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                ),
                Body = self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version)
            )

        # bucket name loaded from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'delete_object')
    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'put_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_put_service_interfaces_with_other_service_keys(self, mock_get_setting, mock_put_object, mock_list_objects_v2, mock_delete_object):

        import api.service_directory as target
        # reload(target)

        # bucket contains keys for host b only 
        mock_list_objects_v2.return_value = {
            'Contents': [
                {
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_B, version)
                    ) 
                } for version in self.TEST_INTERFACE_VERSIONS
            ]
        }

        # put interfaces for host a
        target.put_service_interfaces(
            self.TEST_REQUEST, 
            self.TEST_DEPLOYMENT_NAME, 
            self.TEST_SERVICE_URL(self.HOST_A), 
            {
                "Items": [ 
                    { 
                        'InterfaceId': self.TEST_INTERFACE_ID(version), 
                        'InterfaceUrl': self.TEST_INTERFACE_URL(self.HOST_A, version),
                        'InterfaceSwagger': self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version) 
                    } for version in self.TEST_INTERFACE_VERSIONS
                ]
            }
        )

        # deployment name used when listing bucket contents?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_PREFIX.format(deployment_name = self.TEST_DEPLOYMENT_NAME)
        )

        # correct object added to bucket for each interface?
        for version in self.TEST_INTERFACE_VERSIONS:
            mock_put_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                ),
                Body = self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version)
            )

        # nothing was deleted from the bucket?
        mock_delete_object.assert_not_called()

        # bucket name loaded from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'delete_object')
    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'put_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_put_service_interfaces_with_existing_keys(self, mock_get_setting, mock_put_object, mock_list_objects_v2, mock_delete_object):

        import api.service_directory as target
        # reload(target)

        extra_versions = ['8_0_0', '9_0_0']
        all_versions = [ i for i in itertools.chain( self.TEST_INTERFACE_VERSIONS, extra_versions ) ]

        # add keys for test interfaces and some extra interfaces for host a
        mock_list_objects_v2.return_value = {
            'Contents': [
                {
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                    ) 
                } for version in all_versions
            ]
        }

        # put the test intefaces for host a
        target.put_service_interfaces(
            self.TEST_REQUEST, 
            self.TEST_DEPLOYMENT_NAME, 
            self.TEST_SERVICE_URL(self.HOST_A), 
            {
                "Items": [ 
                    { 
                        'InterfaceId': self.TEST_INTERFACE_ID(version), 
                        'InterfaceUrl': self.TEST_INTERFACE_URL(self.HOST_A, version),
                        'InterfaceSwagger': self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version) 
                    } for version in self.TEST_INTERFACE_VERSIONS
                ]
            }
        )

        # deployment name used when listing bucket contents?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_PREFIX.format(deployment_name = self.TEST_DEPLOYMENT_NAME)
        )

        # correct object added to bucket for each test interface?
        for version in self.TEST_INTERFACE_VERSIONS:
            mock_put_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                ),
                Body = self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version)
            )

        # correct object deleted from bucket for each extra interface?
        for version in extra_versions:
            mock_delete_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                )
            )

        # bucket name loaded from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'get_object')
    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_get_service_interfaces(self, mock_get_setting, mock_list_objects_v2, mock_get_object):

        import api.service_directory as target
        # reload(target)

        # create interface objects for both hosts a and b
        object_map = {
            target.ServiceDirectory.S3_INTERFACE_KEY.format(
                deployment_name = self.TEST_DEPLOYMENT_NAME,
                interface_id = self.TEST_INTERFACE_ID(version),
                interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
            ) : mock_aws.s3_get_object_response(self.TEST_INTERFACE_SWAGGER_STRING(host, version))
            for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
        }
        mock_list_objects_v2.return_value = { 'Contents': [ { 'Key': key } for key in object_map.keys() ] }
        mock_get_object.side_effect = lambda **kwargs: object_map[kwargs['Key']]

        # request interfaces for host a
        result = target.get_service_interfaces(
            self.TEST_REQUEST, 
            self.TEST_DEPLOYMENT_NAME, 
            self.TEST_SERVICE_URL(self.HOST_A)
        )

        # got interfaces for host a and only host a?
        self.assertEquals(result,
            {
                "Items": AnyOrderListMatcher(
                    [ 
                        { 
                            'InterfaceId': self.TEST_INTERFACE_ID(version), 
                            'InterfaceUrl': self.TEST_INTERFACE_URL(self.HOST_A, version),
                            'InterfaceSwagger': self.TEST_INTERFACE_SWAGGER_STRING(self.HOST_A, version) 
                        } for version in self.TEST_INTERFACE_VERSIONS
                    ]
                )
            }
        )

        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_PREFIX.format(deployment_name = self.TEST_DEPLOYMENT_NAME)
        )

        # requested correct interface objects?
        for version in self.TEST_INTERFACE_VERSIONS:
            mock_get_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                )
            )
        self.assertEquals(mock_get_object.call_count, len(self.TEST_INTERFACE_VERSIONS))

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'get_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_get_interface_services_with_base_interface(self, mock_get_setting, mock_get_object, mock_list_objects_v2):

        import api.service_directory as target
        # reload(target)

        # add keys for all interfaces all hosts

        mock_list_objects_v2.return_value = { 
            'Contents': [ 
                { 
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    ) 
                } for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
            ]
        }

        expected_interface_versions = [self.TEST_BASE_INTERFACE_VERSION, self.TEST_COMPATIBLE_INTERFACE_VERSION]

        mock_get_object.side_effect = [ 
             mock_aws.s3_get_object_response(self.TEST_INTERFACE_SWAGGER_STRING(host, version))
                 for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
        ]

        # request list for base interface version
        result = target.get_interface_services(
            self.TEST_REQUEST,
            self.TEST_DEPLOYMENT_NAME,
            self.TEST_INTERFACE_ID(self.TEST_BASE_INTERFACE_VERSION)
        )

        # verify list includes base and compatible version for both hosts
        
        expected = {
            "Items": AnyOrderListMatcher(
                [
                    {
                        "InterfaceId": self.TEST_INTERFACE_ID(version),
                        "InterfaceUrl": self.TEST_INTERFACE_URL(host, version),
                        "InterfaceSwagger": self.TEST_INTERFACE_SWAGGER_STRING(host, version)
                    } for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
                ]
            )
        }

        self.assertEquals(result, expected)

        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_ID_PREFIX.format(
                deployment_name = self.TEST_DEPLOYMENT_NAME,
                interface_id = self.TEST_INTERFACE_ID(self.TEST_COMPATIBLE_INTERFACE_MAJOR_VERSION)
            )
        )

        # got swagger content from s3?
        mock_get_object.assert_has_calls(
            [
                mock.call(
                    Bucket = self.TEST_BUCKET_NAME, 
                    Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    )
                )
                for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions )
            ]
        )

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'get_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_get_interface_services_with_compatible_interface(self, mock_get_setting, mock_get_object, mock_list_objects_v2):

        import api.service_directory as target
        # reload(target)

        # add keys for all interfaces all hosts
        mock_list_objects_v2.return_value = { 
            'Contents': [ 
                { 
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    ) 
                }  for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
            ]
        }

        expected_interface_versions = [self.TEST_COMPATIBLE_INTERFACE_VERSION]

        mock_get_object.side_effect = [ 
             mock_aws.s3_get_object_response(self.TEST_INTERFACE_SWAGGER_STRING(host, version))
                 for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
        ]

        # request list for base interface version
        result = target.get_interface_services(
            self.TEST_REQUEST,
            self.TEST_DEPLOYMENT_NAME,
            self.TEST_INTERFACE_ID(self.TEST_COMPATIBLE_INTERFACE_VERSION)
        )

        # verify list intcludes compatible version for both hosts
        
        expected = {
            "Items": AnyOrderListMatcher(
                [
                    {
                        "InterfaceId": self.TEST_INTERFACE_ID(version),
                        "InterfaceUrl": self.TEST_INTERFACE_URL(host, version),
                        "InterfaceSwagger": self.TEST_INTERFACE_SWAGGER_STRING(host, version)
                    } for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
                ]
            )
        }

        self.assertEquals(result, expected)

        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_ID_PREFIX.format(
                deployment_name = self.TEST_DEPLOYMENT_NAME,
                interface_id = self.TEST_INTERFACE_ID(self.TEST_COMPATIBLE_INTERFACE_MAJOR_VERSION)
            )
        )

        # got swagger content from s3?
        mock_get_object.assert_has_calls(
            [
                mock.call(
                    Bucket = self.TEST_BUCKET_NAME, 
                    Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    )
                )
                for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions )
            ]
        )

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')



    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock_aws.patch_client('s3', 'get_object')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_get_interface_services_with_incompatible_interface(self, mock_get_setting, mock_get_object, mock_list_objects_v2):

        import api.service_directory as target
        # reload(target)

        # add keys for all interfaces all hosts
        mock_list_objects_v2.return_value = { 
            'Contents': [ 
                { 
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    ) 
                }  for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
            ]
        }

        expected_interface_versions = [self.TEST_INCOMPATIBLE_INTERFACE_VERSION]

        mock_get_object.side_effect = [ 
             mock_aws.s3_get_object_response(self.TEST_INTERFACE_SWAGGER_STRING(host, version))
                 for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
        ]

        # request list for incompatible interface version
        result = target.get_interface_services(
            self.TEST_REQUEST,
            self.TEST_DEPLOYMENT_NAME,
            self.TEST_INTERFACE_ID(self.TEST_INCOMPATIBLE_INTERFACE_VERSION)
        )

        # verify list includes incompatible version for both hosts
        
        expected = {
            "Items": AnyOrderListMatcher(
                [
                    {
                        "InterfaceId": self.TEST_INTERFACE_ID(version),
                        "InterfaceUrl": self.TEST_INTERFACE_URL(host, version),
                        "InterfaceSwagger": self.TEST_INTERFACE_SWAGGER_STRING(host, version)
                    } for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions)
                ]
            )
        }

        self.assertEquals(result, expected)


        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_ID_PREFIX.format(
                deployment_name = self.TEST_DEPLOYMENT_NAME,
                interface_id = self.TEST_INTERFACE_ID(self.TEST_INCOMPATIBLE_INTERFACE_MAJOR_VERSION)
            )
        )

        # got swagger content from s3?
        mock_get_object.assert_has_calls(
            [
                mock.call(
                    Bucket = self.TEST_BUCKET_NAME, 
                    Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    )
                )
                for host, version in itertools.product(self.TEST_HOSTS, expected_interface_versions )
            ]
        )

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_get_interface_services_with_no_services(self, mock_get_setting, mock_list_objects_v2):

        import api.service_directory as target
        # reload(target)

        other_interface_prefix = 'other_interface_1_'
        other_interface_id = other_interface_prefix + '2_3'

        # add keys for all interfaces all hosts
        mock_list_objects_v2.return_value = { 
            'Contents': [ 
                { 
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    ) 
                }  for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
            ]
        }

        # request list for incompatible interface version
        result = target.get_interface_services(
            self.TEST_REQUEST,
            self.TEST_DEPLOYMENT_NAME,
            other_interface_id
        )

        # verify list includes incompatible version for both hosts
        
        expected = {
            "Items": AnyOrderListMatcher(
                [
                ]
            )
        }

        self.assertEquals(result, expected)

        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_ID_PREFIX.format(
                deployment_name = self.TEST_DEPLOYMENT_NAME,
                interface_id = other_interface_prefix
            )
        )

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


    @mock_aws.patch_client('s3', 'delete_object')
    @mock_aws.patch_client('s3', 'list_objects_v2')
    @mock.patch('CloudCanvas.get_setting', return_value = TEST_BUCKET_NAME)
    def test_delete_service_interfaces(self, mock_get_setting, mock_list_objects_v2, mock_delete_object):

        import api.service_directory as target
        # reload(target)
       
        # add keys for all interfaces all hosts
        mock_list_objects_v2.return_value = { 
            'Contents': [ 
                { 
                    'Key': target.ServiceDirectory.S3_INTERFACE_KEY.format(
                        deployment_name = self.TEST_DEPLOYMENT_NAME,
                        interface_id = self.TEST_INTERFACE_ID(version),
                        interface_url = self.TEST_INTERFACE_URL_ENCODED(host, version)
                    ) 
                }  for host, version in itertools.product(self.TEST_HOSTS, self.TEST_INTERFACE_VERSIONS)
            ]
        }

        target.delete_service_interfaces(
            self.TEST_REQUEST,
            self.TEST_DEPLOYMENT_NAME,
            self.TEST_SERVICE_URL(self.HOST_A)
        )

        # used deployment name when listing objects?
        mock_list_objects_v2.assert_called_once_with(
            Bucket = self.TEST_BUCKET_NAME,
            Prefix = target.ServiceDirectory.S3_INTERFACE_PREFIX.format(deployment_name = self.TEST_DEPLOYMENT_NAME)
        )

        # correct object deleted from bucket for each extra interface?
        for version in self.TEST_INTERFACE_VERSIONS:
            mock_delete_object.assert_any_call(
                Bucket = self.TEST_BUCKET_NAME, 
                Key = target.ServiceDirectory.S3_INTERFACE_KEY.format(
                    deployment_name = self.TEST_DEPLOYMENT_NAME,
                    interface_id = self.TEST_INTERFACE_ID(version),
                    interface_url = self.TEST_INTERFACE_URL_ENCODED(self.HOST_A, version)
                )
            )
        self.assertEquals(mock_delete_object.call_count, len(self.TEST_INTERFACE_VERSIONS))

        # loaded bucket name from settings?
        mock_get_setting.assert_called_once_with('ProjectConfigurationBucket')


class AnyOrderListMatcher(object):
    
    def __init__(self, expected):
        self.__expected = expected

    def __eq__(self, other):
        
        if len(self.__expected) != len(other):
            return False

        for expected_index, expected_item in enumerate(self.__expected):
            found_index = -1
            for other_index, other_item in enumerate(other):
                if other_item == expected_item:
                    if found_index != -1:
                        print 'Found multiple matches for index {} at index {} and index {}.'.format(expected_index, found_index, other_index)
                        return False # more than one item matched
                    found_index = other_index
            if found_index == -1:
                print 'Found no match for index {}. Expected to find {} in {}.'.format(expected_index, expected_item, other)
                return False

        return True

    def __str__(self):
        return self.__expected.__str__()

    def __repr__(self):
        return self.__expected.__repr__()

if __name__ == '__main__':
    unittest.main()
