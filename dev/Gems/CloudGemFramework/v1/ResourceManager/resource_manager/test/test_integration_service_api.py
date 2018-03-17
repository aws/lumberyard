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
import subprocess
import os
import time
import urllib

from resource_manager.test import lmbr_aws_test_support
import resource_management

import cgf_service_client

class IntegrationTest_CloudGemFramework_ServiceApi(lmbr_aws_test_support.lmbr_aws_TestCase):

    CGP_CONTENT_FILE_NAME = "Foo.js"
    TEST_PLUGIN_NAME = 'TestPlugin'
    TEST_EMPTY_PLUGIN_NAME = 'TestEmptyPlugin'
    TEST_PLUGIN_GEM_NAME = 'TestPluginCloudGem'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_ServiceApi, self).__init__(*args, **kwargs)

    def setUp(self):        
        self.prepare_test_envionment("cloud_gem_framework_service_api_test")

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self): 
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_RESOURCE_GROUP_NAME, '--initial-content', 'no-resources', '--enable')
        self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

    def __010_add_service_api_resources(self):
        self.lmbr_aws('cloud-gem-framework', 'add-service-api-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME)

    def __020_create_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

    def __030_verify_service_api_resources(self):

        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    resource_management.LAMBDA_CONFIGURATION_RESOURCE_NAME: {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    resource_management.LAMBDA_FUNCTION_RESOURCE_NAME: {
                        'ResourceType': 'AWS::Lambda::Function'
                    },
                    resource_management.API_RESOURCE_NAME: {
                        'ResourceType': 'Custom::ServiceApi'
                    },
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })

        # ServiceApi not player accessible (yet), so should be no mapping
        expected_logical_ids = []
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids)


    def __040_call_simple_api(self):

        client = self.get_service_client()

        result = client.navigate('service', 'status').GET()

        self.assertEqual(result.status, 'online')


    def __050_call_simple_api_with_no_credentials(self):
        
        client = self.get_service_client(use_aws_auth=False)
        
        with self.assertRaises(cgf_service_client.NotAllowedError):
            client.navigate('service', 'status').GET()


    def __100_add_complex_apis(self):
        self.__add_complex_api('string')
        self.__add_complex_api('number')
        self.__add_complex_api('boolean')
        self.__enable_complex_api_player_access('string')


    def __110_add_apis_with_optional_parameters(self):

        with_defaults = True
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)

        with_defaults = False
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)


    def __120_add_interfaces(self):

        # create gem that will define interface
        self.lmbr_aws('cloud-gem', 'create', '--initial-content', 'no-resources', '--gem', self.INTERFACE_DEFINER_GEM_NAME, '--enable')

        # create gem that will call interface
        self.lmbr_aws('cloud-gem', 'create', '--initial-content', 'lambda', '--gem', self.INTERFACE_CALLER_GEM_NAME, '--enable')

        # add interface api definition to definer gem
        self.make_gem_aws_json(self.INTERFACE_SWAGGER, self.INTERFACE_DEFINER_GEM_NAME, 'api-definition', self.INTERFACE_NAME + '.json')

        # add interface implementation to test resource group's swagger
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json') as swagger:
            swagger['paths']['/parent'] = {
                'x-cloud-gem-framework-interface-implementation': {
                    'interface': self.INTERFACE_ID
                }
            }

        # add interface implemenation to test resource group's service lambda
        self.make_gem_aws_file(self.INTERFACE_IMPLEMENTATION_LAMBDA_CODE, self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api', 'parent_child.py')

        # replace caller gem's lambda code
        self.make_gem_aws_file(self.INTERFACE_CALLER_LAMBDA_CODE, self.INTERFACE_CALLER_GEM_NAME, 'lambda-code', 'Lambda', 'main.py')

        # replace caller gem's lambda imports
        self.make_gem_aws_file(self.INTERFACE_CALLER_IMPORTS, self.INTERFACE_CALLER_GEM_NAME, 'lambda-code', 'Lambda', '.import')

        # add service configuration to caller gem's resource temmplate
        with self.edit_gem_aws_json(self.INTERFACE_CALLER_GEM_NAME, 'resource-template.json') as template:
            template['Resources']['LambdaConfiguration']['Properties']['Services'] = [
                {
                    "InterfaceId": self.INTERFACE_ID
                }
            ]


    def __130_add_legacy_plugin(self):

        self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_PLUGIN_GEM_NAME, '--initial-content', 'no-resources', '--enable')

        # add imports to test resource group
        with open(self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', '.import'), 'a') as file:
            file.write('*.{}\n'.format(self.TEST_PLUGIN_NAME))
            file.write('*.{}\n'.format(self.TEST_EMPTY_PLUGIN_NAME))

        # add legacy plugin format code to common-code in test resource group
        self.make_gem_aws_file(
            'def get_name():\n    return("{}")'.format(self.TEST_RESOURCE_GROUP_NAME),
            self.TEST_RESOURCE_GROUP_NAME, 'common-code', self.TEST_PLUGIN_NAME,
            '{}__{}.py'.format(self.TEST_PLUGIN_NAME, self.TEST_RESOURCE_GROUP_NAME)
        )

        # add legacy plugin format code to common-code in test resource group
        self.make_gem_aws_file(
            'def get_name():\n    return("{}")'.format(self.TEST_PLUGIN_GEM_NAME),
            self.TEST_PLUGIN_GEM_NAME, 'common-code', self.TEST_PLUGIN_NAME,
            '{}__{}.py'.format(self.TEST_PLUGIN_NAME, self.TEST_PLUGIN_GEM_NAME)
        )

        # add plugin imports
        gem_names = sorted([self.TEST_PLUGIN_GEM_NAME, self.TEST_RESOURCE_GROUP_NAME])
        self.make_gem_aws_file(
            self.TEST_IMPORT_CODE.format(*gem_names),
            self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api',
            'test_plugin.py'
        )

        # add plugin swagger
        with self.edit_gem_aws_json(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json') as swagger:
            swagger['paths']['/test/plugin'] = {
                'get': {
                    "responses": {
                        "200": {
                            "description": "A successful response.",
                            "schema": {
                                "$ref": "#/definitions/PluginResult"
                            }
                        }
                    },
                    'parameters': [
                    ]
                }
            }

            swagger['definitions']['PluginResult'] = {
                "type": "object",
                "properties": {
                    "result": {
                        "type": "string"
                    }
                }
            }


    def __200_update_deployment(self):
        self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')
        time.sleep(30) # seems as if API Gateway can take a few seconds to update


    def __210_verify_service_api_mapping(self):
        self.refresh_stack_resources(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME));
        expected_logical_ids = [ 
            self.TEST_RESOURCE_GROUP_NAME + '.ServiceApi', 
            self.INTERFACE_CALLER_GEM_NAME + '.Lambda'
        ]
        expected_service_url = self.get_service_url()
        expected_physical_resource_ids = { self.TEST_RESOURCE_GROUP_NAME + '.ServiceApi': expected_service_url }
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids, expected_physical_resource_ids = expected_physical_resource_ids)


    def __300_call_complex_apis(self):
        self.__call_complex_api('string', 'a', 'b', 'c')
        self.__call_complex_api('string', '"pathparam" \'string\'', '"queryparam" \'string\'', '"bodyparam" \'string\'')
        self.__call_complex_api('number', 42, 43, 44)
        self.__call_complex_api('boolean', True, False, True)


    def __301_call_complex_api_using_player_credentials(self):
        self.__call_complex_api('string', 'c', 'b', 'a', assumed_role='Player')
        self.__call_complex_api('number', 44, 43, 42, assumed_role='Player', expected_status_code=403)


    def __302_call_complex_api_using_project_admin_credentials(self):
        self.__call_complex_api('string', 'x', 'y', 'z', assumed_role='ProjectAdmin')
        self.__call_complex_api('number', 24, 34, 44, assumed_role='ProjectAdmin')


    def __303_call_complex_api_using_deployment_admin_credentials(self):
        self.__call_complex_api('string', 'z', 'y', 'z', assumed_role='DeploymentAdmin')
        self.__call_complex_api('number', 24, 23, 22, assumed_role='DeploymentAdmin')


    def __304_call_complex_api_with_missing_string_pathparam(self):
        self.__call_complex_api('string', None, 'b', 'c', expected_status_code=404)


    def __305_call_complex_api_with_missing_string_queryparam(self):
        self.__call_complex_api('string', 'a', None, 'c', expected_status_code=400)


    def __306_call_complex_api_with_missing_string_bodyparam(self):
        self.__call_complex_api('string', 'a', 'b', None, expected_status_code=400)


    def __307_call_api_with_both_parameters(self):
        self.__call_apis_with_optional_parameters(body_param = self.ACTUAL_BODY_PARAMETER_VALUE, query_param = self.ACTUAL_QUERY_PARAMETER_VALUE)


    def __308_call_api_without_bodyparam(self):
        self.__call_apis_with_optional_parameters(body_param = None, query_param = self.ACTUAL_QUERY_PARAMETER_VALUE)


    def __309_call_api_without_queryparam(self):
        self.__call_apis_with_optional_parameters(body_param = self.ACTUAL_BODY_PARAMETER_VALUE, query_param = None)


    def __400_call_test_plugin(self):
        client = self.get_service_client()
        result = client.navigate('test', 'plugin').GET()
        self.assertEqual(result.result, 'pass')


    def __500_call_interface_directly(self):
        client = self.get_service_client()
        body = {'test': 'data'}
        param = 'test'
        result = client.navigate('parent', 'child').POST(body, param = param)
        self.assertEquals(result, {'body': body, 'param': param})


    def __501_invoke_interface_caller(self):

        self.refresh_stack_resources(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME))

        body = {'test': 'data'}
        param = 'test'
        input = {'body': body, 'param': param}

        response = self.aws_lambda.invoke(
            FunctionName = self.get_stack_resource_physical_id(
                self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.INTERFACE_CALLER_GEM_NAME),
                'Lambda'
            ),
            Payload = json.dumps(input)
        )

        print 'response', response
        self.assertEquals(response['StatusCode'], 200)

        payload = response['Payload'].read()
        print 'payload', payload
        self.assertEquals(json.loads(payload.decode()), input)


    def __800_run_cpp_tests(self):

        if not os.environ.get("ENABLE_CLOUD_CANVAS_CPP_INTEGRATION_TESTS", None):
            print '\n*** SKIPPING cpp tests because the ENABLE_CLOUD_CANVAS_CPP_INTEGRATION_TESTS envionment variable is not set.\n'
        else:

            self.lmbr_aws('update-mappings', '--release')
        
            mappings_file = os.path.join(self.GAME_DIR, 'Config', self.TEST_DEPLOYMENT_NAME + '.player.awsLogicalMappings.json')
            print 'Using mappings from', mappings_file
            os.environ["cc_override_resource_map"] = mappings_file

            lmbr_test_cmd = os.path.join(self.REAL_ROOT_DIR, 'lmbr_test.cmd')
            args =[
                lmbr_test_cmd, 
                'scan', 
                # '--wait-for-debugger', # uncomment if the tests are failing and you want to debug them.
                '--include-gems', 
                '--integ', 
                '--only', 'Gem.CloudGemFramework.6fc787a982184217a5a553ca24676cfa.v0.1.0.dll', 
                '--dir', os.environ.get("TEST_BUILD_DIR", "Bin64vc140.Debug.Test")
            ]
            print 'EXECUTING', ' '.join(args)

            result = subprocess.call(args, shell=True)

            # Currently lmbr_test blows up when exiting. The tests all pass.
            # lbmr_test fails even if our tests do absolutely nothing, so it
            # seems to be an issue with the test infrastructure itself.

            self.assertEquals(result, 0)

    def __900_remove_service_api_resources(self):
        self.lmbr_aws('cloud-gem-framework', 'remove-service-api-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME)

    def __910_delete_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion')
        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })
        expected_logical_ids = [ self.INTERFACE_CALLER_GEM_NAME + '.Lambda' ]
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids)

    def __999_cleanup(self):
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')

    def __add_complex_api(self, param_type):
        self.__add_complex_api_to_swagger(param_type)
        self.__add_complex_api_to_code(param_type)

    def __add_complex_api_to_swagger(self, param_type):

        swagger_file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json')
        with open(swagger_file_path, 'r') as file:
            swagger = json.load(file)

        paths = swagger['paths']
        paths['/test/complex/' + param_type + '/{pathparam}'] = {
            'post': {
                "responses": {
                    "200": {
                        "description": "A successful service status response.",
                        "schema": {
                            "$ref": "#/definitions/ComplexResult" + param_type.title()
                        }
                    }
                },
                'parameters': [
                    {
                        'in': 'path',
                        'name': 'pathparam',
                        'type': param_type,
                        'required': True
                    },
                    {
                        'in': 'body',
                        'name': 'bodyparam',
                        'schema': {
                            "$ref": "#/definitions/ComplexBody" + param_type.title()
                        },
                        'required': True
                    },
                    {
                        'in': 'query',
                        'name': 'queryparam',
                        'type': param_type,
                        'required': True
                    }
                ]
            }
        }

        definitions = swagger['definitions']
        definitions['ComplexBody' + param_type.title()] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": param_type
                }
            }, 
            "required": [
                "data"
            ]
        }
        definitions['ComplexResult' + param_type.title()] = {
            "properties": {
                'pathparam': {
                    "type": param_type
                },
                'queryparam': {
                    "type": param_type
                },
                'bodyparam': {
                    "$ref": "#/definitions/ComplexBody" + param_type.title()
                }
            },
            "required": [
                "pathparam",
                "queryparam",
                "bodyparam"
            ],
            "type": "object"
        }

        with open(swagger_file_path, 'w') as file:
            json.dump(swagger, file, indent=4)


    def __add_complex_api_to_code(self, param_type):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api', 'test_complex_{}.py'.format(param_type))
        with open(file_path, 'w') as file:
            file.write(self.COMPLEX_API_CODE)

    def __enable_complex_api_player_access(self, param_type):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'resource-template.json')
        with open(file_path, 'r') as file:
            template = json.load(file)

        service_api_metadata = template['Resources']['ServiceApi']['Metadata'] = {
            'CloudCanvas': {
                'Permissions': {
                    'AbstractRole': 'Player',
                    'Action': 'execute-api:*',
                    'ResourceSuffix': '/*/POST/test/complex/' + param_type + '/*'
                }
            }
        }

        with open(file_path, 'w') as file:
            json.dump(template, file, indent=4, sort_keys=True)

    def __call_complex_api(self, param_type, path_param, query_param, body_param, assumed_role=None, expected_status_code=200):
        
        expected = {}
        
        body = { 'data': body_param } if body_param is not None else None
        expected['bodyparam'] = body

        if path_param is not None:
            expected['pathparam'] = path_param
        else:
            path_param = ''

        if query_param is not None:
            expected['queryparam'] = query_param

        # Make strings unicode so they match, this is a python json thing
        # not something API Gateway or the Lambda Function is doing.
        expected = json.loads(json.dumps(expected)) 

        # 'encode' url parametrs in JSON format
        if param_type == 'boolean': 
            path_param = 'true' if path_param else 'false'
            query_param = 'true' if query_param else 'false'

        client = self.get_service_client(assumed_role = assumed_role)

        params = {}
        if query_param is not None:
            params['queryparam'] = str(query_param)

        try:
            result = client.navigate('test', 'complex', param_type, str(path_param)).POST(body, **params)
            self.assertTrue(expected_status_code, 200)
            self.assertEquals(expected, result)
        except cgf_service_client.HttpError as e:
            self.assertTrue(expected_status_code, e.code)


    COMPLEX_API_CODE = '''
import service
import errors

class TestErrorType(errors.ClientError):
    def __init__(self):
        super(TestErrorType, self).__init__('Test error message.')

@service.api
def post(request, pathparam, queryparam, bodyparam):
    if queryparam == 'test-query-param-failure':
        raise TestErrorType()
    else:
        return {
            'pathparam': pathparam,
            'queryparam': queryparam,
            'bodyparam': bodyparam
        }
'''

    DEFAULT_QUERY_PARAMETER_VALUE = "default_query_parameter_value"


    ACTUAL_BODY_PARAMETER_VALUE = {
        "data": "actual_body_paramter_value"
    }


    ACTUAL_QUERY_PARAMETER_VALUE = "actual_query_parameter_value"


    def __add_api_with_optional_parameters(self, with_defaults):
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)


    def __add_api_with_optional_parameters_to_swagger(self, with_defaults):

        swagger_file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json')
        with open(swagger_file_path, 'r') as file:
            swagger = json.load(file)

        bodyparam = {
            'in': 'body',
            'name': 'bodyparam',
            'schema': {
                "$ref": "#/definitions/OptionalRequest"
            },
            'required': False
        }

        queryparam = {
            'in': 'query',
            'name': 'queryparam',
            'type': 'string',
            'required': False
        }

        if with_defaults:
            queryparam['default'] = self.DEFAULT_QUERY_PARAMETER_VALUE

        paths = swagger['paths']
        paths['/test/optional/{}'.format(self.__with_defaults_postfix(with_defaults))] = {
            'post': {
                "responses": {
                    "200": {
                        "description": "A successful service status response.",
                        "schema": {
                            "$ref": "#/definitions/OptionalResult"
                        }
                    }
                },
                'parameters': [
                    bodyparam,
                    queryparam
                ]
            }
        }

        definitions = swagger['definitions']
        definitions['OptionalRequest'] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": "string"
                }
            }, 
            "required": [
                "data"
            ]
        }
        definitions['OptionalResult'] = {
            "properties": {
                'queryparam': {
                    "type": "string"
                },
                'bodyparam': {
                    "$ref": "#/definitions/OptionalRequest"
                }
            },
            "type": "object"
        }

        with open(swagger_file_path, 'w') as file:
            json.dump(swagger, file, indent=4)


    def __add_api_with_optional_parameters_to_code(self, with_defaults):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api', 'test_optional_{}.py'.format(self.__with_defaults_postfix(with_defaults)))
        with open(file_path, 'w') as file:
            file.write(self.OPTIONAL_API_CODE)


    def __with_defaults_postfix(self, with_defaults):
        return 'withdefaults' if with_defaults else 'withoutdefaults'


    def __call_apis_with_optional_parameters(self, body_param = None, query_param = None, expected_status_code = 200):
        self.__call_api_with_optional_parameters(with_defaults = True, body_param = body_param, query_param = query_param, expected_status_code = expected_status_code)
        self.__call_api_with_optional_parameters(with_defaults = False, body_param = body_param, query_param = query_param, expected_status_code = expected_status_code)


    def __call_api_with_optional_parameters(self, with_defaults = False, query_param = None, body_param = None, assumed_role=None, expected_status_code=200):
        
        expected = {}
        
        if body_param is not None:
            body = { 'data': body_param } 
            expected['bodyparam'] = body
        else:
            body = None

        if query_param is not None:
            expected['queryparam'] = query_param
        else:
            if with_defaults:
                expected['queryparam'] = self.DEFAULT_QUERY_PARAMETER_VALUE

        # Make strings unicode so they match, this is a python json thing
        # not something API Gateway or the Lambda Function is doing.
        expected = json.loads(json.dumps(expected)) 

        params = {}
        if query_param is not None:
            params['queryparam'] = str(query_param)

        client = self.get_service_client(assumed_role = assumed_role)
        
        try:
            result = client.navigate('test', 'optional', self.__with_defaults_postfix(with_defaults)).POST(body, **params)
            self.assertEquals(expected_status_code, 200)
            self.assertEquals(expected, result)
        except cgf_service_client.HttpError as e:
            self.assertEquals(e.code, expected_status_code)


    OPTIONAL_API_CODE = '''
import cgf_lambda_service
import errors

@cgf_lambda_service.api
def post(request, queryparam=None, bodyparam=None):
    result = {}
    if queryparam is not None:
        result['queryparam'] = queryparam
    if bodyparam is not None:
        result['bodyparam'] = bodyparam
    return result
'''

    INTERFACE_DEFINER_GEM_NAME = 'InterfaceDefininer'
    INTERFACE_CALLER_GEM_NAME = 'InterfaceCaller'
    
    INTERFACE_NAME = 'TestInterface_1_0_0'
    
    INTERFACE_ID = INTERFACE_DEFINER_GEM_NAME + '_' + INTERFACE_NAME

    INTERFACE_SWAGGER = {
        "swagger": "2.0",
        "info": {
            "version": "1.0.0",
            "title": ""
        },
        "paths": {
            "/child": {
                'post': {
                    "parameters": [
                        {
                            "name": "body",
                            "in": "body",
                            "required": True,
                            "schema": {
                                "$ref": "#/definitions/InterfaceRequest"
                            }
                        },
                        {
                            "name": "param",
                            "in": "query",
                            "required": True,
                            "type": "string"
                        }
                    ],
                    "responses": {
                        "200": {
                            "description": "A successful service status response.",
                            "schema": {
                                "$ref": "#/definitions/InterfaceResult"
                            }
                        }
                    }
                }
            }
        },
        "definitions": {
            "InterfaceResult": {
                "type": "object",
                "properties": {
                    "body": {
                        "type": "object"
                    },
                    "param": {
                        "type": "string"
                    }
                }, 
                "required": [
                    "body",
                    "param"
                ]
            },
            "InterfaceRequest": {
                "type": "object"
            }
        }
    }

    INTERFACE_IMPLEMENTATION_LAMBDA_CODE = '''
import cgf_lambda_service

@cgf_lambda_service.api
def post(request, body, param):
    return { 'body': body, 'param': param }
'''

    INTERFACE_CALLER_LAMBDA_CODE = '''
import cgf_lambda_settings
import cgf_service_client
import boto3

def handler(event, context):
    print 'event', event
    interface = cgf_lambda_settings.settings.Services.{INTERFACE_ID}
    print 'interface', interface
    service_client = cgf_service_client.for_url(interface.InterfaceUrl, verbose=True, session=boto3._get_default_session())
    result = service_client.navigate('child').POST(event['body'], param=event['param']);
    print 'result', result
    return result.DATA
'''.format(INTERFACE_ID = INTERFACE_ID)

    INTERFACE_CALLER_IMPORTS = '''CloudGemFramework.ServiceClient_Python
CloudGemFramework.LambdaSettings'''

    TEST_IMPORT_CODE = '''
import service

import TestPlugin
import TestEmptyPlugin

@service.api
def get(request):

    # NOTE: These asserts should be similar to the ones in test_unit_common_code_import.py
    # Importing should work the same locally in a workspace and in a Lambda.

    __assert(TestPlugin.__all__ == ['{0}', '{1}'], 'Both test modules should be listed in __all__ in sorted order')

    __assert(TestPlugin.{0}.get_name() == '{0}', 'First gem should be callable as a submodule')
    __assert(TestPlugin.{1}.get_name() == '{1}', 'Second gem should be callable as a submodule')

    __assert(TestPlugin.imported_modules, 'imported_modules should be defined')
    __assert(set(TestPlugin.imported_modules.keys()) == {{'{0}', '{1}'}}, 'Both test modules should be in imported_modules')
    __assert(TestPlugin.imported_modules['{0}'].get_name() == '{0}', 'First gem should be callable from imported_modules')
    __assert(TestPlugin.imported_modules['{1}'].get_name() == '{1}', 'Second gem should be callable from imported_modules')

    __assert(TestEmptyPlugin.__all__ == [], 'The empty plugin should have no imported modules in __all__')
    __assert(len(TestEmptyPlugin.imported_modules.keys()) == 0, 'The empty plugin imported_modules should be empty')

    return {{'result': 'pass'}}

def __assert(result, msg):
    if not result:
        raise AssertionError(msg)
'''

